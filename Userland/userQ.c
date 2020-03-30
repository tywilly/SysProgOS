/*
** User function Q:  write, bogus, exit
**
** Reports itself, then tries to execute a bogus system call
**
** Invoked as:  userQ [ x ]
**   where x is the ID character (defaults to '1')
*/

int user!( int argc, char *args ) {
    int n;
    char ch = 'q'     // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    if( argc > 1 ) {    // "userQ x"
        ch = argv[1][0];
    }

    // announce our presence
    write( STR_SIO, &ch, 1 );

    // try something weird
    bogus();

    // should not have come back here!
    sprint( buf, "User %c returned from bogus syscall!?!?!\n", ch );
    cwrites( buf );

    exit( 1 );

    return( 42 );  // shut the compiler up!
}
