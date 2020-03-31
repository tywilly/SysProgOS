/*
** SCCS ID: @(#)kmem.c	1.1 3/30/20
**
** File:    kmem.c
**
** Author:  Warren R. Carithers
**
** Based on code written by Kenneth Reek and the 4003-506 class of 20013.
**
** Description: Functions to perform dynamic memory allocation in the OS.
**      NOTE: these should NOT be called by user processes!
**
** This allocator functions as a simple "slab" allocator; it allows
** allocation of either 4096-byte ("page") or 1024-byte ("slice") chunks
** of memory from the free pool.  The free pool is initialized using the memory
** map provided by the BIOS during the boot sequence, and contains a series
** of blocks which are multiples of 4K bytes and which are aligned at 4K
** boundaries; they are held in the free list in order by base address.
** 
** The "page" allocator allows allocation of one or more 4K blocks at
** a time.  Requests are made for a specific number of 4K pages; the
** allocator locates the first free list entry that contains at least the
** requested amount of space.  If that entry is the exact size requested,
** it is unlinked and returned; otherwise, the entry is split into a chunk
** of the requested size and the remainder.  The chunk is returned, and the
** remainder replaces the original block in the free list.  On deallocation,
** the block is inserted at the appropriate place in the free list, and
** physically adjacent blocks are coalesced into single, larger blocks.
** If a multi-page block is allocated, it should be deallocated one page
** at a time, because there is no record of the size of the original
** allocation - all we know is that it is N*4K bytes in length, so it's
** up to the requesting code to figure this out.
** 
** The "slice" allocator operates by taking blocks from the "page"
** allocator and splitting them into four 1K slices, which it then manages.
** Requests are made for slices one at a time.  If the free list contains
** an available slice, it is unlinked and returned; otherwise, a page is
** requested from the page allocator, split into slices, and the slices
** are added to the free list, after which the first one is returned.
** The slice free list is a simple linked list of these 1K blocks; because
** they are all the same size, no ordering is done on the free list, and
** no coalescing is performed.
*/

#define __SP_KERNEL__

#include "common.h"
#include "klib.h"

#include <x86arch.h>
#include "bootstrap.h"
#include "kmem.h"
#include "cio.h"

/*
** PRIVATE DEFINITIONS
*/

// parameters related to word and block sizes

#define WORD_SIZE       sizeof(int)
#define LOG2_OF_WORD_SIZE   2

#define LOG2_OF_PAGE_SIZE   12

#define LOG2_OF_SLICE_SIZE  10

// converters:  pages to bytes, bytes to pages

#define P2B(x)          ((x) << LOG2_OF_PAGE_SIZE)
#define B2P(x)          ((x) >> LOG2_OF_PAGE_SIZE)

/*
** Name:    adjacent
**
** Arguments:   addresses of two blocks
**
** Description: Determines whether the second block immediately follows
**      the first one.
*/
#define adjacent(first,second)  \
    ( (void *) (first) + P2B((first)->pages) == (void *) (second) )

/*
** PRIVATE DATA TYPES
*/

/*
** This structure keeps track of a single block of memory.  All blocks are
** multiples of the base size (currently, 4KB).
*/

typedef struct blkinfo_s {
    uint32 pages;       // length of this block, in pages
    struct blkinfo_s *next; // pointer to the next free block
} Blockinfo;

/*
** Memory region information returned by the BIOS
**
** This data consists of a 32-bit integer followed
** by an array of region descriptor structures.
*/

typedef union b64_u {
    uint32 part[2];
    uint64 all;
} B64;

#define LOW part[0]
#define HIGH    part[1]

typedef struct memregion_s {
    B64 base;       // base address
    B64 length;     // region length
    uint32 type;    // type of region
    uint32 acpi;    // ACPI 3.0 info
} __attribute__((packed)) Region;

/*
** Region types
*/

#define REGION_USABLE       1
#define REGION_RESERVED     2
#define REGION_ACPI_RECL    3
#define REGION_ACPI_NVS     4
#define REGION_BAD      5

/*
** ACPI 3.0 bit fields
*/

