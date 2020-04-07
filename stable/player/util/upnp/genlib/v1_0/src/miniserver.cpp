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

// $Revision: 1.7 $
// $Date: 2000/10/08 16:51:30 $

#include <util/upnp/api/config.h>


#if EXCLUDE_MINISERVER == 0
#ifndef _WIN32
//#include <arpa/inet.h>
//#include <netinet/in.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#endif
//#include <sys/socket.h>
extern "C"
{
#include <network.h>	// - ecm
#ifdef USE_PTHREADS
#include <cyg/posix/signal.h>
#endif
}
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <util/upnp/genlib/utilall.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/miniserver.h>
#include <util/upnp/genlib/scheduler.h>
#include <util/upnp/genlib/interrupts.h>
#include <util/upnp/api/upnp.h>
#include <util/upnp/api/config.h>

#include <util/upnp/genlib/noexceptions.h>

// read timeout
#define TIMEOUT_SECS 30

enum MiniServerState { MSERV_IDLE, MSERV_RUNNING, MSERV_STOPPING };

CREATE_NEW_EXCEPTION_TYPE( MiniServerReadException, BasicException, "MiniServerReadException" )

enum READ_EXCEPTION_CODE {
        RCODE_SUCCESS               =  0,
        RCODE_NETWORK_READ_ERROR    = -1,
        RCODE_MALFORMED_LINE        = -2,
        RCODE_LENGTH_NOT_SPECIFIED  = -3,
        RCODE_METHOD_NOT_ALLOWED    = -4,
        RCODE_INTERNAL_SERVER_ERROR = -5,
        RCODE_METHOD_NOT_IMPLEMENTED = -6,
        RCODE_TIMEDOUT              = -7,
        };
        
enum HTTP_COMMAND_TYPE { CMD_HTTP_GET,
        CMD_SOAP_POST, CMD_SOAP_MPOST,
        CMD_GENA_SUBSCRIBE, CMD_GENA_UNSUBSCRIBE, CMD_GENA_NOTIFY,
        CMD_HTTP_UNKNOWN,
        CMD_HTTP_MALFORMED };

// module vars

static MiniServerCallback gGetCallback = NULL;
static MiniServerCallback gSoapCallback = NULL;
static MiniServerCallback gGenaCallback = NULL;

static MiniServerState gMServState = MSERV_IDLE;
#ifdef USE_PTHREADS
static pthread_t gMServThread = 0;
#else
static cyg_handle_t gMServThread = 0;
#endif

//////////////

void SetHTTPGetCallback( MiniServerCallback callback )
{
    gGetCallback = callback;
}

MiniServerCallback GetHTTPGetCallback( void )
{
    return gGetCallback;
}

void SetSoapCallback( MiniServerCallback callback )
{
    gSoapCallback = callback;
}

MiniServerCallback GetSoapCallback( void )
{
    return gSoapCallback;
}

void SetGenaCallback( MiniServerCallback callback )
{
    gGenaCallback = callback;
}

MiniServerCallback GetGenaCallback( void )
{
    return gGenaCallback;
}


class NetReader1
{
public:
    NetReader1( int socketfd );
    virtual ~NetReader1();

#ifndef FORCE_NO_EXCEPTIONS	
	// throws MiniServerReadException.RCODE_TIMEDOUT
    int getChar( char& c );
	// throws MiniServerReadException.RCODE_TIMEDOUT
    int getLine( xstring& s, bool& newlineNotFound );
	// throws MiniServerReadException.RCODE_TIMEDOUT
    int readData( void* buf, size_t bufferLen );
#endif	// FORCE_NO_EXCEPTIONS	
	// throws MiniServerReadException.RCODE_TIMEDOUT
    EDERRCODE getCharNoEx( char& c, int* pStatus );
	// throws MiniServerReadException.RCODE_TIMEDOUT
    EDERRCODE getLineNoEx( xstring& s, bool& newlineNotFound, int* pNumRead );
	// throws MiniServerReadException.RCODE_TIMEDOUT
    EDERRCODE readDataNoEx( void* buf, size_t bufferLen, int* pNumRead );
    
    int getMaxBufSize() const
        { return maxbufsize; }
    
private:
    bool bufferHasData() const
    { return offset < buflen; }

#ifndef FORCE_NO_EXCEPTIONS	
	// throws MiniServerReadException.RCODE_TIMEDOUT
    ssize_t refillBuffer();
#endif	// FORCE_NO_EXCEPTIONS	
    EDERRCODE refillBufferNoEx(int *pNumRead);

private:
    enum { MAX_BUFFER_SIZE = 1024 * 2 };

private:
    int sockfd;
    char data[MAX_BUFFER_SIZE + 1]; // extra byte for null terminator
    int offset;
    int buflen;
    int maxbufsize;
};

