/*
** SCCS ID:	@(#)syscalls.h	1.1	3/30/20
**
** File:	syscalls.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	System call module declarations
*/

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// system call codes
//
// these are used in the user-level C library stub functions

#define	SYS_exit	0
#define	SYS_kill	1
#define	SYS_wait	2
#define	SYS_spawn	3
#define	SYS_read	4
#define	SYS_write	5
#define	SYS_sleep	6
#define	SYS_gettime	7
#define	SYS_getpid	8
#define	SYS_getppid	9
#define	SYS_getstate	10
#define SYS_ac97_getvol 11
#define SYS_ac97_setvol 12
#define SYS_ac97_srate  13

// UPDATE THIS DEFINITION IF MORE SYSCALLS ARE ADDED!
#define	N_SYSCALLS	14

// dummy system call code to test our ISR

#define	SYS_bogus	0xbad

// interrupt vector entry for system calls

#define	INT_VEC_SYSCALL		0x42

#ifdef	__SP_KERNEL__

// the following declarations should only be seen by the kernel


#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#include "queues.h"

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/*
** _really_exit - do the real work for exit() and some kill() calls
**
** @param victim  Pointer to the PCB that should exit
** @param parent  Pointer to the victim's parent
** @param status  Termination status
*/
void _really_exit( Pcb *victim, Pcb *parent, int32 status );

/*
** _sys_init()
**
** initializes all syscall-related data structures
*/
void _sys_init( void );

#endif

#endif

#endif
