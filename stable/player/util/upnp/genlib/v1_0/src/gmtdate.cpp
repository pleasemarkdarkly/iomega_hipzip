///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// $Revision: 1.4 $
// $Date: 2000/10/06 16:37:57 $

// gmtdate.cc

#include <util/upnp/api/config.h>


#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifndef _WIN32
//#include <values.h>
#else
//#include "../../inc/interface.h"
#include <util/upnp/api/interface.h>
#endif

#include <util/upnp/genlib/gmtdate.h>
#include <util/upnp/genlib/miscexceptions.h>

#include <util/upnp/genlib/mystring.h>

#include <util/debug/debug.h>

static char g_Days[7][4] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
static char* g_Invalid = "INV";

static char g_Months[12][4] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
      "Sep", "Oct", "Nov", "Dec" };

struct NameInfo
{
    char* name;
    int length;
};
    
#define NUM_MONTHS 12
static NameInfo MonthTable[NUM_MONTHS] =
{
    { "January",    7 },
    { "February",   8 },
    { "March",      5 },

    { "April",      5 },
    { "May",        3 },
    { "June",       4 },

    { "July",       4 },
    { "August",     6 },
    { "September",  9 },

    { "October",    7 },
    { "November",   8 },
    { "December",   8 },
};

#define NUM_DAYSPERWEEK 7
static NameInfo DayOfWeekTable[NUM_DAYSPERWEEK] =
{
    { "Sunday",     6 },
    { "Monday",     6 },
    { "Tuesday",    7 },
    { "Wednesday",  9 },
    { "Thursday",   8 },
    { "Friday",     6 },
    { "Saturday",   8 },
};

static int SearchNameInfo( IN const char* name,
    IN NameInfo* nameTable, IN int tableLen,
    OUT int* charsRead, OUT int* fullLengthMatch )
{
	int i;
	NameInfo *info = NULL;
	bool fullMatch = false;
	int len = 0;
	int code = -1;	// no match

	assert( nameTable != NULL );
	assert( tableLen > 0 );

	if ( name == NULL )
	{
		return -1;
	}

	for ( i = 0; i < tableLen; i++ )
	{
		info = &nameTable[i];

		// try full match
		if ( strncasecmp(name, info->name, info->length) == 0 )
		{
			code = 1;	// 1: full match
			len = info->length;
			fullMatch = true;
			break;
		}

		if ( strncasecmp(name, info->name, 3) == 0 )
		{
			code = 2;	// 2: partial match
			len = 3;
			fullMatch = false;
			break;
		}
	}

	if ( code > 0 )
	{
		if ( fullLengthMatch != NULL )
		{
			*fullLengthMatch = (fullMatch == true);
		}
        
		if ( charsRead != NULL )
		{
			*charsRead = len;
		}
	}

	assert(i >= 0 && i <= tableLen);

	return i;
}

// input: monthStr:  3-letter or full month
// returns month=0..11 or -1 on failure
// output:
//   charsRead - num chars that match the month
//   fullNameMatch - full name match(1) or 3-letter match(0)
//
int ParseMonth( IN const char* monthStr,
    OUT int* charsRead, OUT int* fullNameMatch )
{
    return SearchNameInfo( monthStr, MonthTable, NUM_MONTHS,
        charsRead, fullNameMatch );
}

// input: dayOfWeek:  3-letter or full day of week ("mon" etc)
// returns dayOfWeek=0..6 or -1 on failure
// output:
//   charsRead - num chars that match the month
//   fullNameMatch - full name match(1) or 3-letter match(0)
//
int ParseDayOfWeek( IN const char* dayOfWeek,
    OUT int* charsRead, OUT int* fullNameMatch )
{
    return SearchNameInfo( dayOfWeek, DayOfWeekTable,
        NUM_DAYSPERWEEK, charsRead, fullNameMatch );
}

const char* GetDayOfWeekStr( int day )
{
    if ( day >= 0 && day <= 6 )
    {
        return g_Days[ day ];
    }
    else
    {
        return g_Invalid;
    }
}

const char* GetMonthStr( unsigned month )
{
    if ( month >= 0 && month <= 11 )
    {
        return g_Months[ month ];
    }
    else
    {
        return g_Invalid;
    }
}


