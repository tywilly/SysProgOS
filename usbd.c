/*
** File: usbd.c
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: Driver for the entire USB system. Contains API used to interact
*with the USB system
*/

#define __SP_KERNEL__

#include "usbd.h"

#include "klib.h"
#include "queues.h"
#include "usb_ehci.h"
#include "usb_util.h"

// An array of USB devices.
USBDev _usb_devs[USB_MAX_DEVICES];

// Driver handler queue
Queue _usb_driver_handler_q;

/**
** A callback method for handling interrupts from the host controller
*/
void _isr_handler(void) {
  if (_usb_ehci_get_interrupt_status(
          USB_PORT_CHANGE_INTERRUPT)) { // Handle port change
    _usbd_enumerate_devices();
    _usb_ehci_clear_interrupt(
        USB_PORT_CHANGE_INTERRUPT); // Clear the port change bit
  }
}

/**
** Init the usb driver
*/
void _usbd_init(void) {

  _usb_driver_handler_q = _queue_alloc(NULL); // Create driver handler queue

  for (int i = 0; i < USB_MAX_DEVICES; ++i) {
    _usb_devs[i].active = false; // Set all devices as inactive
  }

  USBEndpoint *endp = _usbd_new_endpoint(
      0, 0, USB_LOW_SPEED, 64);  // Create the default device control pipe
  _usbd_schedule_endpoint(endp); // Schedule the endpoint

  _usb_ehci_isr_callback(&_isr_handler); // Set the isr callback

  __cio_puts(" USBD");
}

/**
** Register a driver handler
*/
void _usbd_register_handler(bool (*handler)(USBDev *, uint8, uint8, uint8)) {
  _queue_enque(_usb_driver_handler_q, handler); // Add handler to queue
}

/**
** Notify all registered driver handlers a new device has connected.
**
** @param dev  The USBDev that just connected
** @param class The class of the new device
** @param subClass  The subClass of the new device
** @param protocol  The protocol of the new device
*/
void _usbd_notify_handlers(USBDev *dev, uint8 class, uint8 subClass,
                           uint8 protocol) {
  QIter iter = _queue_start(_usb_driver_handler_q);

  for (int i = 0; i < _queue_length(_usb_driver_handler_q); ++i) {
    bool (*handler)(USBDev *, uint8, uint8, uint8) = _queue_current(iter);

    if (handler(dev, class, subClass, protocol)) // Call the handler
      return;
    _queue_next(iter);
  }
}

/**
** Set the address of a device
*/
void _usbd_set_device_address(USBDev *dev, uint8 newAddr) {

  USBEndpoint *dep = &(dev->endpoints[0]); // Get the default endpoint of device

  USBPacket setupPacket =
      _usbd_new_packet(USB_SETUP_PACKET, 8); // New setup packet

  // Set the data of the packet to be for setting an address
  uint8 data[8];
  usb_device_request_packet(data, 0x0, SET_ADDRESS, newAddr, 0x0, 0x0);
  _usbd_add_data_packet(setupPacket, data, 8);

  USBPacket statusPacket =
      _usbd_new_packet(USB_IN_PACKET, 0); // The status packet

  // queue the packets for transfer
  _usbd_add_packet_to_endpoint(dep, statusPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);
  _usbd_wait_for_packet(statusPacket); // Wait for status packet

  // If you love something let it go
  _usbd_free_packet(setupPacket);
  _usbd_free_packet(statusPacket);
}

