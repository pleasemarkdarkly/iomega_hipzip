//
// tchar.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef TCHAR_H_
#define TCHAR_H_

//! The TCHAR type is a double-byte character suitable for Unicode strings.
//! This module contains functions for manipulating TCHAR strings.

/** \addtogroup tchar tchar */
//@{

#include <string.h>

typedef unsigned short TCHAR;

//! Appends the source string to the destination string.
//! Both strings must be null-terminated.
//! Returns a pointer to \e strDestination, the concatenated string.
TCHAR* tstrcat(TCHAR* strDestination, const TCHAR* strSource);
//! Appends the specified number of characters from the source string to the destination string.
//! The destination string must be null-terminated.
//! After concatenation the destination string will be null-terminated, no matter if
//! the source string is shorter or longer than the number of characters appended.
TCHAR* tstrncat(TCHAR* strDestination, const TCHAR* strSource, int count);
//! Compares the two strings.
//! \retval -1 If \e string1 < \e string2
//! \retval 0 If \e string1 == \e string2
//! \retval 1 if \e string1 > \e string2
int tstrcmp(const TCHAR* string1, const TCHAR* string2);
//! Compares the first \e count characters of the two strings.
//! \retval -1 If \e string1 < \e string2
//! \retval 0 If \e string1 == \e string2
//! \retval 1 if \e string1 > \e string2
int tstrncmp(const TCHAR* string1, const TCHAR* string2, int count);
//! Compares the two strings, ignoring case.
//! \retval -1 If \e string1 < \e string2
//! \retval 0 If \e string1 == \e string2
//! \retval 1 if \e string1 > \e string2
int tstricmp(const TCHAR* string1, const TCHAR* string2);
//! Compares the first \e count characters of the two strings, ignoring case.
//! \retval -1 If \e string1 < \e string2
//! \retval 0 If \e string1 == \e string2
//! \retval 1 if \e string1 > \e string2
int tstrnicmp(const TCHAR* string1, const TCHAR* string2, int count);
//! Copies the source string to the destination string.
//! Returns a pointer to \e strDestination, the copied string.
TCHAR* tstrcpy(TCHAR* strDestination, const TCHAR* strSource);
//! Copies the specified number of characters from the source string to the destination string.
//! Returns a pointer to \e strDestination, the copied string.
TCHAR* tstrncpy(TCHAR* strDestination, const TCHAR* strSource, int count);
//! Returns the length of the null-terminated string passed in.
int tstrlen(const TCHAR* string);
//! Returns a pointer to the first instance of 'needle' in 'haystack'
TCHAR* tstrstr(const TCHAR* strHaystack, const TCHAR* strNeedle);
//! Returns a pointer to the first instance of 'needle' in 'haystack'
TCHAR* tstrstrn(const TCHAR* strHaystack, const TCHAR* strNeedle, int count);
//! Converts a string to an integer and returns it, or 0 if no string can be found
int tatoi(const TCHAR* string);


//! Converts the source single-byte string into a double-byte string.
//! Returns a pointer to \e strDestination, the converted string.
TCHAR* CharToTchar(TCHAR* strDestination, const char* strSource);
//! Converts the specified number of characters from the source single-byte string into a double-byte string.
//! Returns a pointer to \e strDestination, the converted string.
TCHAR* CharToTcharN(TCHAR* strDestination, const char* strSource, int count);
//! Converts the source double-byte string into a single-byte string.
//! Returns a pointer to \e strDestination, the converted string.
char* TcharToChar(char* strDestination, const TCHAR* strSource);
//! Converts the specified number of characters from the source double-byte string into a single-byte string.
//! Returns a pointer to \e strDestination, the converted string.
char* TcharToCharN(char* strDestination, const TCHAR* strSource, int count);
//! Duplicates a string.
//! Returns a pointer to the newly-allocated copy of the string.
TCHAR* tstrdup(const TCHAR* string);

//@}


#endif	// CCDDAINPUTSTREAM_H_
