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
// $Revision: 1.143 $
// $Date: 2000/10/06 01:14:18 $
//    

//File upnpapi.c


#include <util/upnp/api/config.h>


#include <assert.h>
#include <signal.h>

#include <util/upnp/api/upnpapi.h>


#include <util/upnp/genlib/interrupts.h>
#include <util/upnp/genlib/scheduler.h>

#include <stdlib.h>
#include <string.h>


#include <sys/utsname.h>
#include <cyg/posix/utsname.h>

//#include <sys/socket.h>
#include <network.h>    // - ecm
#include <netinet/in.h>
#include <arpa/inet.h>

#include <util/upnp/genlib/mystring.h>
#include <util/upnp/genlib/scheduler.h>

//************************************
//Needed for GENA

#include <util/upnp/protocols/gena.h>
#include <util/upnp/genlib/service_table.h>
#include <util/upnp/genlib/miniserver.h>

//*******************************************

/* ********************* */
#ifdef INTERNAL_WEB_SERVER

#include <util/upnp/genlib/server.h>
#include <util/upnp/genlib/urlconfig.h>

#endif /* INTERNAL_WEB_SERVER */
/* ****************** */

#ifndef _WIN32
#ifdef USE_PTHREADS
pthread_mutex_t GlobalHndMutex = PTHREAD_MUTEX_INITIALIZER;
#else
cyg_mutex_t GlobalHndMutex;
#endif
#include <util/upnp/genlib/timer_thread.h>
#else
#include "wtimer_thread.h"
extern HANDLE GlobalHndMutex;
#endif

int UpnpSdkInit = 0; // Global variable to denote the state of Upnp SDK
                     // = 0 if uninitialized, = 1 if initialized.

// Globals moved from interface.h.
char LOCAL_HOST[LINE_SIZE];
unsigned short LOCAL_PORT;

void * HandleTable[NUM_HANDLE]; // This will store all the handle
                                // related with client/service



static int UpnpSendByeBye(IN UpnpDevice_Handle Hnd);

EXPORT void UpnpHostChange(IN const char* HostIP, IN unsigned short DestPort ) 
{
#if 1
    DEVICEONLY(UpnpDevice_Handle  device_handle;)
    struct Handle_Info * temp;
    if (GetDeviceHandleInfo(&device_handle, &temp)==HND_DEVICE)
        UpnpSendByeBye(device_handle);
    HandleLock();
    if (HostIP!=NULL)
        strncpy(LOCAL_HOST, HostIP, LINE_SIZE);
    HandleUnlock();
#else
    UpnpFinish();
    UpnpInit( HostIP, DestPort );
#endif
}

static int __once = 0;

EXPORT int UpnpInit(IN const char *HostIP, IN unsigned short DestPort)
{
    int retVal=0;

#ifndef USE_PTHREADS
    cyg_mutex_init(&GlobalHndMutex);
#endif

/*
    if( InitLog() != UPNP_E_SUCCESS) 
    return UPNP_E_INIT_FAILED;
*/
    retVal=InitLog();
    if( retVal != UPNP_E_SUCCESS) 
        return UPNP_E_INIT_FAILED;



    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpInit \n");)


#ifdef _WIN32
    if(InitGlobalMutex() != UPNP_E_SUCCESS) 
        return UPNP_E_INIT_FAILED;
    InitThreadPool(3,10);
#endif

    HandleLock();
    if (HostIP!=NULL)
        strncpy(LOCAL_HOST, HostIP, LINE_SIZE);
    else
    {
#if 0
        if (getlocalhostname(LOCAL_HOST)!=UPNP_E_SUCCESS)
        {
            HandleUnlock();
            return UPNP_E_INIT_FAILED;
        }
#else
        return UPNP_E_INIT_FAILED;
#endif
    }
   
    if(UpnpSdkInit != 0)
    {
        HandleUnlock();
        return UPNP_E_INIT;
    }

    InitHandleList();
    HandleUnlock();

    if( !__once ) {
        tpool_SetMaxThreads(MAX_THREADS + 3); // 3 threads are required for running
        // miniserver, ssdp.
    }

#if !defined(_WIN32) && defined(USE_PTHREADS)
    if (tintr_Init(SIGUSR1) != 0)
       return UPNP_E_INIT_FAILED;
#endif

    UpnpSdkInit = 1; 

    if( !__once ) {
#if EXCLUDE_SOAP == 0
        InitSoap();
#endif
#if EXCLUDE_GENA == 0
        SetGenaCallback(genaCallback);
#endif

        if ((retVal= InitTimerThread(&GLOBAL_TIMER_THREAD))!=UPNP_E_SUCCESS)
        {
            UpnpSdkInit=0;
            UpnpFinish();
            return retVal;
        }

#if EXCLUDE_SSDP == 0
        if ((retVal = InitSsdpLib(SsdpCallbackEventHandler)) != UPNP_E_SUCCESS)
        {
            UpnpSdkInit = 0; 
            if (retVal != -1) 
                return retVal; 
            else // if ssdp is already running for unknown reasons!
                return UPNP_E_INIT_FAILED;
        }
#endif
    }
#if EXCLUDE_MINISERVER == 0
    if ((retVal = StartMiniServer(DestPort))<=0)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"Miniserver failed to start");)
        UpnpFinish();
        UpnpSdkInit = 0; 
        if (retVal != -1) 
            return retVal; 
        else // if miniserver is already running for unknown reasons!
            return UPNP_E_INIT_FAILED;

    }
#endif

    __once=1;
    DestPort=retVal;
    LOCAL_PORT=DestPort; 

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Host Ip: %s Host Port: %d\n",LOCAL_HOST,LOCAL_PORT));
    
    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpInit \n");)
      
    return UPNP_E_SUCCESS; 
} /***************** end of UpnpInit ******************/ 

EXPORT  int UpnpFinish()
{
    DEVICEONLY(UpnpDevice_Handle  device_handle;)
    CLIENTONLY(UpnpClient_Handle  client_handle;)
    struct Handle_Info * temp;

    DBGONLY(
    int retVal1 = 1; 
    int retVal2 = 1;
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpFinish : UpnpSdkInit is :%d:\n", UpnpSdkInit);
    if (UpnpSdkInit == 1)
    {
      UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpFinish : UpnpSdkInit is ONE\n");
    }
    if (UpnpSdkInit != 1)
    {
        return UPNP_E_FINISH;
    }
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Jobs pending = %d : Threads running = %d\n",
       tpool_GetNumJobsPending(), tpool_GetNumThreadsRunning());)

  
    UpnpSdkInit = 0; 

#ifdef INCLUDE_DEVICE_APIS
    if (GetDeviceHandleInfo(&device_handle, &temp)==HND_DEVICE)
      UpnpUnRegisterRootDevice(device_handle);
#endif

#ifdef INCLUDE_CLIENT_APIS
    if (GetClientHandleInfo(&client_handle,&temp)==HND_CLIENT)
      UpnpUnRegisterClient(client_handle);
#endif
    
    //    StopTimerThread(&GLOBAL_TIMER_THREAD);
    
#if EXCLUDE_SSDP == 0
    //    DeInitSsdpLib();
#endif
    //sleep(3);
    cyg_thread_delay(300);

#ifndef _WIN32
    StopMiniServer(); 
#ifdef USE_PTHREADS
    tintr_Done();
#else
    //    CloseThreadPool(1000);
#endif
#endif

#if 0
    DBGONLY(
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpFinish : UpnpSdkInit is :%d:\n", UpnpSdkInit);
    while (retVal1 || retVal2)
    {
      //sleep(3);
      cyg_thread_delay(300);
      retVal1 = tpool_GetNumJobsPending();
      retVal2 = tpool_GetNumThreadsRunning();
      UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Jobs pending = %d : Threads running = %d\n",retVal1, retVal2);
      break;
    }
    tpool_Cleanup();
)
CloseLog();
#endif
    
#ifndef USE_PTHREADS
    cyg_mutex_destroy(&GlobalHndMutex);
#endif

    return UPNP_E_SUCCESS;
}  /********************* End of  UpnpFinish  *************************/

