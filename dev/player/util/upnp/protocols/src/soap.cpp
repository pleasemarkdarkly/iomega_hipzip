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
// * Neither name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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
// $Revision: 1.95 $
// $Date: 2000/10/05 21:38:33 $
// 
#include <util/upnp/api/config.h>

#if EXCLUDE_SOAP == 0
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <util/upnp/api/config.h>


#include <util/upnp/genlib/miniserver.h>
#include <util/upnp/api/interface.h>
#include <util/upnp/genlib/http_client.h>
#include <util/upnp/api/upnp.h>


#include <sys/utsname.h>

#include <util/upnp/genlib/mystring.h>

//#include "autoprofile.h"

#define HEADER_LENGTH 2000
#define MIN_LEN 25
#define TIMEOUT 10
#define XML_HEADER 300



int CreateControlResponse(char * OutBuf, char * ActionNameRes);


char* itoa( int Value, char* string, int radix )
{

  if( radix == 10 ) {
    sprintf( string, "%d", Value );
  } else if( radix == 16 ) {
    sprintf( string, "%x", Value );
  } else if( radix == 8 ) {
    sprintf( string, "%o", Value );
  }
  return( string );
}



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void AddResponseHeader(char * First,char * OutBuff, char * XmlBuff)
 // Description : This function creates the complete request packet by adding the HTTP header and xml header.
 //
 // Parameters  : First : Type of response packet.
 //               OutBuff : Final response buffer.
 //               XmlBuff : XML buffer.
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddResponseHeader(char * First,char * OutBuff, char * XmlBuff)
{
    char Date[40];
    currentTmToHttpDate(Date);
    sprintf(OutBuff,"%sCONTENT-LENGTH:%d\r\n",First,strlen(XmlBuff)+1);
//	sprintf(OutBuff+strlen(OutBuff),"CONTENT-TYPE:text/xml\r\n%sEXT:\r\nSERVER:%s\r\n\r\n",Date,Server);
	sprintf(OutBuff+strlen(OutBuff),"CONTENT-TYPE:text/xml\r\n%sEXT:\r\n%s\r\n",Date,SERVER_IO);
    strcat(OutBuff,XmlBuff);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetBufferErrorCode(char *Buffer)
 // Description : Checks the HTTP header error code.
 //
 // Parameters  : Buffer : Input buffer.
 //
 // Return value: 1 if HTTP code is 200 else -1.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetBufferErrorCode(char *Buffer)
{
   char * Ptr;
   char Header[LINE_SIZE];

   Ptr = strstr(Buffer,"\r\n");
   strncpy(Header,Buffer,Ptr-Buffer);
   Header[Ptr-Buffer]='\0';

   if((char*)strstr(Header,"200") == NULL) return -1;
   else return 1;

}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : char * strstrcase(char * Src, char * Tok)
 // Description : This is a case independent version of std strstr() fn
 //
 // Parameters  : src : source string
 //               Tok : String to find.
 //
 // Return value: Position of found string or NULL.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * strstrcase(char * Src, char * Tok)
{
   int i=0, SrcLen, TokLen;
   char*Tmp, *RetVal=NULL;

   SrcLen = strlen(Src);
   TokLen = strlen(Tok);

   Tmp =(char*)malloc(strlen(Tok)+1);

   while(i< SrcLen && SrcLen-i >= TokLen)
   {
      strncpy(Tmp,Src+i,TokLen);
      Tmp[TokLen] = '\0';
      if( strcasecmp(Tmp,Tok) == 0)
      {
         RetVal=Src+i;
         break;
      }

      ++i;

   }

  free(Tmp);
  return RetVal;
}



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetNode(Upnp_Document TempDoc, char *NodeName, Upnp_Document * RespNode)
 // Description : This function seperates the given node from the root DOM node.
 // Parameters  : TempDoc :  The root DOM node.
 //               NodeName : Node name to be searched.
 //               RespNode : Response/Output node.
 // Return value: 1 if successfull, or -1 if fails.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetNode(Upnp_Document TempDoc, char *NodeName, Upnp_Document * RespNode)
{
	int RetVal=-1;
	Upnp_DOMString  ActBuf=NULL;
	Upnp_NodeList NodeList=NULL;
	Upnp_Node TmpNode=NULL;

	DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Fn:GetNode   Node Name = %s\n ",NodeName);)

	NodeList = UpnpDocument_getElementsByTagName(TempDoc,(Upnp_DOMString)NodeName);
	if (NodeList != NULL)
	{
		TmpNode = UpnpNodeList_item(NodeList,0);
		if(TmpNode != NULL)
		{
			ActBuf = UpnpNewPrintDocument(TmpNode);

			DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Fn:GetNode Node value is %s\n", ActBuf);)

//			*RespNode = UpnpParse_Buffer(ActBuf);
			*RespNode = UpnpParse_BufferNoEx(ActBuf);
			free(ActBuf);
			RetVal=1;
			UpnpNode_free(TmpNode);
		}
		UpnpNodeList_free(NodeList);
	}
	return RetVal;
}


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetActionNode(Upnp_Document TempDoc, char *NodeName, Upnp_Document * RespNode)
 // Description : This function seperates the action node from the root DOM node.
 // Parameters  : TempDoc  : The root DOM node.
 //               NodeName : Node name to be searched.
 //               RespNode : Response/Output node.
 // Return value: 1 if successfull, or -1 if fails.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetActionNode(Upnp_Document TempDoc, char *NodeName, Upnp_Document * RespNode)
{
	Upnp_Node EnvpNode;
	Upnp_Node BodyNode;
	Upnp_Node ActNode;
	Upnp_DOMString ActNodeName=NULL;

	DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Fn:GetActionNode   node name =%s\n ",NodeName);)

	*RespNode = NULL;

	// Got the Envelop node here
	EnvpNode = UpnpDocument_getFirstChild(TempDoc);
	if(EnvpNode == NULL)
		return -1;

	// Got Body here
	BodyNode = UpnpNode_getFirstChild(EnvpNode);
	if( BodyNode == NULL)
	{
		UpnpNode_free(EnvpNode);
		return -1;
	}

	// Got action node here
	ActNode = UpnpNode_getFirstChild(BodyNode);
	if( ActNode == NULL)
	{
		UpnpNode_free(EnvpNode);
		UpnpNode_free(BodyNode);
		return -1;
	}

	//Test whether this is the action node
	ActNodeName = UpnpNode_getNodeName(ActNode);
	if( ActNodeName == NULL)
	{
		UpnpNode_free(EnvpNode);
		UpnpNode_free(BodyNode);
		UpnpNode_free(ActNode);
		return -1;
	}

	if(strstr(ActNodeName, NodeName) == NULL)
	{
		UpnpNode_free(EnvpNode);
		UpnpNode_free(BodyNode);
		UpnpNode_free(ActNode);
		Upnpfree(ActNodeName);
		return -1;
	}
	else 
	{
        Upnpfree(ActNodeName);
		ActNodeName = UpnpNewPrintDocument(ActNode);
//		*RespNode = UpnpParse_Buffer(ActNodeName);
        if( ActNodeName ) {
            *RespNode = UpnpParse_BufferNoEx(ActNodeName);
            free(ActNodeName);
        }
#ifdef PROFILE
		sProfiler.dumpProfileData();
#endif

		UpnpNode_free(EnvpNode);
		UpnpNode_free(BodyNode);
		UpnpNode_free(ActNode);

		return 1;
	}
}


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetVarName(Upnp_Document TempDoc, char * VarName)
 // Description : This function retrieves the requested variable name.
 // Parameters  : TempDoc :  The root DOM node.
 //               VarName :  Output variable name.
 // Return value: 1 if successfull, or -1 if fails.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetVarName(Upnp_Document TempDoc, char * VarName)
{

   Upnp_Node EnvpNode;
   Upnp_Node BodyNode;
   Upnp_Node StNode;
   Upnp_Node VarNameNode;
   Upnp_Node VarNode;
   Upnp_DOMString StNodeName=NULL;
   Upnp_DOMString Temp;
   Upnp_DOMException Err;


   // Got the Envelop node here
   EnvpNode = UpnpDocument_getFirstChild(TempDoc);
   if(EnvpNode == NULL) return -1;

   // Got Body here
   BodyNode = UpnpNode_getFirstChild(EnvpNode);
   if( BodyNode == NULL)
   {
       UpnpNode_free(EnvpNode);
       return -1;
   }

   // Got action node here
   StNode = UpnpNode_getFirstChild(BodyNode);
   if( StNode == NULL)
   {
       UpnpNode_free(EnvpNode);
       UpnpNode_free(BodyNode);
       return -1;
   }

   //Test whether this is the action node
   StNodeName = UpnpNode_getNodeName(StNode);
   if( StNodeName == NULL)
   {
       UpnpNode_free(EnvpNode);
       UpnpNode_free(BodyNode);
       UpnpNode_free(StNode);
       return -1;
   }


   if(strstr(StNodeName, "QueryStateVariable") == NULL)
   {
       UpnpNode_free(EnvpNode);
       UpnpNode_free(BodyNode);
       UpnpNode_free(StNode);
       Upnpfree(StNodeName);
       return -1;
   }
   else
   {
       Upnpfree(StNodeName);
       VarNameNode = UpnpNode_getFirstChild(StNode);
       if( VarNameNode == NULL)
       {
          UpnpNode_free(EnvpNode);
          UpnpNode_free(BodyNode);
          UpnpNode_free(StNode);
          return -1;
       }


       VarNode=UpnpNode_getFirstChild(VarNameNode);
       Temp = UpnpNode_getNodeValue(VarNode,&Err);
       strcpy( VarName,Temp);

       DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Received query for variable  name %s\n",VarName);)

       Upnpfree(Temp);
       UpnpNode_free(VarNode);
       UpnpNode_free(VarNameNode);
       UpnpNode_free(StNode);
       UpnpNode_free(EnvpNode);
       UpnpNode_free(BodyNode);
       return 1;
   }


}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetActionName(char * Header, char * Name)
 // Description : Retrievs the action name from the HTTP header.
 // Parameters  : Header : Http header string.
 //               Name :  Output action name string.
 //
 // Return value: 1 if successfull -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetActionName(char * Header, char * Name)
{
  char * Ptr,Index=0,*Tmp;


  //Parse with HTTP header
  if( ( Ptr = (char*)strstrcase(Header,"SOAPACTION") ) != NULL )
  {

     if( (Ptr = (char*)strstr(Header,"#")) != NULL)
     {
         ++Ptr;
/* - ecm */
/*         while(isalnum(*Ptr) != 0 && isblank(*Ptr) == 0) */
         while(isalnum(*Ptr) != 0)
         {
            *(Name+Index++) = *(Ptr++);
         }
         *(Name+Index++) = '\0';
     }else return -1;
  }
  else //Parse only action header
  {
     if( (Tmp =  (char*)strstr(Header,"<")) != NULL)
     {
        if( (Ptr = (char*)strstr(Tmp,":")) != NULL)
        {
           Ptr = Ptr+1;
           while(*Ptr != ' ')
           *(Name+Index++) = *(Ptr++);
           *(Name+Index++) = '\0';

        }else return -1;
     }

  }

  return 1;

}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetNodeValue(Upnp_Document XmlDoc, char * NodeName, char * NodeVal)
 // Description : Returns the node string based on the node tag name.
 // Parameters  : XmlDoc  :  The root DOM node.
 //               NodeName: Node name to be searched.
 //               NodeVal : Output node val.
 // Return value: 1 if successfull -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetNodeValue(Upnp_Document XmlDoc, char * NodeName, char * NodeVal)
{

   int RetVal=-1;
   Upnp_DOMString  ActBuf=NULL;
   Upnp_NodeList NodeList;
   Upnp_Node RespNode=NULL;
   Upnp_DOMException Err;
   Upnp_Node temp=NULL;

   NodeList = UpnpDocument_getElementsByTagName(XmlDoc,(Upnp_DOMString)NodeName);

   DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__," Fn:GetNodeValue Tagname = %s\n",NodeName);)

   if (NodeList != NULL)
   {

      RespNode = UpnpNodeList_item(NodeList,0);
      if(RespNode != NULL)
      {
			temp=UpnpNode_getFirstChild(RespNode);
			ActBuf = UpnpNode_getNodeValue(temp,&Err);
			UpnpNode_free(temp);

			if (ActBuf)
			{
				DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Found var name = %s\n",ActBuf);)

				strcpy(NodeVal,ActBuf);
				RetVal =1;
			}
			else
			{
				UpnpNode_free(RespNode);
				UpnpNodeList_free(NodeList);
				return -1;
			}
      }
      else
      {
			UpnpNodeList_free(NodeList);
			return -1;
      }
   }

  Upnpfree(ActBuf); 
  UpnpNode_free(RespNode);
  UpnpNodeList_free(NodeList);
  return RetVal;

}




 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : GetVarValue(Upnp_Document XmlDoc, char ** VarVal)
 // Description : Returns the value of node "return".
 // Parameters  : XmlDoc : The root DOM node.
 //               VarVal : Output variable value.
 // Return value: 1 if successfull -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetVarValue(Upnp_Document XmlDoc, char ** VarVal)
{

   int RetVal=-1;
   Upnp_NodeList NodeList;
   Upnp_Node RespNode=NULL;
   Upnp_DOMException Err;
   Upnp_Node temp=NULL;

   *VarVal = NULL;
   NodeList = UpnpDocument_getElementsByTagName(XmlDoc,"return");

   if (NodeList != NULL)
   {

      RespNode = UpnpNodeList_item(NodeList,0);
      if(RespNode != NULL)
      {
          temp=UpnpNode_getFirstChild(RespNode);
          *VarVal = UpnpNode_getNodeValue(temp,&Err);
          UpnpNode_free(temp);
          DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Fn :GetVarValue : Return value is = %s\n", *VarVal);)

          RetVal =1;
      }
      else
      {
          UpnpNodeList_free(NodeList);
          return -1;
      }
   }

  UpnpNode_free(RespNode);
  UpnpNodeList_free(NodeList);
  return RetVal;

}
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void CreateControlRequest(char * OutBuf, char * ActBuf)
 // Description : This function creates the complete Action request by adding the HTTP header to the input Action node
 //               buffer.
 // Parameters  : OutBuf : The complete action o/p  request packet.
 //               ActBuf : Action buffer(In XML format).
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CreateControlRequest(char * OutBuf, char * ActBuf)
{
  
  char S[]="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding\"><s:Body>";
  char E[]="</s:Body></s:Envelope>";

  strcpy(OutBuf,S);
  strcat(OutBuf,ActBuf);
  strcat(OutBuf,E);

}


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetHostHeader(char *CtrlUrl,char *Host, char *Path)
 // Description : This function parses the control URL to Host and path format.
 //
 // Parameters  : CtrlUrl : The complete control URL path
 //               Host : Output Host portion.
 //               Path : Output Path portion.
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetHostHeader(char *CtrlUrl,char *Host, char *Path)
{
    uri_type parsed_url;
    int return_code=-1;

     if ( (return_code=parse_uri(CtrlUrl,strlen(CtrlUrl),&parsed_url))==HTTP_SUCCESS )
     {
          strncpy(Host,parsed_url.hostport.text.buff,parsed_url.hostport.text.size);
          Host[parsed_url.hostport.text.size]='\0';

          strncpy(Path,parsed_url.pathquery.buff,parsed_url.pathquery.size);
          Path[parsed_url.pathquery.size]='\0';

     }

    return return_code;

}

