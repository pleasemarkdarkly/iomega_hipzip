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

// $Revision: 1.6 $
// $Date: 2000/10/06 16:37:57 $

#include <util/upnp/api/config.h>


#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <util/upnp/genlib/netreader.h>
#include <util/upnp/genlib/utilall.h>
#include <util/upnp/genlib/netexception.h>
#ifndef _WIN32
//#include <sys/socket.h>
extern "C"
{
#include <network.h>	// - ecm
}
#include <unistd.h>
#else
#include <io.h>
#include "../../../win32/win32.h"
#endif

#include <util/upnp/genlib/noexceptions.h>

NetReader::NetReader( int socketfd )
{
    sockfd = socketfd;
    datalen = 0;
    offset = 0;
    buf_eof = false;
}
    
NetReader::~NetReader()
{
}
    
EDERRCODE
NetReader::getChar(char* pch)
{
	if ( !bufferHasData() )
	{
		ED_RETURN_EXCEPTION(refillBuffer());
    
		if ( !bufferHasData() )
		{
			if (pch)
				*pch = 0;
			return ED_OK;
		}
	}

	// read next char	
	if (pch)
		*pch = data[offset];
	++offset;
	return ED_OK;
}

    
// throws OutOfBoundsException
EDERRCODE NetReader::pushBack()
{
	if ( offset == 0 )
	{
		// can't move further back
//		throw OutOfBoundsException( "NetReader::pushback()" );
		DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION("NetReader::pushback()");
		return EDERR_OUT_OF_BOUNDS_EXCEPTION;
	}

	offset--;

	return ED_OK;
}

EDERRCODE
NetReader::read( void* buffer, int bufferSize, OUT int* numRead )
{
    int copyLen;
    int dataLeft;
	if (numRead)
		*numRead = 0;
    
    assert( buffer != NULL );
    
    // no more data left
    if ( buf_eof )
    {
        return ED_OK;
    }

    if ( !bufferHasData() )
    {
        ED_RETURN_EXCEPTION(refillBuffer());
        if ( !bufferHasData() )
        {
            return ED_OK;
        }
    }	
    
    dataLeft = datalen - offset;
    
    // copyLen = min( dataLeft, bufferSize )
    copyLen = bufferSize < dataLeft ? bufferSize : dataLeft;

    memcpy( buffer, &data[offset], copyLen );
    
    offset += copyLen;
    
	if (numRead)
		*numRead = copyLen;

	return ED_OK;
}

// throws NetException
EDERRCODE NetReader::refillBuffer()
{
	int numSaveChars;
	int numRead;

	// already reached EOF
	if ( buf_eof )
	{
//		throw EOFException( "NetBuffer2::refillbuffer()" );
		DEBUG_THROW_EOF_EXCEPTION("NetBuffer2::refillbuffer()");
		return EDERR_EOF_EXCEPTION;
	}

	// save MAX_PUSHBACK chars at end, to start of data buffer

	// numSaveChars = min( MAX_PUSHBACK, datalen )
	numSaveChars = MAX_PUSHBACK < datalen ? MAX_PUSHBACK : datalen;

	// save
	memmove( &data[0], &data[datalen-numSaveChars], numSaveChars );

	offset = numSaveChars;	// point after saved chars

	// read rest from network
	numRead = recv( sockfd, &data[offset], BUFSIZE, 0 );

	if ( numRead < 0 )
	{
//		NetException e( "NetReader::refillBuffer()" );
//		e.setErrorCode( numRead );
//		throw e;
		DEBUG_THROW_NET_EXCEPTION("NetBuffer2::refillbuffer()", numRead);
		return EDERR_NET_EXCEPTION;
	}

	if ( numRead == 0 )
	{
		buf_eof = true;
	}

	datalen = numSaveChars + numRead;	

	return ED_OK;
}

bool NetReader::bufferHasData() const
{
    return offset < datalen;
}

#endif
#endif