#ifdef INCLUDE_DEVICE_APIS
EXPORT  int UpnpRegisterRootDevice (
    IN const char *DescUrl,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie,
    OUT UpnpDevice_Handle *Hnd
    )
{

    struct Handle_Info * HInfo; 
    int retVal=0;

    DBGONLY( UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpRegisterRootDevice \n");)

    if(UpnpSdkInit != 1)
    {
        HandleUnlock();
        return UPNP_E_FINISH;
    }
    if (Hnd == NULL || Fun == NULL || DescUrl == NULL || strlen(DescUrl) == 0) 
        return UPNP_E_INVALID_PARAM;

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY; 
    }

    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Root device URL is %s\n", DescUrl);)


    HInfo->HType = HND_DEVICE;
    strncpy(HInfo->DescURL, DescUrl, LINE_SIZE);
    HInfo->Callback = Fun;
    HInfo->Cookie = (char *) Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;
    HInfo->DescDocument = NULL;
    CLIENTONLY(HInfo->ClientSubList=NULL;)
    HInfo->MaxSubscriptions=UPNP_INFINITE;
    HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;
    if ((retVal=UpnpDownloadXmlDoc(HInfo->DescURL, &(HInfo->DescDocument))) 
        != UPNP_E_SUCCESS)
    {
        FreeHandle(*Hnd);
        HandleUnlock();
        return retVal;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: Valid Description\n");
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: DescURL : %s\n", HInfo->DescURL);)

    HInfo->DeviceList  = UpnpDocument_getElementsByTagName(HInfo->DescDocument
                                                            , "device");
    if (HInfo->DeviceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: No devices found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    HInfo->ServiceList = UpnpDocument_getElementsByTagName(HInfo->DescDocument
                                                            , "serviceList");  
    if (HInfo->ServiceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: No services found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: Gena Check\n");)

    //*******************************
    //GENA SET UP
    //*******************************
    if (getServiceTable(HInfo->DescDocument, &HInfo->ServiceTable,
             HInfo->DescURL))
    {

       DBGONLY(
       UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice: GENA Service Table \n");
       UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Here are the known services: \n");
       printServiceTable(&HInfo->ServiceTable,UPNP_INFO,API);)

    }
    else
    {
       DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"\nUpnpRegisterRootDevice: No Eventing Support Found \n");)
    }

    HandleUnlock();


    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting RegisterRootDevice Successfully\n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterRootDevice  *********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
static int UpnpSendByeBye(IN UpnpDevice_Handle Hnd)
{
    struct Handle_Info *HInfo;
    int retVal = 0;
#if EXCLUDE_SSDP == 0
    HandleLock();
    if (GetHandleInfo(Hnd, &HInfo) == HND_INVALID)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }     
    HandleUnlock();
    
    retVal = AdvertiseAndReply(-1, Hnd, SSDP_ALL, (struct sockaddr_in *) NULL, 
                               (char *) NULL, (char *) NULL, (char *) NULL, 
                               HInfo->MaxAge);
#endif
    return retVal;
}

EXPORT  int UpnpUnRegisterRootDevice(IN UpnpDevice_Handle Hnd)
{
    int retVal = 0;
    struct Handle_Info *HInfo;
    struct Handle_Info *info;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpUnRegisterRootDevice \n");)

#if EXCLUDE_GENA == 0
    if (genaUnregisterDevice(Hnd) != UPNP_E_SUCCESS)
        return UPNP_E_INVALID_HANDLE;
#endif

    HandleLock();
    if (GetHandleInfo(Hnd, &HInfo) == HND_INVALID)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }     
    HandleUnlock();

#if EXCLUDE_SSDP == 0
    retVal = AdvertiseAndReply(-1, Hnd, SSDP_ALL, (struct sockaddr_in *) NULL, 
                               (char *) NULL, (char *) NULL, (char *) NULL, 
                               HInfo->MaxAge);
#endif

    info = (struct Handle_Info *) HandleTable[Hnd];
    UpnpNodeList_free( info->DeviceList );
    UpnpNodeList_free( info->ServiceList );
    UpnpDocument_free( info->DescDocument );

#ifdef INTERNAL_WEB_SERVER
    if ( info->aliasInstalled )
    {
        http_RemoveAlias( info->DescAlias );
    }
#endif // INTERNAL_WEB_SERVER

    HandleLock();
    FreeHandle(Hnd);
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Exiting UpnpUnRegisterRootDevice \n");
)

    return  retVal;
}  /****************** End of UpnpUnRegisterRootDevice *********************/


#endif //INCLUDE_DEVICE_APIS


// *************************************************************
#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER

// determines alias for given name which is a file name or URL.
//
// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_EXT_NOT_XML
static int GetNameForAlias( IN char *name, OUT char** alias )
{
    char *ext;
    char *al;

    ext = strrchr( name, '.' );
    if ( ext == NULL || strcasecmp(ext, ".xml") != 0 )
    {
        return UPNP_E_EXT_NOT_XML;
    }

    al = strrchr( name, '/' );
    if ( al == NULL )
    {
        *alias = name;
    }
    else
    {
        *alias = al;
    }

    return UPNP_E_SUCCESS;
}

static void get_server_addr( OUT struct sockaddr_in* serverAddr )
{
    memset( serverAddr, 0, sizeof(struct sockaddr_in) );

    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons( LOCAL_PORT );
    //inet_aton( LOCAL_HOST, &serverAddr->sin_addr );
    serverAddr->sin_addr.s_addr = inet_addr(LOCAL_HOST);
}

// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_URL_TOO_BIG
//   UPNP_E_INVALID_PARAM
//   UPNP_E_FILE_NOT_FOUND
//   UPNP_E_FILE_READ_ERROR
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
//   UPNP_E_EXT_NOT_XML
//   UPNP_E_NETWORK_ERROR
//   UPNP_E_SOCKET_WRITE
//   UPNP_E_SOCKET_READ
//   UPNP_E_SOCKET_BIND
//   UPNP_E_SOCKET_CONNECT
//   UPNP_E_OUTOF_SOCKET
static int GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
    IN char* description,
    IN unsigned int bufferLen,
    IN int config_baseURL,
    OUT Upnp_Document* xmlDoc,
    OUT char* descURL,
    OUT char* descAlias )
{
    int retVal;
    char *membuf = NULL;
    char *descStr = NULL;
    char *actualAliasStr = NULL;
    int descIsMalloked = 0;

    if ( description == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    // non-URL description must have configuration specified
    if ( descriptionType != UPNPREG_URL_DESC && (!config_baseURL) )
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( descriptionType == UPNPREG_URL_DESC )
    {
        if ((retVal=UpnpDownloadXmlDoc(description, xmlDoc))!=UPNP_E_SUCCESS)
        {
            return retVal;
        }
    }
    else if ( descriptionType == UPNPREG_FILENAME_DESC )
    {
        FILE *fp;
        unsigned fileLen;
        unsigned num_read;

        if ( (fp = fopen(description, "rb")) == NULL )
        {
            return UPNP_E_FILE_NOT_FOUND;
        }
        fseek( fp, 0, SEEK_END );
        fileLen = ftell( fp );
        rewind( fp );

        if ( (membuf = (char *)malloc(fileLen + 1)) == NULL )
        {
            fclose( fp );
            return UPNP_E_OUTOF_MEMORY;
        }

        num_read = fread( membuf, 1, fileLen, fp );
        if ( num_read != fileLen )
        {
            fclose( fp );
            free( membuf );
            return UPNP_E_FILE_READ_ERROR;
        }

        membuf[fileLen] = 0;
        fclose( fp );
//      *xmlDoc = UpnpParse_Buffer( membuf );
        *xmlDoc = UpnpParse_BufferNoEx( membuf );
        free( membuf );
    }
    else if ( descriptionType == UPNPREG_BUF_DESC )
    {
//      *xmlDoc = UpnpParse_Buffer( description );
        *xmlDoc = UpnpParse_BufferNoEx( description );
    }
    else
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( *xmlDoc == NULL )
    {
        return UPNP_E_INVALID_DESC;
    }

    // determine alias
    if ( config_baseURL )
    {
        char *aliasStr;
        struct sockaddr_in serverAddr;

        if ( descriptionType == UPNPREG_BUF_DESC )
        {
            aliasStr = "description.xml";
        }
        else  // URL or filename
        {
            retVal = GetNameForAlias( description, &aliasStr );
            if ( retVal != UPNP_E_SUCCESS )
            {
                UpnpDocument_free( *xmlDoc );
                return retVal;
            }
        }

        get_server_addr( &serverAddr );

        // config
        retVal = Configure_Urlbase( *xmlDoc, &serverAddr,
            aliasStr, &actualAliasStr, &descStr );
        if ( retVal != UPNP_E_SUCCESS )
        {
            UpnpDocument_free( *xmlDoc );
            diag_printf("Configure_Urlbase FAILED\n");
            return retVal;
        }
        descIsMalloked = 1;
    }
    else // manual
    {
        descStr = description;
    }

    if ( strlen(descStr) > (LINE_SIZE - 1) )
    {
        UpnpDocument_free( *xmlDoc );
        if ( descIsMalloked )
        {
            free( descStr );
            free( actualAliasStr );
        }
        return UPNP_E_URL_TOO_BIG;
    }

    strcpy( descURL, descStr );

    if ( descIsMalloked )
    {
        strncpy( descAlias, actualAliasStr, LINE_SIZE );
        free( descStr );
        free( actualAliasStr );
    }

//    assert( *xmlDoc != NULL );

    return UPNP_E_SUCCESS;
}

#else   // no web server

// returns:
//   UPNP_E_SUCCESS
//   UPNP_E_NO_WEB_SERVER
//   UPNP_E_INVALID_PARAM
//   UPNP_E_URL_TOO_LONG
static int GetDescDocumentAndURL( IN Upnp_DescType descriptionType,
    IN char* description,
    IN unsigned int bufferLen,
    IN int config_baseURL,
    OUT Upnp_Document* xmlDoc,
    OUT char* descURL,
    OUT char* descAlias )
{
    int retVal;

    if ( (descriptionType != UPNPREG_URL_DESC) || config_baseURL )
    {
        return UPNP_E_NO_WEB_SERVER;
    }

    if ( description == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    if ( strlen(description) > (LINE_SIZE - 1) )
    {
        return UPNP_E_URL_TOO_BIG;
    }
    strcpy( descURL, description );

    if ((retVal=UpnpDownloadXmlDoc(description, xmlDoc))!=UPNP_E_SUCCESS)
    {
        return retVal;
    }

    return UPNP_E_SUCCESS;
}


#endif // INTERNAL_WEB_SERVER
// ********************************************************

// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_URL_TOO_BIG
//   UPNP_E_INVALID_PARAM
//   UPNP_E_FILE_NOT_FOUND
//   UPNP_E_FILE_READ_ERROR
//   UPNP_E_INVALID_URL
//   UPNP_E_INVALID_DESC
//   UPNP_E_EXT_NOT_XML
//   UPNP_E_NETWORK_ERROR
//   UPNP_E_SOCKET_WRITE
//   UPNP_E_SOCKET_READ
//   UPNP_E_SOCKET_BIND
//   UPNP_E_SOCKET_CONNECT
//   UPNP_E_OUTOF_SOCKET
//   UPNP_E_NO_WEB_SERVER
EXPORT int UpnpRegisterRootDevice2( IN Upnp_DescType descriptionType,
        IN const char* description_const,
        IN size_t bufferLen,    // ignored unless descType == UPNPREG_BUF_DESC
        IN int config_baseURL,
        IN Upnp_FunPtr Fun,
        IN const void* Cookie,
        OUT UpnpDevice_Handle* Hnd )
{
    struct Handle_Info * HInfo;
    int retVal=0;
    char *description = (char *)description_const;

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Inside UpnpRegisterRootDevice2 \n");)

    if(UpnpSdkInit != 1)
    {
        return UPNP_E_FINISH;
    }
    if ( Hnd == NULL || Fun == NULL )
    {
        return UPNP_E_INVALID_PARAM;
    }

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE)
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }

    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL)
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    // prevent accidental removal of a non-existent alias
    HInfo->aliasInstalled = 0;

    retVal = GetDescDocumentAndURL( descriptionType, description,
        bufferLen, config_baseURL, &HInfo->DescDocument,
        HInfo->DescURL, HInfo->DescAlias );

    if ( retVal != UPNP_E_SUCCESS )
    {
        FreeHandle(*Hnd);
        HandleUnlock();
        return retVal;
    }

    HInfo->aliasInstalled = (config_baseURL != 0);

    HInfo->HType = HND_DEVICE;
    HInfo->Callback = Fun;
    HInfo->Cookie = (char *)Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;
    CLIENTONLY(HInfo->ClientSubList=NULL;)
    HInfo->MaxSubscriptions=UPNP_INFINITE;
    HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;


    DBGONLY(
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: Valid Description\n");
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: DescURL : %s\n", HInfo->DescURL);)

    HInfo->DeviceList  = UpnpDocument_getElementsByTagName(HInfo->DescDocument, "device");

    if (HInfo->DeviceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: No devices found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    HInfo->ServiceList = UpnpDocument_getElementsByTagName(HInfo->DescDocument , "serviceList");

    if (HInfo->ServiceList == NULL)
    {
        FreeHandle(*Hnd);
        HandleUnlock();

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: No services found for RootDevice\n");)

        return UPNP_E_INVALID_DESC;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: Gena Check\n");)

    //*******************************
    //GENA SET UP
    //*******************************

    if (getServiceTable(HInfo->DescDocument, &HInfo->ServiceTable,HInfo->DescURL))
    {

       DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpRegisterRootDevice2: GENA Service Table \n");)

    }
    else
    {
       DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"\nUpnpRegisterRootDevice2: No Eventing Support Found \n");)
    }
    HandleUnlock();


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting RegisterRootDevice2 Successfully\n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterRootDevice2  *********************/


#endif //INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpRegisterClient(IN Upnp_FunPtr Fun,
                IN const void * Cookie,
                OUT UpnpClient_Handle *Hnd)
{
    struct Handle_Info * HInfo;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRegisterClient \n");)

    if(UpnpSdkInit != 1)
    {
        HandleUnlock();
        return UPNP_E_FINISH;
    }
    if (Fun == NULL || Hnd == NULL) 
        return UPNP_E_INVALID_PARAM;

    HandleLock();
    if ((*Hnd = GetFreeHandle()) == UPNP_E_OUTOF_HANDLE) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY; 
    }
    HInfo = (struct Handle_Info *) malloc (sizeof(struct Handle_Info));
    if (HInfo == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }

    HInfo->HType = HND_CLIENT;
    HInfo->Callback = Fun;
    HInfo->Cookie = (char *) Cookie;
    HInfo->MaxAge = 0;
    HInfo->ClientSubList=NULL;
    DEVICEONLY(HInfo->MaxSubscriptions=UPNP_INFINITE;)
    DEVICEONLY(HInfo->MaxSubscriptionTimeOut=UPNP_INFINITE;)
    
    HandleTable[*Hnd] = HInfo;

    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRegisterClient \n");
)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpRegisterClient   *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpUnRegisterClient(IN UpnpClient_Handle Hnd)
{
    struct Handle_Info *HInfo;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnRegisterClient \n");)
    #if EXCLUDE_GENA == 0
    if (genaUnregisterClient(Hnd) != UPNP_E_SUCCESS)
        return UPNP_E_INVALID_HANDLE;
    #endif
    HandleLock();
    if (GetHandleInfo(Hnd, &HInfo) == HND_INVALID)
    {
       HandleUnlock();
       return UPNP_E_INVALID_HANDLE;
    }     
    FreeHandle(Hnd);
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnRegisterClient \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpUnRegisterClient *********************/
#endif // INCLUDE_CLIENT_APIS

//-----------------------------------------------------------------------------
//
//                                   SSDP interface
//
//-----------------------------------------------------------------------------

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

EXPORT  int UpnpSendAdvertisement(IN UpnpDevice_Handle Hnd, IN int Exp)
{
    struct Handle_Info *  SInfo=NULL; 
    int retVal = 0, *ptrMx;
    upnp_timeout *adEvent;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendAdvertisement \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (Exp < 1)
       Exp = DEFAULT_MAXAGE;
    SInfo->MaxAge = Exp; 
    HandleUnlock(); 
    retVal = AdvertiseAndReply(1, Hnd, SSDP_ALL, (struct sockaddr_in *) NULL, 
                             (char *) NULL, (char *) NULL, (char *) NULL, 
                             Exp);
    
    if (retVal != UPNP_E_SUCCESS)
        return retVal;
    ptrMx = (int *) malloc (sizeof(int));
    if (ptrMx == NULL)
        return UPNP_E_OUTOF_MEMORY;
    adEvent = (upnp_timeout *) malloc (sizeof(upnp_timeout));
    
    if (adEvent == NULL)
    {   
        free(ptrMx); 
        return UPNP_E_OUTOF_MEMORY;
    }
    *ptrMx = Exp;
    adEvent->handle = Hnd;
    adEvent->Event = ptrMx;
    
    if ((retVal = ScheduleTimerEvent(Exp - AUTO_ADVERTISEMENT_TIME, 
                  AutoAdvertise, adEvent, &GLOBAL_TIMER_THREAD, 
                  &(adEvent->eventId))) !=UPNP_E_SUCCESS)
    {
        free(adEvent);
        free(ptrMx);
        return retVal;
    }   

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendAdvertisement \n");
)

    return retVal;
}  /****************** End of UpnpSendAdvertisement *********************/
#endif // INCLUDE_DEVICE_APIS
#endif
#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpSearchAsync(IN UpnpClient_Handle Hnd,
    IN int Mx,
    IN const char *Target_const,
    IN const void *Cookie_const
    )
{
    struct Handle_Info *  SInfo=NULL;
    char *Target = (char *)Target_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSearchAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if( Mx < 1) 
        Mx = DEFAULT_MX;

    if (Target == NULL )
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    SearchByTarget(Mx, Target, (void*) Cookie_const);
    
    HandleUnlock();

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSearchAsync \n");)

    return  UPNP_E_SUCCESS;

}  /****************** End of UpnpSearchAsync *********************/
#endif // INCLUDE_CLIENT_APIS
#endif
//-----------------------------------------------------------------------------
//
//                                   GENA interface 
//
//-----------------------------------------------------------------------------

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS
EXPORT  int UpnpSetMaxSubscriptions(  
    IN UpnpDevice_Handle Hnd, /** The handle of the device for which
                  the maximum subscriptions is being set. */
    IN int MaxSubscriptions   /** The maximum number of subscriptions to be
                  allowed per service. */
    )
{
  struct Handle_Info * SInfo=NULL;

  DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSetMaxSubscriptions \n");)

  HandleLock();
  if ( ( (MaxSubscriptions!=UPNP_INFINITE) && (MaxSubscriptions<0))
       || (GetHandleInfo(Hnd,&SInfo)!=HND_DEVICE))
    {
      HandleUnlock();
      return UPNP_E_INVALID_HANDLE;
    }
  SInfo->MaxSubscriptions=MaxSubscriptions;
  HandleUnlock();

  DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSetMaxSubscriptions \n");)

  return UPNP_E_SUCCESS;
}  /****************** End of UpnpSetMaxSubscriptions *********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
EXPORT  int UpnpSetMaxSubscriptionTimeOut(  
    IN UpnpDevice_Handle Hnd,       /** The handle of the device for which
                     the maximum subscription time-out is being set. */
    IN int MaxSubscriptionTimeOut   /** The maximum subscription time-out to be
                     accepted. */
    )
{
   struct Handle_Info * SInfo=NULL;

   DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSetMaxSubscriptionTimeOut \n");)

   HandleLock();

   if ( ( ( MaxSubscriptionTimeOut!=UPNP_INFINITE) && (MaxSubscriptionTimeOut<0)) 
            || (GetHandleInfo(Hnd,&SInfo)!=HND_DEVICE))
   {
      HandleUnlock();
      return UPNP_E_INVALID_HANDLE;
   }

   SInfo->MaxSubscriptionTimeOut=MaxSubscriptionTimeOut;
   HandleUnlock();

   DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSetMaxSubscriptionTimeOut \n");)

   return UPNP_E_SUCCESS;
  
}  /****************** End of UpnpSetMaxSubscriptionTimeOut ******************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpSubscribeAsync(IN UpnpClient_Handle Hnd,
    IN const char * EvtUrl_const,
    IN int TimeOut,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;
    char *EvtUrl = (char *)EvtUrl_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSubscribeAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (EvtUrl == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_URL;
    }
    if (TimeOut!=UPNP_INFINITE && TimeOut < 1)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
    {
        HandleUnlock();
        return UPNP_E_OUTOF_MEMORY;
    }
    HandleUnlock();

    Param->FunName = SUBSCRIBE;
    Param->Handle = Hnd;
    strncpy(Param->Url, EvtUrl, NAME_SIZE);
    Param->TimeOut = TimeOut;
    Param->Fun = Fun;
    Param->Cookie = (char*) Cookie_const;

    tpool_Schedule(  (void(*)(void*))UpnpThreadDistribution , Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSubscribeAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpSubscribeAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

EXPORT  int  UpnpSubscribe (IN UpnpClient_Handle Hnd,
    IN const char * EvtUrl_const,
    INOUT int *TimeOut,
    OUT Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;
    char *EvtUrl = (char *)EvtUrl_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSubscribe \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (EvtUrl == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_URL;
    }
    if (TimeOut == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();
    RetVal = genaSubscribe(Hnd, EvtUrl, TimeOut, SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSubscribe \n");)

    return RetVal;

}  /****************** End of UpnpSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int  UpnpUnSubscribe (IN UpnpClient_Handle Hnd, IN Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnSubscribe \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    HandleUnlock();
    RetVal = genaUnSubscribe(Hnd, SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnSubscribe \n");)

    return RetVal;

}  /****************** End of UpnpUnSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpUnSubscribeAsync(IN UpnpClient_Handle Hnd,
    IN Upnp_SID SubsId,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{

    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpUnSubscribeAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    HandleUnlock();
    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = UNSUBSCRIBE;
    Param->Handle = Hnd;
    strncpy(Param->SubsId, SubsId, 42);  
    Param->Fun = Fun;
    Param->Cookie = (char*) Cookie_const;

    tpool_Schedule( (void (*)(void*)) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpUnSubscribeAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpUnSubscribeAsync  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int  UpnpRenewSubscription (IN UpnpClient_Handle Hnd, INOUT int *TimeOut, 
                            IN Upnp_SID SubsId)
{
    struct Handle_Info * SInfo=NULL; 
    int RetVal;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRenewSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (TimeOut == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    HandleUnlock();
    RetVal = genaRenewSubscription(Hnd, SubsId, TimeOut);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRenewSubscription \n");)

    return RetVal;

}  /****************** End of UpnpRenewSubscription  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpRenewSubscriptionAsync(IN UpnpClient_Handle Hnd,
    INOUT int TimeOut,
    IN Upnp_SID SubsId,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const )
{

    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpRenewSubscriptionAsync \n");)

    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (TimeOut!=UPNP_INFINITE && TimeOut < 1)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (Fun == NULL) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = RENEW;
    Param->Handle = Hnd;
    strncpy(Param->SubsId, SubsId, 42);
    Param->Fun = Fun;
    Param->Cookie = (char*) Cookie_const;
    Param->TimeOut = TimeOut;

    tpool_Schedule( (void (*)(void*)) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpRenewSubscriptionAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpRenewSubscriptionAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_DEVICE_APIS
EXPORT  int UpnpNotify(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN const char ** VarName_const,
    IN const char ** NewVal_const,
    IN int cVariables)
{

    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;
    char **VarName = (char **)VarName_const;
    char **NewVal = (char **)NewVal_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpNotify \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (VarName == NULL || NewVal == NULL || cVariables < 0)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    
    HandleUnlock();
    retVal = genaNotifyAll (Hnd,DevID,ServName, VarName, NewVal, cVariables);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpNotify \n");)

    return retVal;

}  /****************** End of UpnpNotify *********************/