NetReader1::NetReader1( int socketfd )
{
    sockfd = socketfd;
    offset = 0;
    maxbufsize = MAX_BUFFER_SIZE;
    buflen = 0;
    data[maxbufsize] = 0;
}

NetReader1::~NetReader1()
{

}

#ifndef FORCE_NO_EXCEPTIONS

int NetReader1::getChar( char& c )
{
    int status;
    
    if ( !bufferHasData() )
    {
        status = refillBuffer();
        
        if ( status <= 0 )
            return status;
            
        if ( !bufferHasData() )
            return 0;
    }
        
    c = data[offset];
    offset++;
    
    return 1;   // length of data returned
}

int NetReader1::getLine( xstring& s, bool& newlineNotFound )
{
    int startOffset;
    char c;
    int status;
    bool crFound;
        
    startOffset = offset;
    
    s = "";
    
    newlineNotFound = false;
    crFound = false;
    while ( true )
    {
        status = getChar( c );
        
        if ( status == 0 )
        {
            // no more chars in stream
            newlineNotFound = true;
            return s.length();
        }
        
        if ( status < 0 )
        {
            // some kind of error
            return status;
        }
        
        s += c;
        
        if ( c == 0xA )
        {
            return s.length();
        }
        else if ( c == 0xD )            // CR
        {
            crFound = true;
        }
        else
        {
            // wanted to see LF after CR; error
            if ( crFound )
            {
                newlineNotFound = true;
                return s.length();
            }
        }
    }
    
    return 0;
}

// read data
int NetReader1::readData( void* buf, size_t bufferLen )
{
    int status;
    int copyLen;
    size_t dataLeft;    // size of data left in buffer
    
    if ( bufferLen <= 0 )
        return 0;
    
    // refill empty buffer  
    if ( !bufferHasData() )
    {
        status = refillBuffer();
        
        if ( status <= 0 )
            return status;
            
        if ( !bufferHasData() )
            return 0;
            
        dataLeft = buflen;
    }
    else
    {
        dataLeft = buflen - offset;
    }
    
    if ( bufferLen < dataLeft )
    {
        copyLen = bufferLen;
    }
    else
    {
        copyLen = dataLeft;
    }
    
    memcpy( buf, &data[offset], copyLen );
    
    offset += copyLen;
    
    return copyLen;
}

// throws MiniServerReadException.RCODE_TIMEDOUT
static int SocketRead( int sockfd, char* buffer, size_t bufsize,
    int timeoutSecs )
{
    int retCode;
    fd_set readSet;
    struct timeval timeout;
    int numRead;

    assert( sockfd > 0 );
    assert( buffer != NULL );
    assert( bufsize > 0 );

    FD_ZERO( &readSet );
    FD_SET( sockfd, &readSet );

    timeout.tv_sec = timeoutSecs;
    timeout.tv_usec = 0;

    while ( true )
    {
        retCode = select( sockfd + 1, &readSet, NULL, NULL, &timeout );

        if ( retCode == 0 )
        {
            // timed out
            MiniServerReadException e( "SocketRead(): timed out" );
            e.setErrorCode( RCODE_TIMEDOUT );
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
            throw e;
        }
        if ( retCode == -1 )
        {
            if ( errno == EINTR )
            {
                continue; // ignore interrupts
            }
            return retCode;     // error
        }
        else
        {
            break;
        }
    }

    // read data
    numRead = read( sockfd, buffer, bufsize );
    return numRead;


}

ssize_t NetReader1::refillBuffer()
{
    ssize_t numRead;

    // old code
    // numRead = read( sockfd, data, maxbufsize );
    ///////

    numRead = SocketRead( sockfd, data, maxbufsize, TIMEOUT_SECS );

    if ( numRead >= 0 )
    {
        buflen = numRead;
    }
    else
    {
        buflen = 0;
    }
    
    offset = 0;
        
    return numRead;
}
#endif	// FORCE_NO_EXCEPTIONS

EDERRCODE NetReader1::getCharNoEx( char& c, int* pStatus )
{
	if ( !bufferHasData() )
	{
		ED_RETURN_EXCEPTION(refillBufferNoEx(pStatus));
    
		if ( *pStatus <= 0 )
			return *pStatus;
        
		if ( !bufferHasData() )
			return 0;
	}
    
	c = data[offset];
	offset++;

	*pStatus = 1;
	return ED_OK;
}

