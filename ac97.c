/*
** File:	ac97.c
**
** Author:	Cody Burrows (cxb2114@rit.edu)
**
** Contributor:
**
** Description:	Defines functionality that makes the ac97 hardware work
**              ...in theory...
*/

// include stuff
#define __SP_KERNEL__

#include "common.h"
#include "klib.h"
#include "x86pic.h"

#include "ac97.h"
#include "pci.h"

// the one and only ac97 device we will be keeping track of...at the moment
static AC97Dev dev;
static AC97BufferDescriptor bdl_array[AC97_BDL_LEN];

// Detect and configure the ac97 device.
void _ac97_init(void) {
    // print out that init is starting
    __cio_puts(" AC97");
    dev.status = AC97_STATUS_OK;

    // detect the ac97 controller
    PCIDev *pci_dev = _pci_get_device_id(AC97_VENDOR_ID, AC97_DEVICE_ID, 
                                         AC97_PCI_CLASS, AC97_PCI_SUBCL);

    if (pci_dev != 0) {
        // Keep track of the PCI Device and its relevant BARs
        dev.pci_dev = pci_dev;
        dev.nambar = pci_dev->bar0 & ~((uint32) 1);
        dev.nabmbar = pci_dev->bar1 & ~((uint32) 1);

        _ac97_stop();

        // enable variable rate PCM audio mode
        uint16 ext = __inw(dev.nambar + AC97_EXT_AUDIO_CR);
        __outw(dev.nambar + AC97_EXT_AUDIO_CR, ext | AC97_PCM_VRA_EN);

        // set sample rate to 8khz (low quality, but smallish file size)
        _ac97_set_sample_rate(AC97_SAMPLE_RATE);

        // set the ac97 interrupt service routine function
        // the interrupt comes through the PIC
        __install_isr(pci_dev->interrupt + PIC_EOI, _ac97_isr);

        // enable ac97 interrupts for FIFO Errors and Interrupt On Completion
        __outb(dev.nabmbar + AC97_PCM_OUT_CR, 
               AC97_PCM_OUT_CR_IOCE | AC97_PCM_OUT_CR_FEIE);

        // enable bus mastering, disable memory mapping
        // allows the controller to initiate DMA transfers
        _pci_write_field16(pci_dev, PCI_COMMAND, 0x5);

        // set up buffer desciptor list using a statically allocated memory hunk
        __memclr(bdl_array, sizeof(AC97BufferDescriptor) * AC97_BDL_LEN);
        dev.bdl = bdl_array;
        for (int i = 0; i < AC97_BDL_LEN; ++i) {
            // carve out a brand new page for our buffer
            bdl_array[i].pointer = (uint32) _kalloc_page(1);
            if (bdl_array[i].pointer == 0) {
                WARNING("AC97: Failed to allocate buffer\n");
            }

            // clean it up
            __memclr((void *) bdl_array[i].pointer, AC97_BUFFER_LEN);

            bdl_array[i].control |= AC97_BDL_IOC; // set interrupt on completion
        }

        // inform the ICH where the BDL lives
        __outl(dev.nabmbar + AC97_PCM_OUT_BDBAR, (uint32) dev.bdl);

        // determine the number of volume bits this device supports
        // write 6 bits to the volume
        // 0x2020 sets the left and right levels with a 1 in the 6th bit posn.
        __outw(dev.nambar + AC97_MASTER_VOLUME, 0x2020);
        uint16 vol = __inw(dev.nambar + AC97_MASTER_VOLUME);
        if ((vol & 0x1F) == 0x1F) {
            // we wrote a 1 to the 6th bit, but the lower 5 were set
            // this device only supports 5 bits.
            dev.vol_bits = 5;
        } else {
            dev.vol_bits = 6;
        }

        // set output volume to full, which works best for use in emulators
        // this will be deafening on real hardware
        _ac97_set_volume(63);

        // initialize last valid index, and the head and tail pointers
        dev.lvi = 0;
        __outb(dev.nabmbar + AC97_PCM_OUT_LVI, dev.lvi);
        dev.head = 0;
        dev.tail = 0;
        dev.free_buffers = AC97_NUM_BUFFERS;
    } else {
        // There's no AC97 device installed in the system
        dev.status = AC97_STATUS_NOT_PRESENT;
        __cio_putchar('!');
    }
}

