//
// SerialControl.cpp - the serial control interface to the DJ
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <stdlib.h>
#include <stdio.h>
#include <cyg/fileio/fileio.h>
#include <cyg/infra/diag.h>
#include <stdarg.h>
#include <util/debug/debug.h>
#include <util/diag/diag.h>
#include <cyg/kernel/kapi.h>
#include <util/eventq/EventQueueAPI.h>
#include <main/ui/keys.h>
#include <core/events/SystemEvents.h>
#include <fs/fat/sdapi.h>
#include <main/ui/common/UserInterface.h>
#include <main/ui/UI.h>
#include <main/main/AppSettings.h>

#include <main/externcontrol/ExternControl.h>
#include <main/externcontrol/ExternInterface.h>



DEBUG_MODULE_S(EXTERNCONTROL, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(EXTERNCONTROL);

static cyg_mutex_t g_muExternControlAccess;

static cyg_flag_t g_flagServiceCallbackComplete;

class CExternControlInitializer
{
public:
	CExternControlInitializer() {cyg_mutex_init(&g_muExternControlAccess);
								cyg_flag_init(&g_flagServiceCallbackComplete);};

};

static CExternControlInitializer Initializer;



extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();



static bool g_bExpectingCallback = false;
static int  g_iFnIndex = -1;
static char * g_szRequest = NULL;
static t_ControlReturn g_retValue;
static t_fnControlResponseHandler * g_fnControlResponseHandler;


void ProcessControlRequest(const char * szRequest, int flControlType, t_fnControlResponseHandler * fnResponseHandler)
{

	cyg_mutex_lock(&g_muExternControlAccess);

	// first we must see what function we are doing
	char fname[30];
	memset(fname,0,30);

	char * endfname = strstr(szRequest,"&");

	DEBUGP(EXTERNCONTROL, DBGLEV_TRACE,"endfname %s\n",endfname);

	if (endfname)
	{
		if ((endfname - szRequest) < 30)
		{
			strncpy(fname,szRequest,endfname-szRequest);
			
			DEBUGP(EXTERNCONTROL, DBGLEV_TRACE,"fname case 1 %s\n",fname);
		}
	}
	else
	{
		if (strlen(szRequest) < 30)
		{
			strncpy(fname,szRequest,strlen(szRequest));
			fname[endfname-szRequest] = '\0';
			DEBUGP(EXTERNCONTROL, DBGLEV_TRACE,"fname case 2 %s\n",fname);
		}
		else
		{
			//return 0;
		}
	}

	DEBUGP(EXTERNCONTROL, DBGLEV_TRACE,"Function name is %s\n",fname);

	int i = 0;

	int fnIndex = -1;

	while ((i < NUM_CONTROL_FUNCTIONS) && (fnIndex == -1))
	{
		if (strcmp(fname,g_ExternFunctionHandlers[i].szFunctionName)==0)
		{
			fnIndex = i;
		}
		else
		{
			i++;
		}
	}

	if ((fnIndex >= 0) && (fnIndex < NUM_CONTROL_FUNCTIONS) && 
		(g_ExternFunctionHandlers[fnIndex].flControlMap & flControlType))
	{

		g_bExpectingCallback = true;
		g_iFnIndex = fnIndex;
		g_szRequest = (char*)szRequest;
		g_fnControlResponseHandler = fnResponseHandler;

		// send an event to request service in the main thread context
		CEventQueue::GetInstance()->PutEvent((unsigned int)EVENT_CONTROL_SERVICE_REQUEST,0);

		cyg_flag_wait(&g_flagServiceCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);


	}
	else
	{
		fnResponseHandler(CONTROL_ERR_SYNTAX, NULL);
	}


	cyg_mutex_unlock(&g_muExternControlAccess);
}


void ControlServiceCallback(int cookie)
{
	// this is running on the main thread when it is called

	DBASSERT(EXTERNCONTROL,g_bExpectingCallback,"ControlServiceCallback call unexpected.");
	DBASSERT(EXTERNCONTROL,((g_iFnIndex >= 0) && (g_iFnIndex < NUM_CONTROL_FUNCTIONS)),"Function index out of range.");

	// tweak thread priority to avoid audio drop outs
	int nPrio = GetMainThreadPriority();
    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

	char * szResponse = NULL;

	t_ControlReturn retVal = g_ExternFunctionHandlers[g_iFnIndex].pfnCommandHandler(g_szRequest, &szResponse);

	g_fnControlResponseHandler(retVal, szResponse);

	if (szResponse)
	{
		free(szResponse);
	}

	SetMainThreadPriority(nPrio);

	cyg_flag_setbits(&g_flagServiceCallbackComplete,1);
}

