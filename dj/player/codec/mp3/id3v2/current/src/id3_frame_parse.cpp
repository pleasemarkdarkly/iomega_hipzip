//........................................................................................
//........................................................................................
//.. File Name: id3_frame_parse.cpp
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


#include <codec/mp3/id3v2/id3_frame.h>
#include "zlib.h"


void			ID3_Frame::Parse				( uchar *buffer, luint size )
{
	luint	i;
	luint	posn	= 0;

	for	( i = 0; i < numFields; i++ )
	{
		fields[ i ]->SetVersion ( version, revision );
		posn += fields[ i ]->Parse ( buffer, posn, size );

		// if we just parsed a TEXTENC field, we'd
		// better tell the rest of the concerned string
		// fields in the frame what they are expected to
		// parse (ASCII or Unicode)
		if	( fields[ i ]->name == ID3FN_TEXTENC )
			UpdateStringTypes();
	}

	hasChanged = false;

	return;
}

