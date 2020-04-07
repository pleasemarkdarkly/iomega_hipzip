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
// $Revision: 1.57 $
// $Date: 2000/10/05 21:41:10 $
//     


#ifndef INTERFACE_H
#define INTERFACE_H


#include <cyg/kernel/kapi.h>

#include <util/upnp/api/config.h>
#include <util/upnp/api/upnp.h>

#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/service_table.h>
#include <util/upnp/genlib/client_table.h>
#include <util/upnp/api/upnp_debug.h>


#define SERVER "LINUX" /* server field for ssdp advertisement calls */
#define SERVER_GENA "SERVER: LINUX\r\n" /*server field for gena */
#define SERVER_IO "SERVER: dadio/1.0 UPnP/1.0 DAR/1.0\r\n"	/* server field for all calls */

#define ADV_DURATION 2000 /* timeout duration for ssdp advertisements */

typedef enum {HND_INVALID=-1,HND_CLIENT,HND_DEVICE} Upnp_Handle_Type;
typedef enum SEARCH_TYPE{SERROR=-1,ALL,ROOTDEVICE,DEVICE,DEVICETYPE,SERVICE} SearchType;
typedef enum SsdpCmdType{SSDP_ERROR=-1,SSDP_OK,SSDP_ALIVE,SSDP_BYEBYE,
                         SSDP_SEARCH,SSDP_NOTIFY,SSDP_TIMEOUT} CmdT;
typedef enum SsdpSearchType{SSDP_SERROR=-1,SSDP_ALL,SSDP_ROOTDEVICE,
                           SSDP_DEVICE,SSDP_DEVICETYPE,SSDP_SERVICE} SType;


#ifndef  _WIN32
#ifdef USE_PTHREADS
extern pthread_mutex_t GlobalHndMutex; // = PTHREAD_MUTEX_INITIALIZER;
#define HandleLock()  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Trying Lock")); pthread_mutex_lock(&GlobalHndMutex); DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"LOCK"));
#define HandleUnlock() DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Trying Unlock")); pthread_mutex_unlock(&GlobalHndMutex); DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Unlock"));
#else
extern cyg_mutex_t GlobalHndMutex;
#define HandleLock()  DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Trying Lock")); cyg_mutex_lock(&GlobalHndMutex); DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"LOCK"));
#define HandleUnlock() DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Trying Unlock")); cyg_mutex_unlock(&GlobalHndMutex); DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Unlock"));
#endif
#endif

// Data to be stored in handle table for
struct Handle_Info
{
    Upnp_Handle_Type HType;
    Upnp_FunPtr  Callback; // Callback function pointer.
    char * Cookie;
    char  DescURL[LINE_SIZE];   // URL for the use of SSDP
    char  DescXML[LINE_SIZE];   // XML file path for device description

    int MaxAge;                     // Advertisement timeout
    Upnp_Document DescDocument; // Description parsed in terms of DOM document 
    Upnp_NodeList DeviceList; // List of devices in the description document
    Upnp_NodeList ServiceList;  // List of services in the description document
    DEVICEONLY(service_table ServiceTable;) //table holding subscriptions and 
                                //URL information
    CLIENTONLY(client_subscription * ClientSubList;) //client subscription list
    DEVICEONLY(int MaxSubscriptions;)
    DEVICEONLY(int MaxSubscriptionTimeOut;)

    char  DescAlias[LINE_SIZE]; // alias of desc doc served by web server
    int   aliasInstalled;       // 0 = not installed; otherwise installed
} ;


struct Upnp_Action_Int
{
    int ErrCode;                 // @field The result of the operation.
    char DevUDN[NAME_SIZE];     // @field The unique device ID being
    //   controlled.
    char ServiceType[NAME_SIZE]; // @field The service type of the service
    //   being controlled.
    char ServiceVer[NAME_SIZE];  // @field The version of the service.
    Upnp_Document ActionRequest; // @field The DOM document representing the
    //   action to be performed.
    Upnp_Document ActionResult;  // @field The DOM document representing the
    char * InStr;
    char * OutStr;
    char * Url;

};

typedef struct SsdpEventStruct
{
//	enum CmdType Cmd;
	enum SsdpCmdType Cmd;
//	enum SEARCH_TYPE RequestType;
	enum SsdpSearchType RequestType;
	int  ErrCode;
	int  MaxAge;
	int  Mx;
	char UDN[LINE_SIZE];
	char DeviceType[LINE_SIZE];
	char ServiceType[LINE_SIZE];  //NT or ST
	char Location[LINE_SIZE];
	char HostAddr[LINE_SIZE];
	char Os[LINE_SIZE];
	char Ext[LINE_SIZE];
	char Date[LINE_SIZE];
	struct sockaddr_in * DestAddr;
	void * Cookie;
} Event;

typedef void (* SsdpFunPtr)(Event *);
typedef Event SsdpEvent ;

//UpnpAPI Internal functions, used by other modules(SOAP or GENA etc to get
//handle information.

Upnp_Handle_Type GetHandleInfo(int Hnd, struct Handle_Info **HndInfo);
Upnp_FunPtr GetCallBackFn(int Hnd);
Upnp_Handle_Type GetClientHandleInfo(int *client_handle_out, 
                                     struct Handle_Info **HndInfo);
Upnp_Handle_Type GetDeviceHandleInfo(int *device_handle_out, 
                                     struct Handle_Info **HndInfo);
int PrintHandleInfo(UpnpClient_Handle Hnd);

// All SSDP API from ssdp library.
   
int SearchByTarget(int Mx, char * St, void *Cookie);
void DeInitSsdpLib();
int InitSsdpLib(SsdpFunPtr Fn);
int DeviceAdvertisement(char *DevType,int RootDev,char * Usn,char *Server,char * Location,int  Duration);
int DeviceShutdown(char *DevType,int RootDev,char * Usn,char *Server,char * Location,int  Duration);
int DeviceReply(struct sockaddr_in * DestAddr,char *DevType,int RootDev,char * Usn,char *Server,char * Location,int  Duration);
int SendReply(struct sockaddr_in * DestAddr, char *DevType,int RootDev, char * Udn, char *Server, char * Location, int  Duration, int ByType);
int ServiceAdvertisement( char *Udn,char *ServType,char *Server,char * Location,int  Duration);
int ServiceReply(struct sockaddr_in *DestAddr, char *ServType,char * Usn,char *Server,char * Location,int  Duration);
int ServiceShutdown( char *Udn,char *ServType,char *Server,char * Location,int  Duration);

// GENA 
extern char LOCAL_HOST[LINE_SIZE];
extern unsigned short LOCAL_PORT;

//SOAP module API to be called in Upnp-Dk API
int InitSoap();
int SoapSendAction(IN char * ActionURL,IN char *ServiceType,IN Upnp_Document  ActNode , OUT Upnp_Document  * RespNode) ;//From SOAP module
int SoapGetServiceVarStatus(IN char * ActionURL, IN Upnp_DOMString VarName, OUT Upnp_DOMString * StVar) ;   //From SOAP module

#endif



