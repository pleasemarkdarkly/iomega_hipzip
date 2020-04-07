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
// $Revision: 1.24 $
// $Date: 2000/08/09 23:37:23 $
// 

// File : upnpapi.h

#include <util/upnp/api/upnp.h>
#include <util/upnp/api/interface.h>


#ifndef UPNPDK_H
#define UPNPDK_H

#define DEV_LIMIT 200

#define NUM_HANDLE 200

#define DEFAULT_MX 5

#define DEFAULT_MAXAGE 1800

typedef enum {
    SUBSCRIBE,
    UNSUBSCRIBE,
    DK_NOTIFY,
    QUERY,
    ACTION,
    STATUS,
    DEVDESCRIPTION,
    SERVDESCRIPTION,
    MINI,
    RENEW} UpnpFunName;

struct  UpnpNonblockParam 
{ 
    UpnpFunName  FunName;
    int   Handle;
    int   TimeOut;
    char  VarName[NAME_SIZE];
    char  NewVal[NAME_SIZE];
    char  DevType[NAME_SIZE];
    char  DevId[NAME_SIZE];
    char  ServiceType[NAME_SIZE];
    char  ServiceVer[NAME_SIZE];
    char  Url[NAME_SIZE];
    Upnp_SID   SubsId;
    char  *Cookie;
    Upnp_FunPtr Fun;
    Upnp_Document Act;
    struct DevDesc *Devdesc;
};

#define E_HTTP_SYNTAX -6

// globals
void InitHandleList();
int GetFreeHandle();
int FreeHandle(int Handle);
void UpnpThreadDistribution(struct UpnpNonblockParam * Param);
int AdvertiseAndReply(int AdFlag, UpnpDevice_Handle Hnd, enum SsdpSearchType 
SearchType, struct sockaddr_in *DestAddr, char *DeviceType, char *DeviceUDN, 
char *ServiceType, IN int Exp);
void SsdpCallbackEventHandler(SsdpEvent * Evt);
void AutoAdvertise(void *input);
void printNodes(Upnp_Node tmpRoot, int depth); 
int getlocalhostname(char *out);


#endif



/************************ END OF upnpapi.h **********************/
