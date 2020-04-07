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

// $Revision: 1.2 $
// $Date: 2000/09/28 23:42:11 $

#include <util/upnp/api/config.h>

#if EXCLUDE_MINISERVER == 0
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/memreader.h>
#include <util/upnp/genlib/miscexceptions.h>

#include <util/upnp/genlib/noexceptions.h>

MemReader::MemReader( const char *str )
{
    assert( str != NULL );
    
    buf = (char *)str;
    index = 0;
    len = strlen( str );    
}

MemReader::MemReader( const void* binaryData, int length )
{
    assert( binaryData != NULL );
    assert( length >= 0 );
    
    buf = (char *)binaryData;
    index = 0;
    len = length;
}

EDERRCODE 
MemReader::getChar(char* pch)
{
    if ( index >= len )
    {
//		throw OutOfBoundsException( "MemReader::getChar()" );
		DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION("MemReader::getChar()");
		return EDERR_OUT_OF_BOUNDS_EXCEPTION;
    }

    if (pch)
		*pch = buf[index];
    index++;
	return ED_OK;
}

EDERRCODE MemReader::pushBack()
{
    if ( index <= 0 )
    {
//		throw OutOfBoundsException( "MemReader::pushBack()" );
		DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION("MemReader::pushBack()");
		return EDERR_OUT_OF_BOUNDS_EXCEPTION;
    }
    
    index--;

	return ED_OK;
}

EDERRCODE
MemReader::read( INOUT void* databuf, IN int bufferSize, OUT int* numRead )
{
    int copyLen;
    int dataLeftLen;

    assert( databuf != NULL );
    assert( bufferSize > 0 );
        
    // how much data left in buffer
    dataLeftLen = len - index;
    
    // pick smaller value
    copyLen = MinVal( bufferSize, dataLeftLen );
    
    // copy data to caller's buffer
    memcpy( databuf, &buf[index], copyLen );

    // point to next char in buf
    index += copyLen;

    if (numRead)
		*numRead = copyLen;

	return ED_OK;
}

bool MemReader::endOfStream()
{
    return index >= len;
}

#endif