// converts date to string format: RFC 1123 format:
// Sun, 06 Nov 1994 08:49:37 GMT
// String returned must be freed using free() function
// returns NULL if date is NULL
char* DateToString( const struct tm* date )
{
    if ( date == NULL )
        return NULL;
        
    // max year supported = 2^32 = 4294967295 = 10 digits
    const int DATESTRLEN = 35 + 1;
    char *s;

    // TODO: add exception handling.
    s = (char *)malloc( DATESTRLEN );
    if ( s == NULL )
    {
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//		throw OutOfMemoryException( "DateToString()" );
		return 0;
    }
    
    sprintf( s, "%s, %02d %s %d %02d:%02d:%02d GMT",
        GetDayOfWeekStr(date->tm_wday), date->tm_mday,
        GetMonthStr(date->tm_mon), date->tm_year + 1900,
        date->tm_hour, date->tm_min, date->tm_sec );

    return s;   
}

// returns -1 if invalid num, or -ve num
static int ReadStrNum( IN const char* str, OUT int* numCharsRead )
{
    int val;
    char *endptr;
    
    val = (int)strtol( str, &endptr, 10 );
    
//    if ( val < 0 || val == MAXINT )
    if ( val < 0 || val == 0xFFFFFFFF )	// - ecm
    {
        return -1;
    }
    
    *numCharsRead = endptr - str;
    if ( *numCharsRead == 0 )
    {
        return -1;
    }
    
    return val;
}

enum
{
    DATIME_SPACE,
    DATIME_COMMA,
    DATIME_DASH,
    DATIME_WDAY,
    DATIME_FULLWDAY,
    DATIME_DAY,
    DATIME_MONTH,
    DATIME_YEAR,
    DATIME_YEAR2DIGIT,
    DATIME_TIME,
    DATIME_GMT
};


// parse: DIGIT DIGIT
// returns -1 on error, 0..99 on success
static int ParseTwoDigits( const char* s )
{
    char numStr[3];
    char* endptr;
    
    for ( int i = 0; i < 2; i++ )
    {
        if ( !isdigit(numStr[i] = s[i]) )
        {
            return -1;
        }
    }
    numStr[2] = 0;  
    
    return strtol( s, &endptr, 10 );
}

// parses time in fmt hh:mm:ss, military fmt
// returns 0 on success; -1 on error
int ParseTime( const char* s, int* hour, int* minute, int* second )
{
    int h, m, sec;

    // hour 
    h = ParseTwoDigits( s );
    if ( !(h >= 0 && h <= 23) )
    {
        return -1;
    }
    if ( s[2] != ':' )
    {
        return -1;
    }

    // min
    m = ParseTwoDigits( s + 3 );
    if ( !(m >= 0 && m <= 59) )
    {
        return -1;
    }
    if ( s[5] != ':' )
    {
        return -1;
    }
    
    // sec
    sec = ParseTwoDigits( s + 6 );
    if ( !(sec >= 0 && sec <= 60) )
    {
        return -1;
    }
    
    *hour = h;
    *minute = m;
    *second = sec;
    
    return 0;
}   

static int ParseDatePattern( char* pattern, int length,
    const char *s,
    struct tm* dateTime, int* numCharsParsed )
{
    int i;
    int charsParsed;
    int fullNameMatch;
    const char* saveS = s;
    int spaceCount;
    
    // skip white space
    while ( *s == ' ' || *s == '\t' )
    {
        s++;
    }

    for ( i = 0; i < length; i++ )
    {
        switch (pattern[i])
        {
            case DATIME_SPACE:
            {
                // multiple spaces
                spaceCount = 0;
                while ( *s == ' ' )
                {
                    s++;
                    spaceCount++;
                }
                if ( spaceCount == 0 )
                {
                    return -1;
                }
                break;
            }
                
            case DATIME_COMMA:
            {
                if ( *s++ != ',' )
                {
                    return -1;
                }
                break;
            }
                
            case DATIME_DASH:
            {
                if ( *s++ != '-' )
                {
                    return -1;
                }
                break;
            }
                
            case DATIME_WDAY:
            case DATIME_FULLWDAY:
            {
                dateTime->tm_wday = ParseDayOfWeek( s, &charsParsed,
                    &fullNameMatch );
                if ( (dateTime->tm_wday == -1) ||
                     (pattern[i] == DATIME_WDAY && fullNameMatch) ||
                     (pattern[i] == DATIME_FULLWDAY && !fullNameMatch)
                   )
                {
                    return -1;
                }
                    
                s += charsParsed;   // point to next char
                break;
            }
                
            case DATIME_DAY:
            {
                int mday;
                mday = ReadStrNum( s, &charsParsed );
                if ( mday <= 0 || mday > 31 )
                {
                    return -1;
                }
                dateTime->tm_mday = mday;
                s += charsParsed;
                break;
            }
                
            case DATIME_MONTH:
            {
                int mon;

                mon = ParseMonth( s, &charsParsed, &fullNameMatch );
                if ( mon < 0 || (fullNameMatch && mon != 4) )
                {
                    return -1;
                }
                dateTime->tm_mon = mon;
                s += charsParsed;
                break;
            }
                
            case DATIME_YEAR:   
            case DATIME_YEAR2DIGIT:
            {
                int year;
                year = ReadStrNum( s, &charsParsed );
                if ( pattern[i] == DATIME_YEAR )
                {
                    year -= 1900;
                    if ( year < 0 )
                        return -1;
                }
                else if ( year < 0 || year > 99 )
                {
                    // only 2-digit year allowed
                    return -1;
                }
                
                dateTime->tm_year = year;
                s += charsParsed;
                break;
            }
                
            case DATIME_TIME:
            {
                if ( ParseTime(s, &dateTime->tm_hour, &dateTime->tm_min,
                    &dateTime->tm_sec) == -1 )
                {
                    return -1;
                }
                s += 8;     // 8 chars for hh:mm:ss
                break;
            }

            case DATIME_GMT:
            {
                if ( *s++ != 'G' )
                    return -1;
                    
                if ( *s++ != 'M' )
                    return -1;
                    
                if ( *s++ != 'T' )
                    return -1;

                break;
            }
        }
    }
    
