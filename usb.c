
#define __SP_KERNEL__

#include "common.h"
#include "klib.h"

#include "usb.h"

#include "queues.h"

void _usb_isr( int vector, int ecode ) {

}

void _usb_init() {



  __cio_puts( " USB" );

}
