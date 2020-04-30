/*
** File:    pci.c
**
** Author:  Yann MOULLEC
**
** Contributor:
**
** Description: Implementation of the pci module
*/

#define __SP_KERNEL__

#include "common.h"
#include "pci.h"

/*
** PRIVATE DEFINITIONS
*/

// PCI Device Structure offsets
// common to all header types
#define PCI_VENDOR_ID       0
#define PCI_DEVICE_ID       0x2
#define PCI_COMMAND         0x4
#define PCI_STATUS          0x6
#define PCI_REVISION_ID     0x8
#define PCI_PROG_IF         0x9
#define PCI_SUBCLASS        0xA
#define PCI_CLASS           0xB
#define PCI_CACHE_LINE_SIZE 0xC
#define PCI_LATENCY_TIMER   0xD
#define PCI_HEADER_TYPE     0xE
#define PCI_BIST            0xF
// other useful offsets
#define PCI_BAR0            0x10
#define PCI_BAR1            0x14
#define PCI_SEC_BUS         0x19

// (Arbitrary) value for maximum number of connected devices
// to the PCI
#define PCI_MAX_DEVICES 15

/*
** PRIVATE GLOBAL VARIABLES
*/

// The PCI module stores all static device information in an array
static PCIDevice _pci_dev_list[PCI_MAX_DEVICES];
static int _pci_num_dev = 0;

/*
** PRIVATE FUNCTIONS
*/

// forward declaration
static void _pci_check_bus( uint8 );

//
// _pci_check_function() - check the function for a connected device
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//
static void _pci_check_function( uint8 bus, uint8 device, uint8 function ) {
    uint32 bar0;
    uint32 bar1;
    uint16 vendorID;
    uint16 deviceID;
    uint8 class;
    uint8 subClass;
    uint8 progIF;
    uint8 secondaryBus;
    uint8 headerType;

    // read all function information
    class = _pci_cfg_read_b( bus, device, function, PCI_CLASS );
    subClass = _pci_cfg_read_b( bus, device, function, PCI_SUBCLASS );
    progIF = _pci_cfg_read_b( bus, device, function, PCI_PROG_IF );
    headerType = _pci_cfg_read_b( bus, device, function, PCI_HEADER_TYPE );
    vendorID = _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID );
    deviceID = _pci_cfg_read_w( bus, device, function, PCI_DEVICE_ID );
    bar0 = _pci_cfg_read_l( bus, device, function, PCI_BAR0 );
    bar1 = _pci_cfg_read_l( bus, device, function, PCI_BAR1 );
    
    // check if the device is a secondary bus
    // if so, check the bus for connected devices
    if(( class == 0x06 ) && ( subClass == 0x04 ) && ( headerType == 0x01 )) {
        secondaryBus = _pci_cfg_read_b( bus, device, function, PCI_SEC_BUS );
        _pci_check_bus( secondaryBus );
    }

    // store device information
    PCIDevice *newDevice = &_pci_dev_list[_pci_num_dev];
    newDevice->bus = bus;
    newDevice->device = device;
    newDevice->function = function;
    newDevice->class = class;
    newDevice->subClass = subClass;
    newDevice->progIF = progIF;
    newDevice->vendorID = vendorID;
    newDevice->deviceID = deviceID;
    newDevice->bar1 = bar1;

    // read base address 0 properly
    if( bar0 & 1 ) {                        // I/O Space BAR layout
        newDevice->bar0 = bar0 & 0xFFFFFFFC;
    } else {                                // Memory Space BAR Layout
        if( (bar0 & 0b110) >> 1 == 0 ) {        // BAR is 32-bits wide
            newDevice->bar0 = bar0 & 0xFFFFFFF0;
        } else {                                // Should not happen
            newDevice->bar0 = 0xFFFFFFFF;
            __cio_puts( " PCI: unsupported base address field" );
        }
    }
    
    _pci_num_dev ++;
}

