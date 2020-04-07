//........................................................................................
//........................................................................................
//.. File Name: uniwrap.h
//.. Last Modified By: Donni Reitz-Pesek	donni@iobjects.com
//.. Modification date: 2/17/2000
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.
//..	 All rights reserved. This code may not be redistributed in source or linkable
//.. 	 object form without the express written consent of Interactive Objects.
//.. Contact Information: www.iobjects.com
//........................................................................................
//........................................................................................
//the unicode functions that will need to be written

#include<string.h>
#include<stdlib.h>
#include <codec/mp3/id3v2/id3_field.h>
//#include"uniwrap.cpp"

luint wcslen(wchar_t* string);
void wcsncpy (wchar_t * buffer, wchar_t* source, luint actualChars );
void wcscpy ( wchar_t *temp, wchar_t *data );
luint wcscmp ( wchar_t * buffer, wchar_t * data );
