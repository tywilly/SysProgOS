#define __SP_KERNEL__

#include "common.h"

#include "usb.h"

#define USB_MAX_FRLIST      1024
#define USB_MAX_QTD         512
// USB class, subclass, EHCI progIF
#define USB_CLASS           0xC
#define USB_SUBCLASS        0x3
#define USB_EHCI_PROGIF     0x20
// Capability registers offsets
#define USB_CAP_LEN         0       // Capability Register Length
#define USB_HCI_VERSION     0x2 	// Interface Version Number (BCD)
#define USB_HCS_PARAMS      0x4	 	// Structural Parameters
#define USB_HCC_PARAMS      0x8	 	// Capability Parameters
#define USB_HCSP_PORTROUTE  0xC     // Companion Port Route Description
// Operation registers offsets
#define USB_CMD             0       // USB Command
#define USB_STATUS          0x4     // USB Status
#define USB_INTR_EN         0x8     // USB Interrupt Enable
#define USB_FR_INDEX        0xC     // USB Frame Index
#define USB_SEG_SEL         0x10    // 4G Segment Selector
#define USB_FR_BAR          0x14    // Frame List Base Address
#define USB_NXT_LST_ADDR    0x18    // Next Asynchronous List Address
#define USB_CFG_FLG         0x40    // Configured Flag Register
#define USB_PORT_SC         0x44    // Port Status/Control Register

/**
 * Private types 
 */

typedef struct usb_qtd_s {
    uint32 next_qtd;
    uint32 alt_next_qtd;
    uint32 token;
    uint32 buffer0;
    uint32 buffer1;
    uint32 buffer2;
    uint32 buffer3;
    uint32 buffer4;
} USBQTD;

typedef struct usb_qh_s {
    uint32 qhead_hlink;
    uint32 endpoint_crc;
    uint32 endpoint_cap;
    uint32 current_qtd;
    USBQTD overlay;
} USBQHead;

/**
 * Private variables
 */

static uint8 _usb_bus;
static uint8 _usb_device;
static uint8 _usb_function;
static uint8 _usb_eecp_offset;
static uint32 _usb_eecp;
static uint16 _usb_pci_command; 
static uint32 _usb_base;
static uint32 _usb_op_base;

static uint32 *_usb_frame_list;
static USBQTD _usb_qtds[USB_MAX_QTD];
static Queue _usb_qtd_q;

static USBQHead _usb_qhead;

/**
 * Private functions
 */

static void _usb_set_pci_command( uint16 command ) {
    _usb_pci_command = command;
    _pci_set_command( _usb_bus, _usb_device, _usb_function, _usb_pci_command );
}

/**
 * Sets the pci command register bits specified by the command argument to 1
 * Before:  Register=0x0FFE     Argument=0x0001
 * After:   Register=0x0FFF
 */
static void _usb_pci_command_enable( uint16 toEnable ) {
    _usb_set_pci_command( _usb_pci_command | toEnable );
}

/**
 * Sets the pci command register bits specified by the command argument to 0
 * Before:  Register=0x0FFF     Argument=0x0001
 * After:   Register=0x0FFE
 */
static void _usb_pci_command_disable( uint16 toDisable ) {
    _usb_set_pci_command( _usb_pci_command & (~toDisable) );
}

/**
 * Reads a long word from the base address addr at offset offset
 */
static uint32 _usb_read_l( uint32 addr, uint32 offset ) {
    return( *(uint32 *)( addr + (offset & 0xFFFFFFFC) ) );
}

/**
 * Reads a word from the base address addr at offset offset
 */
static uint16 _usb_read_w( uint32 addr, uint32 offset ) {
    return( *(uint16 *)( addr + (offset & 0xFFFFFFFE) ) );
}

/**
 * Reads a byte from the base address addr at offset offset
 */
static uint8 _usb_read_b( uint32 addr, uint32 offset ) {
    return( *(uint8 *)( addr + offset) );
}