#ifdef INCLUDE_CLIENT_APIS
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int SoapSendAction( char * ActionURL, char *ServiceType, char *ServiceVer,  Upnp_Document  ActNode , Upnp_Document  * RespNode) //From SOAP module
 // Description : Creates the soap action packet, send it to the loaction specified in the control URL, receives the reply and
 //               pass it back to the caller.
 //
 // Parameters  : ActionURL : Address to send this action packet.
 //               ServiceType : Service Type
 //               ServiceVer : Service Version
 //               ActNode : Input Action node
 //               RespNode: Output response node.
 // Return value: UPNP_E_SUCCESS if action successfull < 0 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SoapSendAction( char * ActionURL, char *ServiceType,  Upnp_Document  ActNode , Upnp_Document  * RespNode) //From SOAP module
{
	char *RqstBuff,*RecvBuff=NULL,ActName[NAME_SIZE]="",*XmlPtr, *Xml, Path[NAME_SIZE], Host[NAME_SIZE];
	int retCode=UPNP_E_INVALID_ACTION,Buf_Len;
	Upnp_Document XmlDoc=NULL;
	Upnp_DOMString  ActBuf=NULL;

	DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Inside function  SoapSendAction \n");)

	ActBuf = UpnpNewPrintDocument(ActNode);
	if( ActBuf == NULL)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"SoapSendAction:Error in getting the action node\n");)
		return UPNP_E_INVALID_ACTION;
	}

	if(GetActionName(ActBuf,ActName) < 0 )
	{
		free(ActBuf);
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"SoapSendAction:Error in getting the action name\n");)
		return UPNP_E_INVALID_ACTION;
	}

	if(strlen(ActName) < 1 )
	{
		free(ActBuf);
		return UPNP_E_INVALID_ACTION;
	}

	DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"SoapSendAction : Action name is  =%s\n",ActName);)

	XmlPtr = (char *) malloc(strlen(ActBuf)+ XML_HEADER);
	if(XmlPtr == NULL)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
		return UPNP_E_OUTOF_MEMORY;
	}
	CreateControlRequest(XmlPtr,ActBuf);
	free(ActBuf);

	Buf_Len =  HEADER_LENGTH+strlen(XmlPtr) +1;
	RqstBuff =(char *) malloc(Buf_Len);
	if(RqstBuff == NULL)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
		return UPNP_E_OUTOF_MEMORY;
	}


	if(GetHostHeader(ActionURL,Host,Path) != HTTP_SUCCESS)
		return UPNP_E_INVALID_URL;

	sprintf(RqstBuff,"POST %s HTTP/1.0\r\nContent-Type: text/xml\r\nSOAPACTION: \"%s#%s\"\r\nContent-Length: %d\r\nHost: %s\r\n\r\n%s",Path,ServiceType,ActName,strlen(XmlPtr)+1,Host,XmlPtr);

	DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"SoapSendAction sending buffer = \n%s\n",RqstBuff);)

	free(XmlPtr);

	int httpResult = transferHTTPRaw(RqstBuff,strlen(RqstBuff)+1,&RecvBuff,ActionURL);
	free(RqstBuff);
	
	if (httpResult != HTTP_SUCCESS)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"SoapSendAction:Error in transferHTTPRaw: %d\n",httpResult);)
		retCode = httpResult;
	}
	else
	{

		DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"SoapSendAction Receved buffer\n %s\n",RecvBuff);)

		if( RecvBuff != NULL && strlen(RecvBuff) > MIN_LEN)
		{
			Xml= strstr(RecvBuff,"\r\n\r\n");
			if(Xml == NULL)
			{
				free(RecvBuff);
				return UPNP_E_INVALID_ACTION;
			}
			else Xml = Xml+4; 

//		XmlDoc = UpnpParse_Buffer(Xml);
//diag_printf("Parse begin\n");
			XmlDoc = UpnpParse_BufferNoEx(Xml);
//diag_printf("Parse end\n");

			if(XmlDoc == NULL)
			{
				free(RecvBuff);
				return UPNP_E_INVALID_ACTION;
			}

			if ((retCode=GetBufferErrorCode(RecvBuff)) > 0)
			{
			if( GetActionNode(XmlDoc, ActName, RespNode) < 0)
			{
				retCode = UPNP_E_INVALID_ACTION;
			}
			else if ( *RespNode != NULL)
			{
				retCode = UPNP_E_SUCCESS ;
			}
			else
			{
				retCode = UPNP_E_INVALID_ACTION;
			}
		}
		else if(GetNode(XmlDoc, "UPnPError", RespNode) > 0)
		{
				GetNodeValue(XmlDoc,"errorCode",ActName);   //change here
				retCode = atoi(ActName);
		}

		UpnpDocument_free(XmlDoc);
		free(RecvBuff);
		}
		else
		{
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"SoapSendAction: Received NULL buffer as response\n");)
			retCode = UPNP_E_INVALID_URL;
			*RespNode = NULL;
		}
	}
	return retCode;
}
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int CreateControlResponse(char * OutBuf, char * ActBuf)
 // Description : It adds the required HTTP header and XML buffer to create the complete soap action packet.
 //
 // Parameters  : OutBuf : The complete SOAP action request packet including HTTP header and XML.
 //               ActBuf : Action buffer.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CreateControlResponse(char * OutBuf, char * ActBuf)
{
  char *XmlBuf;


  char S[]="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>\n";
  char E[]="</s:Body> </s:Envelope>";

  XmlBuf =(char *) malloc(HEADER_LENGTH+strlen(ActBuf));
  if(XmlBuf == NULL)
  {
     DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
     return UPNP_E_OUTOF_MEMORY;
  }

  strcpy(XmlBuf,S);
  strcat(XmlBuf,ActBuf);
  strcat(XmlBuf,E);
  AddResponseHeader("HTTP/1.1 200 OK\r\n",OutBuf,XmlBuf);
  free(XmlBuf);
  return 1;


}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int CreateControlFailure(char *OutBuf,int ErrCode, char * ErrStr)
 // Description : This function creates a error response packet by adding the error code and error string in the input
 //               parameter.
 //
 // Parameters  : OutBuf : The complete SOAP request packet including HTTP header and XML.
 //               ErrCode :  Error code to be included in the reply message.
 //               ErrSt   :  Error string to be included in the reply message.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CreateControlFailure(char *OutBuf,int ErrCode, char * ErrStr)
{
   char *XmlBuf;
   char S[]="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><s:Fault><faultcode>s:Client</faultcode><faultstring>UPnPError</faultstring><detail><UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\"><errorCode>";
   char E[]="</UPnPError></detail></s:Fault></s:Body></s:Envelope>";

   char T[]="Unknown Error !!!!";

   XmlBuf = (char *) malloc(HEADER_LENGTH+strlen(ErrStr));
   if(XmlBuf == NULL)
   {
      DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
      return UPNP_E_OUTOF_MEMORY;
   }
   if (ErrStr != NULL)
   sprintf(XmlBuf,"%s%d</errorCode><errorDescription>%s</errorDescription>%s",S,ErrCode,ErrStr,E);
   else sprintf(XmlBuf,"%s%d</errorCode><errorDescription>%s</errorDescription>%s",S,ErrCode,T,E);

   AddResponseHeader("HTTP/1.1 500 Internal Server Error\r\n",OutBuf,XmlBuf);

   DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"CreateControlFailure: Sending \n %s\n",OutBuf);)
   return 1;
}



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    :  void  CreateControlQueryMsg(char * OutBuf,  char *VarName)
 // Description :  This function creates a status variable query message.
 //
 // Parameters  :  OutBuf : The complete SOAP request packet including HTTP header and XML.
 //
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void  CreateControlQueryMsg(char * OutBuf,  char *VarName)
{
  char S[]="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding\"><s:Body> <u:QueryStateVariable xmlns:u=\"urn:schemas-upnp-org:control-1-0\"> <u:varName>";
  char E[]="</u:varName> </u:QueryStateVariable> </s:Body> </s:Envelope>";


  strcpy(OutBuf,S);
  strcat(OutBuf,VarName);
  strcat(OutBuf,E);

}



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int CreateControlQueryResponse(char * OutBuf, char *Var)
 // Description : This function creates the response or reply packet for the devices.
 //
 // Parameters  :  OutBuf  : The complete SOAP response packet including HTTP header and XML.
 //                Var     : The value of the variable.
 //
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CreateControlQueryResponse(char * OutBuf, char *Var)
{

  char *  XmlBuf;

  char S[]="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:QueryStateVariableResponse xmlns:u=\"urn:schemas-upnp-org:control-1-0\"><return>";
  char E[]="</return> </u:QueryStateVariableResponse> </s:Body> </s:Envelope>";

  XmlBuf =(char *) malloc(HEADER_LENGTH+strlen(Var));
  if(XmlBuf == NULL)
  {
     DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
     return UPNP_E_OUTOF_MEMORY;
  }

  strcpy(XmlBuf,S);
  strcat(XmlBuf,Var);
  strcat(XmlBuf,E);
  AddResponseHeader("HTTP/1.1 200 OK\r\n",OutBuf,XmlBuf);
  free(XmlBuf);
  return 1;
}




 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int SoapGetServiceVarStatus( char * ActionURL, char *VarName, char **VarVal)  //From SOAP module
 // Description : This function creates a status variable query message send it to the specified URL. It also
 //               collect the response.
 //
 // Parameters  :  ActionURL :  Address to send this variable query message.
 //                VarName   : Name of the variable.
 //                VarVal    : Output value.
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef INCLUDE_CLIENT_APIS
int SoapGetServiceVarStatus( char * ActionURL, char *VarName, char **VarVal)  //From SOAP module
{

	char *InBuff, LenBuf[7],*Xml,*RecvBuff,*XmlPtr,*Val;
	int RetVal,Buf_Len;
	Upnp_Document XmlDoc;

	Buf_Len =  HEADER_LENGTH+strlen(VarName);

	RecvBuff = NULL;
	InBuff =(char *) malloc(Buf_Len);
	if(InBuff == NULL)
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
		return UPNP_E_OUTOF_MEMORY;;
	}

	// Creating request packet
	strcpy(InBuff,"CONTENT-LENGTH:     \r\n");
	sprintf(InBuff+strlen(InBuff),"CONTENT-TYPE:text/xml\r\nSOAPACTION:urn:schemas-upnp-org:control-1-0#QueryStateVariable\r\n\r\n");
	XmlPtr = InBuff+strlen(InBuff);
	CreateControlQueryMsg(XmlPtr, VarName);
	itoa(strlen(XmlPtr)+1,LenBuf,10);
	strncpy(InBuff+strlen("CONTENT-LENGTH:"),LenBuf,strlen(LenBuf));

	DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"SoapGetServiceVarStatus: Sending packet \n%s\n",InBuff);)

	 // Sending request packet
	transferHTTP("POST",InBuff,strlen(InBuff)+1,&RecvBuff,ActionURL);  // And receive the response.

	if(RecvBuff != NULL && strlen(RecvBuff) > MIN_LEN)
	{
		DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"SoapGetServiceVarStatus: Received packet \n%s\n",RecvBuff);)

		Xml= strstr(RecvBuff,"\r\n\r\n");

		if(Xml == NULL)
		{
			free(RecvBuff);
			free(InBuff);
			return UPNP_E_INVALID_DESC;
		}
		else
			Xml = Xml+4;

