//
// File Name: DJServices.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/upnpservices/XMLdocs.h>
#include <main/ui/common/UserInterface.h>
#include <main/upnpservices/djservices.h>

#include <main/djupnp/DJUPnP.h>

#include <main/upnpservices/DJServiceEvents.h>
#include <main/testharness/testharness.h>



#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_intr.h>           // exception ranges
#include <cyg/hal/hal_cache.h>
#include <devs/audio/dai.h>

#include <datastream/fatfile/FatFile.h>
#include <fs/flash/flashmanager.h>
#include <fs/fat/sdapi.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/DJPlayerState.h>
#include <main/main/Recording.h>

#include <util/datastructures/SimpleList.h>
#include <util/eventq/EventQueueAPI.h>
#include <util/upnp/api/upnp.h>
#include <util/upnp/protocols/gena.h>
#include <main/djupnp/sample_util.h>

#include <stdlib.h>
#include <util/debug/debug.h>

#include <tftp_support.h>
#include <io/net/Net.h>

#include <main/main/AppSettings.h>

#ifdef ENABLE_EXTERN_CONTROL
#include <main/externcontrol/ExternControl.h>
#endif

DEBUG_MODULE_S(DJSERVICES, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DJSERVICES);

#define LOCK_DEVICE_MUTEX(pMutex)	cyg_mutex_lock(pMutex);
#define UNLOCK_DEVICE_MUTEX(pMutex)	cyg_mutex_unlock(pMutex);


/* Global structure for storing the state table for this device */
struct DJService dj_player_service_table[DJ_SERVICE_SERVCOUNT];

UpnpDevice_Handle s_hDeviceHandle = -1;
/* Mutex for protecting the global state table data
   in a multi-threaded, asynchronous environment.
   All functions should lock this mutex before reading
   or writing the state table data. */
cyg_mutex_t DJDevMutex;

static bool bTestRunInProgress;

#define MAX_FORMAT_BUFFER 1024
static char szFormatBuffer[MAX_FORMAT_BUFFER];

typedef int fnActionHandler(struct Upnp_Action_Request *ca_event);

int StopTestStimulator(struct Upnp_Action_Request *ca_event);
int InvokeScriptHandler(struct Upnp_Action_Request *ca_event);
int StartMonkeyTest(struct Upnp_Action_Request *ca_event);
int GetDeviceStatus(struct Upnp_Action_Request *ca_event);
int LogDebugOutput(struct Upnp_Action_Request *ca_event);
int StartNetDebug(struct Upnp_Action_Request *ca_event);
int InvokeFunction(struct Upnp_Action_Request *ca_event);
int StopDebugLog(struct Upnp_Action_Request *ca_event);
int StopNetDebug(struct Upnp_Action_Request *ca_event);
int StartEventLogging(struct Upnp_Action_Request *ca_event);
int StopEventLogging(struct Upnp_Action_Request *ca_event);
int InstantMessage(struct Upnp_Action_Request *ca_event);
int DeleteLogDir(struct Upnp_Action_Request *ca_event);
int GetLogNums(struct Upnp_Action_Request *ca_event);
int StartTestController(struct Upnp_Action_Request *ca_event);

int ControlDeviceHandler(struct Upnp_Action_Request *ca_event);


IUserInterface* g_pUI = NULL;

void UPnPSetUserInterface( IUserInterface* pUI )
{
	g_pUI = pUI;
}

static cyg_flag_t g_flagControlCallbackComplete;

typedef struct s_ActionTable
{
	char * szActionName;
	fnActionHandler *pfnActionFunction;
} t_ActionTable;

#define NUM_ACTION_HANDLERS 16

t_ActionTable g_ActionHandlers[NUM_ACTION_HANDLERS] = {
	{ "ControlDevice", ControlDeviceHandler },
	{ "InvokeScript", InvokeScriptHandler },
	{ "StartMonkeyTest",StartMonkeyTest},
	{ "GetDeviceStatus",GetDeviceStatus},
	{ "StopTestStimulator",StopTestStimulator},
	{ "LogDebugOutput",LogDebugOutput},
	{ "StartNetDebug",StartNetDebug},
	{ "InvokeFunction",InvokeFunction},
	{ "StopDebugLog",StopDebugLog},
	{ "StopNetDebug",StopNetDebug},
	{ "StartEventLogging",StartEventLogging},
	{ "StopEventLogging",StopEventLogging},
	{ "InstantMessage",InstantMessage},
	{ "DeleteLogDir",DeleteLogDir},
	{ "GetLogNums",GetLogNums},
    { "StartTestController",StartTestController}
};

