
#define __SP_KERNEL__

#include "klib.h"

#include "usb.h"

#include "queues.h"
#include "pci.h"

PCIDev* usbController;

USBDev _usb_dev_list[MAX_USB_DEVICES];

uint32 base_addr = 0;
uint32 frame_list_addr;

void _usb_isr( int vector, int ecode ) {

  __cio_printf("V: %x, C: %x", vector, ecode);

}

void _usb_init() {

  usbController = _pci_get_device_class( 0x0C, 0x03, 0x00 );

  base_addr = usbController->bar4 & 0xFFFFFFFC;
  frame_list_addr = _usb_read_word( 0x08 );

  __install_isr( usbController->interrupt, _usb_isr );

  _usb_enable_interrupts(true, true, true, true);

  __cio_puts( " USB" );

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

void _usb_enable_interrupts( bool shor, bool ioc, bool ric, bool time ) {
  uint8 data = 0x00;
  data = (shor << 3) | (ioc << 2) | (ric << 1) | time;
  _usb_write_byte(0x04, data);
}

void _usb_status( void ) {
  uint16 curr_frame, p1, p2;

  curr_frame = _usb_read_word( 0x06 ) & 0x3FF;
  p1 = _usb_read_word( 0x10 );
  p2 = _usb_read_word( 0x12 );
  __cio_printf("FA: %x CF: %d P1: %x P2: %x", frame_list_addr, curr_frame, p1, p2);
}
