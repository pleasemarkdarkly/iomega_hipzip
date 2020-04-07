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

//	$Revision: 1.20 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/Document.h>
#include <stdio.h>
#include <stdlib.h>

#include <cyg/infra/diag.h>
#include <util/debug/debug.h>

#define DEBUG_DELETE(x...)
//#define DEBUG_DELETE(x...) diag_printf(##x)


Document::Document()
{
	this->nact=NULL;
}

Document::~Document()
{
}

void 	Document::createDocument(Document **returnDoc)
{
	*returnDoc = new Document;
	if(!(*returnDoc))
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	(*returnDoc)->nact= new NodeAct(DOCUMENT_NODE, "#document",NULL, (Node*)returnDoc);
	(*returnDoc)->nact->OwnerNode=(*returnDoc)->nact;
	(*returnDoc)->nact->RefCount++;
}

Document* 	Document::createDocument()
{
	Document *returnDoc;
	returnDoc = new Document;
	if(!returnDoc)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnDoc->nact= new NodeAct(DOCUMENT_NODE, "#document",NULL, (Node*)returnDoc);
	if(!(returnDoc->nact))
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	returnDoc->nact->OwnerNode=returnDoc->nact;
	returnDoc->nact->RefCount++;
	return returnDoc;
}
	
	
Attr*	Document::createAttribute(char *name)
{
	Attr *returnAttr;
	returnAttr = new Attr(name);
	if(!returnAttr)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return returnAttr;
}

Attr* 	Document::createAttribute(char *name, char *value)
{
	Attr *returnAttr;
	returnAttr = new Attr(name, value);
	if(!returnAttr)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return returnAttr;
}

Element* Document::createElement(char *tagName)
{
	Element *returnElement;
	returnElement = new Element(tagName);
	if(!returnElement)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	return returnElement;
}

Node* 	Document::createTextNode(char *data)
{
	Node *returnNode;
    Node::createNode(&returnNode,TEXT_NODE,"#text",data);
	return returnNode;
}

#ifndef FORCE_NO_EXCEPTIONS
	
Document& Document::ReadDocumentFileOrBuffer(char * xmlFile, bool file)
{
	//to do: try handling this in a better fashin than allocating a static length..
	char *fileBuffer;
	FILE *XML;
	Document *RootDoc;
	NODE_TYPE NodeType;
	char *NodeName=NULL;
	char *NodeValue=NULL;
	bool IsEnd;
	bool IgnoreWhiteSpace=true;
	Node *a;
	Node *b;
	Element *back;

	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Inside ReadDocumentFileOrBuffer function\n");)
	if(file)
	{
        	XML=fopen(xmlFile, "r");
        	if(!XML)
        	{
       			DBGONLY(UpnpPrintf(UPNP_INFO,DOM,__FILE__,__LINE__,"%s - No Such File Or Directory \n", xmlFile);)
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        		throw DOMException(DOMException::NO_SUCH_FILE, NULL);
        	}
        	else{
	       			fseek(XML,0, SEEK_END);
	       			int fz = ftell(XML);
	       			fileBuffer = (char *)malloc(fz+2);
	       			if(fileBuffer == NULL)
	       			{
       					DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
						diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
	       				throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
	       			}
	       			fseek(XML,0, SEEK_SET);
	       			int sizetBytesRead = fread (fileBuffer, 1, fz, XML);
        			fileBuffer[sizetBytesRead] = '\0'; // append null
        			fclose (XML);
        	}
 	}
 	else
 	{
 		fileBuffer=(char *)malloc(strlen(xmlFile)+1);
		if(fileBuffer == NULL)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
			throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
		}
		strcpy(fileBuffer, xmlFile);
 	}
    try
    { 		   	
	Parser myparser(fileBuffer);
	free(fileBuffer);
   	createDocument(&RootDoc);
   	RootDoc->CurrentNodePtr=RootDoc->nact;
   	back=NULL;
	while(1)
	{
		Element ele;
		Attr attr;
		try
		{
    		if(myparser.getNextNode(NodeType, &NodeName, &NodeValue, IsEnd, IgnoreWhiteSpace)==0)
    		{
				DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"NextNode while parsing Nodetype %d, Nodename %s, Nodevalue %s\n", (int)NodeType, NodeName, NodeValue);)
        		if(!IsEnd)
        		{
        			switch(NodeType)
        			{
        			case ELEMENT_NODE:	
        								ele=createElement(NodeName);
        								a=(Node*)&ele;
        								RootDoc->CurrentNodePtr->appendChild(a->nact);
        								RootDoc->CurrentNodePtr=RootDoc->CurrentNodePtr->LastChild;
        								delete ele.ownerElement;
        								break;
        			case TEXT_NODE:		
        								Node::createNode(&b,NodeType,NodeName,NodeValue);
        								RootDoc->CurrentNodePtr->appendChild(b->nact);
        								delete b;
        								break;
        			case ATTRIBUTE_NODE:
        								attr=createAttribute(NodeName, NodeValue);
        								a=(Node*)&attr;
        								RootDoc->CurrentNodePtr->appendChild(a->nact);
        								delete attr.ownerAttr;
        								break;
        			case INVALID_NODE:
        								break;
        			default:   break;
        			}
        		}
        		//Throw exception if it is an invalid document
        		else
        		{
        			if(!strcmp(NodeName, RootDoc->CurrentNodePtr->NA_NodeName))
        			{
        				RootDoc->CurrentNodePtr = RootDoc->CurrentNodePtr->ParentNode;
        			}
        			else
        			{
        				RootDoc->deleteDocumentTree();
						DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"fatal error during parsing\n");)       				
						diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
        			}
        		}
        		if(NodeType==INVALID_NODE)
        			break;
        		else
        			NodeType=INVALID_NODE;
        		if(NodeName)
        		{										
        			delete NodeName;
        			NodeName=NULL;
        		}
        		if(NodeValue)
        		{
        			delete NodeValue;
        			NodeValue=NULL;
        		}
        		IsEnd=false;
      		}
      		else
      		{
    			RootDoc->deleteDocumentTree();
				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
    			throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
    		}
  		}
  		catch(DOMException& toCatch)
  		{
   			RootDoc->deleteDocumentTree();
			delete RootDoc;
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
   			throw DOMException(toCatch.code);
   		}
 	}
   	}catch(DOMException& toCatch)
   	{
   	    RootDoc=NULL;
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
	    throw DOMException(toCatch.code);
	}
	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"**EndParse**\n");)
	return *RootDoc;
}

