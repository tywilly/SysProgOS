/*
** Initial process; it starts the other top-level user processes.
**
** Prints a message at startup, '+' after each user process is spawned,
** and '!' before transitioning to wait() mode to the SIO, and
** startup and transition messages to the console.  It also reports
** each child process it collects via wait() to the console along
** with that child's exit status.
*/

/*
** For the test programs in the baseline system, command-line arguments
** follow the following rules.  The first two entries are fixed:
**
**      argv[0] is the "command name" (main1, userW, etc.)
**      argv[1] is the "character to print" (identifies the process)
**
** Remaining entries vary depending on the desired behavior of the process:
**
**  For most processes:
**
**      argv[2] is usually an iteration or child count
**      argv[3] is usually a sleep time, in seconds
**
** See the comment at the beginning of each user-code source file for
** information on the argument list that code expects.
**
** Currently, the longest argument list is for main6(), which will
** take up to four arguments on the command line after the command name.
** Should that change, update the users.h header file so that the CPP
** macro MAX_COMMAND_ARGS is large enough to have space for the command
** name, the maximul argument list, and a NULL pointer.
*/

int init( int argc, char *args ) {
    int32 whom;
    char ch = '+';
    static int invoked = 0;
    char buf[128];
    char *argv[MAX_COMMAND_ARGS];

    if( invoked > 0 ) {
        cwrites( "Init RESTARTED???\n" );
        for(;;);
    }

    cwrites( "Init started\n" );
    ++invoked;

    // home up, clear
    swritech( '\x1a' );
    // wait a bit
    DELAY(STD);

    // a bit of Dante to set the mood
    swrites( "\n\nSpem relinquunt qui huc intrasti!\n\n\r" );

    // clear the argument vector
    for( int i = 0; i < MAX_COMMAND_ARGS; ++i ) {
        argv[i] = NULL;
    }

    // set up for users A, B, and C initially
    argv[0] = "main1";
    // argv[1] will vary
    argv[2] = "30";

#ifdef SPAWN_A
    // "main1 A 30"
    argv[1] = "A";
    whom = spawn( main1, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user A failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_B
    // "main1 B 30"
    argv[1] = "B";
    whom = spawn( main1, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user B failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_C
    // "main1 C" (use the default iteration count)
    argv[1] = "C";
    argv[2] = NULL;
    whom = spawn( main1, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user C failed\n" );
    }
    swritech( ch );
#endif

    // User D is like A-C, but uses main2 instead

#ifdef SPAWN_D
    // "main2 D 10"
    argv[0] = "main2";
    argv[1] = "D";
    argv[2] = "10";
    whom = spawn( main2, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user D failed\n" );
    }
    swritech( ch );
#endif

    // Users E, F, and G behave the same way: report, sleep, exit

    argv[0] = "main3";  // main routine
    // argv[1] will be replaced for each process
    argv[2] = "15";     // default iteration count
    // argv[3] will be replaced for each process
    argv[4] = NULL;

#ifdef SPAWN_E
    // "main3 E 15 5"
    argv[1] = "E";
    argv[3] = "5";
    whom = spawn( main3, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user E failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_F
    // "main3 F 15 10"
    argv[1] = "F";
    argv[3] = "10";
    whom = spawn( main3, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user F failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_G
    // "main3 G 15 15"
    argv[1] = "G";
    argv[3] = "15";
    whom = spawn( main3, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user G failed\n" );
    }
    swritech( ch );
#endif

    // User H tests reparenting of orphaned children
#ifdef SPAWN_H
    // "userH H"
    argv[0] = "userH";
    argv[1] = "H";
    argv[2] = NULL;
    whom = spawn( userH, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user H failed\n" );
    }
    swritech( ch );
#endif

    // there is no User I

    // User J tries to spawn 2 * N_PROCS children

#ifdef SPAWN_J
    // "userJ J"
    argv[0] = "userJ";
    argv[1] = "J";
    argv[2] = NULL;
    whom = spawn( user%, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user J failed\n" );
    }
    swritech( ch );
#endif

    // Users K and L iterate spawning copies of userX and sleeping
    // for varying amounts of time.

    argv[0] = "main4";
    // argv[1] - argv[2] will be filled in later
    argv[3] = NULL;

#ifdef SPAWN_K
    // "main5 K 17"
    argv[1] = "K";
    argv[2] = "17";
    whom = spawn( main5, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user K failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_L
    // "main5 L 31"
    argv[1] = "L";
    argv[2] = "31";
    whom = spawn( main5, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user L failed\n" );
    }
    swritech( ch );
#endif

    // Users M and N spawn copies of userW and userZ
    argv[0] = "main5";

#ifdef SPAWN_M
    // "main5 M 5"
    argv[1] = "M";
    argv[2] = "5";
    argv[3] = NULL;   // don't spawn userZ
    whom = spawn( main5, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user M failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_N
    // "main5 N 5 both"
    argv[1] = "N";
    argv[2] = "5";
    argv[3] = "both";   // spawn userZ along with userW
    argv[4] = NULL;
    whom = spawn( main5, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user N failed\n" );
    }
    swritech( ch );
#endif

    // there is no user_o

    // User P iterates, reporting system time and sleeping

#ifdef SPAWN_P
    // "userP 3 2"
    argv[0] = "userP";
    argv[1] = "3";
    argv[2] = "2";
    argv[3] = NULL;
    whom = spawn( userP, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user P failed\n" );
    }
    swritech( ch );
#endif

    // User Q tries to execute a bad system call

#ifdef SPAWN_Q
    // "userQ Q"
    argv[0] = "userQ";
    argv[1] = "Q";
    argv[2] = NULL;
    whom = spawn( userQ, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user Q failed\n" );
    }
    swritech( ch );
#endif

    // User R reads from the SIO one character at a time, forever

#ifdef SPAWN_R
    // "userR 10"
    argv[0] = "userR";
    argv[1] = "10";
    argv[2] = NULL;
    whom = spawn( userR, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user R failed\n" );
    }
    swritech( ch );
#endif

    // User S loops forever, sleeping on each iteration

#ifdef SPAWN_S
    // "userS 20"
    argv[0] = "userS";
    argv[1] = "20";
    argv[2] = NULL;
    whom = spawn( userS, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user S failed\n" );
    }
    swritech( ch );
#endif

    // Users T, U, and V run main6(); they spawn copies of userW,
    // then wait for them all or kill them all

#ifdef SPAWN_T
    // User T:  wait for any child each time
    // "main6 T"
    // we rely on the default parameter values for args 2 and 3
    argv[0] = "userT";
    argv[1] = "T";
    // default number of children, default behavior
    // --> five children, wait for any child
    argv[2] = NULL;
    whom = spawn( main6, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user T failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_U
    // User U:  wait for children by PID
    // "main6 U 6 wait pid"
    argv[0] = "userU";
    argv[1] = "U";
    argv[2] = "6";      // children
    argv[3] = "wait";   // wait
    argv[4] = "pid";    // wait by PID
    argv[5] = NULL;
    whom = spawn( main6, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user U failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_V
    // User V:  kill all children
    // "main6 V 6 kill"
    argv[0] = "userV";
    argv[1] = "V";
    argv[2] = "6";      // children
    argv[3] = "kill";   // not just waiting
    // "kill" is always by PID, so no fourth argument necessary
    argv[4] = NULL;
    whom = spawn( main6, argv );
    if( whom < 0 ) {
        cwrites( "init, spawn() user V failed\n" );
    }
    swritech( ch );
#endif

    // Users W through Z are spawned elsewhere

    swrites( "!\r\n\n" );

    /*
    ** At this point, we go into an infinite loop waiting
    ** for our children (direct, or inherited) to exit.
    */

    cwrites( "init() transitioning to wait() mode\n" );

    for(;;) {
        int32 status;
        Pid whom = wait( 0, &status );

        if( whom == E_NO_CHILDREN ) {
            cwrites( "INIT: wait() says 'no children'???\n" );
            continue;
        } else if( whom > 0 ) {
            sprint( buf, "INIT: pid %d exited, status %d\n", whom, status );
            cwrites( buf );
        } else {
            sprint( buf, "INIT: wait() status %d\n", whom );
            cwrites( buf );
        }
    }

    /*
    ** SHOULD NEVER REACH HERE
    */

    cwrites( "*** INIT IS EXITING???\n" );
    exit( 1 );

    return( 0 );  // shut the compiler up!
}
