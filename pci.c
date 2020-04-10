#define __SP_KERNEL__

#include "common.h"

#include "pci.h"

// Header commons offset
// register 	offset 	bits 31-24 	bits 23-16 	bits 15-8 	bits 7-0
// 00 	00 	Device ID 	Vendor ID
// 01 	04 	Status 	Command
// 02 	08 	Class code 	Subclass 	Prog IF 	Revision ID
// 03 	0C 	BIST 	Header type 	Latency Timer 	Cache Line Size

// PCI Device Structure
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

#define PCI_BAR0            0x10
#define PCI_SEC_BUS         0x19

/**
 * Private variables
*/

static PCIDevice _pci_dev_list[MAX_PCI_DEVICES];
static int _pci_num_dev = 0;

/**
 * Resolving cyclic function dependencies
*/

static void _pci_check_bus( uint8 );

/**
 * Private functions
*/

static uint32 _pci_cfg_read_l( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
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

static uint16 _pci_cfg_read_w( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
    uint32 tmp = _pci_cfg_read_l( bus, device, function, offset );
    // (offset & 2) * 8) = 0 will choose the first word of the 32 bits register
    return( (uint16)((tmp >> ((offset & 2) * 8)) & 0xFFFF) );
}

static uint8 _pci_cfg_read_b( uint8 bus, uint8 device, uint8 function, uint8 offset ) {
    uint16 tmp = _pci_cfg_read_w( bus, device, function, offset );
    // use offset to pick the right byte in tmp
    return(( tmp >> ((offset & 1) * 8)) & 0xFF );
}

static void _pci_check_function( uint8 bus, uint8 device, uint8 function ) {
    uint32 bar0;
    uint16 vendorID;
    uint16 deviceID;
    uint8 class;
    uint8 subClass;
    uint8 progIF;
    uint8 secondaryBus;
    uint8 headerType;

    class = _pci_cfg_read_b( bus, device, function, PCI_CLASS );
    subClass = _pci_cfg_read_b( bus, device, function, PCI_SUBCLASS );
    progIF = _pci_cfg_read_b( bus, device, function, PCI_PROG_IF );
    headerType = _pci_cfg_read_b( bus, device, function, PCI_HEADER_TYPE );
    vendorID = _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID );
    deviceID = _pci_cfg_read_w( bus, device, function, PCI_DEVICE_ID );
    bar0 = _pci_cfg_read_l( bus, device, function, PCI_BAR0 );
    
    if(( class == 0x06 ) && ( subClass == 0x04 ) && ( headerType == 0x01 )) {
        secondaryBus = _pci_cfg_read_b( bus, device, function, PCI_SEC_BUS );
        _pci_check_bus( secondaryBus );
    }

    PCIDevice *newDevice = &_pci_dev_list[_pci_num_dev];
    newDevice->bus = bus;
    newDevice->device = device;
    newDevice->function = function;
    newDevice->class = class;
    newDevice->subClass = subClass;
    newDevice->progIF = progIF;
    newDevice->vendorID = vendorID;
    newDevice->deviceID = deviceID;
    if( bar0 & 1 ) {                // I/O Space BAR layout
        newDevice->bar0 = bar0 & 0xFFFFFFFC;
    } else {                        // Memory Space BAR Layout
        if( (bar0 & 0b110) >> 1 == 0 ) { // BAR is 32-bits wide
            newDevice->bar0 = bar0 & 0xFFFFFFF0;
        } else {                        // Should not happen
            newDevice->bar0 = 0xFFFFFFFF;
            __cio_puts( " PCI: unsupported base address field" );
        }
    }
    
    _pci_num_dev ++;
}

static void _pci_check_device( uint8 bus, uint8 device ) {
    uint8 function = 0;

    uint16 vendorID = _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID );
    
    if( vendorID == 0xFFFF ) return;        // Device doesn't exist
    
    _pci_check_function( bus, device, function );

    uint8 headerType = _pci_cfg_read_b( bus, device, function, PCI_HEADER_TYPE );
    if(( headerType & 0x80 ) != 0) {
        for( function = 1; function < 8; function++ ) {
            if( _pci_cfg_read_w( bus, device, function, PCI_VENDOR_ID ) != 0xFFFF) {
                _pci_check_function( bus, device, function );
            }
        }
    }
}

