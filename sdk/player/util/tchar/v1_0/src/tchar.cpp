//
// tchar.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <util/tchar/tchar.h>

#include <util/debug/debug.h>
#include <stdlib.h> // malloc

DEBUG_MODULE_S(TCHAR, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(TCHAR);

TCHAR*
CharToTchar(TCHAR* strDestination, const char* strSource)
{
    TCHAR* str = strDestination;
    while (*strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

TCHAR*
CharToTcharN(TCHAR* strDestination, const char* strSource, int count)
{
    TCHAR* str = strDestination;
    while (count-- && *strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

char*
TcharToChar(char* strDestination, const TCHAR* strSource)
{
    char* str = strDestination;
    while (*strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

char*
TcharToCharN(char* strDestination, const TCHAR* strSource, int count)
{
    char* str = strDestination;
    while (count-- && *strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

TCHAR*
tstrdup(const TCHAR* string)
{
    TCHAR* strCopy = (TCHAR*)malloc((tstrlen(string) + 1) * sizeof(TCHAR));
    DBASSERT(TCHAR, strCopy, "Error allocating destination string");
    tstrcpy(strCopy, string);
    return strCopy;
}

TCHAR*
tstrcat(TCHAR* strDestination, const TCHAR* strSource)
{
    TCHAR* str = strDestination;
    while (*str)
        ++str;
    while (*strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

TCHAR*
tstrncat(TCHAR* strDestination, const TCHAR* strSource, int count)
{
    TCHAR* str = strDestination;
    while (*str)
        ++str;
    for (; count && *strSource; --count)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

int
tstrcmp(const TCHAR* string1, const TCHAR* string2)
{
    while (*string1 && *string2 && (*string1 == *string2))
    {
        ++string1;
        ++string2;
    }

    if (!*string2)
        if (!*string1)
            return 0;
        else
            return 1;
    else if (!*string1)
        return -1;
    else if (*string1 < *string2)
        return -1;
    else
        return 1;
}

int
tstrncmp(const TCHAR* string1, const TCHAR* string2, int count)
{
    while (count && *string1 && *string2 && (*string1 == *string2))
    {
        ++string1;
        ++string2;
        --count;
    }

    if (!*string2)
        if (!*string1)
            return 0;
        else
            return 1;
    else if (!*string1)
        return -1;
    else if (*string1 < *string2)
        return -1;
    else
        return 1;
}

#define ttolower(ch)    ( ((ch) >= 'A') && ((ch) <= 'Z') ? (ch) - 'A' + 'a' : ch )

int
tstricmp(const TCHAR* string1, const TCHAR* string2)
{
    while (*string1 && *string2 && ((*string1 == *string2) || (ttolower(*string1) == ttolower(*string2))))
    {
        ++string1;
        ++string2;
    }

    if (!*string2)
        if (!*string1)
            return 0;
        else
            return 1;
    else if (!*string1)
        return -1;
    else if (ttolower(*string1) < ttolower(*string2))
        return -1;
    else
        return 1;
}

int
tstrnicmp(const TCHAR* string1, const TCHAR* string2, int count)
{
    while (count && *string1 && *string2 && ((*string1 == *string2) || (ttolower(*string1) == ttolower(*string2))))
    {
        ++string1;
        ++string2;
        --count;
    }

    if (!*string2)
        if (!*string1)
            return 0;
        else
            return 1;
    else if (!*string1)
        return -1;
    else if (ttolower(*string1) < ttolower(*string2))
        return -1;
    else
        return 1;
}

TCHAR*
tstrcpy(TCHAR* strDestination, const TCHAR* strSource)
{
    TCHAR* str = strDestination;
    while (*strSource)
        *str++ = *strSource++;
    *str = '\0';
    return strDestination;
}

TCHAR*
tstrncpy(TCHAR* strDestination, const TCHAR* strSource, int count)
{
    TCHAR* str = strDestination;
    while (count-- && *strSource)
        *str++ = *strSource++;
    *str++ = '\0';
    return strDestination;
}

int
tstrlen(const TCHAR* string)
{
    int i = 0;
    while (*string++)
        ++i;

    return i;
}

TCHAR*
tstrstr(const TCHAR* strHaystack, const TCHAR* strNeedle)
{
    while( *strHaystack ) {
        int i;
        for( i=0; ; i++ ) {
            if( strNeedle[i] == 0 ) {
                return (TCHAR*)strHaystack;
            }
            if( strNeedle[i] != strHaystack[i] ) {
                break;
            }
        }
        strHaystack++;
    }
    return (TCHAR*) NULL;
}

TCHAR*
tstrstrn(const TCHAR* strHaystack, const TCHAR* strNeedle, int count )
{
    while( count ) {
        int i;
        for( i=0; ; i++ ) {
            if( strNeedle[i] == 0 ) {
                return (TCHAR*)strHaystack;
            }
            if( strNeedle[i] != strHaystack[i] ) {
                break;
            }
        }
        strHaystack++;
        count--;
    }
    return (TCHAR*) NULL;
}

int
tatoi(const TCHAR* string)
{
    int res = 0;

    while( *string && *string >= '0' && *string <= '9' ) {
        res *= 10;
        res += (int)(*string - '0');
        string++;
    }
    return res;
}

