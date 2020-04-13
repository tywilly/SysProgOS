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
	uint32 seek;		// The current seek offset in bytes
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
** Reads from a Ramdisk
**
** @param chan  The number of the disk to write to.
** @param buf   The buffer to read data in to.
** @param len   The number of bytes to read.
**
** @return The number of bytes read.
*/
int _ramdisk_read(int chan, void* buf, uint32 len);

/*
** Writes to a Ramdisk
**
** @param chan  The number of the disk to write to.
** @param buf   The buffer containing the data to write.
** @param len   The length of the data to write.
**
** @return The number of bytes written.
*/
int _ramdisk_write(int chan, const void* buf, uint32 len);

/*
** Moves the seek offset for a Ramdisk
**
** @param chan  	The number of the disk to move the offset for.
** @param offset	The offset from whence to position the seek location.
** @param whence	The whence to seek relative to (see types.h).
**
** @return The resulting offset location as measured  in bytes from the
** beginning of the file. On error, the value (off_t) -1 is returned.
*/
int _ramdisk_seek(int chan, int offset, int whence);

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

