/*
** File:    pci.h
**
** Author:  Yann MOULLEC
**
** Contributor:
**
** Description: PCI module declarations 
*/

#ifndef _PCI_H_
#define _PCI_H_

/*
** General (C and/or assembly) definitions
*/

/*
** Start of C-only definitions
*/

/*
** Types
*/

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

/*
** Prototypes
*/

//
// _pci_init() - checks all buses for connected devices
//
void _pci_init( void );

//
// _pci_dev_class() - get the first device descriptor matching the given class
//
// Parameters:
//    class         class code
//    subclass      subclass code
//    progIF        programming interface code
//
// Returns:
//    a pointer to the device descriptor
//
PCIDevice *_pci_dev_class( uint8 class, uint8 subClass, uint8 progIF );

//
// General read functions
//

//
// _pci_cfg_read_l() - read a long word from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the long word read
//
uint32 _pci_cfg_read_l( uint8 bus, uint8 device, uint8 function, uint8 offset );

//
// _pci_cfg_read_w() - read a word from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the word read
//
uint16 _pci_cfg_read_w( uint8 bus, uint8 device, uint8 function, uint8 offset );

//
// _pci_cfg_read_b() - read a byte from the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//
// Returns:
//    the byte read
//
uint8 _pci_cfg_read_b( uint8 bus, uint8 device, uint8 function, uint8 offset );

//
// General write functions
//

//
// _pci_cfg_write_l() - write a long word in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_l( uint8 bus, uint8 device, uint8 function, uint8 offset, uint32 value );

//
// _pci_cfg_write_w() - write a word in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_w( uint8 bus, uint8 device, uint8 function, uint8 offset, uint16 value );

//
// _pci_cfg_write_b() - write a byte in the device configuration register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    offset        configuration register offset
//    value         value to be written
//
void _pci_cfg_write_b( uint8 bus, uint8 device, uint8 function, uint8 offset, uint8 value );

//
// Register specific functions
//

//
// _pci_set_command() - set the device command register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    value         value to be written
//
void _pci_set_command( uint8 bus, uint8 device, uint8 function, uint16 value );

//
// _pci_set_interrupt() - set the device interrupt line and pin registers
//
// Parameters:
//    bus               bus number
//    device            device number
//    function          function number
//    interruptPin      value to be written in interruptPin
//    interruptLine     value to be written in interruptLine
//
uint16 _pci_get_status( uint8 bus, uint8 device, uint8 function );

//
// _pci_get_interrupt_line() - get the device interrupt line register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//
// Returns:
//    the interrupt line register
//
void _pci_set_interrupt( uint8 bus, uint8 device, uint8 function, uint8 interruptPin, uint8 interruptLine );

//
// _pci_get_status() - get the device status register
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//
// Returns:
//    the status register
//
uint8 _pci_get_interrupt_line( uint8 bus, uint8 device, uint8 function );

//
// Debugging function
//

//
// _pci_dump_all() - dump all device information to the console
//
void _pci_dump_all( void );

//
// _pci_dump_header() - dump device header information to the console
//
// Parameters:
//    bus           bus number
//    device        device number
//    function      function number
//    nlines        number of header lines to dump
//
void _pci_dump_header( uint8 bus, uint8 device, uint8 function, uint8 nlines );

#endif
