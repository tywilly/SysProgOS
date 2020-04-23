/*




*/

#define __SP_KERNEL__

#include "usbd.h"

#include "usb_ehci.h"
#include "klib.h"
#include "usb_util.h"

typedef struct _usb_endpoint_s{
  struct _usb_qh_s* qh;
  USBPacket first_packet;
} USBEndpoint;

typedef struct _usb_dev_s{
  bool active;
  USBEndpoint endpoints[USB_MAX_ENDPOINTS];
} USBDev;

USBDev _usb_devs[USB_MAX_DEVICES];

void _isr_handler(void){
  if(_usb_ehci_get_interrupt_status(USB_PORT_CHANGE_INTERRUPT)){ // Handle port change
    _usbd_enumerate_devices();
    _usb_ehci_clear_interrupt(USB_PORT_CHANGE_INTERRUPT);
  }
}

void _usbd_init( void ) {

  for(int i=0;i<USB_MAX_DEVICES;++i) {
    _usb_devs[i].active = false;
  }

  USBEndpoint * endp = _usbd_new_endpoint(0, 0, USB_LOW_SPEED, 64); // Create the default device control pipe
  _usbd_schedule_endpoint(endp); // Schedule the endpoint

  _usb_ehci_isr_callback(&_isr_handler);

  __cio_puts(" USBD");
}

void _usbd_set_device_address(USBDev * dev, uint8 newAddr) {

  USBEndpoint * dep = &(dev->endpoints[0]);

  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);

  uint8 data[8];
  usb_device_request_packet(data, 0x0, SET_ADDRESS, newAddr, 0x0, 0x0);
  _usbd_add_data_packet(setupPacket, data, 8);

  USBPacket statusPacket = _usbd_new_packet(USB_IN_PACKET, 0);

  _usbd_add_packet_to_endpoint(dep, statusPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);
  _usbd_wait_for_packet(statusPacket);

  _usbd_free_packet(setupPacket);
  _usbd_free_packet(statusPacket);
}

void _usbd_set_device_config(USBDev * dev, uint8 bConfigurationValue) {
  USBEndpoint * dep = &(dev->endpoints[0]);

  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);
  USBPacket statusPacket = _usbd_new_packet(USB_IN_PACKET, 0);

  uint8 data[8];
  usb_device_request_packet(data, 0x0, SET_CONFIGURATION,
                            usb_bytes_to_word(0x0, bConfigurationValue), 0x0, 0x0);
  _usbd_add_data_packet(setupPacket, data, 8);

  __cio_printf("Setting config %x\n", bConfigurationValue);

  _usbd_add_packet_to_endpoint(dep, statusPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);
  _usbd_wait_for_packet(statusPacket);

  _usbd_free_packet(setupPacket);
  _usbd_free_packet(statusPacket);
}

void _usbd_get_device_desc(USBDev * dev, void* dst, uint8 descType, uint8 descIndex) {

  USBEndpoint * dep = &(dev->endpoints[0]);

  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);

  uint8 data[8];
  usb_device_request_packet(data, 0x80, GET_DESCRIPTOR, usb_bytes_to_word(descType, descIndex), 0x0, 64);
  _usbd_add_data_packet(setupPacket, data, 8);

  USBPacket inPacket = _usbd_new_packet(USB_IN_PACKET, 64);

  USBPacket outPacket = _usbd_new_packet(USB_OUT_PACKET, 0);

  _usbd_add_packet_to_endpoint(dep, outPacket);
  _usbd_add_packet_to_endpoint(dep, inPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);

  _usbd_wait_for_packet(outPacket);
  _usbd_get_data_packet(inPacket, dst, 64);

  _usbd_free_packet(setupPacket);
  _usbd_free_packet(inPacket);
  _usbd_free_packet(outPacket);
}

void _usbd_get_string_desc(USBDev* dev, void* dst, uint8 descIndex) {

  USBEndpoint * dep = &(dev->endpoints[0]);

  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);

  uint8 data[8];
  usb_device_request_packet(data, 0x80, GET_DESCRIPTOR, usb_bytes_to_word(STRING, descIndex),
                            0x0409, 64);
  _usbd_add_data_packet(setupPacket, data, 8);

  USBPacket inPacket = _usbd_new_packet(USB_IN_PACKET, 64);

  USBPacket outPacket = _usbd_new_packet(USB_OUT_PACKET, 0);

  _usbd_add_packet_to_endpoint(dep, outPacket);
  _usbd_add_packet_to_endpoint(dep, inPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);

  _usbd_wait_for_packet(outPacket);
  _usbd_get_data_packet(inPacket, dst, 64);

  _usbd_free_packet(setupPacket);
  _usbd_free_packet(inPacket);
  _usbd_free_packet(outPacket);
}

