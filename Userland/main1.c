/*
** User function #1:  write, exit
**
** Prints its ID, then loops N times delaying and printing, then exits.
** Verifies the return byte count from each call to write().
**
** Invoked as:  main1 [ x [ n ] ]
**   where x is the ID character (defaults to '1')
**         n is the iteration count (defaults to 30)
*/

int main1( int argc, char *args ) {
    int n;
    int count = 30;   // default iteration count
    char ch = '1'     // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( argc > 2 ) {    // "main1 x n"
        count = str2int( argv[2], 10 );
    }

    if( argc > 1 ) {    // "main1 x"
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
    exit( 0 );

    // should never reach this code; if we do, something is
    // wrong with exit(), so we'll report it

    char msg[] = "*1*";
    msg[1] = ch;
    n = write( CHAN_SIO, msg, 3 );    /* shouldn't happen! */
    if( n != 3 ) {
        sprint( buf, "User %c, write #3 returned %d\n", ch, n );
        cwrites( buf );
    }

    // this should really get us out of here
    return( 42 );
}