/**
** Set the configuration of a device
*/
void _usbd_set_device_config(USBDev *dev, uint8 bConfigurationValue) {
  USBEndpoint *dep = &(dev->endpoints[0]); // Default endpoint of device

  // get packets
  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);
  USBPacket statusPacket = _usbd_new_packet(USB_IN_PACKET, 0);

  // Set data of setup packet
  uint8 data[8];
  usb_device_request_packet(data, 0x0, SET_CONFIGURATION,
                            usb_bytes_to_word(0x0, bConfigurationValue), 0x0,
                            0x0);
  _usbd_add_data_packet(setupPacket, data, 8);

  __cio_printf("Setting config %x\n", bConfigurationValue);

  // Queue packets
  _usbd_add_packet_to_endpoint(dep, statusPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);
  _usbd_wait_for_packet(statusPacket);

  // Release packets
  _usbd_free_packet(setupPacket);
  _usbd_free_packet(statusPacket);
}

/**
** Get the descriptor from a device
*/
void _usbd_get_device_desc(USBDev *dev, void *dst, uint8 descType,
                           uint8 descIndex) {

  USBEndpoint *dep = &(dev->endpoints[0]); // get endpoint 0

  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);

  // Set data in setup packet to get a descriptor
  uint8 data[8];
  usb_device_request_packet(data, 0x80, GET_DESCRIPTOR,
                            usb_bytes_to_word(descType, descIndex), 0x0, 64);
  _usbd_add_data_packet(setupPacket, data, 8);

  // The size of a descriptor is at most 64 bytes. Get a packet that can handle
  // that much
  USBPacket inPacket = _usbd_new_packet(USB_IN_PACKET, 64);

  // The status packet for this transfer is an empty out packet
  USBPacket outPacket = _usbd_new_packet(USB_OUT_PACKET, 0);

  // Queue packets
  _usbd_add_packet_to_endpoint(dep, outPacket);
  _usbd_add_packet_to_endpoint(dep, inPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);

  // Wait for in packet and then copy the data
  _usbd_wait_for_packet(outPacket);
  _usbd_get_data_packet(inPacket, dst, 64);

  // Release packets
  _usbd_free_packet(setupPacket);
  _usbd_free_packet(inPacket);
  _usbd_free_packet(outPacket);
}

/**
** Get a string descriptor from a device
*/
void _usbd_get_string_desc(USBDev *dev, void *dst, uint8 descIndex) {

  USBEndpoint *dep = &(dev->endpoints[0]); // endpoint 0

  // Create setup packet
  USBPacket setupPacket = _usbd_new_packet(USB_SETUP_PACKET, 8);
  uint8 data[8];
  usb_device_request_packet(data, 0x80, GET_DESCRIPTOR,
                            usb_bytes_to_word(STRING, descIndex), 0x0409,
                            64); // Get a string descriptor. 0x0409 is language
                                 // code for english.
  _usbd_add_data_packet(setupPacket, data, 8);

  // Descriptor has a max size of 64 bytes
  USBPacket inPacket = _usbd_new_packet(USB_IN_PACKET, 64);

  // Status packet is an empty out packet
  USBPacket outPacket = _usbd_new_packet(USB_OUT_PACKET, 0);

  // Queue packets
  _usbd_add_packet_to_endpoint(dep, outPacket);
  _usbd_add_packet_to_endpoint(dep, inPacket);
  _usbd_add_packet_to_endpoint(dep, setupPacket);

  // Wait for status packet. Then copy data
  _usbd_wait_for_packet(outPacket);
  _usbd_get_data_packet(inPacket, dst, 64);

  // Release packets
  _usbd_free_packet(setupPacket);
  _usbd_free_packet(inPacket);
  _usbd_free_packet(outPacket);
}