EXPORT  int UpnpNotifyExt(IN UpnpDevice_Handle Hnd,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN Upnp_Document PropSet)
{

    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpNotify \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    
    HandleUnlock();
    retVal = genaNotifyAllExt(Hnd,DevID,ServName,PropSet);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpNotify \n");)

    return retVal;

}  /****************** End of UpnpNotify *********************/

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
EXPORT  int UpnpAcceptSubscription(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN const char ** VarName_const,
    IN const char ** NewVal_const,
    int cVariables,
    IN Upnp_SID SubsId)
{
    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;
    char **VarName = (char **)VarName_const;
    char **NewVal = (char **)NewVal_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpAcceptSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }
    if (VarName == NULL || NewVal == NULL || cVariables < 0)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }
    
    HandleUnlock();
    retVal = genaInitNotify(Hnd,DevID,ServName, VarName, NewVal, cVariables, 
                            SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpAcceptSubscription \n");)

    return retVal;

}  /****************** End of UpnpAcceptSubscription *********************/


EXPORT  int UpnpAcceptSubscriptionExt(IN UpnpDevice_Handle Hnd ,
    IN const char *DevID_const,
    IN const char *ServName_const,
    IN Upnp_Document PropSet,
    IN Upnp_SID SubsId)
{
    struct Handle_Info *SInfo=NULL; 
    int retVal;
    char *DevID = (char *)DevID_const;
    char *ServName = (char *)ServName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpAcceptSubscription \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    if (DevID == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (ServName == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SERVICE;
    }
    if (SubsId == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_SID;
    }

    if (PropSet == NULL)
    {
        HandleUnlock();
        return UPNP_E_INVALID_PARAM;
    }

    
    HandleUnlock();
    retVal = genaInitNotifyExt(Hnd,DevID,ServName,PropSet,SubsId);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpAcceptSubscription \n");)

    return retVal;

}  /****************** End of UpnpAcceptSubscription *********************/

#endif // INCLUDE_DEVICE_APIS
#endif // EXCLUDE_GENA == 0


//-----------------------------------------------------------------------------
//
//                                   SOAP interface 
//
//-----------------------------------------------------------------------------
#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS
EXPORT  int UpnpSendAction( IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *ServiceType_const,
    IN const char *DevUDN_const,
    IN Upnp_Document Action,
    OUT Upnp_Document *RespNodePtr)
{
    struct Handle_Info * SInfo=NULL; 
    int retVal = 0;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendAction \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (ServiceType == NULL || Action == NULL || RespNodePtr == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }    

    retVal = SoapSendAction(ActionURL, ServiceType, Action, 
                            RespNodePtr);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendAction \n");)

    return retVal;
}  /****************** End of UpnpSendAction *********************/
 // INCLUDE_CLIENT_APIS

EXPORT  int UpnpSendActionAsync(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *ServiceType_const,
    IN const char * DevUDN_const,
    IN Upnp_Document Act,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const)
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam  * Param;
    Upnp_DOMString tmpStr;
    char *ActionURL = (char *)ActionURL_const;
    char *ServiceType = (char *)ServiceType_const;
    //char *DevUDN = (char *)DevUDN_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpSendActionAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (ServiceType == NULL || Act == NULL || Fun == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }
    // ecm -- skip printing the document to a string and reparsing it.
    // Just own the document.
    // IF THIS FUNCTION SUCCEEDS, THEN THE UPNP CODE WILL FREE THE DOCUMENT ITSELF.
    // IF THIS FUNCTION FAILS, THEN CALL UpnpDocument_free TO FREE THE ACTION NODE!
/*
    tmpStr = UpnpNewPrintDocument(Act);
    if (tmpStr == NULL)
    {
        return UPNP_E_INVALID_ACTION;
    }
*/
    
    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strncpy(Param->Url, ActionURL, NAME_SIZE); 
    strncpy(Param->ServiceType, ServiceType, NAME_SIZE);

//    Param->Act = UpnpParse_BufferNoEx(tmpStr);
    Param->Act = Act;
//    free(tmpStr);
    Param->Cookie = (char*) Cookie_const;
    Param->Fun = Fun;

    tpool_Schedule( (void (*)(void*)) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpSendActionAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpSendActionAsync *********************/


EXPORT  int UpnpGetServiceVarStatusAsync(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *VarName_const,
    IN Upnp_FunPtr Fun,
    IN const void * Cookie_const
    )
{
    struct Handle_Info * SInfo=NULL; 
    struct UpnpNonblockParam * Param;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpGetServiceVarStatusAsync \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT)
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (VarName == NULL || Fun == NULL ) 
        return UPNP_E_INVALID_PARAM;

    Param = (struct UpnpNonblockParam*)malloc(sizeof(struct UpnpNonblockParam));
    if (Param == NULL) 
        return UPNP_E_OUTOF_MEMORY;

    Param->FunName = STATUS;
    Param->Handle = Hnd;
    strncpy(Param->Url, ActionURL, NAME_SIZE);
    strncpy(Param->VarName, VarName, NAME_SIZE);
    Param->Fun = Fun;
    Param->Cookie = (char*) Cookie_const;

    tpool_Schedule( (void (*)(void*)) UpnpThreadDistribution, Param);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpGetServiceVarStatusAsync \n");)

    return  UPNP_E_SUCCESS;
}  /****************** End of UpnpGetServiceVarStatusAsync ****************/

EXPORT  int UpnpGetServiceVarStatus(IN UpnpClient_Handle Hnd,
    IN const char *ActionURL_const,
    IN const char *VarName_const,
    OUT Upnp_DOMString* StVar)
{
    struct Handle_Info * SInfo=NULL; 
    int retVal = 0;
    char *StVarPtr;
    char *ActionURL = (char *)ActionURL_const;
    char *VarName = (char *)VarName_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpGetServiceVarStatus \n");)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_CLIENT) 
    {
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if (ActionURL == NULL)
    {
        return UPNP_E_INVALID_URL;
    }    
    if (VarName == NULL || StVar == NULL ) 
        return UPNP_E_INVALID_PARAM;

    retVal = SoapGetServiceVarStatus(ActionURL, VarName, &StVarPtr); 
    *StVar = StVarPtr;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpGetServiceVarStatus \n");)

    return retVal;
}  /****************** End of UpnpGetServiceVarStatus *********************/
#endif // INCLUDE_CLIENT_APIS
#endif // EXCLUDE_SOAP

//-----------------------------------------------------------------------------
//
//                                   Client API's 
//
//-----------------------------------------------------------------------------

EXPORT  int UpnpDownloadUrlItem( const char *url_const,
    char **outBuf, char *contentType)
{
    char *tmpBuf;
    int retVal = 0, bufLen = 0;
    http_message msgBuf;
    token ctBuf;
    char *url = (char *)url_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpDownloadUrlItem \n");)

    if (url == NULL || outBuf == NULL || contentType == NULL)
        return UPNP_E_INVALID_PARAM;

    if (contentType !=  NULL)
        strcpy(contentType,"");
    
    if ((retVal = transferHTTP("GET", "\r\n", 2,&tmpBuf, url)) != HTTP_SUCCESS)
    {
        return retVal;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: tmpBuf is = %s\n",tmpBuf);
            UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"************************END OF OUTBUF**************************\n");)
    bufLen = strlen(tmpBuf);

    if ((retVal = parse_http_response(tmpBuf, &msgBuf, bufLen)) != HTTP_SUCCESS)
    {
        free(tmpBuf);
        return retVal;
    }

    if (msgBuf.content.size == 0)
    {

    DBGONLY(
    UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Content Size is ZERO\n");
    //UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Printing msgBuf.content.buff |%s| and the ptr is |%x|\n", msgBuf.content.buff, msgBuf.content.buff);
    //UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: Printing msgBuf.content.buff |%s| and the ptr is |%x|\n", msgBuf.content.buff, msgBuf.content.buff);
    )
    msgBuf.content.buff = NULL;

        free(tmpBuf);
        return UPNP_E_INVALID_URL;
    }
    if (strncasecmp(msgBuf.status.status_code.buff, "200", strlen("200")))
    {
        free(tmpBuf);

        DBGONLY(
        UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: HTTP ERROR IN GETTING THE DOC\n");
        print_token(&msgBuf.status.status_code,UPNP_CRITICAL,API,__FILE__,__LINE__);)

        return UPNP_E_INVALID_URL;
    }
    (*outBuf) = (char *) malloc (msgBuf.content.size + 1);
    if (*outBuf == NULL)
    {
        return UPNP_E_OUTOF_MEMORY;
    }
    strcpy(*outBuf, msgBuf.content.buff);
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: copied the buffer\n");)
    if (search_for_header(&msgBuf,"Content-Type",&ctBuf))
        strncpy(contentType, ctBuf.buff, LINE_SIZE - 1 > ctBuf.size ? 
                ctBuf.size : LINE_SIZE - 1);

/*
    DBGONLY(
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"UpnpDownloadUrlItem: OutBuf is = %s\n",*outBuf);
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"************************END OF OUTBUF**************************\n");)
*/

    free_http_message(&msgBuf); // free the headers inside msgBuf
    free(tmpBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpDownloadUrlItem \n");)

    return UPNP_E_SUCCESS;
}  /****************** End of UpnpDownloadUrlItem *********************/

//#include "XMLDocs.h"

EXPORT  int UpnpDownloadXmlDoc(const char *url_const,
    Upnp_Document *xmlDoc)
{
    char *xmlBuf = NULL;
    char contentType[LINE_SIZE] = "";
    int retVal = 0;
    char *url = (char *)url_const;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpDownloadXmlDoc \n");)

    if (url == NULL || xmlDoc == NULL)
    {
        return UPNP_E_INVALID_PARAM;
    }
#ifdef INCLUDE_CLIENT_APIS
//#if defined(INCLUDE_CLIENT_APIS) && defined(USE_FILESYSTEM)
    if ((retVal = UpnpDownloadUrlItem(url, &xmlBuf, contentType)) != 
         UPNP_E_SUCCESS)
    {
        return retVal;
    }
#else
    strcpy(contentType, "text/xml");
/*
    diag_printf("*** faking download of tvdevicedesc.xml when asked for %s\n", url_const);
    xmlBuf = (char*)malloc(strlen(g_szIOPlayerDeviceDescXML) + 1);
    strcpy(xmlBuf, g_szIOPlayerDeviceDescXML);
*/
    int iFileLen = FakeGetFileLength(url_const);
    if (iFileLen > 0)
    {
diag_printf("*** faking download of %s\n", url_const);
        xmlBuf = (char*)malloc(iFileLen);
        strcpy(xmlBuf, FakeGetFile(url_const));
    }
else
diag_printf("*** couldn't fake download of %s\n", url_const);


/*
diag_printf("***************************************************************\n");
for (retVal = 0; retVal < strlen(xmlBuf); ++retVal)
diag_printf("%c", xmlBuf[retVal]);
diag_printf("***************************************************************\n");
*/
#endif

    if (strncasecmp(contentType, "text/xml",strlen("text/xml")))
    {
        free(xmlBuf);
        return UPNP_E_INVALID_DESC;
    }
    
    DBGONLY(
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Got the Xml file . \n  %s ", xmlBuf);
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"********************* END OF XML FILE*******************\n");
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Going to parse it\n");)

//  *xmlDoc = UpnpParse_Buffer(xmlBuf);
    *xmlDoc = UpnpParse_BufferNoEx(xmlBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Going to free xmlBuf \n");)

    free(xmlBuf);

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"xmlBuf FREED\n");)

    if (*xmlDoc == NULL)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"Invalid desc\n");)
        return UPNP_E_INVALID_DESC;
    }
    else
    {
        DBGONLY(
        xmlBuf = UpnpNewPrintDocument(*xmlDoc);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Printing the Parsed xml document \n %s\n", xmlBuf);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"********************* END OF Parsed XML Doc*******************\n");
        free(xmlBuf);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpDownloadXmlDoc\n");)
        return UPNP_E_SUCCESS;
    }
}  /****************** End of UpnpDownloadXmlDoc *********************/
                 
