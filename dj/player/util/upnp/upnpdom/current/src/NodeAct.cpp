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

//	$Revision: 1.16 $
//	$Date: 2000/10/06 16:03:49 $
#include <util/upnp/api/config.h>
#if EXCLUDE_DOM == 0
#include <util/upnp/upnpdom/NodeAct.h>

#include <cyg/infra/diag.h>
#include <util/debug/debug.h>



NodeAct::NodeAct(NODE_TYPE nt,char *NodeName, char *NodeValue, Node *myCreator)
{
#ifdef DEBUG_CLEANUP
	m_bDeleted = false;
#endif
	if(NodeName !=NULL)
	{
		NA_NodeName=new char[strlen(NodeName)+1];
		if(!NA_NodeName)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		strcpy(NA_NodeName, NodeName);
	}
	else
		NA_NodeName =NULL;
	if(NodeValue !=NULL)
	{
		NA_NodeValue=new char[strlen(NodeValue)+1];
		if(!NA_NodeValue)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		strcpy(NA_NodeValue, NodeValue);
	}
	else
		NA_NodeValue =NULL;
	NA_NodeType = nt;
	ParentNode=NULL;
	OwnerNode=this;
	NextSibling=NULL;
	PrevSibling=NULL;
	FirstChild=NULL;
	LastChild=NULL;
	FirstAttr=NULL;
	LastAttr=NULL;
	Creator=myCreator;
	RefCount=0;
}

NodeAct::NodeAct(const NodeAct &other, bool deep)
{
#ifdef DEBUG_CLEANUP
	m_bDeleted = false;
#endif
	this->setName(other.NA_NodeName);
	this->setValue(other.NA_NodeValue);
	this->NA_NodeType=other.NA_NodeType;
    this->Creator = other.Creator;
	this->OwnerNode=other.OwnerNode;
    this->RefCount=1;
    // Need to break the association w/ original kids
    this->PrevSibling = NULL;
    this->NextSibling = NULL;
    this->ParentNode = NULL;
    this->FirstChild = NULL;
    this->LastChild = NULL;
	this->FirstAttr = NULL;
	this->LastAttr = NULL;

    // Then, if deep, clone the kids too.
    if (deep)
    {
        for (NodeAct *mykid = other.FirstChild;
        mykid != NULL;
        mykid = mykid->NextSibling)
//			this->appendChild(mykid->cloneNode(true));
			// TODO: Technically this can throw an exception that will be caught.
			this->appendChildNoEx(mykid->cloneNode(true));
    }
}

//Returns true if it finds the node in the tree strating from the node under reference
//Searches children as well as attribute nodes.
bool NodeAct::findNode(NodeAct *find)
{
	bool notFound=true;
	
	if(find!=NULL)
	{
		//check the children
		NodeAct *na;
		na=this->FirstChild;
		while(na !=NULL)
		{
			if(na == find)
			{
				notFound=false;
				break;
			}
			na = na->NextSibling;
		}
		//check the attributes if the parent is an element
		if(this->NA_NodeType == ELEMENT_NODE)
		{
			na = this->FirstAttr;
			while(na !=NULL)
			{
				if(na == find)
				{
					notFound=false;
					break;
				}
				na = na->NextSibling;
			}
		}
		return(!notFound);
	}
	else return(false);
}

//Returns true if it finds the node in the whole tree strating from the "from" node
//Searches children as well as attribute nodes.
bool NodeAct::findNodeFromRef(NodeAct *from, NodeAct *find)
{
	static bool Found=false;
	if (!from)
		return false;
    for (NodeAct *mykid = from->FirstChild; mykid != NULL; mykid = mykid->NextSibling)
	{
		findNodeFromRef(mykid, find);
		if(from->findNode(find))
		{
			Found=true;
			return(Found);
		}
	}
	return Found;
}

#ifndef FORCE_NO_EXCEPTIONS

void NodeAct::insertBefore(NodeAct *newChild, NodeAct *refChild)
{
	if(refChild!=NULL)
	{
		//Makesure that the ref child is found
		//Otherwise raise exception NOT_FOUND_ERR
		if(!findNode(refChild))
		{
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
			throw DOMException(DOMException::NOT_FOUND_ERR);
			return;
		}
		if(findNode(newChild))
		{
			removeChild(newChild);
			newChild->NextSibling =NULL;
			newChild->PrevSibling =NULL;
		}
		//Todo: Raise exception for if the node is one of the ancestors, etc..
		newChild->RefCount++;

		newChild->NextSibling=refChild;
		if(refChild->PrevSibling !=NULL)
			refChild->PrevSibling->NextSibling=newChild;
		newChild->NextSibling=refChild;
		refChild->PrevSibling=newChild;
		if((newChild->NA_NodeType != ATTRIBUTE_NODE)&&(newChild->PrevSibling==NULL))
			this->FirstChild=newChild;
		else if((newChild->NA_NodeType == ATTRIBUTE_NODE)&&(newChild->PrevSibling==NULL))
			this->FirstAttr=newChild;
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		//Todo: Atrributes owner node must be changed to newchilds owner node
	}
	else
		appendChild(newChild);
}

