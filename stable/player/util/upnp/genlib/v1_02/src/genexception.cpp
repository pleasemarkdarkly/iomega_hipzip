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

// $Revision: 1.1 $
// $Date: 2000/08/29 19:18:44 $

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/genexception.h>

GenericException::GenericException()
{
    lineNum = 0;
    errorCode = -1;
}

GenericException::GenericException( const char* message )
{
    msg = message;
    lineNum = 0;
    errorCode = -1;
}

GenericException::GenericException( const GenericException& other )
{
    doCopy( other );
}

void GenericException::setValues( const char *message,
    const char* srcfile, int linenum )
{
    msg = message;
    srcFileName = srcfile;
    lineNum = linenum;
}

GenericException::~GenericException()
{
}

GenericException& GenericException::operator =
            ( const GenericException& other )
{
    if ( &other != this )
    {
        doCopy( other );
    }
    
    return *this;
}


const char* GenericException::getMessage() const
{
    return msg.c_str();
}

const char* GenericException::getSrcFileName() const
{
    return srcFileName.c_str();
}

int GenericException::getLineNum() const
{
    return lineNum;
}

void GenericException::setErrorCode( int errCode )
{
    errorCode = errCode;
}

int GenericException::getErrorCode() const
{
    return errorCode;
}


void GenericException::print() const
{
    printf( "%s: %s\n", getClassName(), msg.c_str() );
}

const char* GenericException::getClassName() const
{
    return "GenericException";
}


void GenericException::doCopy( const GenericException& other )
{
    msg =  other.msg;
    srcFileName = other.srcFileName;
    lineNum = other.lineNum;
    errorCode = other.errorCode;
}

void GenericException::appendMessage( const char* message )
{
    msg += message;
}
