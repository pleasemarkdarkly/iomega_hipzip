//
// File Name: DJUPnP.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/djupnp/DJUPnP.h>

#include <cyg/kernel/kapi.h>
#include <main/djupnp/UPnPEvents.h>
#include <main/djupnp/XMLDocs.h>
#include <util/datastructures/SimpleList.h>
#include <util/eventq/EventQueueAPI.h>
#include <util/upnp/api/upnp.h>
#include <main/djupnp/sample_util.h>

#include <stdlib.h>
#include <util/debug/debug.h>
#include <util/diag/diag.h>

DEBUG_MODULE_S(DJUPNP, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DJUPNP);


// Use DEBUG_UNPRINTABLE_STRING when diag_printf(x) complains that it can't print the string.
#define DEBUG_UNPRINTABLE_STRING(x) \
	{ \
		const char* pch = x; \
		while (*pch) \
			diag_printf("%c", *pch++); \
		diag_printf("\n"); \
	}

#define DEBUG_XML_DOC(x) \
	{ \
		char* szTheXMLDocument = UpnpNewPrintDocument(x); \
		if (szTheXMLDocument) \
		{ \
			DEBUG_UNPRINTABLE_STRING(szTheXMLDocument); \
			free(szTheXMLDocument); \
		} \
	}



#define CHECK_ALLOC(blah) DBASSERT(DJUPNP, blah, "Out of memory\n")

// fdecl
static int DJUPnPControlPointCallbackEventHandler(Upnp_EventType eventType, void *pEvent, void *pCookie);

//! Structure for storing fml service identifiers and state table.
typedef struct iml_service_s
{
	char ServiceId[NAME_SIZE];
	char ServiceType[NAME_SIZE];
	char *VariableStrVal[IML_MAXVARS];
	char EventURL[NAME_SIZE];
	char ControlURL[NAME_SIZE];
	char SID[NAME_SIZE];
} iml_service_t;

//! Used to assign a unique device number to an iml record.
static int s_iNextDeviceNumber = 1;

typedef struct iml_device_s
{
    int     iDeviceNumber;
	char    UDN[NAME_SIZE];
	char    DescDocURL[NAME_SIZE];
	char    FriendlyName[NAME_SIZE];
	int     AdvrTimeOut;
    iml_service_t   IMLService[IML_SERVICE_SERVCOUNT];
} iml_device_t;

typedef SimpleList<iml_device_t*> IMLList;
typedef SimpleListIterator<iml_device_t*> IMLListIt;

//! The global device list.
static IMLList s_slGlobalDeviceList;

static UpnpClient_Handle s_hControlPoint = -1;
//! Mutex for protecting the global device list in a multi-threaded, asynchronous environment.
//! All functions should lock this mutex before reading or writing the device list.
static cyg_mutex_t s_mtxDeviceList;

// fdecl
int DJUPnPControlPointRemoveDevice(const char* UDN);
void DJUPnPControlPointRemoveAll();

bool
DJUPnPStartup(const char* szIPAddress, int iPort)
{
	cyg_mutex_init(&s_mtxDeviceList);

    int ret;
	DEBUGP(DJUPNP, DBGLEV_INFO, "Calling UPnPInit\n");
	if ((ret = UpnpInit(szIPAddress, iPort)) != UPNP_E_SUCCESS)
	{
		DEBUG(DJUPNP, DBGLEV_ERROR, "Error with UpnpInit -- %d\n", ret);
		UpnpFinish();
		return false;
	}
	DEBUGP(DJUPNP, DBGLEV_INFO, "UPnP Initialized\n");

	DEBUGP(DJUPNP, DBGLEV_INFO, "Registering the Control Point\n");
	if ((ret = UpnpRegisterClient(DJUPnPControlPointCallbackEventHandler, &s_hControlPoint, &s_hControlPoint)))
	{
		DEBUG(DJUPNP, DBGLEV_ERROR, "Error registering control point : %d\n", ret);
		UpnpFinish();
		return false;
	}
	DEBUGP(DJUPNP, DBGLEV_INFO, "Control Point Registered\n");


    return true;
}

void DJUPnPShutdown()
{
    DJUPnPControlPointRemoveAll();
    UpnpFinish();

    cyg_mutex_destroy(&s_mtxDeviceList);
}