void NodeAct::replaceChild(NodeAct *newChild, NodeAct *oldChild)
{
	if(oldChild!=NULL)
	{
		//Makesure that the ref child is found
		//Otherwise raise exception NOT_FOUND_ERR
		if(!findNode(oldChild))
		{
			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
			throw DOMException(DOMException::NOT_FOUND_ERR);
			return;
		}
		insertBefore(newChild, oldChild);
		removeChild(oldChild);
	}
}

void NodeAct::removeChild(NodeAct *oldChild)
{
	//Makesure that the ref child is found
	//Otherwise raise exception NOT_FOUND_ERR
	if(!findNode(oldChild))
	{
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
		throw DOMException(DOMException::NOT_FOUND_ERR);
		return;
	}
	if(oldChild->PrevSibling !=NULL)
   		oldChild->PrevSibling->NextSibling=oldChild->NextSibling;
   	if(oldChild->NextSibling !=NULL)
   		oldChild->NextSibling->PrevSibling=oldChild->PrevSibling;
   	if(this->FirstChild==oldChild)
   		this->FirstChild=oldChild->NextSibling;
   	if(this->LastChild ==oldChild)
   		this->LastChild=oldChild->PrevSibling;
   	oldChild->OwnerNode=oldChild;
   	oldChild->ParentNode=NULL;
   	oldChild->RefCount=1;
}

//Appends the child to the node.
//If the child exists before its first removed
//If the child has children all of them will be appended
void NodeAct::appendChild(NodeAct *newChild)
{
	if(findNodeFromRef(this->OwnerNode,newChild))
	{
		newChild->ParentNode->removeChild(newChild);
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		newChild->NextSibling =NULL;
		newChild->PrevSibling =NULL;
	}
	else
	{
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		newChild->NextSibling =NULL;
		newChild->PrevSibling =NULL;
	}
	newChild->RefCount++;
	if(newChild->NA_NodeType != ATTRIBUTE_NODE)
	{
		//if this is the first child
		if(this->FirstChild ==NULL)
		{
			this->FirstChild = newChild;
			this->LastChild = newChild;
		}
		else
		{
			this->LastChild->NextSibling=newChild;
			newChild->PrevSibling=this->LastChild;
			this->LastChild=newChild;
		}
	}
	else
	{
		//if this is the first attribute
		if(this->FirstAttr ==NULL)
		{
			this->FirstAttr = newChild;
			this->LastAttr = newChild;
		}
		else
		{
			this->LastAttr->NextSibling=newChild;
			newChild->PrevSibling=this->LastAttr;
			this->LastAttr=newChild;
		}
	}
}

#endif	// FORCE_NO_EXCEPTIONS


EDERRCODE NodeAct::insertBeforeNoEx(NodeAct *newChild, NodeAct *refChild)
{
	if(refChild!=NULL)
	{
		//Makesure that the ref child is found
		//Otherwise raise exception NOT_FOUND_ERR
		if(!findNode(refChild))
		{
			DEBUG_THROW_DOM_NOT_FOUND_ERR_EXCEPTION;
			return EDERR_DOM_NOT_FOUND_ERR_EXCEPTION;
		}
		if(findNode(newChild))
		{
			ED_RETURN_EXCEPTION(removeChildNoEx(newChild));
			newChild->NextSibling =NULL;
			newChild->PrevSibling =NULL;
		}
		//Todo: Raise exception for if the node is one of the ancestors, etc..
		newChild->RefCount++;

		newChild->NextSibling=refChild;
		if(refChild->PrevSibling !=NULL)
			refChild->PrevSibling->NextSibling=newChild;
		newChild->NextSibling=refChild;
		refChild->PrevSibling=newChild;
		if((newChild->NA_NodeType != ATTRIBUTE_NODE)&&(newChild->PrevSibling==NULL))
			this->FirstChild=newChild;
		else if((newChild->NA_NodeType == ATTRIBUTE_NODE)&&(newChild->PrevSibling==NULL))
			this->FirstAttr=newChild;
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		//Todo: Atrributes owner node must be changed to newchilds owner node
	}
	else
		ED_RETURN_EXCEPTION(appendChildNoEx(newChild));

	return ED_OK;
}

EDERRCODE NodeAct::replaceChildNoEx(NodeAct *newChild, NodeAct *oldChild)
{
	if(oldChild!=NULL)
	{
		//Makesure that the ref child is found
		//Otherwise raise exception NOT_FOUND_ERR
		if(!findNode(oldChild))
		{
			DEBUG_THROW_DOM_NOT_FOUND_ERR_EXCEPTION;
			return EDERR_DOM_NOT_FOUND_ERR_EXCEPTION;
		}
		ED_RETURN_EXCEPTION(insertBeforeNoEx(newChild, oldChild));
		ED_RETURN_EXCEPTION(removeChildNoEx(oldChild));
	}
	return ED_OK;
}

