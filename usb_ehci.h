/*
  The USB system



 */

#ifndef _USB_EHCI_H_
#define _USB_EHCI_H_

#define MAX_USB_DEVICES 15

#define MAX_TDS 512

#include "common.h"
#include "pci.h"

#ifndef __SP_ASM__

typedef struct _usb_dev_s {
  uint8 id;
} USBDev;

struct _usb_td_s {
  uint32 link_pointer;
  uint32 control_status;
  uint32 token;
  uint32 buffer_pointer;
};

struct _usb_qh_s {
  uint32 head_pointer;
  uint32 element_pointer;
};

void _usb_ehci_init( PCIDev* pciDev );

void _usb_enable_interrupts( bool shor, bool ioc, bool ric, bool time );

void _usb_ehci_status( void );

void _usb_ehci_schedule_td( struct _usb_td_s* td);

void _usb_ehci_schedule_qh( struct _usb_qh_s* qh );

#endif

#endif
