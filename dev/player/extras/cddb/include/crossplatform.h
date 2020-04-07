/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
** crossplatform.h
**
** Definitions, macros. etc. for simplifying the process of porting
** the abstraction layer other platforms.
*/

#ifndef	__CROSSPLATFORM_H__
#define	__CROSSPLATFORM_H__

#include	<extras/cddb/gn_platform.h>

#if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD) || defined(PLATFORM_SOLARIS)

	#define	PLATFORM_UNIX

#endif

/* generic Unix definitions go here */
#if	defined(PLATFORM_UNIX)

	#define	strcmpi(s1, s2)		strcasecmp(s1, s2)
	#define	stricmp(s1, s2)		strcasecmp(s1, s2)
	#define	strnicmp(s1, s2, n)	strncasecmp(s1, s2, n)
#endif

#if	defined(PLATFORM_WIN32)

	#define	PLATFORM_WINDOWS

	#define	strcmpi(s1, s2)		_stricmp(s1, s2)
	#define	strnicmp(s1, s2, n)	_strnicmp(s1, s2, n)

	#define	snprintf		_snprintf
	
#endif	/* ifdef PLATFORM_WIN32) */

#if	defined(PLATFORM_LINUX)
#endif

#if	defined(PLATFORM_BSD)
#endif

#if	defined(PLATFORM_SOLARIS)
#endif /* end of platform-specific defines */


#endif	/* ifndef __CROSSPLATFORM_H__ */
