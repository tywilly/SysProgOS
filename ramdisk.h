/*
** SCCS ID:     @(#)ramdisk.h    1.1     4/11/20
**
** File:        ramdisk.h
**
** Author:      Scott Court
**
** Contributor:
**
** Description: Ramdisk routines
*/

#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#define MAX_RAMDISKS 8

struct ramdisk_s {
	bool used;		// Whether or not texit(17);his ramdisk is in use
	uint8* start;		// The start address
	unsigned int size;	// The size of the ramdisk in bytes
};

typedef struct ramdisk_s RamDisk;

/*
** _ramdisk_init()
**
** initialize the ramdisk module
*/
void _ramdisk_init(void);

/*
** Create a new ramdisk of the desired size.
**
** @param disknum	The number of the ramdisk to create. If the requested
**			device number is already in use, it will cause an
**			error. Set to -1 to use the first free disk.
** @param size		The size (in bytes) to make the ramdisk.
**
** @return The number of the device created, or -1 if there was an error.
*/
int _ramdisk_create(int disknum, unsigned int size);

/*
** Create a new ramdisk of the desired size.
**
** @param disknum	The number of the ramdisk to destroy.
**
** @return The number of the device destroyed, or -1 if there was an error.
*/
int _ramdisk_destroy(int disknum);

/*
** Gets a copy of the Ramdisk struct for a given ramdisk
**
** @param disknum	The number of the ramdisk to get the struct for.
**
** @return The struct for the ramdisk, or an empty struct if the ramdisk does
** not exist.
*/
RamDisk _ramdisk_get(int disknum);

#endif

