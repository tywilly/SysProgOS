/*
** User function #4:  write, getpid, spawn, sleep, exit
**
** Loops, spawning N copies of userX and sleeping between spawns.
**
** Invoked as:  main4 [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to '1')
**         n is the iteration count (defaults to 5)
**         s is the sleep time (defaults to 30)
*/

int main4( int argc, char *args ) {
    int n;
    int count = 5;    // default iteration count
    char ch = '4'     // default character to print
    int nap = 30;     // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "main4 x n s"
        nap = str2int( argv[3], 10 );
    }

    if( argc > 2 ) {    // "main4 x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "main4 x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // create the argument vector for userX
    argv[0] = "userX";
    argv[1] = buf;
    argv[2] = NULL;

    Pid me = getpid();

    for( int i = 0; i < count ; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        sprint( buf, "%d", (int) pid << 4 + i );
        int whom = spawn( userX, argv );
        if( whom < 0 ) {
            swritech( ch2 );
        }
        sleep( SEC_TO_MS(nap) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
