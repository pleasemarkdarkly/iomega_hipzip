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
// $Revision: 1.49 $
// $Date: 2000/10/06 01:14:33 $
//     
#include <util/upnp/api/config.h>
#if EXCLUDE_GENA == 0

#include <util/upnp/genlib/mystring.h>
#include <util/upnp/protocols/gena.h>


#include <cyg/infra/diag.h>

//#define DEBUG_FUN_ENTRY() diag_printf("+%s %d\n", __FUNCTION__, __LINE__)
#define DEBUG_FUN_ENTRY() /**/
//#define DEBUG_FUN_EXIT() diag_printf("-%s %d\n", __FUNCTION__, __LINE__)
#define DEBUG_FUN_EXIT() /**/
#define DEBUG_UNPRINTABLE_STRING(x) \
	{ \
		cyg_mutex_lock(&GlobalDebugMutex); \
		const char* pch = x; \
		while (*pch) \
			diag_printf("%c", *pch++); \
		diag_printf("\n"); \
		cyg_mutex_unlock(&GlobalDebugMutex); \
	}

/*DEVICEONLY(*/
#ifdef INCLUDE_DEVICE_APIS
int genaUnregisterDevice(UpnpDevice_Handle device_handle)
{
  struct Handle_Info * handle_info;
  HandleLock();
  if (GetHandleInfo(device_handle, & handle_info)!=HND_DEVICE)
  {

    DBGONLY(UpnpPrintf(UPNP_CRITICAL,GENA,__FILE__,__LINE__,"genaUnregisterDevice : BAD Handle : %d\n",
		       device_handle));

    HandleUnlock();
    return GENA_E_BAD_HANDLE;
  }
  freeServiceTable(&handle_info->ServiceTable);
  HandleUnlock();
  
  return UPNP_E_SUCCESS;
}


//********************************************************
//*Name: createURL_list
//*Description:   Function to parse
//*               the Callback header Value in subscription requests
//*               takes in a buffer containing URLS delimited by '<' and '>'
//*               The entire buffer is copied into dynamic memory
//*               and stored in the URL_list.
//*               Pointers to the individual urls within this buffer 
//*               are allocated and stored in the URL_list.
//*               Only URLs with network addresses are considered 
//*                (i.e. host:port or domain name)
//* In:           buffer *URLS 
//* Out:          URL_list *out (storage space is passed in) , if successful, 
//*               then structure should be
//*               deallocated at some time by : freeURL_list
//* Return Codes: Returns the number of URLs parsed 
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               
//********************************************************
int createURL_list(token *URLS, URL_list *out)
{
  int URLcount=0;
  int i;
  int return_code=0;

  

  uri_type temp;
  out->URLs=NULL;
  out->parsedURLs=NULL;
   
  for (i=0;i<URLS->size;i++)
    { 
      if  ( (URLS->buff[i]=='<') && (i+1<URLS->size))
	{
	  if ( ((return_code=parse_uri(&URLS->buff[i+1],
				       URLS->size-i+1,&temp))==HTTP_SUCCESS)
	       && (temp.hostport.text.size!=0) )
	    URLcount++;
	  else
	    if (return_code==UPNP_E_OUTOF_MEMORY)
	      return return_code;
	}
    }
   
   out->URLs=(char *) malloc(URLS->size+1);
 
   out->parsedURLs=(uri_type *) malloc(sizeof(uri_type) * URLcount);
   if ( (out->URLs==NULL) || (out->parsedURLs==NULL))
     return UPNP_E_OUTOF_MEMORY;
   memcpy(out->URLs,URLS->buff,URLS->size);
   out->URLs[URLS->size]=0;
   URLcount=0;
   for (i=0;i<URLS->size;i++)
     {
       if  ( (URLS->buff[i]=='<') && (i+1<URLS->size))
	{
	  if ( ((return_code=parse_uri(&out->URLs[i+1],URLS->size-i+1,
			  &out->parsedURLs[URLcount]))==HTTP_SUCCESS)
	       && (out->parsedURLs[URLcount].hostport.text.size!=0) )
	    URLcount++;
	  else
	    if (return_code==UPNP_E_OUTOF_MEMORY)
	      {
		free (out->URLs);
		free (out->parsedURLs);
		out->URLs=NULL;
		out->parsedURLs=NULL;
		return return_code;
	      }
	}
     }
   out->size=URLcount;
   
   return URLcount;
}

//********************************************************
//*Name: respondOK
//*Description: Function to return OK message in the case of a subscription request.
//*In:          sockfd (socket connection of request)
//*             time_out (accepted duration)
//*             subscription * sub (accepted subscription)
//*Out:         returns error codes from respond or UPNP_E_OUTOF_MEMORY
//********************************************************

int respondOK(int sockfd,int time_out,subscription *sub)
{
  char *temp;
  int size=0;
  int return_code;

  size+= strlen(HTTP_OK);
  size+= HTTP_DATE_LENGTH;
  size+= strlen(SERVER_IO);
  size+= strlen("SID: \r\n") + sizeof(Upnp_SID);
  size+= strlen("TIMEOUT: Second-\r\n\r\n") + MAX_SECONDS +1;
  temp = (char *) malloc(size);
  if (temp==NULL)
    {
      respond(sockfd,UNABLE_MEMORY);
      return UPNP_E_OUTOF_MEMORY;
    }
  strcpy(temp,HTTP_OK);
  currentTmToHttpDate(&temp[strlen(temp)]);
  strcat(temp,SERVER_IO);
  strcat(temp,"SID: ");
  strcat(temp,sub->sid);
  strcat(temp,"\r\n");
  if (time_out>=0)
    sprintf(&temp[strlen(temp)],"TIMEOUT: Second-%d\r\n\r\n",time_out);
  else
    strcat(temp,"TIMEOUT: Second-infinite\r\n\r\n");
  return_code=respond(sockfd,temp);
  free(temp);
  return return_code;
}

