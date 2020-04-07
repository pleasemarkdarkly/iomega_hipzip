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
// $Revision: 1.33 $
// $Date: 2000/10/05 17:25:42 $
//     
#include <util/upnp/api/config.h>

#include <util/upnp/genlib/mystring.h>

#if EXCLUDE_GENA == 0

#include <util/upnp/protocols/gena.h>
#ifdef _WIN32
HANDLE  GlobalClientSubscribeMutex;
#else
#ifdef USE_PTHREADS
pthread_mutex_t GlobalClientSubscribeMutex= PTHREAD_MUTEX_INITIALIZER;
#else
cyg_mutex_t GlobalClientSubscribeMutex;
#endif
#endif

#ifdef INCLUDE_CLIENT_APIS

void GenaAutoRenewSubscription(void *input)
{
  upnp_timeout *event =(upnp_timeout *) input;
  void * cookie;
  Upnp_FunPtr callback_fun;
  struct Handle_Info * handle_info;
  struct Upnp_Event_Subscribe * sub_struct= (struct Upnp_Event_Subscribe *)
    event->Event;
  
  int send_callback =0;
  Upnp_EventType_e eventType = UPNP_CONTROL_ACTION_REQUEST;

  

  if (AUTO_RENEW_TIME==0)
    {
      DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENA SUB EXPIRED"));
      sub_struct->ErrCode=UPNP_E_SUCCESS;
      send_callback=1;
      eventType=UPNP_EVENT_SUBSCRIPTION_EXPIRED;
    }
  else
    {
      DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENA AUTO RENEW"));
      if ( ( (sub_struct->ErrCode=genaRenewSubscription(event->handle,
							sub_struct->Sid,
							&sub_struct->TimeOut))!=UPNP_E_SUCCESS)
	   && (sub_struct->ErrCode!=GENA_E_BAD_SID)
	   && (sub_struct->ErrCode!=GENA_E_BAD_HANDLE))
	{
	  send_callback=1;
	  eventType=UPNP_EVENT_AUTORENEWAL_FAILED;
	}
    }
  if (send_callback)
    {
      HandleLock();
      if ( GetHandleInfo(event->handle,&handle_info)!=HND_CLIENT)
	{
	  HandleUnlock();
	  free_upnp_timeout(event);
	  return;
	}  
      DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"HANDLE IS VALID"));
      
      callback_fun=handle_info->Callback;
      cookie=handle_info->Cookie;
      HandleUnlock();
      //make callback
      callback_fun(eventType,
		   event->Event,cookie);
    }
  
  free_upnp_timeout(event);
  
}


int ScheduleGenaAutoRenew(int client_handle,
			  int TimeOut,
			  client_subscription * sub)
{
  struct Upnp_Event_Subscribe *RenewEventStruct=NULL;
  upnp_timeout * RenewEvent=NULL;
  int return_code=GENA_SUCCESS;

  if (TimeOut==UPNP_INFINITE)
    return GENA_SUCCESS;
    
  RenewEventStruct= (struct Upnp_Event_Subscribe *) 
    malloc (sizeof (struct Upnp_Event_Subscribe));
  
  if (RenewEventStruct==NULL)
    return UPNP_E_OUTOF_MEMORY;
  
  RenewEvent= (upnp_timeout *) malloc(sizeof(upnp_timeout));

  if ( RenewEvent==NULL)
    {
      free(RenewEventStruct);
      return UPNP_E_OUTOF_MEMORY;
    }

  //schedule expire event
  strncpy(RenewEventStruct->Sid,sub->sid, 42);

  RenewEventStruct->ErrCode=UPNP_E_SUCCESS;

  strncpy(RenewEventStruct->PublisherUrl,sub->EventURL,NAME_SIZE-1);

  RenewEventStruct->TimeOut=TimeOut;

  //RenewEvent->EventType=UPNP_EVENT_SUBSCRIPTION_EXPIRE;

  RenewEvent->handle=client_handle;
  RenewEvent->Event=RenewEventStruct; 
  
  if (  (return_code= ScheduleTimerEvent(TimeOut-AUTO_RENEW_TIME,
					 GenaAutoRenewSubscription,
					 RenewEvent,
					 &GLOBAL_TIMER_THREAD,
					 &(RenewEvent->eventId))) !=UPNP_E_SUCCESS)
    {
      free(RenewEvent);
      free(RenewEventStruct);
      return return_code;
    }
  
  sub->RenewEventId=RenewEvent->eventId;

  return GENA_SUCCESS;
  
}


