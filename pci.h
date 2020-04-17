#ifndef _PCI_H_
#define _PCI_H_

#define MAX_PCI_DEVICES 15

typedef struct pci_device_s {
    uint8 bus;
    uint8 device;
    uint8 function;
    uint8 class;
    uint8 subClass;
    uint8 progIF;
    uint16 vendorID;
    uint16 deviceID;
    uint32 bar0;
} PCIDevice;

void _pci_init( void );

uint32 _pci_cfg_read_l( uint8 bus, uint8 device, uint8 function, uint8 offset );
uint16 _pci_cfg_read_w( uint8 bus, uint8 device, uint8 function, uint8 offset );
uint8 _pci_cfg_read_b( uint8 bus, uint8 device, uint8 function, uint8 offset );

void _pci_cfg_write_l( uint8 bus, uint8 device, uint8 function, uint8 offset, uint32 value );
void _pci_cfg_write_w( uint8 bus, uint8 device, uint8 function, uint8 offset, uint16 value );
void _pci_cfg_write_b( uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 value );

void _pci_set_command( uint8 bus, uint8 device, uint8 function, uint16 value );
uint16 _pci_get_status( uint8 bus, uint8 device, uint8 function );

PCIDevice *_pci_dev_class( uint8 class, uint8 subClass, uint8 progIF );

void _pci_dump_all( void );
void _pci_dump_header( uint8 bus, uint8 device, uint8 function, uint8 nlines );

#endif