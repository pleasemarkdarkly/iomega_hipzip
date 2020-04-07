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
// $Revision: 1.14 $
// $Date: 2000/08/24 22:23:27 $
//


#include <main/djupnp/DJUPnP.h>
#include <stdlib.h>         // malloc

#include <util/debug/debug.h>
#include <util/upnp/api/upnptools.h>

DEBUG_MODULE_S(SAMPLEUTIL, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(SAMPLEUTIL);


/********************************************************************************
 * SampleUtil_GetElementValue
 *
 * Description: 
 *       Given a DOM node such as <Channel>11</Channel>, this routine
 *       extracts the value (e.g., 11) from the node and returns it as 
 *       a string.
 *
 * Parameters:
 *   node -- The DOM node from which to extract the value
 *
 ********************************************************************************/
char* SampleUtil_GetElementValue(Upnp_Node node)
{
    Upnp_Node child = UpnpNode_getFirstChild(node);
    Upnp_DOMException err;
    char * temp;


    if ( (child!=0) && (UpnpNode_getNodeType(child)==TEXT_NODE))
    {
        temp= (char *) UpnpNode_getNodeValue(child,&err);
        UpnpNode_free(child);

        return temp;
    }
    else
    {

        return NULL;
    }
}

/********************************************************************************
 * SampleUtil_GetFirstServiceList
 *
 * Description: 
 *       Given a DOM node representing a UPnP Device Description Document,
 *       this routine parses the document and finds the first service list
 *       (i.e., the service list for the root device).  The service list
 *       is returned as a DOM node list.
 *
 * Parameters:
 *   node -- The DOM node from which to extract the service list
 *
 ********************************************************************************/
Upnp_NodeList SampleUtil_GetFirstServiceList(Upnp_Node node) 
{
    Upnp_NodeList ServiceList=NULL;
    Upnp_NodeList servlistnodelist=NULL;
    Upnp_Node servlistnode=NULL;

    servlistnodelist = UpnpDocument_getElementsByTagName(node, "serviceList");
    if(servlistnodelist && UpnpNodeList_getLength(servlistnodelist)) {

        /* we only care about the first service list, from the root device */
        servlistnode = UpnpNodeList_item(servlistnodelist, 0);

        /* create as list of DOM nodes */
        ServiceList = UpnpElement_getElementsByTagName(servlistnode, "service");
    }

    if (servlistnodelist) UpnpNodeList_free(servlistnodelist);
    if (servlistnode) UpnpNode_free(servlistnode);

    return ServiceList;
}

/********************************************************************************
 * SampleUtil_GetFirstDocumentItem
 *
 * Description: 
 *       Given a DOM node, this routine searches for the first element
 *       named by the input string item, and returns its value as a string.
 *       The string returned must be released by calling Upnpfree() or delete [].
 *
 * Parameters:
 *   node -- The DOM node from which to extract the value
 *   item -- The item to search for
 *
 ********************************************************************************/
char* SampleUtil_GetFirstDocumentItem(Upnp_Node node, char *item) 
{
	Upnp_NodeList NodeList=NULL;
	Upnp_Node textNode=NULL;
	Upnp_Node tmpNode=NULL;
	Upnp_DOMException err; 
	char *ret=NULL;

	NodeList = UpnpDocument_getElementsByTagName(node, item);
	if (NodeList == NULL)
	{
		DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error finding %s in XML Node\n", item);
	}
	else
	{
		if ((tmpNode = UpnpNodeList_item(NodeList, 0)) == NULL)
		{
			DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error finding %s value in XML Node\n", item);
		}
		else
		{
			textNode = UpnpNode_getFirstChild(tmpNode);
			if (textNode)
			{
				Upnp_DOMString NodeValue = UpnpNode_getNodeValue(textNode, &err);
				if (err != NO_ERR)
				{
					DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error getting node value for %s in XML Node\n", item);
					if (NodeList)
						UpnpNodeList_free(NodeList);
					if (tmpNode)
						UpnpNode_free(tmpNode);
					if (textNode)
						UpnpNode_free(textNode);
					return ret;
				}
				if (NodeValue)
				{
					ret = NodeValue;
				}
				else
				{
					ret = new char[1];
					if (!ret)
						DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error allocating memory for %s in XML Node\n", item);
					else
						ret[0] = '\0';
					Upnpfree(NodeValue);
				}
			}
		}
	}
	if (NodeList)
		UpnpNodeList_free(NodeList);
	if (tmpNode)
		UpnpNode_free(tmpNode);
	if (textNode)
		UpnpNode_free(textNode);
	return ret;
}

/********************************************************************************
 * SampleUtil_GetFirstElementItem
 *
 * Description: 
 *       Given a DOM element, this routine searches for the first element
 *       named by the input string item, and returns its value as a string.
 *
 * Parameters:
 *   node -- The DOM node from which to extract the value
 *   item -- The item to search for
 *
 ********************************************************************************/
char* SampleUtil_GetFirstElementItem(Upnp_Element node, char *item) 
{
	Upnp_NodeList NodeList=NULL;
	Upnp_Node textNode=NULL;
	Upnp_Node tmpNode=NULL;
	Upnp_DOMException err; 
	char *ret=NULL;

	NodeList = UpnpElement_getElementsByTagName(node, item);
	if (NodeList == NULL)
	{
		DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error finding %s in XML Node\n", item);
	}
	else
	{
		if ((tmpNode = UpnpNodeList_item(NodeList, 0)) == NULL)
		{
			DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error finding %s value in XML Node\n", item);
		}
		else
		{
			textNode = UpnpNode_getFirstChild(tmpNode);
			Upnp_DOMString NodeValue = 0;
			if (textNode)
			{
				NodeValue = UpnpNode_getNodeValue(textNode, &err);
				if (err != NO_ERR)
				{
					DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error getting node value for %s in XML Node\n", item);
					if (NodeList)
						UpnpNodeList_free(NodeList);
					if (tmpNode)
						UpnpNode_free(tmpNode);
					if (textNode)
						UpnpNode_free(textNode);
					return ret;
				}
			}
			if (NodeValue)
			{
                ret = NodeValue;
			}
			else
			{
				ret = new char[1];
				if (!ret)
					DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error allocating memory for %s in XML Node\n", item);
				else
					ret[0] = '\0';
			}
		}
	}
	if (NodeList)
		UpnpNodeList_free(NodeList);
	if (tmpNode)
		UpnpNode_free(tmpNode);
	if (textNode)
		UpnpNode_free(textNode);
	return ret;
}



/********************************************************************************
 * SampleUtil_PrintEventType
 *
 * Description: 
 *       Prints a callback event type as a string.
 *
 * Parameters:
 *   S -- The callback event
 *
 ********************************************************************************/
void SampleUtil_PrintEventType(Upnp_EventType S)
{
    switch(S) {

    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_DISCOVERY_ADVERTISEMENT_ALIVE\n");
	break;
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE\n");
	break;
    case UPNP_DISCOVERY_SEARCH_RESULT:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_DISCOVERY_SEARCH_RESULT\n");
	break;
    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_DISCOVERY_SEARCH_TIMEOUT\n");
	break;


	/* SOAP Stuff */
    case UPNP_CONTROL_ACTION_REQUEST:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_CONTROL_ACTION_REQUEST\n");
	break;
    case UPNP_CONTROL_ACTION_COMPLETE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_CONTROL_ACTION_COMPLETE\n");
	break;
    case UPNP_CONTROL_GET_VAR_REQUEST:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_CONTROL_GET_VAR_REQUEST\n");
	break;
    case UPNP_CONTROL_GET_VAR_COMPLETE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_CONTROL_GET_VAR_COMPLETE\n");
	break;

	/* GENA Stuff */
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_SUBSCRIPTION_REQUEST\n");
	break;
    case UPNP_EVENT_RECEIVED:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_RECEIVED\n");
	break;
    case UPNP_EVENT_RENEWAL_COMPLETE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_RENEWAL_COMPLETE\n");
	break;
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_SUBSCRIBE_COMPLETE\n");
	break;
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_UNSUBSCRIBE_COMPLETE\n");
	break;

    case UPNP_EVENT_AUTORENEWAL_FAILED:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_AUTORENEWAL_FAILED\n");
	break;
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UPNP_EVENT_SUBSCRIPTION_EXPIRED\n");
	break;

    }
}

/********************************************************************************
 * SampleUtil_PrintEvent
 *
 * Description: 
 *       Prints callback event structure details.
 *
 * Parameters:
 *   EventType -- The type of callback event
 *   Event -- The callback event structure
 *
 ********************************************************************************/
int SampleUtil_PrintEvent(Upnp_EventType EventType, 
	       void *Event)
{
#if 0

    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "\n\n\n======================================================================\n");
    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "----------------------------------------------------------------------\n");
    SampleUtil_PrintEventType(EventType);
  
    switch ( EventType) {
      
	/* SSDP Stuff */
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
	struct Upnp_Discovery *d_event = (struct Upnp_Discovery * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",d_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Expires     =  %d\n",d_event->Expires);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "DeviceId    =  %s\n",d_event->DeviceId); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "DeviceType  =  %s\n",d_event->DeviceType);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ServiceType =  %s\n",d_event->ServiceType);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ServiceVer  =  %s\n",d_event->ServiceVer);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Location    =  %s\n",d_event->Location);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "OS          =  %s\n",d_event->Os);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Ext         =  %s\n",d_event->Ext);
	
    }
    break;
      
    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
	// Nothing to print out here
	break;

    /* SOAP Stuff */
    case UPNP_CONTROL_ACTION_REQUEST:
    {
	struct Upnp_Action_Request *a_event = (struct Upnp_Action_Request * ) Event;
	char *xmlbuff=NULL;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",a_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrStr      =  %s\n",a_event->ErrStr); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActionName  =  %s\n",a_event->ActionName); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UDN         =  %s\n",a_event->DevUDN);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ServiceID   =  %s\n",a_event->ServiceID);
	if (a_event->ActionRequest) {
	    xmlbuff = UpnpNewPrintDocument(a_event->ActionRequest);
	    if (xmlbuff) DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActRequest  =  %s\n",xmlbuff);
	    if (xmlbuff) free(xmlbuff);
	    xmlbuff=NULL;
	} else {
	    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActRequest  =  (null)\n");
	}
	if (a_event->ActionResult) {
	    xmlbuff = UpnpNewPrintDocument(a_event->ActionResult);
	    if (xmlbuff) DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActResult   =  %s\n",xmlbuff);
	    if (xmlbuff) free(xmlbuff);
	    xmlbuff=NULL;
	} else {
	    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActResult   =  (null)\n");
	}
    }
    break;
      
    case UPNP_CONTROL_ACTION_COMPLETE:
    {
	struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
	char *xmlbuff=NULL;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",a_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "CtrlUrl     =  %s\n",a_event->CtrlUrl);
	if (a_event->ActionRequest) {
	    xmlbuff = UpnpNewPrintDocument(a_event->ActionRequest);
	    if (xmlbuff) DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActRequest  =  %s\n",xmlbuff);
	    if (xmlbuff) free(xmlbuff);
	    xmlbuff=NULL;
	} else {
	    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActRequest  =  (null)\n");
	}
	if (a_event->ActionResult)
	{
/*
	    xmlbuff = UpnpNewPrintDocument(a_event->ActionResult);
	    if (xmlbuff) DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActResult   =  %s\n",xmlbuff);
	    if (xmlbuff) free(xmlbuff);
	    xmlbuff=NULL;
*/
	}
	else {
	    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ActResult   =  (null)\n");
	}
    }
    break;
      
    case UPNP_CONTROL_GET_VAR_REQUEST:
    {
	struct Upnp_State_Var_Request *sv_event = (struct Upnp_State_Var_Request * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",sv_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrStr      =  %s\n",sv_event->ErrStr); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UDN         =  %s\n",sv_event->DevUDN); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ServiceID   =  %s\n",sv_event->ServiceID); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "StateVarName=  %s\n",sv_event->StateVarName); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "CurrentVal  =  %s\n",sv_event->CurrentVal);
    }
    break;
      
    case UPNP_CONTROL_GET_VAR_COMPLETE:
    {
	struct Upnp_State_Var_Complete *sv_event = (struct Upnp_State_Var_Complete * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",sv_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "CtrlUrl     =  %s\n",sv_event->CtrlUrl); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "StateVarName=  %s\n",sv_event->StateVarName); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "CurrentVal  =  %s\n",sv_event->CurrentVal);
    }
    break;
      
    /* GENA Stuff */
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    {
	struct Upnp_Subscription_Request *sr_event = (struct Upnp_Subscription_Request * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ServiceID   =  %s\n",sr_event->ServiceId);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "UDN         =  %s\n",sr_event->UDN); 
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "SID         =  %s\n",sr_event->Sid);
    }
    break;
      
    case UPNP_EVENT_RECEIVED:
    {
	struct Upnp_Event *e_event = (struct Upnp_Event * ) Event;
	char *xmlbuff=NULL;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "SID         =  %s\n",e_event->Sid);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "EventKey    =  %d\n",e_event->EventKey);
	xmlbuff = UpnpNewPrintDocument(e_event->ChangedVariables);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ChangedVars =  %s\n",xmlbuff);
	free(xmlbuff);
	xmlbuff=NULL;
    }
    break;

    case UPNP_EVENT_RENEWAL_COMPLETE:
    {
	struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "SID         =  %s\n",es_event->Sid);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",es_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "TimeOut     =  %d\n",es_event->TimeOut);
    }
    break;

    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
    {
	struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "SID         =  %s\n",es_event->Sid);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",es_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "PublisherURL=  %s\n",es_event->PublisherUrl);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "TimeOut     =  %d\n",es_event->TimeOut);
    }
    break;

    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
    {
	struct Upnp_Event_Subscribe *es_event = (struct Upnp_Event_Subscribe * ) Event;
        
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "SID         =  %s\n",es_event->Sid);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "ErrCode     =  %d\n",es_event->ErrCode);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "PublisherURL=  %s\n",es_event->PublisherUrl);
	DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "TimeOut     =  %d\n",es_event->TimeOut);
    }
    break;



    }
    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "----------------------------------------------------------------------\n");
    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "======================================================================\n\n\n\n");

#endif	// DEBUG

    return(0);
}




/********************************************************************************
 * SampleUtil_FindAndParseService
 *
 * Description: 
 *       This routine finds the first occurance of a service in a DOM representation
 *       of a description document and parses it.  Note that this function currently
 *       assumes that the eventURL and controlURL values in the service definitions
 *       are full URLs.  Relative URLs are not handled here.
 *
 * Parameters:
 *   DescDoc -- The DOM description document
 *   location -- The location of the description document
 *   serviceSearchType -- The type of service to search for
 *   serviceId -- OUT -- The service ID
 *   eventURL -- OUT -- The event URL for the service
 *   controlURL -- OUT -- The control URL for the service
 *
 ********************************************************************************/
int SampleUtil_FindAndParseService (Upnp_Document DescDoc, char* location, char *serviceSearchType, char **serviceId, char **eventURL, char **controlURL) 
{
    int i, length, found=0;
    int ret;
    char *serviceType=NULL;
    char *baseURL=NULL;
    char *base;
    char *relcontrolURL=NULL, *releventURL=NULL;
    Upnp_NodeList serviceList=NULL;
    Upnp_Node service=NULL;

    baseURL = SampleUtil_GetFirstDocumentItem(DescDoc, "URLBase");

    if (baseURL) 
        base = baseURL;
    else
        base = location;

    serviceList = SampleUtil_GetFirstServiceList(DescDoc);
    length = UpnpNodeList_getLength(serviceList);
    for (i=0;i<length;i++) { 
        service = UpnpNodeList_item(serviceList, i);
        serviceType = SampleUtil_GetFirstElementItem(service, "serviceType");
        if (strcmp(serviceType, serviceSearchType) == 0) {
            DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Found service: %s\n", serviceType);
            *serviceId = SampleUtil_GetFirstElementItem(service, "serviceId");

            relcontrolURL = SampleUtil_GetFirstElementItem(service, "controlURL");
            releventURL = SampleUtil_GetFirstElementItem(service, "eventSubURL");

            *controlURL = new char[strlen(base) + strlen(relcontrolURL) + 1];
            if (*controlURL) {
                ret = UpnpResolveURL(base, relcontrolURL, *controlURL);
                if (ret!=UPNP_E_SUCCESS)
                    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error generating controlURL from %s + %s\n", base, relcontrolURL);
            }

            *eventURL = new char[strlen(base) + strlen(releventURL) + 1];
            if (*eventURL) {
                ret = UpnpResolveURL(base, releventURL, *eventURL);
                if (ret!=UPNP_E_SUCCESS)
                    DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "Error generating eventURL from %s + %s\n", base, releventURL);
            }

            if (relcontrolURL) Upnpfree(relcontrolURL);
            if (releventURL) Upnpfree(releventURL);
            relcontrolURL = releventURL = NULL;

            found=1;
            break;
        }
        if (service) UpnpNode_free(service);
        service=NULL;
        if (serviceType) Upnpfree(serviceType);
        serviceType=NULL;
    }

    if (service) UpnpNode_free(service);
    if (serviceType) Upnpfree(serviceType);
    if (serviceList) UpnpNodeList_free(serviceList);
    if (baseURL) Upnpfree(baseURL);

    return(found);
}


