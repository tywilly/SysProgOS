

#include "usb_util.h"
#include "klib.h"



void usb_packet_write_byte(void* dst, uint8 offset, uint8 data) {
  uint8* dest = (uint8*) dst;
  __memcpy(&dest[offset], &data, 1);
}

void usb_packet_write_word(void* dst, uint8 offset, uint16 data){
  uint8* dest = (uint8*)dst;
  __memcpy(&dest[offset], &data, 2);
}

void usb_device_request_packet(void* dst, uint8 bmRequestType, uint8 bRequest,
                               uint16 wValue, uint16 wIndex, uint16 wLength) {

  usb_packet_write_byte(dst, 0, bmRequestType);
  usb_packet_write_byte(dst, 1, bRequest);
  usb_packet_write_word(dst, 2, wValue);
  usb_packet_write_word(dst, 4, wIndex);
  usb_packet_write_word(dst, 6, wLength);
}

uint16 usb_bytes_to_word(uint8 byte1, uint8 byte2) {
  uint16 tmp = 0x0;
  tmp |= byte2;
  tmp |= ((uint16) byte1) << 8;
  return tmp;
}