EDERRCODE NetReader1::getLineNoEx( xstring& s, bool& newlineNotFound, int* pNumRead )
{
	int startOffset;
	char c;
	bool crFound;
    
	startOffset = offset;

	s = "";

	newlineNotFound = false;
	crFound = false;
	while ( true )
	{
		ED_RETURN_EXCEPTION(getCharNoEx(c, pNumRead));
    
		if ( *pNumRead == 0 )
		{
			// no more chars in stream
			newlineNotFound = true;
			*pNumRead = s.length();
			return ED_OK;
		}
    
		if ( *pNumRead < 0 )
		{
			// some kind of error
			return ED_OK;
		}
    
		s += c;
    
		if ( c == 0xA )
		{
			*pNumRead = s.length();
			return ED_OK;
		}
		else if ( c == 0xD )            // CR
		{
			crFound = true;
		}
		else
		{
			// wanted to see LF after CR; error
			if ( crFound )
			{
				newlineNotFound = true;
				*pNumRead = s.length();
				return ED_OK;
			}
		}
	}

	*pNumRead = 0;
	return ED_OK;
}

// read data
EDERRCODE NetReader1::readDataNoEx( void* buf, size_t bufferLen, int* pNumRead )
{
	int copyLen;
	size_t dataLeft;    // size of data left in buffer

	if ( bufferLen <= 0 )
		return 0;

	// refill empty buffer  
	if ( !bufferHasData() )
	{
		ED_RETURN_EXCEPTION(refillBufferNoEx(pNumRead));
    
		if ( *pNumRead <= 0 )
			return ED_OK;
        
		if ( !bufferHasData() )
		{
			*pNumRead = 0;
			return ED_OK;
		}
        
		dataLeft = buflen;
	}
	else
	{
		dataLeft = buflen - offset;
	}

	if ( bufferLen < dataLeft )
	{
		copyLen = bufferLen;
	}
	else
	{
		copyLen = dataLeft;
	}

	memcpy( buf, &data[offset], copyLen );

	offset += copyLen;

	*pNumRead = copyLen;
	return ED_OK;
}

// throws MiniServerReadException.RCODE_TIMEDOUT
static EDERRCODE
SocketReadNoEx( int sockfd, char* buffer, size_t bufsize, int timeoutSecs, int* pNumRead )
{
	int retCode;
	fd_set readSet;
	struct timeval timeout;

	assert( sockfd > 0 );
	assert( buffer != NULL );
	assert( bufsize > 0 );

	FD_ZERO( &readSet );
	FD_SET( sockfd, &readSet );

	timeout.tv_sec = timeoutSecs;
	timeout.tv_usec = 0;

	while ( true )
	{
		retCode = select( sockfd + 1, &readSet, NULL, NULL, &timeout );

		if ( retCode == 0 )
		{
			// timed out
			DEBUG_THROW_MINISERVER_TIMED_OUT_EXCEPTION;
			return EDERR_MINISERVER_TIMED_OUT_EXCEPTION;
		}
		if ( retCode == -1 )
		{
			if ( errno == EINTR )
			{
				continue; // ignore interrupts
			}
			*pNumRead = -1;
//			return retCode;     // error
			return ED_OK;
		}
		else
		{
			break;
		}
	}

	// read data
	*pNumRead = read( sockfd, buffer, bufsize );

	return ED_OK;
}

EDERRCODE
NetReader1::refillBufferNoEx(int *pNumRead)
{
	ED_RETURN_EXCEPTION(SocketReadNoEx( sockfd, data, maxbufsize, TIMEOUT_SECS, pNumRead ));

	if ( *pNumRead >= 0 )
	{
		buflen = *pNumRead;
	}
	else
	{
		buflen = 0;
	}

	offset = 0;
    
	return ED_OK;
}


static void WriteNetData( const char* s, int sockfd )
{
    write( sockfd, s, strlen(s) );
}
    
// determines type of UPNP command from request line, ln
static HTTP_COMMAND_TYPE GetCommandType( const xstring& ln )
{
    // commands GET, POST, M-POST, SUBSCRIBE, UNSUBSCRIBE, NOTIFY
    
    xstring line = ln;
    int i;
    char c;
    
    char * getStr = "GET";
    char * postStr = "POST";
    char * mpostStr = "M-POST";
    char * subscribeStr = "SUBSCRIBE";
    char * unsubscribeStr = "UNSUBSCRIBE";
    char * notifyStr = "NOTIFY";
    char * pattern;
    HTTP_COMMAND_TYPE retCode=CMD_HTTP_UNKNOWN;

#ifndef FORCE_NO_EXCEPTIONS    
    try
    {
#endif	// FORCE_NO_EXCEPTIONS
        line.toUppercase();
    
        c = line[0];
        
        switch (c)
        {
            case 'G':
                pattern = getStr;
                retCode = CMD_HTTP_GET;
                break;
                
            case 'P':
                pattern = postStr;
                retCode = CMD_SOAP_POST;
                break;
                
            case 'M':
                pattern = mpostStr;
                retCode = CMD_SOAP_MPOST;
                break;
                
            case 'S':
                pattern = subscribeStr;
                retCode = CMD_GENA_SUBSCRIBE;
                break;
                
            case 'U':
                pattern = unsubscribeStr;
                retCode = CMD_GENA_UNSUBSCRIBE;
                break;
                
            case 'N':
                pattern = notifyStr;
                retCode = CMD_GENA_NOTIFY;
                break;
            
            default:
                // unknown method
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -1;
	            return CMD_HTTP_UNKNOWN;
        }
        
        int patLength = strlen( pattern );
        
        for ( i = 1; i < patLength; i++ )
        {
            if ( line[i] != pattern[i] )
			{
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -1;
	            return CMD_HTTP_UNKNOWN;
			}
        }
#ifndef FORCE_NO_EXCEPTIONS    
    }
    catch ( OutOfBoundsException& e )
    {
        return CMD_HTTP_UNKNOWN;
    }
/*
    catch ( int parseCode )
    {
        if ( parseCode == -1 )
        {
            return CMD_HTTP_UNKNOWN;
        }
    }
*/
#endif	// FORCE_NO_EXCEPTIONS
    
    return retCode;
}

