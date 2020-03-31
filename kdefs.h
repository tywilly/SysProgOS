/*
** SCCS ID: @(#)kdefs.h	1.1 3/30/20
**
** File:    kdefs.h
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Kernel-only definitions for the baseline system.
**
*/

#ifndef _KDEFS_H_
#define _KDEFS_H_

// The OS needs the standard system headers

#include "cio.h"
#include "kmem.h"
#include "support.h"
#include "kernel.h"
#include "klib.h"

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// Maximum number of bytes of command-line argument string space

#define MAX_ARG_LIST    4096

// bit patterns for modulus checking of (e.g.) sizes and addresses

#define MOD4_BITS        0x3
#define MOD4_MASK        0xfffffffc

#define MOD16_BITS       0xf
#define MOD16_MASK       0xfffffff0

// Debugging and sanity-checking macros

// Warning messages to the console

#define WARNING(m)  { \
        __cio_printf( "WARN %s (%s @ %s): ", __func__, __FILE__, __LINE__ ); \
        __cio_puts( m ); \
        __cio_putchar( '\n' ); \
    }

// Panic messages to the console

#define PANIC(n,x)  { \
        __sprint( b512, "ASSERT %s (%s @ %s), %d: ", \
                  __func__, __FILE__, __LINE__, n ); \
        _kpanic( b512, # x ); \
    }

// Always-active assertions

#define assert(x)   if( !(x) ) { PANIC(0,x); }

// Other assertions are categorized and can be disabled
// by defining the CPP macro SANITY with a value of 0

#ifndef SANITY
// default sanity check level: check everything!
#define SANITY  9999
#endif

// only provide these macros if the sanity check level is positive

#if SANITY > 0

#define assert1(x)  if( SANITY >= 1 && !(x) ) { PANIC(1,x); }
#define assert2(x)  if( SANITY >= 2 && !(x) ) { PANIC(2,x); }
#define assert3(x)  if( SANITY >= 3 && !(x) ) { PANIC(3,x); }
#define assert4(x)  if( SANITY >= 4 && !(x) ) { PANIC(4,x); }

#else

#define assert1(x)  // do nothing
#define assert2(x)  // do nothing
#define assert3(x)  // do nothing
#define assert4(x)  // do nothing

#endif

#endif

#endif
