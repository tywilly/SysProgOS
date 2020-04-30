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

// flash drive information (from config descriptor)
// the size of the data structures are arbitrarily chosen
// it's assumed that only one device is connected
static struct _usb_dev_s {
    uint8 port;
    uint8 class;
    uint8 addr;
    uint8 n_itf;          // number of interfaces
    uint8 cfg_val;        // configuration value
    uint8 str_man;        // string descriptor index
    uint8 str_pdt;          // for manufacturer, product
    uint8 str_num;          // serial number
    uint8 str_cfg;          // and configuration
} _usb_dev;

static struct _usb_itf_s {
    uint8 n_edp;          // number of endpoints
    uint8 class;          // class (08 for mass storage)
    uint8 str_idx;        // string descriptor index 
} _usb_itf[8];

static struct _usb_edp_s {
    uint8 addr;           // number (3-0) and direction (7)
    uint8 attr;           // transfer (1-0), sync (3-2) and usage type (5-4)
    uint16 packet_size;
    uint8 interval;       // interval for polling endpoint
    USBQHead *qhead;      // associated qhead
} _usb_edp[8][8];

static USBQHead *_usb_edp0;

/*
** PRIVATE FUNCTIONS
*/

//
// PCI related functions
//

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
// USB memory access
//

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
// QTD/QHead related functions
//

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

//
// _usb_free_qhead() - free a qhead and enqueue it in the qhead_q
//
// Parameters:
//    qhead     pointer to the qhead
//
static void _usb_free_qhead( USBQHead *qhead ) {
    qhead->overlay.next_qtd = 1;
    qhead->overlay.alt_next_qtd = 1;
    qhead->overlay.token = 0;
    qhead->overlay.buffer0 = 0;
    qhead->overlay.buffer1 = 0;
    qhead->overlay.buffer2 = 0;
    qhead->overlay.buffer3 = 0;
    qhead->overlay.buffer4 = 0;
    _queue_enque( _usb_qhead_q, qhead );
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
    ((uint16 *)data)[1] = val;
    ((uint16 *)data)[2] = ind;
    ((uint16 *)data)[3] = len;
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
      (len & 0x7FFF) << 16 |                      // total bytes
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
// _usb_get_trans() - perform a get transcation (SETUP + IN + OUT)
//
// Parameters:
//    req_buf       pointer to request buffer
//    res_buf       buffer to write the result
//    len           length of the required data
//
static void _usb_get_trans( uint32 req_buf, uint32 res_buf, uint16 len ) {
    // get device qhead for edp0
    USBQHead *qhead = _usb_edp0;

    // setup qtds
    USBQTD *out = _usb_build_qtd( USB_OUT, 0, NULL, NULL );
    USBQTD *in = _usb_build_qtd( USB_IN, len, (uint32)out, res_buf );
    USBQTD *setup = _usb_build_qtd( USB_SETUP, 0x8, (uint32)in, req_buf );

    // update qh next qtd
    qhead->overlay.next_qtd = (uint32)setup & 0xFFFFFFE0;

    // wait for end of transaction
    while((out->token & 0xFF) == 0x80);
    
    if( 
        (setup->token & 0x7F) != 0 ||
        (out->token & 0x7F) != 0 ||
        (in->token & 0x7F) != 0
    ) {
        __cio_puts( "USB ERR: Transaction error\n" );
    }

    // free used qtds
    _usb_free_qtd( setup );
    _usb_free_qtd( in );
    _usb_free_qtd( out );
}

//
// _usb_get_dev_desc() - get the device descriptor
//
// Parameters:
//    res_buf       buffer to write the result
//    len           length of the descriptor 
//                  (used when maxPacketLength for endpoint0 is not yet known)
//
static void _usb_get_dev_desc( uint32 res_buf, uint16 len ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request(0x80, 0x06, 0x0100, 0x0000, len & 0x7FFF, req_buf);
    
    // get descriptor request
    _usb_get_trans( (uint32)req_buf, res_buf, len );

    // free request buffer
    _kfree_slice( req_buf );
}

//
// _usb_get_cfg_desc() - get the configuration descriptor
//
// Parameters:
//    res_buf       buffer to write the result
//    len           length of the descriptor
//
static void _usb_get_cfg_desc( uint32 res_buf, uint16 len ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request(0x80, 0x06, 0x0200, 0x0000, len & 0x7FFF, req_buf);
    
    // get descriptor request
    _usb_get_trans( (uint32)req_buf, res_buf, len );

    // free request buffer
    _kfree_slice( req_buf );
}

//
// _usb_get_str_desc() - get the string descriptor
//
// Parameters:
//    res_buf       buffer to write the result
//    index         index to read in the descriptor
//    len           length of the descriptor
//
static void _usb_get_str_desc( uint32 res_buf, uint8 index, uint16 len ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request(0x80, 0x06, (0x03 << 8) | index, 0x0000, len & 0x7FFF, req_buf);
    
    // get descriptor request
    _usb_get_trans( (uint32)req_buf, res_buf, len );

    // free request buffer
    _kfree_slice( req_buf );
}

//
// _usb_get_cfg() - get the configuration value
//
// Parameters:
//    res_buf       buffer to write the result
//
static void _usb_get_cfg( uint32 res_buf ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request(0x80, 0x08, 0x0000, 0x0000, 0x0001, req_buf);
    
    // get descriptor request
    _usb_get_trans( (uint32)req_buf, res_buf, 0x0001 );

    // free request buffer
    _kfree_slice( req_buf );
}

//
// _usb_dump_str_desc() - dump contents of string descriptors
//
static void _usb_dump_str_desc( void ) {
    uint8 i;
    char *str_desc = (char *)_kalloc_slice();
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_man, 0x2 );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_man, str_desc[0] );
    __cio_puts( "Manufacturer:  " );
    for( i = 2; i < str_desc[0]; i++ ) {
        __cio_puts( str_desc + i );
    }
    __cio_puts( "\nProduct:       " );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_pdt, 0x2 );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_pdt, str_desc[0] );
    for( i = 2; i < str_desc[0]; i++ ) {
        __cio_puts( str_desc + i );
    }
    __cio_puts( "\nSerial Number: " );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_num, 0x2 );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_num, str_desc[0] );
    for( i = 2; i < str_desc[0]; i++ ) {
        __cio_puts( str_desc + i );
    }
    __cio_puts( "\nConfiguration: " );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_cfg, 0x2 );
    _usb_get_str_desc( (uint32)str_desc, _usb_dev.str_cfg, str_desc[0] );
    for( i = 2; i < str_desc[0]; i++ ) {
        __cio_puts( str_desc + i );
    }
    __cio_puts("\n");
    _kfree_slice( str_desc );
}

