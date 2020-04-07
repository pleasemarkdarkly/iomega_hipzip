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
//
// $Revision: 1.18 $
// $Date: 2000/09/28 23:42:11 $
//


#include <util/upnp/api/config.h>

#include <util/upnp/api/upnptools.h>

#include <util/upnp/genlib/http_client.h>

#include <stdarg.h>

#define HEADER_LENGTH 2000

struct ErrorString
{
	int rc;
	const char *rcError;

};


struct ErrorString ErrorMessages[] = { {UPNP_E_SUCCESS, "UPNP_E_SUCCESS"},
			{UPNP_E_INVALID_HANDLE, "UPNP_E_INVALID_HANDLE"},
			{UPNP_E_INVALID_PARAM,  "UPNP_E_INVALID_PARAM"},
			{UPNP_E_OUTOF_HANDLE,  "UPNP_E_OUTOF_HANDLE"},
			{UPNP_E_OUTOF_CONTEXT, "UPNP_E_OUTOF_CONTEXT"},
			{UPNP_E_OUTOF_MEMORY,   "UPNP_E_OUTOF_MEMOR"},
			{UPNP_E_INIT, "UPNP_E_INIT"},
			{UPNP_E_BUFFER_TOO_SMALL, "UPNP_E_BUFFER_TOO_SMALL"},
			{UPNP_E_INVALID_DESC, "UPNP_E_INVALID_DESC"},
			{UPNP_E_INVALID_URL, "UPNP_E_INVALID_URL"},
			{UPNP_E_INVALID_SID, "UPNP_E_INVALID_SID"},
			{UPNP_E_INVALID_DEVICE, "UPNP_E_INVALID_DEVICE"},
			{UPNP_E_INVALID_SERVICE, "UPNP_E_INVALID_SERVICE"},
			{UPNP_E_BAD_RESPONSE, "UPNP_E_BAD_RESPONSE"},
			{UPNP_E_BAD_REQUEST, "UPNP_E_BAD_REQUEST"},
			{UPNP_E_INVALID_ACTION, "UPNP_E_INVALID_ACTION"},
			{UPNP_E_FINISH, "UPNP_E_FINISH"},
			{UPNP_E_INIT_FAILED, "UPNP_E_INIT_FAILED"},
			{UPNP_E_NETWORK_ERROR, "UPNP_E_NETWORK_ERROR"},
			{UPNP_E_SOCKET_WRITE, "UPNP_E_SOCKET_WRITE"},
			{UPNP_E_SOCKET_READ, "UPNP_E_SOCKET_READ"},
			{UPNP_E_SOCKET_BIND, "UPNP_E_SOCKET_BIND"},
			{UPNP_E_SOCKET_CONNECT, "UPNP_E_SOCKET_CONNECT"},
			{UPNP_E_OUTOF_SOCKET, "UPNP_E_OUTOF_SOCKET"},
			{UPNP_E_LISTEN, "UPNP_E_LISTEN"},
			{UPNP_E_EVENT_PROTOCOL, "UPNP_E_EVENT_PROTOCOL"},
			{UPNP_E_SUBSCRIBE_UNACCEPTED, "UPNP_E_SUBSCRIBE_UNACCEPTED"},
			{UPNP_E_UNSUBSCRIBE_UNACCEPTED, "UPNP_E_UNSUBSCRIBE_UNACCEPTED"},
			{UPNP_E_NOTIFY_UNACCEPTED, "UPNP_E_NOTIFY_UNACCEPTED"},
			{UPNP_E_INTERNAL_ERROR,"UPNP_E_INTERNAL_ERROR"},
			{UPNP_E_INVALID_ARGUMENT,"UPNP_E_INVALID_ARGUMENT"}
 			};

const char *UpnpGetErrorMessage( int rc)
{
  int i;
	
  for ( i=0; i< sizeof( ErrorMessages)/sizeof(ErrorMessages[0]); i++)
    {
      if ( rc == ErrorMessages[i].rc)
	return ErrorMessages[i].rcError;
		  
    }
  
  return "Unknown Error";

}			

int UpnpResolveURL(IN char * BaseURL,
			  IN char * RelURL,
			  OUT char * AbsURL)
{
  // There is some unnecessary allocation and
  // deallocation going on here because of the way
  // resolve_rel_url was originally written and used
  // in the future it would be nice to clean this up

  char * tempRel;

  if (RelURL==NULL)
    return UPNP_E_INVALID_PARAM;
  
  tempRel= new char[strlen(RelURL)+1];

  if (tempRel==NULL)
    return UPNP_E_OUTOF_MEMORY;
  
  strcpy(tempRel,RelURL);

  tempRel=resolve_rel_url(BaseURL,tempRel);
  
  if (tempRel)
    {
      strcpy(AbsURL,tempRel);
      delete [] tempRel;
    }
  else
    return UPNP_E_INVALID_URL;

  return UPNP_E_SUCCESS;

}



