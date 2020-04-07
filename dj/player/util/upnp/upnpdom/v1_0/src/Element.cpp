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

//	$Revision: 1.15 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/Element.h>

#ifndef FORCE_NO_EXCEPTIONS
//For now it just adds a new attribute with the name and value apecified in the
//argument. DOM Level1 actually has to search if there is already a node present
//if there is a node present then it should modify the value
void Element::setAttribute(char *name, char *value)
{
	Attr *addAttr;
	Node* n;

	NamedNodeMap* nm;
	nm=this->getAttributes();
	n = nm->getNamedItem(name);
	if(n->nact==NULL)
	{
		addAttr = new Attr(name, value);
		if(!addAttr)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		nact->appendChild(addAttr->nact);
		delete addAttr;
	}
	else
	{
        if( n->nact->NA_NodeValue ) {
            delete [] n->nact->NA_NodeValue;
        }
		n->nact->NA_NodeValue=new char[strlen(value)+1];
		strcpy(n->nact->NA_NodeValue, value);
	}
    delete n;
    delete nm;
}

Attr& Element::setAttributeNode(Attr& newAttr)
{
	newAttr.nact->RefCount++;
	nact->appendChild(newAttr.nact);
	return newAttr;
}

#endif	// FORCE_NO_EXCEPTIONS

//For now it just adds a new attribute with the name and value apecified in the
//argument. DOM Level1 actually has to search if there is already a node present
//if there is a node present then it should modify the value
EDERRCODE
Element::setAttributeNoEx(char *name, char *value)
{
	Attr *addAttr;
	Node* n;

	NamedNodeMap* nm;
	nm = this->getAttributes();
	n = nm->getNamedItem(name);
	if(n->nact==NULL)
	{
		addAttr = new Attr(name, value);
		if(!addAttr)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		ED_RETURN_EXCEPTION(nact->appendChildNoEx(addAttr->nact));
		delete addAttr;
	}
	else
	{
        if( n->nact->NA_NodeValue ) {
            delete [] n->nact->NA_NodeValue;
        }
		n->nact->NA_NodeValue=new char[strlen(value)+1];
		strcpy(n->nact->NA_NodeValue, value);
	}
    delete n;
    delete nm;

	return ED_OK;
}

EDERRCODE Element::setAttributeNodeNoEx(Attr& newAttr)
{
	newAttr.nact->RefCount++;
	ED_RETURN_EXCEPTION(nact->appendChildNoEx(newAttr.nact));
	return ED_OK;
}

NodeList* Element::getElementsByTagName( char * tagName)
{
	NodeList *returnNodeList;
	returnNodeList= new NodeList;
	if(!returnNodeList)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
    nact->NA_NodeType = DOCUMENT_NODE;
   	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Calling MakeNodeList for element whose tagname is %s\n", tagName);)
	MakeNodeList(this, tagName, &returnNodeList);
    nact->NA_NodeType = ELEMENT_NODE;
	return returnNodeList;
}

// A recursive function to make a nodelist from the tree.
void 	Element::MakeNodeList(Node* n, char* match, NodeList **nl)
{
	Node* child=n->getFirstChild();
	Node* temp;
	while( !child->isNull())
    {
		temp=child->getNextSibling();
//        if((child.getNodeType()==ELEMENT_NODE)&&((!strcmp(match, name))||(!strcmp(match, "*"))))
        if (child->getNodeType()==ELEMENT_NODE)
        {
            char *name;
            if ((child->getNodeNameNoEx(&name) == ED_OK) && (!strcmp(match, name) || !strcmp(match, "*")))
                (*nl)->addToInternalList(child->nact);
            delete [] name;
        }
		MakeNodeList(child, match, nl);
		child = temp;
    }
    if(child->isNull())
        delete child;
	if(n->getNodeType()!=DOCUMENT_NODE)
        delete n;
}

Element::Element(char *name)
{
	nact = new NodeAct(ELEMENT_NODE, name,NULL,this);
	if(!nact)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	nact->RefCount++;
}

Element::Element()
{
	this->nact=NULL;
}

Element::~Element()
{
}

Element & Element::operator = (const Element &other)
{
    if (this->nact != other.nact)
    {
        this->nact = other.nact;
		this->nact->RefCount++;
    }
    return *this;
};

#endif
