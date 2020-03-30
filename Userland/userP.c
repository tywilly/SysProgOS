/*
** User function P:  write, gettime, sleep
**
** Reports itself, then loops reporting the current time
**
** Invoked as:  userP [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to 'p')
**         n is the iteration count (defaults to 3)
**         s is the sleep time (defaults to 2)
*/

int userP( int argc, char *args ) {
    int n;
    int count = 3;    // default iteration count
    char ch = 'p'     // default character to print
    int nap = 2;      // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "user? x n s"
        nap = str2int( argv[3], 10 );
    }

    if( argc > 2 ) {    // "user? x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "user? x"
        ch = argv[1][0];
    }

    // announce our presence
    Time now = gettime();
    sprint( buf, "User %c running, start at %d\n", ch, now );
    cwrites( buf );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < 3; ++i ) {
        sleep( SEC_TO_MS(nap) );
        now = gettime();
        sprint( buf, "User %c reporting time %d\n", ch, now );
        cwrites( buf );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
