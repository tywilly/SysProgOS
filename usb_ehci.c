
#define __SP_KERNEL__

#include "klib.h"

#include "usb_ehci.h"

#include "queues.h"



uint8 _usb_num_ports = 0;

PCIDev* usbController;

struct _usb_qtd_s* _usb_tds;
struct _usb_qh_s* async_list;
struct _usb_qh_s* asyncHead;
Queue _usb_free_qtdq;
Queue _usb_free_qhq;

uint32 base_addr;
uint32* frame_list;

void _usb_isr( int vector, int ecode ) {

  __cio_printf("USB INT V: %x, C: %x", vector, ecode);

}

uint32 _usb_read_long( uint8 offset ) {
  return *(uint32 *)(base_addr + offset);
}

uint16 _usb_read_word( uint8 offset ) {
  return *(uint16 *)(base_addr + offset);
}

uint8 _usb_read_byte( uint8 offset ) {
  return *(uint8 *)(base_addr + offset);
}

uint32 _usb_read_port( uint8 port ) {
  return _usb_read_long(0x44 + (4 * (port - 1)));
}

void _usb_write_long( uint8 offset, uint32 data ){
  uint32* tmp = (uint32 *)(base_addr + offset);
  *tmp = data;
}

void _usb_write_word( uint8 offset, uint16 data ) {
  uint16* tmp = (uint16 *)(base_addr + offset);
  *tmp = data;
}

void _usb_write_byte( uint8 offset, uint8 data ) {
  uint8* tmp = (uint8 *)(base_addr + offset);
  *tmp = data;
}

void _usb_write_port( uint8 port, uint32 data ){
  _usb_write_long(0x44 + (4 * (port-1)), data);
}

void _usb_ehci_init( PCIDev* pciDev ) {

  usbController = pciDev;

  base_addr = usbController->bar0; // Base address for controller

  _usb_num_ports = _usb_read_byte(0x04) & 0xF; // Get number of ports

 
  base_addr = base_addr + _usb_read_byte( 0x00 );

  _usb_write_long(0x0, 0x2); // Reset the host controller

  while((_usb_read_long(0x0) & 0x2) == 0x2) { // Wait while host controller resets
    __cio_printf("WAIT!\n");
  }

  frame_list = (uint32 *)_kalloc_page(1); // Allocate space for frame list
  async_list = (struct _usb_qh_s*)_kalloc_page(1); // Allocate space for asynclist
  _usb_tds = (struct _usb_qtd_s*)_kalloc_page(1); // Allocate all the TDs

  _usb_write_long( 0x14, ((uint32)frame_list) ); // Tell the controller where that list is

  if(frame_list != ((uint32 *)_usb_read_long( 0x14 ))) {
    __panic("USB controller frame list address is not accurate");
  }

  for(int i=0; i<1024;i++) { // Set the pointers to be invalid
    frame_list[i] = 0x00000001;
  }

  _usb_free_qtdq = _queue_alloc(NULL); // Make a queue for free TD's

  for(int i=0;i<MAX_TDS;++i) { // Load queue with free td's
    _queue_enque(_usb_free_qtdq, &(_usb_tds[i]));
  }

  _usb_free_qhq = _queue_alloc(NULL);

  for(int i=0;i<ASYNC_LIST_SIZE;++i) {
    _queue_enque(_usb_free_qhq, &(async_list[i]));
  }


  _pci_set_interrupt( usbController, 0x0 , 0x43 ); // Change the interrupt line

  __install_isr( usbController->interrupt, _usb_isr ); // Install ISR
  //_usb_enable_interrupts(); // Enable interrupts

  _usb_write_long(0x00, _usb_read_long(0x0) | 0x1);

  _usb_write_long(0x40, 0x1); // All ports route to this controller

  __cio_puts( " USB_EHCI" );

}

void _usb_enable_interrupts() {
  _usb_write_long(0x08, 0x3F);
}

void _usb_ehci_status( void ) {
  uint32 curr_frame_ind;
  uint32 addr;

  curr_frame_ind = _usb_read_long( 0x00 );
  addr = _usb_read_long( 0x04 );
  __cio_printf("Command: %x Status: %x ASList: %x\n", curr_frame_ind, addr, _usb_read_long(0x18));
  __cio_printf("Num ports: %x\n", _usb_num_ports);
  for(int i=1;i<=_usb_num_ports;++i){
    __cio_printf("P%d: %x ", i, _usb_read_port(i));
  }

  __cio_printf("\n");

}

struct _usb_qtd_s* _usb_ehci_free_qtd( void ) {
  if(_queue_front(_usb_free_qtdq) == NULL){
    __panic("USB_EHCI: Not enought Queue Transfer Descriptors");
  }
  return (struct _usb_qtd_s*) _queue_deque(_usb_free_qtdq);
}

void _usb_ehci_release_qtd( struct _usb_qtd_s* qtd ){
  _queue_enque(_usb_free_qtdq, qtd);
}

struct _usb_qh_s* _usb_ehci_free_qh( void ){
  if(_queue_front(_usb_free_qhq) == NULL){
    __panic("USB_EHCI: Not enought Queue Heads");
  }
  return (struct _usb_qh_s*) _queue_deque(_usb_free_qhq);
}

void _usb_echi_release_qh( struct _usb_qh_s* qh ) {
  _queue_enque(_usb_free_qhq, qh);
}

void _usb_ehci_schedule_isosync( struct _usb_td_s* td ) {
  frame_list[0] = (uint32)td & 0xFFFFFFF0;
}

void _usb_ehci_schedule_async( struct _usb_qh_s* qh ) {

  if((_usb_read_long(0x0) & 0x20) == 0x0) { // Async list is not enabled
    asyncHead = qh;
    _usb_write_long(0x18, ((uint32) qh) | 0x2); // Set front of list to be QH
    qh->dword1 |= 0x8000; // Set this as the head of the list
    _usb_write_long(0x0, _usb_read_long(0x0) | 0x20); // Enable to async list
  }else{ // A head QH should be present, so add to list
    //_usb_write_long(0x0, _usb_read_long(0x0) & 0xFFFFFFDF); // Disable list
    struct _usb_qh_s* headQh = asyncHead;
    qh->dword0 = headQh->dword0 | 0x2;
    qh->dword1 &= 0xFFFF7FFF; // Disble the head bit. Just incase
    headQh->dword0 = ((uint32)qh) | 0x2;
    //_usb_write_long(0x0, _usb_read_long(0x0) | 0x20); // Renable list
  }
}

bool _usb_ehci_has_port_change( void ) {
  return (_usb_read_long(0x04) & 0x4) == 0x4;
}

uint8 _usb_ehci_find_port_change( void ) {
  for(int i=1;i<=_usb_num_ports;++i){
    if( (_usb_read_port(i) & 0x2) == 0x2 ){
      return i;
    }
  }
  return 0;
}

void _usb_ehci_reset_port( uint8 port ) {
  if(port == 0) return;
  if((_usb_read_port(port) & 0x100) == 0x0) {
    _usb_write_port(port, _usb_read_port(port) | 0x104); // Set reset flag and enable port

    __delay(10); // Wait a little

    _usb_write_port(port, _usb_read_port(port) & 0xFFFFFEFF);

    while((_usb_read_port(port) & 0x100) == 0x100){
      __cio_printf("WAIT!\n");
      __cio_printf("%x\n", _usb_read_port(port));
    }

    __cio_printf("USB_ECHI: port %d has been reset\n", port);
  }
}
