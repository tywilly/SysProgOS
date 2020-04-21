
#define __SP_KERNEL__

#include "common.h"

#include "usb.h"
#include "usb_ehci.h"
#include "usbd.h"
#include "pci.h"


void _usb_init( void ) {

  __cio_puts( " [" );

  _usb_ehci_init(_pci_get_device_class( 0x0C, 0x03, 0x20 )); // Init any EHCI controllers
                                                             //

  _usbd_init(); // Init the USB driver

  __cio_puts( " ] USB" );

}

void _usb_status( void ) {
  _usb_ehci_status();
}
