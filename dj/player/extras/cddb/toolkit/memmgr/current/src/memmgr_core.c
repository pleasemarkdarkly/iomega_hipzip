/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
*/

/* for testing for now... */
#define 	DBG_ALLOC			0
#define 	DBG_LOTSA_ALLOC 	0
#define 	DBG_FREE			0
#define 	DBG_LOTSA_FREE		0

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include	GN_STRING_H
#include	GN_STDIO_H
#include <extras/cddb/gn_memory.h>
#include "memmgr_core.h"
#include <extras/cddb/gn_errors.h>

/*
 * Implements the core memory manager functions - allocation and freeing 
 * fixed and global blocks, etc.
 *
 * XXX: Still to do: 
 *
 *	- Implement heapcheck(), getfreeram(), maxblocksize(), etc.
 *	- Implement malloc-aligned()
 *	- Move to doubly-linked blocklist, remove abs_size, etc.
 *	- Break up this rapidly-growing-ungainly file into several
 *		(at the expense of static-ness of internals)
 */

/* How to compare two blocks for contiguousness */
#define BLOCKS_CONTIG(block1, block2)								\
																	\
	((block_header *)(((char *)(block1)) + block1->abs_size) ==	\
	 (block_header *)(block2))


/* Global list of heaps existing in system. */
static	heap_struct 	gHeapList[kMaxHeaps];
static	int 			gNumHeaps;

/*
 * Flag that gets set on successful initialization and cleared on shutdown 
 */
static gn_bool_t	gMemoryInitialized = GN_FALSE;

static	int 		totalHeapSize = 0;

#if defined(DEBUG)
#define		MEM_INIT_CHAR		0xDB
#else
#define		MEM_INIT_CHAR		0
#endif

#ifdef _BLD_TH_MEM_
gn_size_t	cur_heap_usage, peak_heap_usage;
gn_int32_t	cur_blocks, peak_blocks;
#endif

/*
 * Internal utility routines
 */

#if defined(_GN_LOG_MEM_) || defined(_BLD_TH_MEM_)

static gn_cstr_t
strip_path(gn_cstr_t filepath)
{
	gn_cstr_t	file;

	if (!filepath || !*filepath)
		return "unknown";

#if defined(PLATFORM_WINDOWS)
	file = strrchr(filepath, '\\');
#else
	file = strrchr(filepath, '/');
#endif

	if (!file)
		return filepath;
	return file + 1;
}
#endif

/*
 * initialize_global_heap()
 *
 * Called from gnmem_initialize(), this sets up the internal heap structures and 
 * initializes everything.
 */
static gn_error_t
initialize_global_heap(void* buffer, gn_size_t total_heap_size)
{
	heap_struct*	heap;
	block_header*	block;
	void*			ourbuffer;

	/* Everything is in one big heap */
	heap = &gHeapList[0];
	heap->id = kGlobalHeapID;
	heap->alloc_size = 0;
	heap->in_use_list.next = NULL;

	/* Ensure that the heap start is aligned properly.	This should
	 * always be so, but let's be safe...
	 */
	ourbuffer = (void *)ALIGN_VALUE(buffer);
	total_heap_size -= (gn_uchar_t *)ourbuffer - (gn_uchar_t *)buffer;

	/* check the total heap size against the block overhead requirements */
	if (total_heap_size <= BLOCK_OVERHEAD)
		return MEMERR_Nomem;

	/* make a single memory block out of this chunk */
	block = (block_header *)ourbuffer;
	block->abs_size = total_heap_size;
	block->alloc_size = total_heap_size - BLOCK_OVERHEAD;
	block->heap_id = heap->id;
	block->flags = 0;
		
	block->next = NULL;
	
	/* and put it on the heap */
	heap->free_list.next = block;
	
	/* we have 1 heap so far */
	gNumHeaps = 1;
	
	return ABSTERR_NoError;
}

/*
 * initialize_fixed_heap()
 *
 * Splits a chunk of memory into individual, fixed-sized chunks, and
 * initializes them all into a new heap.
 */
