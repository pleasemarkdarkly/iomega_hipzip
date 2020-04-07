/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gnfs_readln.c:	Read one line from passed file handle, NULL terminate.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include GN_STRING_H
#include GN_STDLIB_H
#include <extras/cddb/gn_fs.h>



/*
 * gnfs_readln()
 *
 * Read into 'buffer' up to 'size' - 1 bytes, or until the first newline, whichever 
 * comes first.  Appends a NUL.
 */
gn_str_t gnfs_readln(gn_handle_t handle, gn_str_t buffer, gn_size_t size)
{
	/* iteration 1 */
	gn_str_t	p;

	/* gosh, let's protect ourselves a bit, shall we? */
	if (handle == FS_INVALID_HANDLE)
		return GN_NULL;
	if (buffer == GN_NULL)
		return GN_NULL;
	if (size == 0)
		return GN_NULL;

	/* read a bufferfull */
	size = gnfs_read(handle, buffer, size - 1);
	
	/* nuthin from nuthin */
	/* or, hey, maybe an error */
	if ((size == 0) || (size == (gn_size_t)-1))
		return NULL;
	
	/* add a nul */
	buffer[size] = 0;
	
	/* see if we passed a newline */
	/* XXX: make the newline char configurable? */
	p = strchr(buffer, '\n');
	if (p != NULL)
	{
		/* 
		** OK, we passed one.  Seek back in the file to just after it.
		** But be sure to take every possible combination of line terminators into
		** account: \r\n, \n, \r or whatever.
		*/
		if (*p == 0x0a && *(p - 1) == 0x0d)
		{
			/* kill carriage return and newline */
			*(p - 1) = 0;
		}
		else
		{
			/* kill newline */
			*p = 0;
		}
		/* reset file pointer */
		gnfs_seek(handle, (gn_foffset_t)(p - buffer - size + 1), FS_SEEK_CURRENT);
	}
	
	/* pass back the buffer */
	return buffer;
}


gn_str_t gnfs_gets(gn_handle_t handle, gn_char_t* buffer, gn_size_t size)
{
	/* iteration 1 */
	gn_char_t*	p = GN_NULL;

	/* gosh, let's protect ourselves a bit, shall we? */
	if (handle == FS_INVALID_HANDLE)
		return GN_NULL;
	if (buffer == GN_NULL)
		return GN_NULL;
	if (size == 0)
		return GN_NULL;

	/* read a bufferfull */
	size = gnfs_read(handle, buffer, size - 1);
	
	/* nuthin from nuthin */
	/* or, hey, maybe an error */
	if ((size == 0) || (size == (gn_size_t)-1))
		return GN_NULL;
	
	/* add a nul */
	buffer[size] = 0;
	
	/* see if we passed a newline */
	/* XXX: make the newline char configurable? */
	p = strchr(buffer, '\n');
	if (p != GN_NULL)
	{
		/* 
		** OK, we passed one.  Seek back in the file to just after it.
		   Include the newline (and, incidentally, the carriage return).
		*/

		p++;
		*p = 0;

		/* reset file pointer */
		gnfs_seek(handle, (gn_foffset_t)(p - buffer - size), FS_SEEK_CURRENT);
	}
	
	/* pass back the buffer */
	return buffer;
}

