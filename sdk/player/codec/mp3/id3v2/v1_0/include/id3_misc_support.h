//........................................................................................
//........................................................................................
//.. File Name: id3_misc_support.h
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


#ifndef	ID3LIB_MISC_SUPPORT_H
#define	ID3LIB_MISC_SUPPORT_H


//#include <wchar.h>
#include <string.h>
#include <codec/mp3/id3v2/id3_types.h>
#include <codec/mp3/id3v2/id3_tag.h>

// in 'id3_misc_support.cpp'
#ifdef FULL_ID3V2
void			ID3_AddTitle					( ID3_Tag *tag, char *text );
void			ID3_AddArtist					( ID3_Tag *tag, char *text );
void			ID3_AddAlbum					( ID3_Tag *tag, char *text );
void			ID3_AddLyrics					( ID3_Tag *tag, char *text );
#endif //FULL_ID3V2

void			ID3_ASCIItoUnicode				( wchar_t *unicode, char *ascii, luint len );
void			ID3_UnicodeToASCII				( char *ascii, wchar_t *unicode, luint len );
// in 'id3_tag_parse_v1.cpp'
void			ID3_RemoveTrailingSpaces		( char *buffer, luint length );


#endif