static gn_error_t
initialize_fixed_heap(void* buffer, gn_size_t num_blocks, gn_size_t abs_block_size)
{
	heap_struct*	heap;
	block_header*	block;
	gn_size_t		alloc_size;
	
	/* Do we have any heap structures available? */
	if (gNumHeaps == kMaxHeaps)
	{
		/* no we do not */
		print_memory_error(NULL, ABSTERR_MEM_No_free_heaps);
		return ABSTERR_MEM_No_free_heaps;
	}
	
	/* the size of the actual allocation */
	alloc_size = abs_block_size - BLOCK_OVERHEAD;
	
	/* Set up the heap header */
	heap = &gHeapList[gNumHeaps++];
	heap->id = (unsigned char)(gNumHeaps - 1);
	heap->alloc_size = alloc_size;
	heap->in_use_list.next = NULL;
	
	/* 
	 * Break the buffer up into individual blocks of the specified size,
	 * taking into account the block header and other overhead (guard bytes,
	 * alignment, etc.)
	 */
	heap->free_list.next = (block_header *)buffer;
	block = heap->free_list.next;
	while (num_blocks--)
	{
		block->abs_size = abs_block_size;
		block->alloc_size = alloc_size;
		block->heap_id = heap->id;
		block->flags = 0;
		MARK_FIXED(block);
				
		if (num_blocks)
		{
			/* link us to the next block in the list */
			buffer = (void *)((char *)buffer + abs_block_size);
			block->next = (block_header *)buffer;
			block = block->next;
		}
		else
		{
			/* all done, terminate list */
			block->next = NULL;
		}
	}
	
	return ABSTERR_NoError;
}

/*
 * find_block_heap()
 *
 * Given a block of memory, try to find the block is was allocated from.
 * This may be an invalid block, and will not have a valid heap
 * associated with it.
 */
static heap_struct*
find_block_heap(block_header* block)
{
	int 	i;
	
	/* Iteration # 1 - the braindead way */
	for (i = 0; i < gNumHeaps; i++)
	{
		if (gHeapList[i].id == block->heap_id)
		{
			/* here it is */
			/* TODO: make sure block is actually in list */
			return &gHeapList[i];
		}
	}
	
	/* nothing matching.  bummer. */
	return NULL;
}

/*
 * validate_block()
 *
 * Look at a block of memory, attempting to determine if it is a legal 
 * memory block and, if so, whether or not it has been corrupted.
 *
 * This will do different things in debug/release modes.
 */
static int
validate_block(block_header* block)
{
#ifdef	__GNMEM_DEBUG__

	if (!check_guard_bytes(block))
	{
		/* somebody stepped on it */
		return false;
	}
#else
	block = 0;
#endif

	/* happy happy */
	return true;
}

/*
 * find_prev_block()
 *
 * Walk the list of allocated blocks for this heap, looking for the one 
 * immediately before this one.
 */
static block_header*
find_prev_block(heap_struct* heap, block_header* block)
{
	block_header*	h;

	if (heap == NULL)
		return (NULL);
			
	h = &heap->in_use_list;
	
	/* in_use_list is a dummy, the first block is inuse->next */
	while (h != NULL)
	{
		if (h->next == block)
		{
			/* found it */
			break;
		}
		h = h->next;
	}
	
	return h;
}

/*
 * alloc_fixed_block()
 *
 * Attempts to allocate a block from one of the fixed-size heaps we
 * may or may not have available.
 */
static block_header*
alloc_fixed_block(heap_struct* heap)
{
	block_header*	block;

	/* make sure there's something here first */
	if (heap == NULL || heap->free_list.next == NULL)
	{
		return NULL;
	}
	
	/* The list is organized such that the first free block
	 * is always at the head.  Get the next block, not forgetting
	 * to update the free list.
	 */
	/* free_list is a dummy, the first block is inuse->next */
	block = heap->free_list.next;

	/* make sure we got something, eh? */
	if (block == NULL)
	{
		return NULL;
	}
	
	/* remove this from the freelist */
	heap->free_list.next = block->next; 
	
	/* track the heap this came from */
	block->heap_id = heap->id;
	
	/* mark this block as in use */
	MARK_INUSE(block);
		
	/* fill out the header */
	block->alloc_size = heap->alloc_size;
	block->abs_size = absBlockSize(block->alloc_size);
	block->heap_id = heap->id;
	
	/* link it into the inuse list */
	/* in_use_list is a dummy, the first block is inuse->next */
	block->next = heap->in_use_list.next;
	heap->in_use_list.next = block;

	/* OK, all done.  Pass it back */
	return block;
}

/*
 * free_fixed_block()
 *
 * Return a previously allocated to its heap.  Does verification to make
 * sure that this is a valid block for this heap.
 */
