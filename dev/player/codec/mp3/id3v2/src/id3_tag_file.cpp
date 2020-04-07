//........................................................................................
//........................................................................................
//.. File Name: id3_tag_file.cpp
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
#include <codec/mp3/id3v2/id3_tag.h>
//#include <tchar.h> (dr) 
#include <datastream/input/InputStream.h>
#include <util/debug/debug.h>

DEBUG_USE_MODULE(ID3V2);

#ifdef FULL_ID3V2
#include "wincehelper.h"

#define DWORD unsigned long

bool			exists							( TCHAR *name )
{
	bool	doesExist	= false;

	if	( name )
	{
#ifdef UNDER_CE
		WIN32_FIND_DATA fd;
		HANDLE hFile = FindFirstFile(name,&fd);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			doesExist = true;
			CloseHandle(hFile);
		}
#else
		FILE	*in;

		if	( in = fopen ( name, "rb" ) )
		{
			doesExist = true;
			fclose ( in );
		}
#endif
	}
	else
		ID3_THROW ( ID3E_NoData );

	return doesExist;
}

#endif //FULL_ID3V2

void			ID3_Tag::OpenLinkedFile			( void )
{
/*
#ifdef UNDER_CE
	DWORD dwError;
	fileHandle = CreateFile( fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetFileSize(fileHandle,NULL);
		fileSize = dw;
		//fileSize = GetFileSize(fileHandle,&dw);
	}
#else
	char	*mode		= "rb+";

	if	( ! exists ( fileName ) )
		mode = "wb+";


	if	( fileHandle = fopen ( fileName, mode ) )
	{
		fseek ( fileHandle, 0, SEEK_END );
		fileSize = ftell ( fileHandle );
		fseek ( fileHandle, 0, SEEK_SET );
	}
*/


//  DWORD dw;
// 	m_pIStream->FileSize(&dw); (using FileInputStream)
  fileSize = m_pIStream->Length();
  if (m_pIStream->CanSeek())
    m_pIStream->Seek(IInputStream::SeekStart, 0);
  else
  {
    //TODO:stop
  }
//	fileSize = dw;
	return;
 
  }   


luint			ID3_Tag::Link					( IInputStream* pIStream )
{
	luint	posn	= 0;

	if	( pIStream )
	{
		//_tcsncpy ( fileName, fileInfo, 256 );

		// if we were attached to some other file
		// file then abort
		if	( m_pIStream && (m_pIStream != pIStream) )
			ID3_THROW ( ID3E_TagAlreadyAttached );

		//GenerateTempName();
		m_pIStream = pIStream;
		OpenLinkedFile();
		oldTagSize = ParseFromHandle();

		if	( oldTagSize )
			oldTagSize += ID3_TAGHEADERSIZE;

		fileSize -= oldTagSize;
		posn = oldTagSize;
	}
	else
		ID3_THROW ( ID3E_NoData );

	return posn;
}

#ifdef FULL_ID3V2

void			ID3_Tag::Update					( void )
{
//	if	( HasChanged() )
//		RenderToHandle();

	return;
}

/*
void			ID3_Tag::Strip					( bool v1Also )
{
#ifdef UNDER_CE
	HANDLE   tempOut;
#else
	FILE	*tempOut;
#endif
	if	( _tcslen ( tempName ) > 0 )
	{
#ifdef UNDER_CE
		if ( (tempOut = CreateFile(tempName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL)) != INVALID_HANDLE_VALUE )
#else
		if	( tempOut = fopen ( tempName, "wb" ) )
#endif
		{
			uchar	*buffer2;

#ifdef UNDER_CE
			SetFilePointer( fileHandle, oldTagSize, NULL, FILE_BEGIN);
#else
			fseek ( fileHandle, oldTagSize, SEEK_SET );
#endif
			if	( buffer2 = new uchar[ BUFF_SIZE ] )
			{
				bool	done		= false;
				luint	bytesCopied	= 0;
				luint	bytesToCopy	= fileSize;
#ifndef UNDER_CE
				int		i;
#endif

				if	( hasV1Tag && v1Also )
					bytesToCopy -= extraBytes;
#ifdef UNDER_CE
				while	( ! IsEof( fileHandle ) && ! done )
#else
				while	( ! feof ( fileHandle ) && ! done )

#endif
				{
					luint	size	= BUFF_SIZE;

					if	( ( bytesToCopy - bytesCopied ) < BUFF_SIZE )
					{
						size = bytesToCopy - bytesCopied;
						done = true;
					}


#ifdef UNDER_CE
					DWORD dw;
					ReadFile(fileHandle,buffer2,size,&dw,NULL);
					WriteFile(tempOut,buffer2,dw,&dw,NULL);
					bytesCopied += dw;
#else
					if	( i = fread ( buffer2, 1, size, fileHandle ) )
						fwrite ( buffer2, 1, i, tempOut );
					bytesCopied += i;
#endif

				}

				delete[] buffer2;
			}

#ifdef UNDER_CE
			CloseHandle ( tempOut );
			CloseHandle ( fileHandle );
			DeleteFile( fileName );
			MoveFile(tempName, fileName);
#else
			fclose ( tempOut );
			fclose ( fileHandle );
			remove ( fileName );
			rename ( tempName, fileName );
#endif

			OpenLinkedFile();

			oldTagSize = 0;
			extraBytes = 0;

			if	( v1Also )
				hasV1Tag = false;

			hasChanged = true;
		}
	}

	return;
}
*/

#endif //FULL_ID3V2
