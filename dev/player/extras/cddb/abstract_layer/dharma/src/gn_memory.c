/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_memory.c - The source code listing for the function prototype
 * declared in gn_memory.h. This implementation uses ANSI memory allocation
 * calls
 */

/*
 * Dependencies
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include GN_STDIO_H
#include GN_MALLOC_H
#include GN_STRING_H
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_errors.h>


/*
 * Local Variables
 */

/* We don't want to do any memory operations if the memory system has not
 * yet been initialized.
 */
static gn_bool_t	mem_initialized = GN_FALSE;


/*
 * Public Functions
 */

/* Initialize the memory manager
 * This won't really do anything if we are not using the memory manager
 */
gn_error_t
gnmem_initialize(void * heap_start, gn_size_t total_heap_size)
{
	if(mem_initialized == GN_TRUE)
	{
		return GNERR_ERR_CODE(MEMERR_Busy);
	}

	mem_initialized = GN_TRUE;

	return(SUCCESS);
}


/* Shut down the memory manager when we're done with it
 */
gn_error_t
gnmem_shutdown(void)
{
	if(mem_initialized == GN_FALSE)
	{
		return GNERR_ERR_CODE(MEMERR_Noinit);
	}

	mem_initialized = GN_FALSE;

	return(SUCCESS);
}


void* 
gnmem_malloc(gn_size_t size)
{
	void*	block;

	if(mem_initialized == GN_FALSE)
	{
		return GN_NULL;
	}

	block = malloc(size);
	return block;
}


void* 
gnmem_realloc(void *memblock, gn_size_t size)
{
	void*	block;

	if(mem_initialized == GN_FALSE)
	{
		return GN_NULL;
	}

	block = realloc(memblock, size);
	return block;
}

	
gn_error_t 
gnmem_free(void *memblock)
{
	if(mem_initialized == GN_FALSE)
	{
		return GNERR_ERR_CODE(MEMERR_Noinit);
	}

	if (memblock == NULL) {
		return GNERR_ERR_CODE(MEMERR_Invalid_mem);
	}

	free(memblock);
	return SUCCESS;
}


/* Copy memory from point A to point B, assuming NO overlap
 */
void *
gnmem_memcpy(void* dest, void* src, gn_size_t size)
{
	return memcpy(dest, src, size);
}


/* Copy memory from point A to point B, allowing for overlap
 */
void *
gnmem_memmove(void* dest, void* src, gn_size_t size)
{
	return memmove(dest, src, size);
}


/* Set a range of memory to a specific value
 */
void *
gnmem_memset(void* dest, int ch, gn_size_t count)
{
	return memset(dest, ch, count);
}


/* Compare two regions of memory
 */
int gnmem_memcmp(const void* buff1, const void* buff2, gn_size_t count)
{
	return (memcmp(buff1, buff2, count));
}