#endif	// FORCE_NO_EXCEPTIONS

EDERRCODE Document::ReadDocumentFileOrBufferNoEx(Document** ppRootDoc, char * xmlFile, bool file)
{
	//to do: try handling this in a better fashin than allocating a static length..
	char *fileBuffer;
	FILE *XML;
	Document *RootDoc;
	NODE_TYPE NodeType;
	char *NodeName=NULL;
	char *NodeValue=NULL;
	bool IsEnd;
	bool IgnoreWhiteSpace=true;
	Node *b;
	Element *back;
	EDERRCODE eec = ED_OK;

	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Inside ReadDocumentFileOrBuffer function\n");)
	if(file)
	{
        	XML=fopen(xmlFile, "r");
        	if(!XML)
        	{
       			DBGONLY(UpnpPrintf(UPNP_INFO,DOM,__FILE__,__LINE__,"%s - No Such File Or Directory \n", xmlFile);)
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//        		throw DOMException(DOMException::NO_SUCH_FILE, NULL);
                DEBUG_THROW_FILE_NOT_FOUND_EXCEPTION(xmlFile);
                return EDERR_FILE_NOT_FOUND_EXCEPTION;
        	}
        	else
			{
	       			fseek(XML,0, SEEK_END);
	       			int fz = ftell(XML);
//	       			fileBuffer = (char *)malloc(fz+2);
	       			fileBuffer = new char[fz+2];
	       			if(fileBuffer == NULL)
	       			{
       					DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
//						diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//	       				throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
				   	    RootDoc=NULL;
						*ppRootDoc = NULL;
						DEBUG_THROW_DOM_INSUFFICIENT_MEMORY_EXCEPTION;
						return EDERR_DOM_INSUFFICIENT_MEMORY_EXCEPTION;
	       			}
	       			fseek(XML,0, SEEK_SET);
	       			int sizetBytesRead = fread (fileBuffer, 1, fz, XML);
        			fileBuffer[sizetBytesRead] = '\0'; // append null
        			fclose (XML);
        	}
 	}
 	else
 	{
		int iLen = strlen(xmlFile);
		fileBuffer = new char[iLen + 1];
//		fileBuffer = (char *)malloc(iLen + 1);
// 		fileBuffer=(char *)malloc(strlen(xmlFile)+1);
		if(fileBuffer == NULL)
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)
	   	    RootDoc=NULL;
			*ppRootDoc = NULL;
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw DOMException(DOMException::INSUFFICIENT_MEMORY, NULL);
			DEBUG_THROW_DOM_INSUFFICIENT_MEMORY_EXCEPTION;
			return EDERR_DOM_INSUFFICIENT_MEMORY_EXCEPTION;
		}
		strcpy(fileBuffer, xmlFile);
 	}
	Parser myparser(fileBuffer, false);
