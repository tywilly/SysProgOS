/*
** File:    usb.c
**
** Author:  Yann MOULLEC
**
** Contributor: Tyler Wilcox (he helped me when I was stuck)
**
** Description: Implementation of the usb module
*/

#define __SP_KERNEL__

#include "common.h"
#include "usb.h"

/*
** PRIVATE DEFINITIONS
*/

// array sizes
#define USB_MAX_FRLIST       1024
#define USB_MAX_QTD          128
#define USB_MAX_QHEAD        64
// USB class, subclass, EHCI progIF (used for PCI module)
#define USB_CLASS            0xC
#define USB_SUBCLASS         0x3
#define USB_EHCI_PROGIF      0x20
// Capability registers offsets
#define USB_CAPLENGTH        0      // Capability Register Length
#define USB_HCIVERSION       0x2    // Interface Version Number (BCD)
#define USB_HCSPARAMS        0x4	// Structural Parameters
#define USB_HCCPARAMS        0x8	// Capability Parameters
#define USB_HCSP_PORTROUTE   0xC    // Companion Port Route Description
// Operation registers offsets
#define USB_CMD              0      // USB Command
#define USB_STS              0x4    // USB Status
#define USB_INTR             0x8    // USB Interrupt Enable
#define USB_FRINDEX          0xC    // USB Frame Index
#define USB_CTRLDSSEGMENT    0x10   // 4G Segment Selector
#define USB_PERIODICLISTBASE 0x14   // Frame List Base Address
#define USB_ASYNCLISTADDR    0x18   // Next Asynchronous List Address
#define USB_CONFIGFLAG       0x40   // Configured Flag Register
#define USB_PORTSC           0x44   // Port Status/Control Register
// QTD CONTENTS
#define USB_SETUP            0b10
#define USB_IN               0b01
#define USB_OUT              0b00

/*
** PRIVATE DATA TYPES
*/

// USB queue transfer descriptor
// QTDs are linked in a linked list to describe a complete
// transaction sequence.
// QTDs contain a description of the transfer (packet) and a
// pointer to the transfer data (if any).
typedef struct usb_qtd_s {
    uint32 next_qtd;        // next qtd in the linked list
    uint32 alt_next_qtd;
    uint32 token;           // qtd transfer information
    uint32 buffer0;         // data buffer0 (only one used here)
    uint32 buffer1;
    uint32 buffer2;
    uint32 buffer3;
    uint32 buffer4;
} USBQTD;

// USB queue head
// QHeads are linked in a linked list.
// The controller follows the linked list to handle them one
// at a time.
// The overlay area is used by the controller to copy current
// qtd data. It doesn't nee to be initialized.
// All Qheads must be 32 byte aligned.
typedef struct usb_qh_s {
    uint32 qhead_hlink;     // next qh in the linked list
    uint32 endpoint_crc;    // endpoint caracteristics
    uint32 endpoint_cap;    // endpoint capabilities
    uint32 current_qtd;     // pointer to current qtd
    USBQTD overlay;         // current qtd contents
    uint32 align0;
    uint32 align1;
    uint32 align2;
    uint32 align3;
} USBQHead;

/*
** PRIVATE GLOBAL VARIABLES
*/

// PCI device related information
static uint8 _usb_bus;
static uint8 _usb_device;
static uint8 _usb_function;
static uint16 _usb_pci_command; // pci command register
// USB controller information
static uint8 _usb_eecp_offset;  // offset for eecp vvv
static uint8 _usb_n_port;
static uint32 _usb_eecp;        // EHCI Extended Capabilities Pointer
// USB controller base addresses
static uint32 _usb_base;
static uint32 _usb_op_base;

// USB data structures
static uint32 *_usb_frame_list; // Frame list
static USBQTD *_usb_qtds;       // QTD array
static Queue _usb_qtd_q;        // QTD Queue
static USBQHead *_usb_qheads;   // QHead array
static Queue _usb_qhead_q;      // QHead Queue

// flash drive information
static USBQHead *_usb_dev_endpoints[16];

/*
** PRIVATE FUNCTIONS
*/

//
// _usb_set_pci_command() - set pci command register
//
// Parameters:
//    command   value for command register
//
static void _usb_set_pci_command( uint16 command ) {
    _usb_pci_command = command;
    _pci_set_command( _usb_bus, _usb_device, _usb_function, _usb_pci_command );
}

//
// _usb_pci_command_enable() - set specific pci command register bits
// Before:  Register=0x0FFE     toEnable=0x0001
// After:   Register=0x0FFF
//
// Parameters:
//    toEnable      bits to set
//
static void _usb_pci_command_enable( uint16 toEnable ) {
    _usb_set_pci_command( _usb_pci_command | toEnable );
}