int genaUnregisterClient(UpnpClient_Handle client_handle)
{
  int request_size=0;
  char * request=NULL;
  char * response =NULL;
  client_subscription sub_copy;
  int return_code=UPNP_E_SUCCESS;
  struct Handle_Info * handle_info=NULL;
  int done=0;

  while (!done)
    {
      HandleLock();
      if (GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT)
	{
	  HandleUnlock();
	  return GENA_E_BAD_HANDLE;
	}
      if (handle_info->ClientSubList==NULL)
	{
	  done=1;
	  return_code=UPNP_E_SUCCESS;
	  break;
	}
      if ( (return_code=copy_client_subscription(handle_info->ClientSubList, &sub_copy)!=HTTP_SUCCESS))
	{
	  done=1;
	  break;
	}
      RemoveClientSubClientSID(&handle_info->ClientSubList,sub_copy.sid);
      HandleUnlock();
      
      request_size=strlen("SID: \r\n\r\n")+ strlen(sub_copy.ActualSID)+1;
      request=(char *) malloc(request_size);
      
      if (request==NULL)
	{
	  return UPNP_E_OUTOF_MEMORY;
	}
      sprintf(request,"SID: %s\r\n\r\n",sub_copy.ActualSID);
      
      return_code=transferHTTP("UNSUBSCRIBE",request,strlen(request),
			       &response,sub_copy.EventURL);
      free(request);  
      if (return_code==HTTP_SUCCESS)
	free(response);
    }
  
  freeClientSubList(handle_info->ClientSubList);
  HandleUnlock();
  return return_code;
}


//********************************************************
//* Name: genaNotifyReceived
//* Description:  Function called from genaCallback to handle reception of events (client).
//*               Function validates that the headers of the request confrom to the Upnp v 1.0 spec.
//*               Function parses the content of the request as XML. (only checks if it is valid XML)
//*               Function then tries to find and lock the client handle which corrsponds to the incoming SID
//*               If the SID is valid, respond OK, increment callback reference count, unlock client handle
//*               make client callback with Upnp_Event struct.
//*               Note: The values passed in the Upnp_event struct are only valid during the callback.
//* In:           http_message request (parsed http_message)
//*               int sockfd  (socket)
//* Out:          None
//* Return Codes: None
//* Error Codes:  None
//*               
//********************************************************

void genaNotifyReceived(http_message request, int sockfd)
{
	struct Upnp_Event event_struct;
	token temp_buff;
	token NT;
	token NTS;
	int eventKey;
	token sid;
	client_subscription * subscription;
	Upnp_Document ChangedVars;
	struct Handle_Info *handle_info;
	void * cookie;
	Upnp_FunPtr callback;
	UpnpClient_Handle client_handle;

	//get SID
	if (!search_for_header(&request,"SID",&sid))
	{
		respond(sockfd, MISSING_SID);
		return;
	}

	//get Event Key
	if ( (!search_for_header(&request,"SEQ",&temp_buff) || (sscanf(temp_buff.buff,"%d",&eventKey)!=1)))
	{
		respond(sockfd,BAD_REQUEST);
		return;
	}

	//get NT and NTS header
	if ( (!search_for_header(&request,"NT",&NT)) || (!search_for_header(&request,"NTS",&NTS)))
	{
		respond(sockfd,BAD_REQUEST);
		return;
	}

	//verify NT and NTS headers
	if ( ( (NT.size== (int)strlen("upnp:event")) && (strncmp(NT.buff,"upnp:event",NT.size)))
		|| ( (NT.size== (int)strlen("upnp:propchange")) 
		&& (strncmp(NTS.buff,"upnp:propchange",NTS.size))) )
	{
		respond(sockfd,INVALID_NT);
		return;
	}

	//parse the content (should be XML)
//	if ( (request.content.size==0) || ( (ChangedVars=UpnpParse_Buffer(request.content.buff))==NULL))
	if ( (request.content.size==0) || ( (ChangedVars=UpnpParse_BufferNoEx(request.content.buff))==NULL))
	{
		respond(sockfd,BAD_REQUEST);
		return;
	}

	//Lock handle
	HandleLock();

	if ( (GetClientHandleInfo(&client_handle, &handle_info)!=HND_CLIENT))
	{
		respond(sockfd,INVALID_SID);
		HandleUnlock();
		UpnpDocument_free(ChangedVars);
		return;
	}

	if (((subscription= GetClientSubActualSID(handle_info->ClientSubList, &sid))==NULL))
	{
		if (eventKey==0) 
		{
			//wait until we've finished processing a subscription (if we are in the middle)
			//this is to avoid mistakenly rejecting the first event if we receive it before the subscription response
			HandleUnlock();
			//try and get Subscription Lock (in case we are in the process of subscribing)
			SubscribeLock();
			//get HandleLock again;
			HandleLock();

			if ( (GetClientHandleInfo(&client_handle, &handle_info)!=HND_CLIENT))
			{
				respond(sockfd,INVALID_SID);
				SubscribeUnlock();
				HandleUnlock();
				UpnpDocument_free(ChangedVars);
				return;
			}

			if (  ( (subscription= GetClientSubActualSID(handle_info->ClientSubList, &sid))==NULL))
			{
				respond(sockfd,INVALID_SID);
				SubscribeUnlock();
				HandleUnlock();
				UpnpDocument_free(ChangedVars);
				return;
			}
			SubscribeUnlock();
		}
		else
		{
			respond(sockfd,INVALID_SID);
			HandleUnlock();
			UpnpDocument_free(ChangedVars);
			return;
		}

	}

	respond(sockfd,HTTP_OK_CRLF);

	// fill event struct
	strncpy((char *)event_struct.Sid,subscription->sid, 42);
	event_struct.EventKey=eventKey;
	event_struct.ChangedVariables=ChangedVars;

	//copy callback
	callback=handle_info->Callback;
	cookie=handle_info->Cookie;

	HandleUnlock();

	//make call back with event struct
	//in the future should find a way of mainting
	//that the handle is not unregistered in the middle of a 
	//callback
	callback(UPNP_EVENT_RECEIVED,&event_struct,cookie);

	UpnpDocument_free(ChangedVars);
 
}