//
// _pci_check_device() - check the device number for connected devices
//
// Parameters:
//    bus           bus number
//    device        device number
//
static void _pci_check_device( uint8 bus, uint8 device ) {
    uint8 function = 0;

    uint16 vendorID = _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID );
    
    if( vendorID == 0xFFFF ) return;        // Device doesn't exist
    
    _pci_check_function( bus, device, function );

    // check if multifunction device
    // if so, check all functions
    uint8 headerType = _pci_cfg_read_b( bus, device, function, PCI_HEADER_TYPE );
    if(( headerType & 0x80 ) != 0) {
        for( function = 1; function < 8; function++ ) {
            if( _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID ) != 0xFFFF) {
                _pci_check_function( bus, device, function );
            }
        }
    }
}

//
// _pci_check_bus() - check the bus for connected devices
//
// Parameters:
//    bus           bus number
//
static void _pci_check_bus( uint8 bus ) {
    uint8 device;

    // check all devices on the bus
    for( device = 0; device < 32; device++ ) {
        _pci_check_device( bus, device );
    }
}

//
// _pci_check_buses() - check all buses for connected devices
//
static void _pci_check_buses(void) {
    uint8 function;
    uint8 bus;

    // check if multifunction device
    // if so, check all functions as buses
    // if not check bus 0
    uint8 headerType = _pci_cfg_read_b( 0, 0, 0, PCI_HEADER_TYPE );
    if(( headerType & 0x80 ) == 0) {
        _pci_check_bus( 0 );
    } else {
        for( function = 0; function < 8; function++ ) {
            if( _pci_cfg_read_w( 0, 0, function, PCI_VENDOR_ID ) != 0xFFFF ) break;
            bus = function;
            _pci_check_bus( bus );
        }
    }
}

/*
** PUBLIC FUNCTIONS
*/

//
// _pci_init() - checks all buses for connected devices
//
void _pci_init() {
    _pci_check_buses();
    __cio_puts( " PCI" );
}

//
// _pci_dev_class() - get the first device descriptor matching the given class
//
// Parameters:
//    class         class code
//    subclass      subclass code
//    progIF        programming interface code
//
// Returns:
//    a pointer to the device descriptor
//
PCIDevice *_pci_dev_class( uint8 class, uint8 subClass, uint8 progIF ) {
    uint8 i;

    for( i = 0; i < _pci_num_dev; i++ ) {
        PCIDevice *dev = &_pci_dev_list[i];
        if( dev->class == class && dev->subClass == subClass && dev->progIF == progIF ) {
            return( dev );
        }
    }

    return( NULL );
}

//
// _pci_dev_vendor() - get the first device descriptor matching the given
//                     parameters.
//
// Parameters:
//    vendor        device vendor code
//    deviceID      device id code
//
// Returns:
//    a pointer to the device descriptor
//
PCIDevice *_pci_dev_vendor( uint16 vendor, uint16 deviceID ) {
    uint8 i;

    for( i = 0; i < _pci_num_dev; i++ ) {
        PCIDevice *dev = &_pci_dev_list[i];
        if( dev->vendorID == vendor && dev->deviceID == deviceID ) {
            return( dev );
        }
    }

    return( NULL );
}

//
// _pci_cfg_read_l() - read a long word from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the long word read
//
uint32 _pci_cfg_read_l( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
    uint32 address;
    uint32 lbus  = (uint32)bus;
    uint32 ldevice = (uint32)device;
    uint32 lfunction = (uint32)function;

    address = (uint32)((lbus << 16) | ((ldevice & 0x1F) << 11) |
              ((lfunction & 0x07) << 8) | (offset & 0xFC) | ((uint32)0x80000000));
 
    // write out the address
    __outl( 0xCF8, address );
    // read in the data
    return( __inl( 0xCFC ) );
}

//
// _pci_cfg_read_w() - read a word from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the word read
//
uint16 _pci_cfg_read_w( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
    uint32 tmp = _pci_cfg_read_l( bus, device, function, offset );
    // (offset & 2) * 8) = 0 will choose the first word of the 32 bits register
    return( (uint16)((tmp >> ((offset & 2) * 8)) & 0xFFFF) );
}