//
// _usb_set_trans() - perform a set transaction (SETUP + IN)
//
// Parameters:
//    req_buf       request buffer
//
static void _usb_set_trans( uint32 req_buf ) {
    // get device qhead for edp0
    USBQHead *qhead = _usb_edp0;

    // setup qtds
    USBQTD *in = _usb_build_qtd( USB_IN, 0, NULL, NULL );
    USBQTD *setup = _usb_build_qtd( USB_SETUP, 0x8, (uint32)in, req_buf );

    // update qh next qtd
    qhead->overlay.next_qtd = (uint32)setup & 0xFFFFFFE0;

    // wait for end of transaction
    while((in->token & 0xFF) == 0x80);

    if( 
        (setup->token & 0x7F) != 0 ||
        (in->token & 0x7F) != 0
    ) {
        __cio_puts( "USB ERR: Transaction error\n" );
    }

    // free used qtds
    _usb_free_qtd( setup );
    _usb_free_qtd( in );
}

//
// _usb_set_addr() - set the device address and updates the field in qheads
//
// Parameters:
//    addr          device address
//
static void _usb_set_addr( uint8 addr ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request( 0x00, 0x05, (uint16)(addr & 0x7F), 0x0000, 0x0000, req_buf );

    // perform set transaction
    _usb_set_trans( (uint32)req_buf );

    // free request buffer
    _kfree_slice( req_buf );

    // change other occurrances of device address
    _usb_dev.addr = addr & 0x7F;
    _usb_edp0->endpoint_crc = (_usb_edp0->endpoint_crc & 0xFFFFFF80) | (addr & 0x7F);
    for( uint8 i = 0; i < _usb_dev.n_itf; i++ ) {
        for( uint8 j = 0; j < _usb_itf[i].n_edp && _usb_edp[i][j].qhead != NULL; j++ ) {
            _usb_edp[i][j].qhead->endpoint_crc =
                (_usb_edp[i][j].qhead->endpoint_crc & 0xFFFFFF80) | (addr & 0x7F);
        }
    }
}

//
// _usb_set_cfg() - set the device configuration
//
// Parameters:
//    value         configuration value (given in the config descriptor)
//
static void _usb_set_cfg( uint8 value ) {
    // setup request buffer
    char *req_buf = (char *)_kalloc_slice();
    _usb_build_request( 0x00, 0x09, (uint16)value, 0x0000, 0x0000, req_buf );

    // perform set transaction
    _usb_set_trans( (uint32)req_buf );

    // free request buffer
    _kfree_slice( req_buf );
}