//********************************************************
//* Name: genaUnSubscribe
//* Description:  Unsubscribes a SID
//*               First Validates the SID and client_handle
//*               Locks the Handle
//*               copies the subscription
//*               removes the subscription
//*               UnLocks the Handle
//*               Sends UNSUBSCRIBE http request to service
//*               processes request
//* In:           UpnpClient_Handle client_handle
//*               SID in_sid
//* Out:          None
//* Return Codes: UPNP_E_SUCCESS : if service responds OK
//* Error Codes:  UPNP_E_OUTOF_MEMORY: subscription may or may not be removed from client side.
//*               UPNP_E_UNSUBSCRIBE_UNACCEPTED : if service responds with other than OK (Note: subscription is still
//*                                                                                       removed from client side)
//*               UPNP_E_NETWORK_ERROR: see upnp.h
//*               
//*               
//********************************************************
int genaUnSubscribe(UpnpClient_Handle client_handle,
		    const Upnp_SID in_sid)
{
  client_subscription * sub;
  char * request;
  int request_size=0;
  int return_code =GENA_SUCCESS;
  char * response;
  http_message parsed_response;
  struct Handle_Info *handle_info;
  client_subscription sub_copy;
  
  HandleLock();
  //validate handle and sid

  if ( GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT)
    {
      HandleUnlock();
      return GENA_E_BAD_HANDLE;
    }
  
  if ( ( ( sub=GetClientSubClientSID(handle_info->ClientSubList,in_sid))==NULL))
     {
       HandleUnlock();
       return GENA_E_BAD_SID;
     }
  
  return_code=copy_client_subscription(sub,&sub_copy);
  
  RemoveClientSubClientSID(&handle_info->ClientSubList,in_sid);
  
  HandleUnlock();
  
  if (return_code!=HTTP_SUCCESS)
    return return_code;
  
  request_size=strlen("SID: \r\n\r\n")+ strlen(sub_copy.ActualSID)+1;
  request=(char *) malloc(request_size);
  
  if (request==NULL)
     {
       return UPNP_E_OUTOF_MEMORY;
     }
  sprintf(request,"SID: %s\r\n\r\n",sub_copy.ActualSID);
  
  return_code=transferHTTP("UNSUBSCRIBE",request,strlen(request),&response,sub_copy.EventURL);
  free(request);  
  free_client_subscription(&sub_copy);
  
  if (return_code!=HTTP_SUCCESS)
    {
      return return_code;
    }
   
  return_code=parse_http_response(response,&parsed_response,strlen(response));
  
  if (return_code==HTTP_SUCCESS)
    {
      if (strncasecmp(parsed_response.status.status_code.buff,"200",
		      strlen("200")))
	return_code=GENA_E_UNSUBSCRIBE_UNACCEPTED;
      else
	return_code=GENA_SUCCESS;
      free_http_message(&parsed_response);
    }
  free(response);
  
  
  return return_code;
  
}

