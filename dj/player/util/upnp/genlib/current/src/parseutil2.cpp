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

// $Revision: 1.5 $
// $Date: 2000/10/06 16:44:30 $

#include <util/upnp/api/config.h>

#if EXCLUDE_WEB_SERVER == 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <util/upnp/genlib/netexception.h>
#include <util/upnp/genlib/tokenizer.h>
#include <util/upnp/genlib/parseutil2.h>
#include <util/upnp/genlib/statuscodes.h>
#include <util/upnp/genlib/utilall.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/gmtdate.h>
#include <util/upnp/genlib/fileexceptions.h>
#include <util/upnp/genlib/memreader.h>

#include <util/upnp/genlib/mystring.h>

#include <util/debug/debug.h>

// HttpParseException
HttpParseException::HttpParseException( const char* s, int lineNumber )
    : BasicException("")
{
    if ( lineNumber != -1 )
    {
        char buf[100];
        
        sprintf( buf, "line %d: ", lineNumber );
        appendMessage( buf );
    }
    appendMessage( s );
}

//////
// callback used to read a HttpHeader value from a scanner
typedef HttpHeaderValue* (*ReadHttpValueCallback)
    ( Tokenizer& scanner );
    
//typedef void (*AddValueToListCallback)
typedef EDERRCODE (*AddValueToListCallback)
    ( HttpHeaderValueList& list, HttpHeaderValue* value );


// module vars ////////////////////

// static -- to do: determine os/version on the fly
static const char* gServerDesc = "Linux/6.0 UPnP/1.0 Intel UPnP/0.9";

static const char* gUserAgentDesc = "Intel UPnP/0.9";

///////////////////////////////////
    
// ******** CREATE callback functions ****
static HttpHeaderValue* CreateIdentifierValue()
{
    return new IdentifierValue;
}

static HttpHeaderValue* CreateIdentifierQValue()
{
    return new IdentifierQValue;
}

static HttpHeaderValue* CreateMediaRange()
{
    return new MediaRange;
}

/* --------------xxxxxxxxxxxxxxxxx
static HttpHeaderValue* CreateLanguageTag()
{
    return new LanguageTag;
}
  ---------------xxxxxxxxxxxxxxxx */

 /* ------------xxxxxxxxxxxxxxxxxxx
static HttpHeaderValue* CreateCacheDirective()
{
    return new CacheDirective;
}
 ------------xxxxxxxxxxxxxx */
// *********** end

struct SortedTableEntry
{
    char* name;
    int   id;
};

#define NUM_HEADERS 52

// table _must_ be sorted by header name
static SortedTableEntry HeaderNameTable[NUM_HEADERS] =
{
    { "ACCEPT",             HDR_ACCEPT },
    { "ACCEPT-CHARSET",     HDR_ACCEPT_CHARSET },
    { "ACCEPT-ENCODING",    HDR_ACCEPT_ENCODING },
    { "ACCEPT-LANGUAGE",    HDR_ACCEPT_LANGUAGE },
    { "ACCEPT-RANGES",      HDR_ACCEPT_RANGES },
    { "AGE",                HDR_AGE },
    { "ALLOW",              HDR_ALLOW },
    { "AUTHORIZATION",      HDR_AUTHORIZATION },
    
    { "CACHE-CONTROL",      HDR_CACHE_CONTROL },
    { "CALLBACK",           HDR_UPNP_CALLBACK },
    { "CONNECTION",         HDR_CONNECTION },
    { "CONTENT-ENCODING",   HDR_CONTENT_ENCODING },
    { "CONTENT-LANGUAGE",   HDR_CONTENT_LANGUAGE },
    { "CONTENT-LENGTH",     HDR_CONTENT_LENGTH },
    { "CONTENT-LOCATION",   HDR_CONTENT_LOCATION },
    { "CONTENT-MD5",        HDR_CONTENT_MD5 },
    { "CONTENT-TYPE",       HDR_CONTENT_TYPE },
    
    { "DATE",               HDR_DATE },
    
    { "ETAG",               HDR_ETAG },
    { "EXPECT",             HDR_EXPECT },
    { "EXPIRES",            HDR_EXPIRES },
    
    { "FROM",               HDR_FROM },
    
    { "HOST",               HDR_HOST },
    
    { "IF-MATCH",           HDR_IF_MATCH },
    { "IF-MODIFIED-SINCE",  HDR_IF_MODIFIED_SINCE },
    { "IF-NONE-MATCH",      HDR_IF_NONE_MATCH },
    { "IF-RANGE",           HDR_IF_RANGE },
    { "IF-UNMODIFIED-SINCE",HDR_IF_UNMODIFIED_SINCE },
    
    { "LAST-MODIFIED",      HDR_LAST_MODIFIED },
    { "LOCATION",           HDR_LOCATION },
    
    { "MAN",                HDR_UPNP_MAN },
    { "MAX-FORWARDS",       HDR_MAX_FORWARDS },
    
    { "NT",                 HDR_UPNP_NT },
    { "NTS",                HDR_UPNP_NTS },
    
    { "PRAGMA",             HDR_PRAGMA },
    { "PROXY-AUTHENTICATE", HDR_PROXY_AUTHENTICATE },
    { "PROXY-AUTHORIZATION",HDR_PROXY_AUTHORIZATION },
    
    { "RANGE",              HDR_RANGE },
    { "REFERER",            HDR_REFERER },
    
    { "SERVER",             HDR_SERVER },
    { "SID",                HDR_UPNP_SID },
    { "SOAPACTION",         HDR_UPNP_SOAPACTION },
    { "ST",                 HDR_UPNP_ST },
    
    { "TE",                 HDR_TE },
    { "TRAILER",            HDR_TRAILER },
    { "TRANSFER-ENCODING",  HDR_TRANSFER_ENCODING },
    
    { "USER-AGENT",         HDR_USER_AGENT },
    { "USN",                HDR_UPNP_USN },
    
    { "VARY",               HDR_VARY },
    { "VIA",                HDR_VIA },
    
    { "WARNING",            HDR_WARNING },
    { "WWW-AUTHENTICATE",   HDR_WWW_AUTHENTICATE },
};

// returns header ID; or -1 on error
int NameToID( const char* name,
    SortedTableEntry* table, int size, bool caseSensitive = true )
{
    int top, mid, bot;
    int cmp;
    
    top = 0;
    bot = size - 1;
    
    while ( top <= bot )
    {
        mid = (top + bot) / 2;
        if ( caseSensitive )
        {
            cmp = strcmp( name, table[mid].name );
        }
        else
        {
            cmp = strcasecmp( name, table[mid].name );
        }
        
        if ( cmp > 0 )
        {
            top = mid + 1;      // look below mid
        }
        else if ( cmp < 0 )
        {
            bot = mid - 1;      // look above mid
        }
        else    // cmp == 0
        {
            return table[mid].id;   // match
        }
    }
        
    return -1;  // header name not found
}

// returns textual representation of id in table;
//   NULL if id is invalid or missing from table
const char* IDToName( int id,
    SortedTableEntry* table, int size )
{
    if ( id < 0 )
        return NULL;
    
    for ( int i = 0; i < size; i++ )
    {
        if ( id == table[i].id )
        {
            return table[i].name;
        }
    }

    return NULL;
}


// skips all blank lines (lines with crlf + optional whitespace);
// removes leading whitespace from first non-blank line
static EDERRCODE SkipBlankLines( IN Tokenizer& scanner )
{
	Token *token;

	while ( true )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( !(token->tokType == Token::CRLF ||
			   token->tokType == Token::WHITESPACE) )
		{
			// not whitespace or crlf; restore and done
			scanner.pushBack();
			return ED_OK;
		}
	}
}

EDERRCODE SkipLWS( IN Tokenizer& scanner, bool* pbMatch )
{
	Token* token;
	bool crlfMatch = true;

	// skip optional CRLF
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::CRLF )
	{
		// not CRLF
		scanner.pushBack();
		crlfMatch = false;
	}

	// match whitespace
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::WHITESPACE )
	{
		// no match
		scanner.pushBack();
    
		// put back crlf as well, if read
		if ( crlfMatch )
			scanner.pushBack();

		if (pbMatch)
			*pbMatch = false;	// input does not match LWS
		return ED_OK;
	}

	if (pbMatch)
		*pbMatch = true;	// match
	return ED_OK;
}

// skips *LWS
EDERRCODE SkipOptionalLWS( IN Tokenizer& scanner )
{
    // skip LWS until no match
	bool bMatch;
	do
	{
		ED_RETURN_EXCEPTION( SkipLWS(scanner, &bMatch) );
	} while(bMatch);

	return ED_OK;
}

static EDERRCODE SkipOptionalWhitespace( IN Tokenizer& scanner )
{
    Token* token;
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::WHITESPACE )
    {
        scanner.pushBack();
    }
	return ED_OK;
}

// reads and discards a header from input stream
static EDERRCODE SkipHeader( IN Tokenizer& scanner )
{
    Token* token;
    
    // skip all lines that are continuation of the header
    while ( true )
    {
        // skip until eol
        do
        {
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
            
            // handle incomplete or bad input
            if ( token->tokType == Token::END_OF_STREAM )
            {
                scanner.pushBack();
				return ED_OK;
            }
        } while ( token->tokType != Token::CRLF );
        
        // header continues or ends?
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
        if ( token->tokType != Token::WHITESPACE )
        {
            // possibly, new header starts here
            scanner.pushBack();
            break;
        }
    }
	return ED_OK;
}

// reads header name from input
// call when scanning start of line
// header ::= headername : value
//
// throws ParseFailException if header identifier not found
static EDERRCODE ParseHeaderName( IN Tokenizer& scanner, OUT xstring& hdrName )
{
	Token *token;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER )
	{
		scanner.pushBack();     // put token back in stream
		// not an identifier
//		throw HttpParseException("ParseHeaderName()", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("ParseHeaderName()", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	hdrName = token->s;
	return ED_OK;
}

// skips the ':' after header name and the all whitespace
//  surrounding it
// precond: cursor pointing after headerName
static EDERRCODE SkipColonLWS( IN Tokenizer& scanner )
{
	Token *token;

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != ':' )
	{
		scanner.pushBack();
//		throw HttpParseException( "SkipColonLWS(): expecting colon", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "SkipColonLWS(): expecting colon", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	return SkipOptionalLWS( scanner ); 
}

