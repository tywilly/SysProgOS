/*
** SCCS ID: @(#)process.c	1.1 3/30/20
**
** File:    process.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Implementation of the process module.
*/

#define __SP_KERNEL__

#include "common.h"
#include "process.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// PCB management
static Pcb *_free_pcbs;

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** PCB manipulation
*/

//
// _pcb_alloc() - allocate a PCB
//
Pcb *_pcb_alloc( void ) {
    Pcb *new;

    // just take the first one from the list
    new = _free_pcbs;

    // assuming we got one, unlink it and initialize it
    if( new != NULL ) {
        _free_pcbs = (Pcb *) (new->queue);
        new->queue = NULL;
        new->state = NEW;
    }

    return( new );
}

//
// _pcb_free() - return a PCB to the free pool
//
void _pcb_free( Pcb *pcb ) {

    // make sure we were given one to release
    if( pcb != NULL ) {
        // mark it as available
        pcb->state = UNUSED;
        // add it to the front of the free list
        pcb->queue = (Queue) _free_pcbs;
        _free_pcbs = pcb;
    }
}

//
// _pcb_find() - locate the PCB for a specified process
//
Pcb *_pcb_find( Pid pid ) {

    // iterate through the PCB table
    for( int i = 0; i < N_PROCS; ++i ) {
        // if this is the one we want, we're done
        if( _pcbs[i].pid == pid && _pcbs[i].state != UNUSED ) {
            return( &_pcbs[i] );
        }
    }

    return( NULL );
}

/*
** Process management/control
*/

//
// _wakeup_cmp() - comparison function for ordered Queue
//
// @param a   The first PCB to examine
// @param b   The second PCB to examine
//
// @returns A comparison between the wakeup times in the two PCBs
//
int _wakeup_cmp( const void *a, const void *b ) {
    const Pcb *p1 = (const Pcb *) a;
    const Pcb *p2 = (const Pcb *) b;

    // simple:
    // if( p1->wakeup < p2->wakeup ) return( -1 );
    // else if( p1->wakeup == p2->wakeup ) return( 0 );
    // else return( 1 );

    // return the lower 32 bits of the difference between the wakeup times
    return (int) ( ((int64)(p1->wakeup) - (int64)(p2->wakeup)) & UI64_LOWER );
}

//
// _proc_init() - initialize the process module
//
void _proc_init( void ) {

    _free_pcbs = NULL;
    _active = 0;

    _next_pid = 1;  // PID of init()

    // add each PCB to the free pool
    for( int i = 0; i < N_PROCS; ++i ) {
        _pcb_free( &_pcbs[i] );
    }

    // report that we're done
    __cio_puts( " PROCS" );
}