/********************************************************************************
 * SampleUtil_PrintMetadata
 *
 ********************************************************************************/
void
SampleUtil_PrintMetadata(Upnp_Node node)
{
	Upnp_NodeList NodeList=NULL;
	Upnp_Node textNode=NULL;
	Upnp_Node tmpNode=NULL;
	Upnp_DOMException err; 
	char *ret=NULL;
	int len;

/*
	NodeList = UpnpElement_getElementsByTagName(node, "metadata");
	if (NodeList)
	{
		if ((tmpNode = UpnpNodeList_item(NodeList, 0)) != NULL)
		{
*/
			Upnp_NodeList ChildNodeList = UpnpNode_getChildNodes(node);
			if (ChildNodeList)
			{
				int cItems = UpnpNodeList_getLength(ChildNodeList);
				for (int j = 0; j < cItems; ++j)
				{
					textNode = UpnpNodeList_item(ChildNodeList, j);
					if (textNode)
					{
						Upnp_DOMString NodeName = UpnpNode_getNodeName(textNode);
						Upnp_DOMString NodeValue = SampleUtil_GetElementValue(textNode);
//						DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "    %s: %s\n", UpnpNode_getNodeName(textNode), SampleUtil_GetElementValue(textNode));
						DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "    %s: %s\n", NodeName, NodeValue);
						UpnpNode_free(textNode);
						Upnpfree(NodeName);
						Upnpfree(NodeValue);
					}
				}
				UpnpNodeList_free(ChildNodeList);
			}