static gn_error_t
free_fixed_block(heap_struct* heap, block_header* block)
{
	block_header*	prev;
	
	/* sanity checkage */
	if (block == NULL || !IS_INUSE(block))
	{
		return ABSTERR_MEM_Invalid_mem;
	}
	
	/* Now that we're here, walk the inuse list to find the previous 
	 * block, if we're validating memory.
	 */
	prev = find_prev_block(heap, block);
	if (prev == NULL)
	{
		/* this block's not on the list - back behind the rope! */
		print_memory_error(block, ABSTERR_MEM_Invalid_mem);
		return ABSTERR_MEM_Invalid_mem;
	}

#ifdef	_BLD_TH_MEM_ 
	cur_heap_usage -= block->alloc_size;
	cur_blocks--;
#endif
	
	/* remove this block from the inuse list... */
	prev->next = block->next;
	block->next = NULL;
	
	/* ... and link it back into the free list */
	block->next = heap->free_list.next;
	heap->free_list.next = block;
	
	/* of course, we're no longer in use */
	MARK_FREE(block);
		
	totalHeapSize += block ->abs_size;

	return ABSTERR_NoError;
}

/*
 * split_block()
 *
 * Split a block into 2 pieces - the requested block and the remainder
 *
 */
static block_header*
split_block(block_header* block, gn_size_t allocSize)
{
	block_header*	remainder;
	gn_size_t		abs_size;
	
	/* how big will this new block be altogether? */
	abs_size = absBlockSize(allocSize);
	
	/* make the new header at the end of the requested block */
	remainder = (block_header *)((char *)block + abs_size);
	
	/* duplicate the existing header, including ptr to next free block */
	*remainder = *block;
	
	/* set the block's new size */
	block->alloc_size = allocSize;
	block->abs_size = abs_size;
	
	/* now setup the remainder to be the original size - this block's size */
	remainder->abs_size -= abs_size;
	remainder->alloc_size = remainder->abs_size - BLOCK_OVERHEAD;
	
	/* pass back the remainder block so we can update the freelist */
	return remainder;
}

/*
 * find_worst_fit_block()
 *
 * Find the block on this heap that will leave the biggest remaining block.
 * Pass back a pointer the the previous block in the list (for connecting
 * it to the block after the one we supposedly find).
 *
 */
static block_header*
find_worst_fit_block(heap_struct* heap, gn_size_t alloc_size)
{
	block_header*	block;
	block_header*	prev;
	block_header*	best_fit;
	gn_size_t		best_size;
	
	/* start with the head of the list */
	prev = &heap->free_list;
	block = prev->next;
	
	best_fit = NULL;
	best_size = 0;
	
	while (block != NULL)
	{
		if (block->alloc_size == alloc_size)
		{
			/*  This is an exact match. */
			return prev;
		}
		if (block->alloc_size > alloc_size && block->alloc_size > best_size)
		{
			/* this one's better than the one we had before */
			best_fit = prev;
			best_size = block->alloc_size;
		}
		
		prev = block;
		block = block->next;
	}
	
	/* If we didn't find anything, fail. */
	if (best_fit == NULL)
		return NULL;
	
	/* OK, it's good.  Pass back the pointer to the previous block. */
	return best_fit;
}	 


/*
 * find_best_free_block()
 *
 * Find and return the free block that best fits the requested size.
 * There are several algorithms that can be used:
 *
 *	- best fit (least waste)
 *	- worst fit (reduces fragmentation, leaves bigger blocks)
 *	- first fit (fast, reputed to reduce fragmentation in general usage)
 *
 */
static block_header*
find_best_free_block(heap_struct* heap, gn_size_t alloc_size)
{
	block_header*	block;
	block_header*	prev;
	
	/*
	 * find a free block that fits, here ***
	 */
	
	/*
	 * Current strategy is biggest-block-first, as the likely scenario
	 * is having many small blocks allocated, and this will help reduce
	 * fragmentation.
	 */
	prev = find_worst_fit_block(heap, alloc_size);
	
	/* nothing available?  that's a problem, huh? */
	if (prev == NULL)
	{
		return NULL;
	}
	
	/*
	 * Assume at this point 'prev' points to the previous block in the 
	 * chain, 'block' to the block of interest.
	 */
	block = prev->next;
	
#if DBG_ALLOC
	dprintf("find_best_free: found %d block\n", block->alloc_size);
#endif	
	
	/* split this block into the requested block and the remainder */
	if (block->alloc_size - alloc_size > BLOCK_OVERHEAD)
	{
		block_header*	remainder;
		
		/* split */
		remainder = split_block(block, alloc_size);
		
		/* update freelist: prev => remainder, remainder => block->next */
		prev->next = remainder;
		if (remainder != NULL)
		{
			remainder->next = block->next;
		}
	}
	else	/* block is exact size (or close), just update freelist */
	{
		 prev->next = block->next;
	}
	
	/* not necessary but I like it */
	block->next = NULL;
	
	/* give it up */
	return block;
}

