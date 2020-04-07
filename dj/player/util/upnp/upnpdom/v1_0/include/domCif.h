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

//	$Revision: 1.14 $
//	$Date: 2000/08/29 23:58:32 $

/**@name Document Object Level 1 APIs
 * The UPnP SDK for Linux implements part of the Document Object Model
 * Level 1 interface as needed for working with the XML documents required
 * by UPnP.  For those interfaces that are documented in the DOM Level
 * 1 specification, the reader is referred to that document rather than
 * recreating the documentation here.
 *
 * The DOM Level 1 Specification is available from the W3C web site at
 * http://www.w3c.org/TR/1998/REC-DOM-Level-1.
 *
 * Of the interfaces defined in DOM Level 1, the UPnP SDK only implements
 * these interfaces:
 *
 * \begin{itemize}
 *   \item Document: DOM Level 1 specification section 1.2 page 22
 *   \item Element: DOM Level 1 specification section 1.2 page 38
 *   \item NamedNodeMap: DOM Level 1 specification section 1.2 page 32
 *   \item Node: DOM Level 1 specification section 1.2 page 25
 *   \item NodeList: DOM Level 1 specification section 1.2 page 32
 * \end{itemize}
 *
 * {\it Note that all page numbers listed for the DOM Level 1 Specification
 * are relative to the PDF format version dated 1 October 1998.}
 *
 * The prototypes for the methods listed here differ slighly from those
 * in the DOM specification.  The UPnP SDK implements a C-style interface
 * for those methods, requiring the DOM object to be passed as the first
 * parameter.  C++ and other object oriented languages (as the
 * Interface Description Language the interfaces are described in the DOM
 * specification) do not require this.
 */

//@{

#ifndef _domCif_h_
#define _domCif_h_

#include <util/upnp/genlib/noexceptions.h>

