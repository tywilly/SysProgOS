/*
** File:	ac97.h
**
** Author:	Cody Burrows (cxb2114@rit.edu)
**
** Contributor:
**
** Description:	Contains constants and declarations for the baseline ac97 
** driver.
*/

#ifndef _AC97_H_
#define _AC97_H_

#include "common.h"

// The place to hold everything we know about the ac97 device
typedef struct ac97_dev {
    uint32 pci_id;
} AC97Dev;

#endif
