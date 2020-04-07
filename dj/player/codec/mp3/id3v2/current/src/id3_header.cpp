//........................................................................................
//........................................................................................
//.. File Name: id3_header.cpp
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


#include <string.h>
#include <codec/mp3/id3v2/id3_header.h>
#include <codec/mp3/id3v2/id3_error.h>


ID3_HeaderInfo	ID3_VersionInfo[] =
{
	//				SIZEOF	SIZEOF	SIZEOF	EXT		EXT		EXPERIM
	//	VER	REV		FRID	FRSZ	FRFL	HEADER	SIZE	BIT
	{	2,	0,		3,		3,		0,		false,	 0,		false		},
	{	2,	1,		3,		3,		0,		true,	 8,		true		},
	{	3,	0,		4,		4,		2,		false,	10,		false		},
	{	0	}
};


ID3_HeaderInfo	*ID3_LookupHeaderInfo			( uchar ver, uchar rev )
{
	ID3_HeaderInfo	*info	= NULL;
	luint			i		= 0;

	while	( ID3_VersionInfo[ i ].version != 0 )
	{
		if	( ID3_VersionInfo[ i ].version == ver &&
			  ID3_VersionInfo[ i ].revision == rev )
			break;
		else
			i++;
	}

	if	( ID3_VersionInfo[ i ].version != 0 )
		info = &ID3_VersionInfo[ i ];

	return info;
}


				ID3_Header::ID3_Header			( void )
{
	SetVersion ( ID3_TAGVERSION, ID3_TAGREVISION );
	dataSize = 0;
	flags = 0;
}


void			ID3_Header::SetVersion			( uchar ver, uchar rev )
{
	version		= ver;
	revision	= rev;
	info		= ID3_LookupHeaderInfo ( version, revision );

	return;
}

#ifdef FULL_ID3V2

void			ID3_Header::SetDataSize			( luint newSize )
{
	dataSize = newSize;

	return;
}


void			ID3_Header::SetFlags			( luint newFlags )
{
	flags = newFlags;

	return;
}

#endif //FULL_ID3V2

