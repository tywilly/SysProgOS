

#ifndef _USBD_H_
#define _USBD_H_

#define USB_OUT_PACKET 0x0
#define USB_IN_PACKET 0x1
#define USB_SETUP_PACKET 0x2

#define USB_MAX_ENDPOINTS 10
#define USB_MAX_DEVICES 128

#define USB_FULL_SPEED 0x0
#define USB_LOW_SPEED 0x1
#define USB_HIGH_SPEED 0x2

#include "common.h"


#ifndef __SP_ASM__

typedef struct _usb_endpoint_s USBEndpoint;
typedef struct _usb_dev_s USBDev;
typedef struct _usb_qtd_s* USBPacket;

void _usbd_init( void );

void _usbd_list_devices( void );

void _usbd_enumerate_devices( void );

USBEndpoint* _usbd_new_endpoint(uint8 addr, uint8 endpoint, uint8 speed, uint16 maxPacketSize);

USBPacket _usbd_new_packet(uint8 type, uint16 size);

void _usbd_free_packet(USBPacket pack);

void _usbd_add_packet_to_endpoint(USBEndpoint* tran, USBPacket pack);

void _usbd_add_data_packet(USBPacket pack, void * data, uint16 size);

void _usbd_schedule_endpoint(USBEndpoint * endpoint);

void _usbd_print_endpoint(USBEndpoint * tran);

void _usbd_set_ioc_packet(USBPacket pack);

void _usbd_get_data_packet(USBPacket pack, void * dst, uint16 size);

void _usbd_wait_for_packet(USBPacket pack);

#endif
#endif