/*
			UpnpNode_free(tmpNode);
		}
		UpnpNodeList_free(NodeList);
	}
*/
}

/********************************************************************************
 * SampleUtil_PrintNodes
 *
 ********************************************************************************/
void SampleUtil_PrintNodes(Upnp_Node topNode, int iLevel)
{
	char szIndent[50] = "\0";
	int i;

	for (i = 0; i < iLevel; ++i)
	{
		strcat(szIndent, " ");
	}

	Upnp_DOMString szNodeName = UpnpNode_getNodeName(topNode);
	if (szNodeName)
	{
		DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "%sNode: %s\n", szIndent, szNodeName);
		Upnpfree(szNodeName);
	}
	Upnp_DOMException blahErr;
	Upnp_DOMString szNodeValue = UpnpNode_getNodeValue(topNode, &blahErr);
	if (blahErr == NO_ERR)
	{
		DEBUGP(SAMPLEUTIL, DBGLEV_INFO, "%sValue: %s\n", szIndent, szNodeValue);
		Upnpfree(szNodeValue);
	}

	Upnp_NodeList NodeList = UpnpNode_getChildNodes(topNode);
	if (NodeList)
	{
		int cBlah = UpnpNodeList_getLength(NodeList);
		for (int j = 0; j < cBlah; ++j)
		{
			Upnp_Node BlahNode = UpnpNodeList_item(NodeList, j);
			if (BlahNode)
			{
				SampleUtil_PrintNodes(BlahNode, iLevel + 1);
				UpnpNode_free(BlahNode);
			}
		}

		UpnpNodeList_free(NodeList);
	}
}


