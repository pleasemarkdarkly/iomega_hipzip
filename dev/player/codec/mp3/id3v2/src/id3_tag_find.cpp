//........................................................................................
//........................................................................................
//.. File Name: id3_tag_find.cpp
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


#include <codec/mp3/id3v2/id3_tag.h>

#ifdef FULL_ID3V2

#include <codec/mp3/id3v2/id3_misc_support.h>
#include "uniwrap.h"


ID3_Elem		*ID3_Tag::Find					( ID3_Frame *frame )
{
	ID3_Elem	*elem	= NULL;
	ID3_Elem	*cur	= frameList;
	bool		done	= false;

	while	( ! done && cur )
	{
		if	( cur->frame == frame )
		{
			elem = cur;
			done = true;
		}
		else
			cur = cur->next;
	}

	return elem;
}

#endif //FULL_ID3V2


ID3_Frame		*ID3_Tag::Find					( ID3_FrameID id )
{
	ID3_Frame	*frame	= NULL;
	ID3_Elem	*cur	= findCursor;
	bool		done	= false;

	if	( cur == NULL )
		findCursor = cur = frameList;

	while	( ! done && cur )
	{
		if	( cur->frame && ( cur->frame->GetID() == id ) )
		{
			frame = cur->frame;

			if	( frame )
			{
				findCursor = cur->next;
				done = true;
			}
		}
		else
			cur = cur->next;

		if	( cur == NULL )
			cur = frameList;

		if	( cur == findCursor )
			done = true;
	}

	return frame;
}

#ifdef FULL_ID3V2

ID3_Frame		*ID3_Tag::Find					( ID3_FrameID id, ID3_FieldID fld, char *data )
{
	ID3_Frame	*frame	= NULL;
	wchar_t		*temp;

	if	( temp = new wchar_t[ strlen ( data ) + 1 ] )
	{
		ID3_ASCIItoUnicode ( temp, data, strlen ( data ) + 1 );

		frame = Find ( id, fld, temp );

		delete[] temp;
	}

	return frame;
}


ID3_Frame		*ID3_Tag::Find					( ID3_FrameID id, ID3_FieldID fld, wchar_t *data )
{
	ID3_Frame	*frame	= NULL;
	ID3_Elem	*cur	= findCursor;
	bool		done	= false;

	if	( cur == NULL )
		findCursor = cur = frameList;

	while	( ! done && cur )
	{
		if	( cur->frame && ( cur->frame->GetID() == id ) )
		{
			frame = cur->frame;

			if	( data && wcslen ( data ) && BS_ISSET ( frame->fieldBits, fld ) )
			{
				wchar_t	*buffer;
				luint	size;

				size = frame->Field ( fld ).BinSize();

				if	( buffer = new wchar_t[ size ] )
				{
					frame->Field ( fld ).Get ( buffer, size );

					if	( wcscmp ( buffer, data ) != 0 )
					{
						frame = NULL;
						cur = cur->next;
					}

					delete[] buffer;
				}
			}

			if	( frame )
			{
				findCursor = cur->next;
				break;
			}
		}
		else
			cur = cur->next;

		if	( cur == NULL )
			cur = frameList;

		if	( cur == findCursor )
			done = true;
	}

	return frame;
}


ID3_Frame		*ID3_Tag::Find					( ID3_FrameID id, ID3_FieldID fld, luint data )
{
	ID3_Frame	*frame	= NULL;
	ID3_Elem	*cur	= findCursor;
	bool		done	= false;

	if	( cur == NULL )
		findCursor = cur = frameList;

	while	( ! done && cur )
	{
		if	( cur->frame && ( cur->frame->GetID() == id ) )
		{
			frame = cur->frame;

			if	( frame->Field ( fld ).Get() != data )
			{
				frame = NULL;
				cur = cur->next;
			}

			if	( frame )
			{
				findCursor = cur->next;
				break;
			}
		}
		else
			cur = cur->next;

		if	( cur == NULL )
			cur = frameList;

		if	( cur == findCursor )
			done = true;
	}

	return frame;
}


ID3_Frame		*ID3_Tag::GetFrameNum			( luint num )
{
	ID3_Frame	*frame	= NULL;
	ID3_Elem	*cur	= frameList;
	bool		done	= false;
	luint		curNum	= 0;

	while	( cur && ! done )
	{
		if	( num == curNum )
		{
			frame = cur->frame;
			done = true;
		}
		else
		{
			curNum++;
			cur = cur->next;
		}
	}

	return frame;
}


ID3_Frame		*ID3_Tag::operator[]			( luint num )
{
	return GetFrameNum ( num );
}


#endif //FULL_ID3V2
