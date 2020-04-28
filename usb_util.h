/*
** File: usb_util.h
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: Useful methods when using USB
*/

#ifndef _USB_UTIL_H_
#define _USB_UTIL_H_

// USB device request types
#define GET_STATUS 0x0
#define CLEAR_FEATURE 0x1
#define SET_FEATURE 0x3
#define SET_ADDRESS 0x5
#define GET_DESCRIPTOR 0x6
#define SET_DESCRIPTOR 0x7
#define GET_CONFIGURATION 0x8
#define SET_CONFIGURATION 0x9
#define GET_INTERFACE 0xA
#define SET_INTERFACE 0xB
#define SYNC_FRAME 0xC

// USB Device descriptor types
#define DEVICE 0x1
#define CONFIGURATION 0x2
#define STRING 0x3
#define INTERFACE 0x4
#define ENDPOINT 0x5
#define DEVICE_QUALIFIER 0x6
#define OTHER_SPEED_CONFIGURATION 0x7
#define INTERFACE_POWER 0x8

#include "common.h"

/**
** Create the payload for a device request packet
**
** @param dst  The buffer to write the payload to
** @param bmRequestType  The request type
** @param bRequest  The request
** @param wValue  The value field of the request
** @param wIndex  The index of the request
** @param wLength  The length of the request
*/
void usb_device_request_packet(void* dst, uint8 bmRequestType, uint8 bRequest,
                               uint16 wValue, uint16 wIndex, uint16 wLength);

/**
** Take two bytes and form a word
**
** @param byte1  The upper 8 bits
** @param byte2  The lower 8 bits
*/
uint16 usb_bytes_to_word(uint8 byte1, uint8 byte2);

#endif