//		XmlDoc=UpnpParse_Buffer(Xml);
		XmlDoc=UpnpParse_BufferNoEx(Xml);
		if(XmlDoc == NULL)
		{
			free(RecvBuff);
			free(InBuff);
			return UPNP_E_INVALID_DESC;
		}

		if ((RetVal=GetBufferErrorCode(RecvBuff)) > 0)
		{
			GetVarValue(XmlDoc,&Val);
			*VarVal = Val;
			RetVal = UPNP_E_SUCCESS;
		}
		else
		{
			GetNodeValue(XmlDoc,"errorCode",InBuff);
			RetVal = atoi(InBuff);
			GetNodeValue(XmlDoc,"errorDescription",InBuff);
			*VarVal = InBuff;
		}

		free(RecvBuff);
	}
	else
	{
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"SoapGetServiceVarStatus: Received NULL packet");)

		*VarVal = NULL;
		free(InBuff);
		RetVal = UPNP_E_INVALID_URL;
	}

	return RetVal;
}
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetCtrlUrl(char * InBuf, char * Url)
 // Description : This function retrieves the control URL from the HTTP message.
 //
 // Parameters  :  InBuf : The complete HTTP buffer.
 //                Url : Control URL.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetCtrlUrl(char * InBuf, char * Url)
{
  char * Sptr,*Eptr,i=0;

  Sptr = (char*)strstrcase(InBuf,"POST");
  if(Sptr != NULL ) Sptr = Sptr + 4;
  else return -1;

  Eptr = (char*)strstrcase(InBuf,"HTTP");
  if(Eptr != NULL) Eptr = Eptr -1;
  else return -1;
  
  while(++Sptr != Eptr)
  *(Url+i++) = *Sptr;

  *(Url+i++) = '\0';

  return 1;
}


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetDeviceInfo(char *CtrlUrl,char *DevUDN, char *ServiceID, Upnp_FunPtr *Fun)
 // Description : This function returns all the information related with service.
 //
 // Parameters  : CtrlUrl : Control URL.
 //               DevUDN : Device UDN.
 //               ServiceID : Service UUID.
 //               Fun : Callback function
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DEVICEONLY(
int GetDeviceInfo(char *CtrlUrl,char *DevUDN, char *ServiceID, Upnp_FunPtr *Fun, void ** Cookie)
{

    struct Handle_Info *HInfo;
    int DeviceHandle ;
    service_info *SInfo;
	
    strcpy(DevUDN, "");
    strcpy(ServiceID, "");
    *Fun=NULL;
    
    HandleLock();
    
    // Write the code to find all the the data related with service


    if ( GetDeviceHandleInfo(&DeviceHandle, &HInfo) != HND_DEVICE )
    {

      DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Client handle , Can't send reply!!!!\n");)

      HandleUnlock();
      return -1;
    }
    else
    {
        if ((SInfo=FindServiceControlURLPath(&HInfo->ServiceTable,CtrlUrl)) == NULL )
        {
           DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in FindServiceControlURLPath\n");)

           HandleUnlock();
           return -1;
        }
        else
        {
            strcpy(ServiceID,SInfo->serviceId);
            strcpy(DevUDN,SInfo->UDN);
            *Fun = HInfo->Callback;
            *Cookie = (void *) HInfo->Cookie;
            HandleUnlock();
            return 1;

        }

    }

    HandleUnlock();
    return -1;
})


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ProcessSoapEventPacket( char * InData, int Socket)
 // Description : This function serves all the request that come from the client, like GetVatStatus or execute action.
 //
 // Parameters  : InData : The input request packet that include HTTP header and XML.
 //               Socket : Socket to send the reply.
 // Return value: 1 if successful.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef INCLUDE_DEVICE_APIS
