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
///////////////////////////////////////////////////////////////////////////
//
// $Revision: 1.28 $
// $Date: 2000/10/05 17:27:42 $
//     

#ifndef _GENA_
#define _GENA_


#include <util/upnp/api/config.h>
#include <util/upnp/api/upnp_debug.h>
#include <util/upnp/genlib/service_table.h>
#include <util/upnp/genlib/miniserver.h>
#include <util/upnp/genlib/http_client.h>
#include <util/upnp/api/upnp.h>
#include <util/upnp/api/interface.h>

#include <time.h>

#include <util/upnp/genlib/scheduler.h>
#include <string.h>
#include <util/upnp/genlib/client_table.h>


#ifndef _WIN32
//#include <uuid/uuid.h>
#else
#include "win32.h"
#endif

#define XML_VERSION "<?xml version='1.0' encoding='ISO-8859-1' ?>\n"
#define XML_PROPERTYSET_HEADER "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">\n"

#define UNABLE_MEMORY "HTTP/1.1 500 Internal Server Error\r\n\r\n"
#define UNABLE_SERVICE_UNKNOWN "HTTP/1.1 404 Not Found\r\n\r\n"
#define UNABLE_SERVICE_NOT_ACCEPT "HTTP/1.1 503 Service Not Available\r\n\r\n"


#define NOT_IMPLEMENTED "HTTP/1.1 501 Not Implemented\r\n\r\n"
#define BAD_REQUEST "HTTP/1.1 400 Bad Request\r\n\r\n"
#define INVALID_NT BAD_CALLBACK
#define BAD_CALLBACK "HTTP/1.1 412 Precondition Failed\r\n\r\n" 
#define HTTP_OK_CRLF "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define INVALID_SID BAD_CALLBACK
#define MISSING_SID BAD_CALLBACK
#define MAX_CONTENT_LENGTH 20
#define MAX_SECONDS 10
#define MAX_EVENTS 20
#define MAX_PORT_SIZE 10
#define GENA_E_BAD_RESPONSE UPNP_E_BAD_RESPONSE
#define GENA_E_BAD_SERVICE UPNP_E_INVALID_SERVICE
#define GENA_E_SUBSCRIPTION_UNACCEPTED UPNP_E_SUBSCRIBE_UNACCEPTED
#define GENA_E_BAD_SID UPNP_E_INVALID_SID
#define GENA_E_UNSUBSCRIBE_UNACCEPTED UPNP_E_UNSUBSCRIBE_UNACCEPTED
#define GENA_E_NOTIFY_UNACCEPTED UPNP_E_NOTIFY_UNACCEPTED
#define GENA_E_NOTIFY_UNACCEPTED_REMOVE_SUB -9
#define GENA_E_BAD_HANDLE UPNP_E_INVALID_HANDLE
#define XML_ERROR -5
#define XML_SUCCESS 1
#define GENA_SUCCESS UPNP_E_SUCCESS
#define CALLBACK_SUCCESS 0
#define DEFAULT_TIMEOUT 1801

#ifndef _WIN32

#ifdef USE_PTHREADS

extern pthread_mutex_t GlobalClientSubscribeMutex;
#define SubscribeLock() DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Trying Subscribe Lock")); pthread_mutex_lock(&GlobalClientSubscribeMutex); DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscribe Lock"))
#define SubscribeUnlock() DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Trying Subscribe UnLock")); pthread_mutex_unlock(&GlobalClientSubscribeMutex); DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscribe UnLock"))

#else	// USE_PTHREADS

extern cyg_mutex_t GlobalClientSubscribeMutex;
#define SubscribeLock() DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Trying Subscribe Lock")); cyg_mutex_lock(&GlobalClientSubscribeMutex); DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscribe Lock"))
#define SubscribeUnlock() DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Trying Subscribe UnLock")); cyg_mutex_unlock(&GlobalClientSubscribeMutex); DBGONLY(UpnpPrintf(UPNP_INFO,GENA,__FILE__,__LINE__,"Subscribe UnLock"))

#endif	// USE_PTHREADS

#endif


void genaCallback (const char *document, int sockfd);


typedef struct NOTIFY_THREAD_STRUCT {
  char * headers;
  char * propertySet;
  char * servId;
  char * UDN;
  Upnp_SID sid;
  int eventKey;
  int *reference_count;
  UpnpDevice_Handle device_handle;
} notify_thread_struct;



int respond(int sockfd, char * message);


//client
CLIENTONLY(void genaNotifyReceived(http_message request, int sockfd);)

CLIENTONLY(int genaSubscribe(UpnpClient_Handle client_handle,char * PublisherURL,int * TimeOut, Upnp_SID  out_sid );)

CLIENTONLY(int genaUnSubscribe(UpnpClient_Handle client_handle,                                                const Upnp_SID in_sid);)

CLIENTONLY(int genaUnregisterClient(UpnpClient_Handle client_handle);)

//server


DEVICEONLY(int genaUnregisterDevice(UpnpDevice_Handle device_handle);)

DEVICEONLY(void genaUnsubscribeRequest(http_message request, int sockfd);)

DEVICEONLY(void genaRenewRequest(http_message request, int sockfd);)

CLIENTONLY(int genaRenewSubscription(UpnpClient_Handle client_handle,
					      const Upnp_SID in_sid,
					      int * TimeOut);)

//int UpnpRenewSubscriptionAsync(IN UpnpClient_Handle Hnd, INOUT int TimeOut,
//              IN Upnp_SID SubsId, IN Upnp_FunPtr Fun,IN void * Cookie);

DEVICEONLY(void genaSubscriptionRequest(http_message request, int sockfd);)

DEVICEONLY(void genaSubscribeOrRenew(http_message request, int sockfd);)

DEVICEONLY(int genaNotifyAll(UpnpDevice_Handle device_handle,
			   char *UDN,
			   char *servId,
			   char **VarNames,
			   char **VarValues,
		    int var_count
				      );)

DEVICEONLY(int genaNotifyAllExt(UpnpDevice_Handle device_handle, char *UDN, char *servId,IN Upnp_Document PropSet);)

DEVICEONLY(int genaInitNotify(UpnpDevice_Handle device_handle,
			    char *UDN,
			    char *servId,
			    char **VarNames,
			    char **VarValues,
		   int var_count,
				       Upnp_SID sid);)

DEVICEONLY( int genaInitNotifyExt(UpnpDevice_Handle device_handle, char *UDN, char *servId,IN Upnp_Document PropSet, Upnp_SID sid);)


#endif
