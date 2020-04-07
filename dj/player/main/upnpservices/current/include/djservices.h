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
// $Date: 2000/08/30 15:50:54 $
//

#ifndef DJ_DEVICE_H
#define DJ_DEVICE_H

#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include <util/upnp/api/upnp.h>
#include <main/djupnp/sample_util.h>


#define DJ_SERVICE_SERVCOUNT  1
#define DJ_SERVICE_CONTROL    0





/* This should be the maximum VARCOUNT from above */
#define TV_MAXVARS 0

extern char DJDeviceType[];
extern char *DJServiceType[];


/* Structure for storing Tv Service
   identifiers and state table */
struct DJService {
    char UDN[NAME_SIZE]; /* Universally Unique Device Name */
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableName[TV_MAXVARS]; 
    char *VariableStrVal[TV_MAXVARS];
    int  VariableCount;
};

extern struct DjService dj_service_table[];


extern UpnpDevice_Handle s_hDeviceHandle;


/* Mutex for protecting the global state table data
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the state table data. */
extern cyg_mutex_t DJDevMutex;



int
DJStartup();

void DJShutdownHdlr(int);

//int DJStateTableInit(char*);
int DJHandleSubscriptionRequest(struct Upnp_Subscription_Request *);
int DJHandleGetVarRequest(struct Upnp_State_Var_Request *);
int DJHandleActionRequest(struct Upnp_Action_Request *);
int DJCallbackEventHandler(Upnp_EventType, void*, void*);

void UPnPSetUserInterface( IUserInterface* pUI );


int DJSetServiceTableVar(unsigned int, unsigned int, char*);
int DJSetPower(int);
int DJPowerOn();
int DJPowerOff();
int DJSetChannel(int);
int DJIncrChannel(int);
int DJSetVolume(int);
int DJIncrVolume(int);
int DJSetColor(int);
int DJIncrColor(int);
int DJSetTint(int);
int DJIncrTint(int);
int DJSetContrast(int);
int DJIncrContrast(int);
int DJSetBrightness(int);
int DJIncrBrightness(int);




#endif