//
// Device/Interface/Endpoint struct related functions
//

//
// _usb_dump_endpoint() - dump information from specified endpoint
//
static void _usb_dump_endpoint( uint8 n_itf, uint8 n_edp ) {
    __cio_printf( "    Endpoint %d: ", _usb_edp[n_itf][n_edp].addr & 0xF);
    if( _usb_edp[n_itf][n_edp].addr >> 7 ) {
        __cio_puts( "dir=IN  - ");
    } else {
        __cio_puts( "dir=OUT - ");
    }
    __cio_printf( "attr=%02x - ", _usb_edp[n_itf][n_edp].attr);
    __cio_printf( "packet_size=%04x - ", _usb_edp[n_itf][n_edp].packet_size);
    __cio_printf( "interval=%02x\n", _usb_edp[n_itf][n_edp].interval);
}

//
// _usb_dump_interface() - dump information from specified interface
//
static void _usb_dump_interface( uint8 n ) {
    __cio_printf( "  Interface %d: class=%02x - %d endpoint(s):\n", 
        n, _usb_itf[n].class, _usb_itf[n].n_edp );
    for( uint8 i = 0; i < _usb_itf[n].n_edp; i++ ) {
        _usb_dump_endpoint( n, i );
    }
}

//
// _usb_dump_interface() - dump information from device
//
static void _usb_dump_dev_info( void ) {
    __cio_printf( "Flash drive: port=%d - cfg_val=%02x - %d interface(s):\n", 
        _usb_dev.port, _usb_dev.cfg_val, _usb_dev.n_itf );
    for( uint8 i = 0; i < _usb_dev.n_itf; i++ ) {
        _usb_dump_interface( i );
    }
}

/*
** PUBLIC FUNCTIONS
*/

