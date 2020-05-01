/*
** File: usbd.h
**
** Author: Tyler Wilcox
**
** Contributors:
**
** Description: Driver for the entire USB system. Contains API used to interact
*with the USB system.
*/

#ifndef _USBD_H_
#define _USBD_H_

// USB Packet types
#define USB_OUT_PACKET 0x0
#define USB_IN_PACKET 0x1
#define USB_SETUP_PACKET 0x2

// Maximums for USB system
#define USB_MAX_ENDPOINTS 10
#define USB_MAX_DEVICES 128
#define USB_MAX_DRIVER_NAME 10

// USB device speed classes
#define USB_FULL_SPEED 0x0
#define USB_LOW_SPEED 0x1
#define USB_HIGH_SPEED 0x2

// USB Endpoint Directions
#define USB_ENDP_IN 0x1
#define USB_ENDP_OUT 0x0

// USB Endpoint Tranfer Type
#define USB_TT_CONTROL 0x0
#define USB_TT_ISO 0x1
#define USB_TT_BULK 0x2
#define USB_TT_INT 0x3

#include "common.h"

#ifndef __SP_ASM__

typedef struct _usb_qtd_s *USBPacket;

/**
** A data structure representing an endpoint
*/
typedef struct _usb_endpoint_s {
  struct _usb_qh_s *qh;   // The Queue Head associated with an endpoint
  USBPacket first_packet; // The first packet in the queue
  bool isInEndpoint;
  uint8 attributes;
} USBEndpoint;

/**
** A data structure representing a USB device
*/
typedef struct _usb_dev_s {
  bool active;                              // Is the device active?
  USBEndpoint endpoints[USB_MAX_ENDPOINTS]; // All the endpoints in a device
  char driverName[USB_MAX_DRIVER_NAME];     // String of the driver name
} USBDev;

/**
** _usbd_init - USB initialization routine
**
** Called by the USB module after the host controllers init.
*/
void _usbd_init(void);

/**
** Register a driver handler function.
** A driver handler function is called whenever a new device is attached.
**
** @param handler  A function pointer to the handler
*/
void _usbd_register_handler(bool (*handler)(USBDev *, uint8, uint8, uint8));

/**
** Set the address of a USB device.
**
** @param dev  The USB device to re-address
** @param newAddr  The new address of the device.
**
** Note: newAddr can only be between 1-128 and must be unique.
**       The address can only be set when the device is in address mode.
**
*/
void _usbd_set_device_address(USBDev *dev, uint8 newAddr);

/**
** Set the device configuration.
**
** @param dev  The USB device
** @param bConfigurationValue  The configuration number
**
** Note: The configuration value must be a valid number.
**       Can be found in the configuration descriptor.
*/
void _usbd_set_device_config(USBDev *dev, uint8 bConfigurationValue);

/**
** Get the configuration descriptor from a device.
**
** @param dev  The USB device
** @param dst  a buffer to copy the data into
** @param descType  The type of descriptor to get
** @param descIndex  The index of the descriptor
*/
void _usbd_get_device_desc(USBDev *dev, void *dst, uint8 descType,
                           uint8 descIndex);

/**
** Get a string descriptor from a device.
**
** @param dev  The USB device
** @param dst  a buffer the copy the string into
** @param descIndex  the index of the string
*/
void _usbd_get_string_desc(USBDev *dev, void *dst, uint8 descIndex);

/**
** Get a USB device based on the class and subClass.
** If no device matches then NULL is returned
**
** @param class  The class to look for
** @param subClass  The subclass to look for
** @param protocol  The protocol to look for
** Note: If subClass is 0xFF, then the subclass and protocol are ommitted
** @returns A poninter to a USBDev
*/
USBDev *_usbd_get_device(uint8 class, uint8 subClass, uint8 protocol);

/**
** Print a list of the currently attached USB devices to the console.
**
*/
void _usbd_list_devices(void);

/**
** If a device has been attached, assign it a unique address
** and set the configuration to the default.
**
**
*/
void _usbd_enumerate_devices(void);

/**
** Create and initalize an endpoint on a device
**
** @param addr  The address of the device
** @param endpoint  The endpoint number to create
** @param speed  The speed class of the new endpoint
** @param maxPacketSize  The maximum packet size of the endpoint
** @returns A pointer to an endpoint struct
*/
USBEndpoint *_usbd_new_endpoint(uint8 addr, uint8 endpoint, uint8 speed,
                                uint16 maxPacketSize);

USBEndpoint *_usbd_get_endpoint(USBDev *dev, uint8 endpointN);

USBEndpoint *_usbd_get_endpoint_type(USBDev *dev, bool endpDir,
                                     uint8 transType);

/**
** Create and return a new USB packet of a specific size
**
** @param type  The packet type
** @param size  The size of the packet in bytes
** @returns  A USBPacket with type 'type' and size 'size'
*/
USBPacket _usbd_new_packet(uint8 type, uint16 size);

/**
** Release a USB packet.
**
** @param pack  The USBPacket to be freed
*/
void _usbd_free_packet(USBPacket pack);

/**
** Add a packet to an endpoint so it can be sent.
**
** Note: The packet list in an endpoint is a linked list.
**       The packets get added from the head of the list.
**       Therefor the last packet shoule be added first
**       and the first packet should be added last.
**
** @param endp  The endpoint
** @param pack  The packet
*/
void _usbd_add_packet_to_endpoint(USBEndpoint *endp, USBPacket pack);

/**
** Add a buffer of data to a packet.
**
** @param pack  The packet
** @param data  The buffer of data to add
** @param size  The size of the buffer
*/
void _usbd_add_data_packet(USBPacket pack, void *data, uint16 size);

/**
** Schedule an endpoint so packets can be sent.
**
** @param endpoint  The endpoint to schedule
*/
void _usbd_schedule_endpoint(USBEndpoint *endpoint);

/**
** Print the current status of an endpoint to the console
**
** @param endp  The endpoint to print
*/
void _usbd_print_endpoint(USBEndpoint *endp);

/**
** Set the interrupt on complete for a packet
**
** @param pack  The packet
*/
void _usbd_set_ioc_packet(USBPacket pack);

/**
** Get the data from a packet and copy it to a buffer
**
** @param pack  The packet
** @param dst  The buffer to copy to
** @param size  The the size in bytes to copy from the packet
*/
void _usbd_get_data_packet(USBPacket pack, void *dst, uint16 size);

/**
** Block execution till a packet has been sent or delievred
**
** @param pack  The packet to wait for
*/
void _usbd_wait_for_packet(USBPacket pack);

#endif
#endif
