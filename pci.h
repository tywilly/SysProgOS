/**
** File: pci.c
**
** Author: Tyler Wilcox
**
** Contributor:
**
** Description: PCI module implementation.
** Refer to https://wiki.osdev.org/PCI for more info.
**/

#ifndef _PCI_H_
#define _PCI_H_

// The maximum number of PCI devices to store
#define MAX_PCI_DEVICES 15

#include "common.h"

#ifndef __SP_ASM__

/**
** A structure representing a PCI device.
** It contains all the important data
**/
typedef struct pci_dev_s {
  int id;             // A unique ID of the deivce
  uint8 bus;          // What bus the device is on
  uint8 slot;         // What slot the device is in
  uint8 func;         // What function number this device has
  uint8 class;        // The class code of the device
  uint8 subclass;     // The subclass of the device
  uint8 headertype;   // The header type
  uint8 progif;       // The progif of the device
  uint8 interrupt;    // What interrupt line the device is on
  uint8 interruptPin; // What interrupt pin the device uses

  uint16 vendorid; // The vender ID of the device
  uint16 deviceid; // The device ID of the device

  uint32 bar0; // Base address 0
  uint32 bar1; // Base address 1
  uint32 bar2; // Base address 2
  uint32 bar3; // Base address 3
  uint32 bar4; // Base address 4
  uint32 bar5; // Base address 5

} PCIDev;

/**
** The initialization routine for the PCI module
**
**/
void _pci_init(void);

/**
** Read a long word of data from the PCI config register for a device.
**
** @param bus  The bus of the device
** @param slot  The slot of the deivce
** @param func  The function of the device
** @param offset  The offset from the config base address
**
** @returns a long word
**/
uint32 _pci_config_read(uint8 bus, uint8 slot, uint8 func, uint8 offset);

/**
** Write a long word of data to the PCI config register for a device.
**
** @param bus  The bus of the device
** @param slot  The slot of the device
** @param func  The function of the device
** @param offset  The offset from the config base address
** @param data  The long word to write
**
**/
void _pci_config_write(uint8 bus, uint8 slot, uint8 func, uint8 offset,
                       uint32 data);

/**
** Set the interrupt pin and interrupt line of a PCI device.
**
** @param dev  The PCI device to update
** @param interruptPin  The new interrupt pin
** @param interrupt  The new interrupt line
**
**/
void _pci_set_interrupt(PCIDev *dev, uint8 interruptPin, uint8 interrupt);

/**
** Detect and add any valid PCI device to the list.
**/
void _pci_enumerate_devices(void);

/**
** List all the connected PCI devices on the console.
**/
void _pci_dump_all(void);

/**
** Get a PCI device reference based on the ID of the device
**
** @param devid  The ID of the device to get
**
** @returns a pointer to a PCIDev. NULL if no device is found.
**/
PCIDev *_pci_get_device(int devid);

/**
** Get a PCI device reference based on the class, subClass and progif.
**
** @param class  The class to look for
** @param subclass The subClass to look for
** @param progif  The progif to look for
**
** @returns a pointer to a PCIDev. NULL if no device matches.
**/
PCIDev *_pci_get_device_class(uint8 class, uint8 subclass, uint8 progif);

#endif

#endif
