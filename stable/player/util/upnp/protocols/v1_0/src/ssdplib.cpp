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
// $Revision: 1.70 $
// $Date: 2000/10/08 18:32:40 $
//  


#include <util/upnp/api/config.h>

#include <util/upnp/protocols/ssdplib.h>

#include <stdio.h>
#include <util/upnp/genlib/scheduler.h>
#include <util/upnp/genlib/interrupts.h>
#include <util/upnp/api/interface.h>


#include <cyg/kernel/kapi.h>

#include <network.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAX_TIME_TOREAD  45
//void RequestHandler();
void RequestHandler(ThreadData *ThData);
Event  ErrotEvt;
enum Listener{Idle,Stopping,Running}ListenerState=Idle;
#ifndef _WIN32
#ifdef USE_PTHREADS
pthread_t  ListenerThread = 0;
#else
cyg_handle_t	ListenerThread = 0;
#endif
#else
#include <io.h>
#include "../win32/wthread.h"
#include <ws2tcpip.h>
extern HANDLE CloseEvent;
#endif

SsdpFunPtr CallBackFn;


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int GetLocalHost(char * Ip)
 // Description : Returns the local IP address string.
 // Parameters  : Ip : Local IP address.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 #ifdef _WIN32
 int getlocalhostname(char * Ip)
 {
    struct hostent * HostData;
    char **pptr;
    unsigned char *p;
    int NumAddr=0, HostName[500];

    gethostname((char*)HostName,500);
    if((HostData = gethostbyname((char *)HostName))!= NULL)
    {
        pptr = HostData->h_addr_list;
        for(;*pptr != NULL; pptr++)
        {
            p = (unsigned char*) *pptr;
            sprintf(Ip, "%d.%d.%d.%d",p[0],p[1],p[2],p[3]);
            ++NumAddr;
        }

   }
   return NumAddr;
}
#endif

#if EXCLUDE_SSDP == 0

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void SendErrorEvent(int ErrCode)
 // Description : This function is used to send the critical error notification to the callback function like memory
 //               allocation, network error etc.
 //
 // Parameters  : ErrCode : Error code to be passed back to the callback function
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void SendErrorEvent(int ErrCode)
 {
   DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation !!!\n");)
   ErrotEvt.ErrCode = ErrCode;
   CallBackFn((Event *)&ErrotEvt);
 }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void GetRandomNumber(int Max)
 // Description : This function generates a milisecond delay random no.
 //
 // Parameters  : Max : Max delay in seconds.
 //
 // Return value: Randon number.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetRandomNumber(int Max)
{
  int MaxLt;
   
  MaxLt = Max*1000 - 500; // 1/2 sec less delay

//  srand(StartupTime);
  return (rand() % MaxLt);
}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void RemoveThreadData(ThreadData *ThData)
 // Description : This function does  the cleanup job, it removes any data allocated by fn PutThreadData().
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void RemoveThreadData(ThreadData *ThData)
 {

     free(ThData->Data);
     free(ThData);

 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int PutThreadData(ThreadData *ThData,char * Rqst, struct sockaddr_in * DestAddr, int Mx)
 // Description : This function stored the data to be used by the independent thread.
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: 1 if successfull , -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int PutThreadData(ThreadData *ThData,char * Rqst, struct sockaddr_in * DestAddr, int Mx)
 {

   ThData->Data = (char *)malloc(strlen(Rqst)+1);
   if( ThData->Data == NULL)
   {
       SendErrorEvent( UPNP_E_OUTOF_MEMORY);
       return -1;
   }

   strcpy(ThData->Data,Rqst);
   ThData->Mx = Mx;

   if(DestAddr != NULL)
   {
       ThData->DestAddr.sin_family = DestAddr->sin_family;
       ThData->DestAddr.sin_addr.s_addr = DestAddr->sin_addr.s_addr;
       ThData->DestAddr.sin_port = DestAddr->sin_port;
   }
   else  ThData->DestAddr.sin_port =0;

   return 1;

 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void TransferResEvent( ThreadData *ThData)
 // Description : This function process the HTTP data packet received by the multicast channel and pass it back to the
 //               callback function.
 //
 // Parameters  : ThData : Data packet to be passed back to the Thread.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TransferResEvent( ThreadData *ThData)
{
    Event * Evt = (Event *)malloc(sizeof(Event));
    Evt->ErrCode = NO_ERROR_FOUND;
    if( Evt == NULL)
    {
         SendErrorEvent( UPNP_E_OUTOF_MEMORY);
         return;
    }
    else
    {
         if (ThData->Data != NULL)
         {
              Evt->DestAddr =  &(ThData->DestAddr);
              if (AnalyzeCommand(ThData->Data,Evt) > 0)
              {
                 if(Evt->Cmd == SSDP_SEARCH && Evt->Mx > 1)
                 {
                    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"Sleeping for %d ms",GetRandomNumber(Evt->Mx));)

                    #ifndef _WIN32
/* - ecm */
/*                       usleep(GetRandomNumber(Evt->Mx)); */
                       cyg_thread_delay(GetRandomNumber(Evt->Mx) / 10);
                    #else
                       Sleep(GetRandomNumber(Evt->Mx));
                    #endif
                 }
 
                 CallBackFn(Evt);

              }
              else
		            {
                 DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in parsing !!!\n");)
              }
         }

    }

    RemoveThreadData(ThData);
    free(Evt);
    return;
 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int StartEventHandler(char *EventBuf, struct sockaddr_in * DestAddr)
 // Description : This function  starts the TransferResEvent thread, which later process this Event packet.
 //
 // Parameters  : EventBuf : Raw HTTP packet received in multicast channel.
 //               DestAddr : Address of the client from which this packet is received.
 // Return value: 1 if successfull , -1 otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int StartEventHandler(char *EventBuf, struct sockaddr_in * DestAddr)
{
	ThreadData *ThData;

	ThData = (ThreadData *)malloc(sizeof(struct TData));
	if(ThData  == NULL)
	{
		SendErrorEvent( UPNP_E_OUTOF_MEMORY);
		return -1;
	}


	PutThreadData(ThData,EventBuf,DestAddr,0);
    tpool_Schedule((ScheduleFunc)TransferResEvent,ThData);
    //TransferResEvent(ThData);
	return 1;

}


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void  ListenMulticastChannel()
 // Description : This function run as a independent thread listen for the response coming on the multicast channel.
 //               It starts during the lib initialization and run until closed by the DeInit() functin call.
 // Parameters  : SsdpSock : Multicast Socket on which it will listen for the request.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _WIN32
 void  ListenMulticastChannel(int SsdpSock)
 {
   int  socklen,ByteReceived;
   struct sockaddr_in ClientAddr;
   fd_set RdSet;
   char	RequestBuf[BUFSIZE];

#ifdef USE_PTHREADS
   ListenerThread = pthread_self();
#else
   ListenerThread = cyg_thread_self();
#endif
   ListenerState = Running;
   DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Multicast listener started...\n");)

//   bzero((char *)&RequestBuf, BUFSIZE);
//   bzero((char *)&ClientAddr, sizeof(struct sockaddr_in));
   memset((char *)&RequestBuf, 0, BUFSIZE);
   memset((char *)&ClientAddr, 0, sizeof(struct sockaddr_in));

   for(;;)
   {


       FD_ZERO(&RdSet);
       FD_SET(SsdpSock,&RdSet);

       if (ListenerState == Stopping) break;

       if (select(SsdpSock+1,&RdSet,NULL,NULL,NULL) == -1)
       {
           if (errno == EINTR && ListenerState == Stopping )
           {
              DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"SSDP got stop signal, Exiting!!!!!!!!!!!!!!\n");)
              break;
           }
           else
           {
              DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select call !!!!!!!!!!!! \n");)
              if ( errno == EBADF)
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
              else if ( errno ==  EINTR)
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught. \n");)}
              else if ( errno ==  EINVAL )
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
              else if ( errno ==   ENOMEM )
		            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Select was unable to allocate memory for internal tables.\n");)}
              break;
           }

       }
       else
       {

          if(FD_ISSET(SsdpSock,&RdSet))
          {
               socklen = sizeof(struct sockaddr_in);
               ByteReceived = recvfrom(SsdpSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&ClientAddr, (socklen_t*)&socklen);
               if(ByteReceived > 0 )
               {
                  RequestBuf[ByteReceived] = '\0';
                  DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Received response !!!  %s From host %s \n", RequestBuf,inet_ntoa(ClientAddr.sin_addr));)
                  DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received multicast packet: \n %s\n",RequestBuf);)
                  StartEventHandler(RequestBuf,&ClientAddr );
               }

            }
       }


   }

   close(SsdpSock);
   ListenerState = Idle;
   return;

 }