//
// _usb_pci_command_disable() - clear specific pci command register bits
// Before:  Register=0x0FFF     toEnable=0x0001
// After:   Register=0x0FFE
//
// Parameters:
//    toDisable     bits to clear
//
static void _usb_pci_command_disable( uint16 toDisable ) {
    _usb_set_pci_command( _usb_pci_command & (~toDisable) );
}

//
// _usb_read_l() - read a long word from the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//
// Returns:
//    the long word read
//
static uint32 _usb_read_l( uint32 addr, uint32 offset ) {
    return( *(uint32 *)( addr + (offset & 0xFFFFFFFC) ) );
}

//
// _usb_read_w() - read a word from the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//
// Returns:
//    the word read
//
static uint16 _usb_read_w( uint32 addr, uint32 offset ) {
    return( *(uint16 *)( addr + (offset & 0xFFFFFFFE) ) );
}

//
// _usb_read_b() - read a byte from the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//
// Returns:
//    the byte read
//
static uint8 _usb_read_b( uint32 addr, uint32 offset ) {
    return( *(uint8 *)( addr + offset) );
}

//
// _usb_write_l() - write a long word to the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//    value     value to be written
//
static void _usb_write_l( uint32 addr, uint32 offset, uint32 value ) {
    *(uint32 *)( addr + (offset & 0xFFFFFFFC) ) = value;
}

//
// _usb_write_w() - write a word to the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//    value     value to be written
//
static void _usb_write_w( uint32 addr, uint32 offset, uint16 value ) {
    *(uint16 *)( addr + (offset & 0xFFFFFFFE) ) = value;
}

//
// _usb_write_b() - write a byte to the usb controller
//
// Parameters:
//    addr      base address
//    offset    offset
//    value     value to be written
//
static void _usb_write_b( uint32 addr, uint32 offset, uint8 value ) {
    *(uint8 *)( addr + offset) = value;
}

//
// _usb_command_enable() - set the specified usb command register bits
// Before:  Register=0xFFFF0FFE     Argument=0x0001
// After:   Register=0xFFFF0FFF
//
// Parameters:
//    toEnable      bits to enable
//
static void _usb_command_enable( uint32 toEnable ) {
    uint32 command = _usb_read_l( _usb_op_base, USB_CMD );
    command = command | toEnable;
    _usb_write_l( _usb_op_base, USB_CMD, command );
}

//
// _usb_command_disable() - clear the specified usb command register bits
// Before:  Register=0xFFFF0FFF     Argument=0x0001
// After:   Register=0xFFFF0FFE
//
// Parameters:
//    toDisable      bits to clear
//
static void _usb_command_disable( uint32 toDisable ) {
    uint32 command = _usb_read_l( _usb_op_base, USB_CMD );
    command = command & (~toDisable);
    _usb_write_l( _usb_op_base, USB_CMD, command );
}

//
// _usb_dump_qtd() - dump QTD contents to console
//
// Parameters:
//    qtd       pointer to QTD
//    vb        if true prints all QTD if not stops at buffer0
//
static void _usb_dump_qtd( USBQTD *qtd, bool vb ) {
    __cio_printf( "QTD\n" );
    __cio_printf( " next_qtd     %08x\n", qtd->next_qtd );
    __cio_printf( " alt_next_qtd %08x\n", qtd->alt_next_qtd );
    __cio_printf( " token        %08x\n", qtd->token );
    __cio_printf( " buffer0      %08x\n", qtd->buffer0 );
    if( vb ) {
        __cio_printf( " buffer1      %08x\n", qtd->buffer1 );
        __cio_printf( " buffer2      %08x\n", qtd->buffer2 );
        __cio_printf( " buffer3      %08x\n", qtd->buffer3 );
        __cio_printf( " buffer4      %08x\n", qtd->buffer4 );
    }
}

//
// _usb_dump_qhead() - dump QHead contents to console
//
// Parameters:
//    qhead     pointer to QHead
//    vb        if true prints all contents if not stops at next_qtd
//
static void _usb_dump_qhead( USBQHead *qhead, bool vb ) {
    __cio_printf( "QHead\n" );
    __cio_printf( " qhead_hlink  %08x\n", qhead->qhead_hlink );
    __cio_printf( " endpoint_crc %08x\n", qhead->endpoint_crc );
    __cio_printf( " endpoint_cap %08x\n", qhead->endpoint_cap );
    __cio_printf( " current_qtd  %08x\n", qhead->current_qtd );
    __cio_printf( " next_qtd     %08x\n", qhead->overlay.next_qtd );
    if( vb ) {
        __cio_printf( " alt_next_qtd %08x\n", qhead->overlay.alt_next_qtd );
        __cio_printf( " token        %08x\n", qhead->overlay.token );
        __cio_printf( " buffer0      %08x\n", qhead->overlay.buffer0 );
    }
}