/**
** Get a USB device based on the class, subClass and protocol
*/
USBDev *_usbd_get_device(uint8 class, uint8 subClass, uint8 protocol) {
  for (int i = 0; i < USB_MAX_DEVICES; ++i) {

    USBDev *dev = &_usb_devs[i];
    if (dev->active) {
      uint8 recBuff[64];
      _usbd_get_device_desc(dev, &recBuff, DEVICE, 0);

      if (recBuff[4] == 0x00) { // Use interface descriptors

        _usbd_get_device_desc(dev, recBuff, CONFIGURATION,
                              0); // Get configuration descriptor

        uint8 *intDesc = (recBuff + recBuff[0]);
        for (int i = 0; i < recBuff[4];
             ++i) { // Loop through all interface descriptors

          if (intDesc[5] == class) // Does the class match now?
          {
            if (subClass != 0xFF) // Do we care about the subClass?
            {
              if (intDesc[6] == subClass &&
                  intDesc[7] ==
                      protocol) // Does the subClass and protocol match?
              {
                return dev;
              }
            } else {
              return dev;
            }
          }

          intDesc = (intDesc + intDesc[0]);
        }
      }

      if (recBuff[4] == class) { // Does the class match?
        if (subClass != 0xFF)    // Do we care about the subClas?
        {
          if (recBuff[5] == subClass &&
              recBuff[6] == protocol) { // Does the subClass and protocol match?
            return dev;                 // Ayy we found it!
          }
        } else {
          return dev;
        }
      }
    }
  }
  return NULL;
}

/**
** Print the currently connected devices to the console
*/
void _usbd_list_devices(void) {
  __cio_printf("USB Devices: \n");
  for (int i = 0; i < USB_MAX_DEVICES; ++i) {
    USBDev *dev = &_usb_devs[i];
    if (dev->active) {
      __cio_printf("ID: %d\n", i);

      uint8 buff[64];
      _usbd_get_device_desc(dev, buff, DEVICE, 0);

      char sBuff[64];
      _usbd_get_string_desc(dev, sBuff, buff[14]);

      __cio_printf(" Manufacture: ");
      for (int x = 2; x < sBuff[0]; ++x) {
        __cio_printf("%c", sBuff[x]);
      }
      __cio_printf("\n");

      _usbd_get_string_desc(dev, sBuff, buff[15]);
      __cio_puts(" Product: ");
      for (int x = 2; x < sBuff[0]; ++x) {
        __cio_printf("%c", sBuff[x]);
      }
      __cio_printf("\n");
      __cio_puts(" Driver: ");
      __cio_puts(dev->driverName);
      __cio_puts("\n");
    }
  }
}

/**
** Find the next available unique address for a USB device
**
** @returns A unique address for a USB device
*/
int8 _usbd_find_free_device(void) {
  for (int i = 1; i < USB_MAX_DEVICES; ++i) {
    USBDev *dev = &_usb_devs[i];
    if (!dev->active) {
      return i;
    }
  }
  return -1;
}

/**
** If a device has been connected, reset the bus, assign
** an address to it and configure it using the default configuration
*/
void _usbd_enumerate_devices(void) {

  if (_usb_ehci_has_port_change()) {

    __cio_printf("Enumerating USB\n");

    while (_usb_ehci_find_port_change() != 0) {

      uint8 deviceAddr = _usbd_find_free_device();

      USBDev *defdev = &_usb_devs[0];          // Default device
      USBDev *newDev = &_usb_devs[deviceAddr]; // New device
      __strcpy(newDev->driverName, "UNKNOWN");

      uint8 port = _usb_ehci_find_port_change(); // Find which port changed
      if (port == 0) {
        __panic("USB Controller says port change, but ports don't");
      }
      _usb_ehci_reset_port(port); // Reset the port that changed

      uint8 descBuff[64];
      _usbd_get_device_desc(defdev, descBuff, DEVICE,
                            0); // Get the device descriptor

      // Create a default endpoint for the new device
      USBEndpoint *defEndp =
          _usbd_new_endpoint(deviceAddr, 0, USB_LOW_SPEED, descBuff[7]);

      // Get the configuration descriptor
      uint8 recBuff[64];
      _usbd_get_device_desc(defdev, recBuff, CONFIGURATION, 0);

      uint8 bConfigVal = recBuff[5];
      uint8 class;
      uint8 subClass;
      uint8 protocol;

      uint8 *intDesc = (recBuff + recBuff[0]);
      for (int i = 0; i < recBuff[4];
           ++i) { // Loop through all interface descriptors

        uint8 *endpDesc = (intDesc + intDesc[0]);
        for (int e = 0; e < intDesc[4]; ++e) { // Loop through all endpoints

          uint16 maxPacketSize =
              usb_bytes_to_word(endpDesc[4], endpDesc[5]) & 0x7FF;
          USBEndpoint *endp = _usbd_new_endpoint(deviceAddr, endpDesc[2] & 0xF,
                                                 USB_HIGH_SPEED, maxPacketSize);

          endp->isInEndpoint = (bool)(endpDesc[2] >> 7);
          endp->attributes = endpDesc[3];

          _usbd_schedule_endpoint(endp); // Schedule the new endpoint
          endpDesc = (endpDesc + endpDesc[0]);
        }

        class = intDesc[5];
        subClass = intDesc[6];
        protocol = intDesc[7];

        intDesc = (intDesc + intDesc[0]);
      }

      _usbd_schedule_endpoint(defEndp); // Schedule the default endpoint
      newDev->active = true;

      _usbd_set_device_address(defdev, deviceAddr); // Set device address

      _usbd_set_device_config(newDev,
                              bConfigVal); // Set the device configuration

      _usbd_notify_handlers(newDev, class, subClass,
                            protocol); // Tell drivers a new device connected
    }

    __cio_printf("USB Enum done\n");
  } else {
    __cio_printf("\nNo new devices\n");
  }
}

