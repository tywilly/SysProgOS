/*
**
** File:    soundblaster.h
**
** Author:  Zach Jones
**
** Contributor:
**
** Description: Soundblaster module, this one works with the soundblaster
**              hardware.
**
*/

#ifndef _SOUND_BLASTER_H_
#define _SOUND_BLASTER_H_

#include "common.h"

#define SOUND_BLASTER_VENDOR_ID 0x1274
#define SOUND_BLASTER_DEVICE_ID 0x5000


/** Detects and configures the soundblaster device. */
void _soundblaster_init(void);

/** Writes a single sample of data to the SoundBlaster device. */
void _soundblaster_write( uint16 sample );

#endif
