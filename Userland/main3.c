/*
** User function #3:  write, sleep, exit
**
** Prints its ID, then loops N times sleeping and printing, then exits.
**
** Invoked as:  main3 [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to '3')
**         n is the iteration count (defaults to 10)
**         s is the sleep time in seconds (defaults to 5)
*/

int main3( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = '3';    // default character to print
    int nap = 5;      // default sleep time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "main3 x n s"
        nap = str2int( argv[3], 10 );
    }

    if( argc > 2 ) {    // "main3 x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "main3 x"
        ch = argv[1][0];
    }

    // announce our presence a little differently
    report( 'E', getpid() );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < count ; ++i ) {
        sleep( SEC_TO_MS(nap) );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
