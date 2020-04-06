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

#define AC97_MUTE       0x8000  // Set bit 15 to mute

// Register Offsets from Native Audio Mixer Base Address Register
// TODO DCB none yet...

// Register Offsets from Native Audio Bus Mastering Base Address Register
#define AC97_PCM_OUT_CR     0x1B    // PCM Output Control Register offset

// Mixer IO Port Offsets
#define AC97_PCM_OUT_VOLUME 0x18    // PCM Output Volume
#define AC97_MASTER_VOLUME  0x02    // Master Volume

// PCM Output Control Register Fields
#define AC97_PCM_OUT_CR_IOCE    (1 << 4)    // Interrupt on Completion Enable
#define AC97_PCM_OUT_CR_FEIE    (1 << 3)    // FIFO Error Interrupt Enable

// save the status for debugging
#define AC97_STATUS_OK              0
#define AC97_STATUS_NOT_PRESENT     1

/**
  * Store everything we know about the ac97 device.
  */
typedef struct ac97_dev {
    uint8 status;
    uint32 nabmbar; // TODO DCB can these be 16 bits?
    uint32 nambar;
} AC97Dev;

/**
  * Detect and configure the ac97 device.
  */
void _ac97_init(void);

/**
  * Interrupt handler for the ac97 controller.
  */
void _ac97_isr(int vector, int code);

/**
  * Set the PCM Output Volume to a 5 bit value. 
  */
void _ac97_set_volume(uint8 vol);

#endif
