#include "common.h"
#include "ulib.h"
#include "consh.h"
#include "userland.h"
#include "ramdisk.h"
#include "fileutils.h"

// Function prototypes
static int runcmd( char* argv[] );
static int parse_and_run( char* cmd );

static int consh_help( char* argv[] );
static int consh_exit( char* argv[] );
static int consh_getpid( char* argv[] );
static int consh_getppid( char* argv[] );
//static int consh_getstat( char* argv[] );

int consh_main( int argc, char* args );

static int consh_echo( int argc, char* args );
static int consh_kill( int argc, char* args );
static int consh_sleep( int argc, char* args );

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

#define CONSH_LINE_LEN 256
#define CONSH_MAX_ARGC 32
const char* consh_prompt = "consh> ";

// Data structure to hold a built in command
struct command_builtin {
	char* name;
	int(*entry)(char**);
	char* help;
};

// Data structure to hold a command applet
struct command_applet {
	char* name;
	int(*entry)(int,char*);
	char* help;
};

// An array of built in commands for consh. Add builtins here. Make sure to
// update consh_num_builtins.
static const struct command_builtin consh_builtins[] = {
	{"help", consh_help,	"Display a help message."},
	{"exit", consh_exit,	"Exit the shell."},
	{"getpid", consh_getpid,	"Print the current process ID."},
	{"getppid", consh_getppid,
		"Print the parent process ID of the current process."}
	/*{"getstat", consh_getstat,
		"Print the exit status of the last process."}*/
};
static const int consh_num_builtins = ARRAY_SIZE(consh_builtins);

// An array of command applets for consh. Add commands here. Make sure to
// update consh_num_applets.
static const struct command_applet consh_applets[] = {
	{"cat", cat_main,	"Print the contents of a file."},
	{"consh", consh_main,	"The interactive console shell."},
	{"echo", consh_echo,	"Echo the arguments back to stdout."},
	{"kill", consh_kill,	"Kill the specified process."},
	{"sleep", consh_sleep,	"Sleep the specified number of milliseconds."},
	{"ramdisk", ramdisk_main,
		"Ramdisk management utility."},
	{"write", write_main,	"Write a string to a file."}
};
static const int consh_num_applets = ARRAY_SIZE(consh_applets);

/* Simple console shell
** 
** A simple interactive console shell.
** 
** Invoked as: consh
*/
int consh_main( int argc, char* args) {
	char buf[CONSH_LINE_LEN];
	char cmdname[32];
	int n;

	while(1) {
		n = readline(consh_prompt, buf, CONSH_LINE_LEN, stdin, stdout);
		if ( n < 0 ) {
			fputs("Failed to read from stdin\r\n", stderr);
		}
		if (buf[n] == '\n')
			buf[n] = '\0';

		// Run the command
		n = parse_and_run(buf);
		strsplit(buf, " \t\n");
		strcpy (cmdname, buf);
		if ( n < 0 ) {
			sprint(buf, "Failed to run %s\n", cmdname);
			fputs(buf, stderr);
			sprint(buf, "%s: command not found. "
					"Run help for a list of commands.\r\n",
					cmdname);
			fputs(buf, stdout);
		}
		else if ( n != 0 ) {
			int32 status;
			sprint(buf, "Started %s as process %d\n",
					cmdname, n);
			fputs(buf, stderr);
			wait(n, &status);
		}
	}
	
	exit( 0 );
	
	return( 42 );  // shut the compiler up!
}

/*
** Parses the cmd into an argv array and then runs it with runcmd.
**
** @param args	The argument string to parse
**
** @return the return value of runcmd
*/
static int parse_and_run( char* cmd ) {
	char* argv[CONSH_MAX_ARGC+1];
	int i;

	argv[0] = cmd;
	for (i = 1; i < CONSH_MAX_ARGC; i++) {
		argv[i] = strsplit(argv[i-1], " \t\n");
		if (argv[i] == NULL)
			break;
	}
	argv[i] = NULL;  // In case we have the max number of args
	return runcmd(argv);
}

