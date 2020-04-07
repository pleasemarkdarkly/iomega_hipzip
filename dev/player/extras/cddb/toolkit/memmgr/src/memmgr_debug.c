/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * Debugging routines: allocating/checking guard bytes, walking the heap,
 * etc.  Most of these are nops in the release version.
 */

#include	<extras/cddb/gn_platform.h>
#include	"memmgr_core.h"

#include	GN_STDIO_H
#include	GN_STDARG_H

#if defined(PLATFORM_WIN32)
#define vsnprintf _vsnprintf
#endif

/*
 * gn_debug.c
 *
 * Implementation of debugging functions under Win32
 */

/*
 * dprintf()
 *
 * Print a formatted string to the debug console
 */
void dprintf(const char * fmt, ...)
{
	char	buff[2048];
	va_list	args;
	
	va_start(args, fmt);

	vsnprintf(buff, sizeof(buff) - 1, fmt, args);
	printf(buff);
	
	va_end(args);
}

/*
 * add_guard_bytes(block_header* block)
 *
 * Add the appropriate set of guard bytes at the beginning and end of this
 * block of memory, so we can see if anyone steps on it.
 */
void add_guard_bytes(block_header* block)
{
#ifdef	__GNMEM_DEBUG__

	int		i;
	long	guard;
	char*	p;
	char*	g;
	
	/* get to the start of memory */
	p = (char *)MEM_PTR(block) - kLeadingGuardBytes;
	
	/* put the guard bytes into a var so we a) know the size and 
	 * b) can take the address
	 */
	guard = kGuardByteValue;
	
	/* 
	 * Do the bit at the head.  If the number of guard bytes are > 8,
	 * copy the value repeatedly.  Then, copy the few odd bytes left over.
	 * XXX: This isn't the most flexible scheme, is it?
	 */
	for (i = 0; i < kLeadingGuardBytes / sizeof(guard); i++)
	{
		*(long *)p = kGuardByteValue;
		p += sizeof(guard);
	}
	
	/* Do the last extra bits */
#if 0	
	gnmem_memcpy(p, &guard, kLeadingGuardBytes % sizeof(guard));
#else
	g = (char *)&guard;
	for (i = 0; i < kLeadingGuardBytes % sizeof(guard); i++)
	{
		*p++ = *g++;
	}
#endif
	
	/*
	 * Now repeat at the end of the block 
	 */
	
	/* point the the byte after the part the user sees */
	p = (char *)MEM_PTR(block) + block->alloc_size;
	
	/* 
	 * Do the bit at the head.  If the number of guard bytes are > 8,
	 * copy the value repeatedly.  Then, copy the few odd bytes left over.
	 * XXX: This isn't the most flexible scheme, is it?
	 */
	for (i = 0; i < kTrailingGuardBytes / sizeof(guard); i++)
	{
		*(long *)p = kGuardByteValue;
		p += sizeof(guard);
	}
	
	/* Do the last extra bits */
#if 0	
	gnmem_memcpy(p, &guard, kTrailingGuardBytes & sizeof(guard));
#else
	g = (char *)&guard;
	for (i = 0; i < kTrailingGuardBytes % sizeof(guard); i++)
	{
		*p++ = *g++;
	}
#endif

#else	/* non-debug version */
	block = NULL;
#endif /* ifdef	__GNMEM_DEBUG__ */
}

/*
 * check_guard_bytes()
 *
 * Look and see if anybody's stepped on the guard bytes we set before.
 */
