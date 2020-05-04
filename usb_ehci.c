/*
** File: usb_ehci.c
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: The module to control a USB EHCI host controller
 */

#define __SP_KERNEL__

#include "klib.h"

#include "usb_ehci.h"

#include "queues.h"

// The number of ports on the controller
uint8 _usb_num_ports = 0;

// The PCI device the controller is connected to
PCIDev *usbController;

struct _usb_qtd_s *_usb_tds;
struct _usb_qh_s *async_list;
struct _usb_qh_s *asyncHead;

// Queues for qTransfer Descriptors
// and QueueHeads
Queue _usb_free_qtdq;
Queue _usb_free_qhq;

// The base memory address of the controller
uint32 base_addr;

// The isosyncronous frame list
uint32 *frame_list;

// The isr callback handler function
void (*_isr_callback)(void);

/**
** The isr handler function.
** Just calls the callback handler
 */
void _usb_isr(int vector, int ecode)
{

  (*_isr_callback)(); // Handle the call
}

/**
** Set the call back handler
 */
void _usb_ehci_isr_callback(void (*callback)(void))
{
  _isr_callback = callback;
}

/**
** Read a long from the controller
**
** @param offset  The offset to the memory port
**
** @returns a 32 bit value
 */
uint32 _usb_read_long(uint8 offset)
{
  return *(uint32 *)(base_addr + offset);
}

/**
** Read a word from the controller
**
** @param offset  The offset to the memory port
**
** @returns a 16 bit value
 */
uint16 _usb_read_word(uint8 offset)
{
  return *(uint16 *)(base_addr + offset);
}

/**
** Read a byte from the controller
**
** @param offset  The offset to the memory port
**
** @returns an 8 bit value
 */
uint8 _usb_read_byte(uint8 offset)
{
  return *(uint8 *)(base_addr + offset);
}

/**
** Read the status of a port
**
** @param port  The port number
**
** @returns a 32 bit value from the port register
 */
uint32 _usb_read_port(uint8 port)
{
  return _usb_read_long(0x44 + (4 * (port - 1)));
}

/**
** Write a long to the controller
**
** @param offset  The offset to the memory port
** @param data  The long to write
 */
void _usb_write_long(uint8 offset, uint32 data)
{
  uint32 *tmp = (uint32 *)(base_addr + offset);
  *tmp = data;
}

/**
** Write a word to the controller
**
** @param offset  The offset to the memory port
** @param data  The word to write
 */
void _usb_write_word(uint8 offset, uint16 data)
{
  uint16 *tmp = (uint16 *)(base_addr + offset);
  *tmp = data;
}

/**
** Write a byte to the controller
**
** @param offset  The offser to the memory port
** @parm data  The byte to write
 */
void _usb_write_byte(uint8 offset, uint8 data)
{
  uint8 *tmp = (uint8 *)(base_addr + offset);
  *tmp = data;
}

/**
** Write a long to a port register
**
** @param port  The port number
** @param data  The long to write
 */
void _usb_write_port(uint8 port, uint32 data)
{
  _usb_write_long(0x44 + (4 * (port - 1)), data);
}

/**
** Initialize the ECHI controller
 */
