/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * test_mem_update.c: Tests memory allocation performed during an eCDDB update
 */

#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>

#include <stdio.h>
#include <extras/cddb/abstract_layer/dharma/test_mem_struct.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];


static long total_heap_usage(ptr_array_t ptr_array[], int ptr_array_len)
{
	long	cur_heap_size = 0;
	int	i;

	for(i = 0, cur_heap_size = 0; i < ptr_array_len; i++)
	{
		if(ptr_array[i].ptr != GN_NULL)
		{
			cur_heap_size += ptr_array[i].bytes;
		}
	}

	return cur_heap_size;
}

void
main_thread(cyg_addrword_t data)
{
	int			i;
	int			k;
	gn_error_t		error;
	long			cur_heap_size;
	long			total_alloc = 0;
	long			heap_max = 0;
	extern alloc_table_t	alloc_table[];
	extern ptr_array_t	ptr_array[];
	extern int		ptr_array_len;

	for(i = 0; i < ptr_array_len; i++)
	{
		ptr_array[i].ptr = GN_NULL;
		ptr_array[i].bytes = 0;
	}

	error = gnmem_initialize(0, 0);
	if(error != SUCCESS)
	{
		diag_printf("gnmem_initialzied failed\n");
		return 1;
	}

	/* simulate 10 update packages */
	for(k = 0; k < 10; k++)
	{
		for(i = 0; alloc_table[i].bytes != -2; i++)
		{
			if(alloc_table[i].bytes >= 0)
			{
				/* do an allocation */
				ptr_array[alloc_table[i].ptr_index].ptr = gnmem_malloc(alloc_table[i].bytes);
				if(ptr_array[alloc_table[i].ptr_index].ptr == GN_NULL)
				{
					diag_printf("Loop index %d %d failed\n", k, i);
					diag_printf("Attempted to allocate %d bytes failed\n", alloc_table[i].bytes);
					cur_heap_size = total_heap_usage(ptr_array, ptr_array_len);
					diag_printf("Current size of heap: %ld bytes\n", cur_heap_size);
					diag_printf("Maximum heap usage: %ld bytes\n", heap_max);
					diag_printf("Total number of bytes allocated (and maybe freed): %ld bytes\n", total_alloc);
					return(1);
				}
				ptr_array[alloc_table[i].ptr_index].bytes = alloc_table[i].bytes;
				total_alloc += alloc_table[i].bytes;
				if(heap_max < total_heap_usage(ptr_array, ptr_array_len))
				{
					heap_max = total_heap_usage(ptr_array, ptr_array_len);
				}
			}
			else
			{
				/* do a free */
				gnmem_free(ptr_array[alloc_table[i].ptr_index].ptr);
				ptr_array[alloc_table[i].ptr_index].ptr = GN_NULL;
				ptr_array[alloc_table[i].ptr_index].bytes = 0;
			}
		}
	}

	diag_printf("Memory allocation test was a success\n");
	cur_heap_size = total_heap_usage(ptr_array, ptr_array_len);
	diag_printf("Current size of heap: %ld bytes\n", cur_heap_size);
	diag_printf("Maximum heap usage: %ld bytes\n", heap_max);
	diag_printf("Total number of bytes allocated (and maybe freed): %ld bytes\n", total_alloc);
}

void
cyg_user_start(void)
{
    cyg_thread_create(10, main_thread, (cyg_addrword_t)0, "main_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}