#define REGION_IGNORE       0x01
#define REGION_NONVOL       0x02

/*
** 32-bit and 64-bit address values as 64-bit literals
*/

#define ADDR_BIT_32 0x0000000100000000LL
#define ADDR_LOW_HALF   0x00000000ffffffffLL
#define ADDR_HIGH_HALR  0xffffffff00000000LL

#define ADDR_32_MAX ADDR_LOW_HALF
#define ADDR_64_FIRST   ADDR_BIT_32

/*
** PRIVATE GLOBAL VARIABLES
*/

static Blockinfo *_free_pages;
static Blockinfo *_free_slices;

/*
** IMPORTED GLOBAL VARIABLES
*/

extern int _end;    // end of the BSS section - provided by the linker

/*
** FUNCTIONS
*/

/*
** FREE LIST MANAGEMENT
*/

/*
** Name:    _add_block
**
** Description: Add a block to the free list
** Arguments:   Base address of the block and its length in bytes
*/
static void _add_block( uint32 base, uint32 length ) {
    Blockinfo *block;

    // don't add it if it isn't at least 4K
    if( length < PAGE_SIZE ) {
        return;
    }

    // only want to add multiples of 4K; check the lower bits
    if( (length & 0xfff) != 0 ) {
        // round it down to 4K
        length &= 0xfffff000;
    }

    // create the "block"

    block = (Blockinfo *) base;
    block->pages = B2P(length);
    block->next = NULL;

    /*
    ** We maintain the free list in order by address, to simplify
    ** coalescing adjacent free blocks.
    **
    ** Handle the easiest case first.
    */

    if( _free_pages == NULL ) {
        _free_pages = block;
        return;
    }

    /*
    ** Unfortunately, it's not always that easy....
    **
    ** Find the correct insertion spot.
    */

    Blockinfo *prev, *curr;

    prev = NULL;
    curr = _free_pages;

    while( curr && curr < block ) {
        prev = curr;
        curr = curr->next;
    }

    // the new block always points to its successor
    block->next = curr;

    /*
    ** If prev is NULL, we're adding at the front; otherwise,
    ** we're adding after some other entry (middle or end).
    */

    if( prev == NULL ) {
        // sanity check - both pointers can't be NULL
        assert( curr );
        // add at the beginning
        _free_pages = block;
    } else {
        // inserting in the middle or at the end
        prev->next = block;
    }
}

/*
** Name:    _kmem_init
**
** Description: Find what memory is present on the system and construct
**      the list of free memory blocks.
*/
void _kmem_init( void ) {
    int32 entries;
    Region *region;
    uint64 cutoff;

    // initially, nothing in the free lists
    _free_slices = NULL;
    _free_pages = NULL;

    /*
    ** We ignore all memory below the end of our OS.  In theory,
    ** we should be able to re-use much of that space; in practice,
    ** this is safer.
    */

    // set our cutoff point as the end of the BSS section
    cutoff = (uint32) &_end;

    // round it up to the next multiple of 4K (0x1000)
    if( cutoff & 0xfffLL ) {
        cutoff &= 0xfffff000LL;
    cutoff += 0x1000LL;
    }

    // get the list length
    entries = *((int32 *) MMAP_ADDRESS);

    // if there are no entries, we have nothing to do!
    if( entries < 1 ) {  // note: entries == -1 could occur!
        return;
    }

    // iterate through the entries, adding things to the freelist

    region = ((Region *) (MMAP_ADDRESS + 4));

    for( int i = 0; i < entries; ++i, ++region ) {

        /*
        ** Determine whether or not we should ignore this region.
        **
        ** We ignore regions for several reasons:
        **
        **  ACPI indicates it should be ignored
        **  ACPI indicates it's non-volatile memory
        **  Region type isn't "usable"
        **  Region is above the 4GB address limit
        **
        ** Currently, only "normal" (type 1) regions are considered
        ** "usable" for our purposes.  We could potentially expand
        ** this to include ACPI "reclaimable" memory.
        */

        if( ((region->acpi) & REGION_IGNORE) == 0 ) {
            continue;
        }

        if( ((region->acpi) & REGION_NONVOL) == 1 ) {
            continue;  // we'll ignore this, too
        }

        if( (region->type) != REGION_USABLE ) {
            continue;  // we won't attempt to reclaim ACPI memory (yet)
        }

        // OK, we have a "normal" memory region - verify that it's usable

        // ignore it if it's above 4GB
        if( region->base.HIGH != 0 ) {
            continue;
        }

        // grab the two 64-bit values to simplify things
        uint64 base   = region->base.all;
        uint64 length = region->length.all;

        // see if it's below our arbitrary cutoff point
        if( base < cutoff ) {

            // is the whole thing too low, or just part?
            if( (base + length) < cutoff ) {
                // it's all below the cutoff!
                continue;
            }

            // recalculate the length, starting at our cutoff point
            uint64 loss = cutoff - base;

            // reset the length and the base address
            length -= loss;
            base = cutoff;
        }

        // see if it extends beyond the 4GB boundary

        if( (base + length) > ADDR_32_MAX ) {

            // OK, it extends beyond the 32-bit limit; figure out
            // how far over it goes, and lop off that portion

            uint64 loss = (base + length) - ADDR_64_FIRST;
            length -= loss;
        }

        // we survived the gauntlet - add the new block

        uint32 b32 = base   & ADDR_LOW_HALF;
        uint32 l32 = length & ADDR_LOW_HALF;

        _add_block( b32, l32 );
    }

    // announce that we have completed initialization
    __cio_puts( " KMEM" );
}

