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

#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <util/upnp/upnpdom/Document.h>
#include <util/upnp/upnpdom/Attr.h>
#include <util/upnp/upnpdom/Node.h>
#include <util/upnp/upnpdom/NodeAct.h>
#include <util/upnp/upnpdom/NodeList.h>
#include <util/upnp/upnpdom/all.h>
#include <util/upnp/genlib/noexceptions.h>

class Document;
class Node;
class Attr;
class NodeList;

//class Parser;
//Element attributes do not constitute in the DOM tree although they are
//"cling on's" to the elements

class Element : public Node
{
public:
    Element& operator = (const Element &other);

#ifndef FORCE_NO_EXCEPTIONS
	void setAttribute(char *name, char *value);
	Attr& setAttributeNode(Attr& newAttr);
#endif	// FORCE_NO_EXCEPTIONS
	EDERRCODE setAttributeNoEx(char *name, char *value);
	NodeList* getElementsByTagName( char * tagName);

//	char *getAttribute(char *name);
	EDERRCODE setAttributeNodeNoEx(Attr& newAttr);
	Element(char *name);
	Element();
	~Element();
//	Attr& getAttributeNode(char *name)
protected:
//	friend class Document;
	void MakeNodeList(Node* n, char* match, NodeList **nl);
	char * tagName;
};

#endif


