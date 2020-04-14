#include <device.h>
#include <types.h>
#include <klib.h>

#include <cio.h>
#include <sio.h>
#include <ramdisk.h>

#include "devfs.h"

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

static char* module_name = "devfs";

typedef struct dev_file_s {
	bool used;
	DeviceDriver* driver;
	int chan;
} DevFile;

typedef struct _device_class_s {
	char* name;
	DeviceDriver driver;
	int num_chan;
} DeviceClass;

static DevFile ofs[MAX_FILES];

/* Device classes and drivers */
static DeviceClass class[] = {
	{ "console", {"cio",NULL,NULL,NULL}, 1 },
	{ "serial", {"sio",NULL,NULL,NULL}, 1 },
	{ "ramdisk",
		{"ramdisk",_ramdisk_write,_ramdisk_read,_ramdisk_seek},
		MAX_RAMDISKS }
};
static const int num_classes = ARRAY_SIZE(class);

/* Gets a device class struct by the class name. */
static DeviceClass* get_device_class(const char* name) {
	int i;
	for ( i = 0; i < num_classes; i++ ) {
		if ( strcmp(name, class[i].name) == 0 )
			return &class[i];
	}
	return NULL;
}

void _devfs_init(void) {
	FsDriver driver;

	// Clear our Memory
	__memset( &ofs, 0, sizeof(ofs) );

	// Map the devfs
	driver.name = module_name;
	driver.write = _devfs_write;
	driver.read = _devfs_read;
	driver.lseek = _devfs_lseek;
	driver.open = _devfs_open;
	driver.close = _devfs_close;
	if ( _fs_map( 0, "devfs", &driver, 0 ) < 0 ) {
		__cio_puts( " **DEVFS (failed)**" );
		return;
	}

	// Print Success!
	__cio_puts( " DEVFS" );
}

int _devfs_write(int chan, int fd, const void* buf, uint32 len) {
	if ( fd < 0 || fd >= MAX_FILES || ofs[fd].used == false )
		return -1;

	return ofs[fd].driver->write( ofs[fd].chan, buf, len );
}

int _devfs_read(int chan, int fd, void* buf, uint32 len) {
	if ( fd < 0 || fd >= MAX_FILES || ofs[fd].used == false )
		return -1;

	return ofs[fd].driver->read( ofs[fd].chan, buf, len );

}

int _devfs_lseek(int chan, int fd, int offset, int whence) {
	if ( fd < 0 || fd >= MAX_FILES || ofs[fd].used == false )
		return -1;

	return ofs[fd].driver->lseek( ofs[fd].chan, offset, whence );
}

int _devfs_open(int chan, int fd, const char* path, int mode) {
	char dev_class_name[MAX_PATH_LEN];
	char* dev_number;
	DeviceClass* dev_class;
	int dev_chan;

	// Check the fd
	if ( fd < 0 || fd >= MAX_FILES || ofs[fd].used == true )
		return -1;

	// Parse and convert the options
	__strcpy( dev_class_name, path );
	dev_number = __strsplit( dev_class_name, "/" );
	dev_chan = ( dev_number == NULL ) ? 0 : __str2int( dev_number, 10 );
	dev_class = get_device_class(dev_class_name);
	if ( dev_class == NULL )
		return -1;

	// Set the fd and return
	ofs[fd].used = true;
	ofs[fd].driver = &dev_class->driver;
	ofs[fd].chan = dev_chan;
	return fd;
}

int _devfs_close(int fd) {
	if ( fd < 0 || fd >= MAX_FILES || ofs[fd].used == false )
		return -1;

	ofs[fd].used = false;
	return 0;
}

