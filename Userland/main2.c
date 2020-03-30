/*
** User function #2:  write
**
** Prints its ID, then loops N times delaying and printing, then returns
** without calling exit().  Verifies the return byte count from each call
** to write().
**
** Invoked as:  main2 [ x [ n ] ]
**   where x is the ID character (defaults to '2')
**         n is the iteration count (defaults to 30)
*/

int main2( int argc, char *args ) {
    int n;
    int count = 30;   // default iteration count
    char ch = '2'     // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "main2 x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "main2 x"
        ch = argv[1][0];
    }

    // announce our presence
    n = swritech( ch );
    if( n != 1 ) {
        sprint( buf, "User %c, write #1 returned %d\n", ch, n );
        cwrites( buf );
    }

    // iterate and print the required number of other characters
    for( int i = 0; i < count; ++i ) {
        DELAY(STD);
        n = swritech( ch );
        if( n != 1 ) {
            sprint( buf, "User %c, write #2 returned %d\n", ch, n );
            cwrites( buf );
        }
    }

    // all done!
    return( 0 );
}
