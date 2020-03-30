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
    char ch = '6'     // default character to print
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