int check_guard_bytes(block_header* block)
{
#ifdef	__GNMEM_DEBUG__
	int		i;
	long	guard;
	char*	p;
	char*	g;
	
	/* get to the start of memory */
	p = (char *)MEM_PTR(block) - kLeadingGuardBytes;
	
	/* 
	 * Do the bit at the head.  If the number of guard bytes are > 8,
	 * look at the value repeatedly.  Then, check the few odd bytes left over.
	 * XXX: This isn't the most flexible scheme, is it?
	 */
	for (i = 0; i < kLeadingGuardBytes / sizeof(guard); i++)
	{
		if (*(long *)p != kGuardByteValue)
		{
			/* stepped on */
			return false;
		}
		p += sizeof(guard);
	}
	
	/* Do the last extra bits */
	g = (char *)&guard;
	for (i = 0; i < kLeadingGuardBytes % sizeof(guard); i++)
	{
		if (*p++ != *g++)
		{
			/* stepped on */
			return false;
		}
	}
	
	/*
	 * That was nice, now do the end
	 */
	
	/* point the the byte after the part the user sees */
	p = (char *)MEM_PTR(block) + block->alloc_size;
	
	for (i = 0; i < kTrailingGuardBytes / sizeof(guard); i++)
	{
		if (*(long *)p != kGuardByteValue)
		{
			/* stepped on */
			return false;
		}
		p += sizeof(guard);
	}
	
	/* Do the last extra bits */
	g = (char *)&guard;
	for (i = 0; i < kTrailingGuardBytes % sizeof(guard); i++)
	{
		if (*p++ != *g++)
		{
			/* stepped on */
			return false;
		}
	}
	
	/* we're cool */
	return true;
	
#else	/* non-debug version */
	block = NULL;

	/* just return success */
	return true;
	
#endif /* ifdef	__GNMEM_DEBUG__ */
}

/*
 * walk_memory_heap()
 *
 * Go through the list of both allocated and free blocks on the given
 * heap, checking to make sure all are valid.
 *
 * XXX: This needs a better output implementation
 */


int walk_memory_heap(heap_struct* heap)
{
	block_header*	block;
	
	dprintf("walking heap 0x%0X: id = %d, size = %d\n", 
			heap, heap->id, heap->alloc_size);
			
	/* check the free list */
	dprintf("free list:\n");
	dprintf("===============================\n");
	
	block = heap->free_list.next;
	while (block != NULL)
	{
		dprintf("  block [0x%0X]: heap id: %d flags: 0x%0X next: 0x%0X\n"
			   "  ptr = 0x%0X, size = %d, abs size = %d\n",
				block, block->heap_id, block->flags, block->next,
				MEM_PTR(block), block->alloc_size, block->abs_size);
		
		block = block->next;		
	}

	/* check the in use list */
	dprintf("\nalloced list:\n");
	dprintf("===============================\n");
	
	block = heap->in_use_list.next;
	while (block != NULL)
	{
		dprintf("  block [0x%0X]: heap id: %d flags: 0x%0X next: 0x%0X\n"
			   "  ptr = 0x%0X, size = %d, abs size = %d\n",
				block, block->heap_id, block->flags, block->next,
				MEM_PTR(block), block->alloc_size, block->abs_size);
		
		if (!check_guard_bytes(block))
		{
			dprintf("    guard bytes have been overwritten!\n");
		}
		
		block = block->next;		
	}
	
	return (0);
}


static char* gMessages[] =
{
	"",
	"mem mgr not yet initialized",
	"invalid pointer on free() or realloc()",
	"heap not found",
	"not enough free memory",
	"no free heaps",
	"invalid heap size",
	"memory block corrupted",
	"heap is corrupted"
};

/*
 * print_memory_error()
 *
 * Something bad happened to this block.
 */
void print_memory_error(block_header* block, int error_no)
{
	char*	msg;
	
	msg = (error_no < 0 || error_no >= sizeof(gMessages) / sizeof(gMessages[0])) ? 
					"" : gMessages[error_no];

	if (block != NULL)
	{
		dprintf("GNMEM: error %d [%s]: ptr = 0x%0x, size = %d\n",
				error_no, msg, MEM_PTR(block), block->alloc_size);
	}
	else
	{
		dprintf("GNMEM: error %d [%s]\n", error_no, msg);
	}
}	
