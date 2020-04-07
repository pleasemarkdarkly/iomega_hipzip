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

// $Revision: 1.15 $
// $Date: 2000/09/15 18:06:44 $

#ifndef GENLIB_NET_HTTP_PARSEUTIL_H
#define GENLIB_NET_HTTP_PARSEUTIL_H

#include <util/upnp/genlib/xstring.h>
#include <util/upnp/genlib/xdlist.h>
#include <util/upnp/genlib/tokenizer.h>
#include <util/upnp/genlib/http_client.h>
#include <util/upnp/genlib/xstring.h>
#include <util/upnp/genlib/dbllist.h>
#ifndef _WIN32
#include <netinet/in.h>
#else
#include "win32.h"
#endif

#include <util/upnp/genlib/noexceptions.h>

// IDs for HTTP headers
#define HDR_UNKNOWN             -1  /* UnknownHeader */

// Std. HTTP Headers
#define HDR_ACCEPT              1   /* CommaSeparatedList<MediaRange> */
#define HDR_ACCEPT_CHARSET      2   /* CommaSeparatedList<IdentifierQValue> */
#define HDR_ACCEPT_ENCODING     3   /* CommaSeparatedList<IdentifierQValue> */
#define HDR_ACCEPT_LANGUAGE     4   // RawHeaderValue /* CommaSeparatedList<LanguageTag> */
#define HDR_ACCEPT_RANGES       5   /* RawHeaderValue */
#define HDR_AGE                 6   /* HttpNumber */
#define HDR_ALLOW               7   /* CommaSeparatedList<IdentifierValue> */
#define HDR_AUTHORIZATION       8   /* RawHeaderValue */

#define HDR_CACHE_CONTROL       9   // RawHeaderValue /* CommaSeparatedList<CacheDirective> */
#define HDR_CONNECTION          10  /* CommaSeparatedList<IdentifierValue> */
#define HDR_CONTENT_ENCODING    11  /* CommaSeparatedList<IdentifierValue> */
#define HDR_CONTENT_LENGTH      12  /* HttpNumber */
#define HDR_CONTENT_LANGUAGE    13  // RawHeaderType /* CommaSepartedList<LanguageTag> */
#define HDR_CONTENT_LOCATION    14  /* UriType */
#define HDR_CONTENT_MD5         15  /* RawHeaderValue */
#define HDR_CONTENT_RANGE       16  /* RawHeaderValue */
#define HDR_CONTENT_TYPE        17  /* MediaRange */

#define HDR_DATE                18  /* HttpDateValue */

#define HDR_ETAG                19  /* RawHeadervalue */
#define HDR_EXPECT              20  /* RawHeaderValue */
#define HDR_EXPIRES             21  /* HttpDateValue */

#define HDR_FROM                22  /* RawHeaderValue */

#define HDR_HOST                23  // RawHeaderValue /* HostPortValue */

#define HDR_IF_MATCH            24  /* RawHeaderValue */
#define HDR_IF_MODIFIED_SINCE   25  /* HttpDateValue */
#define HDR_IF_NONE_MATCH       26  /* RawHeaderValue */
#define HDR_IF_RANGE            27  /* RawHeaderValue */
#define HDR_IF_UNMODIFIED_SINCE 28  /* HttpDateValue */

#define HDR_LAST_MODIFIED       29  /* HttpDateValue */
#define HDR_LOCATION            30  /* UriType (absolute)*/

#define HDR_MAX_FORWARDS        31  /* HttpNumber */

#define HDR_PRAGMA              32  /* RawHeaderValue */
#define HDR_PROXY_AUTHENTICATE  33  /* RawHeaderValue */
#define HDR_PROXY_AUTHORIZATION 34  /* RawHeaderValue */

#define HDR_RANGE               35  /* RawHeaderValue */
#define HDR_REFERER             36  /* RawHeaderValue */
#define HDR_RETRY_AFTER         37  // RawHeaderValue /* HttpDateOrSeconds */

#define HDR_SERVER              38  /* RawHeaderValue */

#define HDR_TE                  39  /* RawHeaderValue */
#define HDR_TRAILER             40  /* RawHeaderValue */
#define HDR_TRANSFER_ENCODING   41  /* CommaSeparatedList<IdentifierValue> */

#define HDR_USER_AGENT          42  /* RawHeaderValue */

