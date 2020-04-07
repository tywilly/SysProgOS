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

#include "ac97.h"
#include "pci.h"

// the one and only ac97 device we will be keeping track of...at the moment
static AC97Dev dev;
static AC97BufferDescriptor bdl_array[AC97_BDL_LEN];
static AC97Buffer buffers[AC97_NUM_BUFFERS];

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
        dev.nambar = pci_dev->bar0;
        dev.nabmbar = pci_dev->bar1;

        // install ac97 interrupt service routine
        __install_isr(pci_dev->interrupt, _ac97_isr);

        // enable ac97 interrupts
        // Write to PCM Out Control Register with FIFO Error Interrupt Enable,
        // and Interrupt on Completion Enable
        __outb(dev.nabmbar + AC97_PCM_OUT_CR, 
               AC97_PCM_OUT_CR_IOCE | AC97_PCM_OUT_CR_FEIE);

        // enable bus mastering, disable memory mapping
        // allows the controller to initiate DMA transfers
        _pci_write_field(pci_dev, PCI_COMMAND, 0x5);

        // set up buffer desciptor list using a statically allocated hunk
        __memclr(bdl_array, sizeof(AC97BufferDescriptor) * AC97_BDL_LEN);
        dev.bdl = bdl_array;

        // inform the ICH where the BDL lives
        __outl(dev.nabmbar + AC97_PCM_OUT_BDBAR, (uint32) dev.bdl);

        // set the last valid index to 2
        // TODO DCB why??
        dev.lvi = 2;
        __outb(dev.nabmbar + AC97_PCM_OUT_LVI, dev.lvi);

        // determine the number of volume bits this device supports
        // write 6 bits to the volume
        // 0x2020 sets the left and right levels with a 1 in the 6th bit posn.
        __outw(dev.nabmbar + AC97_MASTER_VOLUME, 0x2020);
        uint16 vol = __inw(dev.nabmbar + AC97_MASTER_VOLUME);
        if ((vol & 0x1F) == 0x1F) {
            // we wrote a 1 to the 6th bit, but the lower 5 were set
            // this device only supports 5 bits.
            dev.vol_bits = 5;
        } else {
            dev.vol_bits = 6;
        }


        // prevent deafness
        _ac97_set_volume(32); // set to 50%
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
    // TODO DCB remove
    vector = code;
    code = 7;
    __cio_puts("UH OH...AC97 ISR NOT IMPLEMENTED!\n");
}

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

        vol16 = ~vol16;                     // 0 is max volume
        vol16 &= ((1 << dev.vol_bits) - 1); // ignore upper bits
        vol16 |= (vol16 << 8);              // set left and right channels
        
        __outw(dev.nambar + AC97_MASTER_VOLUME, vol16);
    }
}

// Scale a value to a different number of bits
uint8 _ac97_scale(uint8 value, uint8 max_bits, uint8 target_max_bits) {
    return (value) * ((1 << target_max_bits)) / ((1 << max_bits));
}