#else
 void  ListenMulticastChannel(int SsdpSock)
 {
   int  socklen,ByteReceived,k;
   struct sockaddr_in ClientAddr;
   char	RequestBuf[BUFSIZE];
   HANDLE SocketEvent;
   DWORD dwEvent; 
   HANDLE Hnd_Tbl[2],CloseEvt;
   WSANETWORKEVENTS NetworkEvent; 

   SocketEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
   CloseEvt= OpenEvent(SYNCHRONIZE,TRUE,"ThreadCloseEvent");
   k = GetLastError();
   Hnd_Tbl[0] = SocketEvent;
   Hnd_Tbl[1] = CloseEvt;

   if (SocketEvent == NULL) return;
   if(WSAEventSelect(SsdpSock, SocketEvent, FD_READ) == SOCKET_ERROR)
    return;

   DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Multicast listener started.......\n");)

//   bzero((char *)&RequestBuf, BUFSIZE);
//   bzero((char *)&ClientAddr, sizeof(struct sockaddr_in));
   memset((char *)&RequestBuf, 0, BUFSIZE);
   memset((char *)&ClientAddr, 0, sizeof(struct sockaddr_in));

   for(;;)
   {
       dwEvent = WaitForMultipleObjects(2, Hnd_Tbl, FALSE,INFINITE); 
       switch (dwEvent) 
       {    
  
           case WAIT_OBJECT_0 + 0:
                                 if(WSAEnumNetworkEvents(SsdpSock,SocketEvent,&NetworkEvent) == SOCKET_ERROR) 
                                 { 
                                    DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Multicast channel returned %d",WSAGetLastError());) 
 
                                 } 
                                 if((NetworkEvent.lNetworkEvents & FD_READ))
                                 {         
                                    socklen = sizeof(struct sockaddr_in);
                                    ByteReceived = recvfrom(SsdpSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&ClientAddr, &socklen);
                                    if(ByteReceived > 0 )
                                    {
                                       RequestBuf[ByteReceived] = '\0';
                                       DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Received response !!!  %s From host %s \n", RequestBuf,inet_ntoa(ClientAddr.sin_addr));)
                                       DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received multicast packet: \n %s\n",RequestBuf);)
                                       StartEventHandler(RequestBuf,&ClientAddr );
                                    }
                                    else
                                    {
                                       DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in receve\n");)
                                    }
     
                                  }          
                                  break; 


           case WAIT_OBJECT_0 + 1:
                                  DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Received close event, closing multicast channel\n");)
                                  CloseHandle(SocketEvent);
                                  close(SsdpSock);
                                  return;  

          case WAIT_FAILED : k = GetLastError();
                             DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Wait Failed\n");)
                             CloseHandle(SocketEvent);
                             close(SsdpSock);
                             return;
 

           default: k = GetLastError();
                    DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Wait error: %d\n", GetLastError());)
                    CloseHandle(SocketEvent);
                    close(SsdpSock);
                    return; 
      }

   }

   return;

 }
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void DeInitSsdpLib()
 // Description : This function stops  the multicast thread.
 // Parameters  : None
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int __ssdp_sock = -1;
#ifndef _WIN32
 void DeInitSsdpLib()
 {
 	int code = -1;
 	
 	if ( ListenerState == Idle )
 		return;	// not running
 		
 	ListenerState = Stopping;
 	
 	
 	// keep sending signals until server stops
 	while ( 1 )
 	{
 		  if ( ListenerState == Idle )
 		  {
 			   break;
 		  }
 		
 		  DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Sending interrupt\n");)
#ifdef USE_PTHREADS
 		  code = tintr_Interrupt( ListenerThread );
#else
          if( cyg_thread_delete( ListenerThread ) ) {
              ListenerState = Idle;
              if( __ssdp_sock >= 0 ) close(__ssdp_sock);
          }
#endif
 		  if (code < 0)
 		  { 
              DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Interrupt Failed............");)
 		  }
 		
 		  if(ListenerState == Idle)
 		  {
 			    break;
 		  }
 		
 		  cyg_thread_delay( 100 );		// pause before sending signals again
 	}
 }