typedef void fnRemoteFunction();

void FormatDrive();
void DumpStatistics();
void ClearContent();
void RefreshContent();
void ClearCDCache();
void ResetCDDB();
void UpdateImage();

typedef struct s_RemoteFunctionTable
{
	char * szFunctionName;
	char * szFunctionDescription;
	fnRemoteFunction *pRemoteFunction;
} t_RemoteFunctionTable;


const t_RemoteFunctionTable g_RemoteFunctions[] = {
	{ "FormatDrive", "Format the local hard drive.",FormatDrive },
	{ "DumpStatistics","Dump player statistics to the debug output.", DumpStatistics },
    { "ClearContent","Clear the content database.", ClearContent },
    { "RefreshContent","Refresh the content database.", RefreshContent },
    { "ClearCDCache","Clear the cache of freedb selections.", ClearCDCache },
    { "ResetCDDB","Resets the CDDB db to its initial unupdated state.", ResetCDDB },
	{ "UpdateImage","Update the devices image and reboot.", UpdateImage }
};


static struct Upnp_Action_Request *g_ca_event;

#ifdef ENABLE_EXTERN_CONTROL
void SoapControlResponseHandler(t_ControlReturn crReturnValue, const char * szResponse)
{

	char result_str[500];

	if (szResponse == NULL)
	{
		sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"><ControlResponse>!!%d</ControlResponse></u:%sResponse>", 
			g_ca_event->ActionName, DJServiceType[0],(int)crReturnValue,g_ca_event->ActionName);	
		g_ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);
	}
	else if (crReturnValue == CONTROL_OK)
	{
		char * response = (char *)malloc(500+strlen(szResponse));
		sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"><ControlResponse>!#%s</ControlResponse></u:%sResponse>", 
			g_ca_event->ActionName, DJServiceType[0],szResponse,g_ca_event->ActionName);	

		g_ca_event->ActionResult = UpnpParse_BufferNoEx(response);
	}
	// just respond with success regardless
	g_ca_event->ErrCode = UPNP_E_SUCCESS;

	// switch back to the upnp thread that originally called us now
	cyg_flag_setbits(&g_flagControlCallbackComplete,1);
}
#endif


int ControlDeviceHandler(struct Upnp_Action_Request *ca_event)
{
#ifdef ENABLE_EXTERN_CONTROL
	char * szRequest = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "ControlRequest");
	if (szRequest)
	{
		g_ca_event = ca_event;
		ProcessControlRequest(szRequest,CONTROL_SOAPACTION,SoapControlResponseHandler);
		cyg_flag_wait(&g_flagControlCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);
	}

#endif

	return UPNP_E_SUCCESS;
}

