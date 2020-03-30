/*
** SCCS ID: %W% %G%
**
** File:        Offsets.c
**
** Author:      Warren R. Carithers
**
** Description: Print size and offset information for our data structures.
**
** This program exists to simplify life.  If/when fields in a structure are
** changed, this can be modified, recompiled and executed to come up with
** byte offsets for use in accessing structure fields from assembly language.
** With a bit more work, it could generate a header files with CPP defines
** for all of these.
**
** IMPORTANT NOTE:  compiling this on a 64-bit architecture will yield
** incorrect results by default, as 64-bit GCC versions most often use
** the LP64 model (longs and pointers are 64 bits).  Add the "-mx32"
** option to the compiler (compile for x86_64, but use 32-bit sizes),
** and make sure you have the 'libc6-dev-i386' package installed (for
** Ubuntu systems).  (The Makefile for the baseline system is set up to
** do this already.)
*/

#define __SP_KERNEL__

#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include "process.h"
#include "stacks.h"

#include <stdio.h>

Context context;
Pcb pcb;
Stack stack;

int main( void ) {

    puts( "Sizes of basic types (in bytes):" );
    printf( "\tchar %u\tshort %u\tint %u\tlong %u\tlong long %u\n",
        sizeof(char), sizeof(short), sizeof(int),
        sizeof(long), sizeof(long long) );
    putchar( '\n');

    puts( "Sizes of our types (in bytes):" );
    printf( "\tint8    %u\tuint8   %u\n", sizeof(int8), sizeof(uint8) );
    printf( "\tint16   %u\tuint16  %u\n", sizeof(int16), sizeof(uint16) );
    printf( "\tint32   %u\tuint32  %u\n", sizeof(int32), sizeof(uint32) );
    printf( "\tint64   %u\tuint64  %u\n", sizeof(int64), sizeof(uint64) );
    printf( "\tContext %u\tPcb     %u\n", sizeof(Context), sizeof(Pcb) );
    printf( "\tbool    %u", sizeof(bool) );
    printf( "\tState   %u\n", sizeof(State) );
    printf( "\tStatus  %u\tTime    %u\n", sizeof(Status), sizeof(Time) );
    printf( "\tQueue   %u", sizeof(Queue) );
    printf( "\tStack   %u\n", sizeof(Stack) );
    putchar( '\n');

    printf( "Byte offsets into Context (%u bytes):\n", sizeof(context) );
    printf( "   ss:\t\t%d\n", (char *)&context.ss - (char *)&context );
    printf( "   gs:\t\t%d\n", (char *)&context.gs - (char *)&context );
    printf( "   fs:\t\t%d\n", (char *)&context.fs - (char *)&context );
    printf( "   es:\t\t%d\n", (char *)&context.es - (char *)&context );
    printf( "   ds:\t\t%d\n", (char *)&context.ds - (char *)&context );
    printf( "   edi:\t\t%d\n", (char *)&context.edi - (char *)&context );
    printf( "   esi:\t\t%d\n", (char *)&context.esi - (char *)&context );
    printf( "   ebp:\t\t%d\n", (char *)&context.ebp - (char *)&context );
    printf( "   esp:\t\t%d\n", (char *)&context.esp - (char *)&context );
    printf( "   ebx:\t\t%d\n", (char *)&context.ebx - (char *)&context );
    printf( "   edx:\t\t%d\n", (char *)&context.edx - (char *)&context );
    printf( "   ecx:\t\t%d\n", (char *)&context.ecx - (char *)&context );
    printf( "   eax:\t\t%d\n", (char *)&context.eax - (char *)&context );
    printf( "   vector:\t%d\n",(char *)&context.vector - (char *)&context);
    printf( "   code:\t%d\n", (char *)&context.code - (char *)&context );
    printf( "   eip:\t\t%d\n", (char *)&context.eip - (char *)&context );
    printf( "   cs:\t\t%d\n", (char *)&context.cs - (char *)&context );
    printf( "   eflags:\t%d\n",(char *)&context.eflags - (char *)&context);
    putchar( '\n' );

    printf( "Byte offsets into Pcb (%u bytes):\n", sizeof(pcb) );
    printf( "   context:\t%d\n", (char *)&pcb.context - (char *)&pcb );
    printf( "   stack:\t%d\n", (char *)&pcb.stack - (char *)&pcb );
    printf( "   wakeup:\t%d\n", (char *)&pcb.wakeup - (char *)&pcb );
    printf( "   queue:\t%d\n", (char *)&pcb.queue - (char *)&pcb );
    printf( "   exit_status:\t%d\n", (char *)&pcb.exit_status - (char *)&pcb );
    printf( "   pid:\t\t%d\n", (char *)&pcb.pid - (char *)&pcb );
    printf( "   ppid:\t%d\n", (char *)&pcb.ppid - (char *)&pcb );
    printf( "   children:\t%d\n", (char *)&pcb.children - (char *)&pcb );
    printf( "   state:\t%d\n", (char *)&pcb.state - (char *)&pcb );
    printf( "   quantum:\t%d\n",(char *)&pcb.quantum - (char *)&pcb);

    return( 0 );
}
