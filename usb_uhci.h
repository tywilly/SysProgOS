/*
  The USB system



 */

#ifndef _USB_H_
#define _USB_H_

#define MAX_USB_DEVICES 15

#include "common.h"

#ifndef __SP_ASM__

typedef struct _usb_dev_s {
  uint8 id;
} USBDev;

struct _usb_tran_desc_s {
  uint32 link_pointer;
  uint32 control_status;
  uint32 token;
  uint32 buffer_pointer;
};

void _usb_init( void );

uint16 _usb_read_word( uint8 offset );

void _usb_enable_interrupts( bool shor, bool ioc, bool ric, bool time );

void _usb_status( void );

#endif

#endif
