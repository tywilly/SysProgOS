/*
** SCCS ID:	@(#)users.c	1.1	3/30/20
**
** File:	users.c
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	User-level code.
*/

#include "common.h"
#include "users.h"
#include "userSB.h"
#include "ac97.h"

/*
** USER PROCESSES
**
** Each is designed to test some facility of the OS; see the users.h
** header file for a summary of which system calls are tested by
** each user function.
**
** Output from user processes is usually alphabetic.  Uppercase
** characters are "expected" output; lowercase are "erroneous"
** output.
**
** More specific information about each user process can be found in
** the header comment for that function (below).
**
** To spawn a specific user process, uncomment its SPAWN_x
** definition in the users.h header file.
*/

/*
** Prototypes for all user main routines (even ones that may not exist,
** for completeness)
*/

int idle( int, char * );

int main1( int, char * ); int main2( int, char * ); int main3( int, char * );
int main4( int, char * ); int main5( int, char * ); int main6( int, char * );

int userA( int, char * ); int userB( int, char * ); int userC( int, char * );
int userD( int, char * ); int userE( int, char * ); int userF( int, char * );
int userG( int, char * ); int userH( int, char * ); int userI( int, char * );
int userJ( int, char * ); int userK( int, char * ); int userL( int, char * );
int userM( int, char * ); int userN( int, char * ); int userO( int, char * );
int userP( int, char * ); int userQ( int, char * ); int userR( int, char * );
int userS( int, char * ); int userT( int, char * ); int userU( int, char * );
int userV( int, char * ); int userW( int, char * ); int userX( int, char * );
int userY( int, char * ); int userZ( int, char * );

int startsound( int, char * );

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
    char ch = '1';    // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "main1 x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "main1 x"
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
    char ch = '2';    // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "main2 x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "main2 x"
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

/*
** User function #3:  write, sleep, exit
**
** Prints its ID, then loops N times sleeping and printing, then exits.
**
** Invoked as:  main3 [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to '3')
**         n is the iteration count (defaults to 10)
**         s is the sleep time in seconds (defaults to 5)
*/

