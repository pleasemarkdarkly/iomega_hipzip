//........................................................................................
//........................................................................................
//.. File Name: id3_field.h
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


#ifndef	ID3LIB_FIELD_H
#define	ID3LIB_FIELD_H

#include <codec/mp3/id3v2/id3_types.h>
#include <codec/mp3/id3v2/id3_error.h>


// field flags
#define	ID3FF_NULL			( 1 << 0 )
#define	ID3FF_NULLDIVIDE	( 1 << 1 )
#define	ID3FF_ADJUSTENC		( 1 << 2 )
#define	ID3FF_ADJUSTEDBY	( 1 << 3 )


enum ID3_TextEnc
{
	ID3TE_ASCII			= 0,
	ID3TE_UNICODE
};


enum ID3_FieldType
{
	ID3FTY_INTEGER		= 0,
	ID3FTY_BITFIELD,
	ID3FTY_BINARY,
	ID3FTY_ASCIISTRING,
	ID3FTY_UNICODESTRING
};


enum ID3_FieldID
{
	ID3FN_NOFIELD			= 0,
	ID3FN_TEXTENC,
	ID3FN_TEXT,
	ID3FN_URL,
	ID3FN_DATA,
	ID3FN_DESCRIPTION,
	ID3FN_OWNER,
	ID3FN_EMAIL,
	ID3FN_RATING,
	ID3FN_FILENAME,
	ID3FN_LANGUAGE,
	ID3FN_PICTURETYPE,
	ID3FN_IMAGEFORMAT,
	ID3FN_MIMETYPE,
	ID3FN_COUNTER,
	ID3FN_SYMBOL,
	ID3FN_VOLUMEADJ,
	ID3FN_NUMBITS,
	ID3FN_VOLCHGRIGHT,
	ID3FN_VOLCHGLEFT,
	ID3FN_PEAKVOLRIGHT,
	ID3FN_PEAKVOLLEFT,

	ID3FN_LASTFIELDID
};


enum ID3_FrameID
{
	ID3FID_NOFRAME			= 0,
	ID3FID_ORIGALBUM,
	ID3FID_PUBLISHER,
	ID3FID_ENCODEDBY,
	ID3FID_ENCODERSETTINGS,
	ID3FID_ORIGFILENAME,
	ID3FID_LANGUAGE,
	ID3FID_PARTINSET,
	ID3FID_DATE,
	ID3FID_TIME,
	ID3FID_RECORDINGDATES,
	ID3FID_MEDIATYPE,
	ID3FID_FILETYPE,
	ID3FID_NETRADIOSTATION,
	ID3FID_NETRADIOOWNER,
	ID3FID_LYRICIST,
	ID3FID_ORIGARTIST,
	ID3FID_ORIGLYRICIST,
	ID3FID_SUBTITLE,
	ID3FID_MIXARTIST,
	ID3FID_USERTEXT,
	ID3FID_CONTENTGROUP,
	ID3FID_TITLE,
	ID3FID_LEADARTIST,
	ID3FID_BAND,
	ID3FID_ALBUM,
	ID3FID_YEAR,
	ID3FID_CONDUCTOR,
	ID3FID_COMPOSER,
	ID3FID_COPYRIGHT,
	ID3FID_CONTENTTYPE,
	ID3FID_TRACKNUM,
	ID3FID_COMMENT,
	ID3FID_WWWAUDIOFILE,
	ID3FID_WWWARTIST,
	ID3FID_WWWAUDIOSOURCE,
	ID3FID_WWWCOMMERCIALINFO,
	ID3FID_WWWCOPYRIGHT,
	ID3FID_WWWPUBLISHER,
	ID3FID_WWWPAYMENT,
	ID3FID_WWWRADIOPAGE,
	ID3FID_WWWUSER,
	ID3FID_INVOLVEDPEOPLE,
	ID3FID_UNSYNCEDLYRICS,
	ID3FID_PICTURE,
	ID3FID_GENERALOBJECT,
	ID3FID_UNIQUEFILEID,
	ID3FID_PLAYCOUNTER,
	ID3FID_POPULARIMETER,
	ID3FID_GROUPINGREG,
	ID3FID_CRYPTOREG
};