// Ladies and gentlemen, the c97 interrupt service routine. *crowd goes wild*
void _ac97_isr(int vector, int code) {
    // read status to figure out what to do
    uint16 status = __inw(dev.nabmbar + AC97_PCM_OUT_SR);

    if (status == 0) {
        // nothing to do...all is good, probably
        return;
    }

    if (status & AC97_PCM_OUT_SR_BCIS) {
        // a buffer was completed, replace its contents or free it.
        // mark buffers before cur_index as free
        uint8 cur_index = __inb(dev.nabmbar + AC97_PCM_OUT_CIV);
        while (dev.head != cur_index) {
            // TODO DCB deallocate the buffer's page??
            // mark the buffer as free by moving the head, so it can be refilled
            dev.free_buffers++;
            if (dev.head != dev.tail) {
                dev.head = ((dev.head + 1) % AC97_BDL_LEN);
            }
        }
    } 
    
    if (status & AC97_PCM_OUT_SR_LVBCI) {
        // last valid buffer complete interrupt
        __cio_printf("AC97: LVBCI [%02x]\n", status);

        dev.head = dev.tail;
        dev.free_buffers = AC97_BDL_LEN;

        _ac97_stop();
    } 
    
    if (status & AC97_PCM_OUT_SR_FIFOE) {
        // fifo error interrupt
        __cio_printf("AC97: FIFOE [%02x]\n", status);
    }

    // clear status registers so the device will continue blasting tunes
    // status registers are clear-on-write
    __outw(dev.nabmbar + AC97_PCM_OUT_SR, status & 0x1C);

    // acknowledge interrupt, so it can happen again
    if (vector >= 0x20 && vector < 0x30) {
        __outb(PIC_MASTER_CMD_PORT, PIC_EOI);
        if (vector > 0x27) {
            __outb(PIC_SLAVE_CMD_PORT, PIC_EOI);
        }
    }

}

// Set the output volume on a 6 bit scale.
void _ac97_set_volume(uint8 vol) {
    if (vol >= (1 << dev.vol_bits)) {
        __cio_puts("AC97: Volume out of range!\n");
    }

    // set PCM to full volume
    // control loudness with master volume
    __outw(dev.nambar + AC97_PCM_OUT_VOLUME, 0x0);

    if (vol == 0) {
        // mute
        __outw(dev.nambar + AC97_MASTER_VOLUME, AC97_MUTE);
    } else {
        uint16 vol16 = (uint16) _ac97_scale(vol, 6, dev.vol_bits);

        // do some massaging to match the device specification:
        // volume is set by dB attenuation, so 0 dB is the loudest. Higher
        // values increase the quetness. 
        vol16 = ~vol16;                     // 0 is max volume
        vol16 &= ((1 << dev.vol_bits) - 1); // ignore upper bits
        vol16 |= (vol16 << 8);              // set left and right channels
        
        __outw(dev.nambar + AC97_MASTER_VOLUME, vol16);
    }
}

// See what the master volume is set to.
uint8 _ac97_get_volume(void) {
    // only read the lower 6 bits
    uint16 vol = (~__inw(dev.nambar + AC97_MASTER_VOLUME)) & 0x3F;
    return _ac97_scale((uint8) vol, dev.vol_bits, 6);
}

// Scale a value to a different number of bits.
uint8 _ac97_scale(uint8 value, uint8 max_bits, uint8 target_max_bits) {
    return (value) * ((1 << target_max_bits)) / ((1 << max_bits));
}

