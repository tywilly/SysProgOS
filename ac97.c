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
extern const uint32 _binary_winstart_wav_start;
extern const uint32 _binary_winstart_wav_end;
void *pos;

// Detect and configure the ac97 device.
void _ac97_init(void) {
    pos = (void *) _binary_winstart_wav_start;
    // print out that init is starting
    __cio_puts(" AC97");
    dev.status = AC97_STATUS_OK;

    // detect the ac97 controller
    PCIDev *pci_dev = _pci_get_device_id(AC97_VENDOR_ID, AC97_DEVICE_ID, 
                                         AC97_PCI_CLASS, AC97_PCI_SUBCL);

    if (pci_dev != 0) {
        // keep track of the pci device's base address registers
        dev.nambar = pci_dev->bar0 & ~((uint32) 1);
        dev.nabmbar = pci_dev->bar1 & ~((uint32) 1);

        // install ac97 interrupt service routine
        __cio_printf("\nPCI Reported Interrupt: %02x\n", pci_dev->interrupt);
        __cio_printf("Actual Interrupt PIN:   %02x\n", _pci_config_read8(pci_dev, 0x3D));
        __cio_printf("Actual Interrupt LINE:  %02x\n", _pci_config_read8(pci_dev, 0x3C));
        __install_isr(43, _ac97_isr); // TODO DCB why is this 43?

        // enable ac97 interrupts
        // Write to PCM Out Control Register with FIFO Error Interrupt Enable,
        // and Interrupt on Completion Enable
        __outb(dev.nabmbar + AC97_PCM_OUT_CR, 
               AC97_PCM_OUT_CR_IOCE | AC97_PCM_OUT_CR_FEIE);

        // enable bus mastering, disable memory mapping
        // allows the controller to initiate DMA transfers
        _pci_write_field16(pci_dev, PCI_COMMAND, 0x5);

        // set up buffer desciptor list using a statically allocated hunk
        __memclr(bdl_array, sizeof(AC97BufferDescriptor) * AC97_BDL_LEN);
        dev.bdl = bdl_array;
        for (int i = 0; i < AC97_BDL_LEN; ++i) {
            bdl_array[i].pointer = (uint32) _kalloc_page(1);
            __memclr((void *) bdl_array[i].pointer, AC97_BUFFER_LEN);
            bdl_array[i].control |= AC97_BDL_IOC; // set interrupt on completion

            // set length in number of 16 bit samples
            bdl_array[i].control &= ~((uint32) AC97_BDL_LEN_MASK);
            bdl_array[i].control |= (AC97_BUFFER_SAMPLES & AC97_BDL_LEN_MASK);
        }

        // inform the ICH where the BDL lives
        __outl(dev.nabmbar + AC97_PCM_OUT_BDBAR, (uint32) dev.bdl);

        // set the last valid index to 2
        // TODO DCB why??
        dev.lvi = 0;
        __outb(dev.nabmbar + AC97_PCM_OUT_LVI, dev.lvi);

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

        // prevent deafness
        // TODO DCB cause deafness?? (change to about 25% (16) for read hw)
        _ac97_set_volume(63);

        // index into the BDL
        dev.head = 0;
        dev.tail = 0;
        dev.free_buffers = AC97_NUM_BUFFERS;

        // set things to play
        __outb(dev.nabmbar + AC97_PCM_OUT_CR, 
               __inb(dev.nabmbar + AC97_PCM_OUT_CR) | AC97_PCM_OUT_CR_RPBM);
    } else {
        dev.status = AC97_STATUS_NOT_PRESENT;
    }

    if (dev.status != AC97_STATUS_OK) {
        // indicate that init didn't work
        __cio_putchar('!');
    }
}