enum ID3_VerCtl
{
	ID3VC_HIGHER	= 0,
	ID3VC_LOWER
};


struct ID3_FieldDef
{
ID3_FieldID		id;
ID3_FieldType	type;
lsint			fixedLength;
uchar			version;
uchar			revision;
ID3_VerCtl		control;
luint			flags;
ID3_FieldID		linkedField;
};


class ID3_Frame;


struct ID3_FrameDef
{
ID3_FrameID		id;
char			*shortTextID;
char			*longTextID;
lsint			priority;
bool			tagDiscard;
bool			fileDiscard;
bool			(*parseHandler)						( ID3_Frame *frame );
ID3_FieldDef	*fieldDefs;
};


class ID3_Field
{
public:
				ID3_Field						( void );
				~ID3_Field						( void );

void			Clear							( void );

#ifdef FULL_ID3V2
luint			Size							( void );
#endif //FULL_ID3V2

luint			GetNumTextItems					( void );

// integer field functions
ID3_Field&		operator=						( luint newData );
void			Set								( luint newData );
luint			Get	
							( void );
// Unicode string field functions
#ifdef FULL_ID3V2
ID3_Field&		operator=						( wchar_t *string );
#endif //FULL_ID3V2

void			Set								( wchar_t *string );
luint			Get								( wchar_t *buffer, luint maxChars, luint itemNum = 1 );

#ifdef FULL_ID3V2
void			Add								( wchar_t *string );
#endif //FULL_ID3V2

// ASCII string field functions
#ifdef FULL_ID3V2
ID3_Field&		operator=						( char *string );
#endif //FULL_ID3V2

void			Set								( char *string );
luint			Get								( char *buffer, luint maxChars, luint itemNum = 1 );

#ifdef FULL_ID3V2
void			Add								( char *string );
#endif //FULL_ID3V2

// binary field functions
void			Set								( uchar *newData, luint newSize );

#ifdef FULL_ID3V2
void			Get								( uchar *buffer, luint buffLength );
void			FromFile						( TCHAR *info );
void			ToFile							( TCHAR *info );
// *** PRIVATE INTERNAL DATA - DO NOT USE *** PRIVATE INTERNAL DATA - DO NOT USE ***
luint			BinSize							( bool withExtras = true );
bool			HasChanged						( void );
#endif //FULL_ID3V2

void			SetVersion						( uchar ver, uchar rev );

#ifdef FULL_ID3V2
luint			Render							( uchar *buffer );
#endif //FULL_ID3V2

luint			Parse							( uchar *buffer, luint posn, luint buffSize );
ID3_FieldID		name;							// the ID of this field
ID3_FieldType	type;							// what type is this field or should be
lsint			fixedLength;					// if this is positive, the length of the field is fixed
uchar			ioVersion;						// specific version
uchar			ioRevision;						// specific revision
ID3_VerCtl		control;						// render if ver/rev is higher, or lower than frame::version, frame::revision?
luint			flags;							// special field flags
uchar			version;						// the version being rendered/parsed
uchar			revision;						// the revision being rendered/parsed
bool			hasChanged;						// has the field changed since the last parse/render?
protected:

#ifdef FULL_ID3V2
luint			RenderInteger					( uchar *buffer );
luint			RenderASCIIString				( uchar *buffer );
luint			RenderUnicodeString				( uchar *buffer );
luint			RenderBinary					( uchar *buffer );
#endif //FULL_ID3V2

luint			ParseInteger					( uchar *buffer, luint posn, luint buffSize );
luint			ParseASCIIString				( uchar *buffer, luint posn, luint buffSize );

#ifdef FULL_ID3V2
luint			ParseUnicodeString				( uchar *buffer, luint posn, luint buffSize );
#endif //FULL_ID3V2

luint			ParseBinary						( uchar *buffer, luint posn, luint buffSize );

uchar			*data;
luint			size;
};


ID3_FrameDef	*ID3_FindFrameDef				( ID3_FrameID id );
ID3_FrameID		ID3_FindFrameID					( char *id );


#endif


