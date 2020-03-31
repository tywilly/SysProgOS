
#define __SP_KERNEL__

#include "common.h"
#include "klib.h"

#include "pci.h"

void _pci_init( void ) {


  __cio_puts( " PCI" );

}


uint16 pciConfigReadWord (uint8 bus, uint8 slot, uint8 func, uint8 offset ) {
  uint32 address;
  uint32 lbus = (uint32) bus;
  uint32 lslot = (uint32) slot;
  uint32 lfunc = (uint32) func;
  uint16 tmp = 0;

  address = (uint32)(lbus << 16) | (lslot << 11) |
    (lfunc << 8) | (offset & 0xfc) | ((uint32) 0x80000000);

  __outl(0xCF8, address);

  tmp = (uint16)((__inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
  return tmp;

}

uint16 pciCheckVendor (uint8 bus, uint8 slot ) {
  uint16 vendor, device;

  if ( (vendor = pciConfigReadWord( bus, slot, 0, 0 )) != 0xFFFF ) {
    device = pciConfigReadWord( bus, slot, 0, 2 );

    __cio_printf( "Vendor: %x Device: %x", vendor, device);
  }

  return vendor;
}
