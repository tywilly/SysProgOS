/*
** SCCS ID: @(#)users.h	1.1 3/30/20
**
** File:    users.h
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