/**
 * Writes a long word at the base address addr plus offset offset
 */
static void _usb_write_l( uint32 addr, uint32 offset, uint32 value ) {
    *(uint32 *)( addr + (offset & 0xFFFFFFFC) ) = value;
}

/**
 * Writes a word at the base address addr plus offset offset
 */
static void _usb_write_w( uint32 addr, uint32 offset, uint16 value ) {
    *(uint16 *)( addr + (offset & 0xFFFFFFFE) ) = value;
}

/**
 * Writes a byte at the base address addr plus offset offset
 */
static void _usb_write_b( uint32 addr, uint32 offset, uint8 value ) {
    *(uint8 *)( addr + offset) = value;
}

/**
 * Sets the usb command register bits specified by the command argument to 1
 * Before:  Register=0xFFFF0FFE     Argument=0x0001
 * After:   Register=0xFFFF0FFF
 */
static void _usb_command_enable( uint32 toEnable ) {
    uint32 command = _usb_read_l( _usb_op_base, USB_CMD );
    command = command | toEnable;
    _usb_write_l( _usb_op_base, USB_CMD, command );
}

/**
 * Sets the usb command register bits specified by the command argument to 0
 * Before:  Register=0xFFFF0FFF     Argument=0x0001
 * After:   Register=0xFFFF0FFE
 */
static void _usb_command_disable( uint32 toDisable ) {
    uint32 command = _usb_read_l( _usb_op_base, USB_CMD );
    command = command & (~toDisable);
    _usb_write_l( _usb_op_base, USB_CMD, command );
}

/**
 * Public funtions
 */