static int ParseContentLength( const xstring& textLine, bool& malformed )
{
    xstring line;
    xstring asciiNum;
    char *pattern = "CONTENT-LENGTH";
    int patlen = strlen( pattern );
    int i;
    int contentLength;
    
    malformed = false;
    
    contentLength = -1;
    line = textLine;
    line.toUppercase();
    
    if ( strncmp(line.c_str(), pattern, patlen) != 0 )
    {
        // unknown header
        return -1;
    }

    i = patlen;

	malformed = true;

#ifndef FORCE_NO_EXCEPTIONS    
    try
    {
#endif	// FORCE_NO_EXCEPTIONS
        // skip whitespace
        while ( line[i] == ' ' || line [i] == '\t' )
        {
            i++;
        }
        
        // ":"
        if ( line[i] != ':' )
        {
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw -1;
			return -1;
        }
        i++;
        
        char* invalidChar = NULL;
        
        contentLength = strtol( &line[i], &invalidChar, 10 );
        // anything other than crlf or whitespace after number is invalid
        if ( *invalidChar != '\0' )
        {
            // see if there is an invalid number
            while ( *invalidChar )
            {
                char c;
                
                c = *invalidChar;
                
                if ( !(c == ' ' || c == '\t' || c == '\r' || c == '\n') )
                {
                    // invalid char in number
//					diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//					throw -1;
					return -1;
                }
                
                invalidChar++;
            }
        }
#ifndef FORCE_NO_EXCEPTIONS    
    }
    catch ( OutOfBoundsException& e )
    {
        malformed = true;
        return -1;
    }
/*
    catch ( int errCode )
    {
        if ( errCode == -1 )
        {
            malformed = true;
            return -1;
        }
    }
*/
#endif	// FORCE_NO_EXCEPTIONS
    
    malformed = false;
    return contentLength;   
}

#ifndef FORCE_NO_EXCEPTIONS
// throws 
//	OutOfMemoryException
//	MiniServerReadException
//		RCODE_NETWORK_READ_ERROR
//		RCODE_MALFORMED_LINE
//		RCODE_METHOD_NOT_IMPLEMENTED 
//		RCODE_LENGTH_NOT_SPECIFIED
static void ReadRequest( int sockfd, xstring& document,
    HTTP_COMMAND_TYPE& command )
{
    NetReader1 reader( sockfd );
    xstring reqLine;
    xstring line;
    bool newlineNotFound;
    int status;
    HTTP_COMMAND_TYPE cmd;
    int contentLength;
    const int BUFSIZE = 3;
    char buf[ BUFSIZE + 1 ];
    MiniServerReadException excep;
    
    document = "";
    
    // read request-line
    status = reader.getLine( reqLine, newlineNotFound );
    
    if ( status < 0 )
    {
        // read error
        excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw excep;
    }
    
    if ( newlineNotFound )
    {
        // format error
        excep.setErrorCode( RCODE_MALFORMED_LINE );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw excep;
    }
        
    cmd = GetCommandType( reqLine );
    
    if ( cmd == CMD_HTTP_UNKNOWN )
    {
        // unknown or unsupported cmd
        excep.setErrorCode( RCODE_METHOD_NOT_IMPLEMENTED );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw excep;
    }
    
    document += reqLine;
    
    contentLength = -1;     // init
    
    // read headers
    while ( true )
    {
        status = reader.getLine( line, newlineNotFound );
        
        if ( status < 0 )
        {
            // network error
            excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
            throw excep;
        }   
        
        if ( newlineNotFound )
        {
            // bad format
            excep.setErrorCode( RCODE_MALFORMED_LINE );
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
            throw excep;
        }
        
        // get content-length if not obtained already
        if ( contentLength < 0 )
        {
            bool malformed;
            
            contentLength = ParseContentLength( line, malformed );
            
            if ( malformed )
            {
                excep.setErrorCode( RCODE_MALFORMED_LINE );
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                throw excep;
            }
        }
        
        document += line;
        
        // done ?
        if ( line == "\n" || line == "\r\n" )
        {
            break;
        }
    }
    
    // must have body for POST and M-POST msgs
    if ( contentLength < 0 &&
         (cmd == CMD_SOAP_POST || cmd == CMD_SOAP_MPOST)
       )
    {
        // HTTP: length reqd
        excep.setErrorCode( RCODE_LENGTH_NOT_SPECIFIED );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw excep;
    }
    
    if ( contentLength > 0 )
    {
        int totalBytesRead = 0;
        
        // read body
        while ( true )
        {
            int bytesRead;
            
            bytesRead = reader.readData( buf, BUFSIZE );
            buf[ bytesRead ] = 0;   // null terminate string
            if ( bytesRead > 0 )
            {
                document.appendLimited( buf, bytesRead );
                totalBytesRead += bytesRead;
                
                if ( totalBytesRead >= contentLength )
                {
                    // done reading data
                    break;
                }
            }
            else if ( bytesRead == 0 )
            {
                // done
                break;
            }
            else
            {
                // error reading
                excep.setErrorCode( RCODE_NETWORK_READ_ERROR );
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                throw excep;
            }
        }
    }
    
    command = cmd;
}
#endif	// FORCE_NO_EXCEPTIONS