    if ( numCharsParsed != NULL )
    {
        *numCharsParsed = s - saveS;
    }
    
    return 0;   // success
}


int ParseRFC850DateTime( IN const char* str,
    OUT struct tm* dateTime, OUT int* numCharsParsed )
{
    static char pattern[] =
        { DATIME_FULLWDAY, DATIME_COMMA, DATIME_SPACE,
          DATIME_DAY, DATIME_DASH,
          DATIME_MONTH, DATIME_DASH,
          DATIME_YEAR2DIGIT, DATIME_SPACE,
          DATIME_TIME, DATIME_SPACE,
          DATIME_GMT
        };
    
    assert( dateTime != NULL );
    if ( str == NULL )
    {
        return -1;
    }
    
    return ParseDatePattern( pattern, sizeof(pattern), str,
        dateTime, numCharsParsed );
}

int ParseRFC1123DateTime( IN const char* str,
    OUT struct tm* dateTime, OUT int* numCharsParsed )
{
    static char pattern[] =
        { DATIME_WDAY, DATIME_COMMA, DATIME_SPACE,
          DATIME_DAY, DATIME_SPACE,
          DATIME_MONTH, DATIME_SPACE,
          DATIME_YEAR, DATIME_SPACE,
          DATIME_TIME, DATIME_SPACE,
          DATIME_GMT };
        
    assert( dateTime != NULL );
    if ( str == NULL )
    {
        return -1;
    }
    
    return ParseDatePattern( pattern, sizeof(pattern), str,
        dateTime, numCharsParsed );
}

int ParseAsctimeFmt( IN const char* str,
    OUT struct tm* dateTime, OUT int* numCharsParsed )
{
    static char pattern[] =
        { DATIME_WDAY, DATIME_SPACE,
          DATIME_MONTH, DATIME_SPACE,
          DATIME_DAY, DATIME_SPACE,
          DATIME_TIME, DATIME_SPACE,
          DATIME_YEAR
        };
        
    assert( dateTime != NULL );
    if ( str == NULL )
    {
        return -1;
    }
    
    return ParseDatePattern( pattern, sizeof(pattern), str,
        dateTime, numCharsParsed );
}

int ParseDateTime( IN const char* str, OUT struct tm* dateTime,
    int *numCharsParsed )
{
    assert( dateTime != NULL );
    
    if ( str == NULL )
    {
        return -1;
    }

    const char* s = str;    
    int charsParsed;
    int fullNameMatch;
    int wday;
    
    // get week day
    wday = ParseDayOfWeek( s, &charsParsed, &fullNameMatch );
    if ( wday == -1 )
    {
        return -1;
    }

    s += charsParsed;   // point to next char

    if ( fullNameMatch )
    {
        // 850 fmt
        return ParseRFC850DateTime( str, dateTime, numCharsParsed );
    }   
    else
    {
        if ( *s == ',' )
        {
            // 1123 format
            return ParseRFC1123DateTime( str, dateTime, numCharsParsed );
        }
        else if ( *s == ' ' )
        {
            // asctime() fmt
            return ParseAsctimeFmt( str, dateTime, numCharsParsed );
        }
        else
        {
            return -1;
        }
    }
}

#endif
#endif