/**
** Schedule an endpoint so packets can be transfered
*/
void _usbd_schedule_endpoint(USBEndpoint *endpoint) {
  _usb_ehci_schedule_async(endpoint->qh);
}

/**
** Create and initialize an endpoint
*/
USBEndpoint *_usbd_new_endpoint(uint8 addr, uint8 endpoint, uint8 speed,
                                uint16 maxPacketSize) {
  USBEndpoint *trans = &(_usb_devs[addr].endpoints[endpoint]);
  trans->qh = _usb_ehci_free_qh();
  trans->first_packet = 0x0;
  trans->qh->dword0 = 0x0;
  trans->qh->dword0 = (uint32)trans->qh | 0x2; // Set horziontal pointer

  trans->qh->dword1 = 0x0;
  trans->qh->dword1 |= (uint32)(addr & 0x7F);
  trans->qh->dword1 |= ((uint32)(endpoint & 0xF)) << 8;
  trans->qh->dword1 |= ((uint32)(speed & 0x3)) << 12;
  trans->qh->dword1 |= 0x4000; // Always is 1
  trans->qh->dword1 |= ((uint32)(maxPacketSize & 0x3FF)) << 16;

  trans->qh->dword2 = 0x40000000;
  trans->qh->dword3 = 0x0;
  trans->qh->dword4 = 0x1;
  trans->qh->dword5 = 0x1;
  trans->qh->dword6 = 0x0;
  trans->qh->dword7 = 0x0;
  // trans->qh->dword8 = 0x0;
  // trans->qh->dword9 = 0x0;
  // trans->qh->dword10 = 0x0;
  // trans->qh->dword11 = 0x0;

  return trans;
}

USBEndpoint *_usbd_get_endpoint(USBDev *dev, uint8 endpointN) {
  return &(dev->endpoints[endpointN]);
}

USBEndpoint *_usbd_get_endpoint_type(USBDev *dev, bool endpDir,
                                     uint8 transType) {
  for (int i = 0; i < USB_MAX_ENDPOINTS; ++i) {
    USBEndpoint *endp = &(dev->endpoints[i]);
    uint8 tranType = endp->attributes & 0x3;
    if (endp->isInEndpoint == endpDir && tranType == transType) {
      return endp;
    }
  }
  return NULL;
}

/**
** Queue a packet for transfer on an endpoint
*/
void _usbd_add_packet_to_endpoint(USBEndpoint *tran, USBPacket pack) {
  tran->first_packet = pack;
  pack->next_ptr = tran->qh->dword4;
  tran->qh->dword4 = (uint32)pack;
}