// header ::= name : value
// returns value of a header
// called when currToken is pointing to a non-whitespace token
//   after colon
// NOTE: value can be blank; "" (len = 0)
static EDERRCODE ParseHeaderValue( IN Tokenizer& scanner, OUT xstring& value )
{
	Token* token;

	value = "";

	//SkipOptionalLWS( scanner ); // no leading whitespace

	// precond: next token is not whitespace

	while ( true )
	{
		// add all str till eol or end of stream
		while ( true )
		{
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
			if ( token->tokType == Token::END_OF_STREAM )
			{
				scanner.pushBack();
//				throw HttpParseException("ParseHeaderValue(): unexpected end" );
				DEBUG_THROW_HTTP_PARSE_EXCEPTION("ParseHeaderValue(): unexpected end", 0 );
				return EDERR_PARSE_EXCEPTION;
			}
			if ( token->tokType == Token::CRLF )
			{
				break;  // end of value?
			}
			value += token->s;
		}

		ED_RETURN_EXCEPTION(scanner.getToken(&token));
        
		// header continued on new line?
		if ( token->tokType != Token::WHITESPACE )
		{
			// no; header done
			scanner.pushBack();
			scanner.pushBack(); // return CRLF too
			break;
		}
		else
		{
			// header value continued
			value += ' ';
		}
	}

	// precond:
	// value is either empty "", or has one at least one
	//   non-whitespace char; also, no whitespace on left

	if ( value.length() > 0 )
	{
		const char *start_ptr, *eptr;
		int sublen;

		start_ptr = value.c_str();
		eptr = start_ptr + value.length() - 1;  // start at last char
		while ( *eptr == ' ' || *eptr == '\t' )
		{
			eptr--;
		}

		sublen = eptr - start_ptr + 1;

		if ( value.length() != sublen )
		{
			// trim right whitespace
			// TODO: add exception handling
			value.deleteSubstring( eptr - start_ptr + 1, value.length() - sublen );
		}
	}
	else
	{
		value = "";
	}

	return ED_OK;
}

// reads comma separated list header values
// precond:
// scanner points after the header name;
// the colon and any whitespace around it has also been skipped
// minItems: at least this many items should be in list
// maxItems: no more than this many items should be in list
//   -1 means maxItems = infinity
// --------
// adds HttpHeaderValues to the list, 0..infin items could be
//  added
// throws HttpParseException.PARSERR_BAD_LISTCOUNT if
//  item count is not in range [minItems, maxItems]
// throws HttpParseException.PARSERR_BAD_COMMALIST if
//  list does not have a correct comma-separated format
static EDERRCODE ReadCommaSeparatedList( IN Tokenizer& scanner,
    OUT HttpHeaderValueList& list,
    IN int minItems, IN int maxItems,
    IN bool qIsUsed,
    IN CreateNewValueCallback createCallback,
    IN AddValueToListCallback addCallback = NULL )
{
    Token* token;
    HttpHeaderValue* value;
    int itemCount = 0;
	int code = 0;

/*
    try
    {   
*/
		// at this point, all LWS has been skipped;
		// empty list is indicated by a CRLF
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->tokType == Token::CRLF )
		{
			scanner.pushBack();
//			throw -2;       // end of list
			code = -2;	// end of list
			goto CatchError;
		}
    
		scanner.pushBack(); // return item for value to parse

		while ( true )
		{
			// create new item
			value = createCallback();
			if ( value == NULL )
			{
//				throw OutOfMemoryException( "ReadCommaSeparatedList()" );
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("ReadCommaSeparatedList()");
				return EDERR_OUT_OF_MEMORY;
			}
        
			// allow q to be read
			if ( qIsUsed )
			{
				HttpQHeaderValue* qvalue;
            
				qvalue = (HttpQHeaderValue*) value;
				qvalue->qIsUsed = true;
			}
        
			// read list item
//			ED_CATCH_EXCEPTION(value->load( scanner ), CatchError);
			ED_RETURN_EXCEPTION(value->load( scanner ));

			if ( addCallback == NULL )
			{
				list.addAfterTail( value );
/*
				code = list.addAfterTail( value );
				if (ED_FAILED(code))
					goto CatchError;
*/
			}
			else
			{
//				ED_CATCH_EXCEPTION(addCallback( list, value ), CatchError);
				ED_RETURN_EXCEPTION(addCallback( list, value ));
			}
			itemCount++;

			// skip comma and whitespace
			//
        
			ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
    
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
			if ( token->s == "," )
			{
				// ok
			}
			else if ( token->tokType == Token::CRLF )
			{
				scanner.pushBack();
//				throw -2;	// end of list
				code = -2;	// end of list
				goto CatchError;
			}
			else
			{
				scanner.pushBack();
//				throw -3;	// unexpected element
				code = -3;	// unexpected element
				goto CatchError;
			}

			ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
		}

	return ED_OK;
/*
	}
	catch ( int code )
	{
*/
CatchError:
		
		if ( code == -2 )
		{
//			HttpParseException e("ReadCommaSeparatedList():end of list", scanner.getLineNum() );
//			e.setErrorCode( PARSERR_BAD_LISTCOUNT );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("ReadCommaSeparatedList():end of list", scanner.getLineNum());
        
			// end of list
			if ( itemCount < minItems )
			{
//				throw e;    // too few items
//				return PARSERR_BAD_LISTCOUNT;
				return EDERR_PARSE_EXCEPTION;
			}
        
			// maxItems == -1 means no limit
			if ( maxItems >= 0 && itemCount > maxItems )
			{
//				throw e;    // too many items
//				return PARSERR_BAD_LISTCOUNT;
				return EDERR_PARSE_EXCEPTION;
			}
		}
		else if ( code == -3 )
		{
			// unexpected element where comma should've been
//			HttpParseException e("ReadCommaSeparatedList(): comma expected", scanner.getLineNum() );
//			e.setErrorCode( PARSERR_BAD_COMMALIST );
//			throw e;
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("ReadCommaSeparatedList(): comma expected", scanner.getLineNum());
//			return PARSERR_BAD_COMMALIST;
			return EDERR_PARSE_EXCEPTION;
		}
/*
	}
*/
	return ED_OK;
}

////////////////////
// stores integer number 0..INT_MAX from scanner
// returns 0 if successful, PARSERR_BAD_NUMBER if number is
//  invalid or negative; the bad token is put back in the scanner
//
static EDERRCODE loadNum( Tokenizer& scanner, int base, int* pNum )
{
    Token *token;
    char *endptr;
    int num = 0;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));

    errno = 0;  // set this to explicitly avoid bugs
    num = strtol( token->s.c_str(), &endptr, base );

    if ( *endptr != '\0' || num < 0 )
    {
		xstring msg("loadNum(): bad number " );
		msg += token->s;
		scanner.pushBack();
//		throw HttpParseException( msg.c_str(), scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION(msg.c_str(), scanner.getLineNum());
//		return PARSERR_BAD_NUMBER;
		return EDERR_PARSE_EXCEPTION;
    }
    if ( (num == LONG_MIN || num == LONG_MAX) && (errno == ERANGE) )
    {
		xstring msg("loadNum(): out of range '" );
		msg += token->s;
		msg += "'";
		scanner.pushBack();
//		throw HttpParseException( msg.c_str(), scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION(msg.c_str(), scanner.getLineNum());
//		return PARSERR_BAD_NUMBER;
		return EDERR_PARSE_EXCEPTION;
    }
    
	*pNum = num;

    return ED_OK;
}

void NumberToString( int num, xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%d", num );
    s += buf;
}


static EDERRCODE LoadDateTime( Tokenizer& scanner, tm& datetime )
{
	xstring rawValue;
	int numCharsParsed;
	int success;

	ED_RETURN_EXCEPTION(ParseHeaderValue( scanner, rawValue ));

	const char* cstr = rawValue.c_str();

	success = ParseDateTime( cstr, &datetime, &numCharsParsed );

	if ( success == -1 )
	{
//		throw HttpParseException( "LoadDateTime() bad date/time", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadDateTime() bad date/time", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}   
    
	// skip all whitespace after date/time
	const char* s2 = &cstr[numCharsParsed];
	while ( *s2 == ' ' || *s2 == '\t' )
	{
		s2++;
	}

	if ( *s2 != '\0' )
	{
//		throw HttpParseException( "LoadDateTime() bad trailer", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadDateTime() bad trailer", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}   

	return ED_OK;

}


static EDERRCODE LoadUri( IN Tokenizer& scanner, OUT uri_type& uri, OUT xstring& uriStr )
{
	Token *token;

	uriStr = "";
	while ( true )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->tokType == Token::IDENTIFIER ||
			 token->tokType == Token::SEPARATOR ||
			 token->tokType == Token::QUOTED_STRING )
		{
			uriStr += token->s;
		}
		else
		{
			scanner.pushBack();
			break;
		}
	}


	if ( uriStr.length() == 0 )
	{
//		throw HttpParseException( "LoadUri(): no uri", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadUri(): no uri", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	int len;

	len = parse_uri( (char*)(uriStr.c_str()), uriStr.length(), &uri );

	if ( len < 0 )
	{
//		throw HttpParseException( "LoadUri(): bad uri", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadUri(): bad uri", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	return ED_OK;
}

static EDERRCODE ParseMajorMinorNumbers( const char *s,
    int& majorVers, int& minorVers )
{
	int major, minor;
	char* endptr;
	char* s2;
	int code = 0;

//	try
//	{
		errno = 0;
		major = strtol( s, &endptr, 10 );
		if ( (major < 0) ||
			 ((major == LONG_MAX || major == LONG_MIN) && (errno == ERANGE))
			)
		{
//			throw -1;
			code = -1;
			goto CatchError;
		}

		if ( *endptr != '.' )
		{
//			throw -1;
			code = -1;
			goto CatchError;
		}

		s2 = endptr + 1;
		errno = 0;
		minor = strtol( s2, &endptr, 10 );
		if ( (minor < 0) ||
			 (*endptr != 0) ||
			 ((minor == LONG_MAX || minor == LONG_MIN) && (errno == ERANGE))
			)
		{
//			throw -1;
			code = -1;
			goto CatchError;
		}

		majorVers = major;
		minorVers = minor;

	return ED_OK;

CatchError:
//	}
//	catch ( int code )
//	{
		if ( code == -1 )
		{
//			HttpParseException e("ParseMajorMinorNumbers(): bad http version: " );
//			e.appendMessage( s );
//			throw e;
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("ParseMajorMinorNumbers(): bad http version: ", 0 );
			return EDERR_PARSE_EXCEPTION;
		}
//	}
	return ED_OK;
}

static EDERRCODE ParseHttpVersion( IN Tokenizer& scanner,
    OUT int& majorVers, OUT int& minorVers )
{
	Token* token;
//	HttpParseException eVers( "ParseHttpVersion(): bad http version" );

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s.compareNoCase("HTTP") != 0 )
	{
//		throw eVers;
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "ParseHttpVersion(): bad http version", 0 );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(scanner.getToken(&token));

	// skip optional whitespace
	if ( token->tokType == Token::WHITESPACE )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
	}

	if ( token->s != '/' )
	{
//		throw eVers;
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "ParseHttpVersion(): bad http version", 0 );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType == Token::WHITESPACE )
	{
		// skip optional whitespace and read version
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
	}

	return ParseMajorMinorNumbers( token->s.c_str(), majorVers, minorVers );
}