#ifdef __cplusplus
extern "C"{
#endif

#include <util/upnp/upnpdom/all.h>
#ifdef  _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define Upnp_DOMString   char *

#define Upnp_Bool int

#define Upnp_Void   void

#define Upnp_UShort unsigned short

#define Upnp_ULong unsigned long

#define Upnp_DocumentFragment void *

#define Upnp_Node void *

#define Upnp_Document void *

#define Upnp_NodeList void *

#define Upnp_CharacterData void *

#define Upnp_Attr void *

#define Upnp_Element void *

#define Upnp_Text void *

#define Upnp_Comment void *

#define Upnp_CDATASection void *

#define Upnp_ProcessingInstruction void *

#define Upnp_EntityReference void *

#define Upnp_DocumentType void *

#define Upnp_NamedNodeMap void *

#define Upnp_DOMImplementation void *

#define Upnp_Notation void *

#define Upnp_Entity void *

#define Upnp_NodeType NODE_TYPE



typedef enum Upnp_DOMException{ 
	NO_ERR=0, 
	INDEX_SIZE_ERR=1, 
	STRING_SIZE_ERR=2, 
	HIERARCHY_REQUEST_ERR=3, 
	WRONG_DOCUMENT_ERR=4, 
	INVALID_CHARACTER_ERR=5, 
	NO_DATA_ALLOWED_ERR=6, 
	NO_MODIFICATION_ALLOWED_ERR=7, 
	NOT_FOUND_ERR=8, 
	NOT_SUPPORTED_ERR=9, 
	INUSE_ATTRIBUTE_ERR=10
} Upnp_DOMException;

/****************************************************************************/
//Interface Node Implementation
/****************************************************************************/

/** @name Interface Node
 * Refer to the DOM Level 1 specification page 25.
 */

//@{

/** Refer to the DOM Level 1 specification page 29. */
EXPORT Upnp_Node UpnpNode_insertBefore(
	Upnp_Node OperationNode, 
	Upnp_Node newChild,      
	Upnp_Node refChild,      
	Upnp_DOMException *err  
        );

/** Refer to the DOM Level 1 specification page 30. */
EXPORT  Upnp_Node UpnpNode_replaceChild(
	Upnp_Node OperationNode, 
	Upnp_Node newChild, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 30. */
EXPORT  Upnp_Node UpnpNode_removeChild(
	Upnp_Node OperationNode, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 30. */
EXPORT  Upnp_Node UpnpNode_appendChild(
	Upnp_Node OperationNode, 
	Upnp_Node newChild, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 31. */
EXPORT  Upnp_Bool UpnpNode_hasChildNodes(
	Upnp_Node OperationNode
	);

/** Refer to the DOM Level 1 specification page 31. */
EXPORT  Upnp_Node UpnpNode_cloneNode(
	Upnp_Node OperationNode, 
	Upnp_Bool deep
	);

/** Refer to the {\tt nodeName} attribute in the DOM Level 1 specification 
 * page 28. 
 */
EXPORT  Upnp_DOMString UpnpNode_getNodeName(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt nodeValue} attribute in the DOM Level 1 specification 
 * page 28. 
 */
EXPORT  Upnp_DOMString UpnpNode_getNodeValue(
	Upnp_Node OperationNode, 
	Upnp_DOMException *err
	);

/** Refer to the {\tt nodeValue} attribute in the DOM Level 1 specification 
 * page 28. 
 */
EXPORT  Upnp_Void UpnpNode_setNodeValue(
	Upnp_Node OperationNode, 
	Upnp_DOMString nodeValue, 
	Upnp_DOMException *err
	);

/** Refer to the {\tt nodeType} attribute in the DOM Level 1 specification 
 * page 28. 
 */
EXPORT  Upnp_UShort UpnpNode_getNodeType(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt parentNode} attribute in the DOM Level 1 specification 
 * page 28. 
 */
EXPORT  Upnp_Node UpnpNode_getParentNode(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt childNodes} attribute in the DOM Level 1 specification 
 * page 29. 
 */
EXPORT  Upnp_NodeList UpnpNode_getChildNodes(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt firstChild} attribute in the DOM Level 1 specification 
 * page 29. 
 */
EXPORT  Upnp_Node UpnpNode_getFirstChild(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt lastChild} attribute in the DOM Level 1 specification 
 * page 29. 
 */
EXPORT  Upnp_Node UpnpNode_getLastChild(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt previousSibling} attribute in the DOM Level 1 
 * specification page 29. 
 */
EXPORT  Upnp_Node UpnpNode_getPreviousSibling(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt nextSibling} attribute in the DOM Level 1 
 * specification page 29. 
 */
EXPORT  Upnp_Node UpnpNode_getNextSibling(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt attributes} attribute in the DOM Level 1 
 * specification page 29. 
 */
EXPORT  Upnp_NamedNodeMap UpnpNode_getAttributes(
	Upnp_Node OperationNode
	);

/** Refer to the {\tt ownerDocument} attribute in the DOM Level 1 
 * specification page 29. 
 */
EXPORT  Upnp_Document   UpnpNode_getOwnerDocument(Upnp_Node OperationNode);

/** Frees the given node object. 
 *
 * @return This method does not return a value. 
 */
EXPORT  Upnp_Void UpnpNode_free(
	Upnp_Node OperationNode /** The node object to free. */
	);

//@} // Interface Node

/***************************************************************************/
//Interface Document Implementation
/***************************************************************************/

/** @name Interface Document
 * Refer to the DOM Level 1 specification page 22.
 */

//@{

/** Refer to the DOM Level 1 specification page 23. */
EXPORT Upnp_Document UpnpDocument_createDocument(
	Upnp_Document OperationDocument
	);

/** Refer to the DOM Level 1 specification page 23. */
EXPORT  Upnp_Element UpnpDocument_createElement(
	Upnp_Document OperationDocument, 
	Upnp_DOMString tagName, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 23. */
EXPORT  Upnp_Text UpnpDocument_createTextNode(
	Upnp_Document OperationDocument, 
	Upnp_DOMString data
	);

/** Refer to the DOM Level 1 specification page 24. */
EXPORT  Upnp_Attr UpnpDocument_createAttribute(
	Upnp_Document OperationDocument, 
	Upnp_DOMString name, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 24. */
EXPORT  Upnp_NodeList UpnpDocument_getElementsByTagName(
	Upnp_Document OperationDocument, 
	Upnp_DOMString tagname
	);

/** Refer to the {\tt doctype} attribute in the DOM Level 1 
 * specification page 22. 
 */
EXPORT  Upnp_DocumentType UpnpDocument_getDoctype(
	Upnp_Document OperationDocument
	);


/** Refer to the {\tt documentElement} attribute in the DOM Level 1 
 * specification page 22. 
 */
EXPORT  Upnp_Element UpnpDocument_getDocumentElement(
	Upnp_Document OperationDocument
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 29. 
 */
EXPORT  Upnp_Node UpnpDocument_insertBefore(
	Upnp_Document OperationDocument, 
	Upnp_Node newChild, 
	Upnp_Node refChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpDocument_replaceChild(
	Upnp_Document OperationDocument, 
	Upnp_Node newChild, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpDocument_removeChild(
	Upnp_Document OperationDocument, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpDocument_appendChild(
	Upnp_Document OperationDocument, 
	Upnp_Node newChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 31. 
 */
EXPORT  Upnp_Bool UpnpDocument_hasChildNodes(
	Upnp_Document OperationDocument
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 31. 
 */
EXPORT  Upnp_Node UpnpDocument_cloneNode(
	Upnp_Document OperationDocument, 
	Upnp_Bool deep
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_DOMString UpnpDocument_getNodeName(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_DOMString UpnpDocument_getNodeValue(
	Upnp_Document OperationDocument, 
	Upnp_DOMException *err
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_Void UpnpDocument_setNodeValue(
	Upnp_Document OperationDocument, 
	Upnp_DOMString nodeValue, 
	Upnp_DOMException *err
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeType} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT Upnp_UShort UpnpDocument_getNodeType(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt parentNode} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_Node UpnpDocument_getParentNode(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt childNodes} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_NodeList UpnpDocument_getChildNodes(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt firstChild} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpDocument_getFirstChild(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt lastChild} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpDocument_getLastChild(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt previousSibling} 
 * attribute in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpDocument_getPreviousSibling(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt nextSibling} attribute 
 * in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpDocument_getNextSibling(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt attributes} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_NamedNodeMap UpnpDocument_getAttributes(
	Upnp_Document OperationDocument
	);

/** Inherited from Interface Node.  Refer to the {\tt ownerDocument} 
 * attribute in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Document UpnpDocument_getOwnerDocument(
	Upnp_Document OperationDocument
	);

/** Frees a document object.
 *
 * @return This method does not return a value.
 */

EXPORT  Upnp_Void UpnpDocument_free(
	Upnp_Document OperationDocument
	);

EXPORT  Upnp_Void UpnpDocumentTree_free(
	Upnp_Document OperationDocument
	);

//@} // Interface Document

/***************************************************************************/
//Interface Element Implementation
/***************************************************************************/

/** @name Interface Element
 *  Refer tot he DOM Level 1 specification page 38.
 */

//@{

/** Refer to the DOM Level 1 specification page 40. */
EXPORT  Upnp_Void UpnpElement_setAttribute(
	Upnp_Element OperationElement, 
	Upnp_DOMString name, 
	Upnp_DOMString value, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 41. */
EXPORT  Upnp_Attr UpnpElement_setAttributeNode(
	Upnp_Element OperationElement, 
	Upnp_Attr newAttr, 
	Upnp_DOMException *err
	);

/** Refer to the DOM Level 1 specification page 41. */
EXPORT  Upnp_NodeList UpnpElement_getElementsByTagName(
	Upnp_Element OperationElement, 
	Upnp_DOMString name
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 29. 
 */
EXPORT  Upnp_Node UpnpElement_insertBefore(
	Upnp_Element OperationElement, 
	Upnp_Node newChild, 
	Upnp_Node refChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpElement_replaceChild(
	Upnp_Element OperationElement, 
	Upnp_Node newChild, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpElement_removeChild(
	Upnp_Element OperationElement, 
	Upnp_Node oldChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 30. 
 */
EXPORT  Upnp_Node UpnpElement_appendChild(
	Upnp_Element OperationElement, 
	Upnp_Node newChild, 
	Upnp_DOMException *err
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 31. 
 */
EXPORT  Upnp_Bool UpnpElement_hasChildNodes(
	Upnp_Element OperationElement
	);

/** Inheritied from Interface Node.  Refer to the DOM Level 1 specification 
 *  page 31. 
 */
EXPORT  Upnp_Node UpnpElement_cloneNode(
	Upnp_Element OperationElement, 
	Upnp_Bool deep);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT Upnp_DOMString UpnpElement_getNodeName(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT Upnp_DOMString UpnpElement_getNodeValue(
	Upnp_Element OperationElement, 
	Upnp_DOMException *err
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeName} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT Upnp_Void UpnpElement_setNodeValue(
	Upnp_Element OperationElement, 
	Upnp_DOMString nodeValue, 
	Upnp_DOMException *err
	);

/** Inherited from Interface Node.  Refer to the {\tt nodeType} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_UShort UpnpElement_getNodeType(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt parentNode} attribute in 
 * the DOM Level 1 specification page 28. 
 */
EXPORT  Upnp_Node UpnpElement_getParentNode(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt childNodes} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_NodeList UpnpElement_getChildNodes(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt firstChild} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpElement_getFirstChild(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt lastChild} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpElement_getLastChild(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt previousSibling} 
 * attribute in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpElement_getPreviousSibling(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt nextSibling} attribute 
 * in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Node UpnpElement_getNextSibling(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt attributes} attribute in 
 * the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_NamedNodeMap UpnpElement_getAttributes(
	Upnp_Element OperationElement
	);

/** Inherited from Interface Node.  Refer to the {\tt ownerDocument} 
 * attribute in the DOM Level 1 specification page 29. 
 */
EXPORT  Upnp_Document UpnpElement_getOwnerDocument(
	Upnp_Element OperationElement
	);

/** Frees an element object.
 *
 * @return This method does not return a value.
 */
EXPORT  Upnp_Void	UpnpElement_free(Upnp_Element OperationElement);

//@} // Interface Element

/****************************************************************************/
//Interface NodeList Implementation
/****************************************************************************/

/** @name Interface NodeList
 * Refer to the DOM Level 1 specification page 32.
 */

//@{

/** Refer to the DOM Level 1 specification page 32. */
EXPORT  Upnp_Node UpnpNodeList_item(
	Upnp_NodeList OperationNodeList, 
	unsigned long index
	);

/** Refer to the {\tt length} attribute in the DOM Level 1 specification 
 * page 32. 
 */
EXPORT  int UpnpNodeList_getLength(
	Upnp_NodeList OperationNodeList
	);

/** Frees a NodeList object.
 *
 * @return This method does not return a value.
 */
EXPORT  Upnp_Void UpnpNodeList_free(
	Upnp_NodeList OperationNodeList
	);

//@} // Interface NodeList

/****************************************************************************/
//Interface NamedNodeMap Implementation
/****************************************************************************/

/** @name Interface NamedNodeMap
 * Refer to the DOM Level 1 specification page 32.
 */

//@{

/** Refer to the DOM Level 1 specification page 33. */
EXPORT  Upnp_Node UpnpNamedNodeMap_getNamedItem(
	Upnp_NamedNodeMap OperationNamedNodeMap, 
	Upnp_DOMString name
	);

/** Refer to the DOM Level 1 specification page 34. */
EXPORT  Upnp_Node UpnpNamedNodeMap_item(
	Upnp_NamedNodeMap OperationNamedNodeMap, 
	unsigned long index
	);

/** Refer to the {\tt length} attribute in the DOM Level 1 specification 
 * page 39. 
 */
EXPORT  Upnp_ULong UpnpNamedNodeMap_getLength(
	Upnp_NamedNodeMap OperationNamedNodeMap
	);

/** Frees a NamedNodeMap object.
 *
 * @return This method does not return a value.
 */
EXPORT  Upnp_Void UpnpNamedNodeMap_free(
	Upnp_NamedNodeMap OperationNamedNodeMap
	);

//@} // Interface NamedNodeMap

/****************************************************************************/
//Utils
/****************************************************************************/

/** @name Utility functions
 *  The functions in this section are not defined by the DOM Level 1
 *  specification but are useful in dealing with DOM documents.
 */

//@{

/**{\bf UpnpNewPrintDocument} "prints" or renders a DOM object back into an
 * XML text buffer.  The caller is responsible for freeing the returned
 * DOMString using {\bf Upnpfree} when finished.
 *
 * @return A string buffer containing the rendered XML document.
 */
EXPORT  Upnp_DOMString UpnpNewPrintDocument(
	Upnp_Node OperationNode /** The DOM object to render to XML. */
	);
	
/** {\bf UpnpPrintDocument} is obsolete and may be removed in future versions.
 * Use {\bf UpnpNewPrintDocument} instead.
 */	
EXPORT  Upnp_Void UpnpPrintDocument(
	Upnp_Node OperationNode, /** The DOM object to render to XML. */
	char * Upnp_Buff         /** Pointer to a buffer to store the XML. */
	);

/** {\bf UpnpParse_buffer} parses an XML buffer, returning a DOM Document.
 *
 * @return A DOM document representation of the XML buffer.
 */		
#ifndef FORCE_NO_EXCEPTIONS
EXPORT  Upnp_Document UpnpParse_Buffer(
	char *Buff  	/** The buffer containing the XML to parse. */
	);
#endif	// FORCE_NO_EXCEPTIONS

EXPORT  Upnp_Document UpnpParse_BufferNoEx(
	char *Buff  	/** The buffer containing the XML to parse. */
	);

/** {\bf Upnpfree} frees Upnp_DOMString buffers.  It should not be used to
 *  free any other type of memory.
 *
 * @return This function does not return a value.
 */

EXPORT  Upnp_Void Upnpfree(
	Upnp_DOMString in
	);

/** {\bf UpnpParseFileAndGetDocument} parses an XML file, returning a DOM Document.
 *
 * @return A DOM document representation of the XML file.
 */		
#ifndef FORCE_NO_EXCEPTIONS
EXPORT  Upnp_Document  UpnpParseFileAndGetDocument(
	char* xmlFile	/** The name of the XML file to parse. */
	);
#endif	// FORCE_NO_EXCEPTIONS

/** {\bf UpnpCloneDOMString} clones a DOM String.
 *
 * @return the cloned DOM string.
 */		
EXPORT char *UpnpCloneDOMString(char *src);

//@} Utility functions

//@} DOM APIs

#ifdef OBSOLETE
//typedef enum { ELEMENT_NODE=1, ATTRIBUTE_NODE=2, TEXT_NODE=3, CDATA_SECTION_NODE=4, ENTITY_REFERENCE_NODE=5, ENTITY_NODE=6, PROCESSING_INSTRUCTION_NODE=7, COMMENT_NODE=8, DOCUMENT_NODE=9, DOCUMENT_TYPE_NODE=10, DOCUMENT_FRAGMENT_NODE=11, NOTATION_NODE=12 } Upnp_NodeType;

/*******************************************************************************************************************************/
//Interface DOM Implementation
/*******************************************************************************************************************************/
//Upnp_Bool Upnp_DOMImplementation_hasFeature( const Upnp_DOMString & feature,const Upnp_DOMString & version);

//#define Upnp_DOMException1 Upnp_DOMException

#endif

#ifdef __cplusplus
}
#endif
#endif
