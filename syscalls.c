/*
** SCCS ID:        @(#)syscalls.c	1.1        3/30/20
**
** File:    syscalls.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description:  System call implementations
*/

#define __SP_KERNEL__

#include "common.h"

#include <x86arch.h>
#include <x86pic.h>
#include <uart.h>

#include <fs.h>

#include "support.h"
#include "klib.h"

#include "syscalls.h"
#include "scheduler.h"
#include "process.h"
#include "stacks.h"
#include "clock.h"
#include "cio.h"
#include "sio.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

///
// system call jump table
//
// initialized by _sys_init() to ensure that the
// code::function mappings are correct
///

static void (*_syscalls[N_SYSCALLS])( uint32, uint32, uint32 );

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** _sys_isr(vector,code)
**
** Common handler for the system call module.  Selects
** the correct second-level routine to invoke based on
** the contents of EAX.
**
** The second-level routine is invoked with a pointer to
** the PCB for the process.  It is the responsibility of
** that routine to assign all return values for the call.
*/
static void _sys_isr( int vector, int code ) {

    // if there is no current process, we're in deep trouble
    assert( _current );

    // much less likely to occur, but still potentially problematic
    assert2( _current->context );

    // retrieve the arguments to the system call
    // (even if they aren't needed)
    uint32 arg1 = ARG(_current,1);
    uint32 arg2 = ARG(_current,2);
    uint32 arg3 = ARG(_current,3);

    // retrieve the code
    uint32 syscode = REG(_current,eax);

    // validate the code - if it's bad, "toodle-ooo, caribou!"
    if( syscode >= N_SYSCALLS ) {
        __sprint( b256, "PID %d bad syscall %d", _current->pid, syscode );
        WARNING( b256 );
        syscode = SYS_exit;
        arg1 = E_BAD_SYSCALL;
    }

    // handle the system call
    _syscalls[syscode]( arg1, arg2, arg3 );

    // tell the PIC we're done
    __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** Second-level syscall handlers
**
** All have this prototype:
**
**    static void _sys_NAME( uint32 arg1, uint32 arg2, uint32 arg3 );
**
** Values being returned to the user are placed into the EAX
** field in the context save area of the current PCB.
*/

/*
** _sys_exit - terminate the calling process
**
** implements:
**    void exit( int32 status );
*/
static void _sys_exit( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    
    // first, verify that we have a parent
    Pcb *parent = _pcb_find( _current->ppid );
    assert1( parent );
    
    // record the exit status for this process
    _current->exit_status = (int32) arg1;

    // perform all the nasty parts of this mechanism
    _really_exit( _current, parent, (int32) arg1 );
    
    // we need a new current process
    _dispatch();
}   

/*
** _sys_kill - terminate a process with extreme prejudice
**
** implements:
**    int32 kill( Pid pid );
**
** notes:
**    - interprets PID 0 as an alias for this process itself
*/
static void _sys_kill( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    Pid id = (Pid) arg1;
    Pcb *parent, *victim;
    
    // special case:  kill(0) --> suicide!
    if( id == 0 || id == _current->pid ) {
        parent = _pcb_find( _current->pid );
        assert1( parent );  // shouldn't be a problem, but just in case...
        _really_exit( _current, parent, E_KILLED );
        // no current process, so we need to pick another
        _dispatch();
        return;
    }

    // don't let anyone kneecap the system

    if( id == _init_pid || id == _idle_pid ) {
        RET(_current) = E_INVALID;
        return;
    }
    
    // we're after someone else - see if we can find them

    victim = _pcb_find( id );

    if( victim == NULL ) {
        RET(_current) = E_NOT_FOUND;
        return;
    }

    // OK, we found the victim; how to "kill" it?  Options:
    //
    //  - change its state to KILLED, catch it and clean it up the next
    //    time it's scheduled/dispatched/pulled from a queue/etc.
    //
    //  - remove it from whatever queue it's in, modify it so that it is
    //    dispatched into exit_helper() with a special termination status
    //    in EAX, and schedule it
    //
    //  - remove it from whatever queue it's in and deal with it as if
    //    it had called exit(KILLED) w/o letting it execute again
    //
    // We opt for the third approach.

    // remove it from whatever queue it happens to be on
    assert( _queue_remove(victim->queue,victim) == victim );

    // Locate the parent
    parent = _pcb_find( victim->ppid );
    assert1( parent );

    // get rid of the victim
    _really_exit( victim, parent, E_KILLED );

    // let the caller know
    RET(_current) = 0;
}

/*
** _sys_wait - wait for a child process to terminate
**
** implements:
**    int32 wait( Pid pid, int32 *status );
**
** notes:
**    - interprets PID 0 to mean "wait for any child"
*/
static void _sys_wait( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    Pid pid = (Pid) arg1;
    
    // if no children, nobody to wait for!
    if( _current->children < 1 ) {
        RET(_current) = E_NO_CHILDREN;
        return;
    }
    
    // can't wait for ourselves
    if( pid == _current->pid ) {
        RET(_current) = E_INVALID;
        return;
    }
    
    // OK, we're waiting for someone.  Figure out who(m?).

    Pcb *pcb;
    
    if( pid != 0 ) {

        // looking for a specific child
        pcb = _pcb_find( pid );
        if( pcb == NULL ) {
            RET(_current) = E_NOT_FOUND;
            return;
        }

        // found the process; is it ours?
        if( pcb->ppid != _current->pid ) {
            RET(_current) = E_INVALID;
            return;
        }

        // yes - is it ready to be reaped?
        if( pcb->state != ZOMBIE ) {
            // no, so we'll have to block for now
            pcb = NULL;
        }

    } else {
        int i;

        // waiting for any of our children - locate one that has exited
        for( i = 0; i < N_PROCS; ++i ) {
            // must be our child and must have already exited
            if( _pcbs[i].ppid == _current->pid &&
                _pcbs[i].state == ZOMBIE ) {
                break;
            }
        }

        // did we find one?
        if( i >= N_PROCS ) {
            // no, so we'll block
            pcb = NULL;
        } else {
            // yes!
            pcb = &_pcbs[i];
        }
    }
    
    // were we successful?
    if( pcb == NULL ) {

        // no - we need to block
        _current->state = WAITING;
        _current->queue = _waiting;
        assert( _queue_enque(_waiting,(void *)_current) == SUCCESS );

        // we were the current process, so we need a new one
        _dispatch();
        return;
    }

    // OK, we found an exited child; it must be a zombie, so
    // pull it from that queue

    assert( _queue_remove(_zombie,pcb) == pcb );
    
    // the parent gets its PID
    RET(_current) = pcb->pid;

    // and also its termination status, if that was requested
    int32 *status = (int32 *) arg2;
    if( status != NULL ) {
        *status = pcb->exit_status;
    }
    
    // one fewer child
    _current->children -= 1;
        
    // get rid of the evidence
    _proc_cleanup( pcb );
}

/*
** _sys_spawn - spawn a new child process
**
** implements:  int spawn( (*entry)(int,char*), char *argv[] );
*/
static void _sys_spawn( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    Pcb *pcb = _pcb_alloc();
    if( !pcb ) {
        RET(_current) = E_MAX_PROCS;
        return;
    }

    Stack *stk = _stk_alloc();
    if( !stk ) {
        _pcb_free( pcb );
        RET(_current) = E_NO_MEMORY;
        return;
    }

    int result = _proc_create( pcb, stk, _current, arg1, (char **) arg2 );
    if( result <= 0 ) {
        _pcb_free( pcb );
        _stk_free( stk );
    } else {
        _schedule( pcb );
        _active += 1;
    }

    RET(_current) = (uint32) result;
}

/*
** _sys_read - read data from an input channel
**
** implements:  int read( int chan, void *buf, uint32 len );
*/
static void _sys_read( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    int n = 0;
    int chan = arg1;
    char *buf = (char *) arg2;
    uint32 length = arg3;
    fmode_t mode;

    // get the mode bits
    mode = _fs_getmode(chan);

    // try to get the next character
    switch( chan ) {
    case CHAN_CONS:
        // console input doesn't block
        if( __cio_input_queue() < 1 ) {
            RET(_current) = E_NO_DATA;
            return;
        }
        n = __cio_gets( buf, length );
        break;

    case CHAN_SIO:
        // this may block the process; if so, _sio_reads() will dispatch
        n = _sio_reads( buf, length );
        break;

    default:
	n = _fs_read( chan, buf, length );
    }

    // if there was data, return the byte count to the process;
    // otherwise, block the process until data is available (if
    // file is opened with FILE_MODE_BLOCK).
    if( n > 0 || ( mode & FILE_MODE_BLOCK ) == 0 ) {

        RET(_current) = n;

    } else {

        // mark it as blocked
        _current->state = BLOCKED;

        // put this process on the serial i/o input queue
        _current->queue = _reading;
        assert( _queue_enque(_reading,(void *)_current) == SUCCESS );

        // select a new current process
        _dispatch();
    }
}

/*
** _sys_write - write data to an output channel
**
** implements:
**    int write( int chan, const void *buf, uint32 len );
*/
static void _sys_write( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    int chan = arg1;
    const char *buf = (const char *) arg2;
    int length = arg3;
    int written;

    // this is almost insanely simple, but it does separate the
    // low-level device access fromm the higher-level syscall implementation

    switch( chan ) {
    case CHAN_CONS:
        __cio_write( buf, length );
        RET(_current) = length;
        break;

    case CHAN_SIO:
        _sio_write( buf, length );
        RET(_current) = length;
        break;

    default:
	written = _fs_write( chan, buf, length );
        RET(_current) = written;
        break;
    }
}

/*
** _sys_sleep - put the current process to sleep
**
** implements:  void sleep( uint32 ms );
**
** notes:
**    - interprets a sleep time of 0 as a yield() request
**    - maximum sleep request is 49d 17h 2m 47s 295ms
*/
static void _sys_sleep( uint32 arg1, uint32 arg2, uint32 arg3 ) {

    // see if we just want to yield() the CPU
    if( arg1 == 0 ) {
        _schedule( _current );
        _dispatch();
        return;
    }

    // calculate the wakeup time
    _current->wakeup = _system_time + MS_TO_TICKS(arg1);

    // add the current process to the sleep queue
    if( _queue_enque(_sleeping,(void *)_current) != SUCCESS ) {

        // oops - something went wrong
        __sprint( b256, "PID %d can't put to sleep", _current->pid );
        WARNING( b256 );

        // just schedule it instead
        _schedule( _current );
    }

    // successfully inserted, so remember where it is
    _current->queue = _sleeping;

    // pick the next lucky contestant
    _dispatch();
}

/*
** _sys_gettime - retrieve the current system time
**
** implements:  Time gettime( void );
**
** returns:
**    the current system time
**
** notes:
**    - system time is a uint64, so we need to return the two 32-bit
**      halves as %edx:%eax
*/
static void _sys_gettime( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    REG(_current,eax) = (uint32) (_system_time & UI64_LOWER);
    REG(_current,edx) = (uint32) ((_system_time >> 32) & UI64_LOWER);
}

/*
** _sys_getpid - retrieve the PID of the current process
**
** implements:  Pid getpid( void );
**
** returns:
**    the PID of this process
*/
static void _sys_getpid( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    RET(_current) = _current->pid;
}

/*
** _sys_getppid - retrieve the PID of parent of the current process
**
** implements:  Pid getppid( void );
**
** returns:
**    the PID of the parent process
*/
static void _sys_getppid( uint32 arg1, uint32 arg2, uint32 arg3 ) {
    RET(_current) = _current->ppid;
}

/*
** _sys_getstate - retrieve the state of the indicated process
**
** implements:  State getstate( Pid pid );
**
** returns:
**    the state of the indicated process
**
** notes:
**    - pid 0 is a shorthand for "the current process"
*/
static void _sys_getstate( uint32 arg1, uint32 arg2, uint32 arg3 ) {

    // is the user process checking itself?
    if( arg1 == 0 || arg1 == _current->pid ) {
        RET(_current) = _current->state;
        return;
    }

    // no - locate the indicated process
    Pcb *pcb = _pcb_find( arg1 );
    if( pcb == NULL ) {
        // there wasn't one, so we return "UNUSED" as the state
        RET(_current) = UNUSED;
        return;
    }

    // found it!
    RET(_current) = pcb->state;
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _really_exit - do the real work for exit() and some kill() calls
**
** @param victim  Pointer to the PCB that should exit
** @param parent  Pointer to the victim's parent
** @param status  Termination status
*/
void _really_exit( Pcb *victim, Pcb *parent, int32 status ) {
    
    // reparent all the children of this process
    Pid us = victim->pid;
    int n = 0;
    for( int i = 0; i < N_PROCS; ++i ) {
        // if (A) this is one of this process' children, and
        //    (B) it's an active process,
        // hand it off to 'init'
        if( _pcbs[i].ppid == us && _pcbs[i].state != UNUSED ) {
            _pcbs[i].ppid = _init_pid;
            _init_pcb->children += 1;
            ++n;
        }
    }
    
    // verify that we found the correct number of children
    assert1( n == victim->children );

    // if we didn't panic, make sure we at least warn someone
    if( n != victim->children ) {
        __sprint( b256, "found %d kids, expected %d", n, victim->children );
        WARNING( b256 );
    }
    
    // if the parent isn't currently waiting, this
    // process becomes a zombie
    if( parent->state != WAITING ) {

        victim->state = ZOMBIE;
        victim->queue = _zombie;

        // failure to enque is a Bad Thing(tm)
        assert( _queue_enque(_zombie,(void *)victim) == SUCCESS );
        return;
    }
        
    // OK, we know that the parent is currently wait()ing

    // remove it from the waiting queue; if we can't, we're in trouble
    assert( _queue_remove(_waiting,parent) == parent );
    
    // give the parent this process' PID
    RET(parent) = victim->pid;

    // if the parent wants it, also return the exit status
    int32 *sval = (int32 *) ARG(parent,1);
    if( sval != NULL ) {
        *sval = victim->exit_status;  // exit status
    }
    
    // one fewer child for the parent
    parent->children -= 1;
    
    // parent is no longer waiting
    _schedule( parent );
    
    // all done with this process
    _proc_cleanup( victim );
}

/*
** _sys_init()
**
** initialize the syscall module
**
** MUST BE CALLED AFTER THE _sio_init FUNCTION HAS BEEN CALLED,
** SO THAT THE _reading QUEUE HAS BEEN CREATED.
*/
void _sys_init( void ) {

    ///
    // Set up the syscall jump table.  We do this here
    // to ensure that the association between syscall
    // code and function address is correct even if the
    // codes change.
    ///

    _syscalls[ SYS_exit ]      = _sys_exit;
    _syscalls[ SYS_kill ]      = _sys_kill;
    _syscalls[ SYS_wait ]      = _sys_wait;
    _syscalls[ SYS_spawn ]     = _sys_spawn;
    _syscalls[ SYS_read ]      = _sys_read;
    _syscalls[ SYS_write ]     = _sys_write;
    _syscalls[ SYS_sleep ]     = _sys_sleep;
    _syscalls[ SYS_gettime ]   = _sys_gettime;
    _syscalls[ SYS_getpid ]    = _sys_getpid;
    _syscalls[ SYS_getppid ]   = _sys_getppid;
    _syscalls[ SYS_getstate ]  = _sys_getstate;

    // install the second-stage ISR
    __install_isr( INT_VEC_SYSCALL, _sys_isr );

    // report that we made it this far
    __cio_puts( " SYSCALL" );
}