// throws 
//	OutOfMemoryException
//	MiniServerReadException
//		RCODE_NETWORK_READ_ERROR
//		RCODE_MALFORMED_LINE
//		RCODE_METHOD_NOT_IMPLEMENTED 
//		RCODE_LENGTH_NOT_SPECIFIED
static EDERRCODE ReadRequestNoEx( int sockfd, xstring& document,
    HTTP_COMMAND_TYPE& command )
{
	NetReader1 reader( sockfd );
	xstring reqLine;
	xstring line;
	bool newlineNotFound;
	int status;
	HTTP_COMMAND_TYPE cmd;
	int contentLength;
	const int BUFSIZE = 3;
	char buf[ BUFSIZE + 1 ];
	MiniServerReadException excep;

	document = "";

	// read request-line
	ED_RETURN_EXCEPTION(reader.getLineNoEx( reqLine, newlineNotFound, &status ));

	if ( status < 0 )
	{
		// read error
		DEBUG_THROW_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
		return EDERR_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
	}

	if ( newlineNotFound )
	{
		// format error
		DEBUG_THROW_MINISERVER_MALFORMED_LINE_EXCEPTION;
		return EDERR_MINISERVER_MALFORMED_LINE_EXCEPTION;
	}
    
	cmd = GetCommandType( reqLine );

	if ( cmd == CMD_HTTP_UNKNOWN )
	{
		// unknown or unsupported cmd
		DEBUG_THROW_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION;
		return EDERR_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION;
	}

	document += reqLine;

	contentLength = -1;     // init

	// read headers
	while ( true )
	{
		ED_RETURN_EXCEPTION(reader.getLineNoEx( line, newlineNotFound, &status ));
    
		if ( status < 0 )
		{
			// network error
			DEBUG_THROW_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
			return EDERR_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
		}   
    
		if ( newlineNotFound )
		{
			// bad format
			DEBUG_THROW_MINISERVER_MALFORMED_LINE_EXCEPTION;
			return EDERR_MINISERVER_MALFORMED_LINE_EXCEPTION;
		}
    
		// get content-length if not obtained already
		if ( contentLength < 0 )
		{
			bool malformed;
        
			contentLength = ParseContentLength( line, malformed );
        
			if ( malformed )
			{
				DEBUG_THROW_MINISERVER_MALFORMED_LINE_EXCEPTION;
				return EDERR_MINISERVER_MALFORMED_LINE_EXCEPTION;
			}
		}
    
		document += line;
    
		// done ?
		if ( line == "\n" || line == "\r\n" )
		{
			break;
		}
	}

	// must have body for POST and M-POST msgs
	if ( contentLength < 0 &&
		 (cmd == CMD_SOAP_POST || cmd == CMD_SOAP_MPOST)
	   )
	{
		// HTTP: length reqd
		DEBUG_THROW_MINISERVER_LENGTH_NOT_SPECIFIED_EXCEPTION;
		return EDERR_MINISERVER_LENGTH_NOT_SPECIFIED_EXCEPTION;
	}

	if ( contentLength > 0 )
	{
		int totalBytesRead = 0;
    
		// read body
		while ( true )
		{
			int bytesRead;

			ED_RETURN_EXCEPTION(reader.readDataNoEx( buf, BUFSIZE, &bytesRead ));
			buf[ bytesRead ] = 0;   // null terminate string
			if ( bytesRead > 0 )
			{
				document.appendLimited( buf, bytesRead );
				totalBytesRead += bytesRead;
            
				if ( totalBytesRead >= contentLength )
				{
					// done reading data
					break;
				}
			}
			else if ( bytesRead == 0 )
			{
				// done
				break;
			}
			else
			{
				// error reading
				DEBUG_THROW_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
				return EDERR_MINISERVER_NETWORK_READ_ERROR_EXCEPTION;
			}
		}
	}

	command = cmd;

	return ED_OK;
}