#else
 void DeInitSsdpLib()
 {
 }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int InitSsdpLib(SsdpFunPtr Fn)
 // Description : This is the first function to be called in the SSDP library, it creates the multicats socket and starts
 //               the multicast listener thread.
 // Parameters  : FunPtr: Callback function which will receive all the responses.
 //
 // Return value: Less than zero if fails, UPNP_E_SUCCESS  otherwise.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
 int InitSsdpLib(SsdpFunPtr Fn)
 {
    int SsdpSock,On=1,val;
    u_char Ttl=4;
    struct ip_mreq SsdpMcastAddr;
    struct sockaddr_in SelfAddr;
	struct protoent * Protocol = getprotobyname("udp");

    StartupTime = time(NULL);

    InitParser();

    if ( ListenerState != Idle )
    {
    	return -1;		// already running
    }

	if (Protocol == NULL)
	{
		SendErrorEvent(UPNP_E_NETWORK_ERROR);
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n");)
		return UPNP_E_OUTOF_SOCKET;
	}

    __ssdp_sock = SsdpSock = socket(AF_INET, SOCK_DGRAM, Protocol->p_proto);
#ifdef NON_BLOCKING
    if ((val = fcntl(SsdpSock,F_GETFL,0)) == -1)
		diag_printf("Error %d: %s, " __FILE__ " %d\n", errno, strerror(errno), __LINE__);
	if (fcntl(SsdpSock,F_SETFL,val|O_NONBLOCK) == -1)
		diag_printf("Error %d: %s, " __FILE__ " %d\n", errno, strerror(errno), __LINE__);

    if ( SsdpSock == -1 || val == -1)
#else	/* NON_BLOCKING */
    if ( SsdpSock == -1 )
#endif	/* NON_BLOCKING */
    {
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n");)
       return UPNP_E_OUTOF_SOCKET;
    }

    if ( setsockopt(SsdpSock, SOL_SOCKET, SO_REUSEADDR, &On, sizeof(On)) == -1) {
        close( SsdpSock );
        return UPNP_E_OUTOF_SOCKET;
    }

//    bzero(&SsdpMcastAddr, sizeof(struct ip_mreq ));
    memset(&SsdpMcastAddr, 0, sizeof(struct ip_mreq ));
    SsdpMcastAddr.imr_interface.s_addr = htonl(INADDR_ANY);
    SsdpMcastAddr.imr_multiaddr.s_addr = inet_addr(SSDP_IP);


//    bzero((char *)&SelfAddr, sizeof(struct sockaddr_in));
    memset((char *)&SelfAddr, 0, sizeof(struct sockaddr_in));

    SelfAddr.sin_family = AF_INET;
    SelfAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    SelfAddr.sin_port = htons(SSDP_PORT);
    if (bind( SsdpSock, (struct sockaddr *) &SelfAddr, sizeof(SelfAddr)) != 0)
    {
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in binding !!!\n");)
       close(SsdpSock);
       return UPNP_E_SOCKET_BIND;
    }
    val = setsockopt(SsdpSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &SsdpMcastAddr, sizeof(struct ip_mreq));
    val = setsockopt(SsdpSock, IPPROTO_IP, IP_MULTICAST_TTL, &Ttl, sizeof(Ttl));

//#ifdef USE_MULTICAST
    tpool_Schedule((ScheduleFunc)ListenMulticastChannel, (void*)SsdpSock);
//#endif
    CallBackFn = Fn;

    // wait SSDP to Start
#ifdef USE_MULTICAST
    while(ListenerState != Running )
    {
       sleep(1);
    }
#endif
    return UPNP_E_SUCCESS ;

 }
#else

BOOL GetMulticastProvider(int Family,int SocketType,WSAPROTOCOL_INFO *ProtocolInfo) 

{   int nRet,i; 
    WSAPROTOCOL_INFO *ProtocolBuf = NULL; 
    DWORD BufLen = 0; 
    BOOL ProtocolFound = FALSE;
   
    nRet = WSAEnumProtocols(NULL, ProtocolBuf, &BufLen); 
    if (SOCKET_ERROR != nRet)   
    { 
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"WSAEnumProtocols: should not have suceeded\n");)
    } 
    else     
    { 
        ProtocolBuf = (WSAPROTOCOL_INFO *)malloc(BufLen); 
        if (ProtocolBuf)        
        { 
            nRet = WSAEnumProtocols(NULL, ProtocolBuf, &BufLen); 
            if (SOCKET_ERROR == nRet)        
            { 
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"WSAEnumProtocols: %d\n", WSAGetLastError());)
            } 
            else  
            { 
                // loop thru protocols, looking for a matching service provider  
                for (i=0; i<nRet; i++)                    
                { 
                    if (Family == ProtocolBuf[i].iAddressFamily && SocketType ==ProtocolBuf[i].iSocketType)  
                    {  // look for  
                       if ((XP1_SUPPORT_MULTIPOINT == (XP1_SUPPORT_MULTIPOINT &                                              
                            ProtocolBuf[i].dwServiceFlags1)))  
                       { 
                           memcpy(ProtocolInfo,ProtocolBuf+i,sizeof(WSAPROTOCOL_INFO)); 
                           ProtocolFound = TRUE; 
                           break; 
                       } 
                    }  
                 } // for      
               } // else 
            free(ProtocolBuf);  
           } 
        }  
     return(ProtocolFound);  
}  