//
// _proc_create() - create a process from supplied data
//
// returns the PID of the process, or an error code
//
int _proc_create( Pcb *pcb, Stack *stk, Pcb *parent,
                  uint32 arg1, char **arg2 )
{
    char **argv = arg2;
    int argc = 0;
    int buflen = 0;

    // sanity checking!
    assert1( pcb != NULL );
    assert1( stk != NULL );
    assert1( parent != NULL );

    /*
    ** Set up the initial stack contents for a (new) user process.
    **
    ** We reserve one longword at the bottom of the stack as scratch
    ** space.  Above that, we simulate a call from exit_helper() with an
    ** argument vector by pushing the arguments and then the argument
    ** count.  We follow this up by pushing the address of the entry point
    ** of exit_helper() as a "return address".  Above that, we place a
    ** Context area that is initialized with the standard initial register
    ** contents.
    **
    ** The high end of the stack will contain these values:
    **
    **      esp ->  ?            <- context save area
    **              ...          <- context save area
    **              ?            <- context save area
    **              exit_helper  <- return address for faked call to main()
    **              argc         <- argument count for main()
    **        /---- args         <- pointer to first "command-line" argument
    **        \---> ...          <- string buffer for arguments
    **              ...          <--|
    **              ...          <--/
    **              0            <- last word in stack
    **
    ** String buffer space is computed by summing the lengths of the
    ** N individual argv strings plus N (to account for the trailing
    ** NUL bytes).  This value is rounded up to the next multiple of
    ** four address.  We then round that result up to the appropriate
    ** multiple of 16, to align the stack.
    **
    ** When this process is dispatched, the context restore code will
    ** pop all the saved context information off the stack, leaving the
    ** "return address" on the stack as if the main() for the process
    ** had been "called" from the exit_helper() function.  When main()
    ** returns, it will "return" to the entry point of exit_helper(),
    ** which will then call exit().  We think.
    */

    // begin with the argument vector - count the entries
    // and determine the amount of space we'll need to reserve

    for( argc = 0; argv[argc]; ++argc ) {
        buflen += __strlen(argv[argc]) + 1;
    }

    // only allow a certain amount of argument string space

    if( buflen > MAX_ARG_LIST ) {
        return( E_ARGS_TOO_LONG );
    }

    // OK so far - start filling in the process information

    pcb->stack = stk;
    pcb->pid = _next_pid++;

    // this works even for the init process, because 'pcb' and
    // 'parent' point to the same PCB, and we just filled in
    // the PID for the process :-)
    pcb->ppid = parent->pid;

    pcb->wakeup = 0LL;
    pcb->children = 0;
    pcb->exit_status = 0;

    // increment the parent's child count
    parent->children += 1;

    /*
    ** Align the stack.  The SysV ABI i386 supplement, version 1.2
    ** (June 23, 2016) states in section 2.2.2:
    **
    **   "The end of the input argument area shall be aligned on a 16
    **   (32 or 64, if __m256 or __m512 is passed on stack) byte boundary.
    **   In other words, the value (%esp + 4) is always a multiple of 16
    **   (32 or 64) when control is transferred to the function entry
    **   point. The stack pointer, %esp, always points to the end of the
    **   latest allocated stack frame."
    **
    ** Isn't technical documentation fun?
    **
    ** What we do is ensure that the first parameter to the main() function
    ** is at a multiple-of-16 address in the stack.  We need room for argc
    ** and args (8 bytes) plus the string buffer; what's after that is
    ** padding.
    */

    // pointer to the last longword in the stack
    uint32 *lptr = ((uint32 *) (stk + 1)) - 1;

    // convert our pointer into a byte pointer masquerading as a uint32
    uint32 tmp = (uint32) lptr;

    // ... back it up by the buffer size ...
    tmp -= buflen;

    // ... add room for argc and args ...
    tmp -= sizeof(int) + sizeof(char *);

    // ... round that back to the preceding multiple of sixteen ...
    tmp &= MOD16_MASK;

    // ... and convert it back into a longword pointer
    lptr = (uint32 *) tmp;

    // place the argument count there
    *lptr = (uint32) argc;

    // the args pointer comes after it; its value is the address
    // following where the args pointer itself is in the stack
    *(lptr+1) = (uint32) (lptr + 2);

    /*
    ** Now we need to copy in the argument strings.
    **
    ** For each argument string, copy in the string, and advance
    ** the string buffer pointer past the trailing NUL.
    */
    register char *ptr = (char *) (lptr + 2);

    for( int i = 0; i < argc; ++i ) {
        register char *curr = argv[i];
        while( (*ptr++ = *curr++) ) {    // zoom zoom!
            ;
        }
    }

    // copy in the dummy return address
    --lptr;
    *lptr = (uint32) exit_helper;

    // next, set up the context
    Context *cptr = (Context *) lptr;
    --cptr;

    cptr->eip = arg1;               // entry point
    cptr->eflags = DEFAULT_EFLAGS;  // should have IF enabled
    cptr->ds = GDT_DATA;            // all the segment registers
    cptr->es = GDT_DATA;
    cptr->fs = GDT_DATA;
    cptr->gs = GDT_DATA;
    cptr->ss = GDT_STACK;
    cptr->cs = GDT_CODE;

    // set up the PCB entry for context restores
    pcb->context = cptr;

    // let the caller know we succeeded
    return( pcb->pid );
}

