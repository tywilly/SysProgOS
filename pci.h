
#ifndef _PCI_H_
#define _PCI_H_

#include "common.h"

#ifndef __SP_ASM__

void _pci_init( void );

uint16 pciConfigReadWord ( uint8 bus, uint8 slot, uint8 func, uint8 offset );

uint16 pciCheckVendor ( uint8 bus, uint8 slot );

#endif

#endif