/*
 * alloc_global_block()
 *
 * Allocate a block of memory from the global heap, as opposed to one
 * of the fixed size heaps.
 */
static block_header*
alloc_global_block(gn_size_t allocSize, heap_struct* heap)
{
	block_header*	block;

	/* make sure there's something here first */
	if (heap == NULL || heap->free_list.next == NULL)
	{
		return NULL;
	}

	/*
	 * Walk the free list, looking for the "best" fitting free block.
	 * "best fit" will be determined by one of various allocation 
	 * strategies.
	 */
	block = find_best_free_block(heap, allocSize);
	if (block == NULL)
	{
		/* we're out of memory */
		return NULL;
	}
	
	/* mark as in use and link into inuse list */
	MARK_INUSE(block);

	block->next = heap->in_use_list.next;
	heap->in_use_list.next = block;
	
	/* pass it back */
	return (block);
}

/*
 * coalesce_left()
 *
 * Attempt to combine this (free) block with one immediately to the 
 * left of it in memory.
 *
 * If the block to the left is not contiguous, do nothing.
 */
static int
coalesce_left(heap_struct* heap, block_header* block)
{
	block_header*	prev;
	block_header*	free_prev;

	for (prev = heap->free_list.next; prev != NULL; prev = prev->next)
	{
		if (BLOCKS_CONTIG(prev, block))
		{
			/* Yes, combine this block into the previous one. */
			
			/* 
			 * Combine the allocated and total sizes.  Note that
			 * both sizes will increase by the total size allocated 
			 * for this block.
			 */
			prev->alloc_size += block->abs_size;
			prev->abs_size += block->abs_size;
			
			/* remove this block from the chain */
			for (free_prev = &heap->free_list; free_prev != NULL; free_prev = free_prev->next )
			{
				if( free_prev->next == block )
				{
					free_prev->next = block->next;
					break;
				}
			}
			
			return true;
		}
	}
	
	/* not possible */
	return false;
}

/*
 * coalesce_right()
 *
 * Attempt to combine this (free) block with one immediately to the 
 * right of it in memory.
 *
 * If the block to the right is not contiguous, do nothing.
 */
static int
coalesce_right(heap_struct* heap, block_header* block)
{
	block_header*	next;
	block_header*	prev;

	for (next = heap->free_list.next, prev = &heap->free_list; 
		 next != NULL; 
		 prev = next, next = next->next)
	{
		if (BLOCKS_CONTIG(block, next))
		{
			/* Yes, combine the next block into this one. */
			
			/* 
			 * Combine the allocated and total sizes.  Note that
			 * both sizes will increase by the total size allocated 
			 * for that block.
			 */
			block->alloc_size += next->abs_size;
			block->abs_size += next->abs_size;
			
			/* next in chain will be the one after the one that no
			 * longer exists
			 */
			block->next = next->next;
			
			/* link in left */
			prev->next = block;
			
			return true;
		}
	}
	
	/* not possible */
	return false;
}	

/*
 * free_global_block()
 *
 * Return a previously allocated block to the global heap.
 */
static gn_error_t
free_global_block(heap_struct* heap, block_header* block)
{
	block_header*	prev;
	block_header*	next;
	
	/* sanity checks */
	if (block == NULL || !IS_INUSE(block))
	{
		print_memory_error(block, ABSTERR_MEM_Invalid_mem);
		return ABSTERR_MEM_Invalid_mem;
	}
	
	/* Now that we're here, walk the inuse list to find the previous 
	** block, if we're validating memory.
	*/
	prev = find_prev_block(heap, block);
	if (prev == NULL)
	{
		/* this block's not on the list - back behind the rope! */
		print_memory_error(block, ABSTERR_MEM_Invalid_mem);
		return ABSTERR_MEM_Invalid_mem;
	}
	
#ifdef	_BLD_TH_MEM_ 
	cur_heap_usage -= block->alloc_size;
	cur_blocks--;
#endif
	
	totalHeapSize += block->abs_size;

	/* remove this block from the inuse list... */
	prev->next = block->next;
	block->next = NULL;

	/* of course, we're no longer in use */
	MARK_FREE(block);
	
	/* 
	 * Combine contiguous blocks into one. There are three possibilities:
	 *	- the block is not contiguous with any other blocks
	 *	- there is a free block immediately before this one
	 *	- there is a free block immediately after this one
	 * (2) and (3) may both be in effect.
	 */

	/* this is the next free block in the chain */
	next = prev->next;

	/*
	 * Step 1: is there a free block immediately after this one?
	 */
	if (!coalesce_right(heap, block))
	{
		/* No it does not.	Just link this one in */
		block->next = heap->free_list.next;
		heap->free_list.next = block;
	}
	
	/*
	 * Step 2: does this block start immediately after the previous one?
	 */
	if (!coalesce_left(heap, block))
	{
		/* If it can't be combined to anything, do nothing! */
	}
	
	return ABSTERR_NoError;
}	