static void PrintHttpVersion( int major, int minor, xstring& s )
{
    s += "HTTP/";
    NumberToString( major, s );
    s += '.';
    NumberToString( minor, s );
}

static void HeaderValueListToString( IN HttpHeaderValueList& list,
    INOUT xstring& s )
{
    HttpHeaderValueNode* node;
    HttpHeaderValue* value;
    
    node = list.getFirstItem();
    for ( int i = 0; i < list.length(); i++ )
    {
        value = (HttpHeaderValue *) node->data;
        value->toString( s );
        node = list.next( node );
    }
}



//static void QToString( float q, INOUT xstring& s )
//{
//  char buf[50];
    
//  sprintf( buf, ";q=%1.3f", q );
//  s += buf;
//}


// match: ( 0 ['.' 0*3DIGIT] ) | ( '1' 0*3('0') )
// returns 0.0 to 1.0 on success;
// throws HttpParseExecption
//static float ParseQValue( const char *s )
static EDERRCODE ParseQValue( const char *s, float* pfValue )
{
	char c;
	int i;
	char dot;
	int count;
	char *endptr;
	float value = 0.0;
	bool partial;
	int code = 0;

	i = 0;
	partial = true;

//	try
//	{
		c = s[i++];
		if ( !(c == '0' || c == '1') )
		{
//			throw -1;
			code = -1;
			goto CatchError;
		}
    
		dot = s[i++];
		if ( dot == '.' )
		{
			if ( c == '1' )
			{
				// match 3 zeros
				for ( count = 0; count < 3; count++ )
				{
					c = s[i++];
					if ( c == '\0' )
					{
						partial = false;
						break;  // done; success
					}
					if ( c != '0' )
					{
//						throw -1;
						code = -1;
						goto CatchError;
					}
				}
			}
			else
			{
				// c == 0, match digits
				for ( count = 0; count < 3; count++ )
				{
					c = s[i++];
					if ( c == 0 )
					{
						partial = false;
						break;
					}
					if ( !(c >= '0' && c <= '9') )
					{
//						throw -1;
						code = -1;
						goto CatchError;
					}
				}
			}
			// all chars must be processed
			if ( partial && s[i] != '\0' )
			{
//				throw -1;
				code = -1;
				goto CatchError;
			}
		}
		else if ( dot != '\0' )
		{
//			throw -1;
			code = -1;
			goto CatchError;
		}
    
		value = (float)strtod( s, &endptr );

	if (pfValue)
		*pfValue = value;
	return ED_OK;

CatchError:

//	}
//	catch ( int code )
//	{
		if ( code == -1 )
		{
			xstring msg("ParseQValue(): ");
			msg += s;
//			throw HttpParseException( msg.c_str() );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION( msg.c_str(), 0 );
			return EDERR_PARSE_EXCEPTION;
		}
//	}

	if (pfValue)
		*pfValue = value;
	return ED_OK;
}

