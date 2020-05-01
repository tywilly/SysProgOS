/*
** SCCS ID: @(#)kernel.c	1.1 3/30/20
**
** File:    kernel.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor: Cody Burrows (cxb2114@rit.edu)
**              Zach Jones   (ztj3686@rit.edu)
**
** Description: Miscellaneous OS support routines.
*/

#define __SP_KERNEL__

#include "common.h"

#include "kernel.h"
#include "queues.h"
#include "clock.h"
#include "process.h"
#include "bootstrap.h"
#include "syscalls.h"
#include "cio.h"
#include "sio.h"
#include "scheduler.h"
#include "pci.h"
#include "soundblaster.h"
#include "ac97.h"
#include "usb.h"

// need init() and idle() addresses
#include "users.h"

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

// character buffers, usable throughout the OS
// not guaranteed to retain their contents across an exception return
char b256[256];
char b512[512];

// Current system time (managed by clock.c)
Time _system_time;

// Process-related data (managed by process.c and other modules)

// The current process
Pcb *_current;

// Information about the init() process
Pid _init_pid;
Pcb *_init_pcb;

// Information about the idle() process
Pid _idle_pid;
Pcb *_idle_pcb;

// Static array of PCBs
Pcb _pcbs[ N_PROCS ];

// Count of active processes
uint32 _active;

// PID tracker
Pid _next_pid;

// Queues (used by multiple modules)
Queue _waiting;   // processes waiting (for Godot?)
Queue _reading;   // processes blocked on input
Queue _zombie;    // gone, but not forgotten
Queue _sleeping;  // processes catching some Z
Queue _ready;     // processes which are ready to execute

// A separate stack for the OS itself
// (NOTE:  this assumes the OS is not reentrant!)
Stack *_system_stack;
uint32 *_system_esp;


/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/
void _init( void ) {
    Pcb *pcb;

    /*
    ** BOILERPLATE CODE - taken from basic framework
    **
    ** Initialize interrupt stuff.
    */

    __init_interrupts();  // IDT and PIC initialization

    /*
    ** Console I/O system.
    **
    ** Does not depend on the other kernel modules, so we can
    ** initialize it before we initialize the kernel memory
    ** and queue modules.
    */

    __cio_init( NULL );    // no console callback routine

#ifdef TRACE_CX
    __cio_setscroll( 0, 7, 99, 99 );
    __cio_puts_at( 0, 6, "================================================================================" );
#endif

    /*
    ** 20195-SPECIFIC CODE STARTS HERE
    */

    /*
    ** Initialize various OS modules
    **
    ** Other modules (clock, SIO, syscall, etc.) are expected to
    ** install their own ISRs in their initialization routines.
    */
    __cio_clearscreen();
    __cio_puts( "System initialization starting.\n" );
    __cio_puts( "-------------------------------\n" );

    __cio_puts( "Modules:" );

    _kmem_init();    // kernel memory system (must be first)
    _queue_init();   // queues (must be second)

    _clk_init();     // clock
    _proc_init();    // processes
    _sched_init();   // scheduler
    _sio_init();     // serial i/o
    _stk_init();     // stacks
    _sys_init();     // system calls
    _pci_init();     // PCI
    _ac97_init();    // AC97
    _usb_init();     // USB
    _soundblaster_init(); // sound blaster audio

    __cio_puts( "\nModule initialization complete.\n" );
    __cio_puts( "-------------------------------\n" );
    __delay( 2 );  // about 50 milliseconds

    /*
    ** Create the initial process
    **
    ** Code largely stolen from _sys_spawn(); if anything in
    ** that routine changes, SO MUST THIS!!!
    */

    // allocate a PCB and stack

    pcb = _pcb_alloc();
    assert( pcb );

    pcb->stack = _stk_alloc();
    assert( pcb->stack );

    // initialize the stack with the standard context

    char *argv[2] = { "init", NULL };

    // this is a bit weird, because init is its own parent
    // (ref: <https://www.youtube.com/watch?v=eYlJH81dSiw>)
    _init_pid = _proc_create( pcb, pcb->stack, pcb, (uint32) init, argv );
    assert( _init_pid > 0 );

    // _pcb_dump( "init()", pcb );
    // _stk_dump( "init() stack", pcb->stack, 8 );
    // __delay( 2000 );  // about 50 seconds

    // remember it
    _init_pcb = pcb;

    // put it on the ready queue
    _schedule( pcb );


    /*
    ** Next, create the idle process
    */

    // allocate a PCB and stack

    pcb = _pcb_alloc();
    assert( pcb );

    pcb->stack = _stk_alloc();
    assert( pcb->stack );

    // initialize the stack with the standard context

    argv[0] = "idle";

    _idle_pid = _proc_create( pcb, pcb->stack, _init_pcb, (uint32) idle, argv );
    assert( _idle_pid > 0 );

    // _pcb_dump( "init()", pcb );
    // _stk_dump( "init() stack", pcb->stack, 8 );
    // __delay( 2000 );  // about 50 seconds

    // remember it
    _idle_pcb = pcb;

    // DO NOT put it on the ready queue - it is found by
    // _dispatch() whenever it is needed

    _active += 1;

    /*
    ** Turn on the SIO receiver (the transmitter will be turned
    ** on/off as characters are being sent)
    */

    _sio_enable( SIO_RX );

    // dispatch the first user process

    _dispatch();

    /*
    ** END OF 20195-SPECIFIC CODE
    **
    ** Finally, report that we're all done.
    */

    __cio_puts( "System initialization complete.\n" );
    __cio_puts( "-------------------------------\n" );
}

