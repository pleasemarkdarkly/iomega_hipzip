//........................................................................................
//........................................................................................
//.. File Name: id3_field_binary.cpp
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


#include <stdio.h>
#include <codec/mp3/id3v2/id3_field.h>
#include <util/debug/debug.h>

DEBUG_USE_MODULE(ID3V2);

void			ID3_Field::Set					( uchar *newData, luint newSize )
{
	Clear();

	if	( newSize )
	{
		if	( ! ( data = new uchar[ newSize ] ) )
			ID3_THROW ( ID3E_NoMemory );

		memcpy ( data, newData, newSize );
		size = newSize;

		type = ID3FTY_BINARY;
		hasChanged = true;
	}

	return;
}

#ifdef FULL_ID3V2

void			ID3_Field::Get					( uchar *buffer, luint buffLength )
{
	if	( ! buffer )
		ID3_THROW ( ID3E_NoBuffer );

	if	( data && size )
	{
		luint	actualBytes	= MIN ( buffLength, size );

		memcpy ( buffer, data, actualBytes );
	}

	return;
}


void			ID3_Field::FromFile				( TCHAR *info )
{
#ifdef UNDER_CE
	HANDLE temp;
#else
	FILE	*temp;
	luint	fileSize;
#endif
	uchar	*buffer;

	if	( ! info )
		ID3_THROW ( ID3E_NoData );

#ifdef UNDER_CE
	if	( (temp = CreateFile( info , GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE )
	{
		DWORD dwFileSize = GetFileSize(temp, NULL);

		if	( buffer = new uchar[ dwFileSize ] )
		{
			ReadFile(temp,buffer,dwFileSize,&dwFileSize,NULL);
			Set ( buffer, dwFileSize );
			delete[] buffer;
		}
		CloseHandle( temp );
	}
#else
	if	( temp = fopen ( info, "rb" ) )
	{
    diag_printf("Fix id3_field_binary.cpp\n");
    //(dr) fix me
//		fseek ( temp, 0, SEEK_END );
//		fileSize = ftell ( temp );
//		fseek ( temp, 0, SEEK_SET );

		if	( buffer = new uchar[ fileSize ] )
		{
			fread ( buffer, 1, fileSize, temp );

			Set ( buffer, fileSize );

			delete[] buffer;
		}

		fclose ( temp );
	}
#endif
	return;
}

void			ID3_Field::ToFile				( TCHAR *info )
{
	if	( ! info )
		ID3_THROW ( ID3E_NoData );

	if	( ( data != NULL ) && ( size > 0 ) )
	{
#ifdef UNDER_CE
		HANDLE hFile;
		hFile = CreateFile(info,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			WriteFile(hFile,data,size,NULL,NULL);
			CloseHandle(hFile);
		}
#else
		FILE	*temp;

		if	( temp = fopen ( info, "wb" ) )
		{
			fwrite ( data, 1, size, temp );
			fclose ( temp );
		}
#endif
	}

	return;
}

#endif //FULL_ID3V2

luint			ID3_Field::ParseBinary			( uchar *buffer, luint posn, luint buffSize )
{
	luint	bytesUsed	= 0;

	bytesUsed = buffSize - posn;

	if	( fixedLength != -1 )
		bytesUsed = MIN ( fixedLength, bytesUsed );

	Set ( &buffer[ posn ], bytesUsed );

	hasChanged = false;

	return bytesUsed;
}

#ifdef FULL_ID3V2

luint			ID3_Field::RenderBinary			( uchar *buffer )
{
	luint	bytesUsed	= 0;

	bytesUsed = BinSize();
	memcpy ( buffer, (uchar *) data, bytesUsed );

	hasChanged = false;

	return bytesUsed;
}

#endif //FULL_ID3V2