//********************************************************
//*Name: GeneratePropertySet
//*Description: Function to generate XML propery Set for Notifications
//*             Note: XML_VERSION comment is NOT sent due to interop issues with Microsoft ME
//*In:          char **names (each char* is null terminated), char ** values, int count 
//*Out:         char ** out (dynamically allocated must be freed by caller)
//********************************************************

int GeneratePropertySet(char **names, char ** values, int count,
			char **out)
{
  char *buffer;
  int counter=0;
  int size=0;
 
  int temp_counter =0;

  //  size+=strlen(XML_VERSION); Microsoft Windows Millenium interoperability currently will not work with the XML_VERSION 

  size+=strlen(XML_PROPERTYSET_HEADER);
  size+=strlen("</e:propertyset>\n\n"); 
   
   for (temp_counter=0,counter=0;counter<count;counter++)
    { 
      size+=strlen("<e:property>\n</e:property>\n"); 
      
      size+= (2*strlen(names[counter])+strlen(values[counter])+(strlen("<></>\n")));
      
    }  
   buffer=(char*)malloc(size+1);

   if (buffer==NULL)
     { 
      
       return UPNP_E_OUTOF_MEMORY;
     }
   memset(buffer,0,size+1);
  
   //   strcpy(buffer,XML_VERSION); Microsoft Windows interoperability currently doesn't accept the XML_VERSION tag
   strcpy(buffer,XML_PROPERTYSET_HEADER);
   for (counter=0;counter<count;counter++)
     {
       strcat(buffer,"<e:property>\n");
       sprintf(&buffer[strlen(buffer)],"<%s>%s</%s>\n</e:property>\n",names[counter],values[counter],names[counter]);
     }
   strcat(buffer,"</e:propertyset>\n\n");
  
   (*out)=buffer;
   
   return XML_SUCCESS;
 
}

//********************************************************
//* Name: free_notify_struct
//* Description:  frees memory used in notify_threads
//*               if the reference count is 0, actually frees the struct
//* In:           notify_thread_struct * input
//* Out:          None
//* Return Codes: None
//* Error Codes:  None
//********************************************************

void free_notify_struct(notify_thread_struct * input)
{
  (*input->reference_count)--;
  if ((*input->reference_count)==0)
    {
      free(input->headers);
      free(input->propertySet);
      free(input->servId);
      free(input->UDN);
      free(input->reference_count);
    }
  free(input);
}

//********************************************************
//*Name: genaNotify
//*Description: Function to Notify a particular subscription of a particular event
//*             In general the service should NOT be blocked around this call. (this may cause deadlock with a client)
//*             NOTIFY http request is sent and the reply is processed.
//*In:          char * headers (null terminated) (includes all headers (including \r\n) except SID and SEQ)
//*             char * propertySet (null terminated) XML
//*             subscription *sub (subscription to be Notified, Assumes this is valid for life of function)
//*Out:      
//*Return Codes: GENA_SUCCESS  if the event was delivered   (all codes mapped to codes in upnp.h)
//*Error Codes: UPNP_E_OUTOF_MEMORY
//*             HTTP_E_BAD_URL             
//*             HTTP_E_READ_SOCKET 
//*             HTTP_E_WRITE_SOCKET 
//*             HTTP_E_CONNECT_SOCKET 
//*             HTTP_E_SOCKET    
//*             GENA_E_NOTIFY_UNACCEPTED
//*             HTTP_E_BAD_RESPONSE 
//*             GENA_E_NOTIFY_UNACCEPTED_REMOVE (this subscription must be removed)
//********************************************************

int genaNotify(char * headers, 
	       char * propertySet, subscription *sub)
{
DEBUG_FUN_ENTRY();
  int full_size=0;
  char *full_message;
  http_message parsed_response;
  int i;
  int return_code=GENA_E_NOTIFY_UNACCEPTED;

  char * response;
  
 
  
  full_size=strlen(headers)+strlen("SID: \r\n") + SID_SIZE+
    strlen("SEQ: \r\n\r\n") + MAX_EVENTS + strlen(propertySet)+1;

  full_message = (char *) malloc(full_size);

  if (full_message==NULL)
    return UPNP_E_OUTOF_MEMORY;
  
  

  sprintf(full_message,"%sSID: %s\r\nSEQ: %d\r\n\r\n%s",headers,sub->sid,sub->ToSendEventKey,propertySet);


 
  
  for (i=0;i<sub->DeliveryURLs.size;i++)
    {
//diag_printf("--------- %s: %d: addr: %s\n", __FUNCTION__, i, sub->DeliveryURLs.parsedURLs[i].hostport.text.buff);
    
      if (((return_code=transferHTTPparsedURL("NOTIFY", full_message,
					      strlen(full_message)+1,
					      &response,
					      &sub->DeliveryURLs.parsedURLs[i])
	    )==HTTP_SUCCESS))
	{
//DEBUG_UNPRINTABLE_STRING(full_message);
//diag_printf("--------- response:\n");
//DEBUG_UNPRINTABLE_STRING(response);
//diag_printf("---------\n");

	  break;
	}
      
    }

  free(full_message);
  
  if (return_code==HTTP_SUCCESS)
    {
       
      //only error I really care about is Invalid SID
      return_code=parse_http_response(response,&parsed_response,
				      strlen(response));
  
     
      if (return_code==HTTP_SUCCESS)
	{
	  if (!strncasecmp(parsed_response.status.status_code.buff,
			   "200", strlen("200")))
	    return_code=GENA_SUCCESS;
	  else
	  {
	    if (!strncasecmp(parsed_response.status.status_code.buff,
			     "412",strlen("412")))
	      {
		//Invalid SID gets removed
		return_code=GENA_E_NOTIFY_UNACCEPTED_REMOVE_SUB;
	      }
	    else
	      return_code=GENA_E_NOTIFY_UNACCEPTED;
	  }
	  free_http_message(&parsed_response);
	}
      
      free (response);
//diag_printf("%s: return code: %d\n", __FUNCTION__, return_code);
DEBUG_FUN_EXIT();
      return return_code;
    }
DEBUG_FUN_EXIT();

  return return_code;  
}