/********************************************************************************
 * SampleUtil_FindChildByName
 *
 ********************************************************************************/
Upnp_Node SampleUtil_FindChildByName(Upnp_Node topNode, const char *szName)
{
	Upnp_DOMString szNodeName = UpnpNode_getNodeName(topNode);
	if (szNodeName)
	{
		if (!strcmp(szNodeName, szName))
		{
			Upnpfree(szNodeName);
			return topNode;
		}
        Upnpfree(szNodeName);
	}

	Upnp_NodeList NodeList = UpnpNode_getChildNodes(topNode);
	if (NodeList)
	{
		int cBlah = UpnpNodeList_getLength(NodeList);
		for (int j = 0; j < cBlah; ++j)
		{
			Upnp_Node BlahNode = UpnpNodeList_item(NodeList, j);
			if (BlahNode)
			{
				Upnp_DOMString szNodeName = UpnpNode_getNodeName(BlahNode);
				if (szNodeName)
				{
					if (!strcmp(szNodeName, szName))
					{
						Upnpfree(szNodeName);
						UpnpNodeList_free(NodeList);
						return BlahNode;
					}
					Upnpfree(szNodeName);
				}
				Upnp_Node foundNode = SampleUtil_FindChildByName(BlahNode, szName);
				UpnpNode_free(BlahNode);
				if (foundNode)
				{
					UpnpNodeList_free(NodeList);
					return foundNode;
				}
			}
		}
		UpnpNodeList_free(NodeList);
	}
	return 0;
}
