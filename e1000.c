/*
  File: e1000.c

  Author: Oscar Lowry

  Contributor:

  Description: e1000 network driver (writen with qemu in mind)

 */



#include "common.h"
#include "klib.h"

#include "pci.h"

#include "e1000.h"

#define E1000_TXDESC_NUM 16
#define ETHERNET_FRAME_SIZE_MAX 1500


typedef struct _e1000_tx_desc
{
  uint64 addr;
  uint16 length;
  uint8 cso;
  uint8 cmd;
  uint8 status; // Lower 4 bits are reserved 
  uint8 css;
  uint16 special;
} TXDesc;

typedef struct e1000 
{
    TXDesc tx_ring[E1000_TXDESC_NUM];
    uint32 mmio_base_addr;
    uint8 addr[6];
    // rings tx / rx
} E1000dev;



const uint8 ETHERNET_BROADCAST[ETHERNET_ADDR_LEN] = {"\xff\xff\xff\xff\xff\xff"};
const uint8 ETHERNET_PREAMBLE_SFD[8] = {"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xab"};
E1000dev _e1000_dev;

unsigned int _e1000_read_mmio(uint16 reg)
{
    return *(volatile uint32 *)(_e1000_dev.mmio_base_addr + reg);
}

void _e1000_write_mmio(uint16 reg, uint32 val)
{
    *(volatile uint32 *)(_e1000_dev.mmio_base_addr + reg) = val;
}

uint32 _e1000_get_mmio_base_addr(PCIDev* pci_dev)
{
    if(pci_dev->bar0 > 0xffff)
    { return pci_dev->bar0; }
    if(pci_dev->bar1 > 0xffff)
    { return pci_dev->bar1; }
    if(pci_dev->bar2 > 0xffff)
    { return pci_dev->bar2; }
    if(pci_dev->bar3 > 0xffff)
    { return pci_dev->bar3; }
    if(pci_dev->bar4 > 0xffff)
    { return pci_dev->bar4; }
    if(pci_dev->bar5 > 0xffff)
    { return pci_dev->bar5; }
    return 0;
}

uint16 _e1000_eeprom_read(uint8 addr)
{
    uint32 eerd;
    _e1000_write_mmio(E1000_EERD, E1000_EERD_READ | addr << E1000_EERD_ADDR);
    while (!((eerd = _e1000_read_mmio(E1000_EERD)) & E1000_EERD_DONE));
    return (uint16)(eerd >> E1000_EERD_DATA);
}


void _e1000_set_MAC( void )
{
    
    uint16 dat;
    for (int n = 0; n < 3; n++)
    {
        dat = _e1000_eeprom_read(n);
        _e1000_dev.addr[n*2] = dat & 0xff;
        _e1000_dev.addr[n*2+1] = (dat >> 8) & 0xff;
    }

}

void _e1000_dump_MAC( void )
{
    if(!(_e1000_dev.mmio_base_addr))
    {
        __cio_printf("\ne1000 uninitialized\n");
        return;
    }

    // TODO check for _e1000
    __cio_printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
            _e1000_dev.addr[0],
            _e1000_dev.addr[1], 
            _e1000_dev.addr[2], 
            _e1000_dev.addr[3],
            _e1000_dev.addr[4], 
            _e1000_dev.addr[5]);
        
}

void _e1000_rx_init(E1000dev* dev)
{
        
}

void _e1000_tx_init()
{
    // Clear ring
    // initialize tx descriptors
    for (int n = 0; n < E1000_TXDESC_NUM; n++) {
        __memclr(&(_e1000_dev.tx_ring[n]), sizeof(TXDesc));
    }
    // setup tx descriptors
    uint32 base = (uint32)(_e1000_dev.tx_ring);
    __cio_printf("\n%x\n", base);
    _e1000_write_mmio(E1000_TDBAL, base);
    _e1000_write_mmio(E1000_TDBAH, 0 );
    // tx descriptor length
    _e1000_write_mmio(E1000_TDLEN, 
            (uint32)(E1000_TXDESC_NUM * sizeof(TXDesc))
        );
    // setup head/tail
    _e1000_write_mmio(E1000_TDH, 0);
    _e1000_write_mmio(E1000_TDT, 0);
    // set tx control register
    _e1000_write_mmio(E1000_TCTL, (
        E1000_TCTL_EN  | // Enable transmit
        E1000_TCTL_PSP | // pad short packets
        0)
    );
}

int _e1000_tx(uint8 *data, int len)
{
    if(!(_e1000_dev.mmio_base_addr))
    {
        __cio_printf("\ne1000 uninitialized\n");
        return -1;
    }

    uint32 tail = _e1000_read_mmio(E1000_TDT);
    TXDesc *desc = &(_e1000_dev.tx_ring[tail]);
    desc->addr = data;
    desc->length = len;
    desc->status = 0;
    desc->cmd = (
            E1000_TXD_CMD_EOP | // End of packet
            E1000_TXD_CMD_RS  | // Set status bit when done
            E1000_TXD_CMD_IFCS);// Insert frame check sequence
    _e1000_write_mmio(E1000_TDT, (tail + 1) % E1000_TXDESC_NUM);
    uint32 counter = 0;
    while(!(desc->status & 0xff)) { 
        counter++ ; 
        if(counter == 0xfffff){
            __cio_printf("\nPacket Unsent\n"); 
            return -1;
        }
    }
    // unable to send
    __cio_printf("\nSent\n");
    return len;
}

int _e1000_ethernet_frame(uint8 *data, int len, uint16 type)
{
    __cio_printf("\n");
    for(int i = 0; i < len; i++)
    {
        __cio_printf("-%02x-", *(data + i));
    }
    uint8 frame[ETHERNET_FRAME_SIZE_MAX];
    __memcpy( frame, ETHERNET_BROADCAST, 6 );
    __memcpy( frame + 6, _e1000_dev.addr, 6 );
    *(frame + 12) = (type >> 8) & 0xff;
    *(frame + 13) = type & 0xff;
    __memcpy( frame + 14, data, len);

    return _e1000_tx(frame, len + 14);
}

int _e1000_init( void )
{

    // get e1000 dev from pci card
    PCIDev * net_ctrl = _pci_get_device_class( CLASS_NETCTL, CLASS_ETHRNT, 0 );
    if(net_ctrl->vendorid != E1000_VNDID && net_ctrl->deviceid != E1000_VNDID)
    {
        // alert if wrong kind of card
        __cio_printf("E1000 network device incompatible or missing\n");
        return 1;
    }

    _e1000_dev.mmio_base_addr = _e1000_get_mmio_base_addr(net_ctrl);
    if(!(_e1000_dev.mmio_base_addr))
    {
        __cio_printf("\nError initializing e1000 mmio");
        return -1;
    }
    // assert mmio not 0
    _e1000_set_MAC();
    _e1000_tx_init(&_e1000_dev);

    return 0;
}