int StopTestStimulator(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Stopping current script or monkey test.\n"); 
	CTestStimulator::GetInstance()->StopTest();
	UINT16 attribs;
	if ((pc_get_attributes(TEST_RERUN_FILE, &attribs) == YES))
	{
		DEBUGP(DJSERVICES, DBGLEV_INFO, "Found TestFile - attempting to delete.\n");
		if(!pc_unlink(TEST_RERUN_FILE))
		{
			DEBUGP(DJSERVICES, DBGLEV_INFO, "Unable to delete TestFile\n");
		}
		else
		{
			DEBUGP(DJSERVICES, DBGLEV_INFO, "TestFile deleted.\n");
		}
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	bTestRunInProgress = false;

	return UPNP_E_SUCCESS;
}

int StopDebugLog(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Stopping debug log to file.\n"); 

	CDebugRouter::GetInstance()->EnableDebugRouting(false);
	CDebugRouter::GetInstance()->Flush();

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int StopNetDebug(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Stopping Network Debugging.\n"); 

	CDebugRouter::GetInstance()->EnableNetworkRouting(false);
	CDebugRouter::GetInstance()->Flush();

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int DeleteLogDir(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Deleting Log Directory.\n");

	CDebugRouter::GetInstance()->Flush();	//have to close current log file;

	DSTAT myStat;
	char DirName[EMAXPATH];
	char FileName[EMAXPATH];
	sprintf(DirName, "%s/*.*", DEBUG_LOG_FOLDER);
	if(pc_gfirst(&myStat, DirName))
	{

		while(1)
		{
			while(pc_gnext(&myStat));
			if(myStat.fname[0] == '.')
				break;
			else
			{
				sprintf(FileName, "%s/%s", DEBUG_LOG_FOLDER, myStat.fname);
				int i;
				for(i = 0;FileName[i] && FileName[i] != ' ';i++);
				FileName[i] = '\0';
				strcat(FileName, ".");
				strcat(FileName, myStat.fext);
				//diag_printf("attempting to delete file: %s\n", FileName);
				if(!pc_unlink(FileName))
				{
					diag_printf("couldn't remove file: %s\n", FileName);
					break;
				}
				else
					diag_printf("removed file: %s\n", FileName);
				pc_gdone(&myStat);
				pc_gfirst(&myStat, DirName);
			}
		}
		pc_gdone(&myStat);
		if(pc_rmdir(DEBUG_LOG_FOLDER))
			diag_printf("removed dir: %s\n", DEBUG_LOG_FOLDER);
		else
			diag_printf("couldn't remove dir: %s\n", DEBUG_LOG_FOLDER);
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int StartEventLogging(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Starting event logging.\n"); 

	CEventRecorder::GetInstance()->EnableEventLogging(true);

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int StopEventLogging(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Stopping event logging.\n"); 

	CEventRecorder::GetInstance()->EnableEventLogging(false);

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}


int GetDeviceStatus(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Request to get device status.\n"); 

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"><DeviceStatus>%s</DeviceStatus></u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], CDeviceState::GetInstance()->GetDeviceState(),ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int GetLogNums(struct Upnp_Action_Request *ca_event)\
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Request to get Log Numbers.\n");

	char IDString[1024]; //should be large enough
	char SearchDir[20];
	strcpy(SearchDir, DEBUG_LOG_FOLDER);
	strcat(SearchDir, "/*.*");

	DSTAT myStat;
	if(pc_gfirst(&myStat, SearchDir))
	{
		strcpy(IDString, "logs");
		while(pc_gnext(&myStat))
		{
			int i;
			if(strncmp(myStat.fname, "LOG-", 4))
				continue;
			for(i = 0;myStat.fname[i];i++)
			{
				if(myStat.fname[i] == '-')
				{
					i++;
					break;
				}
			}
			if(myStat.fname[i])
			{
				strcat(IDString, ":");
				int CurrentLength = strlen(IDString);
				while(myStat.fname[i] && myStat.fname[i] != ' ')
				{
					IDString[CurrentLength] = myStat.fname[i];
					i++;
					CurrentLength++;
				}
				IDString[CurrentLength] = '\0';
			}
		}
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"><LogIDString>%s</LogIDString></u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], IDString,ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int InvokeScriptHandler(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Invoking test script.\n"); 

	char * szValue;

	szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "ScriptParameters");

	if (szValue && !bTestRunInProgress)
	{
		int h_TestConfigFile;
		sprintf(szFormatBuffer, "script %s",szValue);
		if((h_TestConfigFile = po_open(TEST_RERUN_FILE, PO_TRUNC | PO_WRONLY | PO_CREAT | PO_NOSHAREANY,PS_IWRITE)) != -1)
		{
			po_write(h_TestConfigFile,(UCHAR*)szFormatBuffer,strlen(szFormatBuffer));
			po_close(h_TestConfigFile);
			DEBUGP(DJSERVICES, DBGLEV_INFO, "created test.cfg file.\n");
		}
		else
		{
			DEBUGP(DJSERVICES, DBGLEV_INFO, "couldn't create test.cfg file.\n");
		}
		CTestStimulator::GetInstance()->ExecuteScript(szValue);
		Upnpfree(szValue);
		bTestRunInProgress = true;
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int LogDebugOutput(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Starting debug log to file.\n"); 

	char * szValue;

	szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "DebugFile");

	if (szValue)
	{
		CDebugRouter::GetInstance()->EnableDebugRouting(true);
		CDebugRouter::GetInstance()->RouteToFile(szValue);
		Upnpfree(szValue);
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int StartNetDebug(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Starting Net Debugging.\n");

	char * szServer;

	szServer = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Server");

	if (szServer)
	{
		CDebugRouter::GetInstance()->EnableNetworkRouting(true);
		CDebugRouter::GetInstance()->RouteToNetwork(szServer, 2000);
		Upnpfree(szServer);
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int InvokeFunction(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Invoking function.\n"); 

	char * szValue;
	int i;

	szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "FunctionName");

	if (szValue)
	{
		bool bFunctionFound = 0;
		for (i=0;i<sizeof(g_RemoteFunctions)/sizeof(t_RemoteFunctionTable);i++)
		{
			if (strcmp(szValue,g_RemoteFunctions[i].szFunctionName)==0)
			{
				bFunctionFound = true;
				g_RemoteFunctions[i].pRemoteFunction();
			}
		}

		
		Upnpfree(szValue);
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int InstantMessage(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Instant Message.\n"); 

	char * szValue;

	szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "Message");

	if (szValue)
	{
		
		TCHAR szMessage[256];
        CharToTcharN(szMessage,szValue,255 );
		
		if (g_pUI)
		{
			g_pUI->SetMessage(szMessage);
		}

		
		Upnpfree(szValue);
	}

	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}



int StartMonkeyTest(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Starting monkey test.\n"); 

	char * szValue;

	szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "MonkeyParams");

	if (szValue && !bTestRunInProgress)
	{
		int iHoldDelay,iMaxEventDelay,iMaxHoldCount,iMinEventDelay,iMinHoldCount;
		unsigned int uEventMask, uEventMaskHi;

		if (sscanf(szValue,"%d %d %d %d %d %x %x",&iHoldDelay,&iMaxEventDelay,&iMaxHoldCount,&iMinEventDelay,&iMinHoldCount,&uEventMask,&uEventMaskHi)==7)
		{
			CTestStimulator::GetInstance()->StartMonkeyTest(iHoldDelay,iMaxEventDelay,iMaxHoldCount,iMinEventDelay,iMinHoldCount,uEventMask,uEventMaskHi);
			int h_TestConfigFile;
			sprintf(szFormatBuffer, "monkey %d %d %d %d %d %x %x",iHoldDelay,iMaxEventDelay,iMaxHoldCount,iMinEventDelay,iMinHoldCount,uEventMask,uEventMaskHi);
			if((h_TestConfigFile = po_open(TEST_RERUN_FILE, PO_TRUNC | PO_WRONLY | PO_CREAT | PO_NOSHAREANY,PS_IWRITE)) != -1)
			{
				po_write(h_TestConfigFile,(UCHAR*)szFormatBuffer,strlen(szFormatBuffer));
				po_close(h_TestConfigFile);
				DEBUGP(DJSERVICES, DBGLEV_INFO, "created test.cfg file.\n");
			}
			else
			{
				DEBUGP(DJSERVICES, DBGLEV_INFO, "couldn't create test.cfg file.\n");
			}
		}
		else
		{
			DEBUGP(DJSERVICES, DBGLEV_INFO, "Error in parameters to StartMonkeyTest: %s\n",  szValue); 
		}

		Upnpfree(szValue);

		bTestRunInProgress = true;
	}


	char result_str[500];

	// just respond with success regardless
	ca_event->ErrCode = UPNP_E_SUCCESS;

	sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
			ca_event->ActionName, DJServiceType[0], ca_event->ActionName);

	ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);

	return UPNP_E_SUCCESS;
}

int StartTestController(struct Upnp_Action_Request *ca_event)
{
    DEBUGP(DJSERVICES, DBGLEV_INFO, "Starting test controller.\n"); 
    
    char * szValue;
    
    szValue = SampleUtil_GetFirstDocumentItem(ca_event->ActionRequest, "MultiScriptParams");
    
    if (szValue && !bTestRunInProgress)
    {
        int iNumFiles, iRandom, iRepeat, iRetry;
        
        if (sscanf(szValue,"%d %d %d %d",&iNumFiles,&iRandom,&iRepeat,&iRetry)==4)
        {
            CTestStimulator::GetInstance()->ExecuteMultipleScripts(iNumFiles, (iRandom != 0), (iRepeat != 0), iRetry);
            int h_TestConfigFile;
			sprintf(szFormatBuffer, "multi %d %d %d %d",iNumFiles, iRandom, iRepeat, iRetry);
			if((h_TestConfigFile = po_open(TEST_RERUN_FILE, PO_TRUNC | PO_WRONLY | PO_CREAT | PO_NOSHAREANY,PS_IWRITE)) != -1)
			{
				po_write(h_TestConfigFile,(UCHAR*)szFormatBuffer,strlen(szFormatBuffer));
				po_close(h_TestConfigFile);
				DEBUGP(DJSERVICES, DBGLEV_INFO, "created test.cfg file.\n");
			}
			else
			{
				DEBUGP(DJSERVICES, DBGLEV_INFO, "couldn't create test.cfg file.\n");
			}
        }
        else
        {
            DEBUGP(DJSERVICES, DBGLEV_INFO, "Error in parameters to StartTestController: %s\n",  szValue); 
        }
        
        Upnpfree(szValue);
        
        bTestRunInProgress = true;
    }
    
    char result_str[500];
    
    // just respond with success regardless
    ca_event->ErrCode = UPNP_E_SUCCESS;
    
    sprintf(result_str, "<u:%sResponse xmlns:u=\"%s\"> </u:%sResponse>", 
        ca_event->ActionName, DJServiceType[0], ca_event->ActionName);
    
    ca_event->ActionResult = UpnpParse_BufferNoEx(result_str);
    
    return UPNP_E_SUCCESS;
}

int DJInvokeActionHandler(struct Upnp_Action_Request *ca_event)
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, ("Inside DJInvokeActionHandler.\n"));
	int i;
	int action_succeeded = -1;
	for (i=0;i<NUM_ACTION_HANDLERS;i++)
	{
		if (strcmp(ca_event->ActionName,g_ActionHandlers[i].szActionName)==0)
		{
			action_succeeded= g_ActionHandlers[i].pfnActionFunction(ca_event);
		}
	}
	if (action_succeeded==-1)
	{
		DEBUGP(DJSERVICES, DBGLEV_INFO, "Error in DJInvokeActionHandler: \n"); 
		DEBUGP(DJSERVICES, DBGLEV_INFO, "   Unknown ActionName = %s\n",  ca_event->ActionName); 

		ca_event->ErrCode = 401;
		strcpy(ca_event->ErrStr, "Invalid Action");
		ca_event->ActionResult = NULL;
	}


	return (ca_event->ErrCode);
}


// these are just examples of remote functions

void FormatDrive()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function FormatDrive.\n"); 
}

void DumpStatistics()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function DumpStatistics.\n"); 
}

void ClearContent()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function ClearContent.\n"); 

    CEventQueue::GetInstance()->PutEvent(EVENT_CLEAR_CONTENT, 0);
}

void RefreshContent()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function RefreshContent.\n"); 

    CEventQueue::GetInstance()->PutEvent(EVENT_REFRESH_CONTENT, 0);
}

