
#define __SP_KERNEL__

#include "klib.h"

#include "usb_ehci.h"

#include "queues.h"

PCIDev* usbController;

USBDev _usb_dev_list[MAX_USB_DEVICES];

struct _usb_td_s _usb_tds[MAX_TDS];

Queue _usb_free_tdq;

uint32 base_addr;
uint32* frame_list;

void _usb_isr( int vector, int ecode ) {

  __cio_printf("V: %x, C: %x", vector, ecode);

}

uint32 _usb_read_long( uint8 offset ) {
  return (uint32)(__inl(base_addr + (uint32) offset));
}

uint16 _usb_read_word( uint8 offset ) {
  return (uint16)(__inw(base_addr + (uint32)offset));
}

uint8 _usb_read_byte( uint8 offset ) {
  return (uint8)(__inb(base_addr + (uint32)offset));
}

void _usb_write_long( uint8 offset, uint32 data ){
  __outl(base_addr + (uint32)offset, data);
}

void _usb_write_word( uint8 offset, uint16 data ) {
  __outw(base_addr + (uint32)offset, data);
}

void _usb_write_byte( uint8 offset, uint8 data ) {
  __outb(base_addr + (uint32)offset, data);
}

void _usb_ehci_init( PCIDev* pciDev ) {

  usbController = pciDev;

  base_addr = usbController->bar0 & 0xFFFFFFFC; // Base address for controller

  frame_list = (uint32 *)_kalloc_page(1); // Allocate space for frame list

  _usb_write_long( 0x08, (uint32)frame_list ); // Tell the controller where that list is

  if(frame_list != ((uint32 *)_usb_read_long( 0x08 ))) {
    __panic("USB controller frame list address is not accurate");
  }

  for(int i=0; i<1024;i++) { // Set the pointers to be invalid
    frame_list[i] = 0x00000001;
  }

  _usb_free_tdq = _queue_alloc(NULL); // Make a queue for free TD's

  for(int i=0;i<MAX_TDS;++i) { // Load queue with free td's
    _queue_enque(_usb_free_tdq, (void *)&_usb_tds[i]);
  }

  __install_isr( usbController->interrupt, _usb_isr ); // Install ISR

  _usb_enable_interrupts(true, true, true, true); // Enable interrupts

  __cio_puts( " USB_UHCI" );

}

void _usb_enable_interrupts( bool shor, bool ioc, bool ric, bool time ) {
  uint8 data = 0x00;
  data = (shor << 3) | (ioc << 2) | (ric << 1) | time;
  _usb_write_byte(0x04, data);
}

void _usb_ehci_status( void ) {
  uint16 curr_frame_ind, p1, p2;
  uint32 addr;

  curr_frame_ind = _usb_read_word( 0x00 );
  addr = _usb_read_word( 0x02 );
  p1 = _usb_read_word( 0x10 );
  p2 = _usb_read_word( 0x12 );
  __cio_printf("C: %x S: %x P1: %x P2: %x", curr_frame_ind, addr, p1, p2);

}

struct _usb_td_s* _usb_ehci_free_td( void ) {
  return (struct _usb_td_s*) _queue_front(_usb_free_tdq);
}

void _usb_ehci_schedule_td( struct _usb_td_s* td ) {
  frame_list[0] = (uint32)td & 0xFFFFFFF0;
}

void _usb_ehci_schedule_qh( struct _usb_qh_s* qh ) {
  frame_list[0] = ((uint32)qh & 0xFFFFFFF0) | 0x2;
}
