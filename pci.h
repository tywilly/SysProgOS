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

uint16 _pci_get_command( uint8 bus, uint8 device, uint8 function );

void _pci_command_enable( uint8 bus, uint8 device, uint8 function, uint16 command );
void _pci_command_disable( uint8 bus, uint8 device, uint8 function, uint16 command );

PCIDevice *_pci_dev_class( uint8 class, uint8 subClass, uint8 progIF );

void _pci_dump_all( void );

#endif