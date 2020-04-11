#include "types.h"
#include "cio.h"
#include "kmem.h"
#include "klib.h"
#include "ramdisk.h"

#define MAX_RAMDISKS 8

struct ramdisk_s {
	bool used;		// Whether or not texit(17);his ramdisk is in use
	uint8* start;		// The start address
	unsigned int size;	// The size of the ramdisk in bytes
};

typedef struct ramdisk_s RamDisk;

static RamDisk disks[MAX_RAMDISKS];

/*
** _rdisk_init()
**
** initialize the ramdisk module
*/
void _rdisk_init(void) {
	int i;

	// Initialize all the ramdisks to be empty.
	for (i = 0; i < MAX_RAMDISKS; i++) {
		disks[i].used = false;
		disks[i].start = NULL;
		disks[i].size = 0;
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
int _rdisk_create(int disknum, unsigned int size) {
	uint32 num_slabs;

	if ( disknum < 0 || disknum >= MAX_RAMDISKS )
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
int _rdisk_destroy(int disknum) {
	if ( disknum < 0 || disknum >= MAX_RAMDISKS )
		return -1;
	if ( disks[disknum].used == false )
		return -1;
	_kfree_page(disks[disknum].start);
	disks[disknum].start = 0;
	disks[disknum].size = 0;
	disks[disknum].used = false;
	return disknum;
}