//
// _usb_build_request() - build request data buffer for transaction
//
// Parameters:
//    reqType   type of request
//    req       request code
//    val       value
//    ind       index
//    len       length
//    data      buffer to write in
//
static void _usb_build_request( uint8 reqType, uint8 req, uint16 val, uint16 ind, uint16 len, void* data ) {
    ((uint8 *)data)[0] = reqType;
    ((uint8 *)data)[1] = req;
    ((uint8 *)data)[2] = (uint8)(val >> 8);
    ((uint8 *)data)[3] = (uint8)(val & 0xFF);
    ((uint8 *)data)[4] = (uint8)(ind >> 8);
    ((uint8 *)data)[5] = (uint8)(ind & 0xFF);
    ((uint8 *)data)[6] = (uint8)(len >> 8);
    ((uint8 *)data)[7] = (uint8)(len & 0xFF);
}

//
// _usb_build_qtd() - build a qtd
//
// Parameters:
//    type      qtd type: USB_IN, USB_OUT or USB_SETUP
//    len       total bytes to transfer
//    next      pointer to next qtd
//    buf       pointer to data buffer
//
// Returns:
//    a pointer to the qtd
//
static USBQTD *_usb_build_qtd( uint8 type, uint16 len, uint32 next, uint32 buf ) {
    USBQTD *qtd = (USBQTD *)_queue_deque( _usb_qtd_q );

    // qtd token
    qtd->token = (uint32)(
      (type == USB_IN || type == USB_OUT) << 31 | // dt
      (len & 0xFFFE) << 16 |                      // total bytes
      0b000011 << 10 |                            // ioc, cerr
      (type & 0b11) << 8 |                        // PID code
      0x80                                        // active qtd
    );

    if( next == NULL ) {
        qtd->next_qtd = 1;      // invalid field
    } else {
        qtd->next_qtd = next & 0xFFFFFFE0;
    }

    // not check on buffer (data should not cross a 4k page boundary, see caller)
    qtd->buffer0 = buf; 
    
    return qtd;
}

//
// _usb_free_qtd() - free a qtd and enqueue it in the qtd_q
//
// Parameters:
//    qtd       pointer to the qtd
//
static void _usb_free_qtd( USBQTD *qtd ) {
    qtd->next_qtd = 1;      // invalid
    qtd->alt_next_qtd = 1;  // never used
    qtd->buffer0 = 0;
    qtd->buffer1 = 0;
    qtd->buffer2 = 0;
    qtd->buffer3 = 0;
    qtd->buffer4 = 0;
    _queue_enque( _usb_qtd_q, qtd );
}

/*
** PUBLIC FUNCTIONS
*/

