/*
** File: pci.c
** 
** Author: Tyler Wilcox
** 
** Contributor: Cody Burrows
** 
** Description: PCI module implementation.
** 
** Refer to https://wiki.osdev.org/PCI for more info.
** Note that since our two teams originally intended to work together, this 
** module was co-developed so that we didn't all step on each others' toes too
** badly when it came time to merge everything together. Obviously, that didn't
** pan out...
*/

#define __SP_KERNEL__

#include "common.h"
#include "klib.h"

#include "pci.h"

PCIDev _pci_dev_list[MAX_PCI_DEVICES];

int _pci_num_dev = 0;

//
// _pci_init() - initialize the PCI module
//
void _pci_init( void ) {

  _pci_enumerate_devices (); // Find some PCI devices

  __cio_puts( " PCI" );

}

// Calculate the address of the desired field
uint32 _pci_calculate_address(uint8 bus, uint8 slot, uint8 func, uint8 offset, 
                              uint8 size) {
    uint32 lbus = (uint32) bus;
    uint32 lslot = (uint32) slot;
    uint32 lfunc = (uint32) func;
    uint8 size_mask = 0xFF;
    // This is gross, but it works.
    if (size == 4) {
        size_mask = 0xFC;
    } else if (size == 2) {
        size_mask = 0xFE;
    }

    return (uint32)(lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & size_mask) | ((uint32) 0x80000000);
}

//
// _pci_config_read() - Read a word of data in the PCI config.
//
uint32 _pci_config_read (uint8 bus, uint8 slot, uint8 func, uint8 offset ) {
  uint32 tmp = 0;
  uint32 address = _pci_calculate_address(bus, slot, func, offset, 4);

  __outl(PCI_ADDR_PORT, address);

  tmp = (uint32)((__inl(PCI_VALUE_PORT))); // Get config data for the device
  return tmp;

}

//
// _pci_add_device() - Add the PCI device to the list if it is valid.
//
void _pci_add_device( uint8 bus, uint8 slot, uint8 func ) {
  uint16 vendor = _pci_config_read (bus, slot, func, 0x00) & 0xFFFF;

  if(vendor != 0xFFFF) { // Is it a valid device?
    PCIDev* dev = &_pci_dev_list[_pci_num_dev];
    dev->id = _pci_num_dev;
    dev->vendorid = vendor;
    dev->deviceid = (_pci_config_read (bus, slot, func, 0x00) >> 16);
    uint16 codes = (_pci_config_read (bus, slot, func, 0x08) >> 16);
    dev->class = codes >> 8;
    dev->subclass = codes & 0xFF;
    dev->headertype = (_pci_config_read (bus, slot, func, 0x0C) >> 16) & 0xFF;

    dev->progif = ((_pci_config_read (bus, slot, func, 0x08) >> 8) & 0xFF);

    dev->bar0 = _pci_config_read (bus, slot, func, 0x10);
    dev->bar1 = _pci_config_read (bus, slot, func, 0x14);
    dev->bar2 = _pci_config_read (bus, slot, func, 0x18);
    dev->bar3 = _pci_config_read (bus, slot, func, 0x1C);
    dev->bar4 = _pci_config_read (bus, slot, func, 0x20);
    dev->bar5 = _pci_config_read (bus, slot, func, 0x24);

    dev->interrupt = (_pci_config_read (bus, slot, func, 0x3C) & 0xFF);

    dev->bus = bus;
    dev->slot = slot;
    dev->func = func;

    _pci_num_dev++;

    // Check if the device supports multiple functions
    if ((dev->headertype & 0x80) != 0) {
      for(int f=1; f<8; f++) {
        _pci_add_device(bus, slot, f);
      }
    }

  }

}

