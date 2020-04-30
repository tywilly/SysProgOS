/*
** File:    usb.h
**
** Author:  Yann MOULLEC
**
** Contributor: Tyler Wilcox (he helped me when I was stuck)
**
** Description: USB module declarations
*/

#ifndef _USB_H_
#define _USB_H_

#include "pci.h"
#include "queues.h"

/*
** Prototypes
*/

//
// _usb_init() - USB controller initialization. Since the flash
//               drive is plugged in from the start, device
//               enumeration is done in this function directly.
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
void _usb_init( void );

//
// _usb_dump_all() - dump all device information / string 
//     descriptors gathered from the init sequence
//
void _usb_dump_all( void );

#endif