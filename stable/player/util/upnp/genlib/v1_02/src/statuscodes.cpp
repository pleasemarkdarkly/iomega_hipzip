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

#include <util/upnp/api/config.h>



#include <stdio.h>
#include <util/upnp/genlib/statuscodes.h>

#define NUM_1XX_CODES   2
static char* Http1xxCodes[NUM_1XX_CODES] =
{
    "Continue",
    "Switching Protocols"
};

#define NUM_2XX_CODES   7
static char* Http2xxCodes[NUM_2XX_CODES] =
{
    "OK",
    "Created",
    "Accepted",
    "Non-Authoratative Information",
    "No Content",
    "Reset Content",
    "Partial Content",
};

#define NUM_3XX_CODES   8
static char* Http3xxCodes[NUM_3XX_CODES] =
{
    "Multiple Choices",
    "Moved Permanently",
    "Found",
    "See Other",
    "Not Modified",
    "Use Proxy",
    NULL,
    "Temporary Redirect",
};

#define NUM_4XX_CODES   18
static char* Http4xxCodes[NUM_4XX_CODES] =
{
    "Bad Request",
    "Unauthorized",
    "Payment Required",
    "Forbidden",
    "Not Found",
    "Method Not Allowed",
    "Not Acceptable",
    "Proxy Authentication Required",
    "Request Timeout",
    "Conflict",
    "Gone",
    "Length Required",
    "Precondition Failed",
    "Request Entity Too Large",
    "Request-URI Too Long",
    "Unsupported Media Type",
    "Requested Range Not Satisfiable",
    "Expectation Failed",
};

#define NUM_5XX_CODES   6
static char* Http5xxCodes[NUM_5XX_CODES] =
{
    "Internal Server Error",
    "Not Implemented",
    "Bad Gateway",
    "Service Unavailable",
    "Gateway Timeout",
    "HTTP Version Not Supported",
};


const char* http_GetCodeText( int statusCode )
{
    char **table = NULL;
    int numEntries = -1;

	if ( statusCode < 0 )
		statusCode = -statusCode;

    if ( statusCode < 100 || statusCode >= 600 )
        return NULL;
    
    if ( statusCode >= 100 && statusCode <= 199 )
    {
        table = Http1xxCodes;
        numEntries = NUM_1XX_CODES;
    }
    else if ( statusCode >= 200 && statusCode <= 299 )
    {
        table = Http2xxCodes;
        numEntries = NUM_2XX_CODES;
    }
    else if ( statusCode >= 300 && statusCode <= 399 )
    {
        table = Http3xxCodes;
        numEntries = NUM_3XX_CODES;
    }
    else if ( statusCode >= 400 && statusCode <= 499 )
    {
        table = Http4xxCodes;
        numEntries = NUM_4XX_CODES;
    }
    else if ( statusCode >= 500 && statusCode <= 599 )
    {
        table = Http5xxCodes;
        numEntries = NUM_5XX_CODES;
    }

    int index;

    index = statusCode % 100;
    if ( index >= numEntries )
        return NULL;
    else    
        return table[ index ];
}

//#endif
//#endif