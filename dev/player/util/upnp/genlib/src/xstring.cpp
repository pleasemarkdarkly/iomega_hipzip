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

// $Revision: 1.3 $
// $Date: 2000/09/06 19:53:40 $

// xstring.cpp

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <util/upnp/genlib/xstring.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/utilall.h>

#include <util/upnp/genlib/mystring.h>

#include <cyg/infra/diag.h>

#include <util/upnp/genlib/noexceptions.h>

xstring::xstring()
{
    init();
    doCopy( "", 0 );
}

xstring::xstring( const xstring& s )
{
    init();
    doCopy( s.buf, s.len );
}

xstring::xstring( const char* s )
{
    init();
    doCopy( s, strlen(s) );
}

xstring::xstring( char c )
{
    init();
    
    char s[2];
    
    s[0] = c;
    s[1] = 0;
    doCopy( s, 1 ); 
}

xstring::~xstring()
{
    free( buf );
}

void xstring::init()
{
    sizeIncrement = DEFAULT_SIZE_INCREMENT;
    buf = NULL;
    len = 0;
    bufLen = 0;
}

unsigned xstring::getSizeIncrement()
{
    return sizeIncrement;
}

void xstring::setSizeIncrement( unsigned newIncrement )
{
    sizeIncrement = newIncrement;
}

const char* xstring::c_str() const
{
    return buf;
}

int xstring::length() const
{
    return len;
}

int xstring::capacity() const
{
    return bufLen;
}

void xstring::operator = ( const xstring& s )
{
    doCopy( s.buf, s.len );
}

void xstring::operator = ( const char* s )
{
    doCopy( s, strlen(s) );
}

void xstring::operator = ( char c )
{
    char s[2];
    
    s[0] = c;
    s[1] = 0;
    doCopy( s, 1 ); 
}

void xstring::operator += ( const xstring& s )
{
    if ( s.buf == buf )
    {
        xstring temp(s);
        
        doAdd( temp.buf, temp.len );
    }
    else
    {
        doAdd( s.buf, s.len );
    }
}

void xstring::operator += ( const char* s )
{
    doAdd( s, strlen(s) );
}

void xstring::operator += ( char c )
{
    char s[2];
    
    s[0] = c;
    s[1] = 0;
    doAdd( s, 1 );  
}

xstring xstring::operator + ( const xstring& s ) const
{
    xstring temp( *this );
    
    temp += s;
    
    return temp;
}

char& xstring::operator [] ( int index )
{
    if ( index < 0 || index >= len )
    {
        char s[100];
        sprintf( s, "xstring::[] array index %d", index );
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( s );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( s );
		return buf[0];
#endif	 // FORCE_NO_EXCEPTIONS
    }
        
    return buf[ index ];
}

char xstring::operator [] ( int index ) const
{
    if ( index < 0 || index >= len )
    {
        char s[100];
        sprintf( s, "xstring::[] array index %d", index );
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( s );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( s );
		return 0;
#endif	 // FORCE_NO_EXCEPTIONS
    }
        
    return buf[ index ];
}

bool xstring::operator == ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) == 0;
}

bool xstring::operator != ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) != 0;
}

bool xstring::operator > ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) > 0;
}

bool xstring::operator < ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) < 0;
}

bool xstring::operator >= ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) >= 0;
}

bool xstring::operator <= ( const xstring& s ) const
{
    return strcmp( buf, s.buf ) <= 0;
}

bool xstring::operator == ( const char* s ) const
{
    return strcmp( buf, s ) == 0;
}

bool xstring::operator != ( const char* s ) const
{
    return strcmp( buf, s ) != 0;
}

bool xstring::operator >  ( const char* s ) const
{
    return strcmp( buf, s ) > 0;
}

bool xstring::operator <  ( const char* s ) const
{
    return strcmp( buf, s ) < 0;
}

bool xstring::operator >= ( const char* s ) const
{
    return strcmp( buf, s ) >= 0;
}

bool xstring::operator <= ( const char* s ) const
{
    return strcmp( buf, s ) <= 0;
}

bool xstring::operator == ( char c ) const
{
    return (len == 1 && buf[0] == c);
}

// (!=) == !(==)
bool xstring::operator != ( char c ) const
{
    return !(len == 1 && buf[0] == c);
}

bool xstring::operator >  ( char c ) const
{
    return (len >= 1 && buf[0] > c);
}

