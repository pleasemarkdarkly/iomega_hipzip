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
// $Revision: 1.12 $
// $Date: 2000/10/05 17:25:42 $
//     

#include <util/upnp/api/config.h>


#if EXCLUDE_GENA == 0
#include <util/upnp/protocols/gena.h>

//********************************************************
//*Name: respond
//*Description:   Function to write bytes to a socket and closes the socket
//*               tries to write bytes for RESPONSE_TIMEOUT time, then times out
//* In:           int sockfd (socket file descriptor)
//*               char * message (bytes to be sent NULL terminated)
//* Out:          None
//* Return Codes: Returns UPNP_E_SUCCESS
//* Error Codes:  Returns UPNP_E_SOCKET_WRITE on error
//*               
//********************************************************
int respond(int sockfd, char * message)
{
  int return_code;
  int length=strlen(message);
  return_code=write_bytes(sockfd,message,length,RESPONSE_TIMEOUT);
  close(sockfd);
  if (return_code!=length) 
    {
      DBGONLY(UpnpPrintf(UPNP_CRITICAL,GENA,__FILE__,__LINE__,"Gena server WRITE ERROR: %d sending response:\n'%s'\n",UPNP_E_SOCKET_WRITE,message));
      return UPNP_E_SOCKET_WRITE;
    }
  else
   {
     DBGONLY(UpnpPrintf(UPNP_PACKET,GENA,__FILE__,__LINE__,"Gena server sent response:\n'%s'\n",message));
     return UPNP_E_SUCCESS;
   }
  
 
}

//********************************************************
//* Name: genaCallback
//* Description:  genaCallback called from miniserver when a gena request has been received.
//*               the request is parsed and the appropiate gena function is called.
//* In:           char * document (request passed in from miniserver, 
//*                                caller is responsible for memory)
//*               int sockfd (socket passed in from miniserver)
//*               
//* Out:          None
//* Return Codes: None
//* Error Codes:  None
//*               
//********************************************************

void genaCallback (const char *document, int sockfd)
{
  int success=0;

  http_message request;
  if ( (parse_http_request( (char *) document,&request,strlen(document)))==HTTP_SUCCESS)
    { 
      if ( !(strncasecmp(request.request.method.buff,"SUBSCRIBE",
			 request.request.method.size)))
	{DEVICEONLY(genaSubscribeOrRenew(request, sockfd);success=1);}
      else
	
      if (!(strncasecmp(request.request.method.buff,"UNSUBSCRIBE",
			  request.request.method.size)))
	{DEVICEONLY(genaUnsubscribeRequest(request,sockfd);success=1);}
      
      else
      
      if (!(strncasecmp(request.request.method.buff,"NOTIFY",
			    request.request.method.size)))
	{
	  CLIENTONLY(genaNotifyReceived(request,sockfd);success=1);
	}
      if (!success)
	{
	  respond(sockfd,NOT_IMPLEMENTED);
	}

      free_http_message(&request);
    }
  else
    {
      respond(sockfd,BAD_REQUEST);
      close(sockfd);
    }

}
#endif
