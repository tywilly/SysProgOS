#include "common.h"
#include "ulib.h"
#include "consh.h"
#include "userland.h"

// Function prototypes
static int runcmd( char* argv[] );
static int consh_help( char* argv[] );
static int consh_exit( char* argv[] );
static int consh_getpid( char* argv[] );
static int consh_getppid( char* argv[] );
//static int consh_getstat( char* argv[] );

int consh_main( int argc, char* args );

static int consh_echo( int argc, char* args );
static int consh_kill( int argc, char* args );
static int consh_sleep( int argc, char* args );

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
static const int consh_num_builtins = 4;
static const struct command_builtin consh_builtins[] = {
	{"help", consh_help,	"Display a help message."},
	{"exit", consh_exit,	"Exit the shell."},
	{"getpid", consh_getpid,	"Print the current process ID."},
	{"getppid", consh_getppid,
		"Print the parent process ID of the current process."}
	/*{"getstat", consh_getstat,
		"Print the exit status of the last process."}*/
};

// An array of command applets for consh. Add commands here. Make sure to
// update consh_num_applets.
static const int consh_num_applets = 4;
static const struct command_applet consh_applets[] = {
	{"consh", consh_main,	"The interactive console shell."},
	{"echo", consh_echo,	"Echo the arguments back to stdout."},
	{"kill", consh_kill,	"Kill the specified process."},
	{"sleep", consh_sleep,	"Sleep the specified number of milliseconds."}
};

/* Simple console shell
** 
** A simple interactive console shell.
** 
** Invoked as: consh
*/
int consh_main( int argc, char* args) {
	char buf[CONSH_LINE_LEN];
	int i, n;
	char* pargs[CONSH_MAX_ARGC+1];

	while(1) {
		n = readline(consh_prompt, buf, CONSH_LINE_LEN, stdin, stdout);
		if ( n < 0 ) {
			fputs("Failed to read from stdin\r\n", stderr);
		}
		if (buf[n] == '\n')
			buf[n] = '\0';

		// Parse args
		pargs[0] = buf;
		for (i = 1; i < CONSH_MAX_ARGC; i++) {
			pargs[i] = strsplit(pargs[i-1], " \t\n");
			if (pargs[i] == NULL)
				break;
		}
		pargs[i] = NULL;  // In case we have the max number of args
		
		// Run the command
		n = runcmd(pargs);
		if ( n < 0 ) {
			sprint(pargs[1], "Failed to run %s\n", pargs[0]);
			fputs(pargs[1], stderr);
			sprint(pargs[1], "%s: command not found. "
					"Run help for a list of commands.\r\n",
					pargs[0]);
			fputs(pargs[1], stdout);
		}
		else if ( n != 0 ) {
			int32 status;
			sprint(pargs[1], "Started %s as process %d\n",
					pargs[0], n);
			fputs(pargs[1], stderr);
			wait(n, &status);
			/*sprint(pargs[1], "%s (%d) exited with status %d\r\n",
					pargs[0], n, status);
			fputs(pargs[1], stderr);*/
		}
	}
	
	exit( 0 );
	
	return( 42 );  // shut the compiler up!
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