/*
 * Public routines, called by the outside world
 */

/*
 * gnmem_initialize()
 *
 * Initialize the memory manager.  This must be the first memory-related 
 * call in the program.
 */
gn_error_t
gnmem_initialize(void* heap_start, gn_size_t total_heap_size)
{
	gn_error_t	error = MEMERR_NoError;

	if ((heap_start == GN_NULL) || (total_heap_size == 0))
		return ABSTERR_InvalidArg;
	
	if (gMemoryInitialized == GN_TRUE)
		return MEMERR_Busy;

	/* initialize with this */
	error = initialize_global_heap(heap_start, total_heap_size);

	if (error == SUCCESS)
	{
		gMemoryInitialized = GN_TRUE;
		totalHeapSize = total_heap_size;
	}

	return error;
}

/* Shut down the memory manager when we're done with it */
gn_error_t gnmem_shutdown(void)
{
	if (!gMemoryInitialized)
	{
		return ABSTERR_MEM_Noinit;
	}
	
	/* in debug mode, print out a list of lost, strayed or stolen blocks */
	
	/* perhaps dump out corrupted blocks, too */
	
	gMemoryInitialized = GN_FALSE;
	
	totalHeapSize = 0;

	return ABSTERR_NoError;
}	

/*
 * gnmem_create_heap(gn_size_t alloc_size, int num_blocks)
 *
 * Create a new fixed-sized heap of 'num_blocks' many 'alloc_size' blocks.
 *
 */
gn_error_t
gnmem_create_heap(gn_size_t alloc_size, int num_blocks)
{
	gn_size_t		abs_size;
	void*			buffer;
	gn_error_t		ret_val;
	
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return ABSTERR_MEM_Noinit;
	}
	
	/* Ensure we allocate to an N-byte aligned offset */
	alloc_size = ALIGN_VALUE(alloc_size);
	
	/* 
	 * Allocate enough memory to make the blocks.  Each block will need
	 * to be the allocation size + the overhead (guard bytes, alignment
	 * offsets, etc.
	 */
	abs_size = alloc_size + BLOCK_OVERHEAD;
	buffer = gnmem_malloc(abs_size * num_blocks);
	if (buffer == NULL)
	{
		return ABSTERR_MEM_Nomem;
	}
	
	/* setup this chunk into individual blocks */
	ret_val = initialize_fixed_heap(buffer, num_blocks, abs_size);
	if (ret_val != ABSTERR_NoError)
	{
		/* out of heaps */
		gnmem_free(buffer);
	}
	
	return ret_val;
}

/* 
 * gnmem_malloc()
 *
 * Allocate memory from the system.  This will attempt to allocate the
 * memory first from a fixed-sized heap that matches the request size,
 * then from the global heap if there is no matching heap.
 */
#if defined(_BLD_TH_MEM_)
void*
_gnmem_malloc(gn_size_t size, gn_uint32_t line, gn_str_t file)
#else
void*
gnmem_malloc(gn_size_t size)
#endif	
{
	int 			i;
	block_header*	block;
	
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return NULL;
	}
	
	/* Ensure we allocate to an N-byte aligned offset */
	size = ALIGN_VALUE(size);
	
	/* look through the fixed heaps to see if one might match */
#if DBG_ALLOC
	dprintf("malloc: looking for %d bytes\n", size);
#endif	
	
	/* global heap is ID 0 */
	for (i = 1, block = NULL; i < gNumHeaps; i++)
	{
		if (gHeapList[i].alloc_size == size)
		{
			/* this one will do nicely */
			block = alloc_fixed_block(&gHeapList[i]);
			break;
		}
	}
	
#if DBG_ALLOC
	if (block != NULL)
		dprintf("malloc: found block from fixed size heap\n");