/********************************************************************************
 * IMLAddDevice
 *
 * Description: 
 *       If the device is not already included in the global device list,
 *       add it.  Otherwise, update its advertisement expiration timeout.
 *
 * Parameters:
 *   DescDoc -- The description document for the device
 *   location -- The location of the description document URL
 *   expires -- The expiration time for this advertisement
 *
 ********************************************************************************/
void IMLAddDevice (Upnp_Document DescDoc, char *location, int expires) 
{
    char *deviceType=NULL;
    char *friendlyName=NULL;
    char *baseURL=NULL;
    char *relURL=NULL;
    char *UDN=NULL;
    char *serviceId[IML_SERVICE_SERVCOUNT] = {NULL};
    char *eventURL[IML_SERVICE_SERVCOUNT] = {NULL};
    char *controlURL[IML_SERVICE_SERVCOUNT] = {NULL};
    Upnp_SID eventSID[IML_SERVICE_SERVCOUNT];
    iml_device_t *deviceNode = 0;
    int service, var;

    cyg_mutex_lock(&s_mtxDeviceList);

    /* Read key elements from description document */
    UDN = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
    deviceType = SampleUtil_GetFirstDocumentItem(DescDoc, "deviceType");
    friendlyName = SampleUtil_GetFirstDocumentItem(DescDoc, "friendlyName");
    baseURL = SampleUtil_GetFirstDocumentItem(DescDoc, "URLBase");
    relURL = SampleUtil_GetFirstDocumentItem(DescDoc, "presentationURL");

    if (strcmp(deviceType, g_szIMLDeviceType) == 0)
    {
	    // Check if this device is already in the list
        IMLListIt it = s_slGlobalDeviceList.GetHead();
	    while (it != s_slGlobalDeviceList.GetEnd())
	    {
		    if (strcmp((*it)->UDN, UDN) == 0)
		    {
		        // The device is already there, so just update 
		        // the advertisement timeout field
		        (*it)->AdvrTimeOut = expires;
			    break;
		    }
            ++it;
	    }

	    if (it == s_slGlobalDeviceList.GetEnd())
	    {
		    for (service = 0; service<IML_SERVICE_SERVCOUNT; service++)
		    {
			    if (SampleUtil_FindAndParseService(DescDoc, location, g_szIMLServiceType[service], &serviceId[service], &eventURL[service], &controlURL[service]))
			    {
#if 0
				    DEBUGP(DJUPNP, DBGLEV_INFO, "Subscribing to EventURL %s...\n", eventURL[service]);
				    ret=UpnpSubscribe(ctrlpt_handle,eventURL[service], &TimeOut[service],eventSID[service]);
				    if (ret==UPNP_E_SUCCESS)
				    {
					    DEBUGP(DJUPNP, DBGLEV_INFO, "Subscribed to EventURL with SID=%s\n", eventSID[service]);
				    }
				    else
				    {
					    DEBUG(DJUPNP, DBGLEV_ERROR, "Error Subscribing to EventURL -- %d\n", ret);
					    strcpy(eventSID[service], "");
				    }
#endif
				}
				else
				{
					DEBUG(DJUPNP, DBGLEV_ERROR, "Error: Could not find Service: %s\n", g_szIMLServiceType[service]);
				}
			}

			/* Create a new device node */
            if (UDN && location && friendlyName)
            {
			    deviceNode = (iml_device_t *) malloc(sizeof(iml_device_t));
			    CHECK_ALLOC(deviceNode);
                deviceNode->iDeviceNumber = s_iNextDeviceNumber++;
			    strncpy(deviceNode->UDN, UDN, NAME_SIZE);
			    strncpy(deviceNode->DescDocURL, location, NAME_SIZE);
                strncpy(deviceNode->FriendlyName, friendlyName, NAME_SIZE);
			    deviceNode->AdvrTimeOut = expires;

			    DEBUGP(DJUPNP, DBGLEV_INFO, "Found Fullplay Media Library: %s\n", friendlyName);
            }

/*
            DEBUGP(DJUPNP, DBGLEV_INFO, "((((((((((((((((((((((((((((((((((((((((((\n");
            DEBUGP(DJUPNP, DBGLEV_INFO, "UDN: %s\n", deviceNode->UDN);
            DEBUGP(DJUPNP, DBGLEV_INFO, "BaseURL: %s\n", baseURL ? baseURL : "none");
            DEBUGP(DJUPNP, DBGLEV_INFO, "DescDocURL: %s\n", deviceNode->DescDocURL);
            DEBUGP(DJUPNP, DBGLEV_INFO, "FriendlyName: %s\n", deviceNode->FriendlyName);
            DEBUGP(DJUPNP, DBGLEV_INFO, "Timeout: %d\n", deviceNode->AdvrTimeOut);
            DEBUGP(DJUPNP, DBGLEV_INFO, "))))))))))))))))))))))))))))))))))))))))))\n");
*/
			for (service = 0; service<IML_SERVICE_SERVCOUNT; service++)
			{
				strncpy(deviceNode->IMLService[service].ServiceId, serviceId[service], NAME_SIZE);
				strncpy(deviceNode->IMLService[service].ServiceType, g_szIMLServiceType[service], NAME_SIZE);
				strncpy(deviceNode->IMLService[service].ControlURL, controlURL[service], NAME_SIZE);
				strncpy(deviceNode->IMLService[service].EventURL, eventURL[service], NAME_SIZE);
				strncpy(deviceNode->IMLService[service].SID, eventSID[service], NAME_SIZE);
				for (var = 0; var < IMLVarCount[service]; var++)
				{
					deviceNode->IMLService[service].VariableStrVal[var] = (char *) malloc(NAME_SIZE);
					strcpy(deviceNode->IMLService[service].VariableStrVal[var], "" );
				}
			}

			// Insert the new device node in the list
            s_slGlobalDeviceList.PushBack(deviceNode);
		}
	}

  
	cyg_mutex_unlock(&s_mtxDeviceList);
  
    if (deviceType)
	{
		Upnpfree(deviceType);
	}
    if (UDN)
	{
		Upnpfree(UDN);
	}
    if (relURL)
	{
		Upnpfree(relURL);
	}
  
    for (service = 0; service<IML_SERVICE_SERVCOUNT; service++)
	{
		if (serviceId[service])
			Upnpfree(serviceId[service]);
		if (controlURL[service])
			Upnpfree(controlURL[service]);
		if (eventURL[service])
			Upnpfree(eventURL[service]);
    }


	// This is a new device, so inform the UI to notify the IML manager.
	if (deviceNode)
	{
        iml_found_info_t* pIMLInfo = new iml_found_info_t;
        pIMLInfo->iDeviceNumber = deviceNode->iDeviceNumber;
        // TODO: use real base URL.
        pIMLInfo->szMediaBaseURL = (char*)malloc(strlen(baseURL) + 2);
        strcpy(pIMLInfo->szMediaBaseURL, baseURL);
        if (char* pch = strrchr(pIMLInfo->szMediaBaseURL, ':'))
            strcpy(pch + 1, "3030/");
        pIMLInfo->szFriendlyName = (char*)malloc(strlen(friendlyName) + 1);
        strcpy(pIMLInfo->szFriendlyName, friendlyName);
        pIMLInfo->szUDN = (char*)malloc(strlen(deviceNode->UDN) + 1);
        strcpy(pIMLInfo->szUDN, deviceNode->UDN);
        CEventQueue::GetInstance()->PutEvent(EVENT_IML_FOUND, (void*)pIMLInfo);
	}

    if (friendlyName)
	{
		Upnpfree(friendlyName);
	}
    if (baseURL)
	{
		Upnpfree(baseURL);
	}

}


