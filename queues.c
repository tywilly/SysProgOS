/*
** SCCS ID: @(#)queues.c	1.1 3/30/20
**
** File:    queues.c
**
** Author:  CSCI-452 class of 20195
**
** Contributor:
**
** Description: Implementation of the queue module
*/

#define __SP_KERNEL__

#include "common.h"
#include "queues.h"
#include "process.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** Queue organization
** ------------------
** Our queues are self-ordering, generic queues.  A Queue can contain
** any type of data.  This is accomplished through the use of intermediate
** nodes called QNodes, which contain a void* data member, allowing them
** to point to any type of integral data (integers, pointers, etc.).
** 
** Each Queue has associated with it a comparison function, which may be
** NULL.  Insertions into a Queue are handled according to this function.
** If the function pointer is NULL, the queue is FIFO, and the insertion
** is always done at the end of the queue.  Otherwise, the insertion is
** ordered according to the results from the comparison function.
*/

// queue nodes
typedef struct qnode_s {
    void *data;         // what's in this entry
    struct qnode_s *next;   // link to next queue entry
} QNode;

// the Queue itself
//
// the Queue type is defined in the header file, as "Queue" must be visible
// to the outside world; however, the Queue contents are not visible
struct queue_s {
    QNode *head;        // first element
    QNode *tail;        // last element
    int (*cmp)( const void *, const void * );   // how to compare entries
    uint32 length;      // current occupancy count
};

/*
** PRIVATE GLOBAL VARIABLES
*/

// the list of free qnodes
static QNode *_free_qnodes;

// the list of free queue structures
// (same as Queue, but defined this way for clarity)
static struct queue_s *_free_queues;

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

// forward declaration
static bool _qnode_setup( bool critical );

//
// _qnode_alloc() - allocate a QNode
//
// Returns:
//    a pointer to the allocated QNode on success, else a NULL pointer
//
static QNode *_qnode_alloc( void ) {
    QNode *new;

    // if there aren't any, allocate another slab of 'em
    if( _free_qnodes == NULL ) {
        // if the creation fails, report back to the caller
        if( !_qnode_setup(false) ) {
            return( NULL );
        }
    }

    // take the first one off the free list
    new = _free_qnodes;
    _free_qnodes = new->next;

    // make sure it isn't pointing back to the list
    new->next = NULL;

    return( new );
}

//
// _qnode_free() - deallocate a QNode
//
// Parameters:
//    node    a pointer to the QNode to be deallocated
//
// Panics if an attempt is made to deallocate a NULL pointer
//
static void _qnode_free( QNode *node ) {

    // attempting to free a NULL pointer is a capital offense!
    assert( node );

    // do this the easy way - add it to the front of the free list
    node->next = _free_qnodes;
    node->data = NULL;
    _free_qnodes = node;
}

//
// _qnode_setup() - allocate a slice of memory and turn it into QNodes
//
// Parameters:
//    critical   - true if this is a critical allocation, else false
//
// Returns:
//    true on success, else false
//
// If this is a critical allocation, we panic on allocation failure;
// otherwise, we simply return "false" to indicate the failure.
//
static bool _qnode_setup( bool critical ) {
    QNode *block;

    // get a slice of free memory
    block = (QNode *) _kalloc_slice();
    if( block == NULL ) {

        // the allocation failed - should we give up?
        if( critical ) {
            // this is guaranteed to fail
            assert( block );
        }

        // no; report the failure
        return( false );
    }

    // free the qnodes!
    for( int i = 0; i < (SLICE_SIZE/sizeof(QNode)); ++i ) {
        // N.B.: if the slice size is not an integral multiple of
        // the QNode size, this will leave a small internal fragment
        // at the end of the allocated block of memory
        _qnode_free( block + i );
    }

    // all done - report our success
    return( true );
}

//
// _queue_setup() - allocate a slice of memory and turn it into Queues
//
// This is essentially identical to _qnode_setup(), but for Queues
// instead of QNodes.
//
// Parameters:
//    critical   - true if this is a critical allocation, else false
//
// Returns:
//    true on success, else false
//
// If this is a critical allocation, we panic on allocation failure;
// otherwise, we simply return "false" to indicate the failure.
//
static bool _queue_setup( bool critical ) {
    struct queue_s *block;

    block = (struct queue_s *) _kalloc_slice();

    // if this is a critical allocation and block is NULL, we're done for
    assert( !critical || block );

    // whew - not critical; NULL block is still a problem, though
    if( block == NULL ) {
        return( false );
    }

    // OK, we have a block; split it into Queues and free them
    for( int i = 0; i < (SLICE_SIZE/sizeof(struct queue_s)); ++i ) {
        _queue_free( block + i );
    }

    return( true );
}

/*
** PUBLIC FUNCTIONS
*/

//
// Queue management
//

