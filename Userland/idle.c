/*
** Idle function
**
** Reports itself, then loops forever delaying and printing a character.
**
** Invoked as:  idle
*/

int idle( int argc, char *args ) {
    int32 me;
    uint32 now;
    char buf[128];
    char ch = '.';
    
    me = getpid();
    now = gettime();

    sprint( buf, "Idle [%d] started at %d\n", me, now );
    cwrites( buf );

    write( CHAN_SIO, &ch, 1 );

    // idle() should never block - it must always be available
    // for dispatching when we need to pick a new current process

    for(;;) {
        DELAY(LONG);
        write( CHAN_SIO, &ch, 1 );
    }

    // we should never reach this point!

    now = gettime();
    sprint( buf, "+++ Idle done at %d !?!?!\n", now );
    cwrites( buf );

    exit( 1 );

    return( 42 );  // shut the compiler up!
}
