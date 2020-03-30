/*
** User function R:  write, sleep, read, exit
**
** Reports itself, then loops forever reading and printing SIO characters
**
** Invoked as:  userR [ x [ s ] ]
**   where x is the ID character (defaults to '1')
**         s is the initial delay time (defaults to 10)
*/

int userR( int argc, char *args ) {
    int n;
    char ch = 'r'     // default character to print
    int delay = 10;   // initial delay
    char buf[128];
    char b2[8];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userR x s"
        delay = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userR x"
        ch = argv[1][0];
    }

    // announce our presence
    b2[0] = ch;
    b2[1] = '\0';  // just in case!
    b2[2] = '\0';
    write( CHAN_SIO, b2, 1 );

    sleep( SEC_TO_MS(delay) );

    for(;;) {
        int32 n = read( CHAN_SIO, &b2[1], 1 );
        if( n != 1 ) {
            sprint( buf, "User %c, read returned %d\n", ch, n );
            cwrites( buf );
            if( n == -1 ) {
                // wait a bit
                sleep( SEC_TO_MS(1) );
            }
        } else {
	    write( CHAN_SIO, b2, 2 );
        }
    }

    sprint( buf, "User %c exiting!?!?!?\n", ch );
    cwrites( buf );
    exit( 1 );

    return( 42 );  // shut the compiler up!

}
