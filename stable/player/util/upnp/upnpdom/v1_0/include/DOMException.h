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

//	$Revision: 1.5 $
//	$Date: 2000/08/03 21:21:25 $

#ifndef DOMException_H
#define DOMException_H

#include <string.h>

class DOMException{
public:
        enum ExceptionCode {
                INDEX_SIZE_ERR				= 1,
                DOMSTRING_SIZE_ERR			= 2,
                HIERARCHY_REQUEST_ERR		= 3,
                WRONG_DOCUMENT_ERR			= 4,
                INVALID_CHARACTER_ERR		= 5,
                NO_DATA_ALLOWED_ERR			= 6,
                NO_MODIFICATION_ALLOWED_ERR = 7,
                NOT_FOUND_ERR				= 8,
                NOT_SUPPORTED_ERR			= 9,
                INUSE_ATTRIBUTE_ERR			= 10,
                INVALID_STATE_ERR			= 11,
	       		SYNTAX_ERR					= 12,
        		INVALID_MODIFICATION_ERR    = 13,
        		NAMESPACE_ERR				= 14,
        		INVALID_ACCESS_ERR			= 15,
				NO_SUCH_NODE				= 100,
				NO_SUCH_FILE				= 101,
				DELETE_NODE_NOT_ALLOWED		= 102,
				FATAL_ERROR_DURING_PARSING	= 103,
				INSUFFICIENT_MEMORY			= 104
        };
public:
    DOMException();
    DOMException(ExceptionCode ecode, char *mesg=NULL);
	void writeError();
	~DOMException();
    ExceptionCode   code;
	char msg[500];
};

#endif