/// Look for the device and update its expiration if found
bool DJFindDeviceByUDN(const char * UDN, int expires)
{
    bool found = false;

    cyg_mutex_lock(&s_mtxDeviceList);

    IMLListIt it = s_slGlobalDeviceList.GetHead();
	while (it != s_slGlobalDeviceList.GetEnd())
	{
		if (strcmp((*it)->UDN, UDN) == 0)
		{
			found = true;
		    // The device is already there, so just update 
		    // the advertisement timeout field
		    (*it)->AdvrTimeOut = expires;
			break;
		}
        ++it;
	}

	cyg_mutex_unlock(&s_mtxDeviceList);

	return found;
}





/********************************************************************************
 * DJUPnPControlPointCallbackEventHandler
 *
 * Description: 
 *       The callback handler registered with the SDK while registering
 *       the control point.  Detects the type of callback, and passes the 
 *       request on to the appropriate function.
 *
 * Parameters:
 *   eventType -- The type of callback event
 *   pEvent -- Data structure containing event data
 *   pCookie -- Optional data specified during callback registration
 *
 ********************************************************************************/
int
DJUPnPControlPointCallbackEventHandler(Upnp_EventType eventType, void *pEvent, void *pCookie)
{
	SampleUtil_PrintEvent(eventType, pEvent);

	switch (eventType)
    {
        /* SSDP Stuff */
        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        {
            struct Upnp_Discovery *d_event = (struct Upnp_Discovery * ) pEvent;
            Upnp_Document DescDoc = NULL;
            int ret;

            if (d_event->ErrCode != UPNP_E_SUCCESS)
                DEBUG(DJUPNP, DBGLEV_ERROR, "Error in Discovery Callback -- %d\n", d_event->ErrCode);

			// see if we are a) interested in this device and if we already know about it

			if ((strcmp(d_event->DeviceType, g_szIMLDeviceType)==0) && (!DJFindDeviceByUDN(d_event->DeviceId,d_event->Expires)))
			{
	
				if ((ret=UpnpDownloadXmlDoc(d_event->Location, &DescDoc)) != UPNP_E_SUCCESS) {
	                DEBUG(DJUPNP, DBGLEV_ERROR, "Error obtaining device description from %s -- error = %d\n", d_event->Location, ret );
				} else {
	                IMLAddDevice(DescDoc, d_event->Location, d_event->Expires);
				}
	
				if (DescDoc)
				{
	                UpnpDocument_free(DescDoc);
				}
			}

//            DJUPnPControlPointPrintList();
		}
		break;

	    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
/*
		    // Inform the interface that no media stores were located.
//          CSystemMessageScreen::GetSystemMessageScreen()->NotifyMediaStoreNotFound();
            if (s_bRefreshingDataSources)
            {
	            CMSMediaStoreDataSource::SetMediaStoreSearchDone(true);
	            s_bRefreshingDataSources = false;
            }
*/
			break;

		case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		{
            struct Upnp_Discovery *d_event = (struct Upnp_Discovery * ) pEvent;

            if (d_event->ErrCode != UPNP_E_SUCCESS)
                DEBUG(DJUPNP, DBGLEV_ERROR, "Error in Discovery ByeBye Callback -- %d\n", d_event->ErrCode);

            DEBUGP(DJUPNP, DBGLEV_INFO, "Received ByeBye for Device: %s\n", d_event->DeviceId);
            if (int iDeviceNumber = DJUPnPControlPointRemoveDevice(d_event->DeviceId))
                CEventQueue::GetInstance()->PutEvent(EVENT_IML_BYEBYE, (void*)iDeviceNumber);

//            DEBUGP(DJUPNP, DBGLEV_INFO, "After byebye:\n");
//            DJUPnPControlPointPrintList();
		}
		break;

		/* SOAP Stuff */
		case UPNP_CONTROL_ACTION_COMPLETE:
		{
/*
			struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) pEvent;

			if (a_event->ErrCode != UPNP_E_SUCCESS)
				DEBUG(DJUPNP, DBGLEV_ERROR, "Error in  Action Complete Callback -- %d\n", a_event->ErrCode);
*/
		}
		break;
  
		case UPNP_CONTROL_GET_VAR_COMPLETE:
		{
/*
			struct Upnp_State_Var_Complete *sv_event = (struct Upnp_State_Var_Complete * ) pEvent;

			if (sv_event->ErrCode != UPNP_E_SUCCESS)
				DEBUG(DJUPNP, DBGLEV_ERROR, "Error in Get Var Complete Callback -- %d\n", sv_event->ErrCode);
*/
		}
		break;
  
		/* GENA Stuff */
		case UPNP_EVENT_RECEIVED:
		{
/*
			struct Upnp_Event *e_event = (struct Upnp_Event * ) pEvent;

			MSMediaStoreHandleEvent(e_event->Sid, e_event->EventKey, e_event->ChangedVariables);
*/
		}
		break;

		case UPNP_EVENT_SUBSCRIBE_COMPLETE:
		case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		case UPNP_EVENT_RENEWAL_COMPLETE:
		{
/*
			struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) pEvent;

			if (es_event->ErrCode != UPNP_E_SUCCESS)
				DEBUG(DJUPNP, DBGLEV_ERROR, "Error in pEvent Subscribe Callback -- %d\n", es_event->ErrCode);
			else
				MSMediaStoreHandleSubscribeUpdate(es_event->PublisherUrl, es_event->Sid, es_event->TimeOut);
*/
		}
		break;

		case UPNP_EVENT_AUTORENEWAL_FAILED:
		case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
		{
/*
			int TimeOut = DEFAULT_TIMEOUT;
			Upnp_SID newSID;
			int ret;
			struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) pEvent;

			ret=UpnpSubscribe(s_hControlPoint, es_event->PublisherUrl, &TimeOut, newSID);
			if (ret==UPNP_E_SUCCESS)
			{
				DEBUGP(DJUPNP, DBGLEV_INFO, "Subscribed to EventURL with SID=%s\n", newSID);
				MSMediaStoreHandleSubscribeUpdate(es_event->PublisherUrl, newSID, TimeOut);
			}
			else
			{
				DEBUG(DJUPNP, DBGLEV_ERROR, "Error Subscribing to EventURL -- %d\n", ret);
			}
*/
		}
		break;


		/* ignore these cases, since this is not a device */
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		case UPNP_CONTROL_GET_VAR_REQUEST:
		case UPNP_CONTROL_ACTION_REQUEST:
			break;
	}

    return 0;
}

