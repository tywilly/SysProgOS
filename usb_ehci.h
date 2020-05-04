/*
** File: usb_ehci.h
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: The module to control a USB EHCI host controller
 */

#ifndef _USB_EHCI_H_
#define _USB_EHCI_H_

#define MAX_TDS 20
#define ASYNC_LIST_SIZE 20

// Interrupt types
#define USB_ERROR_INTERRUPT 0x1
#define USB_PORT_CHANGE_INTERRUPT 0x2
#define USB_FRAME_LIST_INTERRUPT 0x3
#define USB_HOST_ERROR_INTERRUPT 0x4
#define USB_ASYNC_INTERRUPT 0x5

#include "common.h"
#include "pci.h"

#ifndef __SP_ASM__

/**
** A data structure representing an isosyncronus
** transfer descrtiptor
 */
struct _usb_itd_s
{
  uint32 link_pointer;   // Pointer to next iTD
  uint32 control_status; // Control and status field
  uint32 token;          // token field of packet
  uint32 buffer_pointer; // Buffer for the packet
};

/**
** A data structure representing a queue head transfer
** descriptor
 */
struct _usb_qtd_s
{
  uint32 next_ptr; // Pointer to next qTD
  uint32 alt_ptr;  //
  uint32 token;    // Token field
  uint32 ptr0;     // pointer to page 1 of buffer
  uint32 ptr1;     // .
  uint32 ptr2;     // .
  uint32 ptr3;     // .
  uint32 ptr4;
};

/**
** A data structure representing a queue head
 */
struct _usb_qh_s
{
  uint32 dword0; // Horizontal pointer
  uint32 dword1; //
  uint32 dword2;
  uint32 dword3;
  uint32 dword4;
  uint32 dword5;
  uint32 dword6;
  uint32 dword7;
  uint32 dword8;
  uint32 dword9;
  uint32 dword10;
  uint32 dword11;
  uint32 dword12;
  uint32 dword13;
  uint32 dword14;
  uint32 dword15;
};

/**
** Set the callback handler for interrupts
**
** @param callback  A pointer to the handler function
 */
void _usb_ehci_isr_callback(void (*callback)(void));

/**
** Initialize the USB controller
**
** @param pciDev  The PCI device that the controller resides on
 */
void _usb_ehci_init(PCIDev *pciDev);

/**
** Enable interrupts for the USB controller
 */
void _usb_enable_interrupts(void);

void _usb_ehci_status(void);

/**
** Get a free qTD from the controller
**
** @returns a pointer to a qTD struct
 */
struct _usb_qtd_s *_usb_ehci_free_qtd(void);

/**
** Release a qTD
**
** @param qtd  A pointer to a qTD struct
 */
void _usb_ehci_release_qtd(struct _usb_qtd_s *qtd);

/**
** Get a free queue head
**
** @returns a pointer to a queue head struct
 */
struct _usb_qh_s *_usb_ehci_free_qh(void);

/**
** Release a queue head
**
** @param qh  A pointer to a queue head struct
 */
void _usb_ehci_release_qh(struct _usb_qh_s *qh);

/**
** Schedule an isosyncronus transfer
**
** @param td  The transfer descriptor to schedule
 */
void _usb_ehci_schedule_isosync(struct _usb_itd_s *td);

/**
** Schedule a queue head on the async list
**
** @param qh  The queue head to schedule
 */
void _usb_ehci_schedule_async(struct _usb_qh_s *qh);

/**
** Determines if a port change has been detected
**
** @returns true if a port has changed, false otherwise
 */
bool _usb_ehci_has_port_change(void);

/**
** Find which port has changed
**
** @returns the port number of the port that has changed
 */
uint8 _usb_ehci_find_port_change(void);

/**
** Reset a port
**
** @param port  The port number to reset
 */
void _usb_ehci_reset_port(uint8 port);

/**
** Clears an interrupt status if the interrupt has been sent
**
** @param interruptNum  The interrupt to have cleared
 */
void _usb_ehci_clear_interrupt(uint8 interruptNum);

/**
** Get the status of an interrupt
**
** @param interruptNum  The interrupt number to check
** @returns true if the interrupt has been set
 */
bool _usb_ehci_get_interrupt_status(uint8 interruptNum);

#endif

#endif