// All the response function will be called from here.
int ProcessSoapEventPacket( char * InData, int Socket)
{
    char *InStr,*RespStr;
    int BuffLen;
    char ActName[NAME_SIZE],CtrlUrl[LINE_SIZE];
    char None[] ="NULL",*Xml;
    Upnp_Document RespNode;
    Upnp_Document XmlDoc;
    struct Upnp_Action_Request *ActParam; // Event to be sent to client callback fn.
    struct Upnp_State_Var_Request *VarParam;
    void * Cookie=NULL;
    Upnp_FunPtr  SoapEventCallback;

    Xml= strstr(InData,"\r\n\r\n");
    if(Xml == NULL)
    {
       close(Socket);
       return -1;
    }
    else Xml = Xml+4;
    XmlDoc=UpnpParse_BufferNoEx(Xml);
    if(XmlDoc == NULL)
    {
       close(Socket);
       return -1;
    }

    DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"Soap server received packet \n %s\n",InData);)

    BuffLen = strlen(InData)+1;
    if(BuffLen < HEADER_LENGTH)BuffLen = HEADER_LENGTH ;

    InStr  = (char*) malloc(BuffLen);
    if(InStr == NULL)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Error in memory allocation!!!!!!!!!!!\n");)
        close(Socket);
        UpnpDocument_free(XmlDoc);
        return UPNP_E_OUTOF_MEMORY;
    }


    if(GetCtrlUrl(InData,CtrlUrl)  > 0 )
    {

        if((char*)strstrcase(InData,"QueryStateVariable") == NULL) //This soap action
        {

             if(GetActionName(InData,ActName)< 0)
             {
                 DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Couldn't find action name in the header\n");)

                 CreateControlFailure(InStr,UPNP_E_INVALID_ACTION,"Invalid action name!!!!!");
                 write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                 close(Socket);
                 free(InStr);
                 UpnpDocument_free(XmlDoc);
                 return -1;
             }

             DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Found action name  = %s\n", ActName);)

             if( GetActionNode(XmlDoc, ActName, &RespNode) < 0)
             {

                 DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Couldn't find action buffer returning error code\n"));
                 CreateControlFailure(InStr,UPNP_E_INVALID_ACTION,"Invalid control URL!!!!!");
                 write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                 close(Socket);
                 free(InStr);
                 UpnpDocument_free(XmlDoc);
                 return -1;
             }

             //UpnpPrintDocument(RespNode, InStr);
             //printf("Action buffer =%s\n",InStr);
             DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Calling Callback\n"));


             ActParam = (struct Upnp_Action_Request *) malloc (sizeof(struct Upnp_Action_Request));
             if(GetDeviceInfo(CtrlUrl,ActParam->DevUDN, ActParam->ServiceID,&SoapEventCallback,&Cookie)< 0)
             {
                  DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Inside Calling GetDeviceInfo\n"));

                  CreateControlFailure(InStr,UPNP_E_INVALID_ACTION,"Invalid control URL!!!!!");
                  write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                  UpnpDocument_free(XmlDoc);
                  UpnpDocument_free(RespNode);
                  free(InStr);
                  free(ActParam);
                  close(Socket);
                  return -1;
             }

             strcpy(ActParam->ActionName,ActName);
             strcpy(ActParam->ErrStr,"");
             ActParam->ActionRequest=RespNode;
             ActParam->ActionResult=NULL;
             ActParam->ErrCode = UPNP_E_SUCCESS;
             SoapEventCallback(UPNP_CONTROL_ACTION_REQUEST,ActParam,Cookie);
             if(ActParam->ErrCode == UPNP_E_SUCCESS && ActParam->ActionResult != NULL)
             {
                 RespStr = UpnpNewPrintDocument(ActParam->ActionResult);
                 if(RespStr == NULL)
                 {
                     UpnpDocument_free(XmlDoc);
                     UpnpDocument_free(RespNode);
                     free(InStr);
                     free(ActParam);
                     close(Socket);
                     return -1;
                 }

                 if(RespStr != NULL && (int)strlen(RespStr) > abs(BuffLen-HEADER_LENGTH))
                 {
                    free(InStr);
                    InStr  = (char*) malloc(strlen(RespStr)+HEADER_LENGTH);
                 }

                 CreateControlResponse(InStr,RespStr);
                 DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"Sending response \n%s\n",InStr);)

                 write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                 UpnpDocument_free(ActParam->ActionResult);
                 free(RespStr);
             }
             else if (strlen(ActParam->ErrStr) > 1)
             {
                  CreateControlFailure(InStr,ActParam->ErrCode,ActParam->ErrStr);
                  write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
             }
             else
             {
                  CreateControlFailure(InStr,ActParam->ErrCode,"Invalid Request!!!!!");
                  write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);

             }

             UpnpDocument_free(XmlDoc);
             UpnpDocument_free(RespNode);
             free(InStr);
             free(ActParam);
             close(Socket);
        }
        else
        {

            if(GetVarName(XmlDoc,InStr) < 0)
            {
               DBGONLY(UpnpPrintf(UPNP_CRITICAL,SOAP,__FILE__,__LINE__,"Received  error in query for var %s\n",InStr);)

               CreateControlFailure(InStr,UPNP_E_INVALID_URL,"Invalid XML!!!!!");
               write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
               close(Socket);
               free(InStr);
               UpnpDocument_free(XmlDoc);
               return -1;
            }


 
            DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Received query for var = %s\n", InStr);)

            VarParam = (struct Upnp_State_Var_Request *) malloc (sizeof(struct Upnp_State_Var_Request));
            if(GetDeviceInfo(CtrlUrl,VarParam->DevUDN, VarParam->ServiceID,&SoapEventCallback,&Cookie)< 0)
            {

                  CreateControlFailure(InStr,UPNP_E_INVALID_URL,"Invalid control URL!!!!!");
                  write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                  free(InStr);
                  free(VarParam);
                  UpnpDocument_free(XmlDoc);
                  close(Socket);
                  return -1;
            }

            strcpy(VarParam->ErrStr,"");
            VarParam->ErrCode = UPNP_E_SUCCESS;
            VarParam->CurrentVal = NULL;
            strcpy(VarParam->StateVarName, InStr);
            SoapEventCallback(UPNP_CONTROL_GET_VAR_REQUEST,VarParam,Cookie);

            DBGONLY(UpnpPrintf(UPNP_INFO,SOAP,__FILE__,__LINE__,"Return from callback for var request\n"));

            if(VarParam->ErrCode == UPNP_E_SUCCESS)
            {
                if(VarParam->CurrentVal != NULL)
                {
                    if((int)strlen(VarParam->CurrentVal) > abs(BuffLen-HEADER_LENGTH))
                    {
                       free(InStr);
                       InStr  = (char*) malloc(strlen(VarParam->CurrentVal)+HEADER_LENGTH);
                    }

                    CreateControlQueryResponse(InStr,VarParam->CurrentVal);

                    DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"Soap  server sending packet \n %s\n",InStr);)


                    write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                    free(VarParam->CurrentVal);
                 }
                 else
                 {
                     CreateControlQueryResponse(InStr,None);
                     DBGONLY(UpnpPrintf(UPNP_PACKET,SOAP,__FILE__,__LINE__,"Soap  server sending packet \n %s\n",InStr);)
                     write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
                 }
             }
             else
             {   if (strlen(VarParam->ErrStr) >1 ) CreateControlFailure(InStr,VarParam->ErrCode,VarParam->ErrStr);
                 else CreateControlFailure(InStr,VarParam->ErrCode,"Unknown Error !!!!!!!!!!!");
                 write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
             }
             free(InStr);
             free(VarParam);
             UpnpDocument_free(XmlDoc);
             close(Socket);
             return -1;
          }
    }
    else
    {
           CreateControlFailure(InStr,UPNP_E_INVALID_URL,"Invalid control URL !!!!!");
           write_bytes(Socket,InStr,strlen(InStr)+1,TIMEOUT);
           free(InStr);
           UpnpDocument_free(XmlDoc);
           close(Socket);
    }

    return 1;
}
#endif
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int InitSoap()
 // Description : This function initializes the SOAP layer with MiniServer.
 //
 // Parameters  : None
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int InitSoap()
{
   #ifdef INCLUDE_DEVICE_APIS
   SetSoapCallback((MiniServerCallback)ProcessSoapEventPacket);
   #endif
   return UPNP_E_SUCCESS;
}
#endif
