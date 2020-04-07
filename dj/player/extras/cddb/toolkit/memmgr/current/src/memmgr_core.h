/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * memmgr_core.h
 *
 * Internal structures and definitions, etc. for Gracenote memory manager code.
 *
 */

#ifndef	__MEMMGR_CORE_H__
#define	__MEMMGR_CORE_H__

#include	<extras/cddb/gn_platform.h>

/* enable debug stuff */
/* #define		__GNMEM_DEBUG__ */

/* general type definitions */
#include <extras/cddb/gn_memory.h>

/* General structure of memory manager internals:
 *
 *	heaps			are lists of blocks
 *	blocks			are chunks of memory, allocated or free
 *	free_lists		are singly-linked lists of free blocks, owned by heaps
 *	in_use_lists	are singly-linked lists of allocated blocks, owned by heaps
 *
 */

/*
** typedefs we use, reinventing the wheel yet again
*/
typedef		unsigned char	byte;

/*
 * error codes
 */

/* XXX MAKE THESE match API standards... */

#define	ERR_SUCCESS					0
#define	ERR_MEM_NOT_INITIALIZED		1
#define	ERR_INVALID_MEM				2
#define	ERR_INVALID_MEM_HEAP		3
#define	ERR_NO_MEM					4
#define	ERR_NO_FREE_HEAPS			5
#define	ERR_INVALID_HEAP_SIZE		6
#define	ERR_BLOCK_CORRUPTED			7
#define	ERR_HEAP_CORRUPTED			8


/* Leading and trailing guard bytes are based on the version:
** Debugging versions have them, release versions probably don't.
*/										   
/**
** XXX: This should probaly be configurable 
**/
#ifdef	__GNMEM_DEBUG__
	#define     kLeadingGuardBytes	    8
	#define     kTrailingGuardBytes		8
	
	#define		kGuardByteValue			0xbeefcafe
#else
	#define     kLeadingGuardBytes		0
	#define     kTrailingGuardBytes		0
#endif

/*
** Some CPUs will require different, interesting alignments for memory 
** blocks.  Take them into account.
**
** This should be specified in platform.h (or an analogue)
*/
#ifndef	GN_MEM_POINTER_ALIGNMENT_OFFSET

	#define			GN_MEM_POINTER_ALIGNMENT_OFFSET	1	/* no alignment necessary */
	
#endif

/* Macro to simplify rounding alloc values to a multiple of the block size */
#define		ALIGN_VALUE(val)		\
	(((unsigned int)val + GN_MEM_POINTER_ALIGNMENT_OFFSET - 1) &~ (GN_MEM_POINTER_ALIGNMENT_OFFSET - 1))
	
/*
 * memory block header
 */
typedef struct block_header
{
	size_t			alloc_size;		/* size of allocated block of memory */
	size_t			abs_size;		/* total size of this structure, excluding guard bytes */
	byte			heap_id;		/* index of heap that owns this block */
	byte			flags;			/* various options, see below */
	struct block_header* next;		/* next block in [free|in_use] list */
#ifdef _BLD_TH_MEM_
	gn_cstr_t		file;			/* The task that allocates the block */
	gn_size_t		line;			/* The routine that allocates the block */
#endif /* _BLD_TH_MEM_ */
} block_header;

#if	(GN_MEM_POINTER_ALIGNMENT_OFFSET == 1)
	#define		kBlockHeaderPadding		0
#else
	#define		kBlockHeaderPadding		\
		(GN_MEM_POINTER_ALIGNMENT_OFFSET - 	\
			((sizeof(block_header) + kLeadingGuardBytes) % GN_MEM_POINTER_ALIGNMENT_OFFSET))
#endif

/*
 * flags that can be set in a memory block header 
 */
#define		MEM_INUSE		0x01
#define		MEM_FIXED		0x02
#define		MEM_IGNORE		0x10

/* 
 * useful macros
 */

/* size of the stuff before the returned pointer */
#define		BLOCK_HEADER_SIZE	(sizeof(block_header) 	+	\
								 kLeadingGuardBytes		+	\
								 kBlockHeaderPadding)
								 
/* make a user's memory block from a block we manage */
#define		MEM_PTR(block)		((void *)((char *)block + BLOCK_HEADER_SIZE))

/* find the block header for this pointer */
#define		BLOCK_HEADER(ptr)	((block_header *)((char *)ptr - BLOCK_HEADER_SIZE))