//-----------------------------------------------------------------------------
//
//                          UPNP-API  Internal function implementation
//
//-----------------------------------------------------------------------------

//********************************************************
//* Name: AdvertiseAndReply
//* Description:  Function to send SSDP advertisements, replies and
//*               shutdown messages.
//* Called by:    UpnpSendAdvertisement, SsdpCallbackHandler, 
//*               UpnpUnRegisterRootDevice.
//* In:           int AdFlag :
//*                           AdFlag = -1 : Send Shutdown 
//*                           AdFlag = 0 : Send Reply
//*                           AdFlag = 1 : Send Advertisement
//*               UpnpDevice_Handle Hnd : Device handle
//*               enum SsdpSearchType SearchType : Search type for sending
//*                                                replies.
//*               struct sockaddr_in *DestAddr : Destination address
//*               char *DeviceType : Device type
//*               char *DeviceUDN : Device UDN
//*               char *ServiceType : Service type
//*               int Exp : Advertisement age
//* Out:          none
//* Return Codes: UPNP_E_SUCCESS
//* Error Codes:  UPNP_E_INVALID_HANDLE
//*
//********************************************************
#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

int AdvertiseAndReply(int AdFlag, UpnpDevice_Handle Hnd, 
                  enum SsdpSearchType SearchType, struct sockaddr_in *DestAddr,
                      char *DeviceType, char *DeviceUDN, 
                      char *ServiceType, int Exp)
{
    int i, j;
    int defaultExp = DEFAULT_MAXAGE;
    struct Handle_Info *  SInfo=NULL; 
    char  UDNstr[100], devType[100], servType[100];
    Upnp_NodeList NodeList = NULL; 
    Upnp_NodeList tmpNodeList = NULL; 
    Upnp_Node tmpNode = NULL;
    Upnp_Node tmpNode2 = NULL;
    Upnp_Node textNode = NULL;
    Upnp_DOMException err; 
    Upnp_DOMString tmpStr;

    DBGONLY(
    Upnp_DOMString dbgStr;
    UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside AdvertiseAndReply with AdFlag = %d\n", AdFlag);)

    HandleLock();
    if(GetHandleInfo(Hnd, &SInfo) != HND_DEVICE) 
    {
        diag_printf("invalid handle\n");
        HandleUnlock();
        return UPNP_E_INVALID_HANDLE;
    }
    defaultExp = SInfo->MaxAge;
    HandleUnlock();

    // parse the device list and send advertisements/replies 
    for (i = 0; ; i++)
    {

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Entering new device list with i = %d\n\n", i);)

        UpnpNode_free(tmpNode);
        tmpNode = UpnpNodeList_item(SInfo->DeviceList, i);
        if (tmpNode == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting new device list with i = %d\n\n", i);)
            break;
        }

        DBGONLY(
        dbgStr = UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Extracting device type once for %s\n", dbgStr);
        delete [] dbgStr;)
        // extract device type 

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "deviceType"); 
        if (NodeList == NULL)
        {
            continue;
        }

        DBGONLY(
        dbgStr = UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting UDN for %s\n", dbgStr);
        delete [] dbgStr;)

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type\n");)

        tmpNode2 = UpnpNodeList_item(NodeList, 0);
        UpnpNodeList_free(NodeList);
        if (tmpNode2 == NULL)
        {
            continue;
        }
        textNode = UpnpNode_getFirstChild(tmpNode2);
        UpnpNode_free(tmpNode2);
        if (textNode == NULL)
        {
            continue;
        }

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type \n");)

        tmpStr = UpnpNode_getNodeValue(textNode, &err);
        UpnpNode_free(textNode);
        if (tmpStr == NULL) 
        {
            continue;
        }
        strncpy(devType, tmpStr, 100);
        Upnpfree(tmpStr);
        if (devType == NULL)
        {
            continue;
        }

        DBGONLY(
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting device type = %s\n", devType);
        if (tmpNode == NULL)
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"TempNode is NULL\n");
        dbgStr =  UpnpNode_getNodeName(tmpNode);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Extracting UDN for %s\n",dbgStr);
        delete [] dbgStr;)

        // extract UDN 

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "UDN"); 
        if (NodeList == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        tmpNode2 = UpnpNodeList_item(NodeList, 0);
        UpnpNodeList_free(NodeList);
        if (tmpNode2 == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        textNode = UpnpNode_getFirstChild(tmpNode2);
        UpnpNode_free(tmpNode2);
        if (textNode == NULL)
        {

            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!\n");)
            continue;
        }
        tmpStr = UpnpNode_getNodeValue(textNode, &err);
        UpnpNode_free(textNode);
        if (tmpStr == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"UDN not found!!!!\n");)
            continue;
        }
        strncpy(UDNstr, tmpStr, 100);
        Upnpfree(tmpStr);
        if (UDNstr == NULL)
        {
            continue;
        }

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Sending UDNStr = %s \n", UDNstr);)

        if (AdFlag)
        {
            // send the device advertisement 
            if (AdFlag == 1)
            {
                DeviceAdvertisement(devType, i==0 , UDNstr, SERVER,SInfo->DescURL, Exp); 
            }
            else // AdFlag == -1
            {
                DeviceShutdown(devType, i==0 , UDNstr, SERVER, SInfo->DescURL, Exp); 
            }
        }
        else
        {
            switch (SearchType)
            {
                case SSDP_ALL :
                    DeviceReply(DestAddr, devType, i==0, UDNstr, SERVER,SInfo->DescURL, defaultExp);
                    break;

                case SSDP_ROOTDEVICE :

                    if (i==0)
                    {
                        SendReply(DestAddr, devType, 1, UDNstr, SERVER,SInfo->DescURL, defaultExp, 0);
                    }
                    break;
                case SSDP_DEVICE :
                    if (DeviceUDN != NULL && strlen(DeviceUDN) != 0) 
                    {
                        if (strcasecmp(DeviceUDN, UDNstr))
                        {
                            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceUDN=%s and search UDN=%s did not match\n",UDNstr, DeviceUDN);)
                            break;
                        }
                        else
                        {

                            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceUDN=%s and search UDN=%s MATCH\n",UDNstr, DeviceUDN);)
                            SendReply(DestAddr, devType, 0, UDNstr,SERVER, SInfo->DescURL, defaultExp, 0);
                            break;
                        }
  
                    }
                    if (!strcasecmp(DeviceType, devType))
                    {

                        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceType=%s and search devType=%s MATCH\n",devType,DeviceType);)
                        SendReply(DestAddr, devType, 0, UDNstr,SERVER, SInfo->DescURL, defaultExp, 1);
                    }

                    DBGONLY(else UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"DeviceType=%s and search devType=%s DID NOT MATCH\n",devType,DeviceType);)

                    break;
                default : break;
            }
        }

        // send service advertisements for services corresponding to the same device 
        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Sending service Advertisement\n");)

        UpnpNode_free(tmpNode);
        tmpNode = UpnpNodeList_item(SInfo->ServiceList, i);
        if (tmpNode == NULL)
            continue;

        NodeList = UpnpElement_getElementsByTagName(tmpNode, "service"); 
        if (NodeList == NULL)
        {
            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"Service not found 3\n");)
            continue;
        }
        for (j = 0; ; j++)
        {
            UpnpNode_free(tmpNode);
            tmpNode = UpnpNodeList_item(NodeList, j);
            if (tmpNode == NULL)
                break;

            tmpNodeList = UpnpElement_getElementsByTagName(tmpNode,"serviceType");

            if (tmpNodeList == NULL)
            {
                DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"ServiceType not found \n");)
                continue;
            }
            tmpNode2 = UpnpNodeList_item(tmpNodeList, 0);
            UpnpNodeList_free(tmpNodeList);
            if (tmpNode2 == NULL)
            {
                continue;
            }
            textNode = UpnpNode_getFirstChild(tmpNode2);
            UpnpNode_free(tmpNode2);
            if (textNode == NULL)
            {
                continue;
            }

            // servType is of format Servicetype:ServiceVersion

            tmpStr = UpnpNode_getNodeValue(textNode, &err);
            UpnpNode_free(textNode);
            if (tmpStr == NULL)
            {
                continue;
            }
            strncpy(servType, tmpStr, 100); 
            Upnpfree(tmpStr);
            if (servType == NULL)
            {
                continue;
            }

            DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"ServiceType = %s\n", servType);)

            if (AdFlag)
            {
                if (AdFlag == 1)
                {
                    ServiceAdvertisement(UDNstr, servType,SERVER, SInfo->DescURL, Exp);
                }
                else // AdFlag == -1
                {
                    ServiceShutdown(UDNstr, servType, SERVER,SInfo->DescURL, Exp);
                }
                
            }
            else
            {
                switch(SearchType)
                {
                    case SSDP_ALL :

                        ServiceReply(DestAddr, servType, UDNstr,SERVER,  SInfo->DescURL, defaultExp);
                        break;

                    case SSDP_SERVICE :

                        if (ServiceType != NULL)
                        {
                            if (!strcasecmp(ServiceType, servType))
                            {
                                ServiceReply(DestAddr, servType,UDNstr, SERVER, SInfo->DescURL, defaultExp);
                            }
                        }
                        break;
                    default : break;
                } // switch(SearchType)               

            }
        }
        UpnpNodeList_free(NodeList);
    }
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting AdvertiseAndReply : \n");)
    return  UPNP_E_SUCCESS;
}  /****************** End of AdvertiseAndReply *********************/

