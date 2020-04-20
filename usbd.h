

#ifndef _USBD_H_
#define _USBD_H_

#define USB_OUT_PACKET 0x0
#define USB_IN_PACKET 0x1
#define USB_SETUP_PACKET 0x2

#define USB_MAX_PIPES 10
#define USB_MAX_DEVICES 128

#define USB_FULL_SPEED 0x0
#define USB_LOW_SPEED 0x1
#define USB_HIGH_SPEED 0x2

#include "common.h"


#ifndef __SP_ASM__


void _usbd_enumerate_devices( void );

#endif
#endif