// throws OutOfMemoryException
static void HandleError( int errCode, int sockfd )
{
    xstring errMsg;
    
    switch (errCode)
    {
    case RCODE_NETWORK_READ_ERROR:
        break;

    case RCODE_TIMEDOUT:
        break;
        
    case RCODE_MALFORMED_LINE:
        errMsg = "400 Bad Request";
        break;
        
    case RCODE_LENGTH_NOT_SPECIFIED:
        errMsg = "411 Length Required";
        break;
        
    case RCODE_METHOD_NOT_ALLOWED:
        errMsg = "405 Method Not Allowed";
        break;

    case RCODE_INTERNAL_SERVER_ERROR:
        errMsg = "500 Internal Server Error";
        break;
                
    case RCODE_METHOD_NOT_IMPLEMENTED:
        errMsg = "511 Not Implemented";
        break;
        
    default:
        DBG(
            UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
            "HandleError: unknown code %d\n", errCode ); )
        break;
    };

    // no error msg to send; done
    if ( errMsg.length() == 0 )
        return;
        
    xstring msg;
    
    msg = "HTTP/1.1 ";
    msg += errMsg;
    msg += "\r\n\r\n";
    
    // send msg
    WriteNetData( msg.c_str(), sockfd );
    
    // dbg
    // sleep so that client does get connection reset
    //sleep( 3 );
    ///////
    
    // dbg
    DBG(
        UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
            "http error: %s\n", msg.c_str() ); )
    ///////
}

#ifndef FORCE_NO_EXCEPTIONS
// throws MiniServerReadException.RCODE_METHOD_NOT_IMPLEMENTED
static void MultiplexCommand( HTTP_COMMAND_TYPE cmd, const xstring& document,
        int sockfd )
{
    MiniServerCallback callback;
    
    switch ( cmd )
    {
    case CMD_SOAP_POST:
    case CMD_SOAP_MPOST:
        callback = gSoapCallback;
        break;
            
    case CMD_GENA_NOTIFY:
    case CMD_GENA_SUBSCRIBE:
    case CMD_GENA_UNSUBSCRIBE:
        callback = gGenaCallback;
        break;
            
    case CMD_HTTP_GET:
        callback = gGetCallback;
        break;
            
    default:
        callback = NULL;
    }

    //DBG(printf("READ>>>>>>\n%s\n<<<<<<READ\n", document.c_str()));
    
    if ( callback == NULL )
    {
        MiniServerReadException e( "callback not defined or unknown method" );
        e.setErrorCode( RCODE_METHOD_NOT_IMPLEMENTED );
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw e;
    }
    
    callback( document.c_str(), sockfd );
}
#endif

// throws MiniServerReadException.RCODE_METHOD_NOT_IMPLEMENTED
static EDERRCODE MultiplexCommandNoEx( HTTP_COMMAND_TYPE cmd, const xstring& document,
        int sockfd )
{
    MiniServerCallback callback;
    
    switch ( cmd )
    {
    case CMD_SOAP_POST:
    case CMD_SOAP_MPOST:
        callback = gSoapCallback;
        break;
            
    case CMD_GENA_NOTIFY:
    case CMD_GENA_SUBSCRIBE:
    case CMD_GENA_UNSUBSCRIBE:
        callback = gGenaCallback;
        break;
            
    case CMD_HTTP_GET:
        callback = gGetCallback;
        break;
            
    default:
        callback = NULL;
    }

    //DBG(printf("READ>>>>>>\n%s\n<<<<<<READ\n", document.c_str()));
    
    if ( callback == NULL )
    {
		DEBUG_THROW_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION;
		return EDERR_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION;
    }
    
    callback( document.c_str(), sockfd );

	return ED_OK;
}

#ifndef FORCE_NO_EXCEPTIONS
static void HandleRequest( void *args )
{
    int sockfd;
    xstring document;
    HTTP_COMMAND_TYPE cmd;
    
    sockfd = (int) args;
    
    try
    {
        ReadRequest( sockfd, document, cmd );
        
        // pass data to callback
        MultiplexCommand( cmd, document, sockfd );
        
        //printf( "input document:\n%s\n", document.c_str() );
    }
    catch ( MiniServerReadException& e )
    {
        //DBG( e.print(); )
        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "error code = %d\n", e.getErrorCode()); )

        HandleError( e.getErrorCode(), sockfd );
        
        // destroy connection
        close( sockfd );
    }   
	catch ( ... )
	{
		DBG(
		    UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
		        "HandleRequest(): unknown error\n"); )
		close( sockfd );
	}
}
#endif	// FORCE_NO_EXCEPTIONS

