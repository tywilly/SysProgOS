/*
** SCCS ID:	%W%	%G%
**
** File:	kernel.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Miscellaneous OS routines
*/

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "common.h"
#include <x86arch.h>

#include "process.h"
#include "queues.h"

// copied from ulib.h
void exit_helper( void );

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// default contents of EFLAGS register

#define DEFAULT_EFLAGS  (EFLAGS_MB1 | EFLAGS_IF)

/*
** Types
*/

/*
** Globals
*/

// character buffers, usable throughout the OS
// not guaranteed to retain their contents across an exception return
extern char b256[256];
extern char b512[512];

// Current system time (managed by clock.c)
extern Time _system_time;

// Process-related data (managed by process.c and other modules)

// The current process
extern Pcb *_current;

// Information about the init() process
extern Pid _init_pid;
extern Pcb *_init_pcb;

// Information about the idle() process
extern Pid _idle_pid;
extern Pcb *_idle_pcb;

// Static array of PCBs
extern Pcb _pcbs[];

// Count of active processes
extern uint32 _active;

// PID tracker
extern Pid _next_pid;

// Queues (used by multiple modules)
extern Queue _waiting;   // processes waiting (for Godot?)
extern Queue _reading;   // processes blocked on input
extern Queue _zombie;    // gone, but not forgotten
extern Queue _sleeping;  // processes catching some Z
extern Queue _ready;     // processes which are ready to execute

// A separate stack for the OS itself
// (NOTE:  this assumes the OS is not reentrant!)
extern Stack *_system_stack;
extern uint32 *_system_esp;

/*
** Prototypes
*/

/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/
void _init( void );

#ifdef CONSOLE_SHELL
/*
** _shell - extremely simple shell for handling console input
**
** Called whenever we want to take input from the console and
** act upon it (e.g., for debugging the kernel)
*/
void _shell( int );

#endif

#endif

#endif
