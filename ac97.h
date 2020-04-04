/*
** File:	ac97.h
**
** Author:	Cody Burrows (cxb2114@rit.edu)
**
** Contributor:
**
** Description:	Contains constants and declarations for the baseline ac97 
** driver.
*/

#ifndef _AC97_H_
#define _AC97_H_

#include "common.h"
#include "pci.h"

// Relevant constants
#define AC97_VENDOR_ID  0x8086  // Intel Corporation
#define AC97_DEVICE_ID  0x2415  // AC'97 Audio Controller
#define AC97_PCI_CLASS  0x04    // Multimedia Controller
#define AC97_PCI_SUBCL  0x01    // Multimedia Audio Controller

// save the status for debugging
#define AC97_STATUS_OK              0
#define AC97_STATUS_NOT_PRESENT     1

/**
  * Store everything we know about the ac97 device.
  */
typedef struct ac97_dev {
    PCIDev *pci_dev;
    uint8 status;
} AC97Dev;

/**
  * Detect and configure the ac97 device
  */
void _ac97_init(void);

#endif