//
// _usb_init() - USB controller initialization. Since the flash
//               drive is plugged in from the start, device
//               enumeration is done in this function directly.
// In order, what this does is:
// - general init sequence
//     - enable bus master and memory space
//     - get controller information from PCI (including base address)
//     - get ownership of the controller if necessary
//     - stop and reset the controller
//     - route all ports to this controller
//     - start the controller
//     - reset ports that changed
// - data structure init
//     - allocate and init QHeads
//     - allocate and init QTDs
// - setup endpoint 0
//     - enable asynchronous transfer
// - get descriptors
//     - get device descriptor
//     - get config descriptor
//     - get interface descriptors
//     - get endpoints descriptors
// - set device address
// - set configuration
// - // setup interrupt isr
// - // setup frame list
//     - // enable periodic schedule
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

    // reset the port where device is connected (it's assumed there is only one)
    _usb_n_port = _usb_read_l( _usb_base, USB_HCSPARAMS ) & 0xF;
    for( uint8 i = 0; i < _usb_n_port; i++ ) {
        uint32 portsc = _usb_read_l( _usb_op_base, 0x44 + 4*i );
        // recently connected device
        if( (portsc & 0b11) == 0b11 ) {
            _usb_write_l( _usb_op_base, 0x44 + 4*i, portsc | 0x100 );
            for( uint16 j = 0; j < 0xFFFF; j++ );
            _usb_write_l( _usb_op_base, 0x44 + 4*i, portsc );
            _usb_dev.port = i;
            break;
        }
    }

    // allocate and init qheads
    _usb_qheads = (USBQHead *)_kalloc_page(1);
    _usb_qhead_q = _queue_alloc( NULL );
    for( int i = 0; i < USB_MAX_QHEAD; i++ ) {
        _usb_free_qhead( &_usb_qheads[i] );
    }

    // allocate and init qtds
    _usb_qtds = (USBQTD *)_kalloc_page(1);
    _usb_qtd_q = _queue_alloc( NULL );
    for( int i = 0; i < USB_MAX_QTD; i++ ) {
        _usb_free_qtd( &_usb_qtds[i] );
    }

    // setup control queue head (device endpoint 0)
    USBQHead *qhead = _queue_deque( _usb_qhead_q );
    qhead->qhead_hlink = (((uint32)qhead) & 0xFFFFFFE0) | 2;
    qhead->endpoint_crc = 0x0040D000;
    qhead->endpoint_cap = 0x40000000;
    qhead->current_qtd = 0;
    _usb_edp0 = qhead;
    _usb_dev.addr = 0;      // temporarily set device address to 0
    _usb_write_l( _usb_op_base, USB_ASYNCLISTADDR, (uint32)qhead );
    _usb_command_enable( 0x20 );

    //
    // First transactions: 
    //   Because descriptor length may vary, descriptors are gotten twice; the
    //     first time to get there length, the second time to get the full
    //     descriptor.
    //   Also, buffers must not cross a 4k page boundary; I'm not sending
    //     or getting more than 1k at the moment so a slice is enough.
    //   Finally, not to overload the code with checks, length that are read
    //     from descriptors are assumed to be lower than 1K, for the
    //     descriptors to fit in the allocated buffers.
    //

    // get device descriptor
    char *dev_desc = (char *)_kalloc_slice();
    _usb_get_dev_desc( (uint32)dev_desc, 0x8 );
    _usb_dev.class = dev_desc[4];       // get device class

    // set maximum packet length for endpoint 0
    _usb_edp0->endpoint_crc =
        (_usb_edp0->endpoint_crc & 0xF800FFFF) | ((dev_desc[7] & 0x7FF) << 16);

    // get full device descriptor
    _usb_get_dev_desc( (uint32)dev_desc, 0x12 );
    _usb_dev.str_man = dev_desc[14];    // get string descriptor indices
    _usb_dev.str_pdt = dev_desc[15];
    _usb_dev.str_num = dev_desc[16];

    // get device configuration descriptor
    char *cfg_desc = (char *)_kalloc_slice();
    _usb_get_cfg_desc( (uint32)cfg_desc, 0x4 );
    _usb_get_cfg_desc( (uint32)cfg_desc, *(uint16 *)(cfg_desc+2) );

    // fill device information from configuration descriptor
    _usb_dev.n_itf = cfg_desc[4];
    _usb_dev.cfg_val = cfg_desc[5];
    _usb_dev.str_cfg = cfg_desc[6];
    uint8 index = 9;
    // iterate on all interfaces of the device
    for( uint8 i = 0; i < _usb_dev.n_itf; i++ ) {
        _usb_itf[i].n_edp = cfg_desc[index + 4];
        _usb_itf[i].class = cfg_desc[index + 5];
        _usb_itf[i].str_idx = cfg_desc[index + 8];
        index += 9;
        // iterate on all endpoints for the given interface
        for( uint8 j = 0; j < _usb_itf[i].n_edp; j++ ) {
            _usb_edp[i][j].addr = cfg_desc[index + 2];
            _usb_edp[i][j].attr = cfg_desc[index + 3];
            _usb_edp[i][j].packet_size = *(uint16 *)(cfg_desc+index+4);
            _usb_edp[i][j].interval = cfg_desc[index + 6];
            index += 7;
        }
    }

    // check that device is mass storage and uses bulk transfer
    // I don't know how to manage the fact that there are potentially many interfaces,
    // so I use the fact that I know the device I use only has 1.
    if( _usb_dev.class != 8 && _usb_itf[0].class != 8 ) {
        __cio_puts( "USB ERR: Device is not mass storage\n" );
    }
    for( uint8 i = 0; i < _usb_itf[0].n_edp; i++ ) {
        if( (_usb_edp[0][i].attr & 0b11) != 0b10 ) {
            __cio_puts( "USB ERR: Device doesn't use bulk transfer\n" );
            break;
        }
    }

    // set device address to 0x66 (because it's a nice number)
    _usb_set_addr( 0x66 );

    // set device configuration
    _usb_set_cfg( _usb_dev.cfg_val );

    // change the interrupt line and install ISR
    // _pci_set_interrupt( _usb_bus, _usb_device, _usb_function, 0x0, 0x43 );
    // __install_isr( 0x43, _usb_isr );

    // setup frame list
    // _usb_frame_list = (uint32 *)_kalloc_page(1);
    // _usb_write_l( _usb_op_base, USB_PERIODICLISTBASE, ((uint32)_usb_frame_list) );

    // assert( _usb_frame_list == ((uint32 *)_usb_read_l( _usb_op_base, USB_PERIODICLISTBASE )));

    // for(int i = 0; i < USB_MAX_FRLIST; i++) { // Set the pointers to be invalid
    //     _usb_frame_list[i] = 1;
    // }

    // enable periodic schedule
    // _usb_command_enable(0x10);

    __cio_puts( " USB" );
}

//
// _usb_dump_all() - dump all device information / string 
//     descriptors gathered from the init sequence
//
void _usb_dump_all() {
    __cio_puts("\n------------------------ USB Configuration ------------------------\n");
    _usb_dump_dev_info();
    __cio_puts("--------------------- USB String descriptors  ---------------------\n");
    _usb_dump_str_desc();
    __cio_puts("-------------------------------------------------------------------\n");
}
