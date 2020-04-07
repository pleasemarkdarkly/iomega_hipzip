//........................................................................................
//........................................................................................
//.. File Name: id3_header_tag.h
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


#ifndef	ID3LIB_HEADER_TAG_H
#define	ID3LIB_HEADER_TAG_H


#include <codec/mp3/id3v2/id3_types.h>
#include <codec/mp3/id3v2/id3_header.h>


#define	ID3_TAGID								"ID3"
#define	ID3_TAGHEADERSIZE						( 10 )


#define	ID3HF_UNSYNC							( 1 << 7 )
#define	ID3HF_EXTENDEDHEADER					( 1 << 6 )
#define	ID3HF_EXPERIMENTAL						( 1 << 5 )


class ID3_TagHeader : public ID3_Header
{
public:

#ifdef FULL_ID3V2
virtual luint	Size							( void );
virtual luint	Render							( uchar *buffer );
#endif //FULL_ID3V2

};


//CDLLEXPORT (dr what is this anyway?)
lsint			ID3_IsTagHeader					( uchar header[ ID3_TAGHEADERSIZE ] );


#endif


