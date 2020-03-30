/*
** User function W:  write, sleep, exit, getpid(), gettime()
**
** Reports its presence, then iterates 'n' times printing identifying
** information and sleeping, before exiting.
**
** Invoked as:  userW [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to '1')
**         n is the iteration count (defaults to 20)
**         s is the sleep time (defaults to 3)
*/

int userW( int argc, char *args ) {
    int n;
    int count = 20;   // default iteration count
    char ch = 'w'     // default character to print
    int nap = 3;      // nap length
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "userW x n s"
        nap = str2int( argv[3], 10 );
    }

    if( argc > 2 ) {    // "userW x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userW x"
        ch = argv[1][0];
    }

    // announce our presence
    report( 'W', getpid() );

    uint32 tlower = (uint32) (gettime() & UINT64_LOWER);

    sprint( buf, " %c[%u] ", ch, tlower );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        sleep( SEC_TO_MS(nap) );
    }


    cwrites( buf );
    cwrites( " exiting\n" );

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
