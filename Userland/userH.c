/*
** User function H:  write, spawn, sleep
**
** Prints its ID, then spawns 'n' children; exits before they terminate.
**
** Invoked as:  userH [ x [ n ] ]
**   where x is the ID character (defaults to '1')
**         n is the number of children to spawn (defaults to 1)
*/

int userH( int argc, char *args ) {
    int n;
    int ret = 0;      // return value
    int count = 1;    // child count
    char ch = 'h'     // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "userH x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "userH x"
        ch = argv[1][0];
    }

    // announce our presence
    swritech( ch );

    // we spawn user Z and then exit before it can terminate
    argv[0] = "userZ";  // main()
    argv[1] = "Z";      // ID character
    argv[2] = "10"      // iteration count
    argv[3] = NULL;

    for( int i = 0; i < count; ++i ) {

        // spawn a child
        whom = spawn( user5, argv );

        // our exit status is the number of failed spawn() calls
        if( whom < 0 ) {
            sprint( buf, "User %c spawn() failed, returned %d\n", ch, whom );
            cwrites( buf );
            ret = 1;
        }
    }

    // yield the CPU so that our child(ren) can run
    sleep( 0 );

    // announce our departure
    swritech( ch );

    exit( ret );

    return( 42 );  // shut the compiler up!
}