int InitSsdpLib(SsdpFunPtr Fn)
 {
    SOCKET SsdpSock;
    int Scope,Error;
    u_char Ttl=4;
    struct sockaddr_in McastAddr,SelfAddr;
	   BOOL Option; 
    DWORD Result,BytesReturned;
    WORD wVersionRequested;
    WSADATA wsaData;
    WSAPROTOCOL_INFO ProtocolBuf;   
    int k;

    StartupTime = time(NULL);

    InitParser();
    CallBackFn = Fn;

    wVersionRequested = MAKEWORD( 2, 2 ); 
    Error = WSAStartup( wVersionRequested, &wsaData );
    if ( Error != 0 ) return UPNP_E_NETWORK_ERROR;

    GetMulticastProvider(AF_INET,SOCK_DGRAM,&ProtocolBuf);

    SsdpSock=WSASocket(FROM_PROTOCOL_INFO,FROM_PROTOCOL_INFO,FROM_PROTOCOL_INFO,&ProtocolBuf,
                      0,WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF);

    if ( SsdpSock == INVALID_SOCKET )
    {
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Error in creating socket !!!\n");)
       return UPNP_E_OUTOF_SOCKET;
    }


    Option = TRUE; 
    if (setsockopt(SsdpSock, SOL_SOCKET, SO_REUSEADDR,(const char* )&Option, sizeof(BOOL)) == -1)
    return UPNP_E_NETWORK_ERROR;

//    bzero((char *)&SelfAddr, sizeof(struct sockaddr_in));
    memset((char *)&SelfAddr, 0, sizeof(struct sockaddr_in));

    SelfAddr.sin_family = AF_INET;
    SelfAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    SelfAddr.sin_port = htons(SSDP_PORT);
    if (bind( SsdpSock, (struct sockaddr *) &SelfAddr, sizeof(SOCKADDR)) != 0)
    {
       k = WSAGetLastError();
       SendErrorEvent(UPNP_E_NETWORK_ERROR);
       DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Error in binding !!!\n");)
       return UPNP_E_SOCKET_BIND;
    }

    DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Socket ID = %d\n",SsdpSock);)
//    bzero((char *)&SelfAddr, sizeof(struct sockaddr_in));
    memset((char *)&SelfAddr, 0, sizeof(struct sockaddr_in));
    
    McastAddr.sin_family = AF_INET;
    McastAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    McastAddr.sin_port = htons(SSDP_PORT);


    if((SsdpSock=WSAJoinLeaf(SsdpSock,(struct sockaddr*)&McastAddr, 
                   sizeof(SOCKADDR),NULL,NULL,NULL,NULL,JL_BOTH)) == INVALID_SOCKET) 
    {
       DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Error in joining ssdp multicast group !!!\n");)
       return UPNP_E_NETWORK_ERROR;
    }

    k = WSAGetLastError();
    Option = TRUE; 
    if (setsockopt(SsdpSock, SOL_SOCKET, SO_BROADCAST,(const char* )&Option, sizeof(BOOL)) == -1)
    return UPNP_E_NETWORK_ERROR;

    /*Loopback=TRUE;
    Result = WSAIoctl(SsdpSock,SIO_MULTIPOINT_LOOPBACK,(PVOID)&Loopback, 
                        sizeof(BOOL),NULL,0, &BytesReturned,NULL, NULL);  

    if(Result == SOCKET_ERROR)
    {  
        DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Error in adding the interface to multicast\n"))
       // return UPNP_E_NETWORK_ERROR;

    }

    k = WSAGetLastError();*/

    Scope  = 4;    
    Result = WSAIoctl(SsdpSock,SIO_MULTICAST_SCOPE,(PVOID)&Scope, 
                        sizeof(INT),NULL,0, &BytesReturned,NULL, NULL);  
    if(Result == SOCKET_ERROR)
    {  
        DBGONLY(UpnpPrintf(UPNP_INFO,SSDP,__FILE__,__LINE__,"Error in in increasing the multicast scope\n");)
        return UPNP_E_NETWORK_ERROR;
    }

    tpool_Schedule((ScheduleFunc)ListenMulticastChannel, (void*)SsdpSock);

    return UPNP_E_SUCCESS ;

 }
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void RequestHandler(ThreadData *ThData)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response. Once it is received, it is passed back to callback function.
 // Parameters  : ThData : Data Packet which contains all the parameter which is requied to send the request on the
 //               multicatst channel.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
void RequestHandler(ThreadData *ThData)
{
	int socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0, RqstSock, ByteReceived,TimeTillRead;
	struct timeval tmout;
	fd_set WrSet,RdSet;
	struct sockaddr_in DestAddr;
	char RequestBuf[BUFSIZE];
	u_char Ttl=4;
	time_t StartTime,CurrentTime,ElapsedTime;
	Event * Evt = (Event *)malloc(sizeof(Event));
	if(Evt  == NULL)
	{
		SendErrorEvent( UPNP_E_OUTOF_MEMORY);
		return;
	}

	RqstSock = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(RqstSock, IPPROTO_IP, IP_MULTICAST_TTL, &Ttl, sizeof(Ttl));
#ifdef NON_BLOCKING
	RetVal = fcntl(RqstSock,F_GETFL,0);
	fcntl(RqstSock,F_SETFL,RetVal|O_NONBLOCK);
	if ( RqstSock == -1 || RetVal == -1)
#else	/* NON_BLOCKING */
	if ( RqstSock == -1 )
#endif	/* NON_BLOCKING */
	{
        if( RqstSock >= 0 ) close(RqstSock);
		SendErrorEvent(UPNP_E_NETWORK_ERROR);
		DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in creating socket !!!\n");)
		return;
	}

	TimeTillRead = ThData->Mx;

	if (TimeTillRead <= 1)
		TimeTillRead = 2;
	else if (TimeTillRead > MAX_TIME_TOREAD)
		TimeTillRead = MAX_TIME_TOREAD ;

	Evt->ErrCode = NO_ERROR_FOUND;
//	bzero((char *)&DestAddr, sizeof(struct sockaddr_in));
	memset((char *)&DestAddr, 0, sizeof(struct sockaddr_in));
	if(ThData->DestAddr.sin_port == 0)
	{
		DestAddr.sin_family = AF_INET;
		DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
		DestAddr.sin_port = htons(SSDP_PORT);
	}
	else
	{
		DestAddr.sin_family = ThData->DestAddr.sin_family;
		DestAddr.sin_addr.s_addr = ThData->DestAddr.sin_addr.s_addr;
		DestAddr.sin_port = ThData->DestAddr.sin_port;
	}

	while(TryIdx < NUM_TRY)
	{
		FD_ZERO(&WrSet);
		FD_SET(RqstSock,&WrSet);
		tmout.tv_sec = 1;
		tmout.tv_usec = 1;

		DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending packet : \n%s\n",ThData->Data);)
		int blah = sendto(RqstSock,ThData->Data,strlen(ThData->Data),0,(struct sockaddr *)&DestAddr, socklen);
		if ( select(RqstSock+1,NULL,&WrSet,NULL,NULL) == -1)
		{
            diag_printf(" error in select %d\n", errno );
			if ( errno == EBADF)
				{DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
			else if ( errno ==  EINTR)
				{DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
			else if ( errno ==  EINVAL )
				{DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
			else if ( errno ==   ENOMEM )
				{DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Select was unable to allocate memory for internal tables.\n");)}

			SendErrorEvent(UPNP_E_NETWORK_ERROR);
			RemoveThreadData(ThData);
			free(Evt);
			close(RqstSock);
			return;
		}
		else if(FD_ISSET(RqstSock,&WrSet))
		{
			RetVal= 1;
			break;
		}
		TryIdx++;
	}


	StartTime = cyg_current_time() / 100;
	CurrentTime = StartTime;
	ElapsedTime = 0;

///*
	while(ElapsedTime < TimeTillRead+2)
	{
//*/
		FD_ZERO(&RdSet);
		FD_SET(RqstSock,&RdSet);

		CurrentTime = cyg_current_time() / 100;
//diag_printf("ST: %d CT: %d ET: %d TTR+2: %d\n", StartTime, CurrentTime, ElapsedTime, TimeTillRead+2);
		ElapsedTime = CurrentTime-StartTime;

		tmout.tv_sec = TimeTillRead+2-ElapsedTime;
		tmout.tv_usec = TimeTillRead+2-ElapsedTime;

		int iRes = select(RqstSock+1,&RdSet,NULL,NULL,&tmout);
		if (iRes == -1)
		{
            diag_printf(" select came back with -1\n");
			DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select call!!!!!!!!");)
			Evt->ErrCode = E_SOCKET;
			RemoveThreadData(ThData);
            close(RqstSock);
			free(Evt);
			return;
		}
		else if(iRes && FD_ISSET(RqstSock,&RdSet))
		{
			socklen = sizeof(struct sockaddr_in);
			ByteReceived = recvfrom(RqstSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&DestAddr, (socklen_t*)&socklen);
			if(ByteReceived > 0 )
			{
				RequestBuf[ByteReceived] = '\0';
				DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received buffer%s\n",RequestBuf);)
				if( AnalyzeCommand(RequestBuf,Evt) >=0)
				{
					Evt->Cookie = ThData->Cookie;
					CallBackFn(Evt);
				}
			}
		} else {
            diag_printf(" select timed out\n");
        }
///*
	}
//*/

	Evt->Cookie = ThData->Cookie;
	Evt->Cmd=SSDP_TIMEOUT;
	CallBackFn(Evt);
	RemoveThreadData(ThData);
	free(Evt);
	close(RqstSock);
	return;
}

#else

void RequestHandler(ThreadData *ThData)
{
      int socklen=sizeof( struct sockaddr_in),TryIdx=0, RqstSock, ByteReceived,TimeTillRead,Scope;
      struct sockaddr_in DestAddr;
      char RequestBuf[BUFSIZE];
      u_char Ttl=4;
      DWORD Result,BytesReturned;
      time_t StartTime,CurrentTime,ElapsedTime;
      Event * Evt = (Event *)malloc(sizeof(Event));

      if(Evt  == NULL)
      {
         SendErrorEvent( UPNP_E_OUTOF_MEMORY);
         return;
      }

      RqstSock = socket(AF_INET, SOCK_DGRAM, 0);
      if ( RqstSock == -1)
      {
            SendErrorEvent(UPNP_E_NETWORK_ERROR);
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket creation !!!\n");)
            return;  
      } 
 

      TimeTillRead = ThData->Mx;

      if (TimeTillRead <= 1) TimeTillRead = 2;
      else if (TimeTillRead > MAX_TIME_TOREAD) TimeTillRead = MAX_TIME_TOREAD ;

       Evt->ErrCode = NO_ERROR_FOUND;
//       bzero((char *)&DestAddr, sizeof(struct sockaddr_in));
       memset((char *)&DestAddr, 0, sizeof(struct sockaddr_in));
       if(ThData->DestAddr.sin_port == 0)
       {
            DestAddr.sin_family = AF_INET;
            DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
            DestAddr.sin_port = htons(SSDP_PORT);
       }
       else
       {
           DestAddr.sin_family = ThData->DestAddr.sin_family;
           DestAddr.sin_addr.s_addr = ThData->DestAddr.sin_addr.s_addr;
           DestAddr.sin_port = ThData->DestAddr.sin_port;
        }

        DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending packet : \n%s\n",ThData->Data);)
        sendto(RqstSock,ThData->Data,strlen(ThData->Data),0,(struct sockaddr *)&DestAddr, socklen);

        StartTime = time(NULL);
        CurrentTime = StartTime;
        ElapsedTime = 0;

        while(ElapsedTime < TimeTillRead+2)
        {

             CurrentTime = time(NULL);
             ElapsedTime = CurrentTime-StartTime;


             socklen = sizeof(struct sockaddr_in);
             ByteReceived = recvfrom(RqstSock, RequestBuf, BUFSIZE, 0,(struct sockaddr*)&DestAddr, &socklen);
             if(ByteReceived > 0 )
             {
                  RequestBuf[ByteReceived] = '\0';
                  DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Received packet%s\n",RequestBuf);)
                  if( AnalyzeCommand(RequestBuf,Evt) >=0)
                  {
                     Evt->Cookie = ThData->Cookie;
                     CallBackFn(Evt);
                  }
               }
         }

         Evt->Cookie = ThData->Cookie;
         Evt->Cmd=TIMEOUT;
         CallBackFn(Evt);
         RemoveThreadData(ThData);
         free(Evt);
         close(RqstSock);
         return;


 }
#endif


#ifdef INCLUDE_CLIENT_APIS
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : void CreateClientRequestPacket(char * RqstBuf,int Mx, char *SearchTarget)
 // Description : This function creates a HTTP search request packet depending on the input parameter.
 // Parameters  : RqstBuf : Output string in HTTP format.
 //               SearchTarget : Search Taget
 //               Mx : Number of seconds to wait to collect all the responses.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 void CreateClientRequestPacket(char * RqstBuf,int Mx, char *SearchTarget)
 {

   char TempBuf[COMMAND_LEN];
   int Port;


   strcpy( RqstBuf,"M-SEARCH * HTTP/1.1\r\n");


   Port = SSDP_PORT;
   strcpy(TempBuf,"HOST:");
   strcat(TempBuf,SSDP_IP);
   sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
   strcat(RqstBuf,TempBuf);

   strcat(RqstBuf,"MAN:\"ssdp:discover\"\r\n");


   if ( Mx > 0)
   {
      sprintf(TempBuf,"MX:%d\r\n",Mx);
      strcat(RqstBuf,TempBuf);
   }


  if (SearchTarget != NULL)
  {
     sprintf(TempBuf,"ST:%s\r\n", SearchTarget);
     strcat(RqstBuf,TempBuf);
  }
  strcat(RqstBuf,"\r\n");

 }




 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int SearchByTarget()
 // Description : Creates and send the search request.
 // Parameters  : Mx : Number of seconds to wait, to collect all the responses.
 //               St : Search target.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int SearchByTarget(int Mx, char * St, void * Cookie)
 {

    char * ReqBuf;
    ThreadData *ThData1, *ThData2;

    ReqBuf = (char *)malloc(BUFSIZE);
    if (ReqBuf == NULL) return UPNP_E_OUTOF_MEMORY;

    CreateClientRequestPacket(ReqBuf,Mx,St);
    DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending request buffer = \n%s\n",ReqBuf);)

    ThData1 = (ThreadData *)malloc(sizeof(struct TData));
    if (ThData1 == NULL) return UPNP_E_OUTOF_MEMORY;
    ThData2 = (ThreadData *)malloc(sizeof(struct TData));
    if (ThData2 == NULL) {
        free( ThData1 );
        return UPNP_E_OUTOF_MEMORY;
    }
    PutThreadData(ThData1,ReqBuf, NULL, Mx);
    PutThreadData(ThData2,ReqBuf, NULL, Mx);
    ThData1->Cookie = ThData2->Cookie = Cookie;
    
    // Since this is a UDP packet, and delivery is not a sure thing, send two copies
    tpool_Schedule((ScheduleFunc)RequestHandler,ThData1);
    tpool_Schedule((ScheduleFunc)RequestHandler,ThData2);

    free(ReqBuf);
    return 1;

 }

#endif

#ifdef INCLUDE_DEVICE_APIS
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceRequestHandler(struct sockaddr_in * DestAddr, char * szStr)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response, once it received, it is passed back to callback function.
 // Parameters  : szSt : Response string in HTTP format.
 //               DestAddr : Ip address, to send the reply.
 //
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
 int ServiceRequestHandler(struct sockaddr_in * DestAddr, char * szStr)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0;
      struct timeval tmout;
      fd_set WrSet;

      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n"));
         return -1;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef NON_BLOCKING
      RetVal = fcntl(ReplySock,F_GETFL,0);
      fcntl(ReplySock,F_SETFL,RetVal|O_NONBLOCK);

      if( ReplySock == -1 || RetVal == -1)
#else	/* NON_BLOCKING */
      if( ReplySock == -1 )
#endif	/* NON_BLOCKING */
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n"));
         free(Evt);
      }


      while(TryIdx < NUM_TRY)
      {

          FD_ZERO(&WrSet);
          FD_SET(ReplySock,&WrSet);

          tmout.tv_sec = 1;
          tmout.tv_usec = 1;

          sendto(ReplySock,szStr,strlen(szStr),0,(struct sockaddr *)DestAddr, socklen);
          if ( select(ReplySock+1,NULL,&WrSet,NULL,NULL) == -1)
          {
             DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select !!!!!!!!\n"));
             if ( errno == EBADF)
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
             else if ( errno ==  EINTR)
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
             else if ( errno ==  EINVAL )
	            {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
				         else if ( errno ==   ENOMEM )
				         {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"select was unable to allocate memory for internal tables.\n");)}

             SendErrorEvent(UPNP_E_NETWORK_ERROR);
             free(Evt);
             break;
          }
          else if(FD_ISSET(ReplySock,&WrSet))
          {
              RetVal= 1;
              break;
          }
          TryIdx++;
      }

      free(Evt);
      close(ReplySock);
     return 1;
 }
#else
 int ServiceRequestHandler(struct sockaddr_in * DestAddr, char * szStr)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in);
      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n");)
         return -1;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

      if( ReplySock == -1)
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n");)
         free(Evt);
      }

      sendto(ReplySock,szStr,strlen(szStr),0,(struct sockaddr *)DestAddr, socklen);
      free(Evt);

     return 1;
 }
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int NewRequestHandler(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket)
 // Description : This function works as a request handler which passes the HTTP request string to multicast channel then
 //               wait for the response, once it received, it is passed back to callback function.
 // Parameters  : RqPacket : Request packet in HTTP format.
 //               DestAddr : Ip address, to send the reply.
 //               NumPacket: Number of packet to be sent.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
 int NewRequestHandler(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),RetVal,TryIdx=0;
      struct timeval tmout;
      fd_set WrSet;
      int NumCopy,Index;

      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n"));
         return UPNP_E_OUTOF_MEMORY;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef NON_BLOCKING
      RetVal = fcntl(ReplySock,F_GETFL,0);
      fcntl(ReplySock,F_SETFL,RetVal|O_NONBLOCK);

      if( ReplySock == -1 || RetVal == -1)
#else	/* NON_BLOCKING */
      if( ReplySock == -1 )
#endif	/* NON_BLOCKING */
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n"));
         free(Evt);
         return UPNP_E_OUTOF_SOCKET;

      }

      for(Index=0;Index< NumPacket;Index++)
      {

          NumCopy =0;
          TryIdx =0;

          while(TryIdx < NUM_TRY && NumCopy < NUM_SSDP_COPY)
          {

             FD_ZERO(&WrSet);
             FD_SET(ReplySock,&WrSet);

             tmout.tv_sec = 1;
             tmout.tv_usec = 1;

             DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending reply %s\n",*(RqPacket+Index));)
             sendto(ReplySock,*(RqPacket+Index),strlen(*(RqPacket+Index)),0,(struct sockaddr *)DestAddr, socklen);

             if ( select(ReplySock+1,NULL,&WrSet,NULL,NULL) == -1)
             {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in select !!!!!!!!\n")) ;
                if ( errno == EBADF)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"An invalid file descriptor was givenin one of the sets. \n");)}
                else if ( errno ==  EINTR)
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"A non blocked signal was caught.    \n");)}
                else if ( errno ==  EINVAL )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"n is negative.  \n");)}
                else if ( errno ==   ENOMEM )
		              {DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"select was unable to allocate memory for internal tables.\n");)}
                SendErrorEvent(UPNP_E_NETWORK_ERROR);
                break;
             }
             else if(FD_ISSET(ReplySock,&WrSet))
             {
                 ++NumCopy;
             }
             else TryIdx++;
         }
       }

       free(Evt);
       close(ReplySock);
       return UPNP_E_SUCCESS;
 }
