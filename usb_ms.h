/**
** File: usb_ms.h
**
** Author: Tyler Wilcox
**
** Contributors:
**
** Description: A driver for USB Mass Storage Devices
*/

#ifndef _USB_MS_H_
#define _USB_MS_H_

// The size of a block in bytes
#define USB_MS_BLOCK_SIZE 32

// Command Block Wrapper direction values
#define USB_MS_TODEVICE_DIR 0x00
#define USB_MS_TOHOST_DIR 0x80

// The signatures for a Command Block Wrapper
// and a Command Status Wrapper
#define USB_MS_CBW_SIG 0x43425355
#define USB_MS_CSW_SIG 0x53425355

#include "common.h"
#include "usbd.h"

/**
** Initialize the USB Mass Storage driver
*/
void _usb_ms_init(void);

/**
** Read a certain number of bytes from the USB Mass Storage Drive
**
** @param data  The buffer to copy the data to
** @param blockAddr  The address of the block to read from
** @param numBytes  The number of bytes to read
*/
void _usb_ms_read(uint8 data[], uint32 blockAddr, uint16 numBytes);

#endif