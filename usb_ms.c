/**
** File: usb_ms.c
**
** Author: Tyler Wilcox
**
** Contributors:
**
** Description: A driver for USB Mass Storage Devices
*/

#include "usb_ms.h"
#include "cio.h"
#include "klib.h"

/**
** This structure outlines the fields in a
** Command Block Wrapper
**/
struct _usb_ms_cbw_s {
  int32 signature;   // A fixed number USB_MS_CBW_SIG
  int32 tag;         // A unique number to each transfer
  int32 length;      // How much data are we transfering
  uint8 direction;   // Which direction will the data flow
  uint8 unitNum;     // The LUN or Logic Unit Number
  uint8 cmdLength;   // Length of cmdData array
  uint8 cmdData[16]; // The command data
};

/**
** This structure outlines the fields in a
** Command Status Wrapper
*/
struct _usb_ms_csw_s {
  int32 signature; // A fixed number USB_MS_CSW_SIG
  int32 tag;       // A unique number to each transfer
                   // Should be the same as the CBW tag
  int32 residue;   // How much data wasn't transfered
  uint8 status;    // The status of the command
};

/**
** The USB Device reference
**/
USBDev *massStorDev;

/**
** A simple iterator for the tag
**/
static int _usb_ms_tag = 10;

/**
** The handler fuction for the USB driver.
** This function determins if a new USB device is a Mass Storage device
** If it is then we save that device
**
** @param dev  The USB device we are checking
** @param class  The class of the USB device
** @param subClass  The subClass of the USB device
** @param protocol  The protocol of the USB device
**
** @returns 'true' if this is a device we want. 'false' otherwise
*/
bool _usb_ms_handler(USBDev *dev, uint8 class, uint8 subClass, uint8 protocol) {

  if (class == 0x08 && subClass == 0x06 &&
      protocol == 0x50) {                // Does the device match what we need?
    massStorDev = dev;                   // Save the device
    __strcpy(dev->driverName, "USB_MS"); // Set the driver name
    return true;
  }
  return false;
}

/**
** Create a new Command Block Wrapper
**
** @param direction  The direction the command will be going
** @param dataLen  The ammount of data to copy in bytes
** @param cmdLen  The length of the command in bytes
** @param cmdData  The command data buffer
**
*/
struct _usb_ms_cbw_s _usb_ms_new_cbw(uint8 direction, uint32 dataLen,
                                     uint8 cmdLen, uint8 cmdData[]) {
  struct _usb_ms_cbw_s cbw;
  cbw.signature = USB_MS_CBW_SIG;
  cbw.tag = ++_usb_ms_tag;
  cbw.length = dataLen;
  cbw.direction = direction;
  cbw.unitNum = 0;
  cbw.cmdLength = cmdLen & 0x1F;
  for (int i = 0; i < cmdLen; ++i) { // Copy the command data
    cbw.cmdData[i] = cmdData[i];
  }

  return cbw;
}

/**
** Convert the data from a device into normal form.
** The data from a device is sent least significant byte first.
** This function converts it to most significant byte first
**
** @param data  The buffer of data to convert
** @param size  The size of the buffer in bytes
*/
void _usb_ms_convert_data(uint8 data[], uint16 size) {
  for (int i = 0; i < size; i += 2) {
    uint8 tmp = data[i];
    data[i] = data[i + 1];
    data[i + 1] = tmp;
  }
}

/**
** Read a block of data from the USB device
**
** @param data  The buffer to copy the data to
** @param blockAddr  The address of the block to read from
** @param numBytes  The number of bytes to read
*/
void _usb_ms_read_block(uint8 data[], uint32 blockAddr, uint16 numBytes) {
  uint8 cmdData[12];
  cmdData[0] = 0x28;
  cmdData[1] = 0x0;
  cmdData[2] = (uint8)(blockAddr >> 24); // Top byte
  cmdData[3] = (uint8)(blockAddr >> 16); // Top middle byte
  cmdData[4] = (uint8)(blockAddr >> 8);  // Bot middle byte
  cmdData[5] = (uint8)(blockAddr);       // Bot byte;
  cmdData[7] = (uint8)(numBytes >> 8);
  cmdData[8] = (uint8)(numBytes);

  cmdData[10] = 0x0;
  cmdData[11] = 0x0;

  // Create a new Command Block Wrapper
  struct _usb_ms_cbw_s cbw =
      _usb_ms_new_cbw(USB_MS_TOHOST_DIR, numBytes, 10, cmdData);

  // Create a new USB packet to send that CBW
  USBPacket commandPack = _usbd_new_packet(USB_OUT_PACKET, 31);
  _usbd_add_data_packet(commandPack, &cbw, 31);

  // Create packets for the in data and the status
  USBPacket inPack = _usbd_new_packet(USB_IN_PACKET, numBytes);
  USBPacket statusPack = _usbd_new_packet(USB_IN_PACKET, 13);

  // Get both endpoints. The BULK in and BULK out
  USBEndpoint *outEndp =
      _usbd_get_endpoint_type(massStorDev, USB_ENDP_OUT, USB_TT_BULK);
  USBEndpoint *inEndp =
      _usbd_get_endpoint_type(massStorDev, USB_ENDP_IN, USB_TT_BULK);

  // Send the command packet and wait
  _usbd_add_packet_to_endpoint(outEndp, commandPack);
  _usbd_wait_for_packet(commandPack);

  // Now we can send the in packet, wait for it to finish
  // then we get the status packet
  _usbd_add_packet_to_endpoint(inEndp, inPack);
  _usbd_wait_for_packet(inPack);
  _usbd_add_packet_to_endpoint(inEndp, statusPack);
  _usbd_wait_for_packet(statusPack);

  // Copy the data from the in packet
  _usbd_get_data_packet(inPack, data, numBytes);

  // Convert the to MSB
  _usb_ms_convert_data(data, numBytes);

  // Get the status packet
  struct _usb_ms_csw_s csw;
  _usbd_get_data_packet(statusPack, &csw, 13);

  // Free up the unused packets
  _usbd_free_packet(commandPack);
  _usbd_free_packet(inPack);
  _usbd_free_packet(statusPack);
}

/**
** Initialize the USB Mass Storage driver
*/
void _usb_ms_init(void) {
  _usbd_register_handler(&_usb_ms_handler); // Register the handler

  __cio_puts(" USB_MS");
}

/**
** Read a certain number of bytes from the USB Mass Storage Drive
*/
void _usb_ms_read(uint8 data[], uint32 blockAddr, uint16 numBytes) {

  for (int i = 0; i < numBytes / USB_MS_BLOCK_SIZE;
       i++) { // Loop for every block to read
    // Get the next block
    _usb_ms_read_block(&data[i * USB_MS_BLOCK_SIZE], blockAddr,
                       USB_MS_BLOCK_SIZE);
  }
}