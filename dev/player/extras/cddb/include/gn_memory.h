/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_memory.h - Abstraction header file containing the declaration 
 * of memory related functions for cross platform compatibility.
 */

#ifndef _GN_MEMORY_H_
#define _GN_MEMORY_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C"{
#endif 

/* define the symbol below to enable logging of gnmem_* function calls */
/* #define		_GN_LOG_MEM_ */

#if defined(_GN_LOG_MEM_) || defined(_BLD_TH_MEM_)

#define gnmem_malloc(size)			_gnmem_malloc(size, __LINE__, __FILE__)
#define gnmem_realloc(ptr, size)	_gnmem_realloc(ptr, size, __LINE__, __FILE__)
#define gnmem_free(ptr)				_gnmem_free(ptr, __LINE__, __FILE__)

/* Allocate memory from the heap */

void* 
_gnmem_malloc(gn_size_t size,gn_uint32_t line,gn_str_t file);

/* Reallocate the memory for the block */
void* 
_gnmem_realloc(void *ptr, gn_size_t size, gn_uint32_t line, gn_str_t file);

/* Free the allocated block */
gn_error_t 
_gnmem_free(void *block, gn_uint32_t line, gn_str_t file);

#else

/* Allocate memory from the heap */

void* 
gnmem_malloc(gn_size_t size);

/* Reallocate the memory for the block */
void* 
gnmem_realloc(void* ptr, gn_size_t size);

/* Free the allocated block */
gn_error_t 
gnmem_free(void* block);


#endif

/*
 * Typedef
 */

typedef struct proc_size{
	gn_int32_t num_blocks_occ;		/* Number of blocks occupied */
	gn_int32_t total_mem_occ;		/* Memory occupied */
	gn_int32_t num_blocks_free;		/* Number of blocks free */
	gn_int32_t total_mem_free;		/* Memory free */
	gn_int32_t max_free_block;		/* Size of largest available block*/
} proc_size_t;


/*
 * Prototypes.
 */

/*
 * Core memory manager operations (allocate/reallocate/free memory)
 */

/* Initialize the memory manager */
gn_error_t
gnmem_initialize(void * heap_start, gn_size_t total_heap_size);

/* Shut down the memory manager when we're done with it */
gn_error_t
gnmem_shutdown(void);

#ifdef	_GN_LOG_MEM_
/* Insert string/comment into the memory log file */
void
gnmem_add_mem_logging_info(gn_cstr_t info);
#endif /* #ifndef _GN_LOG_MEM_ */


/*
 * gnmem_create_heap(gn_size_t alloc_size, int num_blocks)
 *
 * Create a new fixed-sized heap of 'num_blocks' many 'alloc_size' blocks.
 *
 */
gn_error_t
gnmem_create_heap(gn_size_t alloc_size, int num_blocks);

#if defined(_GN_LOG_MEM_) || defined(_BLD_TH_MEM_)

/* Allocate memory from the heap */
void* 
_gnmem_malloc(gn_size_t size, gn_uint32_t line, gn_str_t file);

/* Reallocate the memory for the block */
void* 
_gnmem_realloc(void *ptr, gn_size_t size, gn_uint32_t line, gn_str_t file);

/* Free the allocated block */
gn_error_t 
_gnmem_free(void *block, gn_uint32_t line, gn_str_t file);

#else

/* Allocate memory from the heap */
void* 
gnmem_malloc(gn_size_t size);

/* 
 * Allocate memory that will have a long lifetime, typically the life of
 * the program.  The block will be allocated from the end of the heap.
 * Use of this call can help avoid fragmentation.
 */
void *
gnmem_malloc_fixed(gn_size_t size);

/* Reallocate the memory for the block */
void* 
gnmem_realloc(void *ptr, gn_size_t size);

/* Free the allocated block */
gn_error_t 
gnmem_free(void *block);

#endif


/*
 * Utility routines involving memory (move/copy/fill)
 *
 * Semantics match that of ANSI C.
 */

/* Copy memory from point A to point B, assuming NO overlap */
void *
gnmem_memcpy(void * dest, void * src, gn_size_t size);

/* Copy memory from point A to point B, allowing for overlap */
void *
gnmem_memmove(void * dest, void * src, gn_size_t size);

/* Set a range of memory to a specific value */
void *
gnmem_memset(void * dest, int ch, gn_size_t count);

/* Zero out a range of memory */
#define		gnmem_zero(buff, size)		gnmem_memset(buff, 0, size)

/* Compare two regions of memory */
int
gnmem_memcmp(const void * buff1, const void * buff2, gn_size_t count);


/*
 * Utility routines for finding memory sizes (available free, etc.)
 */

/* Get the size of available RAM */
gn_size_t 
gnmem_getfreeram(void);

/* Get the size of largest available block */
gn_size_t 
gnmem_get_max_block_size(void);

/* Get the heap and stack size */
gn_error_t 
gnmem_get_proc_size(proc_size_t *p_proc_size);

/* How big is this block of memory? */ 
gn_size_t
gnmem_get_pointer_size(void * ptr);

/*
 * Debugging and verification routines.  These may be no-ops in 
 * release versions.
 */

/* Is this pointer a valid one? */
gn_bool_t
gnmem_is_pointer_valid(void * ptr);

/* Check if the heap is Ok */
gn_error_t 
gnmem_checkheap(void);

/*
 * Go through the list of both allocated and free blocks on the given
 * heap, checking to make sure all are valid.
 *
 * XXX: This needs a better output implementation
 */
void
gnmem_walk_heaps(void);

#ifdef __cplusplus
}
#endif 

#endif