#define HDR_VARY                43  /* RawHeaderValue */
#define HDR_VIA                 44  /* RawHeaderValue */

#define HDR_WARNING             45  /* RawHeaderValue */
#define HDR_WWW_AUTHENTICATE    46  /* RawHeaderValue */

// SSDP
#define HDR_UPNP_USN            100     /* RawHeaderValue */
#define HDR_UPNP_ST             101     /* RawHeaderValue */
#define HDR_UPNP_MAN            102     /* RawHeaderValue */

// GENA
#define HDR_UPNP_NT             200     /* RawHeaderValue */
#define HDR_UPNP_NTS            201     // RawHeaderType /* NTSType */
#define HDR_UPNP_CALLBACK       202     /* RawHeaderValue */
#define HDR_UPNP_SID            203     /* RawHeaderValue */

// SOAP
#define HDR_UPNP_SOAPACTION     300     /* RawHeaderValue */


//////////////////////////////////////////

// parse error codes
#define PARSERR_COLON_NOT_FOUND     -2
#define PARSERR_BAD_REQUEST_LINE    -3

// invalid integer format or -ve number
#define PARSERR_BAD_NUMBER          -4      

// list does not satisfy [min, max] range required
#define PARSERR_BAD_LISTCOUNT       -5

// bad comma-separated list
#define PARSERR_BAD_COMMALIST       -6

// method name not known
#define PARSERR_UNKNOWN_METHOD      -7

#define PARSERR_CONTENT_LENGTH_MISSING  -9
#define PARSERR_BAD_ENTITY          -10
#define PARSERR_INCOMPLETE_ENTITY   -11
#define PARSERR_BAD_HEADER_NAME     -12

#define PARSERR_BAD_FORMAT          -15

class HttpParseException : public BasicException
{
public:
    HttpParseException() : BasicException()
    { }

    HttpParseException( const char* s, int lineNumber = -1 );
    
protected:
    virtual const char * getClassName() const
    { return "HttpParseException"; }
};

CREATE_NEW_EXCEPTION_TYPE( ParseNoMatchException, HttpParseException, "ParseNoMatchException" )
CREATE_NEW_EXCEPTION_TYPE( ParseFailException, HttpParseException, "ParseFailException" )

EDERRCODE SkipOptionalLWS( Tokenizer& scanner );


// UPNP_POST, UPNP_MPOST, NOTIFY(opt)
enum UpnpMethodType { HTTP_UNKNOWN_METHOD = -1,
    HTTP_GET, HTTP_HEAD, UPNP_NOTIFY,
    UPNP_MSEARCH, UPNP_POST, UPNP_MPOST, UPNP_SUBSCRIBE,
    UPNP_UNSUBSCRIBE };

// base class for all header values
class HttpHeaderValue
{
public:
    virtual void toString( xstring& s ) = 0;
//	virtual void load( Tokenizer& scanner ) = 0;
	virtual EDERRCODE load( Tokenizer& scanner ) = 0;
    
    virtual ~HttpHeaderValue()
    { }
};

// also allows optionally using q for each value
class HttpQHeaderValue : public HttpHeaderValue
{
public:
    HttpQHeaderValue()
    {
        qIsUsed = true;
    }
    
    virtual void toString( xstring& s );
    
    float q;
    bool qIsUsed;
    
protected:
    EDERRCODE loadOptionalQValue( IN Tokenizer& scanner );
};

//typedef xdlist<HttpHeaderValue*> HttpHeaderValueList;
//typedef xdlistNode<HttpHeaderValue*> HttpHeaderValueNode;

typedef dblList HttpHeaderValueList;        // list<HttpHeaderValue*>
typedef dblListNode HttpHeaderValueNode;    // node<HttpHeaderValue*>

void FreeHttpHeaderValueNode( void* item );

class HttpHeader
{
public:
    HttpHeader();
    virtual ~HttpHeader();
    void toString( xstring& s );
    
public:
    int type;           // header type code: content-length etc
    HttpHeaderValue* value;
};

//typedef xdlist<HttpHeader> HttpHeaderList;
//typedef xdlistNode<HttpHeader> HttpHeaderNode;

typedef dblList HttpHeaderList;         // list<HttpHeader*>
typedef dblListNode HttpHeaderNode;     // node<HttpHeader*>

void FreeHttpHeaderNode( void* item );