/* The number of bytes used by the block header, guard bytes, etc. */
#define		BLOCK_OVERHEAD		(sizeof(block_header) 	+ 	\
								kLeadingGuardBytes		+	\
								kTrailingGuardBytes		+	\
								kBlockHeaderPadding)

/* Total size of a block, based on allocation size */
#define		absBlockSize(alloc_size)	(alloc_size + BLOCK_OVERHEAD)

/* Mark a block as in use */
#define		MARK_INUSE(block)	(block->flags |= MEM_INUSE)

/* Mark a block as free */
#define		MARK_FREE(block)	(block->flags &= ~MEM_INUSE)

/* Is this block in use? */
#define		IS_INUSE(block)		(block->flags & MEM_INUSE)

/* This block is a fixed block */
#define		MARK_FIXED(block)	(block->flags |= MEM_FIXED)

/* Is this block fixed? */
#define		IS_FIXED(block)		(block->flags & MEM_FIXED)

/* Ignore this block in heap dumps */
#define		MARK_IGNORE(block)	(block->flags |= MEM_IGNORE)

/* Don't ignore this block in heap dumps */
#define		MARK_UNIGNORE(block)	(block->flags &= ~MEM_IGNORE)

/* Should this block be ignored? */
#define		IS_RELEVANT(block)		(!(block->flags & MEM_IGNORE))

/*
 * heap structure
 */
typedef struct
{
	byte			id;				/* identifies this heap, block points here */
	size_t			alloc_size;		/* size of requests this heap services */
	block_header 	free_list;		/* list of blocks not currently in use */
	block_header 	in_use_list;	/* list of blocks that are in use */

} heap_struct;

/*
 * Define the ID of the global heap, i.e. the one used for non-fixed sized allocations
 */
#define		kGlobalHeapID			0

/*
 * Global list of heaps owned by the system
 *
 * Redeclare this up or down in platform.h.  The minimum value is 1.
 *
 * XXX - will this be sufficient as a default or will it need to grow?
 *     - or is it overkill?
 *     - or is it underkill?
 *
 */
#ifndef	kMaxHeaps
	#define		kMaxHeaps				32
#endif

/*
 * Other useful definitions
 */

/* as this is C, not C++ */
#define		false					0
#define		true					(!false)



/*
 * Prototypes for internal routines
 */

/* in memmgr_diag.c: */

/* flags passed to get_heap_info: */
#define		GHI_FREE_BLOCKS			0x01
#define		GHI_FREE_MEM			0x02
#define		GHI_MAX_FREE_BLOCK		0x04
#define		GHI_USED_BLOCKS			0x10
#define		GHI_USED_MEM			0x20
#define		GHI_MAX_USED_BLOCK		0x40

#define		GHI_FREE_INFO			0x0F
#define		GHI_USED_INFO			0xF0
#define		GHI_HEAP_INFO			(GHI_USED_INFO | GHI_FREE_INFO)

/* Get information on the state of memory */
void 		gnmem_int_get_heap_info(proc_size_t * mem_info, int action,
									heap_struct heap_list[], int num_heaps);

/* Get the size of available RAM */
gn_size_t 	gnmem_int_getfreeram(heap_struct heap_list[], int num_heaps);

/* Find the largest block that can be allocated */
gn_size_t 	gnmem_int_getmaxblksize(heap_struct heap_list[], int num_heaps);

/* Check the heap for corruption or the lack thereof */				   
gn_error_t 	gnmem_int_checkheap(heap_struct heap_list[], int num_heaps);
				   
/* in memmgr_debug.c: */
/*
 * Add the appropriate set of guard bytes at the beginning and end of this
 * block of memory, so we can see if anyone steps on it.
 */
void		add_guard_bytes(block_header * block);

/*
 * Look and see if anybody's stepped on the guard bytes we set before.
 */
int			check_guard_bytes(block_header * block);

/*
 * Go through the list of both allocated and free blocks on the given
 * heap, checking to make sure all are valid.
 *
 * XXX: This needs a better output implementation
 */

int			walk_memory_heap(heap_struct * heap);

/*
 * Something bad happened to this block.  Tell somebody.
 */
void		print_memory_error(block_header * block, int error_no);

/* print a formatted string to "debug output", wherever that may be */
void		dprintf(const char * fmt, ...);

#endif	/* ifndef	__MEMMGR_CORE_H__ */
