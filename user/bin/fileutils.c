#include <common.h>
#include <ulib.h>
#include <fs.h>
#include <kernel/devfs.h>

#include "fileutils.h"

#define MAX_ARGS 3
#define DATA_BLOCK_SIZE 256

/*
** Writes contents to a file.
**
** Usage: write <file> <contents>
*/
int write_main( int argc, char* args ) {
	int i;
	int fd;
	char* argv[MAX_ARGS];

	if (argc < 3) {
		fputs("Usage: write <file> <contents>\r\n", stdout);
		exit(1);
	}

	argv[0] = args;
	argv[1] = argv[0] + strlen(argv[0]) + 1;
	argv[2] = argv[1] + strlen(argv[1]) + 1;

	for ( i = 3; i < argc; i++ ) {
		argv[2][strlen(argv[2])] = ' ';
	}

	// Open File
	fd = _fs_open(argv[1], FILE_MODE_WRITE);
	if ( fd < 0 ) {
		fputs("Failed to open file\r\n", stdout);
		exit(2);
	}

	// Write
	i = _fs_write(fd, argv[2], strlen(argv[2]));
	if ( i < 0 ) {
		fputs("Failed to write to file\r\n", stdout);
		i = _fs_close(fd);
		exit(2);
	}

	// Close
	i = _fs_close(fd);
	if ( i < 0 ) {
		fputs("Failed to close file\r\n", stdout);
		exit(2);
	}

	exit(0);
	return 0;
}

/*
** Prints the contents of a file.
**
** Usage: cat <file>
*/
int cat_main( int argc, char* args ) {
	int i;
	int fd;
	char* argv[MAX_ARGS];
	char data[DATA_BLOCK_SIZE];

	if (argc < 2) {
		fputs("Usage: cat <file>\r\n", stdout);
		exit(1);
	}

	argv[0] = args;
	argv[1] = argv[0] + strlen(argv[0]) + 1;

	// Open file
	fd = _fs_open(argv[1], FILE_MODE_READ);
	if ( fd < 0 ) {
		fputs("Failed to open file\r\n", stdout);
		exit(2);
	}

	// Read data
	for (;;) {
		i = _fs_read(fd, data, DATA_BLOCK_SIZE);
		if ( i == 0 )
			break;
		else if ( i < 0 ) {
			fputs("Failed to read from file\r\n", stdout);
			i = _fs_close(fd);
			exit(2);
		}
		write(stdout, data, i);
	}

	// Close
	i = _fs_close(fd);
	if ( i < 0 ) {
		fputs("Failed to close file\r\n", stdout);
		exit(2);
	}

	exit(0);
	return 0;
}