/**
** Create a new packet
*/
USBPacket _usbd_new_packet(uint8 type, uint16 size) {
  struct _usb_qtd_s *pack = _usb_ehci_free_qtd();
  pack->next_ptr = 0x0;
  pack->alt_ptr = 0x1;
  pack->token = 0x0;

  if (type == USB_IN_PACKET || type == USB_OUT_PACKET) {
    pack->token |= 0x80000000;
  }

  pack->token = pack->token | (uint32)type << 8;  // Set PID flag
  pack->token = pack->token | (uint32)size << 16; // Set size field
  pack->token = pack->token | 0xC00;              // Set CERR field
  pack->token = pack->token | 0x80;               // Set status field to enable

  if (size > 0) {

    uint8 num_pages = (size % PAGE_SIZE) + 1;
    uint32 *buff = (uint32 *)_kalloc_page(num_pages);
    __memclr(buff, num_pages * PAGE_SIZE);

    pack->ptr0 = (uint32)buff;
    // TODO: Fix the buffers list
  } else {
    pack->ptr0 = 0x0;
  }

  pack->ptr1 = 0x0;
  pack->ptr2 = 0x0;
  pack->ptr3 = 0x0;
  pack->ptr4 = 0x0;

  return pack;
}

/**
** Release a packet
*/
void _usbd_free_packet(USBPacket pack) {
  pack->next_ptr = 0x1; // Just to make sure the packet is marked invalid
  _kfree_page((void *)pack->ptr0); // free up the buffer
  _usb_ehci_release_qtd(pack);
}

/**
** Flag a packet to interrupt on complete
*/
void _usbd_set_ioc_packet(USBPacket pack) {
  pack->token = pack->token | 0x8000;
}

/**
** Print the current status of an endpoint to the console
*/
void _usbd_print_endpoint(USBEndpoint *tran) {
  __cio_printf("USB Endpoint Dump\n");
  __cio_printf("QH %x: HPtr: %x Token: %x EPCh: %x ", tran->qh,
               tran->qh->dword0, tran->qh->dword1, tran->qh->dword2);
  __cio_printf("cQTD: %x nQTD: %x\n", tran->qh->dword3, tran->qh->dword4);
  struct _usb_qtd_s *cqtd = tran->first_packet;
  uint32 cqtda = (uint32)cqtd;
  int cqtdN = 1;
  while ((cqtda & 0x1) != 0x1) {
    uint16 packSize = (cqtd->token & 0x7FFF0000) >> 16;
    __cio_printf("qTD%d %x: Next: %x Token: %x Endpoint Size: %d CPage: %d "
                 "PID: %x Status: %x",
                 cqtdN, cqtd, cqtd->next_ptr, cqtd->token, packSize,
                 (cqtd->token & 0x7000) >> 12, (cqtd->token & 0x300) >> 8,
                 cqtd->token & 0xFF);
    __cio_printf(" Buffptr 0: %x \n", cqtd->ptr0);

    cqtda = cqtd->next_ptr;
    cqtd = (struct _usb_qtd_s *)cqtd->next_ptr;
    cqtdN += 1;
  }
}

/**
** Add data to a packet
*/
void _usbd_add_data_packet(USBPacket pack, void *data, uint16 size) {
  __memcpy((void *)(pack->ptr0 & 0xFFFFF000), data, size);
}

/**
** Get the data from a packet
*/
void _usbd_get_data_packet(USBPacket pack, void *dst, uint16 size) {
  void *src = (void *)(pack->ptr0 & 0xFFFFF000);
  __memcpy(dst, src, size);
}

/**
** Wait for a packet to have been transfered
*/
void _usbd_wait_for_packet(USBPacket pack) {
  while ((pack->token & 0xFF) == 0x80) {
    //__cio_printf("Wait for packet");
    // Wait for the packet
  }
}