//
// _pci_cfg_read_b() - read a byte from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the byte read
//
uint8 _pci_cfg_read_b( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
    uint16 tmp = _pci_cfg_read_w( bus, device, function, offset );
    // use offset to pick the right byte in tmp
    return(( tmp >> ((offset & 1) * 8)) & 0xFF );
}

//
// _pci_cfg_write_l() - write a long word in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_l( uint8 bus, uint8 device, uint8 function, uint8 offset, uint32 value ) {
    uint32 address;
    uint32 lbus  = (uint32)bus;
    uint32 ldevice = (uint32)device;
    uint32 lfunction = (uint32)function;

    address = (uint32)((lbus << 16) | ((ldevice & 0x1F) << 11) |
              ((lfunction & 0x07) << 8) | (offset & 0xFC) | ((uint32)0x80000000));

	__outl( 0xCF8, address );
	__outl( 0xCFC, value );
}

//
// _pci_cfg_write_w() - write a word in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_w( uint8 bus, uint8 device, uint8 function, uint8 offset, uint16 value ) {
    uint32 newValue;
    uint16 content;
    
    content = _pci_cfg_read_w( bus, device, function, offset^2 );
    newValue = (uint32)(content << ((offset^2)&2)*8) | (value << ((offset&2)*8));

	_pci_cfg_write_l( bus, device, function, offset, newValue);
}

//
// _pci_cfg_write_b() - write a byte in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_b( uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 value ) {
    uint16 newValue;
    uint8 content;

    content = _pci_cfg_read_b( bus, device, function, offset^1 );
    newValue = (uint16)(content << ((offset^1)&1)*8) | (value << ((offset&1)*8));
	
    _pci_cfg_write_w( bus, device, function, offset, newValue);
}

//
// _pci_set_command() - set the device command register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    value         value to be written
//
void _pci_set_command( uint8 bus, uint8 device, uint8 function, uint16 value ) {
    _pci_cfg_write_w( bus, device, function, PCI_COMMAND, value );
}

//
// _pci_set_interrupt() - set the device interrupt line and pin registers
//
// Parameters:
//    bus               bus number
//    device            device number
//    function          function number
//    interruptPin      value to be written in interruptPin
//    interruptLine     value to be written in interruptLine
//
void _pci_set_interrupt( uint8 bus, uint8 device, uint8 function, uint8 interruptPin, uint8 interruptLine ) {
    uint16 interrupt = (uint16)((interruptPin << 8) | interruptLine);
    _pci_cfg_write_w( bus, device, function, 0x3C, interrupt );
}

//
// _pci_get_interrupt_line() - get the device interrupt line register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//
// Returns:
//    the interrupt line register
//
uint8 _pci_get_interrupt_line( uint8 bus, uint8 device, uint8 function ) {
    return _pci_cfg_read_b( bus, device, function, 0x3C );
}

//
// _pci_get_status() - get the device status register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//
// Returns:
//    the status register
//
uint16 _pci_get_status( uint8 bus, uint8 device, uint8 function ) {
    return( _pci_cfg_read_w( bus, device, function, PCI_STATUS ));
}

//
// _pci_dump_all() - dump all device information to the console
//
void _pci_dump_all() {
    for(int i = 0; i<_pci_num_dev; i++) {
        PCIDevice *dev = &_pci_dev_list[i];
        __cio_printf( "%d: VendorID: %04x DeviceID: %04x Class: %02x SubClass: %02x ProgIF: %02x\n", i, dev->vendorID,
                  dev->deviceID, dev->class, dev->subClass, dev->progIF);
    }
}

//
// _pci_dump_header() - dump device header information to the console
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    nlines        number of header lines to dump
//
void _pci_dump_header( uint8 bus, uint8 device, uint8 function, uint8 nlines ) {
    uint8 i;
    __cio_puts( "\nHEADER CONTENTS:\n" );
    for( i = 0; i<nlines; i++) {
        __cio_printf( "LINE%d: %08x\n", i, _pci_cfg_read_l( bus, device, function, i*4 ));
    }
}
