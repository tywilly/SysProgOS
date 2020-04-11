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

/*
** _rdisk_init()
**
** initialize the ramdisk module
*/
void _rdisk_init(void);

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
int _rdisk_create(int disknum, unsigned int size);

/*
** Create a new ramdisk of the desired size.
**
** @param disknum	The number of the ramdisk to destroy.
**
** @return The number of the device destroyed, or -1 if there was an error.
*/
int _rdisk_destroy(int disknum);

#endif