// creates a new HttpHeaderValue object
typedef HttpHeaderValue* (*CreateNewValueCallback)(void);

///////////////////////////////////////////////////////
// stores HttpHeaderValues unsorted or sorted by q
class CommaSeparatedList : public HttpHeaderValue
{
public:
    // if HttpQHeaderValue is used, qIsUsed should be true;
    // at least minimumItems will be present in the list
    // at most maximum items will be present; infinite items if
    //  maximumItems == -1
    // set create_callback to NULL if not using load()
    CommaSeparatedList( bool qIsUsed,
        CreateNewValueCallback create_callback,
        int minimumItems = 0,
        int maximumItems = -1 )
            :   valueList(FreeHttpHeaderValueNode),
                qUsed(qIsUsed),
                minItems(minimumItems),
                maxItems(maximumItems)
    {
        createCallback = create_callback;
    }
    
    void toString( xstring& s );
	int load( Tokenizer& scanner );
    
public:
    HttpHeaderValueList valueList;
    CreateNewValueCallback createCallback;
    
private:
    bool qUsed;
    int minItems;
    int maxItems;
};


// value is a simple identifier (token as per Http EBNF)
class IdentifierValue : public HttpHeaderValue
{
public:
    IdentifierValue()
    { }
    
	void toString( xstring& s );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    xstring value;
};

// similar to IdentifierValue; except that this class
//  also supports a q
class IdentifierQValue : public HttpQHeaderValue
{
public:
    IdentifierQValue() : HttpQHeaderValue()
    { }
    
	void toString( xstring& s );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    xstring value;
};


//////////////////////////////////////////////////
// used when http value is a integer
class HttpNumber : public HttpHeaderValue
{
public:
    HttpNumber()
    { }

    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
        
public:
    int num;
};

///////////////////////
// number is encoded in HEX
class HttpHexNumber : public HttpHeaderValue
{
public:
    HttpHexNumber()
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    int num;
};


class MediaExtension : public HttpHeaderValue
{
public:
    MediaExtension()
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    xstring name;
    xstring value;
};

class MediaParam : public HttpQHeaderValue
{
public:
    MediaParam()
        : extList(FreeHttpHeaderValueNode)
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    HttpHeaderValueList extList;    // MediaExtension list
};

// MediaRange   (type/subtype of entity)
class MediaRange : public HttpQHeaderValue
{
public:
    MediaRange()
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );

    int compare( const MediaRange& r ) const;
        
public:
    xstring type;
    xstring subtype;
    MediaParam mparam;
};

/* ------------xxxxxxxxxxxxxxxx
// MediaRangeList
class MediaRangeList : public HttpHeaderValue
{
public:
    MediaRangeList()
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    HttpHeaderValueList mediaList;  // MediaRange list
};
  -------------xxxxxxxxxxxxxxxxxxx */


// matches a language tag eg: en-US
class LanguageTag : public HttpQHeaderValue
{
public:
    LanguageTag()
    {
        qIsUsed = false;
    }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
    const char* getTag() const
    { return lang.c_str(); }
    
    // returns true if value was set successfully
    bool setTag( const char* newLangTag );
    
public:
    xstring lang;
};

/* --------------------xxxxxxxxxxxxxxxxxxxxx
// cache-control info
class CacheDirective : public HttpHeaderValue
{
public:
    enum DirectiveType { UNKNOWN,
        NO_CACHE,
        NO_CACHE_FIELDS,        // use 'fields' list
        NO_STORE,
        MAX_AGE,                // use 'deltaSeconds'; secondsValid always true
        MAX_STALE,              // 'secondsValid' and 'deltaSeconds'
        MIN_FRESH,              // use 'deltaSeconds'; secondsValid always true
        NO_TRANSFORM, ONLY_IF_CACHED, PUBLIC,
        PRIVATE,                // use 'fields'
        MUST_REVALIDATE,
        PROXY_REVALIDATE,
        S_MAXAGE,               // use 'deltaSeconds'; secondsValid always true
        EXTENSION               // use 'extType', 'extensionName', 'extensionValue'
        };
        
    enum ExtensionType { IDENTIFIER, QUOTED_STRING };
        
    
public:
    CacheDirective()
    { }
    
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    DirectiveType type;
    
    // list of 'IdentifierValue'
    //CommaSeparatedList fields;

    xstring fields; // quoted, comma-separated identifiers
    
    HttpNumber deltaSeconds;
    
    // if true, 'deltaSeconds' is valid; else ignore 'deltaSeconds'
    bool secondsValid;  
    
    ExtensionType extType;
    xstring extensionName;
    xstring extensionValue;
    
private:
	int readFields( Tokenizer& scanner, bool* pbFieldsRead );
    int readDeltaSeconds( Tokenizer& scanner, bool optional );
    int readExtension( Tokenizer& scanner );
};

  ----------------xxxxxxxxxxxxxxxxxxxxxx */

