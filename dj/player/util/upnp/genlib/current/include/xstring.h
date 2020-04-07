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
// * Neither name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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

// $Revision: 1.9 $
// $Date: 2000/09/06 19:57:29 $

#ifndef XSTRING_H
#define XSTRING_H

#include <util/upnp/genlib/util.h>

class xstring
{
public:
	// throws OutOfMemoryException
    xstring();

	// throws OutOfMemoryException
    xstring( const xstring& s );

	// throws OutOfMemoryException
    xstring( const char* s );

	// throws OutOfMemoryException
    xstring( char c );

    virtual ~xstring();
    
    const char* c_str() const;
    int length() const;
    int capacity() const;
    
    // sizeIncrement - specifies the size of memory allocated
    //   when expanding string. Higher value increases speed
    //   but increases memory consumed and vice versa
    unsigned getSizeIncrement();
    void setSizeIncrement( unsigned newIncrement );
    
	// throws OutOfMemoryException
    void operator = ( const xstring& s );

	// throws OutOfMemoryException
    void operator = ( const char* s );

	// throws OutOfMemoryException
    void operator = ( char c );

	// throws OutOfBoundsException
    char& operator [] ( int index );

	// throws OutOfBoundsException
    char  operator [] ( int index ) const;

	// throws OutOfMemoryException
    void operator += ( const xstring& s );

	// throws OutOfMemoryException
    void operator += ( const char* s );

	// throws OutOfMemoryException
    void operator += ( char c );

	// throws OutOfMemoryException
    xstring operator + ( const xstring& s ) const;

    bool operator == ( const xstring& s ) const;
    bool operator != ( const xstring& s ) const;
    bool operator >  ( const xstring& s ) const;
    bool operator <  ( const xstring& s ) const;
    bool operator >= ( const xstring& s ) const;
    bool operator <= ( const xstring& s ) const;

    bool operator == ( const char* s ) const;
    bool operator != ( const char* s ) const;
    bool operator >  ( const char* s ) const;
    bool operator <  ( const char* s ) const;
    bool operator >= ( const char* s ) const;
    bool operator <= ( const char* s ) const;

    bool operator == ( char c ) const;
    bool operator != ( char c ) const;
    bool operator >  ( char c ) const;
    bool operator <  ( char c ) const;
    bool operator >= ( char c ) const;
    bool operator <= ( char c ) const;


    int compareNoCase( const char* s    ) const;
    int compareNoCase( const xstring& s ) const;
    
	// throws OutOfBoundsException
    int find( char c, int startIndex = 0 ) const;

	// throws OutOfBoundsException
    int findStr( xstring& s, int startIndex = 0 ) const;

    // append only up to a max character count
	//
	// throws OutOfMemoryException
    void appendLimited( const char* s, int maxLen );

	// copy only up to a max character count
	//
	// throws OutOfMemoryException
    void copyLimited( const char* s, int maxLen );
    
	// throws OutOfBoundsException, OutOfMemoryException
    xstring substring( int startIndex, int numChars ) const;
    
	// throws OutOfBoundsException, OutOfMemoryException
    void deleteSubstring( int startIndex, int numChars );

	// throws OutOfMemoryException, OutOfBoundsException
    void insert( const char* s, int index );

	// throws OutOfMemoryException, OutOfBoundsException
    void insert( const xstring& s, int index );
    
    void toUppercase();
    void toLowercase();
    
    // returns the string value; the value is 'detached' from
    //  xstring and the user is responsible for freeing the
    //  memory using the free() function
	//
	// throws OutOfMemoryException
    char *detach();
    
    // replaces current str with s
    // s - should have been allocated with malloc()
    void attach( char* s );
    
public:
    enum { DEFAULT_SIZE_INCREMENT = 5 };

private:
    void init();
    
	// throws OutOfMemoryException
    void doCopy( const char* s, int slength );

	// throws OutOfMemoryException
    void doAdd( const char* s, int slength );

	// throws OutOfMemoryException, OutOfBoundsException
    void doInsert( const char* s, int slength, int index );
    
	// throws OutOfMemoryException
    void trimBuffer( bool saveExisting );

    // throws OutOfMemoryException
    void setBufSize( int newLen );

    // throws OutOfMemoryException
    void setBufferSizeAssign( int newSize );
    
private:
    char* buf;
    int len;        // length of string (excluding null)
    int bufLen;     // length of buffer (excluding null)
    unsigned sizeIncrement;
};


// util funcs

// throws OutOfMemoryException
void IntToStr( IN int num, OUT xstring& s, int base=10 );

#endif /* XSTRING_H */