#else
 int NewRequestHandler(struct sockaddr_in * DestAddr, int NumPacket, char ** RqPacket)
 {
      int ReplySock, socklen=sizeof( struct sockaddr_in),TryIdx=0;
      int NumCopy,Index;

      Event * Evt = (Event *) malloc(sizeof(Event));
      if ( Evt == NULL)
      {
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in memory allocation : \n");)
         return UPNP_E_OUTOF_MEMORY;
      }
      else Evt->ErrCode =  NO_ERROR_FOUND;


      ReplySock = socket(AF_INET, SOCK_DGRAM, 0);
      if( ReplySock == -1)
      {
         SendErrorEvent(UPNP_E_NETWORK_ERROR);
         DBGONLY(UpnpPrintf(UPNP_CRITICAL,SSDP,__FILE__,__LINE__,"Error in socket operation !!!\n");)
         free(Evt);
         return UPNP_E_OUTOF_SOCKET;

      }

      for(Index=0;Index< NumPacket;Index++)
      {

          NumCopy =0;

          while(NumCopy < NUM_SSDP_COPY)
          {
             DBGONLY(UpnpPrintf(UPNP_PACKET,SSDP,__FILE__,__LINE__,"Sending reply %s\n",*(RqPacket+Index));)
             sendto(ReplySock,*(RqPacket+Index),strlen(*(RqPacket+Index)),0,(struct sockaddr *)DestAddr, socklen);
             ++NumCopy;
          }
       }

       free(Evt);
       close(ReplySock);
       return UPNP_E_SUCCESS;
 }