void _usbd_list_devices( void ) {
  __cio_printf("USB Devices: \n");
  for(int i=0;i<USB_MAX_DEVICES;++i) {
    USBDev * dev = &_usb_devs[i];
    if(dev->active) {
      __cio_printf("ID: %d\n", i);

      uint8 buff[64];
      _usbd_get_device_desc(dev, buff, DEVICE, 0);

      char sBuff[64];
      _usbd_get_string_desc(dev, sBuff, buff[14]);

      __cio_printf("Manufacture: ");
      for(int x=2;x<sBuff[0];++x){
        __cio_printf("%c", sBuff[x]);
      }

      __cio_puts(&(sBuff[2]));

      _usbd_get_string_desc(dev, sBuff, buff[15]);
      __cio_puts("\nProduct: ");
      __cio_puts(&sBuff[2]);

    }
  }
}

int8 _usbd_find_free_device( void ) {
  for(int i=1;i<USB_MAX_DEVICES;++i) {
    USBDev * dev = &_usb_devs[i];
    if(!dev->active){
      return i;
    }
  }
  return -1;
}

void _usbd_enumerate_devices( void ) {

  if(_usb_ehci_has_port_change()) {

    __cio_printf("Enumerating USB\n");

    uint8 deviceAddr = _usbd_find_free_device();

    USBDev * defdev = &_usb_devs[0];
    USBDev * newDev = &_usb_devs[deviceAddr];

    uint8 port = _usb_ehci_find_port_change();
    _usb_ehci_reset_port(port);

    //_usbd_set_device_address(defdev, deviceAddr); // Set device address

    uint8 descBuff[64];
    _usbd_get_device_desc(defdev, descBuff, DEVICE, 0);

    USBEndpoint* defEndp = _usbd_new_endpoint(deviceAddr, 0, USB_LOW_SPEED, descBuff[7]);

    uint8 recBuff[64];
    _usbd_get_device_desc(defdev, recBuff, CONFIGURATION, 0);

    uint8 bConfigVal = recBuff[5];

    uint8* intDesc = (recBuff + recBuff[0]);
    for(int i=0;i<recBuff[4];++i) { // Loop through all interface descriptors

      uint8* endpDesc = (intDesc + intDesc[0]);
      for(int e=0;e<intDesc[4];++e) { // Loop through all endpoints

        uint16 maxPacketSize = usb_bytes_to_word(endpDesc[4], endpDesc[5]) & 0x7FF;
        USBEndpoint * endp = _usbd_new_endpoint(deviceAddr, endpDesc[2] & 0xF,
                                                USB_LOW_SPEED, maxPacketSize);

        _usbd_schedule_endpoint(endp);
        endpDesc = (endpDesc + endpDesc[0]);
      }
      intDesc = (intDesc + intDesc[0]);
    }

    _usbd_schedule_endpoint(defEndp);
    newDev->active = true;

    _usbd_set_device_address(defdev, deviceAddr); // Set device address

    _usbd_set_device_config(newDev, bConfigVal); // Set the device configuration

    __cio_printf("USB Enum done\n");

  }else{
    __cio_printf("\nNo new devices\n");
  }

}

void _usbd_schedule_endpoint(USBEndpoint * endpoint){
  _usb_ehci_schedule_async(endpoint->qh);
}

USBEndpoint * _usbd_new_endpoint(uint8 addr, uint8 endpoint, uint8 speed, uint16 maxPacketSize){
  USBEndpoint * trans = &(_usb_devs[addr].endpoints[endpoint]);
  trans->qh = _usb_ehci_free_qh();
  trans->first_packet = 0x0;
  trans->qh->dword0 = 0x0;
  trans->qh->dword0 = (uint32)trans->qh | 0x2; // Set horziontal pointer

  trans->qh->dword1 = 0x0;
  trans->qh->dword1 |= (uint32) (addr & 0x7F);
  trans->qh->dword1 |= ((uint32) (endpoint & 0xF)) << 8;
  trans->qh->dword1 |= ((uint32)(speed & 0x3)) << 12;
  trans->qh->dword1 |= 0x4000; // Always is 1
  trans->qh->dword1 |= ((uint32)(maxPacketSize & 0x3FF)) << 16;

  trans->qh->dword2 = 0x40000000;
  trans->qh->dword3 = 0x0;
  trans->qh->dword4 = 0x1;
  trans->qh->dword5 = 0x1;
  trans->qh->dword6 = 0x0;
  trans->qh->dword7 = 0x0;
  //trans->qh->dword8 = 0x0;
  //trans->qh->dword9 = 0x0;
  //trans->qh->dword10 = 0x0;
  //trans->qh->dword11 = 0x0;

  return trans;
}

