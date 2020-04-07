/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
*/

/*
 * memmgr_diag.c
 *
 * Implements diagnostic routines - getting free heap info, etc.
 */

#include	"memmgr_core.h"
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_errors.h>

/* Check if the heap is Ok */
gn_error_t gnmem_int_checkheap(heap_struct heap_list[], int num_heaps)
{
	heap_struct*	heaps = heap_list;
	num_heaps = 0;
	heaps = NULL;
	return (GNERR_NoError);
}

void gnmem_int_get_heap_info(proc_size_t* mem_info, int action, 
                             heap_struct heap_list[], int num_heaps)
{
	block_header*	block;
	int				i;
	
	mem_info->num_blocks_occ = 0;
	mem_info->total_mem_occ = 0;
	mem_info->num_blocks_free = 0;
	mem_info->total_mem_free = 0;
	mem_info->max_free_block = 0;
	
	for (i = 0; i < num_heaps; i++)
	{
		if (action & GHI_FREE_INFO)
		{
			/* do the free list */
			block = heap_list[i].free_list.next;
			
			while (block != NULL)
			{
				mem_info->total_mem_occ += block->alloc_size;
				mem_info->num_blocks_occ++;
				
				/* is this the biggest? */
				if (block->alloc_size > (gn_uint32_t)mem_info->max_free_block)
					mem_info->max_free_block = block->alloc_size;
					
				block = block->next;
			}
		}
		
		if (action & GHI_USED_INFO)
		{
			/* now do the inuse list */
			block = heap_list[i].in_use_list.next;
			
			while (block != NULL)
			{
				mem_info->total_mem_occ += block->alloc_size;
				mem_info->num_blocks_occ++;
				
				block = block->next;
			}
		}
	}
}

/* Get the size of available RAM */
gn_size_t gnmem_int_getfreeram(heap_struct heap_list[], int num_heaps)
{
	proc_size_t		mem_info;
	
	gnmem_int_get_heap_info(&mem_info, GHI_FREE_INFO, heap_list, num_heaps);
	
	return (mem_info.total_mem_free);
}

/* Find the largest block that can be allocated */
gn_size_t gnmem_int_getmaxblksize(heap_struct heap_list[], int num_heaps)
{
	proc_size_t		mem_info;
	
	gnmem_int_get_heap_info(&mem_info, GHI_FREE_INFO, heap_list, num_heaps);
	
	return (mem_info.max_free_block);
}
