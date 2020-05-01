/*
**
** File:    soundblaster.h
**
** Author:  Zach Jones   (ztj3686@rit.edu)
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

/** Writes samples of data to the SoundBlaster device from the buffer
**  Buff is a pointer to 16-bit mono samples.
**  Count is the number of samples to write. (half the number of bytes.)
**  Returns the number of samples written.
*/
int _soundblaster_write( const char* buff, int count );

/** Handles the interrupts for the soundblaster device.
*/
void _soundblaster_isr( int vector, int code );

#endif