// HttpDateValue
class HttpDateValue : public HttpHeaderValue
{
public:
    HttpDateValue()
    { }
    
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );

    tm gmtDateTime;
};

/* ---------------xxxxxxxxxxxxxxxxxx
// HttpDateOrSeconds
class HttpDateOrSeconds : public HttpHeaderValue
{
public:
    HttpDateOrSeconds()
    { }
    
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );
    
    bool isSeconds;
    int seconds;
    tm gmtDateTime;
};
  -----------------xxxxxxxxxxxxxxxxxxx */

/* --------------xxxxxxxxxxxxxxxxx
// HostPortValue
class HostPortValue : public HttpHeaderValue
{
public:
    HostPortValue()
    { }

    ~HostPortValue()
    { }

//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );
    
    bool setHostPort( const char* hostName, unsigned short port );
    void getHostPort( sockaddr_in* addr );

private:
    hostport_type hostport;
    xstring tempBuf;
};
  ----------------xxxxxxxxxxxxxxxxxxxxx */

// uri value
class UriType : public HttpHeaderValue
{
public:
    UriType()
    { }

//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );
    bool setUri( const char* newUriValue );

    const char* getUri() const;

    // address will have IP address and port
    // returns -1 on error, 0 on success
    int getIPAddress( OUT sockaddr_in& address );

    uri_type uri;
    
private:
    xstring tempBuf;    // store string for uri_type
};

/* ---------xxxxxxxxxxxxx
// NTSType -- notification subtype
class NTSType : public HttpHeaderValue
{
public:
    enum NTSValue { UPNP_PROPCHANGE, SSDP_ALIVE, SSDP_BYEBYE };
public:
    NTSType()
    { }
    
//	void load( Tokenizer& scanner );
	int load( Tokenizer& scanner );
    void toString( xstring& s );
    
public:
    NTSValue value;
};
----------xxxxxxxxxxxxxxxx */


// RawHeaderValue - matches any header value
class RawHeaderValue : public HttpHeaderValue
{
public:
    RawHeaderValue()
    { }
    
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );

public: 
    xstring value;
};

// when header name is not processed (i.e. HDR_UNKONWN),
//  raw header name and value saved
// note: trailing crlf not consumed
class UnknownHeader : public HttpHeaderValue
{
public:
    UnknownHeader()
    { }
    
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    void toString( xstring& s );
    
    xstring name;
    xstring value;
};

// ========================================================

// first line of a request
class HttpRequestLine
{
public:
    HttpRequestLine()
    { }
    
    virtual ~HttpRequestLine()
    { }

    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
//  void setValue( IN UpnpMethodType umethod, IN const char* uristr,
//      IN int majorVers=1, IN int minorVers = 1 );

public: 
    UpnpMethodType method;
    UriType uri;        // if pathIsStar==false, this contains url
    bool pathIsStar;    // if true, path='*'; uri is ignored
    int majorVersion;
    int minorVersion;
};

/////////
// first line of response
class HttpResponseLine
{
public:
    HttpResponseLine()
    { }
    
    virtual ~HttpResponseLine()
    { }

    // returns 0 on success; -1 if statCode is unknown
    int setValue( int statCode, int majorVers=1, int minorVers=1 );
        
    void toString( xstring& s );
//	void load( Tokenizer& scanner );
	EDERRCODE load( Tokenizer& scanner );
    
public:
    int statusCode;     // response code
    xstring reason;     // response code in msg
    int majorVersion;   // http major/minor version
    int minorVersion;   
};

///////////
// CharsetConverter
//class CharsetConverter
//{
//public:
    // read data from
//  int read( void* buf, unsigned bufsize ) = 0;
//};

class HttpMessage;