//	free(fileBuffer);
   	createDocument(&RootDoc);
   	RootDoc->CurrentNodePtr=RootDoc->nact;
   	back=NULL;
	while(1)
	{
		Element* ele;
		Attr* attr;
// 		if(myparser.getNextNode(NodeType, &NodeName, &NodeValue, IsEnd, IgnoreWhiteSpace)==0)
		int oldretval;
   		if (ED_FAILED(myparser.getNextNodeNoEx(NodeType, &NodeName, &NodeValue, IsEnd, IgnoreWhiteSpace,oldretval)))
		{
			delete RootDoc;
			RootDoc=NULL;
			*ppRootDoc = 0;
			DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
		}
		if (oldretval == 0)
		{
			DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"NextNode while parsing Nodetype %d, Nodename %s, Nodevalue %s\n", (int)NodeType, NodeName, NodeValue);)
        	if(!IsEnd)
        	{
        		switch(NodeType)
        		{
					case ELEMENT_NODE:	
						ele=createElement(NodeName);
//						RootDoc->CurrentNodePtr->appendChildNoEx(a->nact);
						if (ED_FAILED(eec = RootDoc->CurrentNodePtr->appendChildNoEx(ele->nact)))
							goto CatchError;
						RootDoc->CurrentNodePtr=RootDoc->CurrentNodePtr->LastChild;
						delete ele;
						break;
					case TEXT_NODE:		
						Node::createNode(&b,NodeType,NodeName,NodeValue);
//						RootDoc->CurrentNodePtr->appendChildNoEx(b->nact);
						if (ED_FAILED(eec = RootDoc->CurrentNodePtr->appendChildNoEx(b->nact)))
							goto CatchError;
						delete b;
						break;
					case ATTRIBUTE_NODE:
						attr=createAttribute(NodeName, NodeValue);
//						RootDoc->CurrentNodePtr->appendChildNoEx(a->nact);
						if (ED_FAILED(eec = RootDoc->CurrentNodePtr->appendChildNoEx(attr->nact)))
							goto CatchError;
						delete attr;
						break;
					case INVALID_NODE:
						break;
					default:
						break;
        		}
        	}
        	//Throw exception if it is an invalid document
        	else
        	{
        		if(!strcmp(NodeName, RootDoc->CurrentNodePtr->NA_NodeName))
        		{
        			RootDoc->CurrentNodePtr = RootDoc->CurrentNodePtr->ParentNode;
        		}
        		else
        		{
					DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"fatal error during parsing\n");)       				
					delete RootDoc;
					RootDoc=NULL;
					*ppRootDoc = 0;
					DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
					return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
//					diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//  				throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
        		}
        	}
        	if(NodeType==INVALID_NODE)
        		break;
        	else
        		NodeType=INVALID_NODE;
        	if(NodeName)
        	{										
        		free(NodeName);
        		NodeName=NULL;
        	}
        	if(NodeValue)
        	{
        		free(NodeValue);
        		NodeValue=NULL;
        	}
        	IsEnd=false;
      	}
      	else
      	{
			delete RootDoc;
			RootDoc = NULL;
			*ppRootDoc = 0;
			DEBUG_THROW_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
			return EDERR_DOM_FATAL_ERROR_DURING_PARSING_EXCEPTION;
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
    	}
  	}

	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"**EndParse**\n");)
	*ppRootDoc = RootDoc;
	return ED_OK;

CatchError:
	delete RootDoc;
//	diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//	throw DOMException(toCatch.code);

    RootDoc = NULL;
	*ppRootDoc = 0;
	return eec;

}

Document& Document::operator = (const Document &other)
{
    if (this->nact != other.nact)
    {
        this->nact = other.nact;
		this->nact->RefCount++;
    }
    return *this;
};

NodeList* Document::getElementsByTagName( char * tagName)
{
	NodeList *returnNodeList;
	returnNodeList= new NodeList;
	if(!returnNodeList)
		{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	DBGONLY(UpnpPrintf(UPNP_ALL,DOM,__FILE__,__LINE__,"Calling makeNodeList\n");)
	MakeNodeList(this, tagName, &returnNodeList);
	return returnNodeList;
}

// A recursive function to make a nodelist from the tree.
void 	Document::MakeNodeList(Node* n, char* match, NodeList **nl)
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

#endif
