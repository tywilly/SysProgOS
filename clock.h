/*
** SCCS ID:	@(#)clock.h	1.1	3/30/20
**
** File:	clock.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Clock module declarations
*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// timer-related definitions

// clock interrupts per second

#define	CLOCK_FREQUENCY		1000
#define	TICKS_PER_MS		1

// conversion functions to go between seconds, ms, and ticks
// SEC_TO_MS is defined in common.h

#define	MS_TO_TICKS(n)		((n))
#define	SEC_TO_TICKS(n)		(MS_TO_TICKS(SEC_TO_MS(n)))
#define	TICKS_TO_SEC(n)		((n) / CLOCK_FREQUENCY)
#define	TICKS_TO_SEC_ROUNDED(n)	(((n)+(CLOCK_FREQUENCY-1)) / CLOCK_FREQUENCY)

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
// _clk_init() - initialize the clock module
//
void _clk_init( void );

#endif

#endif