void genaNotifyThread(void * input)
{
  subscription *sub;
  service_info *service;
  subscription sub_copy;
  notify_thread_struct *in = (notify_thread_struct *) input;
  int return_code;
  struct Handle_Info * handle_info;

  HandleLock();
  //validate context

  if ( GetHandleInfo(in->device_handle,&handle_info)!=HND_DEVICE)
    {
      free_notify_struct(in);
      HandleUnlock();
      return;
    }
  
  if ( ( (service = FindServiceId( &handle_info->ServiceTable, 
				   in->servId, in->UDN)) ==NULL)
       || (!service->active) 
       || ( (sub=GetSubscriptionSID(in->sid,service))==NULL)
       || ( (copy_subscription(sub,&sub_copy)!=HTTP_SUCCESS)) )
    { 
      free_notify_struct(in);
      HandleUnlock();
      return;
    }
  

/*  
  if (in->eventKey!=sub->ToSendEventKey)
    {
      tpool_Schedule( genaNotifyThread, input); 
      freeSubscription(&sub_copy);
      HandleUnlock();
      return;
    }
*/

  HandleUnlock();
  
  //transmit
 
  return_code = genaNotify(in->headers,
	     in->propertySet, &sub_copy);
  
  freeSubscription(&sub_copy);

  HandleLock();
  
  if ( GetHandleInfo(in->device_handle,&handle_info)!=HND_DEVICE)
    {
      free_notify_struct(in);
      HandleUnlock();
      return;
    }

  //validate context
  if ( ( (service = FindServiceId( &handle_info->ServiceTable, 
				 in->servId, in->UDN)) ==NULL)
       || (!service->active)
       || ( (sub=GetSubscriptionSID(in->sid,service))==NULL) )
    { 
      free_notify_struct(in);
      HandleUnlock();
      return;
    }
  
  sub->ToSendEventKey++;

  if (sub->ToSendEventKey<0) //wrap to 1 for overflow
    sub->ToSendEventKey=1;

  if (return_code==GENA_E_NOTIFY_UNACCEPTED_REMOVE_SUB)
    {
      RemoveSubscriptionSID(in->sid,service);
    }
  

  free_notify_struct(in);
  HandleUnlock();
}