// Dump relevant status on console output.
void _ac97_status(void) {
    __cio_printf("\n== AC97 Status ==\n");
    __cio_printf(" |  PCM OUT SR[16]:     %08x\n", __inw(dev.nabmbar + 0x16));
    __cio_printf(" |  PCM OUT CR[8]:      %08x\n", __inb(dev.nabmbar + 0x1B));
    __cio_printf(" |  PCM OUT CIV[5]:     %08x\n", __inb(dev.nabmbar + 0x14));
    __cio_printf(" |  PCM OUT LVI[5]:     %08x\n", __inb(dev.nabmbar + 0x15));
    __cio_printf(" |  PCM OUT PIV[5]:     %08x\n", __inb(dev.nabmbar + 0x1A));
    __cio_printf(" |  PCM OUT PICB[16]:   %08x\n", __inw(dev.nabmbar + 0x18));
    __cio_printf(" |  GLOB CONTORL[32]:   %08x\n", __inl(dev.nabmbar + 0x2C));
    __cio_printf(" |  GLOB STATUS[32]:    %08x\n", __inl(dev.nabmbar + 0x30));
    __cio_printf(" |  PCI Status[16]:     %08x\n", _pci_config_read16(dev.pci_dev, 0x6));
    __cio_printf(" |  Free Buffers:          %02d/32\n", dev.free_buffers);
    __cio_printf(" |  Head Index:               %02d\n", dev.head);
    __cio_printf(" |  Tail Index:               %02d\n", dev.tail);
    __cio_printf("== AC97 Status ==\n");
}

// Fill the AC97 Buffer.
int _ac97_write(const char *buffer, int length) {
    if (dev.status != AC97_STATUS_OK) {
        return E_BAD_CHANNEL;
    }

    uint32 *source = (uint32 *) buffer;
    int bytes_left = length;
    // fill unused buffers at the tail of the list
    while (dev.free_buffers > 0 && bytes_left >= 4) {
        if (!(dev.head == dev.tail && dev.free_buffers == AC97_BDL_LEN)) {
            // advance the tail only if this isn't the first buffer.
            // initially, head and tail point to the same index, even if
            // there are no elements in the buffer.
            dev.tail = ((dev.tail + 1) % AC97_BDL_LEN);
        }
        dev.free_buffers--;

        // pump the buffer full of music, two samples at a time.
        AC97BufferDescriptor *desc = &bdl_array[dev.tail];
        uint32 *dest = (uint32 *) desc->pointer;
        uint16 samples_left_in_buf = AC97_BUFFER_SAMPLES;
        while (samples_left_in_buf >= 2 && bytes_left >= 4) {
            // note: __memcpy was a little too barbarric for this task,
            // due to potential alignment issues.
            *dest = *source;
            dest++;
            source++;
            bytes_left -= 4;
            samples_left_in_buf -= 2;
        }
        desc->control |= AC97_BDL_IOC; // set interrupt on completion

        // set length in number of 16 bit samples
        desc->control &= ~((uint32) AC97_BDL_LEN_MASK);
        desc->control |= ((AC97_BUFFER_SAMPLES - samples_left_in_buf) & 
                          AC97_BDL_LEN_MASK);

    }

    // tell the device the index of last buffer full of samples
    dev.lvi = dev.tail;
    __outb(dev.nabmbar + AC97_PCM_OUT_LVI, dev.lvi);

    // set things to play
    _ac97_play();

    return length - bytes_left;
}

// Set the PCM output sample rate
uint16 _ac97_set_sample_rate(uint16 rate) {
    if (rate != 0) {
        if (dev.playing) {
            _ac97_stop();
        }

        __outw(dev.nambar + AC97_PCM_FR_DAC_RATE, rate);
    }

    return __inw(dev.nambar + AC97_PCM_FR_DAC_RATE);
}

// Start things playing
void _ac97_play(void) {
    if (!dev.playing) {
        dev.playing = true;
        __outb(dev.nabmbar + AC97_PCM_OUT_CR, 
               __inb(dev.nabmbar + AC97_PCM_OUT_CR) | AC97_PCM_OUT_CR_RPBM);
    }
}

// Pause audio output
void _ac97_stop() {
    dev.playing = false;
    __outb(dev.nabmbar + AC97_PCM_OUT_CR, __inb(dev.nabmbar + AC97_PCM_OUT_CR) &
                                          ~((uint8) AC97_PCM_OUT_CR_RPBM));
}
