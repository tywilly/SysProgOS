/*
** User function S:  write, sleep, exit
**
** Reports itself, then loops forever, sleeping on each iteration
**
** Invoked as:  userS [ x [ s ] ]
**   where x is the ID character (defaults to 's')
**         s is the sleep time (defaults to 20)
*/

int userS( int argc, char *args ) {
    int n;
    char ch = 's'     // default character to print
    int nap = 20;     // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userS x s"
        nap = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userS x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    for(;;) {
        sleep( SEC_TO_MS(nap) );
        write( CHAN_SIO, &ch, 1 );
    }

    sprint( buf, "User %c exiting!?!?!?\n", ch );
    cwrites( buf );
    exit( 1 );

    return( 42 );  // shut the compiler up!
}
