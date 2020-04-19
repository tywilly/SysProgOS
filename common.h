/*
** SCCS ID:	@(#)common.h	1.1	3/30/20
**
** File:	common.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Common definitions for the baseline system.
**
** This header file pulls in the standard header information
** needed by all parts of the system (OS and user levels).
*/

#ifndef _COMMON_H_
#define _COMMON_H_

/*
** General (C and/or assembly) definitions
*/

// NULL pointer value
//
// we define this the traditional way so that
// it's usable from both C and assembly

#define NULL    0

// predefined I/O channels

#define	CHAN_CONS	0
#define	CHAN_SIO	1
#define CHAN_AC97   2
#define CHAN_SB     3

// maximum number of processes in the system

#define N_PROCS     25

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// halves of various data sizes

#define UI16_UPPER  0xff00
#define UI16_LOWER  0x00ff

#define UI32_UPPER  0xffff0000
#define UI32_LOWER  0x0000ffff

#define UI64_UPPER  0xffffffff00000000LL
#define UI64_LOWER  0x00000000ffffffffLL

// Simple conversion pseudo-functions usable by everyone

// convert seconds to ms

#define	SEC_TO_MS(n)		((n) * 1000)

// pull in the standard system type definitions
#include "types.h"

/*
** Additional OS-only or user-only things
*/

#ifdef __SP_KERNEL__
#include "kdefs.h"
#else
#include "udefs.h"
#endif

#endif

#endif