// match: q = floatValue
//static float LoadQValue( Tokenizer& scanner )
static EDERRCODE LoadQValue( Tokenizer& scanner, float* pfValue )
{
	Token* token;
	float q;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != 'q' )
	{
		scanner.pushBack();
//		throw HttpParseException( "LoadQValue(): 'q' expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadQValue(): 'q' expected", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != '=' )
	{
		scanner.pushBack();
//		throw HttpParseException( "LoadQValue(): '=' expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadQValue(): '=' expected", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER )
	{
		scanner.pushBack();
//		throw HttpParseException( "LoadQValue(): number expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadQValue(): number expected", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	// ident to
	ED_RETURN_EXCEPTION(ParseQValue( token->s.c_str(), &q ));
	if ( q < 0 )
	{
//		throw HttpParseException( "LoadQValue(): invalid value", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "LoadQValue(): invalid value", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	if (pfValue)
		*pfValue = q;

	return ED_OK;
}

static void PrintCommaSeparatedList( HttpHeaderValueList& list,
    xstring& s )
{
    bool first = true;
    HttpHeaderValueNode* node;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        if ( !first )
        {
            s += ", ";
        }
        else
        {
            first = false;
        }
        
        HttpHeaderValue* data;
        data = (HttpHeaderValue *) node->data;
        data->toString( s );
        node = list.next( node );
    }
}

// can't include ctype.h because of a xerces bug
static bool IsAlpha( char c )
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// returns true if the tag matches language tag syntax
//  (1*8ALPHA * ("-" 1*8ALPHA)) | *
static bool ValidLanguageTag( const char* tag )
{
    int i;
    char c;
    int count;
    
    if ( strcmp(tag, "*") == 0 )
    {
        return true;
    }
    
    i = 0;
    while ( true )
    {
        // match 1..8 alpha
        for ( count = 0; count < 8; count++ )
        {
            c = tag[i++];
            if ( !IsAlpha(c) )
            {
                if ( count == 0 )
                {
                    return false;   // no alpha found
                }
                i--;    // put back char
                break;  // okay; valid alpha str
            }
        }
        
        c = tag[i++];
        
        if ( c == '\0' )
        {
            return true;    // end of valid tag
        }
        
        if ( c != '-' )
        {
            return false;   // expected '-'
        }
    }
    
    return false;       // this line never executes
}


// parses a header from input stream; returns header type and
//  and value
// on success: headerID > 0  and returnValue != NULL
//             headerID == -1 and returnValue = UnknownHeader
// on failure: returnValue == NULL
//static HttpHeaderValue* ParseHeader( IN Tokenizer& scanner, OUT int& headerID )
static EDERRCODE ParseHeader( IN Tokenizer& scanner, OUT int& headerID, HttpHeaderValue** ppHeaderValue )
{
    int id;
    Token* token;
    xstring name;

    // read header name
    ED_RETURN_EXCEPTION(ParseHeaderName( scanner, name ));

    // map name to id
    id = NameToID( name.c_str(), HeaderNameTable, NUM_HEADERS, false );
    if ( id < 0 )
    {
        // header type currently not known; process raw

        id = HDR_UNKNOWN;

        scanner.pushBack(); // return unknown header name
    }

    // skip past colon;
    if ( id != HDR_UNKNOWN )
    {
        ED_RETURN_EXCEPTION(SkipColonLWS( scanner ));
    }
    // now pointing at first non-whitespace token after colon

    HttpHeaderValue *header = NULL;
	if (ppHeaderValue)
		*ppHeaderValue = NULL;
        
    switch ( id )
    {
        case HDR_ACCEPT:
            header = new CommaSeparatedList( true, CreateMediaRange );
            break;
            
        case HDR_ACCEPT_CHARSET:
        case HDR_ACCEPT_ENCODING:
            header = new CommaSeparatedList( true, CreateIdentifierQValue, 1 );
            break;
            
		/* ------------xxxxxxxxxxxxxx
        case HDR_ACCEPT_LANGUAGE:
            header = new CommaSeparatedList( true, CreateLanguageTag, 1 );
            break;
		  -------------xxxxxxxxxxxxxxxx */

        case HDR_AGE:
        case HDR_CONTENT_LENGTH:
        case HDR_MAX_FORWARDS:
            header = new HttpNumber;
            break;          
            
        case HDR_ALLOW:
        case HDR_CONNECTION:
        case HDR_CONTENT_ENCODING:
        case HDR_TRANSFER_ENCODING:
            header = new CommaSeparatedList( false, CreateIdentifierValue );
            break;
            
		/* -----------xxxxxxxxxxxxxxxxxx
        case HDR_CACHE_CONTROL:
            header = new CommaSeparatedList( false, CreateCacheDirective );
            break;
		   -----------xxxxxxxxxxxxxxxxxx */
            
		/* ------------xxxxxxxxxxxxxxxxx
        case HDR_CONTENT_LANGUAGE:
            header = new CommaSeparatedList( false, CreateLanguageTag );
            break;
		   ------------xxxxxxxxxxxxxxxxx */
            
        case HDR_CONTENT_LOCATION:
        case HDR_LOCATION:
            header = new UriType;
            break;
            
        case HDR_CONTENT_TYPE:
            header = new MediaRange;
            break;
            
        case HDR_DATE:
        case HDR_EXPIRES:
        case HDR_IF_MODIFIED_SINCE:
        case HDR_IF_UNMODIFIED_SINCE:
        case HDR_LAST_MODIFIED:
            header = new HttpDateValue;
            break;
            
		/* ----------xxxxxxxxxxxxxxxxxxxx
        case HDR_HOST:
            header = new HostPortValue;
            break;
		  ----------xxxxxxxxxxxxxxxxxxxx */
        
		/* -----------xxxxxxxxxxxxxxxxx
        case HDR_RETRY_AFTER:
            header = new HttpDateOrSeconds;
            break;
		   -----------xxxxxxxxxxxxxxxxx */
            
		/* ---------------xxxxxxxxxxxx
        case HDR_UPNP_NTS:
            header = new NTSType;
            break;
			--------------xxxxxxxxxxxxxx */

        case HDR_UNKNOWN:
            header = new UnknownHeader;
            break;

        default:
            header = new RawHeaderValue;
    }
    
    if ( header == NULL )
    {
//		throw OutOfMemoryException( "ParseHeader()" );
		DEBUG_THROW_EXCEPTION("OutOfMemoryException");
		return EDERR_OUT_OF_MEMORY;
    }

/*
    try
    {
*/
		int iRetVal = header->load( scanner );
		if (iRetVal == EDERR_PARSE_EXCEPTION)
			goto CatchError;
		else if (ED_FAILED(iRetVal))
			return iRetVal;

        // skip whitespace after value
		iRetVal = SkipOptionalLWS( scanner );
		if (iRetVal == EDERR_PARSE_EXCEPTION)
			goto CatchError;
		else if (ED_FAILED(iRetVal))
			return iRetVal;

        // end of header -- crlf
        iRetVal = scanner.getToken(&token);
		if (iRetVal == EDERR_PARSE_EXCEPTION)
			goto CatchError;
		else if (ED_FAILED(iRetVal))
			return iRetVal;
        if ( token->tokType != Token::CRLF )
        {
//			throw HttpParseException();
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("ParseHeader", 0);
			goto CatchError;
        }

    headerID = id;

	if (ppHeaderValue)
		*ppHeaderValue = header;
    return ED_OK;

CatchError:

/*
    }
    catch ( HttpParseException& )
    {
*/
            SkipHeader( scanner );  // bad header -- ignore it

            id = -2;
            delete header;
            header = NULL;
/*
    }
*/

    headerID = id;
        
	if (ppHeaderValue)
		*ppHeaderValue = header;
	return ED_OK;
}

// precond: scanner pointing to the line after the start line
static EDERRCODE ParseHeaders( IN Tokenizer& scanner,
    OUT HttpHeaderList& list )
{
	Token *token;
	int id;
	HttpHeaderValue* headerValue;
	HttpHeader* header;

	while ( true )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->tokType == Token::CRLF )
		{
			// end of headers
			break;
		}
		else if ( token->tokType == Token::END_OF_STREAM )
		{
			// bad format
			scanner.pushBack();
//			throw HttpParseException( "ParseHeaders(): unexpected end of msg" );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION( "ParseHeaders(): unexpected end of msg", 0 );
			return EDERR_PARSE_EXCEPTION;
		}
		else
		{
			// token was probably start of a header
			scanner.pushBack();
		}

		ED_RETURN_EXCEPTION(ParseHeader( scanner, id, &headerValue ));
		if ( headerValue != NULL )
		{
			header = new HttpHeader;
			if ( header == NULL )
			{
	//				throw OutOfMemoryException( "ParseHeaders()" );
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("ParseHeaders()");
				return EDERR_OUT_OF_MEMORY;
			}

			header->type = id;
			header->value = headerValue;
    
			list.addAfterTail( header );
		}
	}

	return ED_OK;
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

// HttpHeaderQValue
void HttpQHeaderValue::toString( xstring& s )
{
    if ( qIsUsed )
    {
        char buf[100];
        sprintf( buf, ";q=%1.3f", q );
        s += buf;
    }
}

EDERRCODE HttpQHeaderValue::loadOptionalQValue( IN Tokenizer& scanner )
{
	q = 1.0;    // default value

	// try getting q
//	try
//	{
		Token* token;
    
		if ( qIsUsed )
		{
			ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
    
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
			if ( token->s != ';' )
			{
				scanner.pushBack();
				//DBG( printf("HttpQHeaderValue: did not find ;\n"); )
//				throw 1;
				return ED_OK;
			}
			ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
			ED_RETURN_EXCEPTION(LoadQValue( scanner, &q ));
		}
//	}
//	catch ( int /* code */ )
//	{
		// no q value
		//DBG( printf("HttpQHeaderValue: did not find semicolon\n"); )
//	}

	return ED_OK;
}

// HttpNumber
void HttpNumber::toString( xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%d", num );
    s += buf;
}

EDERRCODE HttpNumber::load( Tokenizer& scanner )
{
    return loadNum( scanner, 10, &num );
}

// HttpHexNumber
void HttpHexNumber::toString( xstring& s )
{
    char buf[50];
    
    sprintf( buf, "%X", num );
    s += buf;
}

EDERRCODE HttpHexNumber::load( Tokenizer& scanner )
{
    return loadNum( scanner, 16, &num );
}

// MediaExtension
void MediaExtension::toString( xstring& s )
{
    s += ';';
    s += name;
    s += "=";
    s += value;
}

// match: ext = value
EDERRCODE MediaExtension::load( Tokenizer& scanner )
{
	Token* token;
//	HttpParseException e("MediaExtension::load()", scanner.getLineNum() );
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER )
	{
//		throw e;
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load()", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}
	name = token->s;

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != "=" )
	{
//		throw e;
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load()", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER &&
		 token->tokType != Token::QUOTED_STRING )
	{
//		throw e;
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load()", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}
	value = token->s;

	return ED_OK;
}

// MediaParam
EDERRCODE MediaParam::load( Tokenizer& scanner )
{
	Token* token;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != ';' )
	{
		scanner.pushBack();
//		throw HttpParseException( "MediaParam::load(): ; missing",
//			scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaParam::load(): ; missing", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s == 'q' )
	{
		scanner.pushBack();
		ED_RETURN_EXCEPTION(LoadQValue( scanner, &q ));
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	while ( true )
	{
		MediaExtension* ext;
    
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->s != ';' )
		{
			scanner.pushBack();
			break;  // no more extensions
		}
    
		ext = new MediaExtension();
		if ( ext == NULL )
		{
//			throw OutOfMemoryException( "MediaParam::load()" );
			DEBUG_THROW_EXCEPTION("OutOfMemoryException");
			return EDERR_OUT_OF_MEMORY;
		}

		ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

		ED_RETURN_EXCEPTION(ext->load( scanner ));

		extList.addAfterTail( ext );
	}

	return ED_OK;
}

void MediaParam::toString( xstring& s )
{
    HttpQHeaderValue::toString( s );    // print q
    
    HttpHeaderValue* value;
    HttpHeaderValueNode* node;
    
    node = extList.getFirstItem();
    while ( node != NULL )
    {
        value = (HttpHeaderValue *)node->data;
        value->toString( s );
        
        node = extList.next( node );
    }
}

// MediaRange
void MediaRange::toString( xstring& s )
{
    s += type;
    s += '/';
    s += subtype;

    mparam.toString( s );
}

EDERRCODE MediaRange::load( Tokenizer& scanner )
{
    Token* token;
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::IDENTIFIER )
    {
        scanner.pushBack();
//		throw HttpParseException( "MediaRange::load(): type expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load(): type expected", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }
    type = token->s;

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->s != '/' )
    {
		scanner.pushBack();
//		throw HttpParseException( "MediaRange::load(): / expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load(): / expected", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::IDENTIFIER ||
         (type == '*' && token->s != '*')
        )
    {
		scanner.pushBack();
//		throw HttpParseException( "MediaRange::load(): subtype expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("MediaExtension::load(): subtype expected", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }
    subtype = token->s;

	return mparam.load( scanner );
}

// returns: 0 same, -1 this < r, 1 this > r, -2 unrelated
// returns true if 'this' range has higher precedence than r
int MediaRange::compare( const MediaRange& r ) const
{
    if ( mparam.q > r.mparam.q )
    {
        return 1;
    }
    else if ( mparam.q < r.mparam.q )
    {
        return -1;
    }
    else    // equal
    {
        bool t = false, rt = false;
        
        t = (type == '*');
        rt = (type == '*');
        
        if ( t && !rt ) return -1;
        else if ( !t && rt ) return 1;
        else if ( type == r.type )
        {
            if ( subtype == r.subtype )
            {
                return 0;
            }
            else if ( subtype == '*' && r.subtype != '*' )
            {
                return 1;
            }
            else if ( subtype != '*' && r.subtype == '*' )
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else if ( type != r.type )
        {
            return -2;
        }
    }
    
    return -2;
}

/* ------------------xxxxxxxxxxxxxxxxxxxxxx
// MediaRangeList
void MediaRangeList::toString( xstring& s )
{
    HeaderValueListToString( mediaList, s );
}


// add media range in a sorted manner
static EDERRCODE AddMediaRangeToList( HttpHeaderValueList& list,
    HttpHeaderValue *value )
{
    MediaRange* b = (MediaRange*) value;
    MediaRange* a;
    HttpHeaderValueNode* node;
    int inserted = false;
    int comp = 0;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        a = (MediaRange*) node->data;
        comp = a->compare( *b );
        if ( comp == 1 || comp == 0 )
        {
            list.addBefore( node, b );
            inserted = true;
            break;
        }
        else if ( comp == -1 )
        {
            list.addAfter( node, b );
            inserted = true;
            break;
        }

        node = list.next( node );       
        
    }
    
    if ( !inserted )
    {
        list.addAfterTail( value );
    }

	return ED_OK;
}

EDERRCODE MediaRangeList::load( Tokenizer& scanner )
{
    return ReadCommaSeparatedList( scanner, mediaList,
        0, -1, true, CreateMediaRange, AddMediaRangeToList );
}
 ------------------xxxxxxxxxxxxxxxxx */


// CommaSeparatedList
void CommaSeparatedList::toString( xstring& s )
{
    PrintCommaSeparatedList( valueList, s );
}

// add value sorted list; sorted descending 'q'
static EDERRCODE AddValueToSortedListCallback( HttpHeaderValueList& list,
    HttpHeaderValue *value )
{
    HttpQHeaderValue *a, *b;
    HttpHeaderValueNode* node;
    bool inserted = false;
    
    b = (HttpQHeaderValue *)value;
    
    node = list.getFirstItem();
    while ( node != NULL )
    {
        a = (HttpQHeaderValue*) node->data;
        if ( b->q > a->q )
        {
            list.addBefore( node, b );
            inserted = true;
            break;
        }

        node = list.next( node );       
    }
    
    if ( !inserted )
    {
        list.addAfterTail( value );
    }

	return ED_OK;
}


EDERRCODE CommaSeparatedList::load( Tokenizer& scanner )
{
    AddValueToListCallback addCallback = NULL;
    
    if ( qUsed )
    {
        addCallback = AddValueToSortedListCallback;
    }
    
    return ReadCommaSeparatedList( scanner, valueList,
        minItems, maxItems, qUsed, createCallback, addCallback );
}

// IdentifierValue
void IdentifierValue::toString( xstring& s )
{
    s += value;
}

EDERRCODE IdentifierValue::load( Tokenizer& scanner )
{
    Token* token;
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::IDENTIFIER )
    {
		scanner.pushBack();
//		throw HttpParseException("IdentifierValue::load(): identifier expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("IdentifierValue::load(): identifier expected", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }
    
    value = token->s;

	return ED_OK;
}

// IdentifierQValue
void IdentifierQValue::toString( xstring& s )
{
    s += value;
    HttpQHeaderValue::toString( s );    // print q
}

EDERRCODE IdentifierQValue::load( Tokenizer& scanner )
{
    Token *token;
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::IDENTIFIER )
    {
		scanner.pushBack();
//		throw HttpParseException("IdentifierQValue::load(): identifer expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("IdentifierQValue::load(): identifier expected", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}
    value = token->s;
    
    if ( qIsUsed )
    {
        ED_RETURN_EXCEPTION(loadOptionalQValue( scanner ));
    }

	return ED_OK;
}


/* -----------------xxxxxxxxxxxxxxxxxxxx
// LanguageTag : public HttpHeaderValue
void LanguageTag::toString( xstring& s )
{
    s += lang;
    if ( qIsUsed )
    {
        HttpQHeaderValue::toString( s );
    }
}

EDERRCODE LanguageTag::load( Tokenizer& scanner )
{
    Token* token;
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( !ValidLanguageTag(token->s.c_str()) )
    {
		scanner.pushBack();
		DBG(UpnpPrintf( UPNP_INFO, MSERV, __FILE__, __LINE__, "invalid lang = %s\n", token->s.c_str()); )
//		throw HttpParseException ( "LanguageTag::load() invalid language tag" );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("LanguageTag::load() invalid language tag", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }

    if ( qIsUsed )
    {
		ED_RETURN_EXCEPTION(loadOptionalQValue( scanner ));
    }
    
    lang = token->s;

	return ED_OK;
}

bool LanguageTag::setTag( const char* newLangTag )
{
    if ( !ValidLanguageTag(newLangTag) )
    {
        return false;
    }
    lang = newLangTag;
    return true;
}

 ------------------xxxxxxxxxxxxxx */

 /* -----------------xxxxxxxxxxxxxxxxxxxxx
// CacheDirective
void CacheDirective::toString( xstring& s )
{
    switch ( type )
    {
        case NO_CACHE:
            s += "no-cache";
            break;
            
        case NO_CACHE_FIELDS:
            s += "no-cache=";
            assert( fields.length() > 0 );
            s += fields;
            break;
            
        case NO_STORE:
            s += "no-store";
            break;
            
        case MAX_AGE:
            s += "max-age = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case MAX_STALE:
            s += "max-stale";
            if ( secondsValid )
            {
                s += " = ";
                deltaSeconds.toString( s );
            }
            break;
            
        case MIN_FRESH:
            s += "min-fresh = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case NO_TRANSFORM:
            s += "no-transform";
            break;
            
        case ONLY_IF_CACHED:
            s += "only-if-cached";
            break;
            
        case PUBLIC:
            s += "public";
            break;
            
        case PRIVATE:
            s += "private";
            if ( fields.length() > 0 )
            {
                s += " = ";
                s += fields;
            }
            break;
            
        case MUST_REVALIDATE:
            s += "must-revalidate";
            break;
            
        case PROXY_REVALIDATE:
            s += "proxy-revalidate";
            break;
            
        case S_MAXAGE:
            s += "max age = ";
            assert( secondsValid );
            deltaSeconds.toString( s );
            break;
            
        case EXTENSION:
            s += extensionName;
            s += " = ";
            if ( extType == IDENTIFIER )
            {
                s += extensionValue;
            }
            else if ( extType == QUOTED_STRING )
            {
                s += '"';
                s += extensionValue;
                s += '"';
            }
            else
            {
                assert( 0 );    // unknown type
            }
            break;
        
        case UNKNOWN:
        default:
            assert( 0 );
            break;
    }
}


EDERRCODE CacheDirective::load( Tokenizer& scanner )
{
    Token* token;

    // init
    secondsValid = false;
    fields = "";
    deltaSeconds.num = 0;
    extensionName = "";
    extensionValue = "";


	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->tokType != Token::IDENTIFIER )
    {
		scanner.pushBack();
//		throw HttpParseException( "CacheDirective::load() expected identifier", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("CacheDirective::load() expected identifier", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
    }
    
    if ( token->s == "no-cache" )
    {
        bool fieldsRead;
        
//		fieldsRead = readFields( scanner );
		ED_RETURN_EXCEPTION(readFields( scanner, &fieldsRead ));

        if ( fieldsRead )
        {
            type = NO_CACHE_FIELDS;
        }
        else
        {
            type = NO_CACHE;
        }
	}
	else if ( token->s == "no-store" )
	{
		type = NO_STORE;
	}
	else if ( token->s == "max-age" )
	{
		type = MAX_AGE;
		ED_RETURN_EXCEPTION(readDeltaSeconds( scanner, false ));
	}
	else if ( token->s == "max-stale" )
	{
		type = MAX_STALE;
		ED_RETURN_EXCEPTION(readDeltaSeconds( scanner, true ));
	}
	else if ( token->s == "min-fresh" )
	{
		type = MIN_FRESH;
		ED_RETURN_EXCEPTION(readDeltaSeconds( scanner, false ));
	}
	else if ( token->s == "no-transform" )
	{
		type = NO_TRANSFORM;
	}
	else if ( token->s == "only-if-cached" )
	{
		type = ONLY_IF_CACHED;
	}
	else if ( token->s == "public" )
	{
		type = PUBLIC;
	}
	else if ( token->s == "private" )
	{
		type = PRIVATE;
		ED_RETURN_EXCEPTION(readFields( scanner, 0 ));
	}
	else if ( token->s == "must-revalidate" )
	{
		type = MUST_REVALIDATE;
	}
	else if ( token->s == "proxy-revalidate" )
	{
		type = PROXY_REVALIDATE;
	}
	else if ( token->s == "s-maxage" )
	{
		type = S_MAXAGE;
		ED_RETURN_EXCEPTION(readDeltaSeconds( scanner, false ));
	}
	else
	{
		type = EXTENSION;
		extensionName = token->s;
		ED_RETURN_EXCEPTION(readExtension( scanner ));
	}

	return ED_OK;
}

// returns true if any fields were read
EDERRCODE CacheDirective::readFields( Tokenizer& scanner, bool* pbFieldsRead )
{
	Token* token;

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s == '=' )
	{
		// match: '=' 1#fields
        
		ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->tokType != Token::QUOTED_STRING )
		{
			scanner.pushBack();
//			throw HttpParseException("CacheDirective::load() expected quoted fields", scanner.getLineNum() );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("CacheDirective::load() expected quoted fields", scanner.getLineNum());
			return EDERR_PARSE_EXCEPTION;
		}

		fields = token->s;

//		return true;
		if (pbFieldsRead)
			*pbFieldsRead = true;
		return ED_OK;

	}
	else
	{
		scanner.pushBack();
//		return false;
		if (pbFieldsRead)
			*pbFieldsRead = false;
		return ED_OK;
	}
}

EDERRCODE CacheDirective::readDeltaSeconds( Tokenizer& scanner, bool optional )
{
    Token* token;
    
	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
    if ( token->s == '=' )
    {
		ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
        ED_RETURN_EXCEPTION(deltaSeconds.load( scanner ));
        secondsValid = true;
    }
    else
    {
        scanner.pushBack();
        if ( !optional )
        {
//			throw HttpParseException("CacheDirective::readDeltaSeconds() expected =", scanner.getLineNum() );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION("CacheDirective::readDeltaSeconds() expected =", scanner.getLineNum());
			return EDERR_PARSE_EXCEPTION;
        }
    }

	return ED_OK;
}

EDERRCODE CacheDirective::readExtension( Tokenizer& scanner )
{
	Token* token;

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s != '=' )
	{
		scanner.pushBack();
//		throw HttpParseException("CacheDirective::readExtension() expected =", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("CacheDirective::readExtension() expected =", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(SkipOptionalLWS( scanner ));
    
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType == Token::IDENTIFIER )
	{
		extType = IDENTIFIER;
		extensionValue = token->s;
	}
	else if ( token->tokType == Token::QUOTED_STRING )
	{
		extType = QUOTED_STRING;
		extensionValue = token->s;
	}
	else
	{
		scanner.pushBack();
//		throw HttpParseException("CacheDirective::readExtension() expected ident or quotedstring", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION("CacheDirective::readExtension() expected ident or quotedstring", scanner.getLineNum());
		return EDERR_PARSE_EXCEPTION;
	}

	return ED_OK;
}

  ----------------xxxxxxxxxxxxxxxxxxxxxxxxxx */

//////////////////////////////////
// HttpDateValue
EDERRCODE HttpDateValue::load( Tokenizer& scanner )
{
    return LoadDateTime( scanner, gmtDateTime );
}

void HttpDateValue::toString( xstring& s )
{
    char *str;
    
    str = DateToString( &gmtDateTime );
    s += str;
    free( str );
}


/* -------------xxxxxxxxxxxxxxxxxxxxxxxx
//////////////////////////////////
// HttpDateOrSeconds
EDERRCODE HttpDateOrSeconds::load( Tokenizer& scanner )
{

	// is it seconds?
	if (ED_FAILED(loadNum(scanner, 10, &seconds)))
	{
		// then date/time maybe?
		return LoadDateTime( scanner, gmtDateTime );
	}

	return ED_OK;
}

void HttpDateOrSeconds::toString( xstring& s )
{
    if ( isSeconds )
    {
        NumberToString( seconds, s );
    }   
    else
    {
        char *str;
        str = DateToString( &gmtDateTime );
        s += str;
        free( str );
    }
}
  ----------------xxxxxxxxxxxxxxxxxx */

/* ----------------xxxxxxxxxxxxxxxxxxxxx
//////////////////////////////////
// HostPortValue
EDERRCODE HostPortValue::load( Tokenizer& scanner )
{
	Token* token;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER )
	{
		scanner.pushBack();
//		throw HttpParseException( "HostPortValue::load()", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HostPortValue::load()", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	xstring hport = token->s;
	int len;

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s == ':' )
	{
		hport += ':';
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
		if ( token->tokType != Token::IDENTIFIER )
		{
			scanner.pushBack();
//			throw HttpParseException( "HostPortValue::load()", scanner.getLineNum() );
			DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HostPortValue::load()", scanner.getLineNum() );
			return EDERR_PARSE_EXCEPTION;
		}
		hport += token->s;
	}
	else
	{
		scanner.pushBack();
	}

	len = parse_hostport( (char *)(hport.c_str()), hport.length(), &hostport );
	if ( len < 0 )
	{
//		throw HttpParseException( "HostPortValue::load()", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HostPortValue::load()", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}
	tempBuf = hport;

	return ED_OK;
}

void HostPortValue::toString( xstring& s )
{
    s += tempBuf;
}

bool HostPortValue::setHostPort( const char* hostName, unsigned short port )
{
    xstring s;
    char buf[50];
    int len;
    hostport_type hport;
    
    s = hostName;
    sprintf( buf, ":%d", port );
    s += buf;
    
    len = parse_hostport( (char *)(s.c_str()), s.length(), &hport );
    if ( len < 0 )
    {
        return false;
    }

    tempBuf = s;

    hostport = hport;
    return true;
}

void HostPortValue::getHostPort( sockaddr_in* addr )
{
    *addr = hostport.IPv4address;
}

  --------------------xxxxxxxxxxxxxxxxxxxxxx */

///////////
// UriType
EDERRCODE UriType::load( Tokenizer& scanner )
{
    // uri is composed of identifiers and separators
    return LoadUri( scanner, uri, tempBuf );
}


void UriType::toString( xstring& s )
{
    s += tempBuf;
}

bool UriType::setUri( const char* newUriValue )
{
    xstring uriStr;
    int len;
    uri_type temp;
    
    if ( newUriValue == NULL )
    {
        return false;
    }
    
    uriStr = newUriValue;
    
    len = parse_uri( (char*)(uriStr.c_str()), uriStr.length(),
        &temp );
    if ( len < 0 )
    {
        return false;
    }
    
    uri = temp;

    tempBuf = uriStr;

    return true;
}

const char* UriType::getUri() const
{
    return tempBuf.c_str();
}

int UriType::getIPAddress( OUT sockaddr_in& address )
{
    if ( uri.type != ABSOLUTE )
    {
        return -1;
    }

    memcpy( &address, &uri.hostport.IPv4address, sizeof(sockaddr_in) );
    return 0;
}

/* -------------xxxxxxxxxxxxxxxxxxxxxx
/////////////////
// NTSType
void NTSType::load( Tokenizer& scanner )
{
    xstring strVal;

    ParseHeaderValue( scanner, strVal );

    if ( strVal == "upnp:propchange" )
        value = UPNP_PROPCHANGE;
    else if ( strVal == "ssdp:alive" )
        value = SSDP_ALIVE;
    else if ( strVal == "ssdp:byebye" )
        value = SSDP_BYEBYE;
    else
    {
        DBG( printf("unknown nts: %s\n", strVal.c_str()); )
        throw HttpParseException( "NTSType::load() unknown NTS",
            scanner.getLineNum() );
    }
        
}

void NTSType::toString( xstring& s )
{
    if ( value == UPNP_PROPCHANGE )
        s += "upnp:propchange";
    else if ( value == SSDP_ALIVE )
        s += "ssdp:alive";
    else if ( value == SSDP_BYEBYE )
        s += "ssdp:byebye";
    else
        assert( 0 );    // invalid NTS
}
 -------------xxxxxxxxxxxxxxxxxx */

///////////////////////////////
// RawHeaderValue
EDERRCODE RawHeaderValue::load( Tokenizer& scanner )
{
    return ParseHeaderValue( scanner, value );     
}

void RawHeaderValue::toString( xstring& s )
{
    s += value;
}

////////////////////////////
// UnknownHeader
EDERRCODE UnknownHeader::load( Tokenizer& scanner )
{
    ED_RETURN_EXCEPTION(ParseHeaderName( scanner, name ));
    ED_RETURN_EXCEPTION(SkipColonLWS( scanner ));
    return ParseHeaderValue( scanner, value );
}

void UnknownHeader::toString( xstring& s )
{
    s += name;
    s += ": ";
    s += value;
}

//////////////////////////////
// HttpHeader
HttpHeader::HttpHeader()
{
    value = NULL;
}

HttpHeader::~HttpHeader()
{
    delete value;
}

void HttpHeader::toString( xstring& s )
{
    if ( type != HDR_UNKNOWN )
    {
        const char *name = IDToName( type, HeaderNameTable, NUM_HEADERS );
        s += name;
        s += ": ";
    }
    value->toString( s );
    s += "\r\n";
}

void FreeHttpHeaderNode( void* item )
{
    delete (HttpHeader*)item;
}

void FreeHttpHeaderValueNode( void* item )
{
    delete (HttpHeaderValue*)item;
}

////////////////////////////
// HttpRequestLine

#define NUM_METHODS 8

static SortedTableEntry HttpMethodTable[NUM_METHODS] =
{
    { "GET",        HTTP_GET },
    { "HEAD",       HTTP_HEAD },
    { "M-POST",     UPNP_MPOST },
    { "M-SEARCH",   UPNP_MSEARCH },
    { "NOTIFY",     UPNP_NOTIFY },
    { "POST",       UPNP_POST },
    { "SUBSCRIBE",  UPNP_SUBSCRIBE },
    { "UNSUBSCRIBE",UPNP_UNSUBSCRIBE },
};

EDERRCODE HttpRequestLine::load( Tokenizer& scanner )
{
	Token* token;

	ED_RETURN_EXCEPTION(SkipBlankLines( scanner ));
    
	// method **********
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::IDENTIFIER )
	{
		scanner.pushBack();
//		throw HttpParseException( "HttpRequestLine::load() ident expected", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpRequestLine::load() ident expected", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	method = (UpnpMethodType) NameToID( token->s.c_str(), HttpMethodTable, NUM_METHODS );
	if ( method == -1 )
	{
//		HttpParseException e( "HttpRequestLine::load() unknown method",	scanner.getLineNum() );
//		e.setErrorCode( PARSERR_UNKNOWN_METHOD );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpRequestLine::load() unknown method",	scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::WHITESPACE )
	{
		scanner.pushBack();
//		throw HttpParseException( "HttpRequestLine::load() space 1", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpRequestLine::load() space 1", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}   

	// url ***********  
	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->s == '*' )
	{
		pathIsStar = true;  
	}
	else
	{
		// get url
		scanner.pushBack();
		ED_RETURN_EXCEPTION(uri.load( scanner ));
		pathIsStar = false;
	}

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::WHITESPACE )
	{
		scanner.pushBack();
//		throw HttpParseException( "HttpRequestLine::load() space 2", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpRequestLine::load() space 2", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	// http version ***********
	ED_RETURN_EXCEPTION(ParseHttpVersion( scanner, majorVersion, minorVersion ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType == Token::WHITESPACE )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
	}

	if ( token->tokType != Token::CRLF )
	{
		scanner.pushBack();
//		throw HttpParseException( "RequestLine::load() bad data after vers", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "RequestLine::load() bad data after vers", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	return ED_OK;
}

void HttpRequestLine::toString( xstring& s )
{
    const char* methodStr = IDToName( method, HttpMethodTable,
        NUM_METHODS );
        
    assert( methodStr != NULL );
    
    s += methodStr;
    s += ' ';
    
    // print uri
    if ( pathIsStar )
    {
        s += '*';
    }
    else
    {
        uri.toString( s );
    }

    s += ' ';

    // print vers
    PrintHttpVersion( majorVersion, minorVersion, s );
    
    s += "\r\n";
}

/////////////////////////////////
// HttpResponseLine


int HttpResponseLine::setValue( int statCode, int majorVers,
    int minorVers )
{
    int retVal;
    
    statusCode = statCode;
    majorVersion = majorVers;
    minorVersion = minorVers;
    
    const char *description = http_GetCodeText( statusCode );
    if ( description == NULL )
    {
        reason = "";
        retVal = -1;
    }
    else
    {
        reason = description;
        retVal = 0;
    }
    
    return retVal;
}

EDERRCODE HttpResponseLine::load( Tokenizer& scanner )
{
	Token* token;

	ED_RETURN_EXCEPTION(SkipBlankLines( scanner ));

	ED_RETURN_EXCEPTION(ParseHttpVersion( scanner, majorVersion, minorVersion ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::WHITESPACE )
	{
		scanner.pushBack();
//		throw HttpParseException( "HttpResponseLine::load() space 1", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpResponseLine::load() space 1", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	ED_RETURN_EXCEPTION(loadNum( scanner, 10, &statusCode ));

	ED_RETURN_EXCEPTION(scanner.getToken(&token));
	if ( token->tokType != Token::WHITESPACE )
	{
		scanner.pushBack();
//		throw HttpParseException( "HttpResponseLine::load() space 2", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpResponseLine::load() space 2", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}

	// reason phrase
    
	reason = "";
	while ( true )
	{
		ED_RETURN_EXCEPTION(scanner.getToken(&token));
    
		if (token->tokType == Token::IDENTIFIER ||
			token->tokType == Token::SEPARATOR ||
			token->tokType == Token::WHITESPACE
			)
		{
			reason += token->s;
		}
		else
		{
			break;
		}
	}

	if ( token->tokType != Token::CRLF )
	{
//		throw HttpParseException( "HttpResponseLine::load() no crlf", scanner.getLineNum() );
		DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpResponseLine::load() no crlf", scanner.getLineNum() );
		return EDERR_PARSE_EXCEPTION;
	}
	return ED_OK;
}

void HttpResponseLine::toString( xstring& s )
{
    PrintHttpVersion( majorVersion, minorVersion, s );

    s += ' ';
    NumberToString( statusCode, s );
    s += ' ';
    s += reason;
    s += "\r\n";
}



///////////////////////
// HttpEntity
HttpEntity::HttpEntity()
{
    init();
    type = TEXT;
}

HttpEntity::HttpEntity( const char* file_name )
{
    init();
    type = FILENAME;
    filename = file_name;
}

void HttpEntity::init()
{
    type = TEXT;
    entity = NULL;
    entitySize = 0;
    appendState = IDLE;
    sizeInc = 20;
    allocLen = 0;
}

HttpEntity::~HttpEntity()
{
    if ( appendState == APPENDING )
        appendDone();
        
    if ( type == TEXT && entity != NULL )
        free( entity );
}


HttpEntity::EntityType HttpEntity::getType() const
{
    return type;
}

const char* HttpEntity::getFileName() const
{
    return filename.c_str();
}

// throws FileNotFoundException, OutOfMemoryException
EDERRCODE HttpEntity::appendInit()
{
	assert( appendState == IDLE );

	if ( type == FILENAME )
	{
#ifdef USE_FILESYSTEM
		fp = fopen( filename.c_str(), "wb" );
		if ( fp == NULL )
		{
			throw FileNotFoundException( filename.c_str() );
		}
#endif
		diag_printf("***** REQUEST FOR FILE! *****\n");
		return EDERR_OUT_OF_MEMORY;	// fake a bad return value
	}
	else if ( type == TEXT )
	{
		entity = (char *)malloc( 1 );   // one char for null terminator
		if ( entity == NULL )
		{
//			throw OutOfMemoryException( "HttpEntity::appendInit()" );
			DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpEntity::appendInit()");
			return EDERR_OUT_OF_MEMORY;
		}
	}

	appendState = APPENDING;

	return ED_OK;
}

void HttpEntity::appendDone()
{
    assert( appendState == APPENDING );
    
    if ( type == FILENAME )
    {
        fclose( fp );
    }
    else if ( type == TEXT )
    {
        if ( entity != NULL )
        {
            free( entity );
        }
        entity = NULL;
        entitySize = 0;
        allocLen = 0;
    }
    
    appendState = DONE;
}

// return codes:
// 0 : success
// -1: std error; check errno
// -2: not enough memory
// -3: file write error
int HttpEntity::append( const char* data, unsigned datalen )
{
	int code = 0;
//	try
//	{
		if ( appendState == IDLE )
		{
			code = appendInit();
			if (ED_FAILED(code))
				goto CatchError;
		}

		if ( type == FILENAME )
		{
			size_t numWritten;
        
			numWritten = fwrite( data, datalen, sizeof(char), fp );
			if ( numWritten != sizeof(char) )
			{
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -3;   // file write error
				code = -3;   // file write error
				goto CatchError;
			}
		}
		else if ( type == TEXT )
		{
			code = increaseSizeBy( datalen );
			if (ED_FAILED(code))
				goto CatchError;
        
			memcpy( &entity[entitySize], data, datalen );
			entitySize += datalen;
			entity[entitySize] = 0; // null terminate
		}
//	}
	return 0;

CatchError:

	switch (code)
	{
		case EDERR_FILE_NOT_FOUND_EXCEPTION:
		{
			return -1;
			break;
		}

		case EDERR_OUT_OF_MEMORY:
		{
			return -1;
			break;
		}

		default:
		{
			return code;
		}
	}
}

int HttpEntity::getEntityLen() const
{
    return entitySize;
}

const void* HttpEntity::getEntity()
{
    if ( type == TEXT || type == TEXT_PTR )
    {
        return entity;
    }
    else
    {
        return NULL;
    }
}

unsigned HttpEntity::getSizeIncrement() const
{
    return sizeInc;
}

void HttpEntity::setSizeIncrement( unsigned incSize )
{
    sizeInc = incSize;
}

void* HttpEntity::detachTextEntity()
{
    void *entityBuffer;

    assert( type == TEXT );

    entityBuffer = entity;
    init();     // clean slate
    return entityBuffer;
}

void HttpEntity::attachTextEntity( const void* textEntity, int entityLen )
{
    if ( appendState == APPENDING )
    {
        appendDone();
    }

    entity = (char *)textEntity;
    type = TEXT;
    entitySize = entityLen;
    allocLen = entityLen;
    appendState = APPENDING;
}

void HttpEntity::setTextPtrEntity( const void* textEntity,
    int entityLen )
{
	if ( appendState == APPENDING )
	{
		appendDone();
	}

	entity = (char *)textEntity;
	type = TEXT_PTR;
	entitySize = entityLen;
	allocLen = entityLen;
	appendState = IDLE;
}

EDERRCODE HttpEntity::increaseSizeBy( unsigned sizeDelta )
{
	unsigned inc;
	char *temp;

	if ( allocLen >= (int)(entitySize + sizeDelta) )
		return ED_OK;
    
	inc = MaxVal( sizeInc, sizeDelta );
	temp = (char *) realloc( entity, allocLen + inc + 1 );
	if ( temp == NULL )
	{
		// try smaller size
		if ( sizeInc > sizeDelta )
		{
			inc = sizeDelta;
			temp = (char *)realloc( entity, allocLen + inc + 1 );
		}
    
		if ( temp == NULL )
		{
//			throw OutOfMemoryException( "HttpEntity::increaseSizeBy()" );
			DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpEntity::increaseSizeBy()");
			return EDERR_OUT_OF_MEMORY;
		}
	}   

	entity = temp;
	allocLen += inc;

	//DBG( printf( "entitySize = %d, incDelta = %u, allocLen = %d\n",entitySize, sizeDelta, allocLen ); )
	assert( (int)(entitySize + sizeDelta) <= allocLen );

	return ED_OK;
}



/////////////////////////////
// HttpMessage

EDERRCODE HttpMessage::loadRequest( Tokenizer& scanner, CharReader* reader )
{
    isRequest = true;
    ED_RETURN_EXCEPTION(requestLine.load( scanner ));
    return loadRestOfMessage( scanner, reader, HTTP_UNKNOWN_METHOD );
}

EDERRCODE HttpMessage::loadResponse( Tokenizer& scanner, CharReader* reader,
    UpnpMethodType requestMethod )
{
    isRequest = false;
    ED_RETURN_EXCEPTION(responseLine.load( scanner ));
    return loadRestOfMessage( scanner, reader, requestMethod );    
}

int HttpMessage::loadRequest( const char* request )
{
	int code = -1;

//	try
//	{
		MemReader reader( request );
		Tokenizer scanner( reader );
		switch (loadRequest( scanner, &reader ))
		{
			case EDERR_PARSE_EXCEPTION:
				code = HTTP_E_BAD_MSG_FORMAT;
				break;

			case EDERR_OUT_OF_MEMORY:
				code = HTTP_E_OUT_OF_MEMORY;
				break;

			case EDERR_NET_EXCEPTION:
				code = -1;

			default:
				code = 0;
				break;
		}
/*
	}
	catch ( HttpParseException& )
	{
		code = HTTP_E_BAD_MSG_FORMAT;
	}
	catch ( OutOfMemoryException& )
	{
		code = HTTP_E_OUT_OF_MEMORY;
	}
	catch ( TimeoutException& )
	{
		code = HTTP_E_TIMEDOUT;
	}
	catch ( NetException& )
	{
		code = -1;
	}
*/

	return code;
}

int HttpMessage::loadResponse( const char* response, UpnpMethodType requestMethod )
{
	int code = -1;

//	try
//	{
		MemReader reader( response );
		Tokenizer scanner( reader );
		loadResponse( scanner, &reader, requestMethod );
		switch (loadResponse( scanner, &reader, requestMethod ))
		{
			case EDERR_PARSE_EXCEPTION:
				code = HTTP_E_BAD_MSG_FORMAT;
				break;

			case EDERR_OUT_OF_MEMORY:
				code = HTTP_E_OUT_OF_MEMORY;
				break;

			case EDERR_NET_EXCEPTION:
				code = -1;

			default:
				code = 0;
				break;
		}
/*
	}
	catch ( HttpParseException& )
	{
		code = HTTP_E_BAD_MSG_FORMAT;
	}
	catch ( OutOfMemoryException& )
	{
		code = HTTP_E_OUT_OF_MEMORY;
	}
	catch ( TimeoutException& )
	{
		code = HTTP_E_TIMEDOUT;
	}
	catch ( NetException& )
	{
		code = -1;
	}
*/

	return code;
}

    
int HttpMessage::headerCount() const
{
    return headers.length();
}

EDERRCODE HttpMessage::addHeader( int headerType, HttpHeaderValue* value )
{
	HttpHeader *header;

	header = new HttpHeader;
	if ( header == NULL )
	{
//		throw OutOfMemoryException( "HttpMessage::addHeader()" );
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addHeader()");
		return EDERR_OUT_OF_MEMORY;
	}

	header->type = headerType;
	header->value = value;
	headers.addAfterTail( header );

	return ED_OK;
}

void HttpMessage::deleteHeader( HttpHeaderNode* headerNode )
{
    headers.remove( headerNode );
}

HttpHeaderValue* HttpMessage::getHeaderValue( int headerType )
{
    HttpHeaderNode* node;
    HttpHeaderValue* value = NULL;
    HttpHeader* header;
    
    node = findHeader( headerType );
    if ( node == NULL )
        return NULL;
    else
    {
        header = (HttpHeader *)node->data;
        value = header->value;
    }
    
    return value;
}   

HttpHeaderNode* HttpMessage::findHeader( int headerType )
{
    HttpHeaderNode* node;
    
    node = getFirstHeader();
    while ( node != NULL )
    {
        HttpHeader *header;

        header = (HttpHeader *)node->data;
        if ( header->type == headerType )
        {
            return node;
        }
        node = getNextHeader( node );
    }
    
    return NULL;
}

HttpHeaderNode* HttpMessage::getFirstHeader()
{
    return headers.getFirstItem();
}

HttpHeaderNode* HttpMessage::getNextHeader( HttpHeaderNode* headerNode )
{
    return headers.next( headerNode );
}

void HttpMessage::startLineAndHeadersToString( xstring& s )
{
    if ( isRequest )
    {
        requestLine.toString( s );
    }
    else
    {
        responseLine.toString( s );
    }
    
    HttpHeaderNode* node;
    
    node = getFirstHeader();
    while ( node != NULL )
    {
        HttpHeader *header;

        header = (HttpHeader *)node->data;
        header->toString( s );
        node = getNextHeader( node );
    }
    
    s += "\r\n";
}

// throws OutOfMemoryException
EDERRCODE HttpMessage::addRawHeader( int headerType, const char* value )
{
	RawHeaderValue* rawValue = new RawHeaderValue;
	if ( rawValue == NULL )
	{
//		throw OutOfMemoryException( "HttpMessage::addRawHeader()" );
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addRawHeader()");
		return EDERR_OUT_OF_MEMORY;
	}

	rawValue->value = value;

	return addHeader( headerType, rawValue );
}
    
// throws OutOfMemoryException
EDERRCODE HttpMessage::addContentTypeHeader( const char* type, const char* subtype )
{
    MediaRange *contentType = new MediaRange;
    if ( contentType == NULL )
    {
//		throw OutOfMemoryException( "HttpMessage::addContentTypeHeader()" );
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addContentTypeHeader()");
		return EDERR_OUT_OF_MEMORY;
    }
    
    contentType->type = type;
    contentType->subtype = subtype;
    contentType->mparam.qIsUsed = false;
    
    return addHeader( HDR_CONTENT_TYPE, contentType );
}


// throws OutOfMemoryException
EDERRCODE HttpMessage::addServerHeader()
{
    return addRawHeader( HDR_SERVER, gServerDesc );
}

// throws OutOfMemoryException
EDERRCODE HttpMessage::addUserAgentHeader()
{
    return addRawHeader( HDR_USER_AGENT, gUserAgentDesc );
}


EDERRCODE HttpMessage::addDateTypeHeader( int headerID, const time_t& t )
{
	HttpDateValue* value = new HttpDateValue;
	if ( value == NULL )
	{
//		throw OutOfMemoryException("HttpMessage::addDateTypeHeader()");
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addDateTypeHeader()");
		return EDERR_OUT_OF_MEMORY;
	}

	struct tm *mod_ptr;

	mod_ptr = gmtime( &t );
	memcpy( &value->gmtDateTime, mod_ptr, sizeof(struct tm) );

	return addHeader( headerID, value );
}

EDERRCODE HttpMessage::addNumTypeHeader( int headerID, int num )
{
	HttpNumber* number = new HttpNumber;
	if ( number == NULL )
	{
//		throw OutOfMemoryException( "HttpMessage::addNumTypeHeader()" );
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addNumTypeHeader()");
		return EDERR_OUT_OF_MEMORY;
	}

	number->num = num;
	return addHeader( headerID, number );
}

EDERRCODE HttpMessage::addIdentListHeader( int headerID,
    const char* idents[], int numIdents )
{
	CommaSeparatedList *identList = new CommaSeparatedList( false, NULL );
	if ( identList == NULL )
	{
//		throw OutOfMemoryException( "HttpMessage::addIdentListHeader()" );
		DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addIdentListHeader()");
		return EDERR_OUT_OF_MEMORY;
	}

/*
	try
	{
*/
		for ( int i = 0; i < numIdents; i++ )
		{
			IdentifierValue* value;

			value = new IdentifierValue;
			if ( value == NULL )
			{
//				throw OutOfMemoryException( "HttpMessage::addIdentListHeader()" );
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION("HttpMessage::addIdentListHeader()");
				delete identList;
				return EDERR_OUT_OF_MEMORY;
			}

			value->value = idents[i];

			identList->valueList.addAfterTail( value );
		}
		int iRetVal = addHeader( headerID, identList );
		if (iRetVal == EDERR_OUT_OF_MEMORY)
			delete identList;
		return iRetVal;
/*
	}
	catch ( OutOfMemoryException& )
	{
		delete identList;
		throw;
	}
*/
}


EDERRCODE HttpMessage::loadRestOfMessage( Tokenizer& scanner,
    CharReader* reader, UpnpMethodType requestMethod )
{
	int length;
	int bodyType;
	int code;

	ED_RETURN_EXCEPTION(ParseHeaders( scanner, headers ));

	// determine length if possible
	bodyType = messageBodyLen( requestMethod, length );

	if ( bodyType == -1 )
	{
		// no body
		entity.type = HttpEntity::EMPTY;
	}
	else if ( bodyType == 1 )
	{
		// transfer encoding
		ED_RETURN_EXCEPTION(readEncodedEntity( scanner ));
	}
	else if ( bodyType == 2 )
	{
		// use content length
		code = readEntityUsingLength( scanner, length );
		if ( code < 0 )
		{
//			HttpParseException e( "HttpMessage::loadRestOfMessage(): incomplete body or error reading data\n" );
//			e.setErrorCode( PARSERR_INCOMPLETE_ENTITY );
//			throw e;
			DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpMessage::loadRestOfMessage(): incomplete body or error reading data\n", 0 );
			return EDERR_PARSE_EXCEPTION;
		}
	}
	else if ( bodyType == 3 )
	{
		// use connection close (or stream ending)
		readEntityUntilClose( scanner );
	}
	else
	{
		assert( 0 );    // internal error -- unknown body type
	}
	return ED_OK;
}

// if message is response, requestMethod is method used for request
//   else ignored
// return value:
//              = -1, no body or ignore body
//              =  1, transfer encoding
//              =  2, content-length (length=content length)
//              =  3, connection close
int HttpMessage::messageBodyLen( IN UpnpMethodType requestMethod,
    OUT int& length )
{
    int responseCode = -1;
    int readMethod = -1;
    
    length = -1;

#ifdef USE_EXCEPTIONS    
    try
    {
#endif
        if ( !isRequest )
            responseCode = responseLine.statusCode;

        // std http rules for determining content length
    
        // * no body for 1xx, 204, 304 and head
        //    get, subscribe, unsubscribe
        if ( isRequest )
        {
            if ( requestLine.method == HTTP_HEAD ||
                 requestLine.method == HTTP_GET ||
                 requestLine.method == UPNP_SUBSCRIBE ||
                 requestLine.method == UPNP_UNSUBSCRIBE ||
                 requestLine.method == UPNP_MSEARCH
                )
            {
#ifdef USE_EXCEPTIONS    
                throw -1;
#else
				return -1;
#endif
            }
        }
        else    // response
        {
            if ( responseCode == 204 ||
                 responseCode == 304 ||
                (responseCode >= 100 && responseCode <= 199)
                )
            {
#ifdef USE_EXCEPTIONS    
                throw -1;
#else
				return -1;
#endif
            }
        
            // request was HEAD
            if ( requestMethod == HTTP_HEAD )
#ifdef USE_EXCEPTIONS    
                throw -1;
#else
				return -1;
#endif
        }
    
        // * transfer-encoding -- used to indicate chunked data
        HttpHeaderNode* node;
        node = findHeader( HDR_TRANSFER_ENCODING );
        if ( node != NULL )
        {
            // read method to use transfer encoding
#ifdef USE_EXCEPTIONS    
            throw 1;
#else
			return 1;
#endif
        }

        // * content length present?
        length = getContentLength();
        if ( length >= 0 )
        {
#ifdef USE_EXCEPTIONS    
            throw 2;    // content-length
#else
			return 2;
#endif
        }
    
        // * multi-part/byteranges not supported (yet)
    
        // * length determined by connection closing
        if ( isRequest )
        {
#ifdef USE_EXCEPTIONS    
            throw -1;   // invalid for requests
#else
			return -1;
#endif
        }
    
        // read method = connection closing
		return 3;
#ifdef USE_EXCEPTIONS
    }
    catch ( int code )
    {
        readMethod = code;
    }
#endif
    
    return readMethod;
}

// returns -1 if length not found
int HttpMessage::getContentLength()
{
    HttpNumber* lengthHdr;
    
    lengthHdr = (HttpNumber*) getHeaderValue( HDR_CONTENT_LENGTH );
    if ( lengthHdr == NULL )
        return -1;
    else
        return lengthHdr->num;
}

void HttpMessage::readEntityUntilClose( Tokenizer& scanner )
{
    const int BUFSIZE = 2 * 1024;
    int numRead;
    char buf[BUFSIZE];
    
    while ( !scanner.endOfData() )
    {
        numRead = scanner.read( buf, BUFSIZE );
        if ( numRead == -1 )
        {
            // abort reading -- we now have a partial entity
            break;
        }
        entity.append( buf, numRead );
    }
}

// returns -1 on error
// 0 on success
int HttpMessage::readEntityUsingLength( Tokenizer& scanner,
    int length )
{
    const int BUFSIZE = 2 * 1024;
    int numRead;
    int numLeft = length;
    int readLen;
    char buf[BUFSIZE + 1];  // extra byte for null terminator
        
    while ( !scanner.endOfData() && numLeft > 0 )
    {
        readLen = MinVal( BUFSIZE, numLeft );

        numRead = scanner.read( buf, readLen );
        if ( numRead < 0 )
        {
            // abort reading -- we now have a partial entity
            return -1;
        }
        entity.append( buf, numRead );
        numLeft -= numRead;
    }

    if ( numLeft != 0 )
    {
        return -1;      // partial entity
    }

    return 0;   
}

EDERRCODE HttpMessage::readEncodedEntity( Tokenizer& scanner )
{
	int chunkSize;
	Token* token;
	int code = 0;

//	try
//	{
		while ( true )
		{
			ED_RETURN_EXCEPTION(loadNum( scanner, 16, &chunkSize ));
    
			// parse note: using whitespace as separator instead
			//   of LWS because this could treat chunkdata as header
			//   tokens (spec is vague)
    
			ED_RETURN_EXCEPTION(SkipOptionalWhitespace( scanner ));
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
    
			// read optional extensions (and discard them)
			while ( true )
			{
				if ( token->tokType == Token::CRLF )
				{
					break;  // end of chunk header
				}
				if ( token->s != ';' )
				{
//					throw -1;   // bad entity
					code = -1;	// bad entity
					goto CatchError;
				}

				SkipOptionalWhitespace( scanner );

				ED_RETURN_EXCEPTION(scanner.getToken(&token));

				if ( token->tokType != Token::IDENTIFIER )
				{
//					throw -1;
					code = -1;
					goto CatchError;
				}

				ED_RETURN_EXCEPTION(SkipOptionalWhitespace( scanner ));

				ED_RETURN_EXCEPTION(scanner.getToken(&token));

				if ( token->s != '=' )
				{
					continue;   // could be start of header
				}

				ED_RETURN_EXCEPTION(SkipOptionalWhitespace( scanner ));

				ED_RETURN_EXCEPTION(scanner.getToken(&token));

				if ( !(token->tokType == Token::IDENTIFIER ||
					   token->tokType == Token::QUOTED_STRING) )
				{
//					throw -1;
					code = -1;
					goto CatchError;
				}

				ED_RETURN_EXCEPTION(SkipOptionalWhitespace( scanner ));

				ED_RETURN_EXCEPTION(scanner.getToken(&token));
			}
    
			// read chunk data
        
        
			if ( chunkSize == 0 )
			{
				break;  // last chunk
			}
        
			readEntityUsingLength( scanner, chunkSize );

			// crlf after chunk data
			ED_RETURN_EXCEPTION(scanner.getToken(&token));
			if ( token->tokType != Token::CRLF )
			{
				scanner.pushBack();
//				throw HttpParseException( "HttpMessage::readEncodedEntity(): bad chunk data\r\n", scanner.getLineNum() );
				DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpMessage::readEncodedEntity(): bad chunk data\r\n", scanner.getLineNum() );
				return EDERR_PARSE_EXCEPTION;
			}
		} // while
    
		// read entity headers
		return ParseHeaders( scanner, headers );
//	}
//	catch ( int code )
//	{
CatchError:
		if ( code == -1 )
		{
//			HttpParseException e( "HttpMessage::readEncodedEntity() bad entity", scanner.getLineNum() );
//			e.setErrorCode( PARSERR_BAD_ENTITY );
//			throw e;
			DEBUG_THROW_HTTP_PARSE_EXCEPTION( "HttpMessage::readEncodedEntity() bad entity", scanner.getLineNum() );
			return EDERR_PARSE_EXCEPTION;
		}
//	}

	return ED_OK;
}

#endif
