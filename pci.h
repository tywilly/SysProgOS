
#ifndef _PCI_H_
#define _PCI_H_

#define MAX_PCI_DEVICES 15

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
  uint8 interruptPin;

  uint16 vendorid;
  uint16 deviceid;

  uint32 bar0;
  uint32 bar1;
  uint32 bar2;
  uint32 bar3;
  uint32 bar4;
  uint32 bar5;

} PCIDev;

void _pci_init( void );

uint32 _pci_config_read ( uint8 bus, uint8 slot, uint8 func, uint8 offset );

void _pci_config_write_byte(uint8 bus, uint8 slot, uint8 func, uint8 offset, uint8 data);

void _pci_enumerate_devices ( void );

void _pci_dump_all( void );

PCIDev* _pci_get_device( int devid );

PCIDev* _pci_get_device_class( uint8 class, uint8 subclass, uint8 progif );

void _pci_set_interrupt( PCIDev* dev, uint8 interruptPin, uint8 interrupt );

#endif

#endif
