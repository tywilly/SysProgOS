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
#include "kmem.h"
#include "kdefs.h"

// keep track of the device
static PCIDev* soundblaster_dev;
static uint8 bus;
static uint8 slot;
static uint8 func;

// memory access to the page
static uint16* audio_samples;
static uint16* insert_sample_pointer;

// make sure you configure this so there's at max 2^^16 number of samples.
#define SB_SAMPLE_SIZE       16
#define SB_NUM_PAGES_BUFFER  16
#define SB_BYTES_ALLOCATED   SB_NUM_PAGES_BUFFER * PAGE_SIZE

// Detect and configure the Sound blaster audio device.
void _soundblaster_init(void) {
    // print out starting
    __cio_puts(" SoundBl");
    soundblaster_dev = NULL;

    // find the soundblaster device
    soundblaster_dev = _pci_get_device_vendorid_deviceid(
        SOUND_BLASTER_VENDOR_ID, SOUND_BLASTER_DEVICE_ID);

    if (soundblaster_dev == NULL) {
        __cio_puts("!");
        return;
    } else {
        bus = soundblaster_dev->bus;
        slot = soundblaster_dev->slot;
        func = soundblaster_dev->func;
    }

    // setup
    // allocate 16 pages of RAM to be used as the buffer
    audio_samples = ( uint16* ) _kalloc_page(SB_NUM_PAGES_BUFFER);
    assert( audio_samples ); // if this is null, big system problem

    // start out adding samples to the start.
    insert_sample_pointer = audio_samples;

    // Section to get the info from the device
    // align base address register to 4 bytes -- I/O region type
    uint32 base_address = soundblaster_dev->bar0 >> 2;
    // I/O Address space base address (port)
    uint32 base_io = base_address << 2;

    // select memory page corresponding to DAC-1
    __outl( base_io + 0xC, 0b1100 );

    // set the pointer to the frame for the sound
    __outl( base_io + 0x30, (int) audio_samples );

    // set the number of samples that can be played
    uint32 number_samples = (SB_BYTES_ALLOCATED / SB_SAMPLE_SIZE) & 0xFFFF;
    __outl( base_io + 0x24, number_samples );

    // set 16 bit mono sound
    uint32 mono_sound = __inl(base_io + 0x20);
    mono_sound |= 0b10;
    __outl( base_io + 0x20, mono_sound);

    // TODO - this is temporary so that the audio laster more than 1/10th of a
    //   second.
    // enable loop mode (when reaches end of buffer for DAC 1)
    uint32 loop_mode = __inl(base_io + 0x20);
    loop_mode |= 1 << 14;
    __outl( base_io + 0x20, loop_mode );

    // set 44.1 kHz sound & start the playback
    uint32 frequency_set = __inl(base_io + 0x0);
    frequency_set |= 0b11 << 12;
    frequency_set |= 1 << 6;
    __outl( base_io + 0x0, frequency_set);

    // mark as good to screen
    __cio_puts("+");

    // TODO enable interrupts so we can play more sound
    // TODO -- this currently makes no sound since we haven't given it any data
    //   - need _soundblaster_write to be working to be able to test this
}

void _soundblaster_write( uint16 sample ) {
    // TODO make this not just a print method
    __cio_printf( "%x\n", sample );
}