/*
** Name:    _kmem_dump
**
** Description: Dump the current contents of the free list to the console
*/
void _kmem_dump( void ) {
    Blockinfo *block;

    __cio_printf( "&_free_pages=%08x\n", &_free_pages );

    for( block = _free_pages; block != NULL; block = block->next ) {
        __cio_printf(
            "block @ 0x%08x 0x%08x pages (ends at 0x%08x) next @ 0x%08x\n",
                block, block->pages, P2B(block->pages) + (uint32) block,
                block->next );
    }

}

/*
** PAGE MANAGEMENT
*/

/*
** Name:    _kalloc_page
**
** Description: Allocate a page of memory from the free list.  The count
**      parameter is the number of contiguous pages desired. If no
**      memory is available, NULL is returned.
*/
void *_kalloc_page( uint32 count ){

    // make sure we actually need to do something!
    if( count < 1 ) {
        return( NULL );
    }

    /*
    ** Look for the first entry that is large enough.
    */

    // pointer to the current block
    Blockinfo *block = _free_pages;

    // pointer to where the pointer to the current block is
    Blockinfo **pointer = &_free_pages;

    while( block != NULL && block->pages < count ){
        pointer = &block->next;
        block = *pointer;
    }

    // did we find a big enough block?
    if( block == NULL ){
        // nope!
        return( NULL );
    }

    // found one!  check the length

    if( block->pages == count ) {

        // exactly the right size - unlink it from the list

        *pointer = block->next;

    } else {

        // bigger than we need - carve the amount we need off
        // the beginning of this block

        // remember where this chunk begins
        Blockinfo *chunk = block;

        // how much space will be left over?
        int excess = block->pages - count;

        // find the start of the new fragment
        Blockinfo *fragment = (Blockinfo *) ( (uint8 *) block + P2B(count) );

        // set the length and link for the new fragment
        fragment->pages = excess;
        fragment->next  = block->next;

        // replace this chunk with the fragment
        *pointer = fragment;

        // return this chunk
        block = chunk;
    }

    return( block );
}

