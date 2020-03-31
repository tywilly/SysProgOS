/*
** SCCS ID: @(#)stacks.c	1.1 3/30/20
**
** File:    stacks.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Implementation of the process module.
*/

#define __SP_KERNEL__

#include "common.h"
#include "stacks.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

//
// This is weird.  This structure exists because GCC doesn't
// want to let us do any of the following in _stk_free(),
// where 'stk' and '_free_stacks' are both Stack*:
//
//      *stk = _free_stacks;
//      *stk = (uint32) _free_stacks;
//      stk[0] = (uint32) _free_stacks;
//      *(Stack **)stk = _free_stacks;
//
// This makes it difficult to create a linked list of Stacks.
// Instead, we overlay this dumb little structure on each
// Stack, and the compiler is happy.
//
typedef struct stklist_s {
    struct stklist_s *next;
} Stacklist;

/*
** PRIVATE GLOBAL VARIABLES
*/

// stack management
//
// our "free list" uses the first word in the stack
// as a pointer to the next free stack

static Stacklist _free_stacks;

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

//
// _stk_init() - initialize the stack module
//
void _stk_init( void ) {

    // set up the free list
    _free_stacks.next = NULL;

    // allocate the first stack for the OS
    _system_stack = _stk_alloc();
    assert( _system_stack );

    // set the initial ESP for the OS - it should point to the
    // next-to-last uint32 in the stack, so that when the code
    // at isr_save switches to the system stack and pushes the
    // error code and vector number, ESP is aligned at a multiple
    // of 16 address.
    _system_esp = ((uint32 *) (_system_stack + 1)) - 2;

    // report that we're done
    __cio_puts( " STACKS" );
}

//
// _stk_alloc() - allocate a stack
//
Stack *_stk_alloc( void ) {
    Stacklist *new;

    // just take the first one from the list if there is one
    if( _free_stacks.next != NULL ) {

        // just take the first one
        new = _free_stacks.next;

        // unlink it from the list
        _free_stacks.next = new->next;
        __memclr( new, sizeof(Stack) );

    } else {

        // none available - create a new one
        new = (Stacklist *) _kalloc_page( STACK_PAGES );

    }

    return( (Stack *) new );
}

//
// _stk_free() - return a stack to the free pool
//
void _stk_free( Stack *stk ) {
    Stacklist *tmp;

    // make sure we were given one to release
    if( stk == NULL ) {
        return;
    }

    // add it to the front of the free list
    tmp = (Stacklist *) stk;
    tmp->next = _free_stacks.next;
    _free_stacks.next = tmp;
}

/*
** Debugging/tracing routines
*/

//
// _stk_dump(msg,stk,limit)
//
// dump the contents of the provided stack, eliding duplicate lines
//
// assumes the stack size is a multiple of four words
//
// output lines begin with the 8-digit address, followed by a hex
// interpretation then a character interpretation of four words
//
// hex needs 41 bytes:
// col pos  1         2         3         4
// 1        0         0         0         0
//   xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx
//
// character needs 22 bytes:
//             1    1  2
// 1 3    8    3    8  1
//   cccc cccc cccc cccc
//
// output lines that are identical except for the address are skipped;
// the next non-identical output line will have a '*' after the 8-digit
// address field
//

// buffer sizes (rounded up a bit)
#define HBUFSZ      48
#define CBUFSZ      24

void _stk_dump( const char *msg, Stack *stack, uint32 limit ) {
    int words = sizeof(Stack) / sizeof(uint32);
    int eliding = 0;
    char oldbuf[HBUFSZ], buf[HBUFSZ], cbuf[CBUFSZ];
    uint32 addr = (uint32 ) stack;
    uint32 *sp = (uint32 *) stack;
    char hexdigits[] = "0123456789ABCDEF";

    // if a limit was specified, dump only that many words

    if( limit > 0 ) {
        words = limit;
        if( (words & 0x3) != 0 ) {
            // round up to a multiple of four
            words = (words & 0xfffffffc) + 4;
        }
        // skip to the new starting point
        sp += (STACK_U32 - words);
        addr = (uint32) sp;
    }

    __cio_puts( "*** stack" );
    if( msg != NULL ) {
        __cio_printf( " (%s):\n", msg );
    } else {
        __cio_puts( ":\n" );
    }

    oldbuf[0] = '\0';

    while( words > 0 ) {
        register char *bp = buf;   // start of hex field
        register char *cp = cbuf;  // start of character field
        uint32 start_addr = addr;

        // iterate through the words for this line

        for( int i = 0; i < 4; ++i ) {
            register uint32 curr = *sp++;
            register uint32 data = curr;

            // convert the hex representation

            // two spaces before each entry
            *bp++ = ' ';
            *bp++ = ' ';

            for( int j = 0; j < 8; ++j ) {
                uint32 value = (data >> 28) & 0xf;
                *bp++ = hexdigits[value];
                data <<= 4;
            }

            // now, convert the character version
            data = curr;

            // one space before each entry
            *cp++ = ' ';

            for( int j = 0; j < 4; ++j ) {
                uint32 value = (data >> 24) & 0xff;
                *cp++ = (value >= ' ' && value < 0x7f) ? (char) value : '.';
                data <<= 8;
            }
        }
        *bp = '\0';
        *cp = '\0';
        words -= 4;
        addr += 16;

        // if this line looks like the last one, skip it

        if( __strcmp(oldbuf,buf) == 0 ) {
            ++eliding;
            continue;
        }

        // it's different, so print it

        // start with the address
        __cio_printf( "%08x%c", start_addr, eliding ? '*' : ' ' );
        eliding = 0;

        // print the words
        __cio_printf( "%s %s\n", buf, cbuf );

        // remember this line
        __memcpy( (uint8 *) oldbuf, (uint8 *) buf, HBUFSZ );
    }
}