//********************************************************
//* Name: genaSubscribe
//* Description:  Subscribes to a PublisherURL
//*               First Validates & locks handle (this is currently necessary to prevent client
//*                                               from recieving events, NOTE: this blocks all events and API
//*                                               calls until it finishes, worst case 30 seconds) 
//*               Sends SUBSCRIBE http request to service
//*               processes request
//*               Adds a Subscription to the clients subscription list, if service responds with OK
//* In:           UpnpClient_Handle client_handle
//*               char * PublisherURL (NULL Terminated, of the form : "http://134.134.156.80:4000/RedBulb/Event")
//*               int * TimeOut (requested Duration, if -1, then "infinite".
//* Out:          int * TimeOut (actual Duration granted by Service, -1 for infinite)
//*               SID out_sid (sid of subscription, memory passed in by caller)
//* Return Codes: UPNP_E_SUCCESS : if service responds OK
//* Error Codes:  UPNP_E_OUTOF_MEMORY
//*               UPNP_E_SUBSCRIBE_UNACCEPTED : if service responds with other than OK 
//*               UPNP_E_NETWORK_ERROR: error connecting (see upnp.h)
//*               
//*               
//********************************************************

int genaSubscribe(UpnpClient_Handle client_handle, char * PublisherURL,
		  int * TimeOut, Upnp_SID out_sid)
{
  
  int headers_size=0;
  char timeout[MAX_SECONDS];
  char * headers=NULL;
  char * response=NULL;
  int return_code=GENA_SUCCESS;
  client_subscription * newSubscription=NULL;
#ifdef USE_UUID
  uuid_t uuid;
  char temp_sid[SID_SIZE]; 
#endif
 
  char * ActualSID=NULL;
  token temp_headerValue;
  http_message parsed_response;
  struct Handle_Info *handle_info;
  char * EventURL=NULL;
 

  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"GENA SUBSCRIBE BEGIN"));
  HandleLock();
  //validate handle

  if ( (GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT))
    {
      HandleUnlock();
      return GENA_E_BAD_HANDLE;
    }
  
  HandleUnlock();

  if (TimeOut==NULL)
    sprintf(timeout,"%d",DEFAULT_TIMEOUT);
  else
    if ( (*TimeOut) >= 0)
      sprintf(timeout,"%d",(*TimeOut));
    else
      strcpy(timeout,"infinite");

  headers_size=strlen("CALLBACK: <http:///>\r\n") + strlen(LOCAL_HOST) +1+
    MAX_PORT_SIZE + strlen("NT: upnp:event\r\n") + 
    strlen("TIMEOUT: Second-\r\n\r\n") + MAX_SECONDS +1;
  
  headers=(char *) malloc(headers_size);
  
  if (headers==NULL)
    {
      HandleUnlock();
      return UPNP_E_OUTOF_MEMORY;
    }
  sprintf(headers, "CALLBACK: <http://%s:%d/>\r\nNT: upnp:event\r\nTIMEOUT: Second-%s\r\n\r\n",
	  LOCAL_HOST,LOCAL_PORT,timeout);

  
  SubscribeLock();
  return_code=transferHTTP("SUBSCRIBE",headers,strlen(headers),&response,PublisherURL);
  free(headers);
  HandleLock();
  if (return_code!=HTTP_SUCCESS)
    {
      HandleUnlock();
      SubscribeUnlock();
      DBGONLY(UpnpPrintf(UPNP_CRITICAL,GENA,__FILE__,__LINE__,"SUBSCRIBE FAILED in transfer error code: %d returned\n",
			 return_code));
      return return_code;
    }

  if ( (GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT))
    {
      HandleUnlock();
      SubscribeUnlock();
      free(response);
      return GENA_E_BAD_HANDLE;
    }
  
  //parse response
  
  
  return_code=parse_http_response(response,&parsed_response,strlen(response));
 
  if (return_code==HTTP_SUCCESS)
    {
      return_code=GENA_SUCCESS;
      if (!strncasecmp(parsed_response.status.status_code.buff,"200",strlen("200")))
	{ 
	  
	  if (search_for_header(&parsed_response,"SID",&temp_headerValue))
	    {
	      ActualSID=(char * ) malloc(temp_headerValue.size+1);
	      if (ActualSID==NULL)
		return_code= UPNP_E_OUTOF_MEMORY;
	      else
		{
		  //store ACTUAL SID and generate client sid
		 
		  memcpy(ActualSID,temp_headerValue.buff,
			 temp_headerValue.size); 
		  ActualSID[temp_headerValue.size]=0;
#ifdef USE_UUID
		  uuid_generate(uuid);
		  uuid_unparse(uuid,temp_sid);
		  sprintf((out_sid),"uuid:%s",temp_sid);
#else
//			sprintf((out_sid),"uuid:7d444840-9dc0-11d1-b245-5ffdce74fad2");
static int s_iRequestNumber = 0;
			sprintf((out_sid),"uuid:7d444840-9dc0-11d1-b245-5ffdce%06d", s_iRequestNumber++);
#endif
		}
	    }
	  else
	      return_code= GENA_E_BAD_RESPONSE;
	  
	  if (search_for_header(&parsed_response,"TIMEOUT",&temp_headerValue))
	    {
	      if (sscanf(temp_headerValue.buff,"Second-%d",TimeOut)!=1 )
		{
		  if (!strncasecmp(temp_headerValue.buff,
				   "Second-infinite",
				   strlen("Second-infinite")))
		    (*TimeOut)=-1;
		  else
		      return_code=  GENA_E_BAD_RESPONSE;
		}    
	     
	    }
	  else
	    return_code= GENA_E_BAD_RESPONSE;
	  
	  EventURL=(char*) malloc (strlen(PublisherURL)+1);
	  
	  if (EventURL==NULL)
	    return_code=UPNP_E_OUTOF_MEMORY;
	  else
	    strcpy(EventURL, PublisherURL);
	}
      else
	return_code=GENA_E_SUBSCRIPTION_UNACCEPTED;
      free_http_message(&parsed_response);
    }
  
  free(response);

  if (return_code==GENA_SUCCESS)
    {
      newSubscription=(client_subscription *) malloc(sizeof (client_subscription));
      
     

      if ( newSubscription==NULL)
	return_code=UPNP_E_OUTOF_MEMORY;
      else
	{
	  newSubscription->EventURL=EventURL;
	  newSubscription->ActualSID=ActualSID;
	  strncpy(newSubscription->sid,out_sid, 42);
	  newSubscription->RenewEventId=-1;
	  newSubscription->next=handle_info->ClientSubList;
	  handle_info->ClientSubList=newSubscription;
	  //schedule expire event
	  return_code=ScheduleGenaAutoRenew(client_handle,(*TimeOut),newSubscription);
					
	}	 
    }
    

  if (return_code!=GENA_SUCCESS)
    {
      if (ActualSID)
	free(ActualSID);
      if (EventURL)
	free(EventURL);
      if (newSubscription)
	free(newSubscription);
    }
  
  HandleUnlock();
  SubscribeUnlock();
  return return_code;
}


