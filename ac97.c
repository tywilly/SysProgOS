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
        // keep track of the pci device's base address registers
        dev.nambar = pci_dev->bar0 & ~((uint32) 1);
        dev.nabmbar = pci_dev->bar1 & ~((uint32) 1);

        // install ac97 interrupt service routine
        __install_isr(pci_dev->interrupt, _ac97_isr);

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
            bdl_array[i].pointer = _kalloc_page(1);
            __memclr(bdl_array[i].pointer, AC97_BUFFER_LEN * AC97_SAMPLE_WIDTH);
            bdl_array[i].ioc = 1;   // interrupt on completion
            bdl_array[i].length = AC97_BUFFER_LEN;
        }

        // inform the ICH where the BDL lives
        __outl(dev.nabmbar + AC97_PCM_OUT_BDBAR, (uint32) dev.bdl);

        // set the last valid index to 2
        // TODO DCB why??
        dev.lvi = 2;
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
        _ac97_set_volume(16); // set to ~25%

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
    uint16 status = __inw(dev.nabmbar + AC97_PCM_OUT_SR);

    if (status == 0) {
        return;
    }

    if (status & AC97_PCM_OUT_SR_LVBCI) {
        // last valid buffer complete interrupt
        __cio_printf("AC97: LVBCI [%4x]\n", status);
    } else if (status & AC97_PCM_OUT_SR_FIFOE) {
        // fifo error interrupt
        __cio_printf("AC97: FIFOE [%4x]\n", status);
    } else if (status & AC97_PCM_OUT_SR_BCIS) {
        // TODO DCB put more data into buffers
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