int main3( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = '3';    // default character to print
    int nap = 5;      // default sleep time
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "main3 x n s"
        nap = str2int( argv[3], 10 );
    }

    if( n > 2 ) {    // "main3 x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "main3 x"
        ch = argv[1][0];
    }

    // announce our presence a little differently
    report( ch, getpid() );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < count ; ++i ) {
        sleep( SEC_TO_MS(nap) );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

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
    char ch = '4';    // default character to print
    int nap = 30;     // nap time
    char buf[128];
    char msg2[] = "*4*";
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "main4 x n s"
        nap = str2int( argv[3], 10 );
    }

    if( n > 2 ) {    // "main4 x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "main4 x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    msg2[1] = ch;

    // create the argument vector for userX
    argv[0] = "userX";
    argv[1] = buf;
    argv[2] = NULL;

    Pid me = getpid();

    for( int i = 0; i < count ; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        sprint( buf, "%d", ((int) me << 4) + i );
        int whom = spawn( userX, argv );
        if( whom < 0 ) {
            swrites( msg2 );
        }
        sleep( SEC_TO_MS(nap) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

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
    char ch = '5';  // default character to print
    int alsoZ = 0;  // also do userZ?
    char buf[128];
    char msg2[] = "*5*";
    char *argv[MAX_COMMAND_ARGS] = { NULL };
    char *argv2[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "main5 x n s"
        // we have a third argument, therefore we're doing both
        alsoZ = 1;
    }

    if( n > 2 ) {    // "main5 x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "main5 x"
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
    argv[4] = NULL;

    if( alsoZ ) {
        argv2[0] = "userZ";
        argv2[1] = "Z";
        argv2[2] = "15";
        argv2[3] = NULL;
    }

    for( int i = 0; i < count; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        int32 whom = spawn( userW, argv );
        if( whom < 1 ) {
            swrites( msg2 );
        } else {
            sprint( buf, "User %c spawned W, PID %d\n", ch, whom );
            cwrites( buf );
        }
        if( alsoZ ) {
            int32 whom = spawn( userZ, argv2 );
            if( whom < 1 ) {
                swrites( msg2 );
            } else {
                sprint( buf, "User %c spawned Z, PID %d\n", ch, whom );
                cwrites( buf );
            }
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function main6:  write, spawn, sleep, wait, getpid, kill
**
** Reports, then loops spawing userW, sleeps, then waits for or kills
** all its children.
**
** Invoked as:  main6 [ x [ n [ w [ h ] ] ] ]
**   where x is the ID character (defaults to '6')
**         n the child count (defaults to 3)
**         w indicates waiting or killing (defaults to "wait")
**         h indicates how to wait (defaults to "any")
*/

#define MAX_CHILDREN    50

int main6( int argc, char *args ) {
    int count = 3;    // default child count
    char ch = '6';    // default character to print
    int nap = 8;      // nap time
    char what = '?';  // wait or kill?  default is wait
    char how  = '?';  // wait/kill any child, or by PID?  default is any
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };
    Pid children[MAX_CHILDREN];
    int nkids = 0;

    // parse our command-line string
    int n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    switch( n ) {

    case 5: // main6 x n w h
        // must be waiting
        what = 'w';
        // how to wait:  'p' --> by pid, else any
        how = argv[4][0];
        // FALL THROUGH

    case 4: // main6 x n w
        // wait or kill:  'k' --> kill, else wait
        if( what == '?' ) {
            what = argv[3][0];
        }
        if( what == 'k' ) {
            how = 'p'; // "kill" must be by PID
        }
        // FALL THROUGH

    case 3: // main6 x n
        count = str2int( argv[2], 10 );
        if( count > MAX_CHILDREN ) {
            sprint( buf, "User %c PID %d, too many children (%d)\n",
                    ch, getpid(), count );
            cwrites( buf );
            count = MAX_CHILDREN;
        }
        // FALL THROUGH

    case 2: // main6 x
        ch = argv[1][0];
        break;

    case 1: // main6
        // just use defaults
        break;

    case 0: // nothing, or more than four arguments
        sprint( buf, "main6: bad args '%s'\n", args );
        cwrites( buf );
        exit( 1 );
    }

    // check to be sure we know what we're doing
    if( what == '?' ) {
        // no 'w' or 'h' parameters, so we are waiting for any child
        what = 'w';
        how = 'a';
    }

    if( how == '?' ) {
        // 'w' was given, but 'h' wasn't
        how = what == 'k' ? 'p' : 'a';
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
            if( how == 'p' ) {
                children[nkids++] = whom;
            }
        }
    }

    // let the children start
    sleep( SEC_TO_MS(nap) );

    // collect exit status information

    do {
        int32 this;
        int32 status;

        if( how == 'p' ) {

            // processing by PID - do we have any more?
            if( nkids < 1 ) {
                // all done!
                break;
            }

            // adjust our index value
            --nkids;

            // make sure this entry is valid
            if( children[nkids] < 1 ) {
                continue;
            }

            // are we waiting for or killing it?
            if( what == 'w' ) {
                this = wait( children[nkids], &status );
            } else {
                this = kill( children[nkids] );
            }

            // what was the result?
            if( this < 0 ) {
                // uh-oh - something went wrong
                // "no children" means we're all done
                if( this != E_NO_CHILDREN ) {
                    if( what == 'w' ) {
                        sprint( buf, "User %c: wait() status %d\n", ch, this );
                    } else {
                        sprint( buf, "User %c: kill() status %d\n", ch, this );
                    }
                    cwrites( buf );
                }
                // regardless, we're outta here
                break;
            }

        } else {

            // waiting for any child
            this = wait( 0, &status );

            // what was the result?
            if( this < 0 ) {
                // uh-oh - something went wrong
                // "no children" means we're all done
                if( this != E_NO_CHILDREN ) {
                    sprint( buf, "User %c: wait() status %d\n", ch, this );
                }
                cwrites( buf );
            }
            // regardless, we're outta here
            break;
        }

        sprint( buf, "User %c: child %d status %d\n", ch, this, status );
        cwrites( buf );

    } while( 1 );

    if( how == 'p' && nkids > 0 ) {
        sprint( buf, "User %c: w/k by PID, %d left over???\n", ch, nkids );
        cwrites( buf );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

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
    char ch = 'h';    // default character to print
    char buf[128];
    int32 whom;
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userH x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userH x"
        ch = argv[1][0];
    }

    // announce our presence
    swritech( ch );

    // we spawn user Z and then exit before it can terminate
    argv[0] = "userZ";  // main()
    argv[1] = "Z";      // ID character
    argv[2] = "10";     // iteration count
    argv[3] = NULL;

    for( int i = 0; i < count; ++i ) {

        // spawn a child
        whom = spawn( userZ, argv );

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
    char ch = 'i';    // default character to print
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
    int32 status = kill( children[1] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, children[1], status );
        cwrites( buf );
        children[1] = -42;
    }
    status = kill( children[3] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, children[3], status );
        cwrites( buf );
        children[3] = -42;
    }

    // collect state information
    int num = 3;  // three trips through the loop
    while( num-- ) {
        State state = getstate( getpid() );
        sprint( buf, "User %c: my state %s\n", ch, getstate(state) );
        for( int i = 0; i < count; ++i ) {
            if( children[i] != -42 ) {
                sprint( buf, "User %c: child %d state %s\n", ch, children[i],
                        getstate(state) );
            }
        }
        sleep( SEC_TO_MS(nap) );
    };

    // let init() clean up after us!

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function J:  write, getpid, spawn, exit
**
** Reports, tries to spawn lots of children, then exits
**
** Invoked as:  userJ [ x [ n ] ]
**   where x is the ID character (defaults to '1')
**         n is the number of children to spawn (defaults to 2 * N_PROCS)
**         s is the sleep time (defaults to 30)
*/

int userJ( int argc, char *args ) {
    int n;
    int count = 2 * N_PROCS;   // number of children to spawn
    char ch = 'j';    // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userJ x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userJ x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the command-line arguments
    argv[0] = "userY";
    argv[1] = "Y";
    argv[2] = "10";
    argv[3] = buf;
    argv[4] = NULL;

    Pid me = getpid();

    for( int i = 0; i < count ; ++i ) {
        sprint( buf, "%d.%d", me, i );
        int32 whom = spawn( userY, argv );
        if( whom < 0 ) {
            write( CHAN_SIO, "!j!", 3 );
        } else {
            write( CHAN_SIO, &ch, 1 );
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function P:  write, gettime, sleep
**
** Reports itself, then loops reporting the current time
**
** Invoked as:  userP [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to 'p')
**         n is the iteration count (defaults to 3)
**         s is the sleep time (defaults to 2)
*/

int userP( int argc, char *args ) {
    int n;
    int count = 3;    // default iteration count
    char ch = 'p';    // default character to print
    int nap = 2;      // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "user? x n s"
        nap = str2int( argv[3], 10 );
    }

    if( n > 2 ) {    // "user? x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "user? x"
        ch = argv[1][0];
    }

    // announce our presence
    Time now = gettime();
    sprint( buf, "User %c running, start at %d\n", ch, now );
    cwrites( buf );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < count; ++i ) {
        sleep( SEC_TO_MS(nap) );
        now = gettime();
        sprint( buf, "User %c reporting time %d\n", ch, now );
        cwrites( buf );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function Q:  write, bogus, exit
**
** Reports itself, then tries to execute a bogus system call
**
** Invoked as:  userQ [ x ]
**   where x is the ID character (defaults to '1')
*/

int userQ( int argc, char *args ) {
    int n;
    char ch = 'q';    // default character to print
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    if( n > 1 ) {    // "userQ x"
        ch = argv[1][0];
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // try something weird
    bogus();

    // should not have come back here!
    sprint( buf, "User %c returned from bogus syscall!?!?!\n", ch );
    cwrites( buf );

    exit( 1 );

    return( 42 );  // shut the compiler up!
}

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
    char ch = 'r';    // default character to print
    int delay = 10;   // initial delay
    char buf[128];
    char b2[8];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userR x s"
        delay = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userR x"
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
    char ch = 's';    // default character to print
    int nap = 20;     // nap time
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userS x s"
        nap = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userS x"
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

/*
** User function W:  write, sleep, exit, getpid(), gettime()
**
** Reports its presence, then iterates 'n' times printing identifying
** information and sleeping, before exiting.
**
** Invoked as:  userW [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to '1')
**         n is the iteration count (defaults to 20)
**         s is the sleep time (defaults to 3)
*/

int userW( int argc, char *args ) {
    int n;
    int count = 20;   // default iteration count
    char ch = 'w';    // default character to print
    int nap = 3;      // nap length
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "userW x n s"
        nap = str2int( argv[3], 10 );
    }

    if( n > 2 ) {    // "userW x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userW x"
        ch = argv[1][0];
    }

    // announce our presence
    report( ch, getpid() );

    uint32 tlower = (uint32) (gettime() & UI64_LOWER);

    sprint( buf, " %c[%u] ", ch, tlower );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        sleep( SEC_TO_MS(nap) );
    }


    cwrites( buf );
    cwrites( " exiting\n" );

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function X:  write, exit
**
** Prints its PID at start and exit, iterates printing its character
** N times, and exits with a status equal to its PID.
**
** Invoked as:  userX [ x [ n ] ]
**   where x is the ID character (defaults to 'x')
**         n is a value to be used when printing our character
*/

int userX( int argc, char *args ) {
    int n;
    int count = 20;   // iteration count
    char ch = 'x';    // default character to print
    int status;
    int value = 17;   // default value
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userX x n"
        value = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userX x"
        ch = argv[1][0];
    }

    // announce our presence
    report( ch, status=getpid() );

    sprint( buf, " %c[%d] ", ch, value );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
    }

    cwrites( buf );
    cwrites( " exiting\n" );

    exit( status );

    return( 42 );  // shut the compiler up!
}

/*
** User function Y:  write, sleep, exit
**
** Reports its PID, then iterates N times printing its PID and
** sleeping for one second, then exits.
**
** Invoked as:  userY [ x [ n [ s ] ] ]
**   where x is the ID character (defaults to 'Y')
**         n is the iteration count (defaults to 10)
**         s is a sequence string (defaults to "0.0")
*/

int userY( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = 'y';    // default character to print
    char *seq = "0.0";      // sequence string
    char buf[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 3 ) {    // "userY x n s"
        seq = argv[3];
    }

    if( n > 2 ) {    // "userY x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userY x"
        ch = argv[1][0];
    }

    sprint( buf, " %c[%s] ", ch, seq );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
        sleep( SEC_TO_MS(1) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** User function Z:  write, getpid, getppid, sleep, exit
**
** Prints its ID, then records PID and PPID, loops printing its ID,
** and finally re-gets PPID for comparison.  Yields after every second
** ID print in the loop.
**
** This code is used as a handy "spawn me" test routine; it is spawned
** by several of the standard test processes.
**
** Invoked as:  userZ [ x [ n ] ]
**   where x is the ID character (defaults to '9')
**         n is the iteration count (defaults to 10)
*/

int userZ( int argc, char *args ) {
    int n;
    int count = 10;   // default iteration count
    char ch = 'z';    // default character to print
    char buf[128], buf2[128];
    char *argv[MAX_COMMAND_ARGS] = { NULL };

    // parse our command-line string
    n = parse_args( argc, args, MAX_COMMAND_ARGS, argv );

    // process the argument(s)

    if( n > 2 ) {    // "userZ x n"
        count = str2int( argv[2], 10 );
    }

    if( n > 1 ) {    // "userZ x"
        ch = argv[1][0];
    }

    // announce our presence
    swritech( ch );

    // record the relevant PIDs and report it to the console
    Pid me = getpid();
    Pid parent = getppid();

    sprint( buf, "%c[%u/%u]", ch, me, parent );

    sprint( buf2, "user %s running\n", buf );
    cwrites( buf2 );

    // iterate for a while; occasionally yield the CPU
    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
        if( i & 1 ) {
            sleep( 0 );
        }
    }

    // get "new" parent PID and report the result
    parent = getppid();

    sprint( buf2, "user %s exiting, parent now %d\n", buf, parent );
    cwrites( buf2 );

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

/*
** Play the Windows XP Startup Sound
*/
int startsound( int argc, char *args ) {
    int ch = '@';

    // TODO DCB wav support library
    char *pos = (char *) &_binary_winstart_wav_start + 44;
    char *end = (char *) &_binary_winstart_wav_end;
    while( pos < end ) {
        // play the song until you can't anymore
        int32 numwritten = write( CHAN_AC97, (void *) pos, end - pos );
        pos += numwritten;

        if ((uint32) gettime() % 250 == 0) {
            swritech(ch);
        }

        if (numwritten <= 1024) {
            sleep(0); // yield to let other stuff happen while it catches up
        }
    }

    return 0;
}

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

#ifdef STARTUP_SOUND
    // play the Windows XP startup sound
    argv[0] = NULL; // no arguments
    ac97_setvol(32);
    whom = spawn( startsound, argv );

    if( whom < 0 ) {
        cwrites( "init, spawn() user O failed\n" );
    }
    swritech( ch );
#endif

    // set up for users A, B, and C initially
    argv[0] = "main1";
    // argv[1] will vary
    argv[2] = "30";

#ifdef SPAWN_SB
    argv[1] = "SB";
    whom = spawn( mainSB, argv );
    if (whom < 0 ) {
        cwrites( "init, spawn() sound blaster failed.\n");
    }
    swritech( ch );
#endif

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
    whom = spawn( userJ, argv );
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

/*
** Idle process:  write, getpid, gettime, exit
**
** Reports itself, then loops forever delaying and printing a character.
**
** Invoked as:  idle
*/

int idle( int argc, char *args ) {
    int32 me;
    uint64 now;
    char buf[128];
    char ch = '.';

    me = getpid();
    now = gettime();

    sprint( buf, "Idle [%d] started at %d\n", me, (int32) now );
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
