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

//	$Revision: 1.8 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/Attr.h>

char* Attr::getName()
{
	char *returnName;
	returnName= new char[strlen(this->nact->NA_NodeName)+1];
	if(!returnName)
	{
	   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	return NULL;
	}  		
	strcpy(returnName,this->nact->NA_NodeName);
	return(returnName);
}

char* Attr::getValue()
{
	char *returnValue;
	returnValue= new char[strlen(this->nact->NA_NodeValue)+1];
	if(!returnValue)
	{
	   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	return NULL;
	}  		
	strcpy(returnValue,this->nact->NA_NodeValue);
	return(returnValue);
}

Attr::Attr()
{
	this->nact=NULL;
}

Attr::Attr(char *name)
{
	nact = new NodeAct(ATTRIBUTE_NODE, name,"",this);
	if(!nact)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	nact->RefCount++;
}

Attr::Attr(char *name, char*value)
{
	nact = new NodeAct(ATTRIBUTE_NODE, name,value,this);
	if(!nact)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}	
	nact->RefCount++;
}

Attr::~Attr()
{
}

Attr & Attr::operator = (const Attr &other)
{
    if (this->nact != other.nact)
    {
        this->nact = other.nact;
		this->nact->RefCount++;
    }
    return *this;
};

#endif
