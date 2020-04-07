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
#include <util/upnp/upnpdom/NodeList.h>


Node* NodeList::item(unsigned long index)
{
	InternalList *n;
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
	n=head;
	for(unsigned int i=0; i<index && n!=NULL; ++i)
		n = n->next;
	ReturnNode = new Node;
	if(!ReturnNode)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	ReturnNode->nact = n->ofWho;
	ReturnNode->nact->RefCount++;
    return ReturnNode;
}



void NodeList::addToInternalList(NodeAct *add)
{
	InternalList *traverse, *p;
	InternalList *na = new InternalList;
	if(!na)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	
	na->next=NULL;
	na->ofWho=add;

	if(head==NULL)
	{
		head=na;
		return;
	}
	traverse=head;
	p=traverse;
	while(traverse !=NULL)
	{
		p=traverse;
		traverse=traverse->next;
	}
	p->next=na;
}


unsigned long NodeList::getLength()
{
	InternalList *n;
	unsigned long length=0;

	n=head;
	for(length=0; n!=NULL; ++length, n=n->next);
	return length;
}


NodeList::~NodeList()
{
	InternalList *del, *prev;
	del=head;
	prev=del;
	while(del!=NULL)
	{
		del=del->next;
		delete prev;
		prev=del;
	}
}

#endif