#ifdef CONSOLE_SHELL
/*
** _shell - extremely simple shell for handling console input
**
** Called whenever we want to take input from the console and
** act upon it (e.g., for debugging the kernel)
*/
void _shell( int ch ) {

    // clear the input buffer
    (void) __cio_getchar();

    __cio_puts( "\nInteractive mode ('x' to exit)\n" );

    // loop until we get an "exit" indicator

    while( 1 ) {

        // are we done?
        if( ch == 'x' || ch == EOT ) {
            break;
        }

        switch( ch ) {

        case '\r': // ignore CR and LF
        case '\n':
            break;

        case 'q':  // dump the queues
            _queue_dump( "Sleep queue", _sleeping );
            _queue_dump( "Waiting queue", _waiting );
            _queue_dump( "Reading queue", _reading );
            _queue_dump( "Zombie queue", _zombie );
            _queue_dump( "Ready queue", _ready );
            break;

        case 'a':  // dump the active table
            _active_dump( "\nActive processes", false );
            break;

        case 'p':  // dump the active table and all PCBs
            _active_dump( "\nActive processes", true );
            break;

        case 'c':  // dump context info for all active PCBs
            _context_dump_all( "\nContext dump" );
            break;

        case 's':  // dump stack info for all active PCBS
            __cio_puts( "\nActive stacks (w/5-sec. delays):\n" );
            for( int i = 0; i < N_PROCS; ++i ) {
                if( _pcbs[i].state != UNUSED ) {
                    Pcb *pcb = &_pcbs[i];
                    __cio_printf( "pid %5d: ", pcb->pid );
                    __cio_printf( "EIP %08x, ", pcb->context->eip );
                    _stk_dump( NULL, pcb->stack, 12 );
                    __delay( 200 );
                }
            }
            break;
        case 'l': // List all connected PCI devices
            __cio_puts( "\nPCI Devices:\n" );

            _pci_dump_all();

            break;
        case 'u':
            _usb_dump_all();
            break;

        case 'm':
            _ac97_status();
            break;

        default:
            __cio_printf( "shell: unknown request '0x%02x'\n", ch );

            // FALL THROUGH

        case 'h':  // help message
            __cio_puts( "\nCommands:\n" );
            __cio_puts( "   a  -- dump the active table\n" );
            __cio_puts( "   c  -- dump contexts for active processes\n" );
            __cio_puts( "   h  -- this message\n" );
            __cio_puts( "   p  -- dump the active table and all PCBs\n" );
            __cio_puts( "   q  -- dump the queues\n" );
            __cio_puts( "   s  -- dump stacks for active processes\n" );
            __cio_puts( "   l  -- list all PCI devices\n");
            __cio_puts( "   u  -- dump USB init information\n" );
            __cio_puts( "   m  -- dump status of the AC97 device\n" );
            __cio_puts( "   x  -- exit\n" );
            break;
        }

        __cio_puts( "\n? " );
        ch = __cio_getchar();
    }

    __cio_puts( "\nLeaving interactive mode\n\n" );
}
#endif
