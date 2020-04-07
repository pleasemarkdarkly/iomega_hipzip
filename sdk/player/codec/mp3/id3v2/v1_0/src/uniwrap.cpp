//........................................................................................
//........................................................................................
//.. File Name: uniwrap.cpp
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

#include <codec/mp3/id3v2/id3_types.h>
#include "uniwrap.h"


luint wcslen(wchar_t* string){
//  return (strlen((const char*)string)/2);
  int len = 0;
  while (string[len] != 0)
    ++len;
  return len;
}

void wcsncpy (wchar_t * buffer, wchar_t* source, luint actualChars ){
//  strncpy((char *)buffer, (const char*)source, actualChars);
  memcpy((char *)buffer, (const char*)source, actualChars * sizeof(wchar_t));
}

void wcscpy ( wchar_t *temp, wchar_t *data ){
//  strcpy((char *)temp, (const char *)data);
  wcsncpy(temp, data, wcslen(data));
}

luint wcscmp ( wchar_t * buffer, wchar_t * data ){
//  return strcmp((const char *)buffer, (const char *)data);
  int cur = 0;
  while (buffer[cur] && data[cur])
    if (buffer[cur] != data[cur])
      return 1;
  return 0;
}
			