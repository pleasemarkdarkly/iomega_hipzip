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
// $Revision: 1.14 $
// $Date: 2000/10/05 17:26:12 $
//     

#include <util/upnp/genlib/client_table.h>

CLIENTONLY(
int copy_client_subscription(client_subscription * in, client_subscription * out)
{
  int len = strlen(in->ActualSID) + 1;
  int len1= strlen(in->EventURL)+1;

  memcpy(out->sid,in->sid,SID_SIZE);
  out->sid[SID_SIZE]=0;
  out->ActualSID= (char * ) malloc (len);
  out->EventURL=(char *) malloc (len1);
  if(  ( out->EventURL==NULL) || (out->ActualSID==NULL))
    return UPNP_E_OUTOF_MEMORY;
  memcpy(out->ActualSID,in->ActualSID,len);
  memcpy(out->EventURL, in->EventURL, len1);

  //copies do not get RenewEvent Ids or next
  
  out->RenewEventId=-1;
  out->next=NULL;
  return HTTP_SUCCESS;
  
}

void free_client_subscription(client_subscription * sub)
{
  upnp_timeout *event;
  void *temp;

  if (sub)
    {
      if (sub->ActualSID)
	free(sub->ActualSID);
      if (sub->EventURL)
	free(sub->EventURL);
     
      if (RemoveTimerEvent(sub->RenewEventId,&temp,&GLOBAL_TIMER_THREAD))
	{
	  event=(upnp_timeout *) temp;
	  free_upnp_timeout(event);
	}
      sub->RenewEventId=-1;
    }
}

void freeClientSubList(client_subscription * list)
{
  client_subscription * next;
  while(list)
    {
      free_client_subscription(list);
      next=list->next;
      free(list);
      list=next;
    }
}

void RemoveClientSubClientSID(client_subscription **head, const Upnp_SID sid)
{
  client_subscription * finger=(*head);
  client_subscription * previous=NULL;
  
  while (finger)
    {
      if ( !(strcmp(sid,finger->sid)))
	{
	  if (previous)
	    previous->next=finger->next;
	  else
	    (*head)=finger->next;
	  finger->next=NULL;
	  freeClientSubList(finger);
	  finger=NULL;
	}
      else
	{ previous=finger;
	  finger=finger->next;
	}
    }
}


client_subscription * GetClientSubClientSID(client_subscription *head, 
					    const Upnp_SID sid)
{
  client_subscription * next =head;

  while (next)
    {
      if ( ! strcmp(next->sid,sid))
	break;
      else 
	{
	  next=next->next;
	}
    } 
  return next;

}

client_subscription * GetClientSubActualSID(client_subscription *head, token * sid)
{
  client_subscription * next =head;
  
  while (next)
    {
      if ( ! memcmp(next->ActualSID,sid->buff,sid->size))
	break;
      else 
	{
	  next=next->next;
	}
    } 
  return next;
}
)