EDERRCODE NodeAct::removeChildNoEx(NodeAct *oldChild)
{
	//Makesure that the ref child is found
	//Otherwise raise exception NOT_FOUND_ERR
	if(!findNode(oldChild))
	{
		DEBUG_THROW_DOM_NOT_FOUND_ERR_EXCEPTION;
		return EDERR_DOM_NOT_FOUND_ERR_EXCEPTION;
	}
	if(oldChild->PrevSibling !=NULL)
   		oldChild->PrevSibling->NextSibling=oldChild->NextSibling;
   	if(oldChild->NextSibling !=NULL)
   		oldChild->NextSibling->PrevSibling=oldChild->PrevSibling;
   	if(this->FirstChild==oldChild)
   		this->FirstChild=oldChild->NextSibling;
   	if(this->LastChild ==oldChild)
   		this->LastChild=oldChild->PrevSibling;
   	oldChild->OwnerNode=oldChild;
   	oldChild->ParentNode=NULL;
   	oldChild->RefCount=1;

	return ED_OK;
}

//Appends the child to the node.
//If the child exists before its first removed
//If the child has children all of them will be appended
EDERRCODE NodeAct::appendChildNoEx(NodeAct *newChild)
{
	if(findNodeFromRef(this->OwnerNode,newChild))
	{
		ED_RETURN_EXCEPTION(newChild->ParentNode->removeChildNoEx(newChild));
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		newChild->NextSibling =NULL;
		newChild->PrevSibling =NULL;
	}
	else
	{
		newChild->ParentNode=this;
		newChild->OwnerNode =this->OwnerNode;
		newChild->NextSibling =NULL;
		newChild->PrevSibling =NULL;
	}
	newChild->RefCount++;
	if(newChild->NA_NodeType != ATTRIBUTE_NODE)
	{
		//if this is the first child
		if(this->FirstChild ==NULL)
		{
			this->FirstChild = newChild;
			this->LastChild = newChild;
		}
		else
		{
			this->LastChild->NextSibling=newChild;
			newChild->PrevSibling=this->LastChild;
			this->LastChild=newChild;
		}
	}
	else
	{
		//if this is the first attribute
		if(this->FirstAttr ==NULL)
		{
			this->FirstAttr = newChild;
			this->LastAttr = newChild;
		}
		else
		{
			this->LastAttr->NextSibling=newChild;
			newChild->PrevSibling=this->LastAttr;
			this->LastAttr=newChild;
		}
	}
	return ED_OK;
}

NodeAct * NodeAct::cloneNode(bool deep)
{
    NodeAct *newnode;
    newnode = new NodeAct(*this, deep);
	if(!newnode)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
    return newnode;
}


void NodeAct::setName(char *n)
{
	if(n!=NULL)
	{
		NA_NodeName=new char[strlen(n)+1];
		if(!NA_NodeName)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		strcpy(NA_NodeName, n);
	}
	else
		NA_NodeName=NULL;
}

void NodeAct::setValue(char *v)
{
	if(v!=NULL)
	{
		NA_NodeValue=new char[strlen(v)+1];
		if(!NA_NodeValue)
		   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
		strcpy(NA_NodeValue, v);
	}
	else
		NA_NodeValue=NULL;
}

NodeAct::~NodeAct()
{
#ifdef DEBUG_CLEANUP
	if (m_bDeleted == true)
	{
		diag_printf("@@@ OH SHIT!!! Deleting already deleted node!\n");
	}
#endif

	while(this->FirstChild!=NULL)//delete all except itself
	{
		deleteNodeTree(this);
	}

	if(NA_NodeName!=NULL)
		delete [] NA_NodeName;
	if(NA_NodeValue!=NULL)
		delete [] NA_NodeValue;
//	deleteNodeTree(this);
#ifdef DEBUG_CLEANUP
	m_bDeleted = true;
#endif
}


void NodeAct::deleteNodeTree(NodeAct *na)
{
//recurse through the entire tree and delete the leaf
	if(na->FirstChild != NULL)
	{
		na =na->FirstChild;
		deleteNodeTree(na);
	}
	else
	{
		NodeAct *nap;
		nap=na->ParentNode;
		if(nap != NULL)
		{
			nap->FirstChild=na->NextSibling;//Point the parent to the next sibling
			nap->LastChild=NULL;//dont care
		}
		//delete all the attributes if present in the element node
		if(na->NA_NodeType == ELEMENT_NODE)
		{
			NodeAct *attr;
			attr=na->FirstAttr;
			while(attr!= NULL){
				NodeAct *na1;
				na1=attr->NextSibling;
				delete attr;
				attr =na1;
			}
			na->FirstAttr=NULL;
			na->LastAttr=NULL;
		}
		if(na!=this)
		{
			delete na;
		}
	}
}
#endif
