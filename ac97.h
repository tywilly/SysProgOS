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
#define AC97_VENDOR_ID      0x8086  // Intel Corporation
#define AC97_DEVICE_ID      0x2415  // AC'97 Audio Controller
#define AC97_PCI_CLASS      0x04    // Multimedia Controller
#define AC97_PCI_SUBCL      0x01    // Multimedia Audio Controller
#define AC97_MUTE           0x8000  // Set bit 15 to mute
#define AC97_SAMPLE_RATE    8000    // Hz. bad quality, but small file size

#define AC97_STATUS_OK              0
#define AC97_STATUS_NOT_PRESENT     1

// Native Audio Mixer IO Port Offsets
#define AC97_PCM_VRA_EN         (1 << 0)    // Enable PCM Variable Rate Audio
#define AC97_MASTER_VOLUME      0x02        // Master Volume
#define AC97_PCM_OUT_VOLUME     0x18        // PCM Output Volume
#define AC97_EXT_AUDIO_CR       0x2A        // Extended Audio Control/Status Reg
#define AC97_PCM_FR_DAC_RATE    0x2C        // PCM Front DAC Rate

// Register Offsets from Native Audio Bus Mastering Base Address Register
#define AC97_PCM_OUT_BDBAR  0x10    // PCM Output Buffer Descriptor Base Addr
#define AC97_PCM_OUT_CIV    0x14    // Current Index Value Register
#define AC97_PCM_OUT_LVI    0x15    // Last Valid Index
#define AC97_PCM_OUT_SR     0x16    // PCM Output Status Register Offset
#define AC97_PCM_OUT_CR     0x1B    // PCM Output Control Register offset

// PCM Output Status Register Fields
#define AC97_PCM_OUT_SR_DCH     (1 << 0)    // DMA controller halted
#define AC97_PCM_OUT_SR_LVBCI   (1 << 2)    // Last Valid Buffer Complete Int.
#define AC97_PCM_OUT_SR_BCIS    (1 << 3)    // Buffer Complete Interrupt Status
#define AC97_PCM_OUT_SR_FIFOE   (1 << 4)    // FIFO Error Interrupt.

// PCM Output Control Register Fields
#define AC97_PCM_OUT_CR_IOCE    (1 << 4)    // Interrupt on Completion Enable
#define AC97_PCM_OUT_CR_FEIE    (1 << 3)    // FIFO Error Interrupt Enable
#define AC97_PCM_OUT_CR_RPBM    (1 << 0)    // Run/Pause Bus Master

// buffer descriptor list constants
#define AC97_BDL_LEN        32
#define AC97_BUFFER_LEN     4096
#define AC97_SAMPLE_WIDTH   16
#define AC97_BUFFER_SAMPLES AC97_BUFFER_LEN / AC97_SAMPLE_WIDTH
#define AC97_NUM_BUFFERS    AC97_BDL_LEN
#define AC97_BDL_IOC        ((uint32) 1 << 31); // interrupt on completion
#define AC97_BDL_LEN_MASK   0xFFFF

/**
  * Data format for a buffer descriptor. Note: this isn't implemented as a bit
  * field because the endianness of the structure in memory varies from compiler
  * to compiler...trust me, I tried it and it didn't work.
  */
typedef struct buffer_descriptor_s {
    uint32 pointer;     // 32 bit pointer to the data buffer
    uint32 control;     // holds several fields of interest...
                        // Bit 31       IOC - Interrupt on Completion
                        // Bits 15:0    Len - Number of 16-bit samples in buffer
} __attribute__((packed)) AC97BufferDescriptor;

/**
  * Store everything we know about the ac97 device.
  */
typedef struct ac97_dev {
    PCIDev *pci_dev;            // reference to PCI Device for debugging
    AC97BufferDescriptor *bdl;  // ptr to the first buffer descriptor in the BDL
    uint16 nabmbar;             // Native Audio Bus Master Base Address Register
    uint16 nambar;              // Native Audio Mixer Base Address Register
    uint8 status;               // holds whether or not the device is active
    uint8 lvi;                  // last valid index in the BDL
    uint8 vol_bits;             // bits of volume this device supports (5 or 6)
    uint8 head;                 // first buffer in the BDL
    uint8 tail;                 // last buffer in the BDL
    uint8 free_buffers;         // number of unused buffers in the BDL
    bool playing;               // true if the device is playing audio buffers
} AC97Dev;

// symbols from linked-in audio files
extern const char _binary_winstart_wav_start;
extern const char _binary_winstart_wav_end;

/**
  * Detect and configure the ac97 device.
  *
  * Sets the output volume to maximum, sets the PCM output sample rate to 8 kHz,
  * initializes the buffer descriptor list ring buffer, and allocates a page
  * for each buffer. It currently doesn't ever give them back.
  */
void _ac97_init(void);

/**
  * Interrupt handler for the ac97 controller.
  * Handles cycling data through the device's ring buffer.
  */
void _ac97_isr(int vector, int code);

/**
  * Set the Master Volume on a scale from 0 (silent) to 63 (hearing damage).
  * PCM output defaults to full volume, so the system controls loudness using 
  * the master volume. If the audio device only supports 5 bits of volume
  * resolution, the value will be scaled appropriately before being passed on
  * to the device.
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

/**
  * Dump a bunch of diagnostic information to console output to make it easier
  * to figure out how the AC97 device may be misbehaving.
  */
void _ac97_status(void);

/**
  * Write samples to the sound buffer.
  *
  * The audio will play at the device's current set sample rate. Be careful if
  * you change the sample rate while there is still data in the buffer...
  * All audio data must be stereo PCM with 16-bit signed samples with left and 
  * right audio channels interleaved. Data formatted differrently will sound
  * strange.
  * 
  * buffer: A pointer to the first byte of audio data. Note: make sure it's not
  *     a WAV file header--it probably won't sound very good.
  * length: The number of bytes to write.
  * Returns the number of bytes that were read into the buffer. If the AC97
  *     buffer fills before the whole length of the data buffer is written,
  *     this value could be less than the length parameter.
  */
int _ac97_write(const char *buffer, int length);

/**
  * Adjust the PCM output sample rate. Be careful: if there is data in the
  * buffer when you do this, it will probably sound weird.
  *
  * rate The rate to operate at in Hz. If the exact bitrate entered is not 
  *     supported by the device, the nearest supported frequency will be chosen.
  */
void _ac97_set_sample_rate(uint16 rate);

/**
  * Start playback.
  */
void _ac97_play(void);

/**
  * Stop playback. The data buffer maintains its current state.
  */
void _ac97_stop(void);
#endif