//
// _usb_init() - this is an attempt at initializing the USB controller 
//               and making a first request.
// In order, what this does is:
// - general init sequence
//     - enable bus master and memory space
//     - get device information from PCI
//     - get controller base addresses
//     - get ownership of the controller if necessary
//     - stop and reset the controller
//     - route all ports to this controller
//     - start the controller
//     - reset ports that changed
// - data structure init
//     - allocate and init QHeads
//     - allocate and init QTDs
// - setup first transaction
//     - setup a control queue head
//     - setup a get device descriptor request
//     - enable asynchronous transfer
// - (setup frame list)
// - CRASH MISERABLY: the QTD sequence is SETUP, IN, OUT
//   I get a 'Halt' on the IN QTD, which means something went horribly
//   wrong, but I don't know what.
//
void _usb_init( void ) {

    // Enable Bus Master and Memory Space
    _usb_set_pci_command( 0x0146 );

    // Get device information from PCI
    PCIDevice *dev = _pci_dev_class( USB_CLASS, USB_SUBCLASS, USB_EHCI_PROGIF );
    assert( dev != NULL );
    assert( dev->bar0 != 0xFFFFFFFF );

    // store device location
    _usb_bus = dev->bus;
    _usb_device = dev->device;
    _usb_function = dev->function;

    // store capability registers and operation registers base address
    _usb_base = dev->bar0;
    _usb_op_base = _usb_base + _usb_read_b( _usb_base, USB_CAPLENGTH );

    // fetch EHCI Extended Capabilities Pointer (EECP)
    _usb_eecp_offset = (uint8)((_usb_read_l( _usb_base, USB_HCCPARAMS ) >> 8) & 0xFF);

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

    // stop and reset the controller
    _usb_command_disable(1);
    _usb_command_enable(2);

    while((_usb_read_l(_usb_op_base, USB_CMD) & 2) == 2);

    // all ports route to this controller
    _usb_write_l( _usb_op_base, USB_CONFIGFLAG, 1 );

    // start the controller
    _usb_command_enable( 1 );

    // reset the port where the device is connected
    _usb_n_port = _usb_read_l( _usb_base, USB_HCSPARAMS ) & 0xF;
    for( uint8 i = 0; i < _usb_n_port; i++ ) {
        uint32 portsc = _usb_read_l( _usb_op_base, 0x44 + 4*i );
        // recently connected device
        if( portsc & 0b11 == 0b11 ) {
            _usb_write_l( _usb_op_base, 0x44 + 4*i, portsc | 0x100 );
            for( uint16 j = 0; j < 0xFFFF; j++ );
            _usb_write_l( _usb_op_base, 0x44 + 4*i, portsc );
        }
    }

    // allocate and init qheads
    _usb_qheads = (USBQHead *)_kalloc_page(1);
    _usb_qhead_q = _queue_alloc( NULL );
    for( int i = 0; i < USB_MAX_QHEAD; i++ ) {
        _usb_qheads[i].overlay.next_qtd = 1;
        _usb_qheads[i].overlay.alt_next_qtd = 1;
        _usb_qheads[i].overlay.token = 0;
        _usb_qheads[i].overlay.buffer0 = 0;
        _usb_qheads[i].overlay.buffer1 = 0;
        _usb_qheads[i].overlay.buffer2 = 0;
        _usb_qheads[i].overlay.buffer3 = 0;
        _usb_qheads[i].overlay.buffer4 = 0;
        _queue_enque( _usb_qhead_q, (void *)&_usb_qheads[i] );
    }

    // allocate and init qtds
    _usb_qtds = (USBQTD *)_kalloc_page(1);
    _usb_qtd_q = _queue_alloc( NULL );
    for( int i = 0; i < USB_MAX_QTD; i++ ) {
        _usb_free_qtd( &_usb_qtds[i] );
    }

    // setup control queue head (device endpoint 0)
    USBQHead *qhead = _queue_deque( _usb_qhead_q );
    qhead->qhead_hlink = ((uint32)qhead) & 0xFFFFFFE0 | 2;
    qhead->endpoint_crc = 0x0040D000;
    qhead->endpoint_cap = 0x40000000;
    qhead->current_qtd = 0;
    _usb_dev_endpoints[0] = qhead;

    // setup a get_device_descriptor request
    // Buffers must not cross a 4k page boundary;
    // I'm not sending more than 1k at the moment so a slice is enough.
    char *buf_snd = (char *)_kalloc_slice();
    char *buf_rec = (char *)_kalloc_slice();

    USBQTD *out = _usb_build_qtd( USB_OUT, 0, NULL, NULL );
    USBQTD *in = _usb_build_qtd( USB_IN, 0x40, out, buf_rec );
    _usb_build_request(0x80, 0x06, 0x0001, 0x0000, 0x0040, buf_snd);
    USBQTD *setup = _usb_build_qtd( USB_SETUP, 0x8, in, buf_snd );

    // update qh next qtd
    qhead->overlay.next_qtd = (uint32)setup & 0xFFFFFFE0;

    // change the interrupt line and install ISR
    // _pci_set_interrupt( _usb_bus, _usb_device, _usb_function, 0x0, 0x43 );
    // __install_isr( 0x43, _usb_isr );

    // enable async schedule
    _usb_write_l( _usb_op_base, USB_ASYNCLISTADDR, (uint32)qhead );
    _usb_command_enable( 0x20 );

    // wait for end of transaction
    while((out->token & 0xFF) == 0x80);

    // free used qtds
    _usb_free_qtd( setup );
    _usb_free_qtd( in );
    _usb_free_qtd( out );

    // setup frame list
    // _usb_frame_list = (uint32 *)_kalloc_page(1);
    // _usb_write_l( _usb_op_base, USB_PERIODICLISTBASE, ((uint32)_usb_frame_list) );

    // assert( _usb_frame_list == ((uint32 *)_usb_read_l( _usb_op_base, USB_PERIODICLISTBASE )));

    // for(int i = 0; i < USB_MAX_FRLIST; i++) { // Set the pointers to be invalid
    //     _usb_frame_list[i] = 1;
    // }

    // enable periodic schedule
    // _usb_command_enable(0x10);

    __cio_puts( "\n" );
    _usb_dump_qhead( qhead, false );

    for( uint32 x = 0; x < 8; x++ )
        __cio_printf( "%02x ", buf_rec[x]);
    __cio_puts( "\n" );

    return( 94 );
}
