/*




*/

#define __SP_KERNEL__

#include "usbd.h"

#include "usb_ehci.h"
#include "klib.h"
#include "usb_util.h"

typedef struct _usb_qtd_s* USBPacket;

typedef struct _usb_transfer_s{
  struct _usb_qh_s* qh;
  uint8 endpoint;
  USBPacket first_packet;
} USBPipe;

typedef struct _usb_dev_s{
  uint8 deviceAddr;
  USBPipe pipes[USB_MAX_PIPES];

} USBDev;

USBDev _usb_devs[USB_MAX_DEVICES];
uint8 nextFreeDev = 1;

USBPipe* _usbd_new_transfer(uint8 addr, uint8 endpoint, uint8 speed, uint16 maxPacketSize);
USBPacket _usbd_new_packet(uint8 type, uint16 size);

void _usbd_add_packet_to_transfer(USBPipe* tran, USBPacket pack);

void _usbd_add_data_packet(USBPacket pack, void * data, uint16 size);

void _usbd_schedule_transfer(USBPipe * transfer);

void _usbd_print_transfer(USBPipe * tran);

void _usbd_set_ioc_packet(USBPacket pack);

void _usbd_get_data_packet(USBPacket pack, void * dst, uint16 size);

void _usbd_wait_for_packet(USBPacket pack);

void _usbd_enumerate_devices( void ) {

  if(_usb_ehci_has_port_change()) {

    __cio_printf("Enumerating USB\n");

    USBPipe * trans = _usbd_new_transfer(0, 0, USB_LOW_SPEED, 64); // New transfer to default device

    USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);

    uint8 data[8];
    usb_device_request_packet(data, 0x0, SET_ADDRESS, 1, 0x0, 0x0);
    _usbd_add_data_packet(setupPacket, data, 8);

    USBPacket outPacket = _usbd_new_packet(USB_OUT_PACKET, 0);

    _usbd_add_packet_to_transfer(trans, outPacket);
    _usbd_add_packet_to_transfer(trans, setupPacket);
   
    uint8 port = _usb_ehci_find_port_change();
    _usb_ehci_reset_port(port);

    _usbd_print_transfer(trans);
    _usbd_schedule_transfer(trans);

    __memclr(data, 8);

    USBPacket setup1Packet = _usbd_new_packet(USB_SETUP_PACKET, 8);

    usb_device_request_packet(data, 0x80, GET_DESCRIPTOR, usb_bytes_to_word(CONFIGURATION, 0x0), 0x0, 64);
    _usbd_add_data_packet(setup1Packet, data, 8);

    USBPacket in1Packet = _usbd_new_packet(USB_IN_PACKET, 64);

    USBPacket out1Packet = _usbd_new_packet(USB_OUT_PACKET, 0);

    _usbd_add_packet_to_transfer(trans, out1Packet);
    _usbd_add_packet_to_transfer(trans, in1Packet);
    _usbd_add_packet_to_transfer(trans, setup1Packet);

    _usbd_wait_for_packet(in1Packet);

    uint8 recBuff[64];
    _usbd_get_data_packet(in1Packet, recBuff, 64);

    for(int i=0;i<64;++i){
      __cio_printf(" %x", recBuff[i]);
    }

    __cio_printf("USB Enum done\n");

  }

}

void _usbd_schedule_transfer(USBPipe * transfer){
  _usb_ehci_schedule_async(transfer->qh);
}

USBPipe * _usbd_new_transfer(uint8 addr, uint8 endpoint, uint8 speed, uint16 maxPacketSize){
  USBPipe * trans = &(_usb_devs[addr].pipes[endpoint]);
  trans->qh = _usb_ehci_free_qh();
  trans->first_packet = 0x0;
  trans->qh->dword0 = 0x0;
  trans->qh->dword0 = (uint32)trans->qh | 0x2; // Set horziontal pointer

  trans->qh->dword1 = 0x0;
  trans->qh->dword1 |= (uint32) (addr & 0x7F);
  trans->qh->dword1 |= ((uint32) (endpoint & 0xF)) << 8;
  trans->qh->dword1 |= ((uint32)(speed & 0x3)) << 12;
  trans->qh->dword1 |= 0x4000; // Always is 1
  trans->qh->dword1 |= ((uint32)(maxPacketSize & 0x3FF)) << 16;

  trans->qh->dword2 = 0x40000000;
  trans->qh->dword3 = 0x0;
  trans->qh->dword4 = 0x1;
  trans->qh->dword5 = 0x1;
  trans->qh->dword6 = 0x0;
  trans->qh->dword7 = 0x0;
  trans->qh->dword8 = 0x0;
  trans->qh->dword9 = 0x0;
  trans->qh->dword10 = 0x0;
  trans->qh->dword11 = 0x0;

  return trans;
}

