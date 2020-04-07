//........................................................................................
//........................................................................................
//.. File Name: id3_tag_parse_v1.cpp
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

#ifdef FULL_ID3V2

#include <stdio.h>
#include <string.h>
//#include <memory.h>
#include <codec/mp3/id3v2/id3_tag.h>
#include <codec/mp3/id3v2/id3_misc_support.h>


void			ID3_RemoveTrailingSpaces		( char *buffer, luint length )
{
	for	( lsint i = length - 1; i > -1; i-- )
		if	( buffer[ i ] == 0x20 )
			buffer[ i ] = 0;
		else
			break;

	return;
}


void			ID3_Tag::ParseID3v1				( void )
{
//	if	( fileHandle )
	if(m_pIStream)
	{
		ID3V1_Tag	temp;

		// posn ourselves at 128 bytes from the end of the file
/*
#ifdef UNDER_CE
		DWORD dw;
		SetFilePointer(fileHandle,-128,NULL,FILE_END);
		ReadFile(fileHandle,&temp,128,&dw,NULL);
#else
		fseek ( fileHandle, -128, SEEK_END );
		// read the next 128 bytes in;
		fread ( &temp, 1, 128, fileHandle );
#endif
*/
//		DWORD dw;
        if (m_pIStream->CanSeek())
            m_pIStream->Seek(-128,IInputStream::SeekEnd);
        else
        {
            //TODO:stop
        }
//		m_pIStream->Read((unsigned char*)&temp,128,&dw); (dr)
		m_pIStream->Read((unsigned char*)&temp,128);

		ID3_RemoveTrailingSpaces ( temp.title, sizeof ( temp.title ) );
		ID3_RemoveTrailingSpaces ( temp.artist, sizeof ( temp.artist ) );
		ID3_RemoveTrailingSpaces ( temp.album, sizeof ( temp.album ) );

		if	( temp.comment[ 28 ] == 0 && temp.comment[ 29 ] != 0 )
			ID3_RemoveTrailingSpaces ( temp.comment, sizeof ( temp.comment ) - 1 );
		else
			ID3_RemoveTrailingSpaces ( temp.comment, sizeof ( temp.comment ) );

		// check to see if it was a tag
		if	( memcmp ( temp.ID, "TAG", 3 ) == 0 )
		{
			// guess so, let's start checking the v2 tag for frames
			// which are the equivalent of the v1 fields.  When we
			// come across a v1 field that has no current equivalent
			// v2 frame, we create the frame, copy the data from the
			// v1 frame and attach it to the tag

			hasV1Tag = true;
			extraBytes += sizeof ( ID3V1_Tag );

			ID3_AddTitle ( this, temp.title );
			ID3_AddArtist ( this, temp.artist );
			ID3_AddAlbum ( this, temp.album );

			// the YEAR field/frame
			if	( Find ( ID3FID_YEAR ) == NULL && temp.year[ 0 ] != 0 )
			{
				ID3_Frame	*yearFrame;

				if	( yearFrame = new ID3_Frame )
				{
					char	buff[ 5 ];

					strncpy ( buff, temp.year, 4 );
					buff[ 4 ] = 0;

					yearFrame->SetID ( ID3FID_YEAR );
					yearFrame->Field ( ID3FN_TEXT ) = buff;
					AddFrame ( yearFrame, true );
				}
				else
					ID3_THROW ( ID3E_NoMemory );
			}

			// the COMMENT field/frame
			if	( Find ( ID3FID_COMMENT ) == NULL && strlen ( temp.comment ) > 0 )
			{
				ID3_Frame	*commentFrame;

				if	( commentFrame = new ID3_Frame )
				{
					commentFrame->SetID ( ID3FID_COMMENT );
					commentFrame->Field ( ID3FN_LANGUAGE ) = "eng";
					commentFrame->Field ( ID3FN_TEXT ) = temp.comment;
					AddFrame ( commentFrame, true );
				}
				else
					ID3_THROW ( ID3E_NoMemory );
			}

			// the TRACK field/frame
			if	( temp.comment[ 28 ] == 0 && temp.comment[ 29 ] != 0 )
			{
				if	( Find ( ID3FID_TRACKNUM ) == NULL )
				{
					ID3_Frame	*trackFrame;

					if	( trackFrame = new ID3_Frame )
					{
						char	buff[ 4 ];

						sprintf ( buff, "%d", (luint) temp.comment[ 29 ] );

						trackFrame->SetID ( ID3FID_TRACKNUM );
						trackFrame->Field ( ID3FN_TEXT ) = buff;
						AddFrame ( trackFrame, true );
					}
					else
						ID3_THROW ( ID3E_NoMemory );
				}
			}

			//TODO : Add hook into genre table from ID3v1
			// the GENRE field/frame
			if	( temp.genre != 255 )
			{
				if	( Find ( ID3FID_CONTENTTYPE ) == NULL )
				{
					ID3_Frame	*genreFrame;

					if	( genreFrame = new ID3_Frame )
					{
						char	buff[ 6 ];

						sprintf ( buff, "(%d)", (luint) temp.genre );

						genreFrame->SetID ( ID3FID_CONTENTTYPE );
						genreFrame->Field ( ID3FN_TEXT ) = buff;
						AddFrame ( genreFrame, true );
					}
					else
						ID3_THROW ( ID3E_NoMemory );
				}
			}

		}
	}
	else
		ID3_THROW ( ID3E_NoData );

	return;
}

#endif FULL_ID3V2

