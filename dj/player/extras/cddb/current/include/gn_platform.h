/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_platform.h - Sets the platform define based upon compiler definitions.
 */

#ifndef	_GN_PLATFORM_H_
#define _GN_PLATFORM_H_

/*
 * Platform-specific #defines should be placed in "platform.h"
 *
 * The following symbols may be defined and will affect compilation of
 * some portions of the abstraction layer and lookup layer:
 *
 * Symbol                        Side Effects
 * -------------------------     ------------------------------------------
 * GN_CACHE_DATA                 Always cache data, not toc_ids
 * GN_DCDDB                      Data disk CDDB variant
 * GN_FLASH_PAGE_IO              Flash Memory requires paged I/O
 * GN_MANUAL_DATA_PACK           Platform has odd word alignment issues
 * GN_MEM_CACHE                  Memory-based cache
 * GN_NO_CACHE                   No Local Cache
 * GN_NO_CACHE_BACKUPS           No Cache File Backups
 * GN_NO_FILE_ATTR               No dependence on file system "modes"
 * GN_NO_LOCAL                   No local embedded database is supported
 * GN_NO_LOGGING                 Logging not supported
 * GN_NO_ONLINE                  No on-line functionality included
 * GN_NO_ONLINE_UPDATES          Code for getting on-line updates not included
 * GN_NO_PCUPDATES               No support for PC connectivity
 * GN_NO_PROTO_COMPRESSION       Upstream communications not compressed
 * GN_NO_TOC_READING             Platform does not support TOC lookups on CD
 * GN_NO_UPDATE_BACKUPS          No Update DB Backups
 * GN_NO_UPDATES                 Code for processing updates not included
 * GN_READONLY                   No modifications to file system supported
 *
 *
 * For the Communications Manager:
 *
 * Symbol                        Side Effects
 * -------------------------     ------------------------------------------
 * GN_COMM_HAVE_BSD_SOCKETS      Platform supports socket mechanism
 * GN_COMM_HAVE_SELECT           Platform supports socket select() function
 * 
 * 
 * For the Memory Manager (if used):
 *
 * Symbol                        Side Effects
 * -------------------------     ------------------------------------------
 * GN_MEM_POINTER_ALIGNMENT      Specify alignment requirements for pointers
 * GN_MEM_INIT_REQUIRED          Platform requires memory manager initialization
 * GN_MEM_HAVE_MALLOC_FREE       Platform supports malloc()/free()/realloc() functions
 * GN_MEM_HAVE_MEMSET            Platform supports memset() function
 * GN_MEM_HAVE_MEMCPY            Platform supports memcpy() function
 * GN_MEM_HAVE_MEMCMP            Platform supports memcmp() function
 * GN_MEM_HAVE_MEMMOVE           Platform supports memmove() function
 * 
 */

#include <extras/cddb/dharma/platform.h>

#if defined(WIN32)
#	define	PLATFORM_WIN32
#endif

#if	defined(PLATFORM_WIN32)
	#define	PLATFORM_WINDOWS
#elif	defined(PLATFORM_LINUX) || defined(PLATFORM_BSD) || defined(PLATFORM_SOLARIS)
	#define PLATFORM_UNIX
#endif

/* Ensure we get the proper socket wrapper code included */
#if defined(PLATFORM_UNIX) || defined(PLATFORM_WIN32)
#	define	GN_COMM_HAVE_BSD_SOCKETS
#endif

/* use standard system header files if not defined in "platform.h" */
#if !defined(GN_STDARG_H)
#define		GN_STDARG_H		<stdarg.h>
#endif
#if !defined(GN_STDDEF_H)
#define		GN_STDDEF_H		<stddef.h>
#endif
#if !defined(GN_STDIO_H)
#define		GN_STDIO_H		<stdio.h>
#endif
#if !defined(GN_STRING_H)
#define		GN_STRING_H		<string.h>
#endif
#if !defined(GN_IO_H)
#define		GN_IO_H			<io.h>
#endif
#if !defined(GN_STDLIB_H)
#define		GN_STDLIB_H		<stdlib.h>
#endif
#if !defined(GN_FCNTL_H)
#define		GN_FCNTL_H		<fcntl.h>
#endif
#if !defined(GN_MEMORY_H)
#define		GN_MEMORY_H		<memory.h>
#endif
#if !defined(GN_SYS_TYPES_H)
#define		GN_SYS_TYPES_H	<sys/types.h>
#endif
#if !defined(GN_SYS_STAT_H)
#define		GN_SYS_STAT_H	<sys/stat.h>
#endif
#if !defined(GN_ERRNO_H)
#define		GN_ERRNO_H		<errno.h>
#endif
#if !defined(GN_TIME_H)
#define		GN_TIME_H		<time.h>
#endif
#if !defined(GN_CTYPE_H)
#define		GN_CTYPE_H		<ctype.h>
#endif
#if !defined(GN_MALLOC_H)
#define		GN_MALLOC_H		<malloc.h>
#endif
#if !defined(GN_ASSERT_H)
#define		GN_ASSERT_H		<assert.h>
#endif
#if !defined(GN_MATH_H)
#define		GN_MATH_H		<math.h>
#endif
#if !defined(GN_UNISTD_H)
#define		GN_UNISTD_H		<unistd.h>
#endif
#if !defined(GN_PROCESS_H)
#define		GN_PROCESS_H	<process.h>
#endif

#include GN_STDIO_H	/* required for FILENAME_MAX */

#if !defined(GN_MAX_PATH)
#define		GN_MAX_PATH		FILENAME_MAX
#endif

#if defined(GN_READONLY)
#define	GN_NO_BACKUPS
#define GN_NO_UPDATES
#endif /* #if defined(GN_READONLY) */

/* more platform-specific definitions */
#include <extras/cddb/crossplatform.h>

#endif /* _GN_PLATFORM_H_ */