#endif
#endif
#ifdef INCLUDE_CLIENT_APIS

//********************************************************
//* Name: UpnpThreadDistribution
//* Description:  Function to schedule async functions in threadpool.
//* Called by:    threadpool functions
//* In:           struct UpnpNonblockParam * Param
//* Out:          none
//* Return Codes: none
//* Error Codes:  none
//********************************************************

void UpnpThreadDistribution(struct UpnpNonblockParam * Param)
{


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside UpnpThreadDistribution \n");)

    switch(Param->FunName)
    {
    #if EXCLUDE_GENA == 0
    CLIENTONLY(  
    case SUBSCRIBE: 
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaSubscribe (Param->Handle, Param->Url, 
                                (int *)&(Param->TimeOut), (char *) Evt.Sid);

            strncpy(Evt.PublisherUrl, Param->Url, NAME_SIZE);
            Evt.TimeOut = Param->TimeOut;
            Param->Fun(UPNP_EVENT_SUBSCRIBE_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;
        }
    case UNSUBSCRIBE:
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaUnSubscribe (Param->Handle, Param->SubsId);
            strncpy((char *) Evt.Sid, Param->SubsId, 42);
            Param->Fun(UPNP_EVENT_UNSUBSCRIBE_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;

        }

    case RENEW      :
        {
            struct Upnp_Event_Subscribe Evt;
            Evt.ErrCode = genaRenewSubscription (Param->Handle, Param->SubsId,
                                                 &(Param->TimeOut));

            Evt.TimeOut = Param->TimeOut;
            strncpy((char *) Evt.Sid, Param->SubsId, 42);
            Param->Fun(UPNP_EVENT_RENEWAL_COMPLETE,&Evt,Param->Cookie);
            free(Param);
            break;
        })
    #endif
    #if EXCLUDE_SOAP == 0
    case ACTION:    
        {
            struct Upnp_Action_Complete Evt;

            Evt.ActionResult = NULL;
            #ifdef INCLUDE_CLIENT_APIS

            Evt.ErrCode = SoapSendAction (Param->Url,Param->ServiceType, 
                           Param->Act, &Evt.ActionResult);
            #endif

            Evt.ActionRequest = Param->Act;
            strncpy(Evt.CtrlUrl, Param->Url, NAME_SIZE);

            Param->Fun(UPNP_CONTROL_ACTION_COMPLETE,&Evt,Param->Cookie);

            UpnpDocument_free(Evt.ActionRequest);
            UpnpDocument_free(Evt.ActionResult);
            free(Param);
            break;
        }
    case STATUS:    
        {
            struct Upnp_State_Var_Complete Evt;
            #ifdef INCLUDE_CLIENT_APIS

            Evt.ErrCode = SoapGetServiceVarStatus (Param->Url, Param->VarName,
                                                   &(Evt.CurrentVal));
            #endif
            strncpy(Evt.StateVarName, Param->VarName, NAME_SIZE);
            strncpy(Evt.CtrlUrl, Param->Url, NAME_SIZE);
             
            Param->Fun(UPNP_CONTROL_GET_VAR_COMPLETE,&Evt,Param->Cookie);
//          free(Evt.CurrentVal);
            Upnpfree(Evt.CurrentVal);
            free(Param);
            break;
        }
    #endif //EXCLUDE_SOAP
    default : break;
    } // end of switch(Param->FunName)


    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Exiting UpnpThreadDistribution \n");)

}  /****************** End of UpnpThreadDistribution  *********************/
#endif

