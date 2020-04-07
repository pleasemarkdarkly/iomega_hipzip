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
// $Revision: 1.23 $
// $Date: 2000/10/05 17:31:29 $
//     


//Functions for generating a Service Table from a Dom Node
//and for traversing that list 
// also included are functions to work with Subscription Lists


#ifndef _SERVICE_TABLE
#define _SERVICE_TABLE

#include <util/upnp/api/upnp_debug.h>
#include <util/upnp/genlib/http_client.h>
#include <util/upnp/upnpdom/all.h>
#include <util/upnp/upnpdom/domCif.h>
#include <util/upnp/api/upnp.h>

#include <stdio.h>
//#include <malloc.h>
#include <time.h>

#ifndef _WIN32
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <cyg/kernel/kapi.h>
#endif
#endif
#define SID_SIZE  41

DEVICEONLY(

typedef struct SUBSCRIPTION {
  Upnp_SID sid;
  int eventKey;
  int ToSendEventKey;
  time_t expireTime;
  int active;
  URL_list DeliveryURLs;
  struct SUBSCRIPTION *next;
} subscription;


typedef struct SERVICE_INFO {
  char * serviceType;
  char * serviceId;
  char * SCPDURL ;
  char * controlURL;
  char * eventURL;
  char * UDN;
  int active;
  int TotalSubscriptions;
  subscription *subscriptionList;
  struct SERVICE_INFO * next;
} service_info;

typedef struct SERVICE_TABLE {
  char * URLBase;
  service_info *serviceList;
} service_table;



//finds the serviceID/DeviceID pair in the service table
//returns a pointer to the service info (pointer should NOT be deallocated)
//returns the device handle of the root device for the found service
//returns HND_INVALID if not found
service_info *FindServiceId( service_table * table, 
			     const char * serviceId, const char * UDN);

//finds the eventURLPath in the service table
// paths are matched EXACTLY (including a query portion) and the initial /
//returns HND_INVALID if not found
service_info * FindServiceEventURLPath( service_table *table,
					  char * eventURLPath
					 );

//returns HND_INVALID if not found
service_info * FindServiceControlURLPath( service_table *table,
					  char * controlURLPath);

//for testing
DBGONLY(void printService(service_info *service,Dbg_Level
				   level,
				   Dbg_Module module));

DBGONLY(void printServiceList(service_info *service,
				       Dbg_Level level, Dbg_Module module));

DBGONLY(void printServiceTable(service_table *
					table,Dbg_Level
					level,Dbg_Module module));

//for deallocation (when necessary)

//frees service list (including head)
void freeServiceList(service_info * head);

//frees dynamic memory in table, (does not free table, only memory within the structure)
void freeServiceTable(service_table * table);

//to generate the service table from a dom node and handle
//returns a 1 if successful 0 otherwise
//all services are initially active and have empty subscriptionLists
int getServiceTable(Upnp_Node node,  
		    service_table * out, char * DefaultURLBase);


//functions for Subscriptions

//removes the subscription with the SID from the subscription list pointed to by head
//returns the new list in head
void RemoveSubscriptionSID(Upnp_SID sid, service_info * service);

//returns a pointer to the subscription with the SID, NULL if not found
subscription * GetSubscriptionSID(Upnp_SID sid,service_info * service);   


//returns a pointer to the first subscription
subscription * GetFirstSubscription(service_info *service);

//returns the next ACTIVE subscription that has not expired
// Expired subscriptions are removed
subscription * GetNextSubscription(service_info * service, subscription *current);

//frees subscriptionList (including head)
void freeSubscriptionList(subscription * head);

//Misc helper functions
int getSubElement(const char * element_name,Upnp_Node node, 
		  Upnp_Node *out);

char * getElementValue(Upnp_Node node);

int copy_subscription(subscription *in, subscription *out);

void freeSubscription(subscription * sub);
)
#endif
