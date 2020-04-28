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
** At the top is the Host Controller(usb_ehci.c usb_ehci.h). The Host Controller is the master in the system.
** It controls all flows of data between host and devices.
**
** Next is the USB driver(usbd.c usbd.h).
** The USB driver is the gateway from the hardware world to the software world.
** All software will communicate through here to perform an action
**
** Finally the device driver handles control over a specific device
*/

#ifndef _USB_H_
#define _USB_H_

/**
** Initialize the USB system
*/
void _usb_init( void );

/**
** Print the status of the USB System to the console
**
*/
void _usb_status( void );
#endif