// (<) == ! (>=)
bool xstring::operator <  ( char c ) const
{
    return !(len >= 1 && buf[0] >= c);
}

bool xstring::operator >= ( char c ) const
{
    return (len >= 1 && buf[0] >= c);
}

// (<=) == !(>)
bool xstring::operator <= ( char c ) const
{
    return !(len >= 1 && buf[0] > c);
}

int xstring::compareNoCase( const char* s ) const
{
    return strcasecmp( buf, s );
}

int xstring::compareNoCase( const xstring& s ) const
{
    return strcasecmp( buf, s.buf );
}

int xstring::find( char c, int startIndex ) const
{
    if ( startIndex < 0 || startIndex > len )
	{
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xstring::find(c)" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::find(c)" );
		return -1;
#endif	 // FORCE_NO_EXCEPTIONS
	}
    
    char* s = strchr( &buf[startIndex], c );
    
    if ( s == NULL )
        return -1;
        
    int index;
    
    index = s - buf;
    
    return index;
}

int xstring::findStr( xstring& s, int startIndex ) const
{
    char* str;
    int index;

    if ( startIndex < 0 || startIndex > len )
	{
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xstring::find(xstring)" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::find(xstring)" );
		return -1;
#endif	 // FORCE_NO_EXCEPTIONS
	}
        
    str = strstr( &buf[ startIndex ], s.buf );
    if ( str == NULL )
        return -1;
    
    index = buf - str;
    return index;
}

void xstring::appendLimited( const char* s, int maxLen )
{
    if ( maxLen <= 0 )
        return;
    
    int lenVal;
    int slen = strlen(s);

    if ( slen < maxLen )
        lenVal = slen;
    else
        lenVal = maxLen;
    
    doAdd( s, lenVal );
}

void xstring::copyLimited( const char* s, int maxLen )
{
    if ( maxLen < 0 )
        return;
    
    int lenVal;
    int slen = strlen(s);

    if ( slen < maxLen )
        lenVal = slen;
    else
        lenVal = maxLen;
    
    doCopy( s, lenVal );
}


xstring xstring::substring( int startIndex, int numChars ) const
{
    xstring s;
    
    if ( startIndex < 0 || startIndex >= len || numChars < 0 )
    {
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xstring::substring" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::substring" );
		return s;
#endif	 // FORCE_NO_EXCEPTIONS
    }
    
    int copyLen = numChars;
    if ( startIndex + numChars > len )
    {
        copyLen = startIndex + numChars;
    }
    
    s.copyLimited( &buf[startIndex], numChars );
    
    return s;
}

void xstring::insert( const char* s, int index )
{
    if ( s != NULL )
    {
        doInsert( s, strlen(s), index );
    }
}

void xstring::insert( const xstring& s, int index )
{
    doInsert( s.buf, s.len, index );
}

void xstring::deleteSubstring( int startIndex, int numChars )
{
    if ( numChars == 0 )
    {
        return;
    }
    
    if ( startIndex < 0 || startIndex >= len || numChars < 0 )
    {
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xstring::deleteSubstring()" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::deleteSubstring()" );
		return;
#endif	 // FORCE_NO_EXCEPTIONS
    }
    
    // length of string after deleted substring
    int endLen; 
    int endIndex;

    if ( startIndex + numChars > len )
    {
        numChars = len - startIndex;
    }

    endLen = len - startIndex - numChars;
    endIndex = startIndex + numChars;

    // copy '\0' too    
    memcpy( &buf[startIndex], &buf[endIndex], endLen+1 );
    
    len -= numChars;
    
    trimBuffer( true );
}

void xstring::toUppercase()
{
    for ( int i = 0; i < len; i++ )
    {
        buf[i] = toupper( buf[i] );
    }   
}

void xstring::toLowercase()
{
    for ( int i = 0; i < len; i++ )
    {
        buf[i] = tolower( buf[i] );
    }
}

char *xstring::detach()
{
    char *s;
    
    s = buf;    // get detached str
    
    // buf has empty string
    buf = NULL;
    len = 0;
    bufLen = 0;
    doCopy( "", 0 );
    
    return s;
}

void xstring::attach( char* s )
{
    // manually free mem
    free( buf );
    if ( s == NULL )
    {
        bufLen = 0;
        len = 0;
        doCopy( "", 0 );
    }
    else
    {
        bufLen = len = strlen(s);
        buf = s;
    }
}