/********************************************************************************
 * DJUPnPControlPointDeleteNode
 *
 * Description: 
 *       Delete a device node from the global device list.  Note that this
 *       function is NOT thread safe, and should be called from another
 *       function that has already locked the global device list.
 *
 * Parameters:
 *   node -- The device node
 *
 ********************************************************************************/
void DJUPnPControlPointDeleteNode(iml_device_t *node) 
{
	int service, var;

	if (!node)
	{
		DEBUG(DJUPNP, DBGLEV_ERROR, "error in DJUPnPControlPointDeleteNode: node is empty\n");
		return;
	}

	for (service = 0; service < IML_SERVICE_SERVCOUNT; ++service)
	{
		/* If we have a valid control SID, then unsubscribe */
		if (strcmp(node->IMLService[service].SID, "") != 0)
		{
			int ret = UpnpUnSubscribe(s_hControlPoint, node->IMLService[service].SID);
			if (ret == UPNP_E_SUCCESS)
				DEBUGP(DJUPNP, DBGLEV_INFO, "Unsubscribed from IML %s EventURL with SID=%s\n", g_szIMLServiceName[service], node->IMLService[service].SID);
			else
				DEBUG(DJUPNP, DBGLEV_ERROR, "Error unsubscribing to IML %s EventURL -- %d\n", g_szIMLServiceName[service], ret);
		}

		for (var = 0; var < IMLVarCount[service]; var++)
		{
			if (node->IMLService[service].VariableStrVal[var]) 
				free(node->IMLService[service].VariableStrVal[var]);
		}
	}

	free(node);
	node = NULL;
}

