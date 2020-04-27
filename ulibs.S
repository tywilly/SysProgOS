/*
** SCCS ID:	@(#)ulibs.S	1.1	3/30/20
**
** File:	ulibs.S
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	assembly-language user-level library functions
*/

#define	__SP_ASM__

#include "syscalls.h"

/*
** System call stubs
**
** All have the same structure:
**
**      move a code into EAX
**      generate the interrupt
**      return to the caller
**
** As these are simple "leaf" routines, we don't use
** the standard enter/leave method to set up a stack
** frame - that takes time, and we don't really need it.
*/

#define	SYSCALL(name) \
	.globl	name			; \
name:					; \
	movl	$SYS_##name, %eax	; \
	int	$INT_VEC_SYSCALL	; \
	ret

SYSCALL(exit)
SYSCALL(kill)
SYSCALL(wait)
SYSCALL(spawn)
SYSCALL(read)
SYSCALL(write)
SYSCALL(sleep)
SYSCALL(gettime)
SYSCALL(getpid)
SYSCALL(getppid)
SYSCALL(getstate)
SYSCALL(ac97_getvol)
SYSCALL(ac97_setvol)
SYSCALL(ac97_setrate)
SYSCALL(ac97_initialized);
SYSCALL(ac97_write);

/*
** This is a bogus system call; it's here so that we can test
** our handling of out-of-range syscall codes in the syscall ISR.
*/

SYSCALL(bogus)

/*
** Other library functions
*/

/*
** exit_helper() - dummy "startup" function
**
** support function used as the "return to" address
** for user main() routines; it just calls exit(%eax)
*/

        .globl  exit_helper
exit_helper:
        pushl   %eax    // use whatever was in EAX as the status
        call    exit    // terminate this process

