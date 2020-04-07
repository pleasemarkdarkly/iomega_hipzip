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

// $Revision: 1.8 $
// $Date: 2000/10/06 16:37:57 $

// readwrite.cpp

#include <util/upnp/api/config.h>

#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_WEB_SERVER == 0

#include <stdio.h>
#include <util/upnp/genlib/netreader.h>
#include <util/upnp/genlib/netexception.h>
#include <util/upnp/genlib/parseutil2.h>
#include <util/upnp/genlib/readwrite.h>
#include <util/upnp/genlib/statuscodes.h>

#include <util/debug/debug.h>

#include <unistd.h>
#include <netinet/in.h>
//#include <sys/socket.h>
extern "C"
{
#include <network.h>
#include <main/upnpservices/XMLDocs.h>
}

DEBUG_MODULE_S( READWRITE, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( READWRITE );

// return codes:
//   0: success
//  -1: std error; check errno
//  HTTP_E_OUT_OF_MEMORY
//  HTTP_E_BAD_MSG_FORMAT
//  HTTP_E_TIMEDOUT
int http_RecvMessage( IN int tcpsockfd, OUT HttpMessage& message,
    UpnpMethodType requestMethod, int timeoutSecs )
{
	int retCode = 0;

//	try
//	{
		NetReader reader( tcpsockfd );
		Tokenizer scanner( reader );
        
		if ( requestMethod == HTTP_UNKNOWN_METHOD )
		{
			// reading a request
			retCode = message.loadRequest( scanner, &reader );
//			message.loadRequest( scanner, &reader );
		}
		else
		{
			// read response
			retCode = message.loadResponse( scanner, &reader, requestMethod );
//			message.loadResponse( scanner, &reader, requestMethod );
		}
            
//	}

	return retCode;
}


// returns -1 on system error
//  HTTP_E_FILE_READ
static int SendFile( IN int tcpsockfd, IN const char* filename,
    int timeoutSecs )
{
#ifdef USE_FILESYSTEM
    FILE *fp = NULL;
    const int BUFSIZE = 2 * 1024;
    char buf[BUFSIZE];
    int numRead;
    int numWritten;
    int code = 0;

    try
    {
        fp = fopen( filename, "rb" );
        if ( fp == NULL )
        {
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
            throw -1;
        }

        while ( true )
        {
            // read data
            numRead = fread( buf, sizeof(char), BUFSIZE, fp );
            if ( numRead == 0 )
            {
/*	- ecm
                if ( ferror(fp) )
                {
                    throw HTTP_E_FILE_READ;
                }
*/
                break;
            }
        
            // write data
            numWritten = send( tcpsockfd, buf, numRead, 0 );
            if ( numWritten == -1 )
            {
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                throw -1;
            }
        }
    }
    catch ( int catchCode )
    {
        code = catchCode;
    }

    if ( fp != NULL )
    {
        fclose( fp );
    }

    return code;

#else

	int iFileLen = FakeGetFileLength(filename);
	if (iFileLen > 0)
	{
        DEBUG(READWRITE, DBGLEV_INFO, "Faking request for file %s\n", filename);
		int numWritten = send( tcpsockfd, (const void*)FakeGetFile(filename), iFileLen - 1, 0 );
		if (numWritten == -1)
			return -1;
		else
			return 0;
	}
	else
	{
        DEBUG(READWRITE, DBGLEV_INFO, "Couldn't fake request for file %s\n", filename);
		return -1;
	}



#endif

}


// return codes:
//   0: success
//  -1: std error; check errno
//  HTTP_E_OUT_OF_MEMORY
//  HTTP_E_TIMEDOUT
int http_SendMessage( IN int tcpsockfd, IN HttpMessage& message,
    int timeoutSecs )
{
	int retCode = 0;

	assert( tcpsockfd > 0 );

//	try
//	{
		xstring s;
		ssize_t numWritten;
		int status;
    
		// send headers
		message.startLineAndHeadersToString( s );
    
		numWritten = send( tcpsockfd, s.c_str(), s.length(), 0 );
		if ( numWritten == -1 )
		{
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw -1;
			DEBUG_THROW_NET_EXCEPTION("http_SendMessage", -1);
			retCode = -1;
			goto CatchError;
		}
        
		// send optional body
		{
		HttpEntity &entity = message.entity;
		HttpEntity::EntityType etype;
    
		etype = entity.getType();
		switch ( etype )
		{
			case HttpEntity::EMPTY:
				// nothing to send
				break;
            
			case HttpEntity::TEXT:
			case HttpEntity::TEXT_PTR:
			{
				// precond: content-length is in header and
				//   no transfer-encoding

				const void* entityData;
				entityData = entity.getEntity();
				if ( entityData != NULL )
				{
					numWritten = send( tcpsockfd, (const char *)entity.getEntity(),
						entity.getEntityLen(), 0 );
					if ( numWritten == -1 )
					{
						// TODO: add exception handling to caller
//						diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//						throw -1;
						DEBUG_THROW_NET_EXCEPTION("http_SendMessage", -1);
						retCode = -1;
						goto CatchError;
					}
				}
				break;
			}
            
			case HttpEntity::FILENAME:
				status = SendFile( tcpsockfd, entity.getFileName(),
					timeoutSecs );
				if ( status == -1 )
				{
					// TODO: add exception handling to caller
//					diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//					throw -1;
					DEBUG_THROW_NET_EXCEPTION("http_SendMessage", status);
					retCode = -1;
					goto CatchError;
				}
				break;
        
			default:
				DBG(
					UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
						"http_SendMessage(): unknown HttpEntity type %d", etype ); )
				break;  
		}
		}
//	}

CatchError:

	return retCode;
}