int genaInitNotify(UpnpDevice_Handle device_handle,
		   char *UDN,
		   char *servId,
		   char **VarNames,
		   char **VarValues,
		   int var_count,
		   Upnp_SID sid)
{
  char * UDN_copy=NULL;
  char * servId_copy=NULL;
  char * propertySet=NULL;
  char * headers=NULL;
  subscription * sub=NULL;
  service_info *service=NULL;
  int return_code=GENA_SUCCESS;
  int headers_size;
  int *reference_count=NULL;
  struct Handle_Info * handle_info;

  notify_thread_struct *thread_struct=NULL;

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENA BEGIN INITIAL NOTIFY "));

  reference_count= (int *) malloc(sizeof(int));
   
  if (reference_count==NULL)
    return UPNP_E_OUTOF_MEMORY;
  
  (*reference_count)=0;

  UDN_copy=(char *) malloc(strlen(UDN)+1);
  
  if (UDN_copy==NULL)
    {
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  servId_copy=(char *) malloc(strlen(servId)+1);
  
  if (servId_copy==NULL)
    {
      free(UDN_copy);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  
  strcpy(UDN_copy,UDN);
  strcpy(servId_copy,servId);
  
  HandleLock();

  if ( GetHandleInfo(device_handle,&handle_info)!=HND_DEVICE)
    {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_HANDLE;
    }

  if ( (service = FindServiceId( &handle_info->ServiceTable, 
			     servId, UDN)) ==NULL)
    { 
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_SERVICE;
    }

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"FOUND SERVICE IN INIT NOTFY: UDN %s, ServID: %d ",UDN,servId));
  
  if ( ( (sub=GetSubscriptionSID( sid,service))==NULL) ||
       (sub->active))
    {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_SID;
    }
  
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"FOUND SUBSCRIPTION IN INIT NOTIFY: SID %s ",sid));

  sub->active=1;
  
  if ( (return_code=GeneratePropertySet(VarNames,VarValues,
					var_count,&propertySet))!=XML_SUCCESS)
    {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return return_code;
    }
  
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENERATED PROPERY SET IN INIT NOTIFY: \n'%s'\n",propertySet));

  headers_size=strlen("CONTENT-TYPE text/xml\r\n") +
    strlen("CONTENT-LENGTH: \r\n")+MAX_CONTENT_LENGTH +
    strlen("NT: upnp:event\r\n") +
    strlen("NTS: upnp:propchange\r\n")+1;

    
  headers=(char *) malloc(headers_size);
  
  if (headers==NULL)
    {
      free(propertySet);
      free(UDN_copy);
      free(servId_copy);
      free(reference_count);
      HandleUnlock();
      return UPNP_E_OUTOF_MEMORY;
    }
  
  sprintf(headers,"CONTENT-TYPE: text/xml\r\nCONTENT-LENGTH: %d\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\n",
	  strlen(propertySet)+1);
  
 

  //schedule thread for initial notification

  thread_struct=(notify_thread_struct *) malloc(sizeof(notify_thread_struct));
  if (thread_struct==NULL)
      return_code= UPNP_E_OUTOF_MEMORY;
  else
    {
      (*reference_count)=1;
      thread_struct->servId=servId_copy;
      thread_struct->UDN=UDN_copy;
      thread_struct->headers=headers;
      thread_struct->propertySet=propertySet;
      strcpy(thread_struct->sid,sid);
      thread_struct->eventKey=sub->eventKey++;
      thread_struct->reference_count=reference_count;
      thread_struct->device_handle=device_handle;
      if ( (return_code=tpool_Schedule( genaNotifyThread, thread_struct ))!=0)
	{
	  if (return_code==-1)
	    return_code= UPNP_E_OUTOF_MEMORY;
	}
      else
	return_code=GENA_SUCCESS;
      
    }
  
  if (return_code!=GENA_SUCCESS)
    {
     
      free(reference_count);
      free(UDN_copy);
      free(servId_copy);
      free(thread_struct);
      free(propertySet);
      free(headers);
    }

  HandleUnlock();

  return return_code;
  
  
}