//
// _pci_enumerate_devices() - Find all connected PCI devices and add them to a list.
//
void _pci_enumerate_devices ( void ) {

  _pci_num_dev = 0; // Reset dev counter

  for(uint16 bus=0; bus<256; bus++) { // All 256 bus's
    for(uint8 slot=0; slot<32;slot++) { // All 32 devices
      _pci_add_device( bus, slot, 0 );
    }
  }

}

//
// _pci_dump_all() - List all the connected PCI devices on the console.
//
void _pci_dump_all( void ) {
  for (int i=0; i<MAX_PCI_DEVICES;i++) {
    PCIDev* dev = &_pci_dev_list[i];
    __cio_printf( "%d: Vendor: %04x Device: %04x Class: %02x SubClass: %02x Progif: %02x\n", i, dev->vendorid,
                  dev->deviceid, dev->class, dev->subclass, dev->progif);
  }
}

//
// _pci_get_device() - Get a device by the id
//
PCIDev* _pci_get_device( int devid ) {
  return &_pci_dev_list[devid];
}

//
// _pci_get_device_class() - Get a device by class, subclass and progif.
//
PCIDev* _pci_get_device_class( uint8 class, uint8 subclass, uint8 progif) {
  for(int i=0; i<MAX_PCI_DEVICES; i++) {
    PCIDev dev = _pci_dev_list[i];

    if(dev.class == class && dev.subclass == subclass && dev.progif == progif)
      return &_pci_dev_list[i];
  }
  return 0;
}

// Get a device by vendor id, device id, and sub/class
PCIDev* _pci_get_device_id( uint16 vendor, uint16 device, uint8 class, 
                            uint8 subclass) {
    for (int i = 0; i < MAX_PCI_DEVICES; ++i) {
        PCIDev dev = _pci_dev_list[i];

        if (dev.vendorid == vendor && dev.deviceid == device &&
            dev.class == class && dev.subclass == subclass) {
            return &_pci_dev_list[i];
        }
    }

    return 0;
}

// Write a 32-bit PCI field
void _pci_write_field32( PCIDev *dev, uint8 offset, uint32 value) {
    uint32 address = _pci_calculate_address(dev->bus, dev->slot, dev->func, 
                                            offset, 4);

    __outl(PCI_ADDR_PORT, address);
    __outl(PCI_VALUE_PORT, value);
}

// Read a 32-bit PCI field
uint32 _pci_config_read32( PCIDev *dev, uint8 offset ) {
    return _pci_config_read( dev->bus, dev->slot, dev->func, offset );
}

// Read a 16-bit PCI field
uint16 _pci_config_read16( PCIDev *dev, uint8 offset ) {
  uint16 tmp = 0;
  uint32 address = _pci_calculate_address(dev->bus, dev->slot, dev->func, 
                                          offset, 2);

  __outl(PCI_ADDR_PORT, address);

  tmp = (uint16)((__inw(PCI_VALUE_PORT)));
  return tmp;
}

// Read an 8-bit PCI field
uint8 _pci_config_read8( PCIDev *dev, uint8 offset ) {
  uint8 tmp = 0;
  uint32 address = _pci_calculate_address(dev->bus, dev->slot, dev->func, 
                                          offset, 1);

  __outl(PCI_ADDR_PORT, address);

  tmp = (uint8)((__inb(PCI_VALUE_PORT)));
  return tmp;
}

// Write a 16-bit PCI field
void _pci_write_field16( PCIDev *dev, uint8 offset, uint16 value ) {
    uint32 address = _pci_calculate_address(dev->bus, dev->slot, dev->func, 
                                            offset, 2);

    __outl(PCI_ADDR_PORT, address);
    __outw(PCI_VALUE_PORT, value);
}

// Write an 8-bit PCI field
void _pci_write_field8( PCIDev *dev, uint8 offset, uint8 value) {
    uint32 address = _pci_calculate_address(dev->bus, dev->slot, dev->func, 
                                            offset, 1);

    __outl(PCI_ADDR_PORT, address);
    __outb(PCI_VALUE_PORT, value);
}