//********************************************************
//* Name: GetCallBackFn
//* Description:  Function to get callback function ptr from a handle
//* Called by:    
//* In:           UpnpClient_Handle Hnd
//* Out:          callback function ptr
//********************************************************

Upnp_FunPtr GetCallBackFn(UpnpClient_Handle Hnd)
{
    return ((struct Handle_Info*)HandleTable[Hnd])->Callback;
}  /****************** End of GetCallBackFn *********************/

//********************************************************
//* Name: InitHandleList
//* Description:  Function to initialize handle table
//* Called by:    UpnpInit
//* In:           none
//* Out:          none
//********************************************************

void InitHandleList()
{
    int i;

    for(i=0;i<NUM_HANDLE;i++)
        HandleTable[i] = NULL;
}  /****************** End of InitHandleList *********************/

//********************************************************
//* Name: GetFreeHandle
//* Description:  Function to get a free handle
//* Called by:    UpnpRegisterRootDevice, UpnpRegisterClient
//* In:           none
//* Out:          handle
//* Error code:   UPNP_E_OUTOF_HANDLE
//********************************************************

int GetFreeHandle()
{
    int i=1;

/* Handle 0 is not used as NULL translates to 0 when passed as a handle */
    while(i < NUM_HANDLE)
    {
        if(HandleTable[i++] == NULL) break;
    }
  
    if ( i == NUM_HANDLE) 
        return UPNP_E_OUTOF_HANDLE; //Error
    else 
        return --i; 

}  /****************** End of GetFreeHandle *********************/

//********************************************************
//* Name: GetClientHandleInfo
//* Description:  Function to get client handle info
//* Called by:    
//* In:           UpnpClient_Handle *client_handle_out
//*               struct Handle_Info **HndInfo
//* Out:          client handle and its info
//* Return codes: HND_CLIENT
//* Error codes:  HND_INVALID 
//********************************************************

//Assumes at most one client
Upnp_Handle_Type GetClientHandleInfo(UpnpClient_Handle * client_handle_out, struct Handle_Info **HndInfo)
{
  (*client_handle_out)=1;
  if (GetHandleInfo(1,HndInfo)==HND_CLIENT)
    return HND_CLIENT;
  (*client_handle_out)=2;
  if (GetHandleInfo(2,HndInfo)==HND_CLIENT)
    return HND_CLIENT;
  (*client_handle_out)=-1;
  return HND_INVALID;
}  /****************** End of GetClientHandleInfo *********************/

//********************************************************
//* Name: GetDeviceHandleInfo
//* Description:  Function to get device handle info
//* Called by:
//* In:           UpnpClient_Handle *device_handle_out
//*               struct Handle_Info **HndInfo
//* Out:          device handle and its info
//* Return codes: HND_DEVICE
//* Error codes:  HND_INVALID
//********************************************************   
//Assumes at most one device

Upnp_Handle_Type GetDeviceHandleInfo(UpnpDevice_Handle * device_handle_out, struct Handle_Info **HndInfo)
{
  (*device_handle_out)=1;
  if (GetHandleInfo(1,HndInfo)==HND_DEVICE)
    return HND_DEVICE;
  
  (*device_handle_out)=2;
  if (GetHandleInfo(2,HndInfo)==HND_DEVICE)
    return HND_DEVICE;  
  (*device_handle_out)=-1;

  return HND_INVALID;
}  /****************** End of GetDeviceHandleInfo *********************/


//********************************************************
//* Name: GetHandleInfo
//* Description:  Function to get handle info
//* Called by:
//* In:           UpnpClient_Handle Hnd
//*               struct Handle_Info **HndInfo
//* Out:          handle info
//* Return codes: Handle type
//* Error codes:  UPNP_E_INVALID_HANDLE
//********************************************************  

Upnp_Handle_Type GetHandleInfo(UpnpClient_Handle Hnd, 
                               struct Handle_Info **HndInfo)
{

    DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GetHandleInfo: Handle is %d\n", Hnd);)

    if (Hnd < 1 || Hnd >= NUM_HANDLE)
    {

        DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GetHandleInfo : Handle out of range\n");)
//        return UPNP_E_INVALID_HANDLE;
        return HND_INVALID;
    }
    if (HandleTable[Hnd] != NULL) 
    {

        *HndInfo = (struct Handle_Info *) HandleTable[Hnd];
        return ((struct Handle_Info *) *HndInfo)->HType;
    }

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"GetHandleInfo : exiting\n");)

//    return UPNP_E_INVALID_HANDLE;
    return HND_INVALID;
}  /****************** End of GetHandleInfo *********************/


int PrintHandleInfo(UpnpClient_Handle Hnd)
{
    struct Handle_Info *HndInfo;
    if (HandleTable[Hnd] != NULL)
    {
        HndInfo = (struct Handle_Info *)HandleTable[Hnd];
        DBGONLY(
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Printing information for Handle_%d\n", Hnd);
        UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"HType_%d\n", HndInfo->HType);
        if (HndInfo->HType!=HND_CLIENT)UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"DescURL_%s\n", HndInfo->DescURL);)
    }
    else  return UPNP_E_INVALID_HANDLE;

    return UPNP_E_SUCCESS;
}  /****************** End of PrintHandleInfo *********************/

//********************************************************
//* Name: FreeHandle
//* Description:  Function to free handle info
//* Called by:    UpnpUnRegister functions
//* In:           int Upnp_Handle
//* Out:          none
//* Return codes: UPNP_E_SUCCESS
//* Error codes:  UPNP_E_INVALID_HANDLE
//********************************************************

int FreeHandle(int Upnp_Handle)
{
    if (Upnp_Handle < 1 || Upnp_Handle >= NUM_HANDLE)
    {
        DBGONLY(UpnpPrintf(UPNP_CRITICAL,API,__FILE__,__LINE__,"FreeHandleInfo : Handle out of range\n");)
        return UPNP_E_INVALID_HANDLE;
    }

    if (HandleTable[Upnp_Handle] == NULL)
        return UPNP_E_INVALID_HANDLE;

    free( HandleTable[Upnp_Handle] );
    HandleTable[Upnp_Handle] = NULL;
    return  UPNP_E_SUCCESS;
}  /****************** End of FreeHandle *********************/


void printNodes(Upnp_Node tmpRoot, int depth)
{
    int i;
    Upnp_NodeList NodeList1;
    Upnp_Node ChildNode1;
    unsigned short NodeType;
    Upnp_DOMString NodeValue;
    Upnp_DOMString NodeName;
    Upnp_DOMException err; 

    NodeList1= UpnpNode_getChildNodes(tmpRoot);
    for (i=0; i< 100; i++)
    {
        ChildNode1 = UpnpNodeList_item(NodeList1,i);
        if (ChildNode1 == NULL)
        {
            break;
        }

        printNodes(ChildNode1, depth + 1);

        NodeType = UpnpNode_getNodeType((Upnp_Node)ChildNode1);
        NodeValue = UpnpNode_getNodeValue(ChildNode1, &err);
        NodeName = UpnpNode_getNodeName(ChildNode1);

        DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"DEPTH-%2d-Node Type %d, Node Name: %s, Node Value: %s\n", 
               depth, NodeType, NodeName, NodeValue);)

        Upnpfree(NodeName);
        Upnpfree(NodeValue);
    }
}  /****************** End of printNodes *********************/

#if EXCLUDE_SSDP == 0

//********************************************************
//* Name: SsdpCallbackEventHandler
//* Description:  Ssdp callback handler function for upnp api layer
//* Called by:    ssdp layer
//* In:           SsdpEvent * Evt
//********************************************************

