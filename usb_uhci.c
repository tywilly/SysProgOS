
#define __SP_KERNEL__

#include "klib.h"

#include "usb_uhci.h"

#include "queues.h"

PCIDev* usbController;

USBDev _usb_dev_list[MAX_USB_DEVICES];

uint32 base_addr = 0;
uint32 frame_list_addr;
const uint32* frame_list;

void _usb_isr( int vector, int ecode ) {

  __cio_printf("V: %x, C: %x", vector, ecode);

}

uint16 _usb_read_word( uint8 offset ) {
  return (uint16)(__inw(base_addr + (uint32)offset));
}

void _usb_write_word( uint8 offset, uint16 data ) {
  __outw(base_addr + (uint32)offset, data);
}

void _usb_write_byte( uint8 offset, uint8 data ) {
  __outb(base_addr + (uint32)offset, data);
}
void _usb_uhci_init( PCIDev* pciDev ) {

  usbController = pciDev;

  base_addr = usbController->bar4 & 0xFFFFFFFC;
  frame_list_addr = _usb_read_word( 0x08 );
  frame_list = (uint32 *)frame_list_addr;

  __install_isr( usbController->interrupt, _usb_isr );

  _usb_enable_interrupts(true, true, true, true);

  __cio_puts( " USB_UHCI" );

}

void _usb_enable_interrupts( bool shor, bool ioc, bool ric, bool time ) {
  uint8 data = 0x00;
  data = (shor << 3) | (ioc << 2) | (ric << 1) | time;
  _usb_write_byte(0x04, data);
}

void _usb_status( void ) {
  uint16 curr_frame_ind, p1, p2;
  uint32* addr;

  curr_frame_ind = _usb_read_word( 0x06 ) & 0x3FF;
  addr = &frame_list[curr_frame_ind];
  p1 = _usb_read_word( 0x10 );
  p2 = _usb_read_word( 0x12 );
  __cio_printf("FA: %x CF: %x P1: %x P2: %x", frame_list, addr, p1, p2);
}
