/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*  test_mem_update.h - declarations for memory allocation test
 */

#ifndef _TEST_MEM_STRUCT_H_
#define _TEST_MEM_STRUCT_H_

#include <extras/cddb/gn_memory.h>

/* if bytes -1 then free ptr_index
 * if bytes -2 then end of array
 */
typedef struct alloc_table_s
{
	int	bytes;
	short	ptr_index;
} alloc_table_t;


/* This structure contains the address of the allocated memory block and
 * the number of bytes that were allocted for it
 */
typedef struct ptr_array_s
{
	void	*ptr;
	long	bytes;
} ptr_array_t;


#endif  /* _TEST_MEM_STRUCT_H_ */
