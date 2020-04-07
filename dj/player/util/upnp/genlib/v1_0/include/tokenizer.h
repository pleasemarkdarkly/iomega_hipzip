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

// $Revision: 1.4 $
// $Date: 2000/07/29 21:49:19 $

#ifndef TOKENIZER_H
#define TOKENIZER_H

// genlib/net/http/tokenizer.h

#include <util/upnp/genlib/genexception.h>
#include <util/upnp/genlib/miscexceptions.h>
#include <util/upnp/genlib/xstring.h>
#include <util/upnp/genlib/charreader.h>

#include <util/upnp/genlib/noexceptions.h>

// ParseException : Basic Exception
CREATE_NEW_EXCEPTION_TYPE( ParseException, BasicException, "ParseException" )

// TokenizerException : ParseException
CREATE_NEW_EXCEPTION_TYPE( TokenizerException, ParseException, "TokenizerException" )


class Token
{
public:
    enum TokenType { IDENTIFIER, WHITESPACE, CRLF, CTRL,
        SEPARATOR, QUOTED_STRING, END_OF_STREAM,
        UNKNOWN };  // useful?
        
public:
    Token();
    virtual ~Token();
    const char* getTokenTypeStr() const;
    
public:
    TokenType tokType;
    xstring s;          // token in string form
    char c;             // for single char tokens
    int num;            // if number

public:
    // internal-use only
    
    // t != NULL
    void insertAfterSelf( Token* t );
    void insertBeforeSelf( Token* t );
    void unlinkSelf();

public: 
    // internal use only
    // for doubly-linked list
    Token* prev;
    Token* next;
};

// parses ascii char data 0..127 only
class Tokenizer
{
public:
    Tokenizer( CharReader& r );
    virtual ~Tokenizer();
    
//    Token* getToken();
	EDERRCODE getToken(Token** ppToken);
    EDERRCODE pushBack();
    
    // returns num bytes actually read; -1 on error
    // warning: no pushBack()s immediately after a read;
    //   i.e., getToken()s must balance pushBack()s starting after
    //   read()
    int read( OUT void* buf, IN int bufsize );
    
    bool endOfData() const;
    
    // returns line # of the last token read from reader
    int getLineNum() const
    {
        return lineNum;
    }
    
private:
    enum { MAX_PUSHBACKS = 5 };
    EDERRCODE readNextToken( Token& t );
    void deleteUntilSizeEquals( int newSize );
    
private:
    Token* head;
    Token* tail;
    Token* current;
    
    int listLen;
    CharReader& reader;
    int lineNum;    // current line being processed
};


#endif /* TOKENIZER_H */






