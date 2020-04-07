//........................................................................................
//........................................................................................
//.. File Name: noexceptions.h																	..
//.. Date: 12/5/2000																	..
//.. Author(s): Ed Miller
//.. Description of content: Functions and defines to replace exception handling        ..
//.. Usage: Used to remove exception handling from the code                             ..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef NO_EXCEPTIONS_H_
#define NO_EXCEPTIONS_H_

#include <cyg/infra/diag.h>

#define FORCE_NO_EXCEPTIONS

//#define DEBUG_EXCEPTION(s...) diag_printf(##s)
#define DEBUG_EXCEPTION(s...) /**/

#define DEBUG_THROW_EXCEPTION(s) diag_printf("$$$ Exception: " __FILE__ ": %d: %s\n", __LINE__, s)
//#define DEBUG_THROW_EXCEPTION //

#define DEBUG_THROW_TOKENIZER_EXCEPTION(errstr) diag_printf("$$$ Tokenizer Exception: " __FILE__ ": %d: %s\n", __LINE__, errstr)
//#define DEBUG_THROW_TOKENIZER_EXCEPTION //

#define DEBUG_THROW_HTTP_PARSE_EXCEPTION(errstr, lineno) diag_printf("$$$ HTTP Parse Exception: " __FILE__ ": %d: %s: Line %d\n", __LINE__, errstr, lineno)
//#define DEBUG_THROW_HTTP_PARSE_EXCEPTION //

#define DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION(errstr) diag_printf("$$$ Out of Memory Exception: " __FILE__ ": %d: %s\n", __LINE__, errstr)
//#define DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION //

#define DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION(errstr) diag_printf("$$$ Out of Bounds Exception: " __FILE__ ": %d: %s\n", __LINE__, errstr)
//#define DEBUG_THROW_OUT_OF_BOUNDS_EXCEPTION //

#define DEBUG_THROW_NET_EXCEPTION(errstr, errcode) diag_printf("$$$ HTTP Parse Exception: " __FILE__ ": %d: %s: error %d\n", __LINE__, errstr, errcode)
//#define DEBUG_THROW_NET_EXCEPTION //

#define DEBUG_THROW_EOF_EXCEPTION(s) diag_printf("$$$ EOF Exception: " __FILE__ ": %d: %s\n", __LINE__, s)
//#define DEBUG_EOF_EXCEPTION //

#define DEBUG_THROW_FILE_NOT_FOUND_EXCEPTION(s) diag_printf("$$$ File Not Found Exception: " __FILE__ ": %d: %s\n", __LINE__, s)
//#define DEBUG_THROW_FILE_NOT_FOUND_EXCEPTION //

#define DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION diag_printf("$$$ DOM Fatal error during parsing Exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION //

#define DEBUG_THROW_DOM_INSUFFICIENT_MEMORY_EXCEPTION diag_printf("$$$ DOM Insufficient Memory Exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_DOM_INSUFFICIENT_MEMORY_EXCEPTION //

#define DEBUG_THROW_DOM_NOT_FOUND_ERR_EXCEPTION diag_printf("$$$ DOM Not Found Error: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_DOM_NOT_FOUND_ERR_EXCEPTION //

#define DEBUG_THROW_DOM_NO_SUCH_NODE_EXCEPTION diag_printf("$$$ DOM No Such Node Exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_DOM_NO_SUCH_NODE_EXCEPTION //

#define DEBUG_THROW_MINISERVER_NETWORK_READ_ERROR_EXCEPTION diag_printf("$$$ Miniserver network read error exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_MINISERVER_NETWORK_READ_ERROR_EXCEPTION //

#define DEBUG_THROW_MINISERVER_MALFORMED_LINE_EXCEPTION diag_printf("$$$ Miniserver malformed line exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_MINISERVER_MALFORMED_LINE_EXCEPTION //

#define DEBUG_THROW_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION diag_printf("$$$ Miniserver method not implemented exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION //

#define DEBUG_THROW_MINISERVER_LENGTH_NOT_SPECIFIED_EXCEPTION diag_printf("$$$ Miniserver length not specified exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_MINISERVER_LENGTH_NOT_SPECIFIED_EXCEPTION //

