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

#define AC97_MUTE           0x8000  // Set bit 15 to mute
#define AC97_SAMPLE_RATE_8K 8000    // bad quality, but small file size

// Register Offsets from Native Audio Mixer Base Address Register
#define AC97_EXT_AUDIO_CR       0x2A // Extended Audio Control/Status Register
#define AC97_PCM_FR_DAC_RATE    0x2C // PCM Front DAC Rate

#define AC97_PCM_VRA_EN         (1 << 0)    // Enable PCM Variable Rate Audio

// Mixer IO Port Offsets
#define AC97_PCM_OUT_VOLUME 0x18    // PCM Output Volume
#define AC97_MASTER_VOLUME  0x02    // Master Volume

// Register Offsets from Native Audio Bus Mastering Base Address Register
#define AC97_PCM_OUT_CR     0x1B    // PCM Output Control Register offset
#define AC97_PCM_OUT_SR     0x16    // PCM Output Status Register Offset
#define AC97_PCM_OUT_LVI    0x15    // Last Valid Index
#define AC97_PCM_OUT_CIV    0x14    // Current Index Value Register
#define AC97_PCM_OUT_BDBAR  0x10    // PCM Output Buffer Descriptor Base Addr

// PCM Output Status Register Fields
#define AC97_PCM_OUT_SR_DCH     (1 << 0)    // DMA controller halted
#define AC97_PCM_OUT_SR_LVBCI   (1 << 2)    // Last Valid Buffer Complete Int.
#define AC97_PCM_OUT_SR_BCIS    (1 << 3)    // Buffer Complete Interrupt Status
#define AC97_PCM_OUT_SR_FIFOE   (1 << 4)    // FIFO Error Interrupt.

// PCM Output Control Register Fields
#define AC97_PCM_OUT_CR_IOCE    (1 << 4)    // Interrupt on Completion Enable
#define AC97_PCM_OUT_CR_FEIE    (1 << 3)    // FIFO Error Interrupt Enable
#define AC97_PCM_OUT_CR_RPBM    (1 << 0)    // Run/Pause Bus Master

// save the status for debugging
#define AC97_STATUS_OK              0
#define AC97_STATUS_NOT_PRESENT     1

// buffer descriptor list things
#define AC97_BDL_LEN        32
#define AC97_BUFFER_LEN     4096
#define AC97_SAMPLE_WIDTH   16
#define AC97_BUFFER_SAMPLES AC97_BUFFER_LEN / AC97_SAMPLE_WIDTH
#define AC97_NUM_BUFFERS    32
#define AC97_BDL_IOC        ((uint32) 1 << 31); // interrupt on completion
#define AC97_BDL_BUP        ((uint32) 1 << 30);
#define AC97_BDL_LEN_MASK   0xFFFF

typedef struct buffer_descriptor_s {
    uint32 pointer;
    uint32 control;
} __attribute__((packed)) AC97BufferDescriptor;

/**
  * Store everything we know about the ac97 device.
  */
typedef struct ac97_dev {
    PCIDev *pci_dev;
    uint8 status;
    uint16 nabmbar;
    uint16 nambar;
    AC97BufferDescriptor *bdl;
    uint8 lvi;  // last valid index
    uint8 vol_bits; // bits of volume this device supports (5 or 6)
    uint8 head;
    uint8 tail;
    uint8 free_buffers;
    uint16 splrate;
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
  * Set the PCM Output Volume using a scale from 0 to 63 (6 bits). If the audio
  * device only supports 5 bits of volume resolution, the value will be scaled
  * before being passed on to the device.
  */
void _ac97_set_volume(uint8 vol);

/**
  * Convert a value on a scale of 0 to 2^max_bits to a scale of 0 to
  * 2^target_max_bits. This is helpful for volume scaling.
  */
uint8 _ac97_scale(uint8 value, uint8 max_bits, uint8 target_max_bits);

/**
  * Read the master volume level of the ac97 controller. It will be
  * returned as a 6-bit value.
  */
uint8 _ac97_get_volume(void);

void _ac97_status(void);

void *_wav_start(const char *start_sym);
#endif
