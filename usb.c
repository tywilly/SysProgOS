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
static uint8 _usb_eecp;
static uint16 _usb_pci_command; 
static uint32 _usb_base;
static uint32 _usb_op_base;

// uint16 _usb_read_word( uint8 offset ) {
//   return (uint16)(__inw(base_addr + (uint32)offset));
// }

// void _usb_write_word( uint8 offset, uint16 data ) {
//   __outw(base_addr + (uint32)offset, data);
// }

// void _usb_write_byte( uint8 offset, uint8 data ) {
//   __outb(base_addr + (uint32)offset, data);
// }
// void _usb_uhci_init( PCIDev* pciDev ) {

//   usbController = pciDev;

//   base_addr = usbController->bar4 & 0xFFFFFFFC;
//   frame_list_addr = _usb_read_word( 0x08 );
//   frame_list = (uint32 *)frame_list_addr;

//   __install_isr( usbController->interrupt, _usb_isr );

//   _usb_enable_interrupts(true, true, true, true);

//   __cio_puts( " USB_UHCI" );

// }

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

static uint32 _usb_read_l( uint32 addr, uint32 offset ) {
    return( __inl( addr + (offset & 0xFFFFFFFC) ) );
}

static uint16 _usb_read_w( uint32 addr, uint32 offset ) {
    uint32 l = _usb_read_l( addr, offset );
    return( (uint16)((l >> ((offset & 2) * 8)) & 0xFFFF) );
}

static uint8 _usb_read_b( uint32 addr, uint32 offset ) {
    uint16 w = _usb_read_w( addr, offset );
    return( (uint8)((w >> ((offset & 1) * 8)) & 0xFF) );
}

/**
 * Public funtions
*/

void _usb_init( void ) {
    uint8 eecpOffset;

    __cio_puts( "\n--------------------USB SHIT--------------------\n" );

    // Enable Bus Master and Memory Space
    _usb_set_pci_command( 0x0146 );

    PCIDevice *dev = _pci_dev_class( USB_CLASS, USB_SUBCLASS, USB_EHCI_PROGIF );

    assert( dev != NULL );
    assert( dev->bar0 != 0xFFFFFFFF );

    __cio_printf( "BAR0: %x\n", dev->bar0 );

    // store device location
    _usb_bus = dev->bus;
    _usb_device = dev->device;
    _usb_function = dev->function;

    // store capability registers and operation registers base address
    _usb_base = dev->bar0;
    _usb_op_base = _usb_base + _usb_read_b( _usb_base, USB_CAP_LEN );

    __cio_printf( "READ AT BAR0 %08x\n", __inl( _usb_base ));

    // fetch EHCI Extended Capabilities Pointer (EECP)
    eecpOffset = (uint8)((_usb_read_l( _usb_base, USB_HCC_PARAMS ) >> 8) & 0xFF);

    // check if BIOS owns the controller; if true, get ownership
    if( eecpOffset >= 0x40 ) {
        _usb_eecp = _pci_cfg_read_l( _usb_bus, _usb_device, _usb_function, eecpOffset );
        // _pci_cfg_write_b( _usb_bus, _usb_device, _usb_function, eecpOffset+ )
        __cio_printf( "EECP = %x, TODO: BIOS thing\n", _usb_eecp );
    }
    __cio_printf( "PCI STATUS: %x\n", _pci_get_status( _usb_bus, _usb_device, _usb_function ));
    __cio_printf( "CAPLEN: %x\n", _usb_read_b( _usb_base, USB_CAP_LEN ) );
    __cio_printf( "OP BASE: %x\n", _usb_op_base );
    __cio_printf( "HCIVERSION: %x\n", _usb_read_w( _usb_base, USB_HCI_VERSION ));
    __cio_printf( "HCSPARAMS: %x\n", _usb_read_l( _usb_base, USB_HCS_PARAMS ));
    __cio_printf( "HCCPARAMS: %x\n", _usb_read_l( _usb_base, USB_HCC_PARAMS ));
    
    
    // stop and reset controller
    // command = __inl( _usb_op_base + USB_CMD );
    // __cio_printf( "before change: %x\n", command );
    // command &= 0xFFFFFFFE;
    // __cio_printf( "value sent: %x\n", command );
    // __outl( _usb_op_base + USB_CMD, command );
    // command = __inl( _usb_op_base + USB_CMD );
    // __cio_printf( "after change: %x\n", command );

    __cio_puts( "--------------------USB SHIT--------------------\n" );
    return( 94 );
}