#endif	
	
	if (block == NULL)
	{
		/* 
		 * If we get here, either there is no matching heap or that 
		 * heap is full.  Use the global heap instead.
		 */
		block = alloc_global_block(size, &gHeapList[0]);
	}
	
	if (block == NULL)
	{
		print_memory_error(block, ABSTERR_MEM_Nomem);
		return NULL;
	}
	
	/* add the guard bytes, if so configured */
#ifdef	__GNMEM_DEBUG__
	add_guard_bytes(block);
#endif

#ifdef	_BLD_TH_MEM_ 
	cur_heap_usage += size;
	if (cur_heap_usage > peak_heap_usage) 
	{
		peak_heap_usage = cur_heap_usage;
	}
	
	cur_blocks++;
	if (cur_blocks > peak_blocks) 
	{
		peak_blocks = cur_blocks;
	}
#endif
#if defined(_BLD_TH_MEM_)
	block->file = strip_path(file);
	block->line = line;
#endif
	
	/* counts for debug */
	totalHeapSize -= block->abs_size;

	/* initialize memory to known value */
	gnmem_memset(MEM_PTR(block), MEM_INIT_CHAR, block->alloc_size);

	/* pass back a pointer to the allocated memory */
	return MEM_PTR(block);
}


/* 
 * Allocate memory that will have a long lifetime, typically the life of
 * the program.  The block will be allocated from the end of the heap.
 * Use of this call can help avoid fragmentation.
 */
void*
gnmem_malloc_fixed(gn_size_t size)
{
	  /*
	   * XXX: NOT YET IMPLEMENTED as FIXED
	   */
	  
	  return gnmem_malloc(size);
}

/* Reallocate the memory for the block */
#if defined(_BLD_TH_MEM_)
void*
_gnmem_realloc(void* ptr, gn_size_t new_size, gn_uint32_t line, gn_str_t file)
#else
void*
gnmem_realloc(void* ptr, gn_size_t new_size)
#endif
{
	heap_struct*	heap;
	block_header*	block;
	block_header*	prev;
	
	/*
	 * Change the size of the block.  If the new size is smaller, 
	 * return the unused portion to the proper heap.  If it's bigger,
	 * attempt to reallocate a new block of the requested size.
	 * If it's the same, do nothing.
	 */
	
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return NULL;
	}
	
	/* if ptr is NULL, allocate a new block */
	if (ptr == NULL)
	{
		return gnmem_malloc(new_size);
	}
	
#if DBG_ALLOC
	dprintf("realloc: resize from %d to %d\n", 
			BLOCK_HEADER(ptr)->alloc_size, new_size);
#endif	
	
	/* if the new size is 0, free the block */
	if (new_size == 0)
	{
		gnmem_free(ptr);
		return NULL;	
	}

	/* Ensure we allocate to an N-byte aligned offset */
	new_size = ALIGN_VALUE(new_size);
	
	/* find the header that corresponds to this block */
	block = BLOCK_HEADER(ptr);
	
	/* is it the same size? */
	if (new_size == block->alloc_size)
	{
		return ptr;
	}
	
	/* find our owning heap */
	heap = find_block_heap(block);
	if (heap == NULL)
	{
		/* nogo */
		print_memory_error(block, ABSTERR_MEM_Invalid_mem_heap);
		return (NULL);
	}
	
	/* make sure it's a legally allocated block */
	prev = find_prev_block(heap, block);
	if (prev == NULL)
	{
		/* no it is not */
		print_memory_error(block, ABSTERR_MEM_Invalid_mem);
		return (NULL);
	}
	
	/* make sure that nobody has stepped on it */
	if (!validate_block(block))
	{
		/* well now what do we do? */
		print_memory_error(block, ABSTERR_MEM_Corrupt);
		return NULL;
	}
	
#ifdef	_BLD_TH_MEM_ 
	cur_heap_usage += new_size - block->alloc_size;
	if (cur_heap_usage > peak_heap_usage) 
	{
		peak_heap_usage = cur_heap_usage;
	}
