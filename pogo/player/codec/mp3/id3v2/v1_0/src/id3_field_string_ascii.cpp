//........................................................................................
//........................................................................................
//.. File Name: id3_field_string_ascii.cpp
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


#include <stdlib.h>
#include <codec/mp3/id3v2/id3_field.h>
#include <codec/mp3/id3v2/id3_misc_support.h>
#include <util/debug/debug.h>

DEBUG_USE_MODULE(ID3V2);

#ifdef FULL_ID3V2
ID3_Field&		ID3_Field::operator=			( char *string )
{
	Set ( string );

	return *this;
}


#endif //FULL_ID3V2

void			ID3_Field::Set					( char *newString )
{
	if	( newString )
	{
		wchar_t	*temp;

		Clear();

		if	( temp = new wchar_t[ strlen ( newString ) + 1 ] )
		{
			ID3_ASCIItoUnicode ( temp, newString, strlen ( newString ) + 1 );
			Set ( temp );
			delete[] temp;

			type = ID3FTY_ASCIISTRING;
		}
	}

	return;
}


luint			ID3_Field::Get					( char *buffer, luint maxLength, luint itemNum )
{
	luint	bytesUsed	= 0;
	wchar_t	*temp;
	char	*ascii;

	if	( temp = new wchar_t[ maxLength ] )
	{
		luint	len;

		if	( len = Get ( temp, maxLength, itemNum ) )
		{
			if	( ascii = new char[ len + 1 ] )
			{
				luint	length;

				ID3_UnicodeToASCII ( ascii, temp, len + 1 );

				length = MIN ( strlen ( ascii ), maxLength );

				strncpy ( buffer, ascii, length );
				buffer[ length ] = 0;
				bytesUsed = length;

				delete[] ascii;
			}
			else
			    ID3_THROW ( ID3E_NoMemory );
		}

		delete[] temp;
	}
	else
        ID3_THROW ( ID3E_NoMemory );

	return bytesUsed;
}


#ifdef FULL_ID3V2

void			ID3_Field::Add					( char *newString )
{
	if	( newString )
	{
		wchar_t	*temp;

		if	( temp = new wchar_t[ strlen ( newString ) + 1 ] )
		{
			ID3_ASCIItoUnicode ( temp, newString, strlen ( newString ) + 1 );
			Add ( temp );
			delete[] temp;

			type = ID3FTY_ASCIISTRING;
		}
	}

	return;
}

#endif //FULL_ID3V2


luint			ID3_Field::ParseASCIIString		( uchar *buffer, luint posn, luint buffSize )
{
	luint	bytesUsed	= 0;
	char	*temp		= NULL;

	if	( fixedLength != -1 )
		bytesUsed = fixedLength;
	else
	{
		if	( flags & ID3FF_NULL )
			while	( ( posn + bytesUsed ) < buffSize && buffer[ posn + bytesUsed ] != 0 )
				bytesUsed++;
		else
			bytesUsed = buffSize - posn;
	}

	if	( bytesUsed )
	{
		if	( temp = new char[ bytesUsed + 1 ] )
		{
			memcpy ( temp, &buffer[ posn ], bytesUsed );
			temp[ bytesUsed ] = 0;

			Set ( temp );

			delete[] temp;
		}
		else
			ID3_THROW ( ID3E_NoMemory );
	}

	if	( flags & ID3FF_NULL )
		bytesUsed++;

	hasChanged = false;

	return bytesUsed;
}


#ifdef FULL_ID3V2

luint			ID3_Field::RenderASCIIString	( uchar *buffer )
{
	luint	bytesUsed	= 0;
	char	*ascii;

	bytesUsed = BinSize();

	if	( data && size )
	{
		if	( ascii = new char[ size ] )
		{
			luint	i;

			ID3_UnicodeToASCII ( ascii, (wchar_t *) data, size );
			memcpy ( buffer, (uchar *) ascii, bytesUsed );

			// now we convert the internal dividers to what they
			// are supposed to be
			for	( i = 0; i < bytesUsed; i++ )
				if	( buffer[ i ] == 1 )
				{
					char	sub	=	'/';

					if	( flags & ID3FF_NULLDIVIDE )
						sub = '\0';

					buffer[ i ] = sub;
				}

			if	( size - 1 < bytesUsed )
				for	( i = 0; i < ( size - 1 - bytesUsed ); i++ )
					buffer[ bytesUsed + i ] = 0x20;

			delete[] ascii;
		}
		else
			ID3_THROW ( ID3E_NoMemory );
	}

	if	( bytesUsed == 1 && flags & ID3FF_NULL )
		buffer[ 0 ] = 0;

	hasChanged = false;

	return bytesUsed;
}

#endif //FULL_ID3V2

