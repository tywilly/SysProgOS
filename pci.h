/**
 ** File: pci.h
 **
 ** Author: Tyler Wilcox
 **
 ** Contributor: Cody Burrows (cxb2114@rit.edu)
 **
 ** This file declares various PCI-related functionality.
 */

#ifndef _PCI_H_
#define _PCI_H_

#define MAX_PCI_DEVICES 15
#define PCI_ADDR_PORT   0xCF8
#define PCI_VALUE_PORT  0xCFC
#define PCI_COMMAND     0x04

#include "common.h"

#ifndef __SP_ASM__

// Stores information about the PCI device
typedef struct pci_dev_s {
    int id;
    uint8 class;
    uint8 subclass;
    uint8 headertype;
    uint8 progif;
    uint8 interrupt;

    uint16 vendorid;
    uint16 deviceid;

    uint32 bar0;
    uint32 bar1;
    uint32 bar2;
    uint32 bar3;
    uint32 bar4;
    uint32 bar5;

    uint8 bus;
    uint8 slot;
    uint8 func;

} PCIDev;

/** Initializes the PCI module */
void _pci_init( void );

/**
  * Read the PCI device's configuration given its bus, slot, function, and
  * an offset.
  */
uint32 _pci_config_read ( uint8 bus, uint8 slot, uint8 func, uint8 offset );

/**
  * Helper function for reading a 1-byte PCI field.
  *
  * dev: A pointer to the PCI device to query
  * offset: The offset of the field to read.
  */
uint8 _pci_config_read8 ( PCIDev *dev, uint8 offset );

/**
  * Helper function for reading a 2-byte PCI field.
  *
  * dev: A pointer to the PCI device to query
  * offset: The offset of the field to read.
  */
uint16 _pci_config_read16 ( PCIDev *dev, uint8 offset );

/**
  * Helper function for reading a 4-byte PCI field.
  *
  * dev: A pointer to the PCI device to query
  * offset: The offset of the field to read.
  */
uint32 _pci_config_read32 ( PCIDev *dev, uint8 offset );

/** Detect all of the PCI devices present in the system */
void _pci_enumerate_devices ( void );

/** Print all of the detected devices to the console */
void _pci_dump_all( void );

/** Get a device by its ID */
PCIDev* _pci_get_device( int devid );

/** Get a device by its class, subclass and progif. */
PCIDev* _pci_get_device_class( uint8 class, uint8 subclass, uint8 progif );

/** Get a device by it's vendor and device ID. */
PCIDev* _pci_get_device_vendorid_deviceid( uint16 vendor, uint16 device );

/** Get a device by its vendor, device ID, class, and subclass. */
PCIDev* _pci_get_device_id( uint16 vendor, uint16 device, uint8 class,
                            uint8 subclass );

/**
  * A helper function to write a 1-byte value to a PCI field
  *
  * dev: The device to write the value to
  * offset: The offset of the field to write
  * value: what to write there
  */
void _pci_write_field8( PCIDev *dev, uint8 offset, uint8 value );

/**
  * A helper function to write a 2-byte value to a PCI field
  *
  * dev: The device to write the value to
  * offset: The offset of the field to write
  * value: what to write there
  */
void _pci_write_field16( PCIDev *dev, uint8 offset, uint16 value );

/**
  * A helper function to write a 4-byte value to a PCI field
  *
  * dev: The device to write the value to
  * offset: The offset of the field to write
  * value: what to write there
  */
void _pci_write_field32( PCIDev *dev, uint8 offset, uint32 value );

/**
  * Calculate the address of a PCI field given its bus, slot, function, offset,
  * and the size of the field.
  */
uint32 _pci_calculate_address(uint8 bus, uint8 slot, uint8 func, uint8 offset,
                              uint8 size);

#endif

#endif