void _usbd_add_packet_to_transfer(USBPipe* tran, USBPacket pack) {
  tran->first_packet = pack;
  pack->next_ptr = tran->qh->dword4;
  tran->qh->dword4 = (uint32) pack;

  //while((pack->token & 0xF) == 0x80){
   // __cio_printf("Wait for packet");
  //}
}

USBPacket _usbd_new_packet(uint8 type, uint16 size) {
  struct _usb_qtd_s* pack = _usb_ehci_free_qtd();
  pack->next_ptr = 0x0;
  pack->alt_ptr = 0x1;
  pack->token = 0x0;

  if(type == USB_IN_PACKET || type == USB_OUT_PACKET){
    pack->token |= 0x80000000;
  }

  pack->token = pack->token | (uint32)type << 8; // Set PID flag
  pack->token = pack->token | (uint32)size << 16; // Set size field
  pack->token = pack->token | 0xC00; // Set CERR field
  pack->token = pack->token | 0x80; // Set status field to enable

  if(size > 0) {

    uint8 num_pages = (size % PAGE_SIZE) + 1;
    uint32 * buff = (uint32 *)_kalloc_page(num_pages);
    __memclr(buff, num_pages * PAGE_SIZE);

    pack->ptr0 = (uint32)buff;
    //TODO: Fix the buffers list
  }else{
    pack->ptr0 = 0x0;
  }

  pack->ptr1 = 0x0;
  pack->ptr2 = 0x0;
  pack->ptr3 = 0x0;
  pack->ptr4 = 0x0;

  return pack;
}

void _usbd_set_ioc_packet(USBPacket pack) {
  pack->token = pack->token | 0x8000;
}

void _usbd_print_transfer(USBPipe * tran) {
  __cio_printf("USB Transfer Dump\n");
  __cio_printf("QH %x: HPtr: %x Token: %x EPCh: %x ", tran->qh, tran->qh->dword0, tran->qh->dword1, tran->qh->dword2);
  __cio_printf("cQTD: %x nQTD: %x\n", tran->qh->dword3, tran->qh->dword4);
  struct _usb_qtd_s* cqtd = tran->first_packet;
  uint32 cqtda = (uint32)cqtd;
  int cqtdN = 1;
  while((cqtda & 0x1) != 0x1 ) {
    uint16 packSize = (cqtd->token & 0x7FFF0000) >> 16;
    __cio_printf("qTD%d %x: Next: %x Token: %x Transfer Size: %d CPage: %d PID: %x Status: %x",
                 cqtdN, cqtd, cqtd->next_ptr, cqtd->token, packSize,
                 (cqtd->token & 0x7000) >> 12, (cqtd->token & 0x300) >> 8, cqtd->token & 0xFF);
    __cio_printf(" Buffptr 0: %x \n", cqtd->ptr0);

    cqtda = cqtd->next_ptr;
    cqtd = (struct _usb_qtd_s*)cqtd->next_ptr;
    cqtdN += 1;
  }

}

void _usbd_add_data_packet(USBPacket pack, void * data, uint16 size) {
  __memcpy((void*)(pack->ptr0 & 0xFFFFF000), data, size);
}

void _usbd_get_data_packet(USBPacket pack, void * dst, uint16 size) {
  void * src = (void *)(pack->ptr0 & 0xFFFFF000);
  __memcpy(dst, src, size);
}

void _usbd_wait_for_packet(USBPacket pack) {
  while((pack->token & 0xFF) == 0x80){
    //__cio_printf("Wait for packet");
    // Wait for the packet
  }
}