#endif

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    :  void CreateServiceRequestPacket(int Notf,char *RqstBuf,char * NtSt,char *Usn,char *Server,char * S,char * Location,int  Duration)
 // Description : This function creates a HTTP request packet.  Depending on the input parameter it either creates a service advertisement
 //               request or service shutdown request etc.
 // Parameters  : Notf : Specify the type of notification, either advertisement or shutdown etc.
 //               RqstBuf  : Output buffer filled with HTTP statement.
 //               RqstId   : Same ID as send with the request HTTP statement.
 //               ServType : Service Type or category
 //               ServId   :Service unique name or ID
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


 void CreateServiceRequestPacket(int Notf,char *RqstBuf,char * NtSt,char *Usn,char *Server,char * Location,int  Duration)
 {
   char TempBuf[COMMAND_LEN],Temp[COMMAND_LEN];
   int  Port = SSDP_PORT;
   char Date[40];

   //Note  Notf=0 means service shutdown, Notf=1 means service advertisement, Notf =2 means reply
   currentTmToHttpDate(Date);

   if(Notf ==2)
   {
     strcpy(RqstBuf,"HTTP/1.1 200 OK \r\n");
     sprintf(TempBuf,"CACHE-CONTROL: max-age=%d\r\n",Duration);
     strcat(RqstBuf,TempBuf);
     strcat(RqstBuf,Date);
     strcat(RqstBuf,"EXT:\r\n");
     sprintf(TempBuf,"LOCATION: %s\r\n",Location);
     strcat(RqstBuf,TempBuf);
/*
     strcpy(Temp,SERVER);
     sprintf(TempBuf,"SERVER:%s\r\n",Temp);
*/
     strcat(RqstBuf,SERVER_IO);
     sprintf(TempBuf,"ST: %s\r\n",NtSt);
     strcat(RqstBuf,TempBuf);


   }
   else if(Notf ==1)
   {
      strcpy(RqstBuf,"NOTIFY * HTTP/1.1 \r\n");
      strcpy(TempBuf,"HOST: ");
      strcat(TempBuf,SSDP_IP);
      sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"CACHE-CONTROL: max-age=%d\r\n",Duration);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"LOCATION: %s\r\n",Location);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"NT: %s\r\n",NtSt);
      strcat(RqstBuf,TempBuf);
      strcat(RqstBuf,"NTS: ssdp:alive\r\n");
      strcpy(Temp,SERVER);
      sprintf(TempBuf,"SERVER:%s\r\n",Temp);
      strcat(RqstBuf,TempBuf);
   }
   else if(Notf ==0)
   {
      strcpy(RqstBuf,"NOTIFY * HTTP/1.1 \r\n");
      strcpy(TempBuf,"HOST: ");
      strcat(TempBuf,SSDP_IP);
      sprintf(TempBuf,"%s:%d\r\n",TempBuf,Port);
      strcat(RqstBuf,TempBuf);

      // Following two header is added to interop with Windows Millenium but this is not
      // a part of UPNP spec 1.0 
      sprintf(TempBuf,"CACHE-CONTROL: max-age=%d\r\n",Duration);
      strcat(RqstBuf,TempBuf);
      sprintf(TempBuf,"LOCATION: %s\r\n",Location);
      strcat(RqstBuf,TempBuf);


      sprintf(TempBuf,"NT: %s\r\n",NtSt);
      strcat(RqstBuf,TempBuf);
      strcat(RqstBuf,"NTS: ssdp:byebye\r\n");  

   }

   sprintf(TempBuf,"USN: %s\r\n",Usn);
   strcat(RqstBuf,TempBuf);
   strcat(RqstBuf,"\r\n\r\n");

 }



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceAdvertisement(int RootDev,char * Udn, char * DevType,char *Server,char * Location,int  Duration)
 // Description : This function creates the device advertisement request based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udm : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int DeviceAdvertisement(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration)
{

    struct sockaddr_in DestAddr;
    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"In function SendDeviceAdvertisemenrt\n");)

    DestAddr.sin_family = AF_INET;
    DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    DestAddr.sin_port = htons(SSDP_PORT);

    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);

       if (szReq[0] == NULL || szReq[1] == NULL || szReq[2] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(1,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(1,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,3, szReq) ;

       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(1,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,2, szReq);
       free(szReq[0]);
       free(szReq[1]);


     }


    return RetVal;


}

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceReply(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates the reply packet based on the input parameter, and send it to the client addesss
 //               given in its input parameter DestAddr.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udn : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : destination IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int SendReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration, int ByType)
 {

    char *szReq[1], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(DestAddr,1, szReq) ;

       free(szReq[0]);

    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

        szReq[0] = (char*)malloc(BUFSIZE);
        if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY;


        if(ByType == 0)
        {
           sprintf(Mil_Nt,"%s",Udn);
           sprintf(Mil_Usn,"%s",Udn);
           CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
           RetVal = NewRequestHandler(DestAddr,1, szReq);
        }
        else
        {
           sprintf(Mil_Nt,"%s",DevType);
           sprintf(Mil_Usn,"%s::%s",Udn,DevType);
           CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
           RetVal = NewRequestHandler(DestAddr,1, szReq);

        }

        free(szReq[0]);
    }

    return RetVal;

 }



 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceReply(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates the reply packet based on the input parameter, and send it to the client addesss
 //               given in its input parameter DestAddr.
 // Parameters  : RootDev : 1 means root device 0 means embedded device.
 //               Udn : Device UDN
 //               DevType : Device Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : destination IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  int DeviceReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration)
 {

    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;


    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL|| szReq[2] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(2,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(2,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);


       RetVal = NewRequestHandler(DestAddr,3, szReq) ;

       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(2,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       RetVal = NewRequestHandler(DestAddr,2, szReq);
       free(szReq[0]);
       free(szReq[1]);


     }

    return RetVal;
 }


 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceAdvertisement( char * Udn, char * ServType,char *Server,char * Location,int  Duration)
 // Description : This function creates the advertisement packet based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : Server : Os.
 //               Udn : Device UDN
 //               ServType : Service Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   int ServiceAdvertisement( char * Udn, char * ServType,char *Server,char * Location,int  Duration)
  {
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       struct sockaddr_in DestAddr;
       int RetVal;


       fflush(stdout);


       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       DestAddr.sin_family = AF_INET;
       DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
       DestAddr.sin_port = htons(SSDP_PORT);


       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(1,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandler(&DestAddr,1, szReq);


       free(szReq[0]);
       return RetVal;
  }

 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceReply( char * Udn, char * ServType,char *Server,char * Location,int  Duration,struct sockaddr_in *DestAddr)
 // Description : This function creates the advertisement packet based on the input parameter, and send it to the
 //               multicast channel.
 // Parameters  : Server : Os
 //               Udn : Device UDN
 //               ServType : Service Type.
 //               Location : Loaction of Device description document.
 //               Duration : Life time of this device.
 //               DestAddr : Client IP address.
 // Return value: 1 if successfull.
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

   int ServiceReply(struct sockaddr_in *DestAddr,  char * ServType, char * Udn, char *Server,char * Location,int  Duration)
  {
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       int RetVal;

       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(2,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandler(DestAddr,1, szReq);

       free(szReq[0]);
       return RetVal;

  }



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int ServiceShutdown(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates a HTTP service shutdown request packet and sent it to the multicast channel through
 //               RequestHandler.
 // Parameters  : RootDev  : 1 means root device.
 //               DevType  : Device Type or category
 //               Udn      : Device UDN
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 //
 // Return value: None
 ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////
  int ServiceShutdown( char * Udn, char * ServType,char *Server,char * Location,int  Duration)
{
        
       char Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
       char * szReq[1];
       struct sockaddr_in DestAddr;
       int RetVal;


       szReq[0] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL) return UPNP_E_OUTOF_MEMORY ;

       DestAddr.sin_family = AF_INET;
       DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
       DestAddr.sin_port = htons(SSDP_PORT);


       sprintf(Mil_Nt,"%s",ServType);
       sprintf(Mil_Usn,"%s::%s",Udn,ServType);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);
       RetVal = NewRequestHandler(&DestAddr,1, szReq);


       free(szReq[0]);
       return RetVal;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // Function    : int DeviceShutdown(int RootDev, char *DevType,char * Udn,struct sockaddr_in * DestAddr,char *Server,char * Location,int  Duration)
 // Description : This function creates a HTTP service shutdown request packet and sent it to the multicast channel through
 //               RequestHandler.
 // Parameters  : RootDev  : 1 means root device.
 //               DevType  : Device Type or category
 //               Udn      : Device UDN
 //               Location : Location URL.
 //               Duration : Service duration in sec.
 //
 // Return value: None
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 int DeviceShutdown(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration)
 {

    struct sockaddr_in DestAddr;
    char *szReq[3], Mil_Nt[LINE_SIZE], Mil_Usn[LINE_SIZE];
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,SSDP,__FILE__,__LINE__,"In function DeviceShutdown\n");)


    DestAddr.sin_family = AF_INET;
    DestAddr.sin_addr.s_addr = inet_addr(SSDP_IP);
    DestAddr.sin_port = htons(SSDP_PORT);

    if(RootDev) //If deviceis a root device , here we need to send 3 advertisement or reply
    {
       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       szReq[2] = (char*)malloc(BUFSIZE);

       if (szReq[0] == NULL || szReq[1] == NULL || szReq[2] == NULL) return UPNP_E_OUTOF_MEMORY ;

       strcpy(Mil_Nt,"upnp:rootdevice");
       sprintf(Mil_Usn,"%s::upnp:rootdevice",Udn);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(0,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(0,szReq[2],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,3, szReq) ;


       free(szReq[0]);
       free(szReq[1]);
       free(szReq[2]);


    }
    else //If device is not a root device then it is a sub-device., here we need to send 2 advertisement or reply
    {

       szReq[0] = (char*)malloc(BUFSIZE);
       szReq[1] = (char*)malloc(BUFSIZE);
       if (szReq[0] == NULL || szReq[1] == NULL ) return UPNP_E_OUTOF_MEMORY ;

       sprintf(Mil_Nt,"%s",Udn);
       sprintf(Mil_Usn,"%s",Udn);
       CreateServiceRequestPacket(0,szReq[0],Mil_Nt,Mil_Usn,Server,Location,Duration);


       sprintf(Mil_Nt,"%s",DevType);
       sprintf(Mil_Usn,"%s::%s",Udn,DevType);
       CreateServiceRequestPacket(0,szReq[1],Mil_Nt,Mil_Usn,Server,Location,Duration);

       RetVal = NewRequestHandler(&DestAddr,2, szReq);

       free(szReq[0]);
       free(szReq[1]);


     }
     return RetVal;

}

#endif
#endif