#endif
	
	/* counts for debug */
	totalHeapSize -= (new_size - block->alloc_size);

	/* Is the block to be made bigger? */
	/*
	 * XXX: make this smarter and look for free space immediately 
	 * after this block - can we extend?
	 */
	if (new_size > block->alloc_size)
	{
		void*	new_ptr;
		
		/* allocate a new block, copy old stuff into it, free old, return new */
		new_ptr = gnmem_malloc(new_size);
		if (new_ptr != NULL)
		{
#if defined(_BLD_TH_MEM_)
			block_header*	newblock;

			newblock = BLOCK_HEADER(new_ptr);
			if (newblock)
			{
				newblock->file = strip_path(file);
				newblock->line = line;
			}
#endif
			gnmem_memcpy(new_ptr, ptr, block->alloc_size);
			gnmem_free(ptr);
		}
		
		return new_ptr;
	}
	else	/* make it smaller */
	{
		/* 
		 * If this is a fixed size block, we can't change it.  But we 
		 * don't need to tell them that.
		 * 
		 * Also, if the new size will leave a block smaller than we can
		 * use (abs_size < some_useful_value), don't do anything.
		 */
		if (!IS_FIXED(block) && block->alloc_size - new_size > BLOCK_OVERHEAD)
		{
			block_header*	remainder;
			
			/* split this block in two */
			remainder = split_block(block, new_size);
			
			/* mark this chunk as now free */
			MARK_FREE(remainder);
			
			/* deal with this remainder - can we combine it with another? */
			if (!coalesce_right(heap, remainder))
			{
				/* nope, put it on the list */
				remainder->next = heap->free_list.next;
				heap->free_list.next = remainder;
			}

			/* re-add the guard bytes, if so configured */
#ifdef	__GNMEM_DEBUG__
			add_guard_bytes(block);
#endif
		}
	}
	
	return ptr;
}

/* Free the allocated block */
#if defined(_BLD_TH_MEM_)
gn_error_t
_gnmem_free(void* ptr, gn_uint32_t line, gn_str_t file)
#else
gn_error_t
gnmem_free(void* ptr)
#endif
{
	heap_struct*	heap;
	block_header*	block;

#if defined(_BLD_TH_MEM_)
	line = 0; file = NULL;
#endif
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return ABSTERR_MEM_Noinit;
	}
	
	/* is there anything at all? */
	if (ptr == NULL)
	{
		/* it's okay to ignore this without an error */
		return ABSTERR_NoError;
	}
	
	/* find the header that corresponds to this block */
	block = BLOCK_HEADER(ptr);
	
	/* make sure that nobody has stepped on it */
	if (!validate_block(block))
	{
		print_memory_error(block, ABSTERR_MEM_Corrupt);
	}
	
	/* find our owning heap */
	heap = find_block_heap(block);
	if (heap == NULL)
	{
		return ABSTERR_MEM_Invalid_mem_heap;
	}

	/* Is this a fixed size or a global block?	Free accordingly. */
	if (IS_FIXED(block))
	{
		return free_fixed_block(heap, block);
	}	
	else
	{
		return free_global_block(heap, block);
	}
}

#if defined(_BLD_TH_MEM_)

#undef	gnmem_malloc
#undef	gnmem_realloc
#undef	gnmem_free

void*
gnmem_malloc(gn_size_t size)
{
	return _gnmem_malloc(size, 0, NULL);
}

void*
gnmem_realloc(void* ptr, gn_size_t new_size)
{
	return _gnmem_realloc(ptr, new_size, 0, NULL);
}

gn_error_t
gnmem_free(void* ptr)
{
	return _gnmem_free(ptr, 0, NULL);
}
#endif


/* Copy memory from point A to point B, assuming NO overlap */
void*
gnmem_memcpy(void* dest, void* src, gn_size_t size)
{
#ifdef	GN_MEM_HAVE_MEMCPY

	if (dest == NULL || src == NULL)
		return dest;

	return memcpy(dest, src, size);
	
#else

	gn_size_t	ind = 0;

	if (dest != NULL && src != NULL)
	{
		/* do some work */
		for (ind = 0 ; ind < size ; ind += sizeof(gn_char_t)) 
		{
			((gn_char_t *)dest)[ind] = ((gn_char_t *)src)[ind];
		}
	}
	
	return dest;
		
#endif		
}

/* Copy memory from point A to point B, allowing for overlap */
void*
gnmem_memmove(void* dest, void* src, gn_size_t size)
{
#ifdef	GN_MEM_HAVE_MEMMOVE
	
	if (dest == NULL || src == NULL)
		return dest;
		
	return memmove(dest, src, size);
	
#else

	gn_size_t	ind = 0;
	
	if (dest != NULL && src != NULL)
	{
		/* TODO: do some work */
		for (ind = 0; ind < size ; ind += sizeof(gn_char_t))	
		{
			((gn_char_t *)dest)[ind] = ((gn_char_t *)src)[ind];
		}
	}
	
	return dest;
	
#endif		
}