/********************************************************************************
 * DJUPnPControlPointRemoveDevice
 *
 * Description: 
 *       Remove a device from the global device list.
 *
 * Parameters:
 *   UDN -- The Unique Device Name for the device to remove
 *
 ********************************************************************************/
int DJUPnPControlPointRemoveDevice(const char* UDN) 
{
    int iDeviceNumber = 0;
    bool bFound = false;

    cyg_mutex_lock(&s_mtxDeviceList);

    IMLListIt it = s_slGlobalDeviceList.GetHead();
	while (it != s_slGlobalDeviceList.GetEnd())
    {
	    if (strcmp((*it)->UDN, UDN) == 0)
	    {
            iDeviceNumber = (*it)->iDeviceNumber;
            bFound = true;
		    DJUPnPControlPointDeleteNode(s_slGlobalDeviceList.Remove(it));
            break;
	    }
        ++it;
    }

    cyg_mutex_unlock(&s_mtxDeviceList);

    return bFound ? iDeviceNumber : 0;
}

/********************************************************************************
 * DJUPnPControlPointRemoveDevice
 *
 * Description: 
 *       Remove a device from the global device list.
 *
 * Parameters:
 *   iDeviceNumber -- The device number for the device to remove
 *
 ********************************************************************************/
int DJUPnPControlPointRemoveDevice(int iDeviceNumber) 
{
    bool bFound = false;

    cyg_mutex_lock(&s_mtxDeviceList);

    IMLListIt it = s_slGlobalDeviceList.GetHead();
	while (it != s_slGlobalDeviceList.GetEnd())
    {
	    if (iDeviceNumber == (*it)->iDeviceNumber)
	    {
            bFound = true;
		    DJUPnPControlPointDeleteNode(s_slGlobalDeviceList.Remove(it));
            break;
	    }
        ++it;
    }

    cyg_mutex_unlock(&s_mtxDeviceList);

    if (bFound)
        CEventQueue::GetInstance()->PutEvent(EVENT_IML_BYEBYE, (void*)iDeviceNumber);
    
    return bFound ? iDeviceNumber : 0;
}

