/*
** SCCS ID: @(#)clock.c	1.1 3/30/20
**
** File:    clock.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Clock module implementation.
*/

#define __SP_KERNEL__

#include <x86arch.h>
#include <x86pic.h>
#include <x86pit.h>

#include "common.h"
#include "klib.h"

#include "clock.h"
#include "process.h"
#include "queues.h"
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

// pinwheel control

static uint32 _pinwheel;   // pinwheel counter
static uint32 _pindex;     // index into pinwheel string

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

//
// _clk_isr() - the clock ISR
//
// Interrupt handler for the clock module.  Spins the pinwheel, wakes up
// sleeping processes, and handles quantum expiration for the current
// process.
//
static void _clk_isr( int vector, int ecode ) {

    // spin the pinwheel

    ++_pinwheel;
    if( _pinwheel == (CLOCK_FREQUENCY / 10) ) {
        _pinwheel = 0;
        ++_pindex;
        __cio_putchar_at( 0, 0, "|/-\\"[ _pindex & 3 ] );
    }

#if defined(STATUS)
    // Periodically, dump the queue lengths and the SIO status (along
    // with the SIO buffers, if non-empty).

    if( (_system_time % SEC_TO_TICKS(STATUS)) == 0 ) {
        __cio_printf_at( 3, 0,
            "%3d procs:  sl/%d wt/%d rd/%d zo/%d  r %d     ",
                _active_procs,
                _queue_length(_sleeping), _queue_length(_waiting),
                _queue_length(_reading), _queue_length(_zombie),
                _queue_length(_ready)
        );
        _sio_dump( true );
        // _active_dump( "Ptbl", false );
    }
#endif

    // time marches on

    ++_system_time;

    // wake up any sleeping processes whose time has come
    //
    // we give them preference over the current process
    // (when it is scheduled again)

    Pcb *pcb = _queue_front( _sleeping );
    while( pcb != NULL && pcb->wakeup <= _system_time ) {

        // time to wake this one up
        pcb = _queue_deque( _sleeping );

        // put it on the ready queue
        _schedule( pcb );

        // peek at the next one
        pcb = _queue_front( _sleeping );
    }

    // check the current process to see if its time slice has expired
    _current->quantum -= 1;
    if( _current->quantum < 1 ) {
        // yes!  however, if it's idle(),
        // we don't want to schedule it
        if( _current != _idle_pcb ) {
            _schedule( _current );
        }
        _dispatch();
    }

    // tell the PIC we're done
    __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** PUBLIC FUNCTIONS
*/

//
// _clk_init() - initialize the clock module
//
void _clk_init( void ) {
    uint32 divisor;

    // start the pinwheel
    _pinwheel = (CLOCK_FREQUENCY / 10) - 1;
    _pindex = 0;

    // return to the epoch
    _system_time = 0;

    // configure the clock
    divisor = TIMER_FREQUENCY / CLOCK_FREQUENCY;
    __outb( TIMER_CONTROL_PORT, TIMER_0_LOAD | TIMER_0_SQUARE );
    __outb( TIMER_0_PORT, divisor & 0xff );        // LSB of divisor
    __outb( TIMER_0_PORT, (divisor >> 8) & 0xff ); // MSB of divisor

    // register the ISR
    __install_isr( INT_VEC_TIMER, _clk_isr );

    // report that we're all set
    __cio_puts( " CLOCK" );
}
