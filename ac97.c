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

        // prevent deafness
        _ac97_set_volume(25);
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
    
    if (vol > (1 << 5)) {
        __cio_puts("AC97: Volume out of range!\n");
    }

    // set master to full
    // use PCM OUT volume to control lousness
    __outw(dev.nambar + AC97_MASTER_VOLUME, 0x0);

    if (vol == 0) {
        // mute
        __outw(dev.nambar + AC97_PCM_OUT_VOLUME, AC97_MUTE);
    } else {
        vol = ~vol;         // 0 is max volume
        vol &= 0x1F;        // ignore upper bits
        vol &= (vol << 8);  // set left and right channels to the same volume

        // TODO DCB could some devices support 6 bits?
        
        __outw(dev.nambar + AC97_PCM_OUT_VOLUME, vol);
    }
}
