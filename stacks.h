/*
** SCCS ID: %W% %G%
**
** File:    stacks.h
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Declarations for the stack module.
*/

#ifndef _STACKS_H_
#define _STACKS_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#include "types.h"
#include "kmem.h"

// stack size, in bytes and words
//
// our stack is a multiple of the page size, for simplicity

#define STACK_MULT      4

#define STACK_SIZE      (PAGE_SIZE * STACK_MULT)
#define STACK_U32       (STACK_SIZE / sizeof(uint32))
#define STACK_PAGES     (STACK_SIZE / PAGE_SIZE)

/*
** Types
*/


// the stack
//
// somewhat anticlimactic....

typedef uint32 Stack[STACK_U32];

/*
** Globals
*/

/*
** Prototypes
*/

//
// _stk_init() - initialize the stack module
//
void _stk_init( void );

//
// _stk_alloc() - allocate a stack
//
Stack *_stk_alloc( void );

//
// _stk_free() - return a stack to the free pool
//
void _stk_free( Stack *stk );

/*
** Debugging/tracing routines
*/

//
// _stk_dump(msg,stk,limit)
//
// dump the contents of this Stack to the console
//
void _stk_dump( const char *msg, Stack *stk, uint32 limit );

#endif

#endif