//
// _queue_init() - initialize the queue module
//
void _queue_init( void ) {

    // set up qnode list
    _free_qnodes = NULL;
    _qnode_setup( true );

    // set up queue list
    _free_queues = NULL;
    _queue_setup( true );

    // create the queues for the OS
    _waiting = _queue_alloc( NULL );
    assert( _waiting );

    _reading = _queue_alloc( NULL );
    assert( _reading );

    _zombie = _queue_alloc( NULL );
    assert( _zombie )

    _sleeping = _queue_alloc( _wakeup_cmp );
    assert( _sleeping );

    // report that we are ready
    __cio_puts( " QUEUE" );
}

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
Queue _queue_alloc( int (*cmp)(const void*,const void *) ) {
    Queue new;

    // if there are no free Queues, create another batch of them
    if( _free_queues == NULL ) {
    // an allocation failure here is not critical
        if( !_queue_setup(false) ) {
            return( NULL );
        }
    }

    // remove the first entry from the free list
    new = _free_queues;
    _free_queues = (struct queue_s *) (new->head);

    // make sure it's empty
    new->head = new->tail = NULL;
    new->length = 0;

    // set up the ordering for the queue
    new->cmp = cmp;

    // return it to the caller
    return( new );
}

//
// _queue_free() - deallocate a Queue
//
// Parameters:
//    q    the Queue to be deallocated
//
// Reports attempts to deallocate a NULL pointer, but doesn't panic
//
void _queue_free( Queue q ) {

    if( q == NULL ) {
        __sprint( b256, "NULL ptr from 0x%08x\n", __get_ra() );
        WARNING( b256 );
        return;
    }

    // overload the 'head' pointer
    q->head = (QNode *) _free_queues;
    _free_queues = (struct queue_s *) q;
}

//
// Queue manipulation
//

//
// _queue_enque() - add something to a queue
//
// Parameters:
//    queue   the Queue to be manipulated
//    data    value to add to the queue
//
// Returns:
//    a status indicating the result of the operation
//
Status _queue_enque( Queue queue, void *data ) {

    // sanity: NULL queue?
    if( queue == NULL ) {
        return( E_PARAM );
    }

    // we'll need a QNode
    QNode *qn = _qnode_alloc();
    if( qn == NULL ) {
        return( E_SPACE );
    }

    // OK, we have the QNode; fill in the data field
    qn->data = data;

    // if the queue is empty, this will become the first node
    if( queue->length == 0 ) {
        queue->head = queue->tail = qn;
        queue->length = 1;
        return( SUCCESS );
    }

    // sanity: the queue is not empty, so there must already
    // be a first node - verify that this is the case
    assert2( queue->head != 0 && queue->tail != 0 );

    // if there is no comparison function, this is a FIFO queue
    if( queue->cmp == 0 ) {
        queue->tail->next = qn;
        queue->tail = qn;
        queue->length += 1;
        return( SUCCESS );
    }

    // OK, this is an ordered queue; locate the insertion point
    QNode *prev, *curr;

    prev = NULL;
    curr = queue->head;

    // iterate until we either run out of nodes, or
    // we find the node that should follow the new one
    while( curr && queue->cmp(data,curr->data) >= 0 ) {
        prev = curr;
        curr = curr->next;
    }

    // curr prev
    //   0    0   -> can't happen
    //   0   !0   -> add at end of queue
    //  !0    0   -> add at beginning
    //  !0   !0   -> add in middle

    // sanity:  at least one pointer must not be NULL
    //         curr || prev
    assert2( !(curr == NULL && prev == NULL) );

    // curr always points to the successor, even if it's NULL
    qn->next = curr;

    // prev tells us whether or not there's a predecessor
    if( prev == NULL ) {
    // nope - this is the new head node
        queue->head = qn;
    } else {
    // yes - we have a predecessor
        prev->next = qn;
    // if no successor, we're also the tail node
        if( curr == NULL ) {
            queue->tail = qn;
        }
    }

    // one more thing in the list
    queue->length += 1;

    return( SUCCESS );
}

//
// _queue_deque() - remove the first item from a queue
//
// Parameters:
//    queue   the queue to be manipulated
//
// Returns:
//    a pointer to the removed item, or NULL
//
void *_queue_deque( Queue queue ) {

    // sanity:  NULL queue pointer is a Bad Thing(tm)
    assert2( queue );

    // is there anything to remove?
    if( queue->length == 0 ) {
        return( NULL );
    }

    // OK, we have at least one element; remember it
    QNode *qn = queue->head;

    // unlink the head node
    queue->head = qn->next;

    // remember the data from this node, and free the node
    void *data = qn->data;
    _qnode_free( qn );

    // we have shortened the queue
    queue->length -= 1;

    // if the queue is now empty, make sure the head 
    // and tail pointers are both NULL
    if( queue->length == 0 ) {
        queue->tail = NULL;
        assert2( queue->head == NULL );
    }

    // send the removed data back
    return( data );
}