int genaInitNotifyExt(UpnpDevice_Handle device_handle, char *UDN, char *servId,IN Upnp_Document PropSet, Upnp_SID sid)
{
  char * UDN_copy=NULL;
  char * servId_copy=NULL;
  char * headers=NULL;
  subscription * sub=NULL;
  service_info *service=NULL;
  int return_code=GENA_SUCCESS;
  int headers_size;
  int *reference_count=NULL;
  struct Handle_Info * handle_info;
  Upnp_DOMString propertySet=NULL;
  Upnp_DOMString TempPropSet=NULL;

  notify_thread_struct *thread_struct=NULL;

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENA BEGIN INITIAL NOTIFY EXT"));
  reference_count= (int *) malloc(sizeof(int));
   
  if (reference_count==NULL)
    return UPNP_E_OUTOF_MEMORY;
  
  (*reference_count)=0;

  UDN_copy=(char *) malloc(strlen(UDN)+1);
  
  if (UDN_copy==NULL)
    {
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  servId_copy=(char *) malloc(strlen(servId)+1);
  
  if (servId_copy==NULL)
    {
      free(UDN_copy);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  
  strcpy(UDN_copy,UDN);
  strcpy(servId_copy,servId);
  
  HandleLock();

  if ( GetHandleInfo(device_handle,&handle_info)!=HND_DEVICE)
    {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_HANDLE;
    }

  if ( (service = FindServiceId( &handle_info->ServiceTable, 
			     servId, UDN)) ==NULL)
    { 
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_SERVICE;
    }
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"FOUND SERVICE IN INIT NOTFY EXT: UDN %s, ServID: %d\n",UDN,servId));
  
  
  if ( ( (sub=GetSubscriptionSID( sid,service))==NULL) ||
       (sub->active))
    {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return GENA_E_BAD_SID;
    }
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"FOUND SUBSCRIPTION IN INIT NOTIFY EXT: SID %s",sid));


  sub->active=1;
  

  TempPropSet = UpnpNewPrintDocument(PropSet);
  if(TempPropSet == NULL)
  {
      free(UDN_copy);
      free(reference_count);
      free(servId_copy);
      HandleUnlock();
      return UPNP_E_INVALID_PARAM;
  }
  else
  {
      propertySet=strstr(TempPropSet,"<e:propertyset");
      if(propertySet == NULL)
      {
         free(UDN_copy);
         free(reference_count);
         free(servId_copy);
         HandleUnlock();
         Upnpfree(TempPropSet);
         return UPNP_E_INVALID_PARAM;
      }
  }

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENERATED PROPERY SET IN INIT EXT NOTIFY: %s",propertySet));


  headers_size=strlen("CONTENT-TYPE text/xml\r\n") +
    strlen("CONTENT-LENGTH: \r\n")+MAX_CONTENT_LENGTH +
    strlen("NT: upnp:event\r\n") +
    strlen("NTS: upnp:propchange\r\n")+1;

    
  headers=(char *) malloc(headers_size);
  
  if (headers==NULL)
    {
      free(propertySet);
      free(UDN_copy);
      free(servId_copy);
      free(reference_count);
      HandleUnlock();
      return UPNP_E_OUTOF_MEMORY;
    }
  
  sprintf(headers,"CONTENT-TYPE: text/xml\r\nCONTENT-LENGTH: %d\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\n",
	  strlen(propertySet)+1);
  
 

  //schedule thread for initial notification

  thread_struct=(notify_thread_struct *) malloc(sizeof(notify_thread_struct));
  if (thread_struct==NULL)
      return_code= UPNP_E_OUTOF_MEMORY;
  else
    {
      (*reference_count)=1;
      thread_struct->servId=servId_copy;
      thread_struct->UDN=UDN_copy;
      thread_struct->headers=headers;
      thread_struct->propertySet=propertySet;
      strcpy(thread_struct->sid,sid);
      thread_struct->eventKey=sub->eventKey++;
      thread_struct->reference_count=reference_count;
      thread_struct->device_handle=device_handle;
      if ( (return_code=tpool_Schedule( genaNotifyThread, thread_struct ))!=0)
	{
	  if (return_code==-1)
	    return_code= UPNP_E_OUTOF_MEMORY;
	}
      else
	return_code=GENA_SUCCESS;
      
    }
  
  if (return_code!=GENA_SUCCESS)
    {
     
      free(reference_count);
      free(UDN_copy);
      free(servId_copy);
      free(thread_struct);
      free(propertySet);
      free(headers);
    }

  HandleUnlock();

  return return_code;
  
  
}
int genaNotifyAllExt(UpnpDevice_Handle device_handle, char *UDN, char *servId,IN Upnp_Document PropSet)
{
  char * headers=NULL;
  int headers_size;
  int return_code=GENA_SUCCESS; 
  char * UDN_copy=NULL;
  char * servId_copy=NULL;
  int *reference_count =NULL;
  struct Handle_Info *handle_info;
  Upnp_DOMString propertySet=NULL;
  Upnp_DOMString TempPropSet=NULL;


  subscription * finger=NULL;

  notify_thread_struct * thread_struct=NULL;

  service_info *service=NULL;

  reference_count= (int *) malloc(sizeof(int));

  if (reference_count==NULL)
    return UPNP_E_OUTOF_MEMORY;

  (*reference_count=0);

  UDN_copy=(char *) malloc(strlen(UDN)+1);
  

  if (UDN_copy==NULL)
  {
    free(reference_count);
    return UPNP_E_OUTOF_MEMORY;
  }

  servId_copy=(char *) malloc(strlen(servId)+1);
  
  if (servId_copy==NULL)
    {
      free(UDN_copy);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
    
  strcpy(UDN_copy,UDN);
  strcpy(servId_copy,servId);
  
  TempPropSet = UpnpNewPrintDocument(PropSet);
  if(TempPropSet == NULL)
  {
     free(UDN_copy);
     free(servId_copy);
     free(reference_count);
     return UPNP_E_INVALID_PARAM;
  }
  else
  {
      propertySet=strstr(TempPropSet,"<e:propertyset");
      if(propertySet == NULL)
      {
         free(UDN_copy);
         free(servId_copy);
         free(reference_count);
         Upnpfree(TempPropSet);
         return UPNP_E_INVALID_PARAM;
      }
  }
    

  headers_size=strlen("CONTENT-TYPE text/xml\r\n") +
    strlen("CONTENT-LENGTH: \r\n")+MAX_CONTENT_LENGTH +
    strlen("NT: upnp:event\r\n") +
    strlen("NTS: upnp:propchange\r\n")+1;
  
  headers=(char *) malloc(headers_size);
  if (headers==NULL)
    {
      free(UDN_copy);
      free(servId_copy);
      free(TempPropSet);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  //changed to add null terminator at end of content
  //content length = (length in bytes of property set) + null char
  sprintf(headers,"CONTENT-TYPE: text/xml\r\nCONTENT-LENGTH: %d\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\n",strlen(propertySet)+1);
  
  HandleLock();

  if ( GetHandleInfo(device_handle,&handle_info)!=HND_DEVICE)
    return_code=GENA_E_BAD_HANDLE;
  else
    {
      if ( (service = FindServiceId( &handle_info->ServiceTable, 
				     servId, UDN)) !=NULL)
	{ 
	  finger=GetFirstSubscription(service);
	  
	  while (finger)
	    {
	      thread_struct=(notify_thread_struct *) malloc(sizeof(notify_thread_struct));
	      if (thread_struct==NULL)
		{
		  break;
		  return_code= UPNP_E_OUTOF_MEMORY;
		}
	      (*reference_count)++;
	      thread_struct->reference_count=reference_count;
	      thread_struct->UDN=UDN_copy;
	      thread_struct->servId=servId_copy;
	      thread_struct->headers=headers;
	      thread_struct->propertySet=propertySet;
	      strcpy(thread_struct->sid,finger->sid);
	      thread_struct->eventKey=finger->eventKey++;
	      thread_struct->device_handle=device_handle;
	      //if overflow, wrap to 1
	      if (finger->eventKey<0)
		finger->eventKey=1;
	      
	      if ( (return_code=tpool_Schedule( genaNotifyThread, thread_struct ))!=0)
		{
		  if (return_code==-1)
		    return_code= UPNP_E_OUTOF_MEMORY;
		  break;
		}
	      
	      finger=GetNextSubscription(service,finger);
	      
	    }
	}
      else
	return_code=GENA_E_BAD_SERVICE;  
    }
  
  if ((*reference_count)==0)
    {
      free(reference_count);
      free(headers);
      free(TempPropSet);
      free(UDN_copy);
      free(servId_copy);
    }
  HandleUnlock();
  
  return return_code;
}



int genaNotifyAll(UpnpDevice_Handle device_handle,
	       char *UDN,
	       char *servId,
	       char **VarNames,
	       char **VarValues,
		  int var_count
	       )
{
  char * headers=NULL;
  char * propertySet=NULL;
  int headers_size;
  int return_code=GENA_SUCCESS;
  char * UDN_copy=NULL;
  char * servId_copy=NULL;
  int *reference_count =NULL;
  struct Handle_Info *handle_info;

  subscription * finger=NULL;

  notify_thread_struct * thread_struct=NULL;

  service_info *service=NULL;

  reference_count= (int *) malloc(sizeof(int));

  if (reference_count==NULL)
    return UPNP_E_OUTOF_MEMORY;

  (*reference_count=0);

  UDN_copy=(char *) malloc(strlen(UDN)+1);
  

  if (UDN_copy==NULL)
  {
    free(reference_count);
    return UPNP_E_OUTOF_MEMORY;
  }

  servId_copy=(char *) malloc(strlen(servId)+1);
  
  if (servId_copy==NULL)
    {
      free(UDN_copy);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
    
  strcpy(UDN_copy,UDN);
  strcpy(servId_copy,servId);
  
  
  if ( (return_code=GeneratePropertySet(VarNames,VarValues,
					var_count,&propertySet))!=XML_SUCCESS)
  {
    free(UDN_copy);
    free(servId_copy);
    free(reference_count);
    return return_code;
  }
  
  headers_size=strlen("CONTENT-TYPE text/xml\r\n") +
    strlen("CONTENT-LENGTH: \r\n")+MAX_CONTENT_LENGTH +
    strlen("NT: upnp:event\r\n") +
    strlen("NTS: upnp:propchange\r\n")+1;
  
  headers=(char *) malloc(headers_size);
  if (headers==NULL)
    {
      free(UDN_copy);
      free(servId_copy);
      free(propertySet);
      free(reference_count);
      return UPNP_E_OUTOF_MEMORY;
    }
  //changed to add null terminator at end of content
  //content length = (length in bytes of property set) + null char
  sprintf(headers,"CONTENT-TYPE: text/xml\r\nCONTENT-LENGTH: %d\r\nNT: upnp:event\r\nNTS: upnp:propchange\r\n",strlen(propertySet)+1);
  
  HandleLock();

  if ( GetHandleInfo(device_handle,&handle_info)!=HND_DEVICE)
    return_code=GENA_E_BAD_HANDLE;
  else
    {
      if ( (service = FindServiceId( &handle_info->ServiceTable, 
				     servId, UDN)) !=NULL)
	{ 
	  finger=GetFirstSubscription(service);
	  
	  while (finger)
	    {
	      thread_struct=(notify_thread_struct *) malloc(sizeof(notify_thread_struct));
	      if (thread_struct==NULL)
		{
		  break;
		  return_code= UPNP_E_OUTOF_MEMORY;
		}
	      (*reference_count)++;
	      thread_struct->reference_count=reference_count;
	      thread_struct->UDN=UDN_copy;
	      thread_struct->servId=servId_copy;
	      thread_struct->headers=headers;
	      thread_struct->propertySet=propertySet;
	      strcpy(thread_struct->sid,finger->sid);
	      thread_struct->eventKey=finger->eventKey++;
	      thread_struct->device_handle=device_handle;
	      //if overflow, wrap to 1
	      if (finger->eventKey<0)
		finger->eventKey=1;
	      
	      if ( (return_code=tpool_Schedule( genaNotifyThread, thread_struct ))!=0)
		{
		  if (return_code==-1)
		    return_code= UPNP_E_OUTOF_MEMORY;
		  break;
		}
	      
	      finger=GetNextSubscription(service,finger);
	      
	    }
	}
      else
	return_code=GENA_E_BAD_SERVICE;  
    }
  
  if ((*reference_count)==0)
    {
      free(reference_count);
      free(headers);
      free(propertySet);
      free(UDN_copy);
      free(servId_copy);
    }
  HandleUnlock();
  
  return return_code;
}

void genaUnsubscribeRequest(http_message request, int sockfd)
{
  char * eventURLpath;
  token temp_buff;
  Upnp_SID sid;
  service_info * service;
  struct Handle_Info * handle_info;
  UpnpDevice_Handle device_handle;

  //if the callback or NT is present then there is an error
  if ( (search_for_header(&request,"CALLBACK",&temp_buff))
       || (search_for_header(&request,"NT",&temp_buff) ) )
    {
      respond(sockfd,BAD_REQUEST);
      return;
    }
  
  //get SID
  if ( ( !search_for_header(&request,"SID",&temp_buff))
       || (temp_buff.size>SID_SIZE) )
    {
      respond(sockfd, MISSING_SID);
      return;
    }
  memcpy(sid,temp_buff.buff,temp_buff.size);
  sid[SID_SIZE]=0;

 
  //look up service by eventURL 
  eventURLpath=(char*) malloc(request.request.request_uri.pathquery.size+1);
  
  if (eventURLpath==NULL)
    {
      respond(sockfd,UNABLE_MEMORY);
      return;
    }
  
  memcpy(eventURLpath,request.request.request_uri.pathquery.buff,request.request.request_uri.pathquery.size);
  eventURLpath[request.request.request_uri.pathquery.size]=0;

  HandleLock();

  //CURRENTLY ONLY SUPPORT ONE DEVICE

  if ( (GetDeviceHandleInfo(&device_handle,&handle_info)!=HND_DEVICE))
    {
      respond(sockfd,INVALID_SID);
      HandleUnlock();
      return;
    }
  
  service=FindServiceEventURLPath(&handle_info->ServiceTable,
				  eventURLpath);
  free(eventURLpath);

  if ( (service==NULL) || (!service->active)
       || (GetSubscriptionSID(sid,service)==NULL))
    {
      respond(sockfd,INVALID_SID);
      HandleUnlock();
      return;
    }
  
  RemoveSubscriptionSID(sid,service);

  respond(sockfd,HTTP_OK_CRLF);
  HandleUnlock();
}






void genaRenewRequest(http_message request, int sockfd)
{
  token temp_buff;
  Upnp_SID sid;
  char * eventURLpath;
  token timeout;
  subscription * sub;
  int time_out=1801;
  service_info *service;
  time_t current_time;
  struct Handle_Info * handle_info;
  UpnpDevice_Handle device_handle;

  //if the callback or NT is present then there is an error
  if ( (search_for_header(&request,"CALLBACK",&temp_buff))
       || (search_for_header(&request,"NT",&temp_buff) ) )
    {
      respond(sockfd,BAD_REQUEST);
      return;
    }
  
  //get SID
  if ( ( !search_for_header(&request,"SID",&temp_buff))
       || (temp_buff.size>SID_SIZE) )
    {
      respond(sockfd, MISSING_SID);
      return;
    }
  memcpy(sid,temp_buff.buff,temp_buff.size);
  sid[SID_SIZE]=0;


  //look up service by eventURL 
  eventURLpath=(char*) malloc(request.request.request_uri.pathquery.size+1);
  
  if (eventURLpath==NULL)
    {
      respond(sockfd,UNABLE_MEMORY);
      return;
    }

 

  memcpy(eventURLpath,request.request.request_uri.pathquery.buff,request.request.request_uri.pathquery.size);
  eventURLpath[request.request.request_uri.pathquery.size]=0;
  
  HandleLock();

  //CURRENTLY ONLY SUPPORT ONE DEVICE

  if ( (GetDeviceHandleInfo(&device_handle, &handle_info))!=HND_DEVICE)
    {
      respond(sockfd,INVALID_SID);
      HandleUnlock();
      return;
    }
    
  service=FindServiceEventURLPath(&handle_info->ServiceTable,
				  eventURLpath);
  free(eventURLpath);
  
  //get Subscription
  if ( (service==NULL) || (!service->active)
       || ( (sub=GetSubscriptionSID(sid,service))==NULL))
    {
      respond(sockfd,INVALID_SID);
      HandleUnlock();
      return;
    }

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Renew request: Number of subscriptions already: %d\n Max Subscriptions allowed:%d\n",service->TotalSubscriptions,
		     handle_info->MaxSubscriptions));
 
  if (handle_info->MaxSubscriptions!=-1)
    if (service->TotalSubscriptions>handle_info->MaxSubscriptions)
      {
	respond(sockfd,UNABLE_MEMORY);
	RemoveSubscriptionSID(sub->sid,service);
	HandleUnlock();
	return;
      }
  
  //Set the timeout
  if (search_for_header(&request,"TIMEOUT",&timeout))
    if (sscanf(timeout.buff,"Second-%d",&time_out)!=1)
      if (!strncmp(timeout.buff,"Second-infinite",strlen("Second-infinite")))
	time_out=-1;
      else
	time_out=DEFAULT_TIMEOUT; //default is 1801 seconds
  
  if (handle_info->MaxSubscriptionTimeOut!=-1)
    if ( (time_out==-1) || (time_out>handle_info->MaxSubscriptionTimeOut))
      time_out=handle_info->MaxSubscriptionTimeOut;
  
  
  time(&current_time);
  
  if (time_out>0)
    sub->expireTime=current_time+time_out;
  else 
    sub->expireTime=0;
  
   //respond
  if ( (respondOK(sockfd,time_out,sub)!=UPNP_E_SUCCESS))
    {
      RemoveSubscriptionSID(sub->sid,service);
    }
  HandleUnlock();
  
}

void genaSubscriptionRequest(http_message request, int sockfd)
{
  char * eventURLpath;
  char temp_sid[SID_SIZE];

  token timeout;
  token callback;
 
  
  int return_code=1;

  int time_out=1801;

  service_info * service;
  struct Upnp_Subscription_Request request_struct;
 

  subscription *sub;
#ifdef USE_UUID
  uuid_t uuid;
#endif
  time_t current_time;
  
  struct Handle_Info *handle_info;
  void * cookie;
  Upnp_FunPtr callback_fun;
  UpnpDevice_Handle device_handle;
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscription Request Received:\n"));
  DBGONLY(print_http_request(&request,UPNP_PACKET,GENA,__FILE__,__LINE__));

  //Check NT header
  
  //Windows Millenium Interoperability:
  //we accept either upnp:event, or upnp:propchange for the NT header
  if  ( (!search_for_header(&request,"NT",&callback))
	||  ( (strncasecmp(callback.buff,"upnp:event",callback.size))
	      && (strncasecmp(callback.buff,"upnp:propchange",callback.size))) )
    {
      respond(sockfd,INVALID_NT);
      return;
    }
  
  //if a SID is present then the we have a bad request
  // "incompatible headers"
  if ( search_for_header(&request,"SID",&callback))
    {
      respond(sockfd, BAD_REQUEST);
      return;
    }

  //look up service by eventURL 
  eventURLpath=(char*) malloc(request.request.request_uri.pathquery.size+1);
  
  if (eventURLpath==NULL)
    {
      respond(sockfd,UNABLE_MEMORY);
      return;
    }
  
  
  memcpy(eventURLpath,request.request.request_uri.pathquery.buff,request.request.request_uri.pathquery.size);
  eventURLpath[request.request.request_uri.pathquery.size]=0;

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"SubscriptionRequest for event URL path: %s\n",eventURLpath);)

  HandleLock();

  //CURRENTLY ONLY ONE DEVICE

  if ( (GetDeviceHandleInfo(&device_handle, &handle_info)!=HND_DEVICE))
    {
      respond(sockfd,UNABLE_SERVICE_UNKNOWN);
      HandleUnlock();
      return;
    }
  

  service=FindServiceEventURLPath(&handle_info->ServiceTable,
				  eventURLpath);
  
  free(eventURLpath);
 
  if ( (service==NULL) || (!service->active) )
    { 
      respond(sockfd,UNABLE_SERVICE_UNKNOWN);
      HandleUnlock();
      return;
    }
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscription Request: Number of Subscriptions already %d\n Max Subscriptions allowed: %d\n",service->TotalSubscriptions,handle_info->MaxSubscriptions));

  if (handle_info->MaxSubscriptions!=-1)
  if ( (service->TotalSubscriptions>=handle_info->MaxSubscriptions))
    {
      respond(sockfd,UNABLE_MEMORY); //return 500 series error code
      HandleUnlock();
      return;
    }
  
  
  //generate new subscription
  sub=(subscription *) malloc(sizeof(subscription));
  

  if (sub==NULL)
    {
      respond(sockfd, UNABLE_MEMORY);
      HandleUnlock();
      return;
    }
  
  sub->eventKey=0;
  sub->ToSendEventKey=0;
  sub->active=0;
  sub->next=NULL;
  
  //check for valid callbacks
  if ( (!search_for_header(&request,"CALLBACK",&callback))
       || ( (return_code=createURL_list(&callback,&sub->DeliveryURLs))==0 ))
    { 
      respond(sockfd, BAD_CALLBACK);
      freeSubscriptionList(sub);
      HandleUnlock();
      return;
    }
  else
    if (return_code==UPNP_E_OUTOF_MEMORY)
      { 
	respond(sockfd,UNABLE_MEMORY);
	freeSubscriptionList(sub);
	HandleUnlock();
	return;
     }
  
  //Set the timeout
  if (search_for_header(&request,"TIMEOUT",&timeout))
    if (sscanf(timeout.buff,"Second-%d",&time_out)!=1)
      if (!strncmp(timeout.buff,"Second-infinite",strlen("Second-infinite")))
	time_out=-1;
      else
	time_out=DEFAULT_TIMEOUT; //default is >1800 seconds
  
  if (handle_info->MaxSubscriptionTimeOut!=-1)
    if ( (time_out==-1) || (time_out>handle_info->MaxSubscriptionTimeOut))
      time_out=handle_info->MaxSubscriptionTimeOut;
  
  time(&current_time);
  
  if (time_out>0)
    sub->expireTime=current_time+time_out;
  else 
    sub->expireTime=0;
  
