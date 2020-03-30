/*
** User function X:  write, exit
**
** Prints its PID at start and exit, iterates printing its character
** N times, and exits with a status equal to its PID.
**
** Invoked as:  userX [ x [ n ] ]
**   where x is the ID character (defaults to 'x')
**         n is a value to be used when printing our character
*/

int userX( int argc, char *args ) {
    int n;
    int count = 20;   // iteration count
    char ch = 'x'     // default character to print
    int status;
    int value = 17;   // default value
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userX x n"
        value = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userX x"
        ch = argv[1][0];
    }

    // announce our presence
    report( 'X', status=getpid() );

    sprint( buf, " %c[%d] ", ch, value );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
    }

    cwrites( buf );
    cwrites( " exiting\n" );

    exit( status );

    return( 42 );  // shut the compiler up!
}
