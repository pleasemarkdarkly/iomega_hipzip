/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * platform.h - Contains platform-specific definitions.
 */

/*
 * NOTE: THIS FILE IS A PLACEHOLDER TO ALLOW COMPILATION ON WINDOWS
 * AND LINUX PLATFORMS.  WHEN PORTING TO ANOTHER PLATFORM, YOU SHOULD
 * CREATE A FILE IN AN INCLUDE DIRECTORY WHICH PRECEDES THE DEFAULT
 * GRACENOTE INCLUDE DIRECTORY IN THE SEARCH PATH.  USE THE platform.h
 * FILE TO PROVIDE DEFINITIONS FOR THE SYMBOLS USED TO DEFINE STANDARD
 * HEADER FILES (e.g., GN_STDIO_H) IN gn_platform.h.
 */

                                                                                                                       
/* 
 * For the memory manager 
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <fs/fat/sdapi.h>   /* EMAXPATH */

#define		GN_NO_LOGGING

/* what's here from the standard library? */
#define		GN_MEM_HAVE_MEMCPY
#define		GN_MEM_HAVE_MEMMOVE
#define		GN_MEM_HAVE_MEMSET
#define		GN_MEM_HAVE_MEMCMP
#define		GN_MEM_HAVE_MALLOC_FREE
#define		GN_NO_PROTO_COMPRESSION

#define     GN_NO_FILE_ATTR
#define		GN_NO_PCUPDATES

#define		GN_COMM_HAVE_BSD_SOCKETS
#define		GN_COMM_HAVE_SELECT

/* we don't need to do any special initialization to call malloc() */
#undef		GN_MEM_INIT_REQUIRED

#define		PLATFORM_DHARMA

#define		GN_MAX_PATH		EMAXPATH

typedef long long	gn_uint64_t;
#define	LONGLONG_TYPE

#endif /* #ifndef _PLATFORM_H_ */
