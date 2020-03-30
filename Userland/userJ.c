/*
** User function J:  write, getpid, spawn, exit
**
** Reports, tries to spawn lots of children, then exits
**
** Invoked as:  userJ [ x [ n ] ]
**   where x is the ID character (defaults to '1')
**         n is the number of children to spawn (defaults to 2 * N_PROCS)
**         s is the sleep time (defaults to 30)
*/

int userJ( int argc, char *args ) {
    int n;
    int count = 2 * N_PROCS;   // number of children to spawn
    char ch = 'j'     // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userJ x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userJ x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the command-line arguments
    argv[0] = "userY";
    argv[1] = "Y";
    argv[2] = count;
    argv[3] = buf;
    argv[4] = NULL;

    Pid me = getpid();

    for( int i = 0; i < count ; ++i ) {
        sprint( buf, "%d.%d", me, i );
        int32 whom = spawn( userY, argv );
        if( whom < 0 ) {
            write( CHAN_SIO, "!j!", 3 );
        } else {
            write( CHAN_SIO, &ch, 1 );
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
