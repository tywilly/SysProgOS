/*
  The USB system



 */

#ifndef _USB_EHCI_H_
#define _USB_EHCI_H_

#define MAX_USB_DEVICES 15

#define MAX_TDS 20
#define ASYNC_LIST_SIZE 20

#define USB_ERROR_INTERRUPT 0x1
#define USB_PORT_CHANGE_INTERRUPT 0x2
#define USB_FRAME_LIST_INTERRUPT 0x3
#define USB_HOST_ERROR_INTERRUPT 0x4
#define USB_ASYNC_INTERRUPT 0x5

#include "common.h"
#include "pci.h"

#ifndef __SP_ASM__

struct _usb_td_s {
  uint32 link_pointer;
  uint32 control_status;
  uint32 token;
  uint32 buffer_pointer;
};

struct _usb_qtd_s {
  uint32 next_ptr;
  uint32 alt_ptr;
  uint32 token;
  uint32 ptr0;
  uint32 ptr1;
  uint32 ptr2;
  uint32 ptr3;
  uint32 ptr4;
};

struct _usb_qh_s {
  uint32 dword0;
  uint32 dword1;
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

void _usb_ehci_isr_callback(void (*callback)(void));

void _usb_ehci_init( PCIDev* pciDev );

void _usb_enable_interrupts( void );

void _usb_ehci_status( void );

struct _usb_qtd_s* _usb_ehci_free_qtd( void );

void _usb_ehci_release_qtd( struct _usb_qtd_s* qtd );

struct _usb_qh_s* _usb_ehci_free_qh( void );

void _usb_ehci_release_qh( struct _usb_qh_s* qh );

void _usb_ehci_schedule_isosync( struct _usb_td_s* td);

void _usb_ehci_schedule_async( struct _usb_qh_s* qh );

bool _usb_ehci_has_port_change( void );

uint8 _usb_ehci_find_port_change( void );

void _usb_ehci_reset_port( uint8 port );

void _usb_ehci_clear_interrupt(uint8 interruptNum);

bool _usb_ehci_get_interrupt_status(uint8 interruptNum);

#endif

#endif
