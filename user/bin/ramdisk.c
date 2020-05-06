#include <common.h>
#include <ulib.h>
#include <kernel/ramdisk.h>

#include "ramdisk.h"

#define MAX_ARGS 4

static const char* helpstr = 
"Ramdisk Utility Help\r\n"
"Usage: ramdisk <subcommand> [arguments]\r\n"
"\r\n"
"Subcommands:\r\n"
"\r\n"
"help	Display this help message.\r\n"
"\r\n"
"list	List the status of all the system's ramdisks.\r\n"
"\r\n"
"create <disk_number> <size>\r\n"
"	Create a new ramdisk\r\n"
"	disk_number = the disk number to create (-1 for next available)\r\n"
"	size = the size of the ramdisk in bytes\r\n"
"\r\n"
"destroy <disk_number>\r\n"
"	Destroy a ramdisk\r\n"
"	disk_number = the disk number to destroy\r\n";

// Function prototypes
static void list(void);
static void help(void);
static void create(int disknum, uint32 size);
static void destroy(int disknum);
static void test(int disknum);

int ramdisk_main( int argc, char* args ) {
	int i;
	char* argv[MAX_ARGS];

	if (argc == 1) {
		fputs(	"Usage: ramdisk <subcommand>\r\n"
			"Use ramdisk help for help.\r\n", stdout);
		exit(1);
	}
	if (argc > MAX_ARGS) {
		fputs("Too many arguments\r\n", stdout);
		exit(1);
	}

	for (i = 0; i < argc; i++) {
		argv[i] = args;
		args += strlen(args) + 1;
	}

	// Main switch on argv[1]
	if (strcmp(argv[1], "list") == 0) {
		list();
	}
	else if (strcmp(argv[1], "help") == 0) {
		help();
	}
	else if (strcmp(argv[1], "create") == 0) {
		if (argc != 4) {
			fputs("Usage: ramdisk create <device_number> "
					"<size>\r\n", stdout);
			exit(1);
		}
		int disknum = str2int(argv[2], 10);
		int size = str2int(argv[3], 10);
		create(disknum, size);
	}
	else if (strcmp(argv[1], "destroy") == 0) {
		if (argc != 3) {
			fputs("Usage: ramdisk destroy <device_number>\r\n",
					stdout);
			exit(1);
		}
		int disknum = str2int(argv[2], 10);
		destroy(disknum);
	}
	else if (strcmp(argv[1], "test") == 0) {
		if (argc != 3) {
			fputs("Usage: ramdisk test <device_number>\r\n",
					stdout);
			exit(1);
		}
		int disknum = str2int(argv[2], 10);
		test(disknum);
	}
	else {
		fputs(	"Usage: ramdisk <subcommand>\r\n"
			"Use ramdisk help for help.\r\n", stdout);
		exit(1);
	}

	exit(0);
	return 0;
}

static void help(void) {
	fputs(helpstr, stdout);
}

static void list(void) {
	int i;
	RamDisk r;
	char buf[64];

	fputs("Number\tUsed\tStart Address\tSize\t\tSeek\r\n", stdout);
	for (i = 0; i < MAX_RAMDISKS; i++) {
		r = _ramdisk_get(i);
		sprint(buf, "%d\t%d\t0x%08x\t0x%08x\t%d\r\n",
				i, r.used, r.start, r.size, r.seek);
		fputs(buf, stdout);
	}
}

static void create(int disknum, uint32 size) {
	int n;
	char buf[32];

	n = _ramdisk_create(disknum, size);
	if (n < 0) {
		fputs("Failed to create Ramdisk\r\n", stdout);
		exit(1);
	}

	sprint(buf, "Created ramdisk %d\r\n", n);
	fputs(buf, stdout);

}

static void destroy(int disknum) {
	int n;
	char buf[32];

	n = _ramdisk_destroy(disknum);
	if (n < 0) {
		fputs("Failed to destroy Ramdisk\r\n", stdout);
		exit(1);
	}

	sprint(buf, "Destroyed ramdisk %d\r\n", n);
	fputs(buf, stdout);
}

static void test(int disknum) {
	int n;
	char buf[64];
	char buf2[64];

	// Test _ramdisk_write
	fputs("Writing test data 0123456789ABCDEF\r\n", stdout);
	n = _ramdisk_write(disknum, "0123456789ABCDEF", 16);
	sprint(buf, "Wrote %d bytes\r\n", n);
	fputs(buf, stdout);

	// Test _ramdisk_seek
	fputs("Seeking\r\n", stdout);
	n = _ramdisk_seek(disknum, SEEK_SET, 0);
	sprint(buf, "Seeked to offset %d\r\n", n);
	fputs(buf, stdout);

	// Test _ramdisk_read
	fputs("Reading test data\r\n", stdout);
	n = _ramdisk_read(disknum, buf2, 16);
	buf2[n] = '\0';
	sprint(buf, "Read %d bytes: %s\r\n", n, buf2);
	fputs(buf, stdout);


}