void _usb_ehci_init(PCIDev *pciDev)
{

  usbController = pciDev;

  base_addr = usbController->bar0; // Base address for controller

  _usb_num_ports = _usb_read_byte(0x04) & 0xF; // Get number of ports

  base_addr = base_addr + _usb_read_byte(0x00);

  _usb_write_long(0x0, 0x2); // Reset the host controller

  while ((_usb_read_long(0x0) & 0x2) == 0x2)
  { // Wait while host controller resets
    __cio_printf("WAIT!\n");
  }

  frame_list = (uint32 *)_kalloc_page(1);           // Allocate space for frame list
  async_list = (struct _usb_qh_s *)_kalloc_page(1); // Allocate space for asynclist
  _usb_tds = (struct _usb_qtd_s *)_kalloc_page(1);  // Allocate all the TDs

  _usb_write_long(0x14, ((uint32)frame_list)); // Tell the controller where that list is

  if (frame_list != ((uint32 *)_usb_read_long(0x14)))
  {
    __panic("USB controller frame list address is not accurate");
  }

  for (int i = 0; i < 1024; i++)
  { // Set the pointers to be invalid
    frame_list[i] = 0x00000001;
  }

  _usb_free_qtdq = _queue_alloc(NULL); // Make a queue for free TD's

  for (int i = 0; i < MAX_TDS; ++i)
  { // Load queue with free td's
    _queue_enque(_usb_free_qtdq, &(_usb_tds[i]));
  }

  _usb_free_qhq = _queue_alloc(NULL);

  for (int i = 0; i < ASYNC_LIST_SIZE; ++i)
  {
    _queue_enque(_usb_free_qhq, &(async_list[i]));
  }

  _pci_set_interrupt(usbController, 0x0, 43); // Change the interrupt line

  __install_isr(usbController->interrupt, _usb_isr); // Install ISR
  _usb_enable_interrupts();                          // Enable interrupts

  _usb_write_long(0x00, _usb_read_long(0x0) | 0x1); // Enable the controller

  _usb_write_long(0x40, 0x1); // All ports route to this controller

  __cio_puts(" USB_EHCI");
}

/**
** Enable interrupts
 */
void _usb_enable_interrupts()
{
  _usb_write_long(0x08, 0x3F);
}

/**
**
 */
void _usb_ehci_status(void)
{
  uint32 curr_frame_ind;
  uint32 addr;

  curr_frame_ind = _usb_read_long(0x00);
  addr = _usb_read_long(0x04);
  __cio_printf("Command: %x Status: %x ASList: %x\n", curr_frame_ind, addr, _usb_read_long(0x18));
  __cio_printf("Num ports: %x\n", _usb_num_ports);
  for (int i = 1; i <= _usb_num_ports; ++i)
  {
    __cio_printf("P%d: %x ", i, _usb_read_port(i));
  }

  __cio_printf("\n");
}

/**
** Get a free queue Transfer Descriptor
 */
struct _usb_qtd_s *_usb_ehci_free_qtd(void)
{
  if (_queue_front(_usb_free_qtdq) == NULL)
  {
    __panic("USB_EHCI: Not enought Queue Transfer Descriptors");
  }
  return (struct _usb_qtd_s *)_queue_deque(_usb_free_qtdq);
}

/**
** Release a Queue Transfer Descriptor
 */
void _usb_ehci_release_qtd(struct _usb_qtd_s *qtd)
{
  _queue_enque(_usb_free_qtdq, qtd);
}

/**
** Get a free Queue Head
 */
struct _usb_qh_s *_usb_ehci_free_qh(void)
{
  if (_queue_front(_usb_free_qhq) == NULL)
  {
    __panic("USB_EHCI: Not enought Queue Heads");
  }
  return (struct _usb_qh_s *)_queue_deque(_usb_free_qhq);
}

/**
** Release a Queue Head
 */
void _usb_echi_release_qh(struct _usb_qh_s *qh)
{
  _queue_enque(_usb_free_qhq, qh);
}

/**
** Schedule an isosyncronous transfer
 */
void _usb_ehci_schedule_isosync(struct _usb_itd_s *td)
{
  frame_list[0] = (uint32)td & 0xFFFFFFF0;
}

/**
** Schedule an asynchronous transfer
 */