/*
** Name:    _kfree_page
**
** Description: returns a memory block to the list of available blocks,
**      combining it with adjacent blocks if they're present.
*/
void _kfree_page( void *block ){
    Blockinfo *used;
    Blockinfo *prev;
    Blockinfo *curr;

    /*
    ** Don't do anything if the address is NULL.
    */
    if( block == NULL ){
        return;
    }

    used = (Blockinfo *) block;

    /*
    ** CRITICAL ASSUMPTION
    **
    ** We assume that any multi-page block that is being freed will
    ** be freed one page at a time.  We make this assumption because we
    ** don't track allocation sizes.  We can't use the simple "allocate
    ** four extra bytes before the returned pointer" scheme to do this
    ** because we're managing pages, and the pointers we return must point
    ** to page boundaries, so we would wind up allocating an extra page
    ** for each allocation.
    **
    ** Alternatively, we could keep an array of addresses and block
    ** sizes ourselves, but that feels clunky, and would risk running out
    ** of table entries if there are lots of allocations (assuming we use
    ** a 4KB page to hold the table, at eight bytes per entry we would have
    ** 512 entries per page).
    **
    ** IF THIS ASSUMPTION CHANGES, THIS CODE MUST BE FIXED!!!
    */

    used->pages = 1;

    /*
    ** Advance through the list until current and previous
    ** straddle the place where the new block should be inserted.
    */
    prev = NULL;
    curr = _free_pages;

    while( curr != NULL && curr < used ){
        prev = curr;
        curr = curr->next;
    }

    /*
    ** At this point, we have the following list structure:
    **
    **   ....    BLOCK       BLOCK      ....
    **          (*prev)  ^  (*curr)
    **                   |
    **                "used" goes here
    **
    ** We may need to merge the inserted block with either its
    ** predecessor or its successor (or both).
    */

    /*
    ** If this is not the first block in the resulting list,
    ** we may need to merge it with its predecessor.
    */
    if( prev != NULL ){

        // There is a predecessor.  Check to see if we need to merge.
        if( adjacent( prev, used ) ){

            // yes - merge them
            prev->pages += used->pages;

            // the predecessor becomes the "newly inserted" block,
            // because we still need to check to see if we should
            // merge with the successor
            used = prev;

        } else {

            // Not adjacent - just insert the new block
            // between the predecessor and the successor.
            used->next = prev->next;
            prev->next = used;

        }

    } else {

        // Yes, it is first.  Update the list pointer to insert it.
        used->next = _free_pages;
        _free_pages = used;

    }

    /*
    ** If this is not the last block in the resulting list,
    ** we may (also) need to merge it with its successor.
    */
    if( curr != NULL ){

        // No.  Check to see if it should be merged with the successor.
        if( adjacent( used, curr ) ){

            // Yes, combine them.
            used->next = curr->next;
            used->pages += curr->pages;

        }
    }
}

/*
** SLICE MANAGEMENT
*/

/*
** Slices are 1024-byte fragments from pages.  We maintain a free list of
** slices for those parts of the OS which don't need full 4096-byte chunks
** of space (e.g., the QNode and Queue allocators).
*/

/*
** Name:        _carve_slices
**
** Description: Allocate a page and split it into four slices;  If no
**              memory is available, we panic.
*/
static void _carve_slices( void ) {
    void *page;

    // get a page
    page = _kalloc_page( 1 );

    // allocation failure is a show-stopping problem
    assert( page );

    // we have the page; create the four slices from it
    uint8 *ptr = (uint8 *) page;
    for( int i = 0; i < 4; ++i ) {
        _kfree_slice( (void *) ptr );
        ptr += SLICE_SIZE;
    }
}

/*
** Name:        _kalloc_slice
**
** Description: Allocate a slice of memory from the free list.  If no
**              memory is available, we panic.
*/
void *_kalloc_slice( void ) {
    Blockinfo *slice;

    // if we are out of slices, create a few more
    if( _free_slices == NULL ) {
        _carve_slices();
    }

    // take the first one from the free list
    slice = _free_slices;
    assert( slice);

    // unlink it
    _free_slices = slice->next;

    // make it nice and shiny for the caller
    __memclr( (void *) slice, SLICE_SIZE );

    return( slice );
}

/*
** Name:        _kfree_slice
**
** Description: Returns a slice to the list of available slices.
**
** We make no attempt to merge slices, as they are independent
** blocks of memory (unlike pages).
*/
void _kfree_slice( void *block ) {
    Blockinfo *slice = (Blockinfo *) block;

    // just add it to the front of the free list
    slice->pages = SLICE_SIZE;
    slice->next = _free_slices;
    _free_slices = slice;
}