// on success, returns socket connect to server
// on failure, returns -1, check errno
int http_Connect( const char* resourceURL )
{
    UriType uri;
    int sockfd;
    int status;
    sockaddr_in address;
    
    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd == -1 )
        return -1;
        
    uri.setUri( resourceURL );
//    bzero( &address, sizeof(sockaddr_in) );
    memset( &address, 0, sizeof(sockaddr_in) );
    uri.getIPAddress( address );

    // dc- set a timeout on the connect socket
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
        
    status = connect( sockfd, (sockaddr*) &address, sizeof(sockaddr_in) );
    if ( status == -1 )
    {
        close( sockfd );
        return -1;
    }

    return sockfd;
}

// return codes
//   0: success
//  -1: std error; check errno
//  -2: out of memory
//  -3: timeout
int http_Download( IN const char* resourceURL, OUT HttpMessage& resource,
    IN int timeoutSecs )
{
	int retCode = 0;
	int connfd = -1;
	int status = 0;
	HttpMessage request;

//	try
//	{
//		try
//		{
			// build request
			request.isRequest = true;
    
			HttpRequestLine& requestLine = request.requestLine;
			requestLine.method = HTTP_GET;
			requestLine.uri.setUri( resourceURL );
			requestLine.majorVersion = 1;
			requestLine.minorVersion = 1;

			// connect
			connfd = http_Connect( resourceURL );
			if ( connfd == -1 )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -1;
				DEBUG_THROW_NET_EXCEPTION("http_Download", -1);
				retCode = -1;
				goto CatchError;
			}
        
			// send request
			status = http_SendMessage( connfd, request, timeoutSecs );
			if ( status < 0 )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw status;
				DEBUG_THROW_NET_EXCEPTION("http_Download", status);
				retCode = status;
				goto CatchError;
			}
            
			// read reply
			status = http_RecvMessage( connfd, resource,
				requestLine.method, timeoutSecs );
			if ( status < 0 )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw status;
				DEBUG_THROW_NET_EXCEPTION("http_Download", status);
				retCode = status;
				goto CatchError;
			}
            
			// free resources
			close( connfd );
			return 0;
//		}

CatchError:

	// free resources
	if ( connfd != -1 )
	{
		close( connfd );
	}

	switch (retCode)
	{
		case EDERR_OUT_OF_MEMORY:
		{
			retCode = HTTP_E_OUT_OF_MEMORY;
			break;
		}
#if 0
		catch ( TimeoutException& /* e */ )
		{
			retCode = HTTP_E_TIMEDOUT;
			break;
		}
#endif
		default:
			return retCode;
	};

	return retCode;
}

#endif
#endif

