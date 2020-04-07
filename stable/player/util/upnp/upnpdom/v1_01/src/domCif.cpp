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

//	$Revision: 1.28 $
//	$Date: 2000/10/06 00:31:39 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <util/upnp/upnpdom/domCif.h>
#include <util/upnp/upnpdom/Parser.h>
#include <util/upnp/upnpdom/Node.h>
#include <util/upnp/upnpdom/NodeAct.h>
#include <util/upnp/upnpdom/NodeList.h>
#include <util/upnp/upnpdom/Document.h>

#include <cyg/infra/diag.h>
#include <util/debug/debug.h>

#define DEBUG_DELETE(x...)


// ---------------------------------------------------------------------------
//  Forward references
// ---------------------------------------------------------------------------
void copyToTarget(char*& target, const char* p);
void DumpDocument(char *&target, Node* n);


// todo::
//bool  Upnp_DOMImplementation_hasFeature( const Upnp_DOMString & feature,const Upnp_DOMString & version)
//{
//	DOMImplementation UpnpDOM;
//	return(UpnpDOM.hasFeature(feature,version));
//}


/*******************************************************************************************************************************/
//Interface Node Implementation
/*******************************************************************************************************************************/

EXPORT  Upnp_Node    UpnpNode_insertBefore(Upnp_Node OperationNode, Upnp_Node newChild, Upnp_Node refChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Node *)OperationNode).insertBeforeNoEx((*(Node *)newChild),(*(Node *)refChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT    Upnp_Node    UpnpNode_replaceChild(Upnp_Node OperationNode, Upnp_Node newChild, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Node *)OperationNode).replaceChildNoEx((*(Node *)newChild),(*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpNode_removeChild(Upnp_Node OperationNode, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Node *)OperationNode).removeChildNoEx((*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpNode_appendChild(Upnp_Node OperationNode, Upnp_Node newChild, Upnp_DOMException *err)
{
	*err =NO_ERR;
	if (ED_FAILED((*(Node *)OperationNode).appendChildNoEx((*(Node *)newChild))))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

    // Technically we're supposed to return the result from Node::appendChild.
    // But the return value from that call is ALWAYS the node that's passed in.
    // So avoid memory headaches and just pass that node back.
    return newChild;
}	

EXPORT  Upnp_Bool   UpnpNode_hasChildNodes(Upnp_Node OperationNode)
{
	return((*(Node *)OperationNode).hasChildNodes());
}
	
EXPORT  Upnp_Node     UpnpNode_cloneNode(Upnp_Node OperationNode, Upnp_Bool deep)
{
	Node *ret = (*(Node *)OperationNode).cloneNode((deep>0));
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_DOMString   UpnpNode_getNodeName(Upnp_Node OperationNode)
{
	Upnp_DOMString temp;
//	temp = (*(Node *)OperationNode).getNodeName();
	// TODO: check exceptions.
	(*(Node *)OperationNode).getNodeNameNoEx(&temp);
	return(temp);
}

EXPORT  Upnp_DOMString   UpnpNode_getNodeValue(Upnp_Node OperationNode, Upnp_DOMException *err)
{
/*
	Upnp_DOMString temp=NULL;
	*err =NO_ERR;
	try
	{
		temp = (*(Node *)OperationNode).getNodeValue();
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	Upnp_DOMString temp=NULL;
	*err =NO_ERR;
	if (ED_FAILED((*(Node *)OperationNode).getNodeValueNoEx(&temp)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	return(temp);
}

EXPORT  Upnp_Void UpnpNode_setNodeValue(Upnp_Node OperationNode, Upnp_DOMString nodeValue, Upnp_DOMException *err)
{
	*err =NO_ERR;
/*
	try
	{
		(*(Node *)OperationNode).setNodeValue(nodeValue);
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	if (ED_FAILED((*(Node *)OperationNode).setNodeValueNoEx(nodeValue)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
	}
}

EXPORT  Upnp_UShort     UpnpNode_getNodeType(Upnp_Node OperationNode)
{
	return((*(Node *)OperationNode).getNodeType());
}

 EXPORT  Upnp_Node     UpnpNode_getParentNode(Upnp_Node OperationNode)
{
	Node *ret = (*(Node *)OperationNode).getParentNode();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NodeList      UpnpNode_getChildNodes(Upnp_Node OperationNode)
{
	NodeList *ret = (*(Node *)OperationNode).getChildNodes();
	if(!ret->getLength())
	{
		UpnpNodeList_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node    UpnpNode_getFirstChild(Upnp_Node OperationNode)
{
	Node *ret = (*(Node *)OperationNode).getFirstChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpNode_getLastChild(Upnp_Node OperationNode)
{
	Node *ret = (*(Node *)OperationNode).getLastChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpNode_getPreviousSibling(Upnp_Node OperationNode)
{
	Node *ret = (*(Node *)OperationNode).getPreviousSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node UpnpNode_getNextSibling(Upnp_Node OperationNode)
{
	Node *ret = (*(Node *)OperationNode).getNextSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NamedNodeMap   UpnpNode_getAttributes(Upnp_Node OperationNode)
{
	NamedNodeMap *ret = (*(Node *)OperationNode).getAttributes();
	if(!ret->getLength())
	{
		UpnpNamedNodeMap_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Document   UpnpNode_getOwnerDocument(Upnp_Node OperationNode)
{
	Document *ret = (*(Node *)OperationNode).getOwnerDocument();
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}


EXPORT  Upnp_Void	UpnpNode_free(Upnp_Node OperationNode)
{
	if(OperationNode == NULL)
		return;
	Node *rmv;
	rmv=(Node *)OperationNode;
	delete rmv;
	rmv=NULL;
}

/*******************************************************************************************************************************/
//Interface Document Implementation
/*******************************************************************************************************************************/

EXPORT  Upnp_Document   UpnpDocument_createDocument(Upnp_Document OperationDocument)
{
	Document *ret = (*(Document *)OperationDocument).createDocument();
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Element   UpnpDocument_createElement(Upnp_Document OperationDocument, Upnp_DOMString tagName, Upnp_DOMException *err)
{
	*err =NO_ERR;
    Element *ret = (*(Document *)OperationDocument).createElement(tagName);

	if(ret->isNull())
	{
		UpnpElement_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Text   UpnpDocument_createTextNode(Upnp_Document OperationDocument, Upnp_DOMString data)
{
	Node *ret = (*(Document *)OperationDocument).createTextNode(data);
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Attr   UpnpDocument_createAttribute(Upnp_Document OperationDocument, Upnp_DOMString name, Upnp_DOMException *err)
{
    Attr *ret = (*(Document *)OperationDocument).createAttribute(name);

	if(ret->isNull())
	{
		delete ret;
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NodeList   UpnpDocument_getElementsByTagName(Upnp_Document OperationDocument, Upnp_DOMString tagname)
{
	NodeList *ret = (*(Document *)OperationDocument).getElementsByTagName(tagname);
	if(!ret->getLength())
	{
		UpnpNodeList_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}
/*
Upnp_Element   UpnpDocument_getDocumentElement(Upnp_Document OperationDocument)
{
	Element *ret = new Element;
	*ret = (*(Document *)OperationDocument).getDocumentElement();
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}
*/
EXPORT  Upnp_Node    UpnpDocument_insertBefore(Upnp_Document OperationDocument, Upnp_Node newChild, Upnp_Node refChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Document *)OperationDocument).insertBeforeNoEx((*(Node *)newChild),(*(Node *)refChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpDocument_replaceChild(Upnp_Document OperationDocument, Upnp_Node newChild, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Document *)OperationDocument).replaceChildNoEx((*(Node *)newChild),(*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpDocument_removeChild(Upnp_Document OperationDocument, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Document *)OperationDocument).removeChildNoEx((*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpDocument_appendChild(Upnp_Document OperationDocument, Upnp_Node newChild, Upnp_DOMException *err)
{
	*err =NO_ERR;
	if (ED_FAILED((*(Document *)OperationDocument).appendChildNoEx((*(Node *)newChild))))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

    // Technically we're supposed to return the result from Node::appendChild.
    // But the return value from that call is ALWAYS the node that's passed in.
    // So avoid memory headaches and just pass that node back.
    return newChild;
}	

EXPORT  Upnp_Bool   UpnpDocument_hasChildNodes(Upnp_Document OperationDocument)
{
	return((*(Document *)OperationDocument).hasChildNodes());
}
	
EXPORT  Upnp_Node     UpnpDocument_cloneNode(Upnp_Document OperationDocument, Upnp_Bool deep)
{
	Node *ret = (*(Document *)OperationDocument).cloneNode((deep>0));
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}


EXPORT  Upnp_DOMString   UpnpDocument_getNodeName(Upnp_Document OperationDocument)
{
	Upnp_DOMString temp;
//	temp = (*(Document *)OperationDocument).getNodeName();
	// TODO: check exceptions.
	(*(Document *)OperationDocument).getNodeNameNoEx(&temp);
	return(temp);
}

EXPORT  Upnp_DOMString   UpnpDocument_getNodeValue(Upnp_Document OperationDocument, Upnp_DOMException *err)
{
	Upnp_DOMString temp=NULL;
	*err =NO_ERR;
/*
	try
	{
		temp = (*(Document *)OperationDocument).getNodeValue();
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	if (ED_FAILED((*(Document *)OperationDocument).getNodeValueNoEx(&temp)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	return(temp);
}

EXPORT  Upnp_Void UpnpDocument_setNodeValue(Upnp_Document OperationDocument, Upnp_DOMString nodeValue, Upnp_DOMException *err)
{
	*err =NO_ERR;
/*
	try
	{
		(*(Document *)OperationDocument).setNodeValue(nodeValue);
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	if (ED_FAILED((*(Document *)OperationDocument).setNodeValueNoEx(nodeValue)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
	}
}

EXPORT  Upnp_UShort     UpnpDocument_getNodeType(Upnp_Document OperationDocument)
{
	return((*(Document *)OperationDocument).getNodeType());
}

EXPORT  Upnp_Node     UpnpDocument_getParentNode(Upnp_Document OperationDocument)
{
	Node *ret = (*(Document *)OperationDocument).getParentNode();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NodeList      UpnpDocument_getChildNodes(Upnp_Document OperationDocument)
{
	NodeList *ret = (*(Document *)OperationDocument).getChildNodes();
	if(!ret->getLength())
	{
		UpnpNodeList_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node    UpnpDocument_getFirstChild(Upnp_Document OperationDocument)
{
	Node *ret = (*(Document *)OperationDocument).getFirstChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpDocument_getLastChild(Upnp_Document OperationDocument)
{
	Node *ret = (*(Document *)OperationDocument).getLastChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpDocument_getPreviousSibling(Upnp_Document OperationDocument)
{
	Node *ret = (*(Document *)OperationDocument).getPreviousSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node UpnpDocument_getNextSibling(Upnp_Document OperationDocument)
{
	Node *ret = (*(Document *)OperationDocument).getNextSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NamedNodeMap   UpnpDocument_getAttributes(Upnp_Document OperationDocument)
{
	NamedNodeMap *ret = (*(Document *)OperationDocument).getAttributes();
	if(!ret->getLength())
	{
		UpnpNamedNodeMap_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Document   UpnpDocument_getOwnerDocument(Upnp_Document OperationDocument)
{
	Document *ret = (*(Document *)OperationDocument).getOwnerDocument();
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Void	UpnpDocument_free(Upnp_Document OperationDocument)
{
	if(OperationDocument == NULL)
		return;
	Document *rmv;
	rmv=(Document *)OperationDocument;
	delete rmv;
	rmv=NULL;
}

EXPORT  Upnp_Void	UpnpDocumentTree_free(Upnp_Document OperationDocument)
{
	if(OperationDocument == NULL)
		return; 
	Document *rmv;
	rmv=(Document *)OperationDocument;
	delete rmv;
	rmv=NULL;
}
	

/*******************************************************************************************************************************/
//Interface NodeList Implementation
/*******************************************************************************************************************************/
EXPORT  Upnp_Node   UpnpNodeList_item(Upnp_NodeList OperationNodeList, unsigned long index)
{
	Node *ret = (*(NodeList *)OperationNodeList).item(index);
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  int   UpnpNodeList_getLength(Upnp_NodeList OperationNodeList)
{
	return((*(NodeList *)OperationNodeList).getLength());
}


EXPORT Upnp_Void	UpnpNodeList_free(Upnp_NodeList OperationNodeList)
{
	if(OperationNodeList ==NULL)
		return;
	NodeList *rmv;
	rmv=(NodeList *)OperationNodeList;
	delete rmv;
	rmv=NULL;
}


/*******************************************************************************************************************************/
//Interface Element Implementation
/*******************************************************************************************************************************/
EXPORT  Upnp_Void UpnpElement_setAttribute(Upnp_Element OperationElement, Upnp_DOMString name, Upnp_DOMString value, Upnp_DOMException *err)
{
	*err =NO_ERR;
/*
	try
	{
		(*(Element *)OperationElement).setAttribute(name, value);
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	EDERRCODE ederr = (*(Element *)OperationElement).setAttributeNoEx(name, value);
	if (ED_FAILED(ederr))
		*err = NOT_FOUND_ERR;
}

EXPORT  Upnp_Attr   UpnpElement_setAttributeNode(Upnp_Element OperationElement, Upnp_Attr newAttr, Upnp_DOMException *err)
{
	Attr *ret = new Attr;
	if(!ret)
	{
	   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	return NULL;
	}  		
	*err =NO_ERR;

	(*(Element *)OperationElement).setAttributeNodeNoEx(*(Attr *)newAttr);

	if(ret->isNull())
	{
		delete ret;
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NodeList   UpnpElement_getElementsByTagName(Upnp_Element OperationElement, Upnp_DOMString name)
{
	NodeList *ret = (*(Element *)OperationElement).getElementsByTagName(name);
	if(!ret->getLength())
	{
		UpnpNodeList_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node    UpnpElement_insertBefore(Upnp_Element OperationElement, Upnp_Node newChild, Upnp_Node refChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Element *)OperationElement).insertBeforeNoEx((*(Node *)newChild),(*(Node *)refChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpElement_replaceChild(Upnp_Element OperationElement, Upnp_Node newChild, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Element *)OperationElement).replaceChildNoEx((*(Node *)newChild),(*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpElement_removeChild(Upnp_Element OperationElement, Upnp_Node oldChild, Upnp_DOMException *err)
{
	Node *ret = 0;
	*err =NO_ERR;
	if (ED_FAILED((*(Element *)OperationElement).removeChildNoEx((*(Node *)oldChild), &ret)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}	

EXPORT  Upnp_Node    UpnpElement_appendChild(Upnp_Element OperationElement, Upnp_Node newChild, Upnp_DOMException *err)
{
	*err =NO_ERR;
	if (ED_FAILED((*(Element *)OperationElement).appendChildNoEx((*(Node *)newChild))))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

    // Technically we're supposed to return the result from Node::appendChild.
    // But the return value from that call is ALWAYS the node that's passed in.
    // So avoid memory headaches and just pass that node back.
    return newChild;
}	

EXPORT   Upnp_Bool   UpnpElement_hasChildNodes(Upnp_Element OperationElement)
{
	return((*(Element *)OperationElement).hasChildNodes());
}
	
EXPORT  Upnp_Node     UpnpElement_cloneNode(Upnp_Element OperationNode, Upnp_Bool deep)
{
	Node *ret = (*(Element *)OperationNode).cloneNode((deep>0));
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_DOMString   UpnpElement_getNodeName(Upnp_Element OperationElement)
{
	Upnp_DOMString temp;
//	temp = (*(Element *)OperationElement).getNodeName();
	// TODO: check exceptions.
	(*(Element *)OperationElement).getNodeNameNoEx(&temp);
	return(temp);
}

EXPORT  Upnp_DOMString   UpnpElement_getNodeValue(Upnp_Element OperationElement, Upnp_DOMException *err)
{
	Upnp_DOMString temp=NULL;
	*err =NO_ERR;
/*
	try
	{
		temp = (*(Element *)OperationElement).getNodeValue();
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	if (ED_FAILED((*(Element *)OperationElement).getNodeValueNoEx(&temp)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
		return NULL;
	}

	return(temp);
}

EXPORT  Upnp_Void UpnpElement_setNodeValue(Upnp_Element OperationElement, Upnp_DOMString nodeValue, Upnp_DOMException *err)
{
	*err =NO_ERR;
/*
	try
	{
		(*(Element *)OperationElement).setNodeValue(nodeValue);
	}
	catch(DOMException &tocatch)
	{
		*err = (enum Upnp_DOMException)tocatch.code;
	}
*/
	if (ED_FAILED((*(Element *)OperationElement).setNodeValueNoEx(nodeValue)))
	{
		*err =NOT_SUPPORTED_ERR;	// TODO: convert to real error code
	}
}

EXPORT  Upnp_UShort     UpnpElement_getNodeType(Upnp_Element OperationElement)
{
	return((*(Element *)OperationElement).getNodeType());
}

EXPORT Upnp_Node     UpnpElement_getParentNode(Upnp_Element OperationElement)
{
	Node *ret = (*(Element *)OperationElement).getParentNode();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NodeList      UpnpElement_getChildNodes(Upnp_Element OperationElement)
{
	NodeList *ret = (*(Element *)OperationElement).getChildNodes();
	if(!ret->getLength())
	{
		UpnpNodeList_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node    UpnpElement_getFirstChild(Upnp_Element OperationElement)
{
	Node *ret = (*(Element *)OperationElement).getFirstChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpElement_getLastChild(Upnp_Element OperationElement)
{
	Node *ret = (*(Element *)OperationElement).getLastChild();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node     UpnpElement_getPreviousSibling(Upnp_Element OperationElement)
{
	Node *ret = (*(Element *)OperationElement).getPreviousSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Node UpnpElement_getNextSibling(Upnp_Element OperationElement)
{
	Node *ret = (*(Element *)OperationElement).getNextSibling();
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_NamedNodeMap   UpnpElement_getAttributes(Upnp_Element OperationElement)
{
	NamedNodeMap *ret = (*(Element *)OperationElement).getAttributes();
	if(!ret->getLength())
	{
		UpnpNamedNodeMap_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Document   UpnpElement_getOwnerDocument(Upnp_Element OperationElement)
{
	Document *ret = (*(Element *)OperationElement).getOwnerDocument();
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_Void	UpnpElement_free(Upnp_Element OperationElement)
{
	if(OperationElement == NULL)
		return;
	Element *rmv;
	rmv=(Element *)OperationElement;
	delete rmv;
	rmv=NULL;
}

/*******************************************************************************************************************************/
//Interface NamedNodeMap Implementation
/*******************************************************************************************************************************/

EXPORT  Upnp_Node   UpnpNamedNodeMap_getNamedItem(Upnp_NamedNodeMap OperationNamedNodeMap, Upnp_DOMString name)
{
	Node *ret = (*(NamedNodeMap *)OperationNamedNodeMap).getNamedItem(name);
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT Upnp_Node   UpnpNamedNodeMap_item(Upnp_NamedNodeMap OperationNamedNodeMap, unsigned long index)
{
	Node *ret = (*(NamedNodeMap *)OperationNamedNodeMap).item(index);
	if(ret->isNull())
	{
		UpnpNode_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

EXPORT  Upnp_ULong UpnpNamedNodeMap_getLength(Upnp_NamedNodeMap OperationNamedNodeMap)
{
	return((*(NamedNodeMap *)OperationNamedNodeMap).getLength());
}

EXPORT  Upnp_Void	UpnpNamedNodeMap_free(Upnp_NamedNodeMap OperationNamedNodeMap)
{
	if(OperationNamedNodeMap == NULL)
		return;
	NamedNodeMap *rmv;
	rmv=(NamedNodeMap *)OperationNamedNodeMap;
	delete rmv;
	rmv=NULL;
}

/*******************************************************************************************************************************/
//DOMLevel 1 Implementation End
/*******************************************************************************************************************************/


EXPORT  Upnp_Void Upnpfree(Upnp_DOMString in)
{
  delete[] in;
}

#ifndef FORCE_NO_EXCEPTIONS

EXPORT  Upnp_Document  UpnpParseFileAndGetDocument(char* xmlFile)
{
	Document *ret = new Document;
	if(!ret)
	{
	   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	return NULL;
	}  		
	try
	{
		*ret=(*ret).ReadDocumentFileOrBuffer(xmlFile, true);
	}
	catch (DOMException& /* toCatch */)
	{
//		DBGONLY(printf("%s\n",toCatch.msg));
		return NULL;
	}
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

#endif	// FORCE_NO_EXCEPTIONS

EXPORT  Upnp_DOMString  UpnpNewPrintDocument(Upnp_Node OperationNode)
{
	if(OperationNode == NULL)
		return NULL;
	Upnp_DOMString Upnp_Buff=NULL;
	if((*(Node *)OperationNode).isNull())
		return NULL;
    if((*(Node *)OperationNode).isNull())
    	return NULL;
    DumpDocument(Upnp_Buff, (Node *)OperationNode);
  	return Upnp_Buff;
}

EXPORT  Upnp_Void  UpnpPrintDocument(Upnp_Node OperationNode, char * Upnp_Buff)
{
	Upnp_DOMString temp=NULL;
    DumpDocument(temp, (Node *)OperationNode);
  	strcpy(Upnp_Buff, temp);
	delete temp;
}

#ifndef FORCE_NO_EXCEPTIONS

EXPORT  Upnp_Document  UpnpParse_Buffer(char *Buff1)
{
	if(Buff1 == NULL || !strlen(Buff1))
		return NULL;
	
	Document *ret = new Document;
	if(!ret)
	{
	   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	return NULL;
	}  		
	try
	{
		*ret=(*ret).ReadDocumentFileOrBuffer(Buff1, false);
	}
	catch (DOMException& /* toCatch */)
	{
//		DBGONLY(printf("%s\n",toCatch.msg));
		return NULL;
	}
	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

#endif	// FORCE_NO_EXCEPTIONS

EXPORT  Upnp_Document  UpnpParse_BufferNoEx(char *Buff1)
{
	if(Buff1 == NULL || !strlen(Buff1))
		return NULL;
	
	Document *ret;

    if (ED_FAILED(Document::ReadDocumentFileOrBufferNoEx(&ret, Buff1, false)))
		return NULL;

	if(ret->isNull())
	{
		UpnpDocument_free(ret);
		return NULL;
	}
	else
		return((void *)ret);
}

void copyToTarget(char*& target, const char* p)
{
	if(target==NULL)
	{
		target = (char *)malloc(strlen(p)+1);
		if (!target)
        {
		   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
            return;
        }
		strcpy(target, p);
	}
	else
	{
		target = (char *)realloc(target, strlen(target)+strlen(p)+1);
		if (!target)
        {
		   	DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
            return;
        }
		strcat(target,p);
	}
}
//This function takes only a document node
void DumpDocument(char*& target, Node* n)
{
//	char*   nodeName  = n->getNodeName();
//	char*   nodeValue = n->getNodeValue();
	// TODO: check exceptions.

	char*   nodeName;
	char*   nodeValue;
	n->getNodeNameNoEx(&nodeName);
	n->getNodeValueNoEx(&nodeValue);

    static 	int loop=0;

    switch (n->getNodeType())
    {
		case TEXT_NODE:
        {
            copyToTarget(target,nodeValue);
            break;
        }

        case PROCESSING_INSTRUCTION_NODE :
        {
			copyToTarget(target,"<?");
			copyToTarget(target,nodeName);
			copyToTarget(target," ");
			copyToTarget(target,nodeValue);
			copyToTarget(target,"?>\n");
            break;
        }

        case DOCUMENT_NODE :
        {
            Node* child=n->getFirstChild();
			Node *del = child->isNull() ? child : 0;
            Node* temp;
            while(!child->isNull())
            {
                temp = child->getNextSibling();
                loop++;
				DumpDocument(target, child);
				loop--;
                child=temp;
            }
            if (child->isNull())
                delete child;
            delete del;
            break;
        }

        case ELEMENT_NODE :
        {
            // Output the element start tag.
			copyToTarget(target,"<");
			copyToTarget(target,nodeName);
            // Output any attributes on this element
            NamedNodeMap* attributes = n->getAttributes();
            int attrCount = attributes->getLength();
            for (int i = 0; i < attrCount; i++)
            {
                Node*  attribute = attributes->item(i);
//				char *nn=attribute.getNodeName();
				// TODO: check exceptions.
				char *nn;
				char *nv;
				attribute->getNodeNameNoEx(&nn);
				attribute->getNodeValueNoEx(&nv);
				copyToTarget(target,"  ");
				copyToTarget(target,nn);
				copyToTarget(target,"=\"");
				copyToTarget(target,nv);
				copyToTarget(target,"\"");
				delete [] nn;
				delete [] nv;
                delete attribute;
            }
            delete attributes;
			
            //
            //  Test for the presence of children, which includes both
            //  text content and nested elements.
            //
            Node *child, *temp;
			child = n->getFirstChild();
			Node *del=child;
            if (!child->isNull())
            {
                // There are children. Close start-tag, and output children.
                if(child->getNodeType()!=3)
					copyToTarget(target,">\n");
                else
					copyToTarget(target,">");
                while( !child->isNull())
                {
                    temp = child->getNextSibling();
                    loop++;
					DumpDocument(target, child);
					loop--;
                    child=temp;
                }
				if(child->isNull())
                    delete child;

                // Done with children.  Output the end tag.
				copyToTarget(target,"</");
				copyToTarget(target,nodeName);
				copyToTarget(target,">\n");
            }
            else
            {
                //
                //  There were no children.  Output the short form close of the
                //  element start tag, making it an empty-element tag.
                //
                delete del;
				copyToTarget(target,"/>\n");
            }
            break;
        }
        default:
//            DBGONLY(printf("Unrecognized node type = %d\n",n->getNodeType()));
            break;
    }
	//do not delete the topmost node's parent node as it is created by someone else
	//for now dont delete the document node
	delete [] nodeName;
	delete [] nodeValue;
//	if(n->getNodeType()!=DOCUMENT_NODE)
	if(loop!=0)
        delete n;
}

EXPORT char *UpnpCloneDOMString(char *src)
{
	char *ret=NULL;
	if(src == NULL)
		return NULL;
	ret = (Upnp_DOMString)malloc(strlen(src)+1);
	if(!ret)
		return NULL;
	strcpy(ret, src);
	return ret;
}
#endif