#define DEBUG_THROW_MINISERVER_TIMED_OUT_EXCEPTION diag_printf("$$$ Miniserver timed out exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_MINISERVER_TIMED_OUT_EXCEPTION //

#define DEBUG_THROW_HTTP_NOT_IMPLEMENTED_EXCEPTION diag_printf("$$$ HTTP not implemented exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_NOT_IMPLEMENTED_EXCEPTION //

#define DEBUG_THROW_HTTP_VERSION_NOT_SUPPORTED_EXCEPTION diag_printf("$$$ HTTP version not supported exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_VERSION_NOT_SUPPORTED_EXCEPTION //

#define DEBUG_THROW_HTTP_BAD_REQUEST_EXCEPTION diag_printf("$$$ HTTP bad request exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_BAD_REQUEST_EXCEPTION //

#define DEBUG_THROW_HTTP_FORBIDDEN_EXCEPTION diag_printf("$$$ HTTP forbidden exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_FORBIDDEN_EXCEPTION //

#define DEBUG_THROW_HTTP_NOT_FOUND_EXCEPTION diag_printf("$$$ HTTP not found exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_NOT_FOUND_EXCEPTION //

#define DEBUG_THROW_HTTP_INTERNAL_SERVER_ERROR_EXCEPTION diag_printf("$$$ HTTP internal server error exception: " __FILE__ ": %d\n", __LINE__)
//#define DEBUG_THROW_HTTP_INTERNAL_SERVER_ERROR_EXCEPTION //

typedef int EDERRCODE;

#define EDERR_PARSE_EXCEPTION			-101
#define EDERR_TOKENIZER_EXCEPTION		-102
#define EDERR_OUT_OF_BOUNDS_EXCEPTION	-103
#define EDERR_NET_EXCEPTION				-104
#define EDERR_EOF_EXCEPTION				-105
#define EDERR_FILE_NOT_FOUND_EXCEPTION	-106

#define EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION	-201
#define EDERR_DOM_INSUFFICIENT_MEMORY_EXCEPTION			-202
#define EDERR_DOM_NOT_FOUND_ERR_EXCEPTION				-203
#define EDERR_DOM_NO_SUCH_NODE_EXCEPTION				-204

#define EDERR_MINISERVER_NETWORK_READ_ERROR_EXCEPTION		-301
#define EDERR_MINISERVER_MALFORMED_LINE_EXCEPTION			-302
#define EDERR_MINISERVER_LENGTH_NOT_SPECIFIED_EXCEPTION		-303
#define EDERR_MINISERVER_METHOD_NOT_ALLOWED_EXCEPTION		-304
#define EDERR_MINISERVER_INTERNAL_SERVER_ERROR_EXCEPTION	-305
#define EDERR_MINISERVER_METHOD_NOT_IMPLEMENTED_EXCEPTION	-306
#define EDERR_MINISERVER_TIMED_OUT_EXCEPTION				-307

#define EDERR_HTTP_NOT_IMPLEMENTED_EXCEPTION			-401
#define EDERR_HTTP_VERSION_NOT_SUPPORTED_EXCEPTION		-402
#define EDERR_HTTP_BAD_REQUEST_EXCEPTION				-403
#define EDERR_HTTP_FORBIDDEN_EXCEPTION					-404
#define EDERR_HTTP_NOT_FOUND_EXCEPTION					-405
#define EDERR_HTTP_INTERNAL_SERVER_ERROR_EXCEPTION		-406

#define EDERR_TO_READ_EXCEPTION_CODE(x) (300 + x)

#define EDERR_OUT_OF_MEMORY				-666

#define	ED_OK						0
#define ED_FAILED(x) ((x) < 0)

#define ED_RETURN_EXCEPTION(x) { int iRetVal = x; if (iRetVal < 0) return iRetVal; }
#define ED_CATCH_EXCEPTION(x, label) { if ((x) < 0) goto label; }

#endif	// NO_EXCEPTIONS_H_