Upnp_Document UpnpMakeAction(char *ActionName,char *ServType, int NumArg, char* Arg,...)
{
	va_list ArgList;
	char * ArgName, *ArgValue;
	char * ActBuff;
	int Idx=0;
	Upnp_Document ActionDoc;
	Upnp_DOMException err;
	Upnp_Node Node;
	Upnp_Element Ele;
	Upnp_Text Txt;


	if(ActionName == NULL || ServType == NULL ) return NULL;

	ActBuff = (char*)malloc(HEADER_LENGTH);
	if(ActBuff == NULL ) return NULL;

	sprintf(ActBuff,"<u:%s xmlns:u=\"%s\"></u:%s>",ActionName,ServType,ActionName);
//	ActionDoc = UpnpParse_Buffer(ActBuff);
	ActionDoc = UpnpParse_BufferNoEx(ActBuff);
	free(ActBuff);

	if( NumArg > 0)
	{
		va_start(ArgList, Arg);
		ArgName = Arg;
		while(Idx++ != NumArg)
		{

			ArgValue = va_arg(ArgList,char*);

			if(ArgName != NULL && ArgValue != NULL)
			{
				Node = UpnpDocument_getFirstChild(ActionDoc);

				Ele  = UpnpDocument_createElement(ActionDoc, ArgName,&err);
				Txt = UpnpDocument_createTextNode(ActionDoc,ArgValue);
				UpnpElement_appendChild(Ele,Txt,&err);
				UpnpNode_appendChild(Node,Ele,&err);
				UpnpElement_free(Ele);
				UpnpNode_free(Node);
				UpnpNode_free(Txt);
				//Temp = UpnpNewPrintDocument(ActionDoc);
				//Upnpfree(Temp);
			}

			ArgName = va_arg(ArgList,char*);
		}
		va_end(ArgList);
	}

	return ActionDoc;

}


int UpnpAddToAction(Upnp_Document * ActionDoc, char *ActionName,char *ServType, char * ArgName, char * ArgValue)
{
	char * ActBuff;
	Upnp_DOMException err;
	Upnp_Node Node;
	Upnp_Element Ele;
	Upnp_Text Txt;

	if(ActionName == NULL || ServType == NULL )
		return UPNP_E_INVALID_PARAM;

	if( *ActionDoc == NULL)
	{
		ActBuff = (char*)malloc(HEADER_LENGTH);
		if(ActBuff == NULL ) return UPNP_E_OUTOF_MEMORY;
			sprintf(ActBuff,"<u:%s xmlns:u=\"%s\"></u:%s>",ActionName,ServType,ActionName);
//		*ActionDoc = UpnpParse_Buffer(ActBuff);
		*ActionDoc = UpnpParse_BufferNoEx(ActBuff);
		free(ActBuff);
	}

	if(ArgName != NULL && ArgValue != NULL)
	{
		Node = UpnpDocument_getFirstChild(*ActionDoc);

		Ele  = UpnpDocument_createElement(*ActionDoc, ArgName,&err);
		Txt = UpnpDocument_createTextNode(*ActionDoc,ArgValue);
		UpnpElement_appendChild(Ele,Txt,&err);
		UpnpNode_appendChild(Node,Ele,&err);
		UpnpElement_free(Ele);
		UpnpNode_free(Node);
		UpnpNode_free(Txt);
	}

	return UPNP_E_SUCCESS;

}

int UpnpAddToPropertySet(Upnp_Document * PropSet, char * ArgName, char * ArgValue)
{
	char BlankDoc[]="<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\"></e:propertyset>";
	Upnp_DOMException err;
	Upnp_Node Node;
	Upnp_Element Ele;
	Upnp_Element Ele1;
	Upnp_Text Txt;

	if (ArgName == NULL || ArgValue == NULL)
		return UPNP_E_INVALID_PARAM;

	if( *PropSet == NULL)
//		*PropSet = UpnpParse_Buffer(BlankDoc);
		*PropSet = UpnpParse_BufferNoEx(BlankDoc);

	Node = UpnpDocument_getFirstChild(*PropSet);

	Ele1 = UpnpDocument_createElement(*PropSet,"e:property",&err);
	Ele  = UpnpDocument_createElement(*PropSet, ArgName,&err);
	Txt = UpnpDocument_createTextNode(*PropSet,ArgValue);
	UpnpElement_appendChild(Ele,Txt,&err);
	UpnpElement_appendChild(Ele1,Ele,&err);
	UpnpNode_appendChild(Node,Ele1,&err);

	UpnpNode_free(Node);
	UpnpElement_free(Ele1);
	UpnpElement_free(Ele);
	UpnpNode_free(Txt);

	return UPNP_E_SUCCESS;
}




Upnp_Document UpnpCreatePropertySet(int NumArg, char* Arg,...)
{
	va_list ArgList;
	int Idx=0;
	char BlankDoc[]="<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\"></e:propertyset>";
	char *ArgName,*ArgValue;
	Upnp_DOMException err;
	Upnp_Node Node;
	Upnp_Element Ele;
	Upnp_Element Ele1;
	Upnp_Text Txt;
//	Upnp_Document PropSet = UpnpParse_Buffer(BlankDoc);
	Upnp_Document PropSet = UpnpParse_BufferNoEx(BlankDoc);

	if(NumArg < 1)
		return NULL;

	va_start(ArgList, Arg);
	ArgName = Arg;

	while(Idx++ != NumArg)
	{
		ArgValue = va_arg(ArgList,char*);

		if(ArgName != NULL && ArgValue != NULL)
		{
			Node = UpnpDocument_getFirstChild(PropSet);
			Ele1 = UpnpDocument_createElement(PropSet,"e:property",&err);
			Ele  = UpnpDocument_createElement(PropSet, ArgName,&err);
			Txt = UpnpDocument_createTextNode(PropSet,ArgValue);
			UpnpElement_appendChild(Ele,Txt,&err);
			UpnpElement_appendChild(Ele1,Ele,&err);
			UpnpNode_appendChild(Node,Ele1,&err);

			UpnpNode_free(Node);
			UpnpElement_free(Ele1);
			UpnpElement_free(Ele);
			UpnpNode_free(Txt);
		}

		ArgName = va_arg(ArgList,char*);
	}

	va_end(ArgList); 
	return PropSet;
}