void _usb_ehci_schedule_async(struct _usb_qh_s *qh)
{

  if ((_usb_read_long(0x0) & 0x20) == 0x0)
  { // Async list is not enabled
    asyncHead = qh;
    _usb_write_long(0x18, ((uint32)qh) | 0x2);        // Set front of list to be QH
    qh->dword1 |= 0x8000;                             // Set this as the head of the list
    _usb_write_long(0x0, _usb_read_long(0x0) | 0x20); // Enable to async list
  }
  else
  { // A head QH should be present, so add to list
    //_usb_write_long(0x0, _usb_read_long(0x0) & 0xFFFFFFDF); // Disable list
    struct _usb_qh_s *headQh = asyncHead;
    qh->dword0 = headQh->dword0 | 0x2;
    qh->dword1 &= 0xFFFF7FFF; // Disble the head bit. Just incase
    headQh->dword0 = ((uint32)qh) | 0x2;
    //_usb_write_long(0x0, _usb_read_long(0x0) | 0x20); // Renable list
  }
}

/**
** Check if a port has changed
 */
bool _usb_ehci_has_port_change(void)
{
  return (_usb_read_long(0x04) & 0x4) == 0x4;
}

/**
** Find which port has changed
 */
uint8 _usb_ehci_find_port_change(void)
{
  for (int i = 1; i <= _usb_num_ports; ++i)
  {
    if ((_usb_read_port(i) & 0x2) == 0x2)
    {
      return i;
    }
  }
  return 0;
}

/**
** Reset a port
 */
void _usb_ehci_reset_port(uint8 port)
{
  if (port == 0)
    return;
  if ((_usb_read_port(port) & 0x100) == 0x0)
  {
    _usb_write_port(port, _usb_read_port(port) | 0x104); // Set reset flag and enable port

    __delay(10); // Wait a little

    _usb_write_port(port, _usb_read_port(port) & 0xFFFFFEFF);

    while ((_usb_read_port(port) & 0x100) == 0x100)
    {
      __cio_printf("WAIT!\n");
      __cio_printf("%x\n", _usb_read_port(port));
    }
  }
}

/**
** Clear an interrupt signal
 */
void _usb_ehci_clear_interrupt(uint8 interruptNum)
{
  switch (interruptNum)
  {
  case USB_ERROR_INTERRUPT:
    _usb_write_long(0x4, (_usb_read_long(0x4) & 0xF03D) | 0b10);
    break;
  case USB_PORT_CHANGE_INTERRUPT:
    _usb_write_long(0x4, (_usb_read_long(0x4) & 0xF03B) | 0b100);
    break;
  case USB_FRAME_LIST_INTERRUPT:
    _usb_write_long(0x4, (_usb_read_long(0x4) & 0xF037) | 0b1000);
    break;
  case USB_HOST_ERROR_INTERRUPT:
    _usb_write_long(0x4, (_usb_read_long(0x4) & 0xF02F) | 0b10000);
    break;
  case USB_ASYNC_INTERRUPT:
    _usb_write_long(0x4, (_usb_read_long(0x4) & 0xF01F) | 0b100000);
    break;
  default:
    break;
  }
}

/**
** Check if an interrupt signal of type 'interruptNum' has happened
 */
bool _usb_ehci_get_interrupt_status(uint8 interruptNum)
{
  uint32 intSts, usbSts;
  intSts = _usb_read_long(0x08);
  usbSts = _usb_read_long(0x04);
  switch (interruptNum)
  {

  case USB_ERROR_INTERRUPT:
    return (intSts & 0b11) == 0b11 && (usbSts & 0b10) == 0b10;
  case USB_PORT_CHANGE_INTERRUPT:
    return (intSts & 0b101) == 0b101 && (usbSts & 0b100) == 0b100;
  case USB_FRAME_LIST_INTERRUPT:
    return (intSts & 0b1001) == 0b1001 && (usbSts & 0b1000) == 0b1000;
  case USB_HOST_ERROR_INTERRUPT:
    return (intSts & 0b10001) == 0b10001 && (usbSts & 0b10000) == 0b10000;
  case USB_ASYNC_INTERRUPT:
    return (intSts & 0b100001) == 0b100001 && (usbSts & 0b100000) == 0b100000;
  default:
    return false;
  }
}