// ac97 interrupt service routine
void _ac97_isr(int vector, int code) {
    __cio_printf("\n\nAC97 ISR!!\n\n");
    uint16 status = __inw(dev.nabmbar + AC97_PCM_OUT_SR);

    if (status == 0) {
        return;
    }

    if (status & AC97_PCM_OUT_SR_BCIS) {
        __cio_printf("AC97: BCIS [%02x]\n", status);

        // update the descriptor list
        uint8 cur_index = __inb(dev.nabmbar + AC97_PCM_OUT_CIV) & 0x1F;
        __cio_printf("CURRENT INDEX: %x\n", cur_index);
        while (dev.head != cur_index) {
            // TODO DCB free head???
            //__cio_printf("FREED A BUFFER [%d]\n", dev.free_buffers);
            dev.free_buffers++;
            if (dev.head == AC97_BDL_LEN - 1) {
                // end of the list
                dev.head = 0;
            } else {
                // still somewhere in the list
                dev.head++;
            }
        }

        // fill new buffers at the tail of the list
        while (dev.free_buffers > 0) {
            dev.free_buffers--;
            //__cio_printf("GRABBING A NEW BUFFER [%d]\n", dev.free_buffers);
            dev.tail = ((dev.tail + 1) % AC97_NUM_BUFFERS);
            //__cio_printf("MOVING TAIL to %d\n", dev.tail);

            // init the new buffer
            // everything should still be all set from last time it was used
            // copy data into buffer
            AC97BufferDescriptor desc = bdl_array[dev.tail];
            __memcpy((void *) desc.pointer, pos, AC97_BUFFER_LEN);
            pos = (void *) ((uint32) pos + AC97_BUFFER_LEN);
            __cio_printf("CONTROL: %08x\n", desc.control);
            desc.control |= AC97_BDL_IOC; // set interrupt on completion

            dev.lvi = dev.tail;
            __outb(dev.nabmbar + AC97_PCM_OUT_LVI, dev.lvi);
            //__cio_printf("SETTING LVI TO %d\n", dev.lvi);
        }
    } else if (status & AC97_PCM_OUT_SR_LVBCI) {
        // last valid buffer complete interrupt
        __cio_printf("AC97: LVBCI [%02x]\n", status);
    } else if (status & AC97_PCM_OUT_SR_FIFOE) {
        // fifo error interrupt
        __cio_printf("AC97: FIFOE [%02x]\n", status);
    } else {
        __cio_printf("AC97: ignoring interrupt [%02x]\n", status);
    }

    // clear DMA halt
    __outw(dev.nabmbar + AC97_PCM_OUT_SR, status & ~(AC97_PCM_OUT_SR_DCH));

    // acknowledge interrupt
    __outb(PIC_MASTER_CMD_PORT, PIC_EOI);
}

void _ac97_set_volume(uint8 vol) {
    if (vol >= (1 << dev.vol_bits)) {
        __cio_puts("AC97: Volume out of range!\n");
    }

    // set PCM to full volume
    // control loudness with master volume
    __outw(dev.nambar + AC97_PCM_OUT_VOLUME, 0x0);
    __cio_printf("PCM VOL: %04x\n", __inw(dev.nambar + AC97_PCM_OUT_VOLUME));

    if (vol == 0) {
        // mute
        __outw(dev.nambar + AC97_MASTER_VOLUME, AC97_MUTE);
    } else {
        uint16 vol16 = (uint16) _ac97_scale(vol, 6, dev.vol_bits);

        vol16 = ~vol16;                     // 0 is max volume
        vol16 &= ((1 << dev.vol_bits) - 1); // ignore upper bits
        vol16 |= (vol16 << 8);              // set left and right channels
        
        __outw(dev.nambar + AC97_MASTER_VOLUME, vol16);
    }

    __cio_printf("MASTER: %04x\n", __inw(dev.nambar + AC97_MASTER_VOLUME));
}

// see what the master volume is set to
uint8 _ac97_get_volume(void) {
    // only read the lower 6 bits
    uint16 vol = __inw(dev.nambar + AC97_MASTER_VOLUME) & 0x3F;
    return _ac97_scale((uint8) vol, dev.vol_bits, 6);
}

// Scale a value to a different number of bits
uint8 _ac97_scale(uint8 value, uint8 max_bits, uint8 target_max_bits) {
    return (value) * ((1 << target_max_bits)) / ((1 << max_bits));
}

uint16 _ac97_status(void) {
    return __inw(dev.nabmbar + AC97_PCM_OUT_SR);
}