//********************************************************
//* Name: genaRenewSubscription
//* Description:  Renews a SID
//*               First Validates the SID and client_handle
//*               Locks the Handle
//*               copies the subscription
//*               UnLocks the Handle
//*               Sends RENEW (modified SUBSCRIBE) http request to service
//*               Locks Handle
//*               processes request
//*               Unlocks Handle
//* In:           UpnpClient_Handle client_handle
//*               SID in_sid 
//*               int * TimeOut (requested duration)
//* Out:          int * TimeOut (actual duration)
//* Return Codes: UPNP_E_SUCCESS : if service responds OK
//* Error Codes:  UPNP_E_OUTOF_MEMORY: subscription may or may not be removed from client side.
//*               UPNP_E_UNSUBSCRIBE_UNACCEPTED : if service responds with other than OK (Note: subscription is 
//*                                                                                       removed from client side)
//*               
//*               
//********************************************************

int genaRenewSubscription(UpnpClient_Handle client_handle,
			  const Upnp_SID in_sid,
			  int * TimeOut)
{
  int headers_size=0;
  char timeout[MAX_SECONDS];
  char * headers;
  char * response;
  int return_code=GENA_SUCCESS;
  client_subscription * sub; 
  client_subscription sub_copy;
  upnp_timeout * temp_event;
  struct Handle_Info *handle_info;
  void * temp;

  token temp_headerValue;

  http_message parsed_response;
 

  HandleLock();
  //validate handle and sid

  if ( GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT)
    {
      HandleUnlock();
      return GENA_E_BAD_HANDLE;
    }

  if ( ( ( sub=GetClientSubClientSID(handle_info->ClientSubList,in_sid))==NULL))
    {
      HandleUnlock();
      return GENA_E_BAD_SID;
    }

   //remove old events
  
  if (RemoveTimerEvent(sub->RenewEventId,&temp,&GLOBAL_TIMER_THREAD))
    {
      temp_event=(upnp_timeout *) temp;
      free_upnp_timeout(temp_event);
    }
  DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"REMOVED AUTO RENEW  EVENT"));
  sub->RenewEventId=-1;
  return_code=copy_client_subscription(sub,&sub_copy);
  
  HandleUnlock();
  
  if (return_code!=HTTP_SUCCESS)
    return return_code;
  
  if (TimeOut==NULL)
      sprintf(timeout,"%d",1801);
  else
    if ( (*TimeOut) >= 0)
      sprintf(timeout,"%d",(*TimeOut));
    else
      sprintf(timeout,"infinite");
  
    
  headers_size=strlen("SID: \r\n") + strlen(sub_copy.ActualSID) + 
    strlen("TIMEOUT: Second-\r\n\r\n") + MAX_SECONDS +1;
  
  headers=(char *) malloc(headers_size);
  
  if (headers==NULL)
    {
      return UPNP_E_OUTOF_MEMORY;
    }
  
  sprintf(headers, "SID: %s\r\nTIMEOUT: Second-%s\r\n\r\n",sub_copy.ActualSID,
	  timeout);

  
  return_code=transferHTTP("SUBSCRIBE",headers,strlen(headers),&response,sub_copy.EventURL);
  free(headers);
  free_client_subscription(&sub_copy);
  
  HandleLock();
  
  if ( GetHandleInfo(client_handle,&handle_info)!=HND_CLIENT)
    {
      HandleUnlock();
      if (return_code==HTTP_SUCCESS)
	free(response);
      return GENA_E_BAD_HANDLE;
    }

  
  if (return_code!=HTTP_SUCCESS)
    { 
      //network failure (remove client sub)
      RemoveClientSubClientSID(&handle_info->ClientSubList,in_sid);
      HandleUnlock();
      return return_code;
    }
  
  
  //validate sid
  
  if ( ( ( sub=GetClientSubClientSID(handle_info->ClientSubList,in_sid))==NULL))
    {
      HandleUnlock();
      free(response);
      return GENA_E_BAD_SID;
    }
  
  //parse response


  return_code=parse_http_response(response,&parsed_response,strlen(response));

  
  if (return_code==HTTP_SUCCESS)
    {
      if (!strncasecmp(parsed_response.status.status_code.buff,"200",strlen("200")))
	{
	  return_code=GENA_SUCCESS;
	  //get SID
	  if (search_for_header(&parsed_response,"SID",&temp_headerValue))
	    {
	      free(sub->ActualSID);

	      sub->ActualSID=(char * ) malloc(temp_headerValue.size+1);
	      if (sub->ActualSID==NULL)
		return_code = UPNP_E_OUTOF_MEMORY;
	      else
		{
		  //store ACTUAL SID 
		  memcpy(sub->ActualSID,temp_headerValue.buff,
			 temp_headerValue.size); 
		  sub->ActualSID[temp_headerValue.size]=0;
		}
	    }
	  else
	      return_code= GENA_E_BAD_RESPONSE;
	  
	  //get Timeout
	  if (search_for_header(&parsed_response,"TIMEOUT",&temp_headerValue))
	    {
	      if (sscanf(temp_headerValue.buff,"Second-%d",TimeOut)!=1 )
		{
		  if (!strncasecmp(temp_headerValue.buff,"Second-infinite",strlen("Second-infinite")))
		    (*TimeOut)=-1; 
		  else
		    return_code=  GENA_E_BAD_RESPONSE;
		}    
	    }
	  else
	    return_code = GENA_E_BAD_RESPONSE;
	}
      else
	return_code=GENA_E_SUBSCRIPTION_UNACCEPTED;
      
      free_http_message(&parsed_response);
    }
  
  free(response);
  
  if (return_code==GENA_SUCCESS)
    return_code=ScheduleGenaAutoRenew(client_handle,(*TimeOut),sub);

  if (return_code!=GENA_SUCCESS)
    RemoveClientSubClientSID(&handle_info->ClientSubList,sub->sid);

  HandleUnlock();
  return return_code;
}

#endif	// INCLUDE_CLIENT_APIS

#endif
