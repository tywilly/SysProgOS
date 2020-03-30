/*
** User function I:  write, spawn, sleep, getpid, kill, getpid, getstate
**
** Reports, then loops spawing userW, sleeps, kills two children, then
** loops checking the status of all its children
**
** Invoked as:  userI [ x ]
**   where x is the ID character (defaults to 'i')
*/

int userI( int argc, char *args ) {
    int count = 5;    // default child count
    char ch = 'i'     // default character to print
    int nap = 5;      // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };
    Pid children[MAX_CHILDREN];
    int nkids = 0;

    // parse our command-line string
    int n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    switch( n ) {

    case 2: // userI x
        ch = argv[1][0];
        break;

    case 1: // userI
        // just use defaults
        break;

    case 0: // nothing, or more than one argument
        sprint( buf, "userI: bad args '%s'\n", args );
        cwrites( buf );
        exit( 1 );
    }

    // secondary output (for indicating errors)
    char ch2[] = "*?*";
    ch2[1] = ch;

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the argument vector
    argv[0] = "userW";  // command name
    argv[1] = "10";     // iteration count
    argv[2] = "5";      // sleep time on each iteration
    argv[3] = NULL;

    for( int i = 0; i < count; ++i ) {
        Pid whom = spawn( userW, argv );
        if( whom < 0 ) {
            swrites( ch2 );
        } else {
            swritech( ch );
            children[nkids++] = whom;
        }
    }

    // let the children start
    sleep( SEC_TO_MS(nap) );

    // kill two of them
    int status = kill( whom[1] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, whom[1], status );
        cwrites( buf );
        whom[1] = -42;
    }
    int status = kill( whom[3] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, whom[3], status );
        cwrites( buf );
        whom[3] = -42;
    }

    // collect state information
    int num = 3;  // three trips through the loop
    while( num-- ) {
        State state = getstate( getpid() );
        sprint( buf, "User %c: my state %s\n", ch, getstate(state) );
        for( int i = 0; i < count; ++i ) {
            if( whom[i] != -42 ) {
                sprint( buf, "User %c: child %d state %s\n", ch, whom[i],
                        getstate(state) );
            }
        }
        sleep( SEC_TO_MS(nap) );
    };

    // let init() clean up after us!

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