/* Set a range of memory to a specific value */
void*
gnmem_memset(void* dest, int ch, gn_size_t count)
{
#ifdef	GN_MEM_HAVE_MEMSET

	if (dest == NULL)
		return dest;
		
	return memset(dest, ch, count);
	
#else

	gn_size_t	ind = 0;
	
	if (dest != NULL)
	{
		for (ind = 0 ; ind < count ; ind += sizeof(gn_char_t))	
		{
			((gn_char_t *) dest)[ind] = ch;
		}
	}
	
	return dest;	
	
#endif	
}

/* Compare two regions of memory */
int
gnmem_memcmp(const void* buff1, const void* buff2, gn_size_t count)
{
#ifdef	GN_MEM_HAVE_MEMCMP

	return memcmp(buff1, buff2, count);
	
#else

	gn_size_t	ind = 0;
	
	for (ind = 0; ind < count; ind += sizeof(gn_char_t))	
	{
		if (((gn_char_t *)buff1)[ind] != ((gn_char_t *)buff2)[ind])
			return ((gn_char_t *) buff1)[ind] - ((gn_char_t *) buff2)[ind];
	}

	return 0;
	
#endif	
}

/* Check if the heap is Ok */
gn_error_t
gnmem_checkheap(void)
{
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return ABSTERR_MEM_Noinit;
	}

	return (gnmem_int_checkheap(gHeapList, gNumHeaps));
}

/* Get the size of available RAM */
gn_size_t
gnmem_getfreeram()
{
	/* Have we even been initialized yet? */
	if (!gMemoryInitialized)
	{
		return (gn_size_t)-1;
	}
	
	return gnmem_int_getfreeram(gHeapList, gNumHeaps);
}

/* Find the largest block that can be allocated */
gn_size_t
gnmem_getmaxblksize(void)
{
	if (!gMemoryInitialized)
	{
		return (gn_size_t)-1;
	}
	
	return gnmem_int_getmaxblksize(gHeapList, gNumHeaps);
}

/* Get interesting information about heap usage */
gn_error_t 
gnmem_get_proc_size(proc_size_t *mem_info)
{
	if (!gMemoryInitialized || mem_info == NULL)
		return FAILURE;
	
	/* now then */
	gnmem_int_get_heap_info(mem_info, GHI_HEAP_INFO, gHeapList, gNumHeaps);
	return SUCCESS;
}

/* How big is this block of memory? */ 
gn_size_t gnmem_get_pointer_size(void * ptr)
{
	block_header *	block;
	
	if (!gMemoryInitialized || ptr == NULL)
	{
		return (gn_size_t)-1;
	}
	
	block = BLOCK_HEADER(ptr);
	
	return block->alloc_size;
}



/*
 * Debugging and verification routines.  These may be no-ops in 
 * release versions.
 */

/* Is this pointer a valid one? */
gn_bool_t gnmem_is_pointer_valid(void * ptr)
{
	block_header *	block;
	
	if (!gMemoryInitialized || ptr == NULL)
	{
		return GN_FALSE;
	}
	
	block = BLOCK_HEADER(ptr);
	if (block == NULL || !IS_INUSE(block))
	{
		return GN_FALSE;
	}
	
	/* Now that we're here, walk the inuse list to find a block that 
	** points to this one.
	*/
	return (gn_bool_t)(find_prev_block(find_block_heap(block), block) != NULL);
}

/*
 * Go through the list of both allocated and free blocks on the given
 * heap, checking to make sure all are valid.
 *
 * XXX: This needs a better output implementation
 */
void
gnmem_walk_heaps(void)
{
	/* Go through all heaps, global and fixed */
	int 	i;
	
	for (i = 0; i < gNumHeaps; i++)
	{
		walk_memory_heap(&gHeapList[i]);
	}
}

/* Building the test harness */
#if defined(_BLD_TH_MEM_)

/* Display the blocks which exist 
 */
void disp_blk_exist_imp()
{
	/* Go through all heaps, global and fixed */
	int				i;
	block_header*	block;
	heap_struct*	heap;

	for (i = 0; i < gNumHeaps; i++) 	{
		heap = &gHeapList[i];
		block = heap->in_use_list.next;
		while (block != NULL) {
			printf("Block to free: block [0x%0X]: heap id: %d flags: 0x%0X next: 0x%0X\n"
				"  ptr = 0x%0X, size = %d, abs size = %d\n File = %s, line = %d\n",
				block, block->heap_id, block->flags, block->next,
				MEM_PTR(block), block->alloc_size, block->abs_size, 
				block->file, block->line);

			block = block->next;
		}
	}
}
	
#endif /* _BLD_TH_MEM_ */