static void HandleRequestNoEx( void *args )
{
	int sockfd;
	xstring document;
	HTTP_COMMAND_TYPE cmd;

	sockfd = (int) args;

	EDERRCODE ederr = ReadRequestNoEx( sockfd, document, cmd );
	if (ED_FAILED(ederr))
		goto CatchMiniserverReadException;
    
	// pass data to callback
	ederr = MultiplexCommandNoEx( cmd, document, sockfd );
	if (ED_FAILED(ederr))
		goto CatchMiniserverReadException;
    
	return;

CatchMiniserverReadException:

	HandleError( EDERR_TO_READ_EXCEPTION_CODE(ederr), sockfd );

	// destroy connection
	close( sockfd );
}

#ifndef FORCE_NO_EXCEPTIONS
static void RunMiniServer( void* args )
{
    struct sockaddr_in clientAddr;
    int listenfd;

    listenfd = (int)args;   

#ifdef USE_PTHREADS
    gMServThread = pthread_self();
#else
    gMServThread = cyg_thread_self();
#endif
    gMServState = MSERV_RUNNING;

    try
    {
        while ( true )
        {
            int connectfd;
            socklen_t clientLen;
            
            DBG(UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__, "Waiting...\n" ); )
            
            // get a client request
            while ( true )
            {
                // stop server
                if ( gMServState == MSERV_STOPPING )
                {
					diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                    throw -9;
                }

                connectfd = accept( listenfd, (sockaddr*) &clientAddr,
                        &clientLen );
                if ( connectfd > 0 )
                {
                    // valid connection
                    break;
                }
                if ( connectfd == -1 && errno == EINTR )
                {
                    // interrupted -- stop?
                    if ( gMServState == MSERV_STOPPING )
                    {
						diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                        throw -9;
                    }
                    else
                    {
                        // ignore interruption
                        continue;
                    }
                }
                else
                {
                    xstring errStr =  "Error: RunMiniServer: accept(): ";
                    errStr = strerror( errno );
					diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
                    throw BasicException( errStr.c_str() );
                }
            }
            
            int sched_stat;

/*
            sched_stat = tpool_Schedule( HandleRequest, (void*)connectfd );
            if ( sched_stat < 0 )
            {
                HandleError( RCODE_INTERNAL_SERVER_ERROR, connectfd );              
            }
*/

//			HandleRequest( (void *)connectfd );
			HandleRequestNoEx( (void *)connectfd );
        }
    }
    catch ( GenericException& e )
    {
        //DBG( e.print(); )
    }
    catch ( int code )
    {
        if ( code == -9 )
        {
            // stop miniserver
            assert( gMServState == MSERV_STOPPING );
            
            DBG(
                UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                    "Miniserver: recvd STOP signal\n"); )
            
            close( listenfd );
            
            gMServState = MSERV_IDLE;
            gMServThread = 0;
        }
    }
}
#endif	// FORCE_NO_EXCEPTIONS

static void RunMiniServerNoEx( void* args )
{
    struct sockaddr_in clientAddr;
    int listenfd;
	int code = 0;

    listenfd = (int)args;   

#ifdef USE_PTHREADS
    gMServThread = pthread_self();
#else
    gMServThread = cyg_thread_self();
#endif
    gMServState = MSERV_RUNNING;

    while ( true )
    {
        int connectfd;
        socklen_t clientLen;
        
        DBG(UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__, "Waiting...\n" ); )
        
        // get a client request
        while ( true )
        {
            // stop server
            if ( gMServState == MSERV_STOPPING )
            {
				code = -9;
				goto CatchError;
            }

            connectfd = accept( listenfd, (sockaddr*) &clientAddr, &clientLen );
            if ( connectfd > 0 )
            {
                // valid connection
                break;
            }
            if ( connectfd == -1 && errno == EINTR )
            {
                // interrupted -- stop?
                if ( gMServState == MSERV_STOPPING )
                {
					code = -9;
					goto CatchError;
                }
                else
                {
                    // ignore interruption
                    continue;
                }
            }
            else
            {
				xstring errStr =  "Error: RunMiniServer: accept(): ";
				errStr = strerror( errno );
				diag_printf("%s\n", errStr);
				continue;
            }
        }
        
        int sched_stat;

/*
        sched_stat = tpool_Schedule( HandleRequest, (void*)connectfd );
        if ( sched_stat < 0 )
        {
            HandleError( RCODE_INTERNAL_SERVER_ERROR, connectfd );              
        }
*/

		HandleRequestNoEx( (void *)connectfd );
    }


CatchError:

	if ( code == -9 )
	{
		// stop miniserver
		assert( gMServState == MSERV_STOPPING );
    
		DBG(UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__, "Miniserver: recvd STOP signal\n"); )
    
		close( listenfd );
    
		gMServState = MSERV_IDLE;
		gMServThread = 0;
    }
}

