#include <common.h>
#include <klib.h>
#include <cio.h>

#include "fs.h"
#include "fs/devfs.h"

#define MAX_DRIVES 27

typedef struct filesystem {
	bool used;
	char devname[MAX_PATH_LEN];
	FsDriver driver;
	int chan;
} FileSystem;

typedef struct open_file {
	bool used;
	uint8 mode;
	int drv_fd;
	FileSystem fs;
} OpenFile;

static FileSystem fs[MAX_DRIVES];
static OpenFile ofs[MAX_FILES];

void _fs_init(void) {
	__memset(fs, sizeof(fs), 0);
	__memset(ofs, sizeof(ofs), 0);
	__cio_puts( " FS (" );

	// Initialize the filesystem drivers
	_devfs_init();

	__cio_puts( " )" );

	__cio_puts( "\n" );
	_fs_print();
}

/*
** Map a filesystem driver for the specified drive number.
**
** @param drivenum	The number of the drive to map the filesystem to.
** @param devnam	A human readable device name.
** @param driver	The driver for the filesystem.
** @param chan		The channel to pass to the filesystem driver for
** 			each operation on this filesystem.
** @return 0 on success, a negative value on error.
*/
int _fs_map(uint8 drivenum, char* devname, FsDriver* driver, int chan) {
	if ( drivenum > MAX_DRIVES || fs[drivenum].used == true )
		return E_BAD_CHANNEL;

	__strcpy( fs[drivenum].devname, devname );
	__memcpy( &fs[drivenum].driver, driver, sizeof(FsDriver) );
	fs[drivenum].chan = chan;
	fs[drivenum].used = true;

	return 0;
}

/*
** Prints the currently mapped filesystems to the debug output.
*/
void _fs_print(void) {
	int i;
	char buf[MAX_PATH_LEN + 64];

	__cio_puts( "Drive | Driver | Channel | Device Name\n");
	for ( i = 0; i < MAX_DRIVES; i++ ) {
		if ( fs[i].used == true ) {
			__sprint( buf, "%c/ | %s | %d | \n",
					i + 64,
					fs[i].driver.name,
					fs[i].chan,
					fs[i].devname );
			__cio_puts( buf );
		}
	}
}

/*
** Reads from a file
**
** @param fd	The file descriptor to read from
** @param buf   The buffer to read data in to.
** @param len   The number of bytes to read.
**
** @return The number of bytes read.
*/
int _fs_read(int fd, const void* buf, uint32 len) { return -1; }

/*
** Writes to a file
**
** @param fd	The file descriptor to write to.
** @param buf   The buffer containing the data to write.
** @param len   The length of the data to write.
**
** @return The number of bytes written.
*/
int _fs_write(int fd, const void* buf, uint32 len) { return -1; }

/*
** Moves the seek offset for a file.
**
** @param fd	        The file descriptor to move the offset for.
** @param offset        The offset from whence to position the seek location.
** @param whence        The whence to seek relative to (see types.h).
**
** @return The resulting offset location as measured  in bytes from the
** beginning of the file. On error, the value (off_t) -1 is returned.
*/
int _fs_seek(int fd, int offset, int whence) { return -1; }

/*
** Opens a file for reading and/or writing.
**
** @param path	The path to the file to open (i.e. "A/foo.txt").
** @param mode	Mode flags, Or'ed together.
**
** @return The file descriptor if open was successful, or a negative value if
** the open operation failed.
*/
int _fs_open(const char* path, int mode) { return -1; }

/*
** Closes a file.
**
** @param fd	The file descriptor for the file to close.
**
** @return 0 on success, a negative value if there was an error.
*/
int _fs_close(int fd) { return -1; }

