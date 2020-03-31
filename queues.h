/*
** SCCS ID:	@(#)queues.h	1.1	3/30/20
**
** File:	queues.h
**
** Author:	CSCI-452 class of 20195
**
** Contributor:
**
** Description:	Queue module declarations
*/

#ifndef _QUEUES_H_
#define _QUEUES_H_

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

// The queue itself is an opaque type

typedef struct queue_s *Queue;

// We also provide a queue iterator

typedef void *QIter;

/*
** Globals
*/

/*
** Prototypes
*/

//
// Queue management
//

//
// _queue_init() - initialize the queue module
//
void _queue_init( void );

//
// _queue_alloc() - allocate a Queue
//
// Parameters:
//    cmp    comparison function to use if this is an ordered Queue,
//           else NULL (for a FIFO queue)
//
// Returns:
//    a pointer to the newly-allocated queue on success, else NULL
//
Queue _queue_alloc( int (*cmp)(const void*,const void*) );

//
// _queue_free() - deallocate a Queue
//
// Parameters:
//    q    the Queue to be deallocated
//
// Reports attempts to deallocate a NULL pointer, but doesn't panic
//
void _queue_free( Queue queue );

//
// Queue manipulation
//

//
// _queue_enque - add something to a queue
//
Status _queue_enque( Queue queue, void *data );

//
// _queue_deque - remove something from a queue
//
void *_queue_deque( Queue queue );

//
// _queue_remove - remove a specific entry from a queue
//
void *_queue_remove( Queue queue, void *entry );

//
// _queue_front() - peek at the front of a Queue
//
// Parameters:
//    q    the Queue to be examined
//
// Returns:
//    the data field from the first entry in the Queue, else NULL
//
// If the supplied Queue pointer is NULL or there is nothing in the
// Queue, we return a NULL pointer.  This is indistinguishable from
// the return value from a non-empty Queue if the first entry in
// that Queue is itself 0 or a NULL pointer.  The calling routine
// must verify that the Queue was not empty.
//
void *_queue_front( Queue queue );

//
// _queue_length() - return the occupancy count of a Queue
//
// Parameters:
//    q    the Queue to be examined
//
// Returns:
//    the number of elements in the Queue
//
// If a bad Queue pointer is supplied as the parameter, we return a
// count of 0.  There is no way for the calling routine to distinguish
// this from the length of an actual empty queue without checking the
// parameter it passed into this function.
//
uint32 _queue_length( Queue queue );

//
// Queue iteration
//

//
// _queue_start - initialize a queue iterator
//
QIter _queue_start( Queue queue );

//
// _queue_current - peek at the current element in an iteration
//
void *_queue_current( QIter iter );

//
// _queue_next - advance the iterator to the next element
//
void *_queue_next( QIter *iter );

/*
** Debugging/tracing routines
*/

//
// _queue_dump(msg,que)
//
// dump the contents of the specified queue to the console
//
void _queue_dump( const char *msg, Queue q );

#endif

#endif
