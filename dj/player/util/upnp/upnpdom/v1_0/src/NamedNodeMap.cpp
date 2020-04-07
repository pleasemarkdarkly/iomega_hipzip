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

//	$Revision: 1.7 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/NamedNodeMap.h>

Node* NamedNodeMap::getNamedItem(char *name)
{
	long index;
	index=getItemNumber(name);
	if(index== -1)
	{
		Node *ReturnNode;
		ReturnNode = new Node;
    	if(!ReturnNode)
    	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		ReturnNode->nact=NULL;
		return ReturnNode;
	}
	else
		return(item((unsigned long)index));
}

long NamedNodeMap::getItemNumber(char *name)
{
	NodeAct *n;
	unsigned int returnItemNo=0;

	n=ofWho->nact->FirstAttr;
	while(n!=NULL)
	{
		if(!strcmp(name, n->NA_NodeName))
			return returnItemNo;
		n = n->NextSibling;
		returnItemNo++;
	}
	return -1;
}

Node* NamedNodeMap::item(unsigned long index)
{
	NodeAct *n;
	Node *ReturnNode;
//	Node ReturnNode;
	if(index > getLength()-1)
	{
		ReturnNode = new Node;
		if(!ReturnNode)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		ReturnNode->nact=NULL;
		return ReturnNode;
	}
	n=ofWho->nact->FirstAttr;
	for(unsigned int i=0; i<index && n!=NULL; ++i)
		n = n->NextSibling;
	ReturnNode = new Node;
	if(!ReturnNode)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	ReturnNode->nact = n;
	n->RefCount++;
    return ReturnNode;
}

unsigned long NamedNodeMap::getLength()
{
	NodeAct *n;
	unsigned long length=0;

	n=ofWho->nact->FirstAttr;
	for(length=0; n!=NULL; ++length, n=n->NextSibling);
	return length;
}

#endif