void ClearCDCache()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function ClearCDCache.\n"); 

    CEventQueue::GetInstance()->PutEvent(EVENT_CLEAR_CD_CACHE, 0);
}

void ResetCDDB()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function ResetCDDB.\n"); 

    CEventQueue::GetInstance()->PutEvent(EVENT_RESET_CDDB, 0);
}

void UpdateImage()
{
	DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to function UpdateImage.\n");

	return ;
}

/********************************************************************************
 * DJHandleActionRequest
 *
 * Description: 
 *       Called during an action request callback.  If the
 *       request is for this device and either its control service
 *       or picture service, then perform the action and respond.
 *
 * Parameters:
 *   ca_event -- The control action request event structure
 *
 ********************************************************************************/
int DJHandleActionRequest(struct Upnp_Action_Request *ca_event) 
{
    DEBUGP(DJSERVICES, DBGLEV_INFO, "Call to DJHandleActionRequest.\n");
	/* Defaults if action not found */
	int action_succeeded = -1;
	int err=401;

	ca_event->ErrCode = 0;
	ca_event->ActionResult = NULL;

	return DJInvokeActionHandler(ca_event);

}





/********************************************************************************
 * DJDeviceCallbackEventHandler
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       root device or sending a search request.  Detects the type of 
 *       callback, and passes the request on to the appropriate procedure.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- Data structure containing event data
 *   Cookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
int DJDeviceCallbackEventHandler(Upnp_EventType EventType, 
			 void *Event, 
			 void *Cookie)
{
	/* Print a summary of the event received */
	SampleUtil_PrintEvent(EventType, Event);

	switch ( EventType) {

/*		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			DJHandleSubscriptionRequest(
			(struct Upnp_Subscription_Request *) Event);
			break;

		case UPNP_CONTROL_GET_VAR_REQUEST:
			DJHandleGetVarRequest(
			(struct Upnp_State_Var_Request *) Event);
			break;
*/
		case UPNP_CONTROL_ACTION_REQUEST:
			DJHandleActionRequest(
			(struct Upnp_Action_Request *) Event);
			break;

		/* ignore these cases, since this is not a control point */
		case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
		case UPNP_DISCOVERY_SEARCH_RESULT:
		case UPNP_DISCOVERY_SEARCH_TIMEOUT:
		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		case UPNP_CONTROL_ACTION_COMPLETE:
		case UPNP_CONTROL_GET_VAR_COMPLETE:
		case UPNP_EVENT_RECEIVED:
		case UPNP_EVENT_RENEWAL_COMPLETE:
		case UPNP_EVENT_SUBSCRIBE_COMPLETE:
		case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
			break;

		default:
			DEBUGP(DJSERVICES, DBGLEV_INFO, "Error in DJDeviceCallbackEventHandler: unknown event type %d\n", EventType);
	}

	return(0);
}


