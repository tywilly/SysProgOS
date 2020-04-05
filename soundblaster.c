/*
**
** File:    soundblaster.c
**
** Author:  Zach Jones
**
** Contributor:
**
** Description: Soundblaster module, this one works with the soundblaster
**              hardware.
**
*/

#include "common.h"
#include "klib.h"
#include "cio.h"

#include "soundblaster.h"
#include "pci.h"

// keep track of the device
static PCIDev* soundblaster_dev;

// Detect and configure the Sound blaster audio device.
void _soundblaster_init(void) {
    // print out starting
    __cio_puts(" SoundBl");
    soundblaster_dev = NULL;

    // find the soundblaster device
    soundblaster_dev = _pci_get_device_vendorid_deviceid(
        SOUND_BLASTER_VENDOR_ID, SOUND_BLASTER_DEVICE_ID);

    // this might be silly but I like it
    if (soundblaster_dev == NULL) {
        __cio_puts(":(");
    } else {
        __cio_puts(":)");
    }

    // TODO set to 44.1 Khz

    //
    // TODO create the buffer to put data into and enable interrupts,
    //  create channels, ...

}

void _soundblaster_write( uint16 sample ) {
    // TODO make this not just a print method
    __cio_printf( "%x\n", sample );
}
