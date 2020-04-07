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
// $Date: 2000/09/21 19:52:36 $
//  


#ifndef SSDPLIB_H
#define SSDPLIB_H 

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _WIN32
//#include <syslog.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <cyg/kernel/kapi.h>
#endif
//#include <sys/socket.h>
#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */
#include <network.h>	// - ecm
#ifdef __cplusplus
}
#endif	/* __cplusplus */
#include <netinet/in.h>
/*#include <netinet/in_systm.h>*/
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <arpa/inet.h>
#else
#include "../../win32/win32.h"
#endif

#include <util/upnp/api/interface.h>

#if 0
#include <sys/cdefs.h>
#include <sys/types.h>

typedef u_int16_t n_short;      /* short as received from the net */
typedef u_int32_t n_long;       /* long as received from the net  */
typedef u_int32_t n_time;       /* ms since 00:00 GMT, byte rev   */
#endif

//Constant
#define	 BUFSIZE   2500
#define  SSDP_IP   "239.255.255.250"
#define  SSDP_PORT 1900
#define  NUM_TRY 3
#define  NUM_COPY 2
#define  THREAD_LIMIT 50
#define  COMMAND_LEN  300

//Error code
#define NO_ERROR_FOUND    0
#define E_REQUEST_INVALID  	-3
#define E_RES_EXPIRED		-4
#define E_MEM_ALLOC		-5
#define E_HTTP_SYNTEX		-6
#define E_SOCKET 		-7
#define RQST_TIMEOUT    20

// For Parser
#define NUM_TOKEN  12
static time_t StartupTime;


 typedef struct TData
 {
    int Mx;
    void * Cookie;
    char * Data;
    struct sockaddr_in DestAddr;

 }ThreadData;


/* globals */
//extern int 	errno;

typedef int (*ParserFun)(char *, Event *);
//SsdpFunPtr CallBackFn;


void InitParser();
int AnalyzeCommand(char * szCommand, Event * Evt);

#endif