//////////
// body of http message
// note: entity may or may not be ascii string, but always has
//   a null terminator as its last character
class HttpEntity
{
public:
    enum EntityType { EMPTY, TEXT, TEXT_PTR, FILENAME };
    
public:
    HttpEntity();                           // type = text
    HttpEntity( const char* file_name );    // type = file
    
    virtual ~HttpEntity();

    int append( const char* data, unsigned datalen );
    void appendDone();
    
    int getEntityLen() const;
    const void* getEntity();

    EntityType getType() const;
    const char* getFileName() const;
    
    unsigned getSizeIncrement() const;
    void setSizeIncrement( unsigned incSize );

    // valid only if type == TEXT
    // after use, caller should destroy entity using free()
    void* detachTextEntity();

    void attachTextEntity( const void* textEntity, int entityLen );

    void setTextPtrEntity( const void* textEntity, int entityLen );

    EntityType type;
    xstring filename;
        
private:
    enum AppendStateType { IDLE, APPENDING, DONE }; 
    
private:
    // for type = text
    char *entity;
    int entitySize;
    int allocLen;
        
    // type = filename
    
    AppendStateType appendState;
    unsigned sizeInc;
    FILE* fp;

    friend class HttpMessage;
    
private:    
    void init();
    EDERRCODE appendInit();
    EDERRCODE increaseSizeBy( unsigned newSize );
};

///////////////////////////////
// entire http message
class HttpMessage
{
public:
    HttpMessage()
        : headers(FreeHttpHeaderNode)
    { }

    HttpMessage( const char* filename )
        : entity( filename ),
        headers(FreeHttpHeaderNode)
    { }

    virtual ~HttpMessage()
    { }
    
    EDERRCODE loadRequest( Tokenizer& scanner, CharReader* reader );
    EDERRCODE loadResponse( Tokenizer& scanner, CharReader* reader,
        UpnpMethodType requestMethod );
        
    // returns
    // 0: success
    // -1: std error; check errno
    // or HTTP_E_ error codes
    int loadRequest( const char* request );
    int loadResponse( const char* response, 
        UpnpMethodType requestMethod );
    
    int headerCount() const;
    
    EDERRCODE addHeader( int headerType, HttpHeaderValue* value );
    void deleteHeader( HttpHeaderNode* headerNode );
    
    HttpHeaderNode* findHeader( int headerType );
    HttpHeaderValue* getHeaderValue( int headerType );
    
    HttpHeaderNode* getFirstHeader();
    HttpHeaderNode* getNextHeader( HttpHeaderNode* headerNode );
    void startLineAndHeadersToString( xstring& s );
    
    /////////////////
    // utility funcs
    /////////////////
    
    // warning: headerType MUST be a http header type that is associated
    //   with RawHeaderValue
    EDERRCODE addRawHeader( int headerType, const char* value );
    
    EDERRCODE addContentTypeHeader( const char* type, const char* subtype );
    EDERRCODE addServerHeader();
    EDERRCODE addLastModifiedHeader( time_t last_mod );
    EDERRCODE addUserAgentHeader();

    // headerID has values of type HttpDateValue
    //  t is time in local timezone
    EDERRCODE addDateTypeHeader( int headerID, const time_t& t );

    // headerID has value of type HttpNumber
    EDERRCODE addNumTypeHeader( int headerID, int num );

    // headerID is associated with value CommaSeparatedList<IdentifierValue>
    EDERRCODE addIdentListHeader( int headerID, const char* idents[],
        int numIdents );

public:
    HttpRequestLine requestLine;
    HttpResponseLine responseLine;
    HttpEntity entity;
    bool isRequest;
    
private:
    HttpHeaderList headers;

private:
    // load request/response entity
    // tor read request, requestMethod = HTTP_UNKNOWN
    // to read response, requestMethod = whatever the request method was
    EDERRCODE loadRestOfMessage( Tokenizer& scanner, CharReader* reader,
        UpnpMethodType requestMethod );
        
    int messageBodyLen( IN UpnpMethodType requestMethod,
        OUT int& length );
    int getContentLength();
    
    void readEntityUntilClose( Tokenizer& scanner );
    int readEntityUsingLength( Tokenizer& scanner, int length );
    EDERRCODE readEncodedEntity( Tokenizer& scanner );
};



#endif /* GENLIB_NET_HTTP_PARSEUTIL_H */
