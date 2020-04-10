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
static uint32 _usb_base_addr;
static uint32 _usb_op_base_addr;

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

void _usb_init( void ) {
    // uint32 command;

    __cio_puts( "\n--------------------USB SHIT--------------------\n" );

    // Enable Bus Master and Memory Space
    _pci_command_enable( _usb_bus, _usb_device, _usb_function, 0x0006 );

    PCIDevice *dev = _pci_dev_class( USB_CLASS, USB_SUBCLASS, USB_EHCI_PROGIF );

    assert( dev );

    __cio_printf( "BAR0: %x\n", dev->bar0 );

    _usb_bus = dev->bus;
    _usb_device = dev->device;
    _usb_function = dev->function;
    // store capability registers and operation registers base address
    _usb_base_addr = dev->bar0;
    _usb_op_base_addr = _usb_base_addr + __inb( _usb_base_addr + USB_CAP_LEN );

    // fetch EHCI Extended Capabilities Pointer (EECP)
    _usb_eecp = (uint8)(__inl( _usb_base_addr + USB_HCC_PARAMS ) >> 8) & 0xFF;

    if( _usb_eecp >= 40 ) {
        __cio_printf( "TODO: Check if BIOS owns the controller; if true, get ownership\n" );
    }

    // __cio_printf( "CAP REG: %x\n", _usb_base_addr );
    // __cio_printf( "OP REG: %x\n", _usb_op_base_addr );
    // __cio_printf( "HCIVERSION: %x\n", __inw(_usb_base_addr + USB_HCI_VERSION ));

    _pci_command_disable( _usb_bus, _usb_device, _usb_function, 0x6 );
    __cio_printf( "THIS DOES NOT WORK COMMAND: %x\n", _pci_get_command( _usb_bus, _usb_device, _usb_function ));
    
    // stop and reset controller
    // command = __inl( _usb_op_base_addr + USB_CMD );
    // __cio_printf( "before change: %x\n", command );
    // command &= 0xFFFFFFFE;
    // __cio_printf( "value sent: %x\n", command );
    // __outl( _usb_op_base_addr + USB_CMD, command );
    // command = __inl( _usb_op_base_addr + USB_CMD );
    // __cio_printf( "after change: %x\n", command );

    __cio_puts( "--------------------USB SHIT--------------------\n" );
    return( 94 );
}
