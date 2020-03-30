/*
** SCCS ID: %W% %G%
**
** File:    process.h
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Declarations for the process module.
*/

#ifndef _PROCESS_H_
#define _PROCESS_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

#include "common.h"

#include "stacks.h"
#include "queues.h"

#include "bootstrap.h"

/*
** Start of C-only definitions
*/

// REG(pcb,x) -- access a specific register in a process context

#define REG(pcb,x)  ((pcb)->context->x)

// RET(pcb) -- access return value register in a process context

#define RET(pcb)    ((pcb)->context->eax)

// ARG(pcb,n) -- access argument #n from the indicated process
//
// ARG(pcb,0) --> return address
// ARG(pcb,1) --> first parameter
// ARG(pcb,2) --> second parameter
// etc.
//
// ASSUMES THE STANDARD 32-BIT ABI, WITH PARAMETERS PUSHED ONTO THE
// STACK.  IF THE PARAMETER PASSING MECHANISM CHANGES, SO MUST THIS!

#define ARG(pcb,n)  ( ( (uint32 *) (((pcb)->context) + 1) ) [(n)] )

/*
** Types
*/

// process states are defined in types.h so that
// they're available in userland

// process context structure
//
// NOTE:  the order of data members here depends on the
// register save code in isr_stubs.S!!!!

typedef struct context {
    uint32 ss;
    uint32 gs;
    uint32 fs;
    uint32 es;
    uint32 ds;
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 esp;
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;
    uint32 vector;
    uint32 code;
    uint32 eip;
    uint32 cs;
    uint32 eflags;
} Context;

// the process control block
//
// ideally, its size should divide evenly into 1024 bytes
//
// currently, 32 bytes

typedef struct pcb_s {
    // Start with these eight bytes, for easy access in assembly
    Context *context;       // pointer to context save area on stack
    Stack *stack;           // pointer to process stack

    // eight-byte values
    Time wakeup;            // wakeup time for sleeping process

    // four-byte values
    Queue queue;            // pointer to whatever queue it's on
                            // (doubles as the "free PCB list" link)

    int32 exit_status;      // termination status

    // two-byte values
    Pid pid;                // unique PID for this process
    Pid ppid;               // PID of the parent
    uint16 children;        // count of children

    // one-byte values
    State state;            // current state (see types.h)
    uint8 quantum;          // remaining quantum
} Pcb;

/*
** Globals
*/

/*
** Prototypes
*/

/*
** PCB manipulation
*/

//
// _pcb_alloc() - allocate a PCB
//
Pcb *_pcb_alloc( void );

//
// _pcb_free() - return a PCB to the free pool
//
void _pcb_free( Pcb *pcb );

//
// _pcb_find() - locate the PCB for a specified process
//
Pcb *_pcb_find( Pid pid );

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
int _wakeup_cmp( const void *a, const void *b );

//
// _proc_init() - initialize the process module
//
void _proc_init( void );

//
// _proc_create() - create a process from supplied data
//
// returns the PID of the process, or an error code
//
int _proc_create( Pcb *pcb, Stack *stk, Pcb *parent,
                  uint32 arg1, char **arg2 );

//
// _proc_cleanup() - clean up a defunct process
//
void _proc_cleanup( Pcb *pcb );

/*
** Debugging/tracing routines
*/

//
// _pcb_dump(msg,pcb)
//
// dump the contents of this PCB to the console
//
void _pcb_dump( const char *msg, Pcb *pcb );

//
// _context_dump(msg,context)
//
// dump the contents of this process context to the console
//
void _context_dump( const char *msg, Context *context );

//
// _context_dump_all(msg)
//
// dump the process context for all active processes
//
void _context_dump_all( const char *msg );

//
// _active_dump(msg,all)
//
// dump the contents of the "active processes" table
//
void _active_dump( const char *msg, bool all );

#endif

#endif
