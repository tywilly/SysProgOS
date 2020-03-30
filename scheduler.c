/*
** SCCS ID: %W% %G%
**
** File:    scheduler.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Implementation of the scheduler.
*/

#define __SP_KERNEL__

#include "common.h"

#include "scheduler.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

//
// _sched_init() - initialize the scheduler module
//
void _sched_init( void ) {

    // create the ready queue
    _ready = _queue_alloc( NULL );

    // if we can't, life is not worth living
    assert( _ready );

    // no current process yet
    _current = NULL;

    // announce that we're ready
    __cio_puts( " SCHED" );
}

//
// _schedule() - the scheduler
//
// @param pcb   The process to be scheduled
//
void _schedule( Pcb *pcb ) {

    // avoid scheduling nothing
    assert1( pcb );

    // state transition
    pcb->state = READY;

    // queue membership
    pcb->queue = _ready;

    // add it to the collection; failure is not an option!
    assert( _queue_enque(_ready,(void *)pcb) == SUCCESS );
}

//
// _dispatch() - give the CPU to a process
//
void _dispatch( void ) {

#ifdef CONSOLE_SHELL
    // check to see if there is any console input
    if( __cio_input_queue() > 0 ) {
        // yes - deal with it
        _shell( 'h' );
    }
#endif

    // if there isn't anyone waiting to run, stop
    // and smell the roses until something happens

    // while( _queue_length(_ready) == 0 ) {
        // __cio_puts( "_dispatch: pausing\n" );
        // __pause();
    // }

    if( _queue_length(_ready) == 0 ) {
        _current = _idle_pcb;
    } else {
        // dispatch the first thing on the queue
        _current = (Pcb *) _queue_deque( _ready );
    }

    // if that failed, we have a serious problem
    assert( _current );

    // all's well; let this process loose on the world
    _current->queue = NULL;
    _current->state = RUNNING;
    _current->quantum = QUANTUM_STD;
}