//
// _queue_remove() - remove a specific entry from a queue
//
// Parameters:
//    queue   the queue to be manipulated
//    data    the value to be located and removed
//
// Returns:
//    the data field from the removed item, or NULL
//
void *_queue_remove( Queue queue, void *data ) {
    QNode *curr, *prev;
    
    // sanity check!
    assert1( queue );
    
    // can't get blood from a stone, as they say
    if( queue->length < 1 ) {
        return( NULL );
    }
    
    // find this entry in the queue
    //
    // this is a basic singly-linked list removal

    prev = NULL;
    curr = queue->head;
    
    while( curr && curr->data != data ) {
        prev = curr;
        curr = curr->next;
    }
    
    // did we find it?
    if( !curr ) {
        // no - not in this queue
        return( NULL );
    }
    
    /*
    ** found it!  we have these possible situations:
    **
    **  prev  curr  next  situation
    **    *     0     *   can't happen
    **    0    !0     0   removing only node
    **    0    !0    !0   removing first node
    **   !0    !0    !0   removing middle node
    **   !0    !0     0   removing last node
    **
    ** removing the only node is equivalent to removing *both* the
    ** first node *and* the last node, so we can handle it that way
    */

    if( prev ) {
        // middle or last node
        prev->next = curr->next;
    } else {
        // first node
        queue->head = curr->next;
    }
    
    if( curr->next == NULL ) {
        // last node
        queue->tail = prev;
    }
    
    // we've reduced the queue length
    queue->length -= 1;
    
    // reclaim the qnode
    _qnode_free( curr );
    
    // the data we found is the data we were looking for
    return( data );
}

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
void *_queue_front( Queue q ) {

    // sanity check!
    if( q == NULL ) {
        return( NULL );
    }

    // nothing in the queue -> no data
    if( q->length == 0 ) {
        return( NULL );
    }

    // return whatever was there
    return( q->head->data );
}

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
uint32 _queue_length( Queue q ) {

    // sanity check!
    if( q == NULL ) {
        return( 0 );
    }

    // report the facts
    return( q->length );
}

//
// Queue iteration
//
// We assume that the queue will not be modified while we're using an
// iterator to step through it.  If the OS is modified so that it is
// reentrant, that constraint may be violated.
//
// Our QIter type is just a void*, and points to the current QNode in
// the queue.  We do this because the QNode is a private type and isn't
// available outside this module, so we use void* as a generic type.
//

//
// _queue_start() - initialize a queue iterator
//
// Parameter:
//      queue   the Queue to be examined
//
// Returns:
//      a pointer to the first element in the Queue (really a
//      pointer to the first QNode in the Queue)
//
QIter _queue_start( Queue q ) {

    // NULL queue pointer means there's nothing in it
    if( q == NULL ) {
        return( NULL );
    }

    // first element in the queue
    return( q->head );
}

//
// _queue_current() - peek at the current element in an iteration
//
// Parameter:
//    iter   the iterator being used
//
// Returns:
//    a pointer to the current element in the queue
//
void *_queue_current( QIter iter ) {

    // NULL iterator means we've run off the end of the queue
    if( iter == NULL ) {
        return( NULL );
    }

    // return the current data value
    return( ( (QNode *) iter)->data );
}

//
// _queue_next() - return the current node's data, and advance the iterator
//
// Parameter:
//    iter   a pointer to the iterator being advanced
//
// Returns:
//    a pointer to the element just removed
//
void *_queue_next( QIter *iter ) {
    QNode *qn;

    // NULL iter means bad news!
    assert2( iter );

    // NULL iter contents means we're off the end of the queue
    if( *iter == NULL ) {
        return( NULL );
    }

    // remember the current node and its data
    qn = (QNode *) *iter;
    void *data = qn->data;

    // advance the iterator
    *iter = qn->next;

    // return the data from the node we just left
    return( data );
}

/*
** Debugging/tracing routines
*/

//
// _queue_dump(msg,que)
//
// dump the contents of the specified queue to the console
//
void _queue_dump( const char *msg, Queue q ) {

    // report on this queue
    __cio_printf( "%s: ", msg );
    if( q == NULL ) {
        __cio_puts( "NULL???\n" );
        return;
    }

    // first, the basic data
    __cio_printf( "head %08x tail %08x %d items",
                  (uint32) q->head, (uint32) q->tail, q->length );

    // next, how the queue is ordered
    if( q->cmp ) {
        __cio_printf( " cmp %08x\n", (uint32) q->cmp );
    } else {
        __cio_puts( " FIFO\n" );
    }

    // if there are members in the queue, dump the first five data pointers
    if( q->length > 0 ) {
        __cio_puts( " data: " );
        QNode *tmp;
        int i = 0;
        for( tmp = q->head; i < 5 && tmp != NULL; ++i, tmp = tmp->next ) {
            __cio_printf( " [%08x]", (uint32) tmp->data );
        }

        if( tmp != NULL ) {
            __cio_puts( " ..." );
        }

        __cio_putchar( '\n' );
    }
}