/********************************************************************************
 * DJUPnPControlPointRemoveAll
 *
 * Description: 
 *       Remove all devices from the global device list.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
void DJUPnPControlPointRemoveAll() 
{
    cyg_mutex_lock(&s_mtxDeviceList);

    while (!s_slGlobalDeviceList.IsEmpty())
    {
        DJUPnPControlPointDeleteNode(s_slGlobalDeviceList.PopFront());
    }

    cyg_mutex_unlock(&s_mtxDeviceList);
}

/********************************************************************************
 * DJUPnPControlPointRefresh
 *
 * Description: 
 *       Clear the current global device list and issue new search
 *	 requests to build it up again from scratch.
 *
 * Parameters:
 *   None
 *
 ********************************************************************************/
bool
DJUPnPControlPointRefresh(bool bClear) 
{
    int ret;
//    s_bRefreshingDataSources = true;

    if (bClear)
        DJUPnPControlPointRemoveAll();

    /* Search for all iML devices
       waiting for up to 8 seconds for the response */
    ret = UpnpSearchAsync(s_hControlPoint, 8, g_szIMLDeviceType, NULL);

    if (ret != UPNP_E_SUCCESS)
    {
	    DEBUG(DJUPNP, DBGLEV_ERROR, "Error sending search request %d\n", ret);
        return false;
    }

    return true;
}

/********************************************************************************
 * DJUPNPGetDevice
 *
 * Description: 
 *       Given a list number, returns the pointer to the device
 *       node at that position in the global device list.  Note
 *       that this function is not thread safe.  It must be called 
 *       from a function that has locked the global device list.
 *
 * Parameters:
 *   iDeviceNumber -- The number of the device
 *   devnode -- The output device node pointer
 *
 ********************************************************************************/
bool DJUPNPGetDevice(int iDeviceNumber, iml_device_t **devnode) 
{
    IMLListIt it = s_slGlobalDeviceList.GetHead();
	while (it != s_slGlobalDeviceList.GetEnd())
    {
        if ((*it)->iDeviceNumber == iDeviceNumber)
        {
		    *devnode = *it;
		    return true;
        }
        ++it;
    }

	DEBUG(DJUPNP, DBGLEV_ERROR, "Error finding IML device number -- %d\n", iDeviceNumber);
	return false;
}

/********************************************************************************
 * DJUPNPSendAction
 *
 * Description: 
 *       Send an Action request to the specified service of a device.
 *
 * Parameters:
 *   service -- The service
 *   iDeviceNumber -- The number of the device
 *   actionname -- The name of the action.
 *   param_name -- An array of parameter names
 *   param_val -- The corresponding parameter values
 *   param_count -- The number of parameters
 *
 ********************************************************************************/

int DJUPNPSendAction(int service, int iDeviceNumber, char *actionname, char **param_name, char **param_val, int param_count, Upnp_FunPtr pCallbackFunction = 0, const void *pCookie)
{
	iml_device_t *devnode;
	Upnp_Document actionNode=NULL;
	int ret=0;
	int param;

	cyg_mutex_lock(&s_mtxDeviceList);

	if (!DJUPNPGetDevice(iDeviceNumber, &devnode))
	{
		ret = 0;
	}
	else
	{
		if( param_count == 0)
        {
			actionNode = UpnpMakeAction(actionname,g_szIMLServiceType[service],0,NULL);
        }
		else
        {
			for (param = 0; param < param_count; param++)
			{
				if( UpnpAddToAction(&actionNode,actionname,g_szIMLServiceType[service],param_name[param], param_val[param]) != UPNP_E_SUCCESS)
				{
					DEBUG(DJUPNP, DBGLEV_ERROR, "Error in adding action parameter !!!!!!!!!!!!!!!!\n");
                    cyg_mutex_unlock(&s_mtxDeviceList);
					return -1;
				}
			}
        }

		ret = UpnpSendActionAsync( s_hControlPoint, devnode->IMLService[service].ControlURL, g_szIMLServiceType[service],
			devnode->UDN, actionNode, pCallbackFunction, pCookie);
		if (ret != UPNP_E_SUCCESS)
        {
			DEBUG(DJUPNP, DBGLEV_ERROR, "Error in UpnpSendActionAsync -- %d\n", ret);
            UpnpDocument_free(actionNode);
        }
	}

	cyg_mutex_unlock(&s_mtxDeviceList);


	return(ret);
}