void SsdpCallbackEventHandler(SsdpEvent * Evt)
{
    int h;
    struct Upnp_Discovery *param;
    struct Handle_Info *SInfo = NULL;
//    Upnp_EventType retEventType = UPNP_E_SUCCESS;
    // -ecm: This is really, really stupid, but that's what's going on in the code
    // with their dumbshit enum types.
    Upnp_EventType retEventType = UPNP_CONTROL_ACTION_REQUEST;
    char *cptr;

    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside SsdpCallbackEventHandler \n");)

    if( Evt!= NULL && Evt->ErrCode == 0)
    {
        switch(Evt->Cmd)
        {

            case SSDP_OK:      

            case SSDP_ALIVE:   

            case SSDP_BYEBYE: 
 
            case SSDP_TIMEOUT:                
                 DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"SsdpCallbackEventHandler with Cmd %d \n", Evt->Cmd);)

                /* we are assuming that there can be only one client supported at a time */
                if(GetClientHandleInfo(&h, &SInfo) != HND_CLIENT) {
                    diag_printf(" getclienthandleinfo failed\n");
                    break;
                }
                 
                if (Evt->Cmd == SSDP_TIMEOUT)
                {
                    diag_printf("SSDP timeout hit\n");
                    SInfo->Callback(UPNP_DISCOVERY_SEARCH_TIMEOUT, NULL, 
                                Evt->Cookie);
                    break;
                }
                /* callback on client with Upnp_Discovery */

                param = (struct Upnp_Discovery *) malloc (sizeof(struct Upnp_Discovery)); 

                param->ErrCode = Evt->ErrCode;
                param->Expires = Evt->MaxAge;
                strncpy(param->DeviceType, Evt->DeviceType, LINE_SIZE);
                strncpy(param->DeviceId, Evt->UDN, LINE_SIZE);
                strncpy(param->ServiceType, Evt->ServiceType, LINE_SIZE);
                // fix to eliminate leading spaces in location
                for (cptr = Evt->Location; *cptr == ' '; cptr++);
                strncpy(param->Location, cptr, LINE_SIZE);
                strncpy(param->Os, Evt->Os, LINE_SIZE);
                strncpy(param->Date, Evt->Date, LINE_SIZE);
                strncpy(param->Ext, Evt->Ext, LINE_SIZE);
                param->DestAddr = (struct sockaddr_in *)Evt->DestAddr;
                switch (Evt->Cmd)
                {
                    case SSDP_OK :
                                      retEventType = UPNP_DISCOVERY_SEARCH_RESULT;
                                      break;

                    case SSDP_ALIVE : Evt->Cookie =  SInfo->Cookie;
                                      retEventType = UPNP_DISCOVERY_ADVERTISEMENT_ALIVE;
                                      break;

                    case SSDP_BYEBYE : Evt->Cookie =  SInfo->Cookie;
                                       retEventType = UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE;
                                       break;
                    default : break;
                }
                SInfo->Callback(retEventType, param,Evt->Cookie);
                free(param);

                DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"SsdpCallbackEventHandler : after client callback \n");)

                break;

            #ifdef INCLUDE_DEVICE_APIS
            case SSDP_SEARCH:
                //                DBGONLY(
                //#undef UpnpPrintf
                //#define UpnpPrintf(a,b,c,d,s...) diag_printf("%s %d", c, d); diag_printf(##s)
#if 0
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"SsdpCallbackEventHandler with Cmd %d SEARCH\n", Evt->Cmd);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MAX-AGE     =  %d\n",Evt->MaxAge);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MX     =  %d\n",Evt->Mx);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceType   =  %s\n",Evt->DeviceType);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceUuid   =  %s\n",Evt->UDN);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"ServiceType =  %s\n",Evt->ServiceType);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Location    =  %s\n",Evt->Location);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"HostAddr    =  %s\n",Evt->HostAddr);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"OS    =  %s\n",Evt->Os);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Date    =  %s\n",Evt->Date);
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Ext    =  %s\n",Evt->Ext);
#endif
/*
                diag_printf("SsdpCallbackEventHandler with Cmd %d SEARCH\nMAX-AGE     =  %d\nMX     =  %d\nDeviceType   =  %s\nDeviceUuid   =  %s\nServiceType =  %s\nLocation    =  %s\nHostAddr    =  %s\nOS    =  %s\nDate    =  %s\nExt    =  %s\n",
                    Evt->Cmd, Evt->MaxAge, Evt->Mx, Evt->DeviceType, Evt->UDN, Evt->ServiceType, Evt->Location, Evt->HostAddr, Evt->Os, Evt->Date, Evt->Ext);
*/
                UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"SsdpCallbackEventHandler with Cmd %d SEARCH\nMAX-AGE     =  %d\nMX     =  %d\nDeviceType   =  %s\nDeviceUuid   =  %s\nServiceType =  %s\nLocation    =  %s\nHostAddr    =  %s\nOS    =  %s\nDate    =  %s\nExt    =  %s\n",
                    Evt->Cmd, Evt->MaxAge, Evt->Mx, Evt->DeviceType, Evt->UDN, Evt->ServiceType, Evt->Location, Evt->HostAddr, Evt->Os, Evt->Date, Evt->Ext);
                //                fflush(stdout);)

                if (GetDeviceHandleInfo(&h, &SInfo) != HND_DEVICE) {
                    diag_printf("getdevicehandleinfo failed\n");
                    break;
                }

                AdvertiseAndReply(0, h, Evt->RequestType, Evt->DestAddr, 
                Evt->DeviceType, Evt->UDN, Evt->ServiceType, SInfo->MaxAge);
                break;
             #endif

            default : break;
        } // END OF switch (Evt->Cmd)

    }  // END OF if ( Evt!= NULL && Evt->ErrCode == 0)


    DBGONLY(
/*
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MAX-AGE     =  %d\n",Evt->MaxAge);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceType   =  %s\n",Evt->DeviceType);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"DeviceUuid   =  %s\n",Evt->UDN);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"ServiceType =  %s\n",Evt->ServiceType);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Location    =  %s\n",Evt->Location);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"HostAddr    =  %s\n",Evt->HostAddr);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"OS    =  %s\n",Evt->Os);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Date    =  %s\n",Evt->Date);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Ext    =  %s\n",Evt->Ext);
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"Exiting SsdpCallbakEventHandler\n");
*/
/*
    diag_printf("MAX-AGE     =  %d\nMX     =  %d\nDeviceType   =  %s\nDeviceUuid   =  %s\nServiceType =  %s\nLocation    =  %s\nHostAddr    =  %s\nOS    =  %s\nDate    =  %s\nExt    =  %s\n",
        Evt->MaxAge, Evt->Mx, Evt->DeviceType, Evt->UDN, Evt->ServiceType, Evt->Location, Evt->HostAddr, Evt->Os, Evt->Date, Evt->Ext);
*/
    UpnpPrintf(UPNP_PACKET,API,__FILE__,__LINE__,"MAX-AGE     =  %d\nMX     =  %d\nDeviceType   =  %s\nDeviceUuid   =  %s\nServiceType =  %s\nLocation    =  %s\nHostAddr    =  %s\nOS    =  %s\nDate    =  %s\nExt    =  %s\n",
        Evt->MaxAge, Evt->Mx, Evt->DeviceType, Evt->UDN, Evt->ServiceType, Evt->Location, Evt->HostAddr, Evt->Os, Evt->Date, Evt->Ext);
    )

}  /****************** End of SsdpCallbackEventHandler  *********************/
#endif // EXCLUDE_SSDP



//********************************************************
//* Name: getlocalhostname
//* Description:  Function to get local IP address
//* Called by:    UpnpInit
//* In:           char *out
//* Out:          Ip address
//* Return codes: UPNP_E_SUCCESS
//* Error codes:  -1 
//********************************************************

#ifndef _WIN32
int getlocalhostname(char *out)
{
#ifdef USE_PTHREADS
    struct hostent *hptr;
    struct utsname name;
    struct in_addr temp_addr;

      
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname\n");)
    if (uname(&name)<0)
        return -1;
  
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname : after uname\n");)
//diag_printf("*** " __FILE__ ": %d: gethostbyname: %s\n", __LINE__, name.nodename);
    if ( (hptr=gethostbyname(name.nodename))==NULL)
        return -1;
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname : after gethostbyname\n");)
    temp_addr=*(struct in_addr *)hptr->h_addr;
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname : after  temp_addr\n");)
    strncpy(out,inet_ntoa(temp_addr),LINE_SIZE);
    DBGONLY(UpnpPrintf(UPNP_ALL,API,__FILE__,__LINE__,"Inside getlocalhostname : after strncpy\n");)
#endif
    return UPNP_E_SUCCESS;
}

#endif // _WIN32

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

void AutoAdvertise(void *input)
{
    upnp_timeout *event =(upnp_timeout *) input;
    UpnpSendAdvertisement(event->handle, *((int *) event->Event));
    free_upnp_timeout(event);
}
#endif //INCLUDE_DEVICE_APIS
#endif

/* **************************** */
#ifdef INTERNAL_WEB_SERVER
EXPORT int UpnpSetWebServerRootDir( IN const char* rootDir )
{
    http_SetRootDir( rootDir );

    if ( rootDir == NULL )
    {
        // Disable GET command
        SetHTTPGetCallback( NULL );
    }
    else
    {
        // enable GET command
        SetHTTPGetCallback( http_OldServerCallback );
    }
    return UPNP_E_SUCCESS;
}
#endif /* INTERNAL_WEB_SERVER */
/* *************************** */
                       
/*********************** END OF FILE upnpapi.c :) ************************/