#ifdef USE_UUID
  //generate SID
  uuid_generate(uuid);
  uuid_unparse(uuid,temp_sid);
  sprintf(sub->sid,"uuid:%s",temp_sid);
#else
//	sprintf(sub->sid,"uuid:7d444840-9dc0-11d1-b245-5ffdce74fad2");
static int s_iRequestNumber = 0;
	sprintf(sub->sid,"uuid:7d444840-9dc0-11d1-b245-5ffdcf%06d", s_iRequestNumber++);

#endif
  
  //respond
  if (respondOK(sockfd,time_out,sub)!=UPNP_E_SUCCESS)
    {
      freeSubscriptionList(sub);
      HandleUnlock();
      return;
    }

  //add to subscription list
  sub->next=service->subscriptionList;
  service->subscriptionList=sub;
  service->TotalSubscriptions++;
  
  //finally generate callback for init table dump
  request_struct.ServiceId=service->serviceId;
  request_struct.UDN=service->UDN;
  strcpy((char *) request_struct.Sid,sub->sid);
  
  //copy callback
  callback_fun=handle_info->Callback;
  cookie=handle_info->Cookie;
  
  HandleUnlock();

  //make call back with request struct
  //in the future should find a way of mainting
  //that the handle is not unregistered in the middle of a 
  //callback
  
  callback_fun(UPNP_EVENT_SUBSCRIPTION_REQUEST,
	       &request_struct,cookie);
 
}

void genaSubscribeOrRenew(http_message request, int sockfd)
{
  token NT;

  if ( search_for_header(&request,"NT",&NT))
    {
      
      genaSubscriptionRequest(request, sockfd);
    }
  else
    { 
      genaRenewRequest(request, sockfd);
    }
}
/*)*/
#endif	/* INCLUDE_DEVICE_APIS */

#endif