void xstring::doInsert( const char* s, int slength, int index )
{
    if ( index < 0 || index > len )
    {
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfBoundsException( "xstring::diInsert()" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::diInsert()" );
		return;
#endif	 // FORCE_NO_EXCEPTIONS
    }
    
    if ( slength <= 0 )
        return;
    
    int newLen;
    
    newLen = len + slength;
    
    setBufSize( newLen );
    
    // move right-side string to make space for inserted data
    memmove( &buf[index+slength], &buf[index], len - index + 1 );
    
    // copy inserted data
    memcpy( &buf[index], s, slength );

    len = newLen;
}

// frees excess memory
void xstring::trimBuffer( bool saveExisting )
{
    if ( bufLen - len <= (int)sizeIncrement )
    {
        return;
    }
    
    bufLen = len + sizeIncrement;
    
    if ( saveExisting )
    {
        buf = (char *)realloc( buf, bufLen + 1 );
    }
    else
    {
        free( buf );
        buf = (char *)malloc( bufLen + 1 );
    }

	if ( buf == NULL )
	{
#ifndef FORCE_NO_EXCEPTIONS
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw OutOfMemoryException( "xstring::trimBuffer()" );
#else	 // FORCE_NO_EXCEPTIONS
        DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::trimBuffer()" );
#endif	 // FORCE_NO_EXCEPTIONS
	}
}

// only used to expand buffer to at least newLen; not to shrink
// expands buf to 'newLen' or more if bufsize is less than
//  newLen
// if 'saveExisting' is true, current buf data is copied to
//   new string; else it is discarded
void xstring::setBufSize( int newLen )
{
    // have enuf memory
    if ( bufLen >= newLen )
    {
        return;
    }

    char *temp;
    int allocLen;

    allocLen = MaxVal( newLen, (int)(bufLen + sizeIncrement) );
    temp = (char *) realloc( buf, allocLen + 1 );
    if ( temp == NULL )
    {
        // try smaller size
        if ( newLen < (int)(bufLen + sizeIncrement) )
        {
            temp = (char *) realloc( buf, newLen + 1 );
        }

        if ( temp == NULL )
        {
#ifndef FORCE_NO_EXCEPTIONS
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
			throw OutOfMemoryException("xstring::setBufSize()");
#else	 // FORCE_NO_EXCEPTIONS
			DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION("xstring::setBufSize()");
			return;
#endif	 // FORCE_NO_EXCEPTIONS
        }
    }

    bufLen = allocLen;
    buf = temp;
}

// sets buffer size for assignment; throws away old data
void xstring::setBufferSizeAssign( int newSize )
{
    assert( newSize >= 0 );

    // skip if enuf mem
    int diff;
    diff = bufLen - newSize;
    if ( buf != NULL && diff >= 0 && diff < (int)sizeIncrement )
    {
        return;
    }

    // free old
    if ( buf != NULL )
    {
        free( buf );
    }
    bufLen = 0;

    // alloc new buf
    int allocLen;

    allocLen = MaxVal( newSize, (int)sizeIncrement );
    buf = (char *) malloc( allocLen + 1 );
    if ( buf == NULL )
    {
        // try smaller size
        if ( newSize < (int)sizeIncrement )
        {
            allocLen = newSize;
            buf = (char *)malloc( newSize + 1 );
        }

        if ( buf == NULL )
        {
#ifndef FORCE_NO_EXCEPTIONS
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
			throw OutOfMemoryException( "xstring::setBufferSizeAssign()" );
#else	 // FORCE_NO_EXCEPTIONS
			DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION( "xstring::setBufferSizeAssign()" );
			return;
#endif	 // FORCE_NO_EXCEPTIONS
        }
    }
    bufLen = allocLen;
}


void xstring::doCopy( const char* s, int slength )
{
    if ( buf == s )
        return;

    setBufferSizeAssign( slength );
    memcpy( buf, s, slength );
    buf[slength] = 0;
    len = slength;
}

void xstring::doAdd( const char* s, int slength )
{
    setBufSize( len + slength );
    memcpy( buf + len, s, slength );
    len += slength;
    buf[len] = 0;
}


// util funcs
////////////////

void IntToStr( IN int num, OUT xstring& s, int base )
{
    char buf[50];
    char *pattern;
    
    if ( base == 16 )
    {
        pattern = "%x";
    }
    else
    {
        pattern = "%d";
    }
    
    sprintf( buf, pattern, num );
    s = buf;
}
