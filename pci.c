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

#define __SP_KERNEL__

#include "common.h"
#include "klib.h"
#include "e1000.h"

#include "pci.h"

// A list of PCI devices
PCIDev _pci_dev_list[MAX_PCI_DEVICES];

// The current number of PCI devices
int _pci_num_dev = 0;

/**
** Add the PCI device to the list if it is valid.
** Note: This routine is recursive.
** If a device has multiple functions this routine will add them.
**
** @param bus  The bus of the device
** @param slot  The slot of the device
** @param func  The function of the device
**/
void _pci_add_device(uint8 bus, uint8 slot, uint8 func) {
  uint16 vendor = _pci_config_read(bus, slot, func, 0x00) & 0xFFFF;

  if (vendor != 0xFFFF) { // Is it a valid device?
    // Get an empty PCIDev reference
    PCIDev *dev = &_pci_dev_list[_pci_num_dev];

    // Set all the device data
    dev->id = _pci_num_dev;
    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;
    dev->vendorid = vendor;
    dev->deviceid = (_pci_config_read(bus, slot, func, 0x00) >> 16);
    uint16 codes = (_pci_config_read(bus, slot, func, 0x08) >> 16);
    dev->class = codes >> 8;
    dev->subclass = codes & 0xFF;
    dev->headertype = (_pci_config_read(bus, slot, func, 0x0C) >> 16) & 0xFF;
    dev->progif = ((_pci_config_read(bus, slot, func, 0x08) >> 8) & 0xFF);

    // Set the base address pointers
    dev->bar0 = _pci_config_read(bus, slot, func, 0x10);
    dev->bar1 = _pci_config_read(bus, slot, func, 0x14);
    dev->bar2 = _pci_config_read(bus, slot, func, 0x18);
    dev->bar3 = _pci_config_read(bus, slot, func, 0x1C);
    dev->bar4 = _pci_config_read(bus, slot, func, 0x20);
    dev->bar5 = _pci_config_read(bus, slot, func, 0x24);

    // Set the interrupt pin and line
    dev->interrupt = (uint8)(_pci_config_read(bus, slot, func, 0x3C) & 0xFF);
    dev->interruptPin =
        (uint8)(_pci_config_read(bus, slot, func, 0x3C) >> 8 & 0xFF);

    _pci_num_dev++;

    // Check if the device supports multiple functions
    if ((dev->headertype & 0x80) != 0) {
      for (int f = 1; f < 8; f++) {
        _pci_add_device(bus, slot, f);
      }
    }
  }
}

//
// _pci_init() - initialize the PCI module
//
void _pci_init(void) {

  _pci_enumerate_devices(); // Find some PCI devices

  __cio_puts(" PCI");
}

//
// _pci_config_read() - Read a long word of data in the PCI config.
//
uint32 _pci_config_read(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
  uint32 address;
  uint32 lbus = (uint32)bus;
  uint32 lslot = (uint32)slot;
  uint32 lfunc = (uint32)func;
  uint32 tmp = 0;

  address = (uint32)(lbus << 16) | (lslot << 11) | (lfunc << 8) |
            (offset & 0xfc) | ((uint32)0x80000000); // Construct device address

  __outl(0xCF8, address);

  tmp = (uint32)((__inl(0xCFC))); // Get config data for the device
  return tmp;
}

//
// _pci_config_write() - Write a long word of data to the PCI config.
//
void _pci_config_write(uint8 bus, uint8 slot, uint8 func, uint8 offset,
                       uint32 data) {
  uint32 address;
  uint32 lbus = (uint32)bus;
  uint32 lslot = (uint32)slot;
  uint32 lfunc = (uint32)func;

  address = (uint32)(lbus << 16) | (lslot << 11) | (lfunc << 8) |
            (offset & 0xfc) | ((uint32)0x80000000);

  __outl(0xCF8, address);

  __outl(0xCFC, data);
}

//
// _pci_set_interrupt() - Set the interrupt pin and line of a device.
//
void _pci_set_interrupt(PCIDev *dev, uint8 interruptPin, uint8 interrupt) {
  dev->interrupt = interrupt;

  // Get the current long word from the config
  uint32 tmp = _pci_config_read(dev->bus, dev->slot, dev->func, 0x3C);
  // Reset the interrupt data and then set the new data
  tmp = tmp & 0xFFFF0000;
  tmp = tmp | (uint32)interruptPin << 8;
  tmp = tmp | (uint32)interrupt;

  // Write it back to the config
  _pci_config_write(dev->bus, dev->slot, dev->func, 0x3C, tmp);
}

//
// _pci_enumerate_devices() - Find all connected PCI devices and add them to a
// list.
//
void _pci_enumerate_devices(void) {
  _pci_num_dev = 0; // Reset dev counter

  for (uint16 bus = 0; bus < 256; bus++) {    // All 256 bus's
    for (uint8 slot = 0; slot < 32; slot++) { // All 32 devices
      _pci_add_device(bus, slot, 0);
    }
  }
}

//
// _pci_dump_all() - List all the connected PCI devices on the console.
//
void _pci_dump_all(void) {
  for (int i = 0; i < MAX_PCI_DEVICES; i++) {
    PCIDev *dev = &_pci_dev_list[i];
    __cio_printf("%d: Vendor: %04x Device: %04x Class: %02x SubClass: %02x "
                 "Progif: %02x Int: %02x\n",
                 i, dev->vendorid, dev->deviceid, dev->class, dev->subclass,
                 dev->progif, dev->interrupt);
  }
}

//
// _pci_get_device() - Get a device by the id
//
PCIDev *_pci_get_device(int devid) {
  if (devid < MAX_PCI_DEVICES) {
    return &_pci_dev_list[devid];
  }
  return NULL;
}

//
// _pci_get_device_class() - Get a device by class, subclass and progif.
//
PCIDev *_pci_get_device_class(uint8 class, uint8 subclass, uint8 progif) {
  for (int i = 0; i < MAX_PCI_DEVICES; i++) { // Loop through all the devices
    PCIDev dev = _pci_dev_list[i];

    // Does the device match what we want?
    if (dev.class == class && dev.subclass == subclass && dev.progif == progif)
      return &_pci_dev_list[i]; // Return it!
  }
  return NULL;
}
