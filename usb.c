#define __SP_KERNEL__

#include "common.h"

#include "usb.h"

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

static uint8 _usb_bus;
static uint8 _usb_device;
static uint8 _usb_function;
static uint8 _usb_eecp_offset;
static uint32 _usb_eecp;
static uint16 _usb_pci_command; 
static uint32 _usb_base;
static uint32 _usb_op_base;

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

    // stop and reset the controller
    _usb_command_disable(1);
    _usb_command_enable(2);

    // setup queue head

    // start the controller

    __cio_puts( "--------------------USB SHIT--------------------\n" );
    return( 94 );
}
