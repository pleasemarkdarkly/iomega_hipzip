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

//	$Revision: 1.11 $
//	$Date: 2000/09/28 22:01:10 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/DOMException.h>
DOMException::DOMException()
{
        code = (ExceptionCode) 0;
};

DOMException::DOMException(ExceptionCode ecode, char *mesg)
{
	code=ecode;
	writeError();
}

DOMException::~DOMException() 
{
};

void DOMException::writeError()
{
	switch(code)
	{
	case NOT_FOUND_ERR:				strcpy(msg,"\n UPnPDOM: Referencing a non-existing node");
									break;
	case NO_SUCH_NODE:				strcpy(msg,"\n UPnPDOM: Node Not found: First create a node");
									break;
	case DELETE_NODE_NOT_ALLOWED:	strcpy(msg,"\n UPnPDOM: Cannot Delete Node: First delete all its attributes and children");
									break;
	case FATAL_ERROR_DURING_PARSING:strcpy(msg,"\n UPnPDOM: Fatal Error During Parsing: Check the document\n");
									break;
	case NO_SUCH_FILE:				strcpy(msg,"\n UPnPDOM: Fatal Error During Parsing: Check if the file exists\n");
									break;
	case INSUFFICIENT_MEMORY:		strcpy(msg,"\n UPnPDOM: Fatal Error During Memory Allocation: Free up some memory\n");
									break;
 	default: break;
	}
}

#endif