/*
** Runs a command applet with the specified argv[0], spawning a new process for
** the applet. The argv list should be NULL terminated.
**
** @param argv	The arguments to pass to the new process (NULL terminated).
**
** @return the PID of the new process on success, 0 if the command was a
** builtin, -1 if the command applet could not be started.
*/
static int runcmd( char* argv[] ) {
	int i;

	// Check if no command was specified and be silent if so
	if (argv[0][1] == '\0')
		return 0;

	// Check if there is a builtin
	for (i = 0; i < consh_num_builtins; i++) {
		if (strcmp(argv[0], consh_builtins[i].name) == 0) {
			consh_builtins[i].entry(argv);
			return 0;
		}
	}

	// If not, check if there is an applet.
	for (i = 0; i < consh_num_applets; i++) {
		if (strcmp(argv[0], consh_applets[i].name) == 0) {
			return spawn( consh_applets[i].entry, argv );
		}
	}
	
	return -1;
}

/*
** Displays a help message, listing help for each command.
**
** Invoked as: help
*/
static int consh_help( char* argv[] ) {
	int i;
	char buf[128];

	fputs("Shell Builtins:\r\n", stdout);
	for (i = 0; i < consh_num_builtins; i++) {
		sprint(buf, "%s\t%s\r\n",
			consh_builtins[i].name,
			consh_builtins[i].help);
		fputs(buf, stdout);
	}

	fputs("\r\nCommand Applets:\r\n", stdout);
	for (i = 0; i < consh_num_applets; i++) {
		sprint(buf, "%s\t%s\r\n",
			consh_applets[i].name,
			consh_applets[i].help);
		fputs(buf, stdout);
	}

	return 0;
}

/*
** Exits the current shell. Does not return.
**
** @param argv	The value to exit with.
**
** Invoked as: exit [status]
*/
static int consh_exit( char* argv[] ) {
	int32 status = argv[1] == NULL ? 0 : str2int(argv[1], 10);
	exit(status);

	return 42; // Make the compiler happy
}

/*
** Prints the current process ID.
**
** Invoked as: getpid
*/
static int consh_getpid( char* argv[] ) {
	char buf[8];
	Pid pid = getpid();

	sprint(buf, "%d\r\n", pid);
	fputs(buf, stdout);
	return 0;
}

/*
** Prints the current process's parent process ID.
**
** Invoked as: getppid
*/
static int consh_getppid( char* argv[] ) {
	char buf[8];
	Pid pid = getppid();

	sprint(buf, "%d\r\n", pid);
	fputs(buf, stdout);
	return 0;
}

/* Echo arguments back to stdout
** 
** Invoked as: echo <args> ...
*/
int consh_echo( int argc, char* args) {
	char* arg;
	int i;

	arg = args + strlen(args) + 1;
	for (i = 1; i < argc; i++) {
		fputs(arg, stdout);
		if (i != argc-1)
			fputc(' ', stdout);
		arg = arg + strlen(arg) + 1;
	}

	fputs("\r\n", stdout);
	exit(0);
	return 42;
}

/* Kills a specified process
** 
** Invoked as: kill <pid>
*/
int consh_kill( int argc, char* args) {
	Pid pid;
	char* arg1;

	if (argc != 2) {
		fputs("Usage: kill <pid>\r\n", stdout);
		exit(1);
	}

	arg1 = args + strlen(args) + 1;
	pid = str2int(arg1, 10);
	kill(pid);
	exit(0);
	return 42;
}

/* Sleep a specified number of milliseconds
** 
** Invoked as: sleep <milliseconds>
*/
int consh_sleep( int argc, char* args) {
	uint32 msec;
	char* arg1;

	if (argc != 2) {
		fputs("Usage: sleep <millisconds>\r\n", stdout);
		exit(1);
	}

	arg1 = args + strlen(args) + 1;
	msec = str2int(arg1, 10);
	sleep(msec);
	exit(0);
	return 42;
}

