#include <types.h>
#include <cio.h>
#include <kmem.h>
#include <klib.h>
#include <ramdisk.h>

static RamDisk disks[MAX_RAMDISKS];
static RamDisk empty_disk;

/*
** _ramdisk_init()
**
** initialize the ramdisk module
*/
void _ramdisk_init(void) {
	int i;

	// Initialize all the ramdisks to be empty.
	empty_disk.used = false;
	empty_disk.start = NULL;
	empty_disk.size = 0;
	empty_disk.seek = 0;
	for (i = 0; i < MAX_RAMDISKS; i++) {
		__memcpy(&disks[i], &empty_disk, sizeof(RamDisk));
	}

	__cio_puts( " RAMDISK" );
}

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
int _ramdisk_create(int disknum, unsigned int size) {
	uint32 num_slabs;

	if ( disknum < -1 || disknum >= MAX_RAMDISKS \
			|| disks[disknum].used == true)
		return -1;

	// Dynamic device selection
	if (disknum == -1) {
		int i;
		// Try to find an available disk
		for (i = 0; i < MAX_RAMDISKS; i++) {
			if (disks[i].used == false) {
				disknum = i;
				break;
			}
		}
		// If none was found, error
		if (disknum == -1)
			return -1;
	}
	// Fixed device selection
	else {
		if (disks[disknum].used == true)
			return -1;
	}

	// Calculate the number of slabs
	num_slabs = size / SLAB_SIZE;
	if (size % SLAB_SIZE != 0)
		num_slabs++;

	// Allocate the memory
	disks[disknum].start = (uint8*) _kalloc_page(num_slabs);
	if (disks[disknum].start == NULL)
		return -1;
	disks[disknum].size = size;
	disks[disknum].used = true;

	// Zero the memory
	__memset(disks[disknum].start, num_slabs * SLAB_SIZE, 0);

	return disknum;
}

/*
** Create a new ramdisk of the desired size.
**
** @param disknum	The number of the ramdisk to destroy.
**
** @return The number of the device destroyed, or -1 if there was an error.
*/
int _ramdisk_destroy(int disknum) {
	if ( disknum < 0 || disknum >= MAX_RAMDISKS \
			|| disks[disknum].used == false )
		return -1;
	if ( disks[disknum].used == false )
		return -1;
	_kfree_page(disks[disknum].start);
	disks[disknum].start = 0;
	disks[disknum].size = 0;
	disks[disknum].used = false;
	return disknum;
}

/*
** Reads from a Ramdisk
**
** @param chan  The number of the disk to write to.
** @param buf   The buffer to read data in to.
** @param len   The number of bytes to read.
**
** @return The number of bytes read.
*/
int _ramdisk_read(int chan, void* buf, uint32 len) {
	if ( chan < 0 || chan >= MAX_RAMDISKS || disks[chan].used == false )
		return E_BAD_CHANNEL;

	if ( len + disks[chan].seek >= disks[chan].size )
		len = disks[chan].size - disks[chan].seek;

	__memcpy( buf, disks[chan].start + disks[chan].seek, len );
	disks[chan].seek += len;
	return len;
}

/*
** Writes to a Ramdisk
**
** @param chan  The number of the disk to write to.
** @param buf   The buffer containing the data to write.
** @param len   The length of the data to write.
**
** @return The number of bytes written.
*/
int _ramdisk_write(int chan, const void* buf, uint32 len) {
	if ( chan < 0 || chan >= MAX_RAMDISKS || disks[chan].used == false )
		return E_BAD_CHANNEL;

	if ( len + disks[chan].seek >= disks[chan].size )
		len = disks[chan].size - disks[chan].seek;

	__memcpy( disks[chan].start + disks[chan].seek, buf, len );
	disks[chan].seek += len;
	return len;
}

/*
** Moves the seek offset for a Ramdisk
**
** @param chan  	The number of the disk to move the offset for.
** @param offset	The offset from whence to position the seek location.
** @param whence	The whence to seek relative to (see types.h).
**
** @return The resulting offset location as measured  in bytes from the
** beginning of the file. On error, a negative value is returned, indicating
** the error type.
*/
int _ramdisk_seek(int chan, int offset, int whence) {
	if ( chan < 0 || chan >= MAX_RAMDISKS || disks[chan].used == false )
		return E_BAD_CHANNEL;

	switch (whence) {
		case SEEK_SET:
			disks[chan].seek = offset;
			break;
		case SEEK_CUR:
			disks[chan].seek += offset;
			break;
		case SEEK_END:
			disks[chan].seek = disks[chan].size + offset;
			break;
		default:
			return E_INVALID;
	}

	return disks[chan].seek;
}

/*
** Gets a copy of the Ramdisk struct for a given ramdisk
**
** @param disknum	The number of the ramdisk to get the struct for.
**
** @return The struct for the ramdisk, or an empty struct if the ramdisk does
** not exist.
*/
RamDisk _ramdisk_get(int disknum) {
	if (disknum >= 0 && disknum < MAX_RAMDISKS) {
		return disks[disknum];
	}
	else {
		return(empty_disk);
	}
}

