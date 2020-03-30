/*
** User function Z:  write, getpid, getppid, sleep, exit
**
** Prints its ID, then records PID and PPID, loops printing its ID,
** and finally re-gets PPID for comparison.  Yields after every second
** ID print in the loop.
**
** This code is used as a handy "spawn me" test routine; it is spawned
** by several of the standard test processes.
**
** Invoked as:  userZ [ x [ n ] ]
**   where x is the ID character (defaults to '9')
**         n is the iteration count (defaults to 10)
*/

int userZ( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = 'z'     // default character to print
    char buf[128], buf2[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userZ x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userZ x"
        ch = argv[1][0];
    }

    // announce our presence
    swritech( ch );

    // record the relevant PIDs and report it to the console
    Pid me = getpid();
    Pid parent = getppid();

    sprint( buf, "%c[%u/%u]", ch, me, parent );

    sprint( buf2, "user %s running\n", buf );
    cwrites( buf2 );

    // iterate for a while; occasionally yield the CPU
    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
        if( i & 1 ) {
            sleep( 0 );
        }
    }

    // get "new" parent PID and report the result
    parent = getppid();

    sprint( buf2, "user %s exiting, parent now %d\n", buf, parent );
    cwrites( buf2 );

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
