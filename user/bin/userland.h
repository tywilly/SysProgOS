/*
** SCCS ID: @(#)userland.h	1.1 3/30/20
**
** File:    userland.h
**
** Author:  Warren R. Carithers and various CSCI-452 classes
**
** Contributor:
**
** Description: control of user-level routines
*/

#ifndef _USERS_H_
#define _USERS_H_

/*
** General (C and/or assembly) definitions
*/

// delay loop counts

#define DELAY_LONG  100000000
#define DELAY_MED     4500000
#define DELAY_SHORT   2500000

#define DELAY_STD   DELAY_SHORT

// a delay loop

#define DELAY(n)    for(int _dlc = 0; _dlc < (DELAY_##n); ++_dlc) continue;

// maximum number of command-line arguments a process can have
// (not a true limit - used to dimension the argv[] arrays in
// the various user functions, for convenience)

#define MAX_COMMAND_ARGS    6

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** User process controls.
**
** The comment field of these definitions contains a brief description
** of the functionality of each user process.
**
** To spawn a specific user process from the initial process, uncomment
** its entry in this list.
*/

//
// Users W-Z are spawned from other processes; they
// should never be spawned directly by init().
//

//
// System call matrix
//
// System calls in this system:   exit, wait, kill, spawn, read, write,
//  sleep, gettime, getpid, getppid, getstate
//
// These are the system calls which are used in each of the user-level
// main functions.  Some main functions only invoke certain system calls
// when given particular command-line arguments (e.g., main6).
//
//                        baseline system calls in use
//  fcn   exit wait kill spawn read write sleep time pid ppid state bogus
// -----  ---- ---- ---- ----- ---- ----- ----- ---- --- ---- ----- -----
// main1    X    .    .    .     .    X     .    .    .    .    .     .
// main2    .    .    .    .     .    X     .    .    .    .    .     .
// main3    X    .    .    .     .    X     X    .    .    .    .     .
// main4    X    .    .    X     .    X     X    .    X    .    .     .
// main5    X    .    .    X     .    X     .    .    .    .    .     .
// main6    X    X    X    X     .    X     .    .    .    .    .     .
//
// userH    X    .    .    X     .    X     X    .    .    .    .     .
// userI    X    .    X    X     .    X     X    .    X    .    X     .
// userJ    X    .    .    X     .    X     .    .    X    .    .     .
// userP    X    .    .    .     .    X     X    X    .    .    .     .
// userQ    X    .    .    .     .    X     .    .    .    .    .     X
// userR    X    .    .    .     X    X     X    .    .    .    .     .
// userS    X    .    .    .     .    X     X    .    .    .    .     .
// userT    X    X    .    X     .    X     X    .    X    .    .     .
// userU    X    X    .    X     .    X     X    .    X    .    .     .
// userV    X    .    X    X     .    X     X    .    X    .    .     .
// userW    X    .    .    .     .    X     X    X    X    .    .     .
// userX    X    .    .    .     .    X     .    .    .    .    .     .
// userY    X    .    .    .     .    X     X    .    .    .    .     .
// userZ    X    .    .    .     .    X     X    .    X    X    .     .

/*
** Prototypes for externally-visible routines
*/

/*
** init - initial user process
**
** after spawning the other user processes, loops forever calling wait()
*/
int init( int argc, char *args );

/*
** idle - the idle process
**
** Reports itself, then loops forever delaying and printing a character.
*/
int idle( int argc, char *args );

#endif

#endif
