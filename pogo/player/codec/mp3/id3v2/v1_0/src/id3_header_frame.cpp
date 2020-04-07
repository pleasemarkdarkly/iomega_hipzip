//........................................................................................
//........................................................................................
//.. File Name: id3_header_frame.cpp
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
//#include <memory.h> (We don't have this)
#include <codec/mp3/id3v2/id3_header_frame.h>

#ifdef FULL_ID3V2
#include <codec/mp3/id3v2/id3_error.h>

void			ID3_FrameHeader::SetFrameID		( ID3_FrameID id )
{
	frameID = id;

	return;
}

#endif //FULL_ID3V2

luint			ID3_FrameHeader::Size			( void )
{
	return info->frameIDBytes + info->frameSizeBytes + info->frameFlagsBytes;
}


luint			ID3_FrameHeader::GetFrameInfo	( ID3_FrameAttr &attr, uchar *buffer )
{
	luint			posn			= 0;
	luint			i				= 0;

	strncpy ( attr.textID, (char *) buffer, info->frameIDBytes );
	attr.textID[ info->frameIDBytes ] = 0;

	posn += info->frameIDBytes;

	attr.size = 0;

	for	( i = 0; i < info->frameSizeBytes; i++ )
		attr.size |= buffer[ posn + i ] << ( ( info->frameSizeBytes - 1 - i ) * 8 );

	posn += info->frameSizeBytes;

	attr.flags = 0;

	for	( i = 0; i < info->frameFlagsBytes; i++ )
		attr.flags |= buffer[ posn + i ] << ( ( info->frameFlagsBytes - 1 - i ) * 8 );

	posn += info->frameFlagsBytes;

	return posn;
}


luint			ID3_FrameHeader::Render			( uchar *buffer )
{
#ifdef FULL_ID3V2
	luint			bytesUsed	= 0;
	ID3_FrameDef	*frameDef	= NULL;
	char			*textID		= NULL;
	luint			i;

	if	( frameDef = ID3_FindFrameDef ( frameID ) )
	{
		if	( info->frameIDBytes < strlen ( frameDef->longTextID ) )
			textID = frameDef->shortTextID;
		else
			textID = frameDef->longTextID;
	}
	else
		ID3_THROW ( ID3E_InvalidFrameID );

	memcpy ( &buffer[ bytesUsed ], (uchar *) textID, info->frameIDBytes );
	bytesUsed += info->frameIDBytes;

	for	( i = 0; i < info->frameSizeBytes; i++ )
		buffer[ bytesUsed + i ] = (uchar) ( ( dataSize >> ( ( info->frameSizeBytes - i - 1 ) * 8 ) ) & 0xFF );

	bytesUsed += info->frameSizeBytes;

	for	( i = 0; i < info->frameFlagsBytes; i++ )
		buffer[ bytesUsed + i ] = (uchar) ( ( flags >> ( ( info->frameFlagsBytes - i - 1 ) * 8 ) ) & 0xFF );

	bytesUsed += info->frameFlagsBytes;

	return bytesUsed;
#endif //FULL_ID3V2
	return 0;
}


