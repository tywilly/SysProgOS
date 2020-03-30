/*
** User function Y:  write, sleep, exit
**
** Reports its PID, then iterates N times printing its PID and
** sleeping for one second, then exits.
**
** Invoked as:  userY [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to 'Y')
**         n is the iteration count (defaults to 10)
**         s is a sequence string (defaults to "0.0")
*/

int userY( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = 'y'     // default character to print
    char *seq = "0.0";      // sequence string
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "userY x n s"
        seq = str2int( argv[3], 10 );
    }

    if( argc > 2 ) {    // "userY x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userY x"
        ch = argv[1][0];
    }

    sprint( buf, " %c[%s] ", ch, seq );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
        sleep( SEC_TO_MS(1) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