// returns port to which socket, sockfd, is bound.
// -1 on error; check errno
// > 0 means port number
static int get_port( int sockfd )
{
    sockaddr_in sockinfo;
    socklen_t len;
    int code;
    int port;

	len = sizeof(sockinfo);
    code = getsockname( sockfd, (sockaddr*)&sockinfo, &len );
    if ( code == -1 )
    {
        return -1;
    }

    port = htons( sockinfo.sin_port );
    DBG(
        UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
            "sockfd = %d, .... port = %d\n", sockfd, port ); )

    return port;
}


// if listen port is 0, port is dynamically picked
// returns:
//   on success: actual port socket is bound to
//   on error:   a negative number UPNP_E_XXX
static int ms_sock=0;
int StartMiniServer( unsigned short listen_port )
{
	struct sockaddr_in serverAddr;
	int listenfd = 0;
	int success;
	int actual_port;
	int on =1;
	int retCode = 0;

	if ( gMServState != MSERV_IDLE )
	{
		return UPNP_E_INTERNAL_ERROR;  // miniserver running
	}

	//printf("listen port: %d\n",listen_port);
	listenfd = socket( AF_INET, SOCK_STREAM, 0 );
	if ( listenfd <= 0 )
	{
		return UPNP_E_OUTOF_SOCKET;  // miniserver running
	}

//	bzero( &serverAddr, sizeof(serverAddr) );
	memset( &serverAddr, 0, sizeof(serverAddr) );
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	serverAddr.sin_port = htons( listen_port );

	//THIS IS ALLOWS US TO BIND AGAIN IMMEDIATELY
	//AFTER OUR SERVER HAS BEEN CLOSED
	//THIS MAY CAUSE TCP TO BECOME LESS RELIABLE
	//HOWEVER IT HAS BEEN SUGESTED FOR TCP SERVERS
	if (setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on, sizeof(int))==-1)
	{
		retCode = UPNP_E_SOCKET_BIND;
		goto CatchError;
	}
	if (setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&on, sizeof(int))==-1)
	{
		retCode = UPNP_E_SOCKET_BIND;
		goto CatchError;
	}

	success = bind( listenfd, (sockaddr*)&serverAddr,
		sizeof(serverAddr) );
	if ( success == -1 )
	{
		retCode = UPNP_E_SOCKET_BIND;
		goto CatchError;
	}

	success = listen( listenfd, 10 );
	if ( success == -1 )
	{
		retCode = UPNP_E_LISTEN;
		goto CatchError;
	}

	actual_port = get_port( listenfd );
	if ( actual_port <= 0 )
	{
		retCode = UPNP_E_INTERNAL_ERROR;
		goto CatchError;
	}

//	success = tpool_Schedule( RunMiniServer, (void *)listenfd );
	success = tpool_Schedule( RunMiniServerNoEx, (void *)listenfd );
	if ( success < 0 )
	{
		retCode = UPNP_E_OUTOF_MEMORY;
		goto CatchError;
	}

	// wait for miniserver to start
	while ( gMServState != MSERV_RUNNING )
	{
		//sleep(1);
		cyg_thread_delay(10);
	}

    ms_sock = listenfd;
	retCode = actual_port;

	return retCode;

CatchError:

    if ( listenfd != 0 )
        close( listenfd );

    return retCode;
}

// returns 0: success; -2 if miniserver is idle
int StopMiniServer( void )
{
    if ( gMServState == MSERV_IDLE )
    {
        return -2;
    }
        
    gMServState = MSERV_STOPPING;
    
#ifdef USE_PTHREADS
    // keep sending signals until server stops
    while ( true )
    {
        if ( gMServState == MSERV_IDLE )
        {
            break;
        }

        DBG(
            UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__,
                "StopMiniServer(): sending interrupt\n"); )
        
        int code = 
		tintr_Interrupt( gMServThread );
        if ( code < 0 )
        {
            DBG(
                UpnpPrintf( UPNP_CRITICAL, MSERV, __FILE__, __LINE__,
                    "%s: StopMiniServer(): interrupt failed",
                    strerror(errno) ); )

            //DBG( perror("StopMiniServer(): interrupt failed"); )
        }
        if ( gMServState == MSERV_IDLE )
        {
            break;
        }
        
        //sleep( 1 );     // pause before signalling again
		cyg_thread_delay(100);
    }
#else
    // we can't signal the thread to die, so just be hostile about it
    // unfortunately the socket is local to the thread, so we can't clean it up properly here
    while( !cyg_thread_delete( gMServThread ) )
    {
        cyg_thread_delay(1);
    }
    if( ms_sock ) {
        close(ms_sock);
        ms_sock = 0;
        gMServState = MSERV_IDLE;
        gMServThread = 0;
    }
#endif
    
    return 0;
}
#endif
