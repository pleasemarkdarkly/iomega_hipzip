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

#ifndef GENEXCEPTION_H
#define GENEXCEPTION_H

// genlib/util/genexception.h

// $Revision: 1.6 $
// $Date: 2000/09/06 20:03:03 $

#include <util/upnp/genlib/xstring.h>

class GenericException
{
public:
	// throw OutOfMemoryException
    GenericException();

	// throw OutOfMemoryException
    GenericException( const char* message );

	// throw OutOfMemoryException
    GenericException( const GenericException& other );

    virtual ~GenericException();
    
	// throw OutOfMemoryException
    virtual GenericException& operator =
            ( const GenericException& other );
    
	// throw OutOfMemoryException
    virtual void setValues( const char *message,
        const char* srcfile = NULL, int linenum = 0 );
        
    virtual const char* getMessage() const;
    virtual const char* getSrcFileName() const;
    virtual int getLineNum() const;
    
    virtual int getErrorCode() const;
    virtual void setErrorCode( int errCode );
    
    virtual void print() const;

	// throw OutOfMemoryException
    virtual void appendMessage( const char* msg );
    
protected:
    // all inherited classes MUST override this method
    virtual const char* getClassName() const;
    
private:
    static void setStrValue( char*& str, const char *value );
    void doCopy( const GenericException& other );
    
private:
    xstring msg;
    xstring srcFileName;
    int lineNum;
    int errorCode;  // for interoperating with C code if necessary
};

// allows creation of new exception type with a heirarchy
// C - name of new exception class
// P - name of existing parent class
// CLASSNAME - name of new exception class in quotes
// Eg:
// CREATE_NEW_EXCEPTION_TYPE( MyFoo, GenericException, "MyFoo" )

#define CREATE_NEW_EXCEPTION_TYPE( C, P, CLASSNAME )    \
class C : public P                          \
{                                           \
public:                                     \
    C() : P()   { }                         \
    C( const char* s ) : P(s)   { }         \
    C( const C& other ) : P( other ) { }    \
protected:                                  \
    const char* getClassName() const        \
    { return CLASSNAME; }                   \
};


#endif /* GENEXCEPTION_H */
