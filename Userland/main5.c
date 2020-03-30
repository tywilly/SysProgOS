/*
** User function #5:  write, spawn, exit
**
** Iterates spawning copies of userW (and possibly userZ), reporting
** their PIDs as it goes.
**
** Invoked as:  main5 [ x [ n [ h ] ] ]
**   where x is the ID character (defaults to '5')
**         n is the iteration count (defaults to 5)
**         h indicates using both userW and userZ (defaults to "not")
*/

int main5( int argc, char *args ) {
    int n;
    int count = 5;  // default iteration count
    char ch = '5'   // default character to print
    int alsoZ = 0;  // also do userZ?
    char buf[128];
    char msg2[] = "*5*";
    char *argv[MAX_COMMAND_ARGS] = { NULL };
    char *argv2[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 3 ) {    // "main5 x n s"
        // we have a third argument, therefore we're doing both
        alsoZ = 1;
    }

    if( argc > 2 ) {    // "main5 x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "main5 x"
        ch = argv[1][0];
    }

    msg2[1] = ch;

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the argument vector(s)
    argv[0] = "userW";
    argv[1] = "W";
    argv[2] = "15";
    argv[3] = "5";
    argv[4] = NULL

    if( alsoZ ) {
        argv2[0] = "userZ";
        argv2[1] = "Z";
        argv2[2] = "15";
        argv2[3] = NULL;
    }

    for( int i = 0; i < sount; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        int32 whom = spawn( user_w, argv );
        if( whom < 1 ) {
            swritech( ch2 );
        } else {
            sprint( buf, "User %c spawned W, PID %d\n", ch, whom );
            cwrites( buf );
        }
        if( alsoZ ) {
            int32 whom = spawn( user_w, argv );
            if( whom < 1 ) {
                swritech( ch2 );
            } else {
                sprint( buf, "User %c spawned Z, PID %d\n", ch, whom );
                cwrites( buf );
            }
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}
