//........................................................................................
//........................................................................................
//.. File Name: id3_types.h
//.. Last Modified By: Donni Reitz-Pesek	donni@iobjects.com
//.. Modification date: 2/17/2000
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.
//..	 All rights reserved. This code may not be redistributed in source or linkable
//.. 	 object form without the express written consent of Interactive Objects.
//.. Contact Information: www.iobjects.com
//........................................................................................
//........................................................................................
//  The authors have released ID3Lib as Public Domain (PD) and claim no copyright,
//  patent or other intellectual property protection in this work.  This means that
//  it may be modified, redistributed and used in commercial and non-commercial
//  software and hardware without restrictions.  ID3Lib is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
//  
//  The ID3Lib authors encourage improvements and optimisations to be sent to the
//  ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved
//  submissions may be altered, and will be included and released under these terms.
//  
//  Mon Nov 23 18:34:01 1998


#ifndef	ID3LIB_TYPES_H
#define	ID3LIB_TYPES_H


#ifdef WIN32
#include <windows.h>
#endif
//#include <wchar.h>


#ifdef __DLL
#define	DLLEXPORT	__declspec ( dllexport )
#define	CDLLEXPORT	extern "C" __declspec ( dllexport )
#else
#define	DLLEXPORT
#define	CDLLEXPORT
#endif


typedef	unsigned char		uchar;
typedef short signed int	ssint;
typedef short unsigned int	suint;
typedef unsigned short		usint;
typedef long signed int		lsint;
typedef long unsigned int	luint;
typedef	long double			ldoub;
typedef long unsigned int *	bitset;

#define	BS_SET(v,x)			( (v)[ (x) / ( sizeof ( luint ) * 8 ) ] |=  ( 1 << ( (x) % ( sizeof ( luint ) * 8 ) ) ) )
#define	BS_CLEAR(v,x)		( (v)[ (x) / ( sizeof ( luint ) * 8 ) ] &= ~( 1 << ( (x) % ( sizeof ( luint ) * 8 ) ) ) )
#define	BS_ISSET(v,x)		( (v)[ (x) / ( sizeof ( luint ) * 8 ) ] &   ( 1 << ( (x) % ( sizeof ( luint ) * 8 ) ) ) )

#ifndef	NULL
#define	NULL	(0L)
#endif

#ifndef	MIN
inline lsint	MIN								( lsint x, lsint y )
{
	return x < y ? x : y;
}
#endif

#ifndef	MAX
inline lsint	MAX								( lsint x, lsint y )
{
	return x > y ? x : y;
}
#endif

// include other abstract types here because they
// may depend on the types defined above
#include <codec/mp3/id3v2/id3_int28.h>

// hack to replace all the wchar_t references to a 
#define wchar_t usint

#endif


