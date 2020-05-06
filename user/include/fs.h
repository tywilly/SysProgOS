#ifndef _USER_FS_H_
#define _USER_FS_H_

#define MAX_FILES 64
#define MAX_PATH_LEN 128

#define FILE_TYPE_FILE 	(0)
#define FILE_TYPE_DEV	(1)

#define FILE_MODE_READ		(0x1)
#define FILE_MODE_WRITE		(0x2)
#define FILE_MODE_APPEND        (0x4)

typedef struct file_s {
	char path[MAX_PATH_LEN];
	uint8 type;
} File;

/*
** Reads from a file
**
** @param fd    The file descriptor to read from
** @param buf   The buffer to read data in to.
** @param len   The number of bytes to read.
**
** @return The number of bytes read.
*/
int _fs_read(int fd, void* buf, uint32 len);

/*
** Writes to a file
**
** @param fd    The file descriptor to write to.
** @param buf   The buffer containing the data to write.
** @param len   The length of the data to write.
**
** @return The number of bytes written.
*/
int _fs_write(int fd, const void* buf, uint32 len);

/*
** Moves the seek offset for a file.
**
** @param fd            The file descriptor to move the offset for.
** @param offset        The offset from whence to position the seek location.
** @param whence        The whence to seek relative to (see types.h).
**
** @return The resulting offset location as measured  in bytes from the
** beginning of the file. On error, the value (off_t) -1 is returned.
*/
int _fs_seek(int fd, int offset, int whence);

/*
** Opens a file for reading and/or writing.
**
** @param path  The path to the file to open (i.e. "A:/foo.txt").
** @param mode  Mode flags, Or'ed together.
**
** @return The file descriptor if open was successful, or a negative value if
** the open operation failed.
*/
int _fs_open(const char* path, int mode);

/*
** Closes a file.
**
** @param fd    The file descriptor for the file to close.
**
** @return 0 on success, a negative value if there was an error.
*/
int _fs_close(int fd);

#endif