static void _pci_check_bus( uint8 bus ) {
    uint8 device;

    for( device = 0; device < 32; device++ ) {
        _pci_check_device( bus, device );
    }
}

static void _pci_check_buses() {
    uint8 function;
    uint8 bus;

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

/**
 * Public functions
*/

void _pci_init() {
    _pci_check_buses();
    __cio_puts( " PCI" );
}

void _pci_cfg_write_l( uint8 bus, uint8 device, uint8 function, uint8 offset, uint32 value ) {
    uint32 address;
    uint32 lbus  = (uint32)bus;
    uint32 ldevice = (uint32)device;
    uint32 lfunction = (uint32)function;

    address = (uint32)((lbus << 16) | ((ldevice & 0x1F) << 11) |
              ((lfunction & 0x07) << 8) | (offset & 0xFC) | ((uint32)0x80000000));

	__outl( address, 0xCF8 );
	__outl( value, 0xCFC );
}

void _pci_cfg_write_w( uint8 bus, uint8 device, uint8 function, uint8 offset, uint16 value ) {
    uint32 newValue;
    uint16 content;
    
    content = _pci_cfg_read_w( bus, device, function, offset^2 );
    newValue = (uint32)(content << ((offset^2)&2)*8) | (value << ((offset&2)*8));

	_pci_cfg_write_l( bus, device, function, offset, newValue);
}

void _pci_cfg_write_b( uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 value ) {
    uint16 newValue;
    uint8 content;

    content = _pci_cfg_read_b( bus, device, function, offset^1 );
    newValue = (uint16)(content << ((offset^1)&1)*8) | (value << ((offset&1)*8));
	
    _pci_cfg_write_w( bus, device, function, offset, newValue);
}

uint16 _pci_get_command( uint8 bus, uint8 device, uint8 function ) {
    return( _pci_cfg_read_w( bus, device, function, PCI_COMMAND ));
}

/**
 * Sets the command register bits specified by the command argument to 1
 * Before:  Register=0x0FFE     Argument=0x0001
 * After:   Register=0x0FFF
 */
void _pci_command_enable( uint8 bus, uint8 device, uint8 function, uint16 command ) {
    uint16 commandReg;

    commandReg = _pci_cfg_read_w( bus, device, function, PCI_COMMAND );
    commandReg = commandReg | command;

    _pci_cfg_write_w( bus, device, function, PCI_COMMAND, commandReg );
}

/**
 * Sets the command register bits specified by the command argument to 0
 * Before:  Register=0x0FFF     Argument=0x0001
 * After:   Register=0x0FFE
 */
void _pci_command_disable( uint8 bus, uint8 device, uint8 function, uint16 command ) {
    uint16 commandReg;

    commandReg = _pci_cfg_read_w( bus, device, function, PCI_COMMAND );
    commandReg = commandReg & (~command);

    // _pci_cfg_write_w( bus, device, function, PCI_COMMAND, commandReg );
    
    __cio_printf("debug command read before %x\n", _pci_cfg_read_l( bus, device, function, PCI_COMMAND ));
    __cio_printf("debug command written %x\n", (uint32)(commandReg | 0xFFFF0000));
    _pci_cfg_write_l( bus, device, function, PCI_COMMAND, (uint32)(commandReg | 0xFFFF0000));
    __cio_printf("debug command read after %x\n", _pci_cfg_read_l( bus, device, function, PCI_COMMAND ));
}

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

void _pci_dump_all() {
    for(int i = 0; i<_pci_num_dev; i++) {
        PCIDevice *dev = &_pci_dev_list[i];
        __cio_printf( "%d: VendorID: %04x DeviceID: %04x Class: %02x SubClass: %02x ProgIF: %02x\n", i, dev->vendorID,
                  dev->deviceID, dev->class, dev->subClass, dev->progIF);
    }
}