void _usb_init( void ) {
    __cio_puts( "\n--------------------USB SHIT--------------------\n" );

    // Enable Bus Master and Memory Space
    _usb_set_pci_command( 0x0146 );

    PCIDevice *dev = _pci_dev_class( USB_CLASS, USB_SUBCLASS, USB_EHCI_PROGIF );

    assert( dev != NULL );
    assert( dev->bar0 != 0xFFFFFFFF );

    // store device location
    _usb_bus = dev->bus;
    _usb_device = dev->device;
    _usb_function = dev->function;

    // store capability registers and operation registers base address
    _usb_base = dev->bar0;
    _usb_op_base = _usb_base + _usb_read_b( _usb_base, USB_CAP_LEN );

    // fetch EHCI Extended Capabilities Pointer (EECP)
    _usb_eecp_offset = (uint8)((_usb_read_l( _usb_base, USB_HCC_PARAMS ) >> 8) & 0xFF);

    // check if BIOS owns the controller; if true, get ownership
    if( _usb_eecp_offset >= 0x40 ) {
        _usb_eecp = _pci_cfg_read_l( _usb_bus, _usb_device, _usb_function, _usb_eecp_offset );
        if( (_usb_eecp >> 24) & 1 ) {
            __cio_puts( " USB EHCI: BIOS owns the controller\n" );
        } else {
            // get ownership of the controller
            _usb_eecp = _usb_eecp | 0x00010000;
            _pci_cfg_write_l( _usb_bus, _usb_device, _usb_function, _usb_eecp_offset, _usb_eecp );
        }
    }

    // n_port?
    __cio_printf( "N_PORT: %02x\n", _usb_read_l(_usb_base, USB_HCS_PARAMS) & 0xF);

    // stop and reset the controller
    _usb_command_disable(1);
    _usb_command_enable(2);

    // allocate and init qtds
    _usb_qtd_q = _queue_alloc( NULL );
    for( int i = 0; i < USB_MAX_QTD; i++ ) {
        _usb_qtds[i].next_qtd = 1;      // invalid
        _usb_qtds[i].alt_next_qtd = 1;  // never used
        _usb_qtds[i].token = 1;
        _usb_qtds[i].buffer0 = 0;
        _usb_qtds[i].buffer1 = 0;
        _usb_qtds[i].buffer2 = 0;
        _usb_qtds[i].buffer3 = 0;
        _usb_qtds[i].buffer4 = 0;
        _queue_enque( _usb_qtd_q, (void *)&_usb_qtds[i] );
    }

    // setup control queue head
    assert((uint32)&_usb_qhead == ((uint32)&_usb_qhead & 0xFFFFFFE0));
        // QHeads must be 32 bit aligned
        // this must be addressed when several will be used
    _usb_qhead.qhead_hlink = ((uint32)&_usb_qhead) & 0xFFFFFFE0 | 2;
    _usb_qhead.endpoint_crc = 0x0040D000;
    _usb_qhead.endpoint_cap = 0x04000000;
    _usb_qhead.current_qtd = 0;
    _usb_qhead.overlay.next_qtd = 1;
    _usb_qhead.overlay.alt_next_qtd = 1;
    _usb_qhead.overlay.token = 0;
    _usb_qhead.overlay.buffer0 = 0;
    _usb_qhead.overlay.buffer1 = 0;
    _usb_qhead.overlay.buffer2 = 0;
    _usb_qhead.overlay.buffer3 = 0;
    _usb_qhead.overlay.buffer4 = 0;

    // get descriptor setup packet
    char buf[256];
    __cio_puts( "buf ");
    for( uint32 x = 0; x < 8; x++ ) {
        buf[x] = 0;
        __cio_printf( "%02x ", buf[x]);
    }
    __cio_puts( "\n" );
    USBQTD *out = (USBQTD *)_queue_deque( _usb_qtd_q );
    out->token = 0x80008C80;
    USBQTD *in = (USBQTD *)_queue_deque( _usb_qtd_q );
    in->next_qtd = ((uint32)out & 0xFFFFFFE0);
    in->token = 0x80400D80;
    in->buffer0 = (uint32)buf;
    USBQTD* setup = (USBQTD *)_queue_deque( _usb_qtd_q );
    setup->next_qtd = ((uint32)in & 0xFFFFFFE0);
    setup->token = 0x00080E80;

    // update next qh next qtd
    _usb_qhead.overlay.next_qtd = (uint32)setup & 0xFFFFFFE0;

    // portcs
    for( int i = 0; i < 6; i++ ) {
        __cio_printf( "port %d %08x\n", i+1, _usb_read_l( _usb_op_base, 0x44 + 4*i ));
    }

    // setup frame list
    // _usb_frame_list = (uint32 *)_kalloc_page(1);
    // _usb_write_l( _usb_op_base, USB_FR_BAR, ((uint32)_usb_frame_list) );

    // assert( _usb_frame_list == ((uint32 *)_usb_read_l( _usb_op_base, USB_FR_BAR )));

    // for(int i = 0; i < USB_MAX_FRLIST; i++) { // Set the pointers to be invalid
    //     _usb_frame_list[i] = 1;
    // }

    // enable periodic schedule
    // _usb_command_enable(0x10);

    // start the controller
    _usb_command_enable(1);

    __cio_printf( "\ncommand %08x\n", _usb_read_l( _usb_op_base, USB_CMD ));
    __cio_printf( "status %08x\n", _usb_read_l( _usb_op_base, USB_STATUS ));
    for( uint32 j = 0; j < 10; j++ ) {
        for( uint32 i = 0; i < 0xAFFFFFF; i++ );
            __cio_puts( "buf ");
            for( uint32 x = 0; x < 8; x++ )
                __cio_printf( "%02x ", buf[x]);
        __cio_printf( "\ncommand %08x\n", _usb_read_l( _usb_op_base, USB_CMD ));
        __cio_printf( "status %08x\n", _usb_read_l( _usb_op_base, USB_STATUS ));
    }

    __cio_puts( "--------------------USB SHIT--------------------\n" );
    return( 94 );
}
