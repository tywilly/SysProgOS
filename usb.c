/*
** File: usb.h
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: The overall USB System.
**
** USB consists of multiple layers of hardware and software.
**
** At the top is the Host Controller(usb_ehci.c usb_ehci.h). The Host Controller
*is the master in the system.
** It controls all flows of data between host and devices.
**
** Next is the USB driver(usbd.c usbd.h).
** The USB driver is the gateway from the hardware world to the software world.
** All software will communicate through here to perform an action
**
** Finally the device driver handles control over a specific device
*/

#define __SP_KERNEL__

#include "common.h"

#include "pci.h"
#include "usb.h"
#include "usb_ehci.h"
#include "usb_ms.h"
#include "usbd.h"

/**
** Initialize the USB system
*/
void _usb_init(void) {

  __cio_puts(" [");

  _usb_ehci_init(
      _pci_get_device_class(0x0C, 0x03, 0x20)); // Init any EHCI controllers

  _usbd_init(); // Init the USB driver

  _usb_ms_init(); // Init USB Mass Storage driver

  __cio_puts(" ] USB");
}

/**
** Print the status of the USB system to the console
**/
void _usb_status(void) { _usb_ehci_status(); }