/********************************************************************************
 * DJStartup
 *
 * Parameters:
 *    None
 * Call this after upnp is initialized
 ********************************************************************************/

int
DJStartup()
{
	int ret=1;
	int port;
	char desc_doc_url[200];


	cyg_flag_init(&g_flagControlCallbackComplete);

	cyg_mutex_init(&DJDevMutex);
	bTestRunInProgress = false;

	char ip_address[20],
		*desc_doc_name = "DJdesc.xml",
		*web_dir_path = "a:/web";

    struct in_addr ip;
    GetInterfaceAddresses( "eth0", (unsigned int*)&ip, NULL );
	strcpy(ip_address, inet_ntoa(ip));
//	ip_address = "63.165.188.232";
	port = 5431;

    sprintf(desc_doc_url, "http://%s:%d/%s", ip_address, port, desc_doc_name);

	BuildFakeFileTable(ip_address, port);

//	DEBUGP(DJSERVICES, DBGLEV_INFO, "Intializing UPnP \nwith desc_doc_url=%s\n", desc_doc_url);
//	DEBUGP(DJSERVICES, DBGLEV_INFO, "     ipaddress=%s port=%d\n", ip_address, port);
//	DEBUGP(DJSERVICES, DBGLEV_INFO, "     web_dir_path=%s\n", web_dir_path);
//	if ((ret = UpnpInit(ip_address, port)) != UPNP_E_SUCCESS)
//	{
//		DEBUGP(DJSERVICES, DBGLEV_INFO, "Error with UpnpInit -- %d\n", ret);
//		UpnpFinish();
//		return 1;
//	}
//	DEBUGP(DJSERVICES, DBGLEV_INFO, "UPnP Initialized\n");

	DEBUGP(DJSERVICES, DBGLEV_INFO, "Specifying the webserver root directory -- %s\n", web_dir_path);
	if ((ret = UpnpSetWebServerRootDir(web_dir_path)) != UPNP_E_SUCCESS)
	{
		DEBUGP(DJSERVICES, DBGLEV_INFO, "Error specifying webserver root directory -- %s: %d\n", web_dir_path, ret);
		UpnpFinish();
		return 1;
	}


	DEBUGP(DJSERVICES, DBGLEV_INFO, "Registering the RootDevice\n");
	if ((ret = UpnpRegisterRootDevice(desc_doc_url, DJDeviceCallbackEventHandler, &s_hDeviceHandle, &s_hDeviceHandle)) != UPNP_E_SUCCESS)
	{
		DEBUGP(DJSERVICES, DBGLEV_INFO, "Error registering the rootdevice : %d\n", ret);
		UpnpFinish();
		return 1;
	}
	else
	{
		DEBUGP(DJSERVICES, DBGLEV_INFO, "RootDevice Registered\n");
     
//		DEBUGP(DJSERVICES, DBGLEV_INFO, "Initializing State Table\n");
//		DJStateTableInit(desc_doc_url);
//		DEBUGP(DJSERVICES, DBGLEV_INFO, "State Table Initialized\n");

/*		if ((ret = UpnpSendAdvertisement(s_hDeviceHandle, DEFAULT_TIMEOUT)) != UPNP_E_SUCCESS) {
			DEBUGP(DJSERVICES, DBGLEV_INFO, "Error sending advertisements : %d\n", ret);
		    UpnpFinish();
			return 1;
		}

		DEBUGP(DJSERVICES, DBGLEV_INFO, "Advertisements Sent\n");
		*/
    }

	return 0;
}


int
DJShutdown()
{
    UpnpUnRegisterRootDevice(s_hDeviceHandle);
	cyg_mutex_destroy(&DJDevMutex);

	return 0;
}