void _usbd_add_packet_to_endpoint(USBEndpoint* tran, USBPacket pack) {
  tran->first_packet = pack;
  pack->next_ptr = tran->qh->dword4;
  tran->qh->dword4 = (uint32) pack;
}

USBPacket _usbd_new_packet(uint8 type, uint16 size) {
  struct _usb_qtd_s* pack = _usb_ehci_free_qtd();
  pack->next_ptr = 0x0;
  pack->alt_ptr = 0x1;
  pack->token = 0x0;

  if(type == USB_IN_PACKET || type == USB_OUT_PACKET){
    pack->token |= 0x80000000;
  }

  pack->token = pack->token | (uint32)type << 8; // Set PID flag
  pack->token = pack->token | (uint32)size << 16; // Set size field
  pack->token = pack->token | 0xC00; // Set CERR field
  pack->token = pack->token | 0x80; // Set status field to enable

  if(size > 0) {

    uint8 num_pages = (size % PAGE_SIZE) + 1;
    uint32 * buff = (uint32 *)_kalloc_page(num_pages);
    __memclr(buff, num_pages * PAGE_SIZE);

    pack->ptr0 = (uint32)buff;
    //TODO: Fix the buffers list
  }else{
    pack->ptr0 = 0x0;
  }

  pack->ptr1 = 0x0;
  pack->ptr2 = 0x0;
  pack->ptr3 = 0x0;
  pack->ptr4 = 0x0;

  return pack;
}

void _usbd_free_packet(USBPacket pack) {
  pack->next_ptr = 0x1; // Just to make sure the packet is marked invalid
  _kfree_page((void *)pack->ptr0);
  _usb_ehci_release_qtd(pack);
}

void _usbd_set_ioc_packet(USBPacket pack) {
  pack->token = pack->token | 0x8000;
}

void _usbd_print_endpoint(USBEndpoint * tran) {
  __cio_printf("USB Endpoint Dump\n");
  __cio_printf("QH %x: HPtr: %x Token: %x EPCh: %x ", tran->qh, tran->qh->dword0, tran->qh->dword1, tran->qh->dword2);
  __cio_printf("cQTD: %x nQTD: %x\n", tran->qh->dword3, tran->qh->dword4);
  struct _usb_qtd_s* cqtd = tran->first_packet;
  uint32 cqtda = (uint32)cqtd;
  int cqtdN = 1;
  while((cqtda & 0x1) != 0x1 ) {
    uint16 packSize = (cqtd->token & 0x7FFF0000) >> 16;
    __cio_printf("qTD%d %x: Next: %x Token: %x Endpoint Size: %d CPage: %d PID: %x Status: %x",
                 cqtdN, cqtd, cqtd->next_ptr, cqtd->token, packSize,
                 (cqtd->token & 0x7000) >> 12, (cqtd->token & 0x300) >> 8, cqtd->token & 0xFF);
    __cio_printf(" Buffptr 0: %x \n", cqtd->ptr0);

    cqtda = cqtd->next_ptr;
    cqtd = (struct _usb_qtd_s*)cqtd->next_ptr;
    cqtdN += 1;
  }

}

void _usbd_add_data_packet(USBPacket pack, void * data, uint16 size) {
  __memcpy((void*)(pack->ptr0 & 0xFFFFF000), data, size);
}

void _usbd_get_data_packet(USBPacket pack, void * dst, uint16 size) {
  void * src = (void *)(pack->ptr0 & 0xFFFFF000);
  __memcpy(dst, src, size);
}

void _usbd_wait_for_packet(USBPacket pack) {
  while((pack->token & 0xFF) == 0x80){
    //__cio_printf("Wait for packet");
    // Wait for the packet
  }
}
