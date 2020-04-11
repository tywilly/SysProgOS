/*
** SCCS ID:     @(#)types.h	1.1 3/30/20
**
** File:    types.h
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: standard types used throughout the system
*/

#ifndef _TYPES_H_
#define _TYPES_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

// standard integer sized types

typedef char                    int8;
typedef unsigned char           uint8;
typedef short                   int16;
typedef unsigned short          uint16;
typedef int                     int32;
typedef unsigned int            uint32;
typedef long long int           int64;
typedef unsigned long long int  uint64;

// a Boolean type and its values

typedef uint8   bool;

#define true    1
#define false   0

// Process states

enum state_e {
    UNUSED = 0, NEW, READY, RUNNING, SLEEPING, WAITING, BLOCKED, ZOMBIE
};

typedef uint8 State;

#define VALID_STATE(n)  ((n) >= UNUSED && (n) <= ZOMBIE)

// Process IDs

typedef uint16 Pid;

// Time values

typedef uint64 Time;

// a Status type and its values

typedef int Status;

// NULL pointer
#define NULL 0

// Error return values

#define SUCCESS         (0)
#define E_SUCCESS       SUCCESS

#define E_ARGS_TOO_LONG (-1)
#define E_BAD_CHANNEL   (-2)
#define E_BAD_SYSCALL   (-3)
#define E_INVALID       (-4)
#define E_KILLED        (-5)
#define E_MAX_PROCS     (-6)
#define E_NOT_FOUND     (-7)
#define E_NO_CHILDREN   (-8)
#define E_NO_DATA       (-9)
#define E_NO_MEMORY     (-10)
#define E_PARAM         (-11)
#define E_SPACE         (-12)

#endif

#endif
