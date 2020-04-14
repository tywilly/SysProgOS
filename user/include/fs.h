#ifndef _USER_FS_H_
#define _USER_FS_H_

#define MAX_FILES 64
#define MAX_PATH_LEN 128

#define FILE_TYPE_FILE 	(0)
#define FILE_TYPE_DEV	(1)

#define FILE_MODE_READ		(0x1)
#define FILE_MODE_WRITE		(0x2)

typedef struct file_s {
	char path[MAX_PATH_LEN];
	uint8 type;
} File;

#endif

