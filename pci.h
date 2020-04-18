
#ifndef _PCI_H_
#define _PCI_H_

#define MAX_PCI_DEVICES 15
#define PCI_ADDR_PORT   0xCF8
#define PCI_VALUE_PORT  0xCFC
#define PCI_COMMAND     0x04

#include "common.h"

#ifndef __SP_ASM__


typedef struct pci_dev_s {
  int id;

  uint8 bus;
  uint8 slot;
  uint8 func;

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

void _pci_init( void );

uint32 _pci_config_read ( uint8 bus, uint8 slot, uint8 func, uint8 offset );
uint32 _pci_config_read32 ( PCIDev *dev, uint8 offset );
uint8 _pci_config_read8 ( PCIDev *dev, uint8 offset );
uint16 _pci_config_read16 ( PCIDev *dev, uint8 offset );

void _pci_enumerate_devices ( void );

void _pci_dump_all( void );

PCIDev* _pci_get_device( int devid );

PCIDev* _pci_get_device_class( uint8 class, uint8 subclass, uint8 progif );

PCIDev* _pci_get_device_vendorid_deviceid( uint16 vendor, uint16 device );

PCIDev* _pci_get_device_id( uint16 vendor, uint16 device, uint8 class,
                            uint8 subclass );

void _pci_write_field32( PCIDev *dev, uint8 offset, uint32 value );
void _pci_write_field16( PCIDev *dev, uint8 offset, uint16 value );
void _pci_write_field8( PCIDev *dev, uint8 offset, uint8 value );

uint32 _pci_calculate_address(uint8 bus, uint8 slot, uint8 func, uint8 offset,
                              uint8 size);

#endif

#endif
