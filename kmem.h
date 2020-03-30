/*
** SCCS ID:	%W%	%G%
**
** File:	kmem.h
**
** Author:	Kenneth Reek and the 4003-506 class of 20013
**
** Contributor:	Warren R. Carithers
**
** Description:	Structures and functions to support dynamic memory
**		allocation within the OS.
**
**		The list of free blocks is ordered by address to facilitate
**		combining freed blocks with adjacent blocks that are already
**		free.
**
**		All requests for memory are satisfied with blocks that are
**		an integral number of 4-byte words.  More memory may be
**		provided than was requested if the fragment left over after
**		the allocation would not be large enough to be useable.
*/

#ifndef	_KMEM_H_
#define	_KMEM_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// Slab and slice sizes, in bytes

#define	SLICE_SIZE	1024
#define	SLAB_SIZE	4096

// Page size, in bytes

#define PAGE_SIZE   SLAB_SIZE

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/*
** Name: _kmem_init
**
** Description:	Builds a free list of blocks from the information
**		provided by the BIOS to the bootstrap.
*/
void _kmem_init( void );

/*
** Name:	_kmem_dump
**
** Description:	Dump the current contents of the freelist to the console
*/
void _kmem_dump( void );

/*
** Functions that manipulate free memory blocks.
*/

/*
** Name:	_kalloc_page
**
** Description:	Dynamically allocates one or more pages from the free list.
** Arguments:	Number of contiguous pages desired
** Returns:	A pointer to a block of memory of that size, or NULL
**		if no space was available
*/
void *_kalloc_page( uint32 count );

/*
** Name:	_kfree_page
**
** Description:	Frees a previously allocated page of dynamic memory.
** Arguments:	A pointer to the block to be freed
*/
void _kfree_page( void *block );

/*
** Name:	_kalloc_slice
**
** Description:	Dynamically allocates a slice (1/4 of a page)
** Returns:	A pointer to a block of memory of that size, or NULL
**		if no space was available
*/
void *_kalloc_slice( void );

/*
** Name:	_kfree_slice
**
** Description:	Frees a previously allocated slice of dynamic memory.
** Arguments:	A pointer to the block to be freed
*/
void _kfree_slice( void *block );

#endif

#endif
