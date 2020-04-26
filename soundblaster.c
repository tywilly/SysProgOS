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
#include "sio.h"
#include "x86pic.h"

#include "soundblaster.h"
#include "pci.h"
#include "kmem.h"
#include "kdefs.h"

// private functions
static void update_pointers( void );

// keep track of the device
static PCIDev* soundblaster_dev;
static uint8 bus;
static uint8 slot;
static uint8 func;

// memory access to the page
static uint16* audio_samples;
static uint16* insert_sample_pointer;
static uint32  base_io;

// make sure you configure this so there's at max 2^^16 number of samples.
#define SB_SAMPLE_SIZE       16
#define SB_NUM_PAGES_BUFFER  64
#define SB_BYTES_ALLOCATED   SB_NUM_PAGES_BUFFER * PAGE_SIZE

// temporary to debug
static int on;

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
    // allocate somes pages of RAM to be used as the buffer
    audio_samples = ( uint16* ) _kalloc_page(SB_NUM_PAGES_BUFFER);
    assert( audio_samples ); // if this is null, big system problem

    // install ISR
    __install_isr( soundblaster_dev->interrupt + PIC_EOI, _soundblaster_isr );

    // start out adding samples to the start.
    insert_sample_pointer = audio_samples;

    // Section to get the info from the device
    // align base address register to 4 bytes -- I/O region type
    uint32 base_address = soundblaster_dev->bar0 >> 2;
    // I/O Address space base address (port)
    base_io = base_address << 2;

    // enable bus master flag
    // enable bus mastering, disable memory mapping
    // allows the controller to initiate DMA transfers
    _pci_write_field16(soundblaster_dev, PCI_COMMAND, 0x7);

    // reset controller
    __outl( base_io + 0x4, 0x20 );

    // set the master volume and PCM out volume levels
    // codec register is 0x10 or 0x14, not sure, think is 10 for 1370
    // set the master volume and the PCM out volume levels, in the codec
    //  space. So have to write to the codec register those values

    // set the volume
    //uint32 volume = __inl(base_io + 0x10)
    uint16 master_volume_set = 0x0000;
    __outw( base_io + 0x10, master_volume_set );
    master_volume_set = 0x0100;
    __outw( base_io + 0x10, master_volume_set );
    uint16 dac_volume_set = 0x0200;
    __outw( base_io + 0x10, dac_volume_set );
    master_volume_set = 0x0300;
    __outw( base_io + 0x10, master_volume_set );


    // select memory page for playback
    __outl( base_io + 0xC, 0b1100 );

    // update the pointer in the sound card to memory
    update_pointers();

    // enable 16-bit mono, interruts, and looped mode
    __outl( base_io + 0x20, 0x00200208 );

    // mark as good to screen
    __cio_puts("+");

    // TODO enable interrupts so we can play more sound
    // TODO -- this currently makes no sound since we haven't given it any data
    //   - need _soundblaster_write to be working to be able to test this

    on = 0;
}

static void update_pointers( void ) {
    // set the pointer to the frame for the sound
    __outl( base_io + 0x38, (uint32) audio_samples );

    // set the number of samples that can be played
    uint32 number_samples = (SB_BYTES_ALLOCATED / SB_SAMPLE_SIZE) & 0xFFFF;
    __outl( base_io + 0x3C, number_samples / 2 );

    // set the number of frames to play before issuing interrupt
    __outl( base_io + 0x28, number_samples );
}

// handle interrupts from this device.
void _soundblaster_isr( int vector, int code ) {

    _sio_puts("&");

    // TODO do a queueing thing

    // for now, resetting to the start of the samples.
    update_pointers();

    // clear the interrupt status for DAC-2 and then re-enable
    //  otherwise the device will not think it is handled
    uint32 serial_interface = __inl( base_io + 0x20 );
    // clear the interrupt
    serial_interface &= 0xFFFFFDFF;
    __outl( base_io + 0x20, serial_interface );

    // re-enable the interrupt so that it happens later again
    serial_interface |= 0x200;
    __outl( base_io + 0x20, serial_interface );


    // acknowledge interrupt, so it can happen again
    if (vector >= 0x20 && vector < 0x30) {
        __outb(PIC_MASTER_CMD_PORT, PIC_EOI);
        if (vector > 0x27) {
            __outb(PIC_SLAVE_CMD_PORT, PIC_EOI);
        }
    }
}


int _soundblaster_write( const char* buff, int count ) {

    uint16* max = SB_BYTES_ALLOCATED/16 + audio_samples;

    if (insert_sample_pointer >= max) {
        //__cio_puts( "+" );

        if (on == 0) {
            on = 1;

            // turn on the sound
            uint32 frequency_set = __inl(base_io + 0x0);
            frequency_set |= 1 << 5;
            __outl( base_io + 0x0, 0x20);

            __cio_puts( "Turned on.\n" );
            __cio_printf("Test: %x ", _pci_config_read16( soundblaster_dev, 0x6 ));
        }

        return 0;

        // TODO this is where we would put the process on the waiting queue.
    }

    int count_moved = 0;
    for (int i = 0; i < count && insert_sample_pointer < max; i++) {
        uint16 sample = ( (uint16*) buff)[i];
        // write and increment
        *insert_sample_pointer = sample;
        insert_sample_pointer++;

        count_moved++;

    }

    return count_moved;
}
