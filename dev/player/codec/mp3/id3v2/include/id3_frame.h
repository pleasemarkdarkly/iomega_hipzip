//........................................................................................
//........................................................................................
//.. File Name: id3_frame.h
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


#include <codec/mp3/id3v2/id3_types.h>
#include <codec/mp3/id3v2/id3_field.h>
#include <codec/mp3/id3v2/id3_header_frame.h>


class ID3_Frame
{
public:
				ID3_Frame						( ID3_FrameID id = ID3FID_NOFRAME );
				~ID3_Frame						( void );

void			Clear							( void );
void			SetID							( ID3_FrameID id );
ID3_FrameID		GetID							( void );
ID3_Field&		Field							( ID3_FieldID name );

// *** PRIVATE INTERNAL DATA - DO NOT USE *** PRIVATE INTERNAL DATA - DO NOT USE ***

#ifdef FULL_ID3V2
bool			HasChanged						( void );
void			SetVersion						( uchar ver, uchar rev );
#endif //FULL_ID3V2

void			Parse							( uchar *buffer, luint size );

#ifdef FULL_ID3V2
luint			Size							( void );
luint			Render							( uchar *buffer );
#endif //FULL_ID3V2

char			encryptionID[ 256 ];			// the encryption method with which this frame is encrypted
char			groupingID[ 256 ];				// the group to which this frame belongs
bool			compression;					// should we try to compress?
bool			hasChanged;						// has the frame changed since the last parse/render?
bitset			fieldBits;						// which fields are present?
ID3_FrameID		frameID;						// what frame are we?
protected:
void			UpdateStringTypes				( void );

#ifdef FULL_ID3V2
void			UpdateFieldDeps					( void );
#endif //FULL_ID3V2

lsint			FindField						( ID3_FieldID name );
uchar			version;						// what version tag?
uchar			revision;						// what revision tag?
luint			numFields;						// how many fields are in this frame?
ID3_Field		**fields;						// an array of field object pointers
};