//
// _proc_cleanup() - clean up a defunct process
//
// @param pcb   The PCB to be reclaimed
//
// This will need to be expanded if the PCB is modified so that
// it contains any other allocated data (e.g., page tables, etc.)
//
void _proc_cleanup( Pcb *pcb ) {
    
    if( pcb != NULL ) {
        // eliminate the stack, if there is one
        if( pcb->stack != NULL ) {
            _stk_free( pcb->stack );
        }
        // now, reclaim the PCB
        _pcb_free( pcb );
        // no longer active!
        _active -= 1;
    }
}

/*
** Debugging/tracing routines
*/

//
// _pcb_dump(msg,pcb)
//
// dump the contents of this PCB to the console
//
void _pcb_dump( const char *msg, Pcb *pcb ) {

    __cio_printf( "%s @ %08x: ", msg, (uint32) pcb );
    if( pcb == NULL ) {
        __cio_puts( " NULL???\n" );
        return;
    }

    __cio_printf( " pids %d/%d state %d", pcb->pid, pcb->ppid, pcb->state );

    __cio_printf( "\n kids %d ticks %d xit %d",
                  pcb->children, pcb->quantum, pcb->exit_status );

    __cio_printf( "\n context %08x stack %08x\n",
                  (uint32) pcb->context, (uint32) pcb->stack );
}

//
// _context_dump(msg,context)
//
// dump the contents of this process context to the console
//
void _context_dump( const char *msg, register Context *c ) {

    if( msg ) {
        __cio_printf( "%s ", msg );
    }
    __cio_printf( "@ %08x: ", (uint32) c );

    if( c == NULL ) {
        __cio_puts( " NULL???\n" );
        return;
    }

    __cio_printf( "ss %04x gs %04x fs %04x es %04x ds %04x cs %04x\n",
                  c->ss & 0xff, c->gs & 0xff, c->fs & 0xff,
                  c->es & 0xff, c->ds & 0xff, c->cs & 0xff );
    __cio_printf( "  edi %08x esi %08x ebp %08x esp %08x\n",
                  c->edi, c->esi, c->ebp, c->esp );
    __cio_printf( "  ebx %08x edx %08x ecx %08x eax %08x\n",
                  c->ebx, c->edx, c->ecx, c->eax );
    __cio_printf( "  vec %08x cod %08x eip %08x eflags %08x\n",
                  c->vector, c->code, c->eip, c->eflags );
}

//
// _context_dump_all(msg)
//
// dump the process context for all active processes
//
void _context_dump_all( const char *msg ) {

    __cio_printf( "%s: %d active processes\n", msg, _active );

    if( _active < 1 ) {
        return;
    }

    int n = 0;
    for( int i = 0; i < N_PROCS; ++i ) {
        Pcb *pcb = &_pcbs[i];
        if( pcb->state != UNUSED ) {
            ++n;
            __cio_printf( "%2d[%2d]: ", n, i );
            _context_dump( NULL, pcb->context );
        }
    }
}

//
// _active_dump(msg,all)
//
// dump the contents of the "active processes" table
//
void _active_dump( const char *msg, bool all ) {

    if( msg ) {
        __cio_printf( "%s: ", msg );
    }

    int used = 0;
    int empty = 0;

    for( int i = 0; i < N_PROCS; ++i ) {
        register Pcb *pcb = &_pcbs[i];
        if( pcb->state == UNUSED ) {

            // an empty slot
            ++empty;

        } else {

            // a non-empty slot
            ++used;

            // if not dumping everything, add commas if needed
            if( !all && used ) {
                __cio_putchar( ',' );
            }

            // things that are always printed
            __cio_printf( " #%d: %d/%d %d", i, pcb->pid, pcb->ppid,
                          pcb->state );

            // do we want more info?
            if( all ) {
                __cio_printf( " stk %08x EIP %08x\n",
                      (uint32)pcb->stack, pcb->context->eip );
            }
        }
    }

    // only need this if we're doing one-line output
    if( !all ) {
        __cio_putchar( '\n' );
    }

    // sanity check - make sure we saw the correct number of table slots
    if( (used + empty) != N_PROCS ) {
        __cio_printf( "Table size %d, used %d + empty %d = %d???\n",
                      N_PROCS, used, empty, used + empty );
    }
}
