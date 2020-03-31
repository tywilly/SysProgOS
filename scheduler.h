/*
** SCCS ID:	@(#)scheduler.h	1.1	3/30/20
**
** File:	scheduler.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Declarations for the scheduler module.
*/

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// Quanta for processes (in clock ticks)

#define	QUANTUM_STD	5

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

//
// _sched_init() - initialize the scheduler module
//
void _sched_init( void );

//
// _schedule() - the scheduler
//
// @param pcb   The process to be scheduled
//
void _schedule( Pcb *pcb );

//
// _dispatch() - give the CPU to a process
//
void _dispatch( void );

#endif

#endif
