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
// $Revision: 1.106 $
// $Date: 2000/09/13 14:56:08 $
//

#ifndef UPNP_H
#define UPNP_H

/**@name UPnP SDK v1.0 for Linux

 \begin{center}
   {\bf Universal Plug and Play Software Development Kit Version
     1.0 for Linux}

   Revision 1.0.0 (\Date)
 \end{center}
 
 This document gives a brief description of the Universal Plug and
 Play Software Development Kit for Linux API.  Section 1 covers the
 license that the UPnP SDK is distributed under.  Section 2 talks
 about the callback functions used in many parts of the SDK.  Finally,
 section 3 details the structures and functions that comprise the API.

*/

//@{

/**@name License

 Copyright (C) 2000 Intel Corporation 
 All rights reserved. 

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met: 

 \begin{itemize}
 \item Redistributions of source code must retain the above copyright 
 notice, this list of conditions and the following disclaimer. 

 \item Redistributions in binary form must reproduce the above copyright 
 notice, this list of conditions and the following disclaimer in the 
 documentation and/or other materials provided with the distribution. 

 \item Neither name of Intel Corporation nor the names of its 
 contributors may be used to endorse or promote products derived from 
 this software without specific prior written permission.

 \end{itemize}
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** @name About Callbacks

 The UPnP SDK contains functions that generate asynchronous callbacks.
 To simplify the application callback functions, these callbacks
 are executed on a thread owned by the UPnP SDK library itself.
 The UPnP library executes the application's callback function in a thread 
 context so the application can allocate memory and preserve the information it
 needs. The application can also use standard thread synchronization methods 
 to ensure data integrity.  Due to the possibility of deadlock, the 
 application cannot call back into the UPnP library during these callbacks 
 unless explicitly noted.  There is no restriction in calling into the 
 operating system or any other application interface.
*/

/** @name The API */

//@{

#include <stdio.h>
#include <util/upnp/upnpdom/domCif.h>
#include <util/upnp/api/config.h>

#define NUM_HANDLE 200
#define LINE_SIZE  180
#define NAME_SIZE  256
#define MNFT_NAME_SIZE  64
#define MODL_NAME_SIZE  32
#define SERL_NUMR_SIZE  64
#define MODL_DESC_SIZE  64
#define UPNP_INFINITE -1 

#define UPNP_E_SUCCESS          0
#define UPNP_E_INVALID_HANDLE   -100
#define UPNP_E_INVALID_PARAM    -101
#define UPNP_E_OUTOF_HANDLE     -102
#define UPNP_E_OUTOF_CONTEXT    -103
#define UPNP_E_OUTOF_MEMORY     -104
#define UPNP_E_INIT             -105
#define UPNP_E_BUFFER_TOO_SMALL -106
#define UPNP_E_INVALID_DESC     -107
#define UPNP_E_INVALID_URL      -108
#define UPNP_E_INVALID_SID      -109
#define UPNP_E_INVALID_DEVICE   -110
#define UPNP_E_INVALID_SERVICE  -111
#define UPNP_E_BAD_RESPONSE     -113
#define UPNP_E_BAD_REQUEST      -114
#define UPNP_E_INVALID_ACTION   -115
#define UPNP_E_FINISH           -116
#define UPNP_E_INIT_FAILED      -117
#define UPNP_E_URL_TOO_BIG      -118

#define UPNP_E_NETWORK_ERROR    -200 
#define UPNP_E_SOCKET_WRITE     -201
#define UPNP_E_SOCKET_READ      -202
#define UPNP_E_SOCKET_BIND      -203
#define UPNP_E_SOCKET_CONNECT   -204
#define UPNP_E_OUTOF_SOCKET     -205
#define UPNP_E_LISTEN           -206

#define UPNP_E_EVENT_PROTOCOL         -300
#define UPNP_E_SUBSCRIBE_UNACCEPTED   -301
#define UPNP_E_UNSUBSCRIBE_UNACCEPTED -302
#define UPNP_E_NOTIFY_UNACCEPTED      -303

#define UPNP_E_INVALID_ARGUMENT       -501
#define UPNP_E_FILE_NOT_FOUND         -502
#define UPNP_E_FILE_READ_ERROR        -503
#define UPNP_E_EXT_NOT_XML            -504
#define UPNP_E_NO_WEB_SERVER          -505

#define UPNP_E_INTERNAL_ERROR         -911

#ifndef OUT
#define OUT
#endif 

#ifndef IN 
#define IN
#endif

#ifndef INOUT
#define INOUT
#endif

/// @name Constants, Structures, and Types
//@{

/** Returned when a control point application registers with {\bf
 *   UpnpRegisterClient}.  Client handles can only be used with UPnP
 *   functions that operate with a client handle.  */

typedef int  UpnpClient_Handle;

/** Returned when a device application registers with {\bf
 * UpnpRegisterRootDevice}.  Device handles can only be used with UPnP
 * functions that operate with a device handle.  */

typedef int  UpnpDevice_Handle;

/** The reason code for an event callback. */

enum Upnp_EventType_e {

  //
  // Control callbacks
  //

  /** Received by a device when a control point issues a control
   *  request.  The {\bf Event} parameter contains a pointer to a {\bf
   *  Upnp_Action_Request} structure containing the action.  The application
   *  stores the results of the action in this structure. */

  UPNP_CONTROL_ACTION_REQUEST,

  /** A {\bf UpnpSendActionAsync} call completed. The {\bf Event}
   *  parameter contains a pointer to a {\bf Upnp_Action_Complete} structure
   *  with the results of the action.  */

  UPNP_CONTROL_ACTION_COMPLETE,

  /** Received by a device when a query for a single service variable
   *  arrives.  The {\bf Event} parameter contains a pointer to a {\bf
   *  Upnp_State_Var_Request} structure containing the name of the variable
   *  and value.  */

  UPNP_CONTROL_GET_VAR_REQUEST,


  /** A {\bf UpnpGetServiceVarStatus} call completed. The {\bf Event}
   *  parameter contains a pointer to a {\bf Upnp_State_Var_Complete} structure
   *  containing the value for the variable.  */

  UPNP_CONTROL_GET_VAR_COMPLETE,

  //
  // Discovery callbacks
  //

  /** Received by a control point when a new device or service is available.  
   *  The {\bf Event} parameter contains a pointer to a {\bf
   *  Upnp_Discovery} structure with the information about the device
   *  or service.  */

  UPNP_DISCOVERY_ADVERTISEMENT_ALIVE,

  /** Received by a control point when a device or service shuts down. The {\bf
   *  Event} parameter contains a pointer to a {\bf Upnp_Discovery}
   *  structure containing the information about the device or
   *  service.  */

  UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE,

  /** Recevied by a control point when a matching device or service responds.
   *  The {\bf Event} parameter contains a pointer to a {\bf
   *  Upnp_Discovery} structure containing the information about
   *  the reply to the search request.  */

  UPNP_DISCOVERY_SEARCH_RESULT,

  /** Received by a control point when the search timeout expires.  The
   *  UPnP library  generates no more callbacks for this search after this 
   *  event.  The {\bf Event} parameter is {\tt NULL}.  */

  UPNP_DISCOVERY_SEARCH_TIMEOUT,

  //
  // Eventing callbacks
  //

  /** Received by a device when a subscription arrives.
   *  The {\bf Event} parameter contains a pointer to a {\bf
   *  Upnp_Subscription_Request} structure.  At this point, the
   *  subscription has already been accepted.  {\bf UpnpAcceptSubscription}
   *  needs to be called to confirm the subscription and transmit the
   *  initial state table.  This can be done during this callback.  The UPnP
   *  library generates no events for a subscription unless the device 
   *  application calls {\bf UpnpAcceptSubscription}.
   */

  UPNP_EVENT_SUBSCRIPTION_REQUEST,

  /** Received by a control point when an event arrives.  The {\bf
   *  Event} parameter contains a {\bf Upnp_Event} structure
   *  with the information about the event.  */

  UPNP_EVENT_RECEIVED,

  /** A {\bf UpnpRenewSubscriptionAsync} call completed. The status of
   *  the renewal is in the {\bf Event} parameter as a {\bf
   *  Upnp_Event_Subscription} structure.  */

  UPNP_EVENT_RENEWAL_COMPLETE,

  /** A {\bf UpnpSubscribeAsync} call completed. The status of the
   * subscription is in the {\bf Event} parameter as a {\bf
   * Upnp_Event_Subscription} structure.  */

  UPNP_EVENT_SUBSCRIBE_COMPLETE,

  /** A {\bf UpnpUnSubscribeAsync} call completed. The status of the
   *  subscription is in the {\bf Event} parameter as a {\bf
   *  Upnp_Event_Subscribe} structure.  */

  UPNP_EVENT_UNSUBSCRIBE_COMPLETE,

  /** The auto-renewal of a client subscription failed.   
   *  The {\bf Event} parameter is a  
   *  {\bf Upnp_Event_Subscribe} structure with the error 
   *  code set appropriately. The subscription is no longer 
   *  valid. */

  UPNP_EVENT_AUTORENEWAL_FAILED,

  /** A client subscription has expired. This will only occur 
   *  if auto-renewal of subscriptions is disabled.
   *  The {\bf Event} parameter is a {\bf Upnp_Event_Subscribe}
   *  structure. The subscription is no longer valid. */
  
  UPNP_EVENT_SUBSCRIPTION_EXPIRED

};

typedef enum Upnp_EventType_e Upnp_EventType;

typedef char Upnp_SID[42];

/** Represents the different types of searches that
 *   can be performed using the UPnP API.  */

enum Upnp_SType_e {

  /** Search for all devices and services on the network. */
  UPNP_S_ALL,    

  /** Search for all root devices on the network. */
  UPNP_S_ROOT,   

  /** Search for a particular device type or a particular device
      instance. */
  UPNP_S_DEVICE, 
                       
  /** Search for a particular service type, possibly on a particular
   *  device type or device instance.  */
  UPNP_S_SERVICE 
                       
};

typedef enum Upnp_SType_e Upnp_SType;

/** Specifies the type of description in UpnpRegisterRootDevice(). */
enum Upnp_DescType_e { 

	/** The description is a URL. */
	UPNPREG_URL_DESC, 
	
	/** The description is a file name. */
	UPNPREG_FILENAME_DESC,
    
	/** The description is a pointer to a char array. */
	UPNPREG_BUF_DESC 

};

typedef enum Upnp_DescType_e Upnp_DescType;


/** Returned as part of a {\bf UPNP_CONTROL_ACTION_COMPLETE} callback.  */

struct Upnp_Action_Request
{
  /** The result of the operation. */
  int ErrCode;

  int Socket;

  /** The error string in case of error */
  char ErrStr[LINE_SIZE];

 /** The Action Name. */
  char ActionName[NAME_SIZE];

  /** The unique device ID. */
  char DevUDN[NAME_SIZE];

  /** The  service ID. */
  char ServiceID[NAME_SIZE];

  /** The DOM document describing the action. */
  Upnp_Document ActionRequest;

  /** The DOM document describing the result of the action. */
  Upnp_Document ActionResult;

};

struct Upnp_Action_Complete
{
  /** The result of the operation. */
  int ErrCode;

  /** The control URL for service. */
  char CtrlUrl[NAME_SIZE];

  /** The DOM document describing the action. */
  Upnp_Document ActionRequest;

  /** The DOM document describing the result of the action. */
  Upnp_Document ActionResult;

};

/** Represents the request for current value of a state variable in a service
 *  state table.  */

struct Upnp_State_Var_Request
{
  /** The result of the operation. */
  int ErrCode;

  int Socket;


  /** The error string in case of error. */
  char ErrStr[LINE_SIZE];

  /** The unique device ID. */
  char DevUDN[NAME_SIZE];

  /** The  service ID. */
  char ServiceID[NAME_SIZE];

  /** The name of the variable. */
  char StateVarName[NAME_SIZE];

  /** The current value of the variable. This needs to be allocated by 
   *  the caller.  When finished with it, the UPnP library frees this 
   *  DOMString. */
  Upnp_DOMString CurrentVal;
};

/** Represents the reply for the current value of a state variable in an
    asynchronous call. */

struct Upnp_State_Var_Complete
{
  /** The result of the operation. */
  int ErrCode;

 /** The control URL for the service. */
  char CtrlUrl[NAME_SIZE];

  /** The name of the variable.*/
  char StateVarName[NAME_SIZE];

  /** The current value of the variable or error string in case of error.*/
  Upnp_DOMString CurrentVal;
};

/** Returned along with a {\bf UPNP_EVENT_RECEIVED} callback.  */

struct Upnp_Event
{
  /** The subscription ID for this subscription. */
  Upnp_SID Sid;

  /** The event sequence number. */
  int EventKey;

  /** The DOM tree representing the changes generating the event. */
  Upnp_Document ChangedVariables;

};

//
// This typedef is required by Doc++ to parse the last entry of the 
// Upnp_Discovery structure correctly.
//

typedef struct sockaddr_in SOCKADDRIN;

/** Returned in a {\bf UPNP_DISCOVERY_RESULT} callback. */

struct Upnp_Discovery
{

  /** The result code of the {\bf UpnpSearchAsync} call. */
  int  ErrCode;                  
				     
  /** The expiration time of the advertisement. */
  int  Expires;                  
                                     
  /** The unique device identifier. */
  char DeviceId[LINE_SIZE];      

  /** The device type. */
  char DeviceType[LINE_SIZE];    

  /** The service type. */
  char ServiceType[LINE_SIZE];

  /** The service version. */
  char ServiceVer[LINE_SIZE];    

  /** The URL to the UPnP description document for the device. */
  char Location[LINE_SIZE];      

  /** The operating system the device is running. */
  char Os[LINE_SIZE];            
				     
  /** Date when the response was generated */
  char Date[LINE_SIZE];            
				     
  /** Confirmation that the MAN header was understood by the device. */
  char Ext[LINE_SIZE];           
				     
  /** The host address of the device responding to the search. */
  SOCKADDRIN * DestAddr; 

};

/** Returned along with a {\bf UPNP_EVENT_RENEWAL_COMPLETE} callback.  */

struct Upnp_Event_Renewal {
  
  /** The subscription ID for the renewal. */
   Upnp_SID Sid; 

  /** The result code of the renewal. */
  int ErrCode;    
};

/** Returned along with a {\bf UPNP_EVENT_SUBSCRIBE_COMPLETE} or {\bf
 * UPNP_EVENT_UNSUBSCRIBE_COMPLETE} callback.  */

struct Upnp_Event_Subscribe {

  /** The SID for this subscription.  For subscriptions, this only
   *  contains a valid SID if the {\tt Upnp_EventSubscribe.result} field
   *  contains a {\tt UPNP_E_SUCCESS} result code.  For unsubscriptions,
   *  this contains the SID from which the subscription is being
   *  unsubscribed.  */

  Upnp_SID Sid;            

  /** The result of the operation. */
  int ErrCode;              

  /** The event URL being subscribed to or removed from. */
  char PublisherUrl[NAME_SIZE]; 

  /** The actual subscription time (for subscriptions only). */
  int TimeOut;              
                              
};
  
/** Returned along with a {\bf UPNP_EVENT_SUBSCRIPTION_REQUEST}
 * callback.  */

struct Upnp_Subscription_Request
{
  /** The identifier for the service being subscribed to. */
  char *ServiceId; 

  /** Universal device name. */
  char *UDN;       

  /** The assigned subscription ID for this subscription. */
  Upnp_SID Sid;

};

/** All callback functions share the same prototype, documented below.
 *   Note that any memory passed to the callback function
 *   is valid only during the callback and should be copied if it
 *   needs to persist.  This callback function needs to be thread
 *   safe.  The context of the callback is always on a valid thread 
 *   context and standard synchronization methods can be used.  Note, 
 *   however, because of this the callback cannot call UPnP library functions
 *   unless explicitly noted.
 *
 *   \begin{verbatim}
       int CallbackFxn( Upnp_EventType EventType, void* Event, void* Cookie );
     \end{verbatim} 
 *
 * where {\bf EventType} is the event that triggered the callback, 
 * {\bf Event} is a structure that denotes event-specific information for that
 * event, and {\bf Cookie} is the user data passed when the callback was
 * registered.
 *
 * See {\bf Upnp_EventType} for more information on the callback values and
 * the assoicated {\bf Event} parameter.  
 *
 * The return value of the callback is currently ignored.  It may be used
 * in the future to communicate results back to the UPnP library.
 */

typedef int  (*Upnp_FunPtr) (
    IN Upnp_EventType EventType, 
    IN void *Event, 
    IN void *Cookie
    );

//@} // Constants, Structures, and Types

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///@name Initialization and Registration
//@{
/** Initializes the UPnP Software Development Kit. This function must be called
 *  before any other API function can be called.  It should be called
 *  only once.  Subsequent calls to this API return a {\tt UPNP_E_INIT}
 *  error code.
 *
 *  Optionally, the application can specify a host IP address (in the
 *  case of a multi-homed configuration) and a port number to use for
 *  all UPnP operations.  Since a port number can be used only by one
 *  process, multiple processes using the UPnP library must specify
 *  different port numbers.
 *
 *  If unspecified, the UPnP library will use the first adapter's IP address 
 *  and an arbitrary port
 *
 *  This call is synchronous.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist 
 *              to initialize the UPnP library.
 *      \item {\tt UPNP_E_INIT}: The UPnP library is already initialized. 
 *      \item {\tt UPNP_E_INIT_FAILED}: The UPnP library initialization 
 *              failed for an unknown reason.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_LISTEN}: An error occurred listening to a socket.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error ocurred creating a socket.
 *      \item {\tt UPNP_E_INTERNAL_ERROR}: An internal error ocurred.
 *    \end{itemize} */

EXPORT int UpnpInit(
    IN const char *HostIP, /** The host IP address to use, in
                           string format, for example "192.168.0.1", or 
		                   {\tt NULL} to use the first adapter's IP 
                            address. */
    IN unsigned short DestPort  /** The destination port number to
                             use.  0 will pick an arbitrary free port */
    );

/* change the local ip address */
EXPORT void UpnpHostChange(IN const char* HostIP, IN unsigned short DestPort ) ;

/** Terminates the UPnP Software Development Kit. This function must be the
 *  last API function called. It should be called only once. Subsequent 
 *  calls to this API return a {\tt UPNP_E_FINISH} error code.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_FINISH}: The UPnP library is already terminated or 
 *                                 it is not initialized. 
 *    \end{itemize} */

EXPORT int UpnpFinish();


/** {\bf UpnpRegisterClient} registers a control point application with the
 *  UPnP API.  A control point application cannot make any other API calls
 *  until it registers using this function.
 *
 *  {\bf UpnpRegisterClient} is a synchronous call and generates no callbacks.
 *  Callbacks can occur as soon as this function returns.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_FINISH}: The UPnP library is already terminated or 
 *                                 is not initialized. 
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf Callback} or {\bf Hnd} 
 *              is not a valid pointer.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              register this control point.
 *    \end{itemize}
 */

EXPORT int UpnpRegisterClient(
    IN Upnp_FunPtr Callback,   /** Pointer to a function for receiving 
                                   asynchronous events. */
    IN const void *Cookie,     /** Pointer to user data returned with the
                                   callback function when invoked. */
    OUT UpnpClient_Handle *Hnd /** Pointer to a variable to store the 
                                   new control point handle. */
    );  

/** {\bf UpnpRegisterRootDevice} registers a device application with
 *  the UPnP API.  A device application cannot make any other API
 *  calls until it registers using this function.  Device applications
 *  can also register as control points (see {\bf UpnpRegisterClient}
 *  to get a control point handle to perform control point
 *  functionality).
 *
 *  {\bf UpnpRegisterRootDevice} is synchronous and does not generate
 *  any callbacks.  Callbacks can occur as soon as this function returns.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_FINISH}: The UPnP library is already terminated or 
 *                                 is not initialized. 
 *      \item {\tt UPNP_E_INVALID_DESC}: The description document was not 
 *              found or it does not contain a valid device description.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf Callback} or {\bf Hnd} 
 *              are not valid pointers or {\bf DescURL} is {\tt NULL}.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occurred.
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting the 
 *              socket.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: Too many sockets are currently 
 *              allocated.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: There are insufficient resources to 
 *              register this root device.
 *    \end{itemize} */

EXPORT int UpnpRegisterRootDevice(
    IN const char *DescUrl,    /** Pointer to a string containing the
                                   description URL for this root device 
                                   instance. */
    IN Upnp_FunPtr Callback,   /** Pointer to the callback function for 
                                   receiving asynchronous events. */
    IN const void *Cookie,     /** Pointer to user data returned with the
                                   callback function when invoked. */
    OUT UpnpDevice_Handle *Hnd /** Pointer to a variable to store the 
                                   new device handle. */
    );


/** {\bf UpnpRegisterRootDevice2} registers a device application with
 *  the UPnP API.  A device application cannot make any other API
 *  calls until it registers using this function.  Device applications
 *  can also register as control points (see {\bf UpnpRegisterClient}
 *  to get a control point handle to perform control point
 *  functionality).
 *
 *  This function is similar to {\bf UpnpRegisterRootDevice}, except that it
 *  also allows the description document to be specified as a file or a
 *  memory buffer. The description can also be configured to have the
 *  correct IP and port address.
 *
 *  NOTE: For configuration to be functional, the internal web server
 *  MUST be present. In addition, the web server MUST be activated
 *  (using {\bf UpnpSetWebServerRootDir}) before calling this function.
 *  The only condition the web server can be absent is if the description
 *  document is specfied as a URL and no configuration is required
 *  (i.e. {\tt config_baseURL = 0}.)
 *
 *  {\bf UpnpRegisterRootDevice2} is synchronous and does not generate
 *  any callbacks.  Callbacks can occur as soon as this function returns.
 *
 *  Using different types of description documents:
 *  1) Description specified as a URL:
 *        descriptionType == UPNPREG_URL_DESC
 *        description is the URL
 *        bufferLen = 0 (or any other value, ignored)
 *  2) Description specified as a file:
 *        descriptionType == UPNPREG_FILENAME_DESC
 *        description is a filename
 *        bufferLen ignored
 *  3) Description specified as a memory buffer:
 *        descriptionType == UPNPREG_BUF_DESC
 *        description is pointer to a memory buffer
 *        bufferLen == length of memory buffer
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_FINISH}: The UPnP library is already terminated or
 *                                 is not initialized.
 *      \item {\tt UPNP_E_INVALID_DESC}: The description document was not
 *              found or it does not contain a valid device description.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf Callback} or {\bf Hnd}
 *              are not valid pointers or {\bf DescURL} is {\tt NULL}.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occurred.
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting the
 *              socket.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: Too many sockets are currently
 *              allocated.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: There are insufficient resources to
 *              register this root device.
 *      \item {\tt UPNP_E_URL_TOO_BIG}: Length of URL is bigger than
 *              internal buffer.
 *      \item {\tt UPNP_E_FILE_NOT_FOUND}: Description file not found.
 *      \item {\tt UPNP_E_FILE_READ_ERROR}: Error reading the description
 *              file.
 *      \item {\tt UPNP_E_INVALID_URL}: Description url invalid.
 *      \item {\tt UPNP_E_EXT_NOT_XML}: Description url or file should have
 *              a .xml extension
 *      \item {\tt UPNP_E_NO_WEB_SERVER}: The internal web server has been
 *              compiled out; cannot configure description doc and server
 *              it in this case.
 *    \end{itemize} */
EXPORT int UpnpRegisterRootDevice2(
        IN Upnp_DescType descriptionType,/** The type of description 
                                             document. */
        IN const char* description,      /** Treated as a URL, file name or
                                             memory buffer depending on 
                                             description type. */
        IN size_t bufferLen,             /** Length of memory buffer if 
                                             {\tt descriptionType ==
                                             UPNPREG_BUF_DESC}; otherwise, 
					                         ignored. */
        IN int config_baseURL,           /** If nonzero, URLBase of 
                                             description document is 
					     configured and the description 
					     is served using the internal 
					     web server. */
        IN Upnp_FunPtr Fun,              /** Pointer to the callback function 
					     for receiving asynchronous 
					     events. */
        IN const void* Cookie,           /** Pointer to user data returned
					     with the callback function when 
					     invoked. */
        OUT UpnpDevice_Handle* Hnd       /** Pointer to a variable to store 
					     the new device handle. */
        );


/** {\bf UpnpUnRegisterClient} unregisters a control point application, 
 *  unsubscribing all active subscriptions.
 *  After this call, the {\bf UpnpClient_Handle} is no longer valid.
 *
 *  {\bf UpnpUnRegisterClient} is a synchronous call and generates no
 *  callbacks.  The UPnP library generates no more callbacks after this 
 *  function returns.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *                   point handle.
 *    \end{itemize} */

EXPORT int UpnpUnRegisterClient(
    IN UpnpClient_Handle Hnd  /** The handle of the control point instance 
                               to unregister. */
    );

/** Unregisters a root device registered with {\bf UpnpRegisterRootDevice}.
 *  After this call, the {\bf UpnpDevice_Handle} is no longer valid. 
 *  For all advertisements that have not yet expired, the UPnP library sends a
 *  device unavailable message automatically.
 *
 *  {\bf UpnpUnRegisterRootDevice} is a synchronous call and generates no
 *  callbacks.  Once this call returns, the UPnP library will no longer 
 *  generate callbacks to the application.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid UPnP 
 *              device handle.
 *    \end{itemize}
 */

EXPORT int UpnpUnRegisterRootDevice(
   IN UpnpDevice_Handle /** The handle of the root device instance to 
                            unregister. */
   );

//@} // Initialization and Registration

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        D I S C O V E R Y                           //
//                                                                    //
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

///@name Discovery
//@{

/** {\bf UpnpSearchAsync} searches for devices matching the given
 *  search target.  The function returns immediately and the UPnP library 
 *  calls the default callback function for each matching root device,
 *  device, or service.  The application specifies the search type by the 
 *  {\bf Target} parameter.  
 *
 *  Note that there is no way for the UPnP library to distinguish which client
 *  instance issued a particular search.  Therefore, the client can get
 *  search callbacks that do not match the original criteria of the search.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf Target} is {\tt NULL}.
 *    \end{itemize} */

EXPORT int UpnpSearchAsync(
    IN UpnpClient_Handle Hnd, /** The handle of the client performing 
                                  the search. */
    IN int Mx,                /** The time, in seconds, to delay the 
                                  responses. */ 
    IN const char *Target,    /** The search target as defined in the UPnP
                                  Device Architecture v1.0 specification. */
    IN const void *Cookie     /** The user data to pass when the callback
                                  function is invoked. */
    ); 

/** {\bf UpnpSendAdvertisement} sends out the discovery announcements for
 *  all devices and services for a device.  Each announcement is made with
 *  the same expiration time.
 *
 *  {\bf UpnpSendAdvertisement} is a synchronous call.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid 
 *              device handle.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: There are insufficient resources to 
 *              send future advertisements.
 *    \end{itemize}
 */

EXPORT int UpnpSendAdvertisement(
    IN UpnpDevice_Handle Hnd, /** The device handle for which to send out the 
                                  announcements. */
    IN int Exp                /** The expiration age, in seconds, of 
                                  the announcements. */
    );

//@} // Discovery

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                            C O N T R O L                           //
//                                                                    //
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

///@name Control
//@{

/** {\bf UpnpGetServiceVarStatus} queries the state of a state 
 *  variable of a service on another device.  This is a synchronous call.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: {\bf ActionUrl} is not a valid URL.
 *      \item {\tt UPNP_E_INVALID_DESC}: The XML document was not 
 *              found or it does not contain a valid XML description.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf StVarVal} is not a valid 
 *              pointer or {\bf VarName} is {\tt NULL}.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpGetServiceVarStatus(
    IN UpnpClient_Handle Hnd,     /** The handle of the control point. */
    IN const char *ActionURL,     /** The URL of the service. */
    IN const char *VarName,       /** The name of the variable to query. */
    OUT Upnp_DOMString *StVarVal  /** The pointer to store the value
                                      for {\bf VarName}. The UPnP library 
                                      allocates this string and the caller 
                                      needs to free it. */
    );

/** {\bf UpnpGetServiceVarStatusAsync} queries the state of a variable of a 
 *  service, generating a callback when the operation is complete.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: The {\bf ActionUrl} is not a valid URL.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf VarName} or {\bf Fun} 
 *              is not a valid pointer.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpGetServiceVarStatusAsync(
    IN UpnpClient_Handle Hnd, /** The handle of the control point. */
    IN const char *ActionURL, /** The URL of the service. */
    IN const char *VarName,   /** The name of the variable to query. */
    IN Upnp_FunPtr Fun,       /** Pointer to a callback function to 
                                  be invoked when the operation is complete. */
    IN const void *Cookie     /** Pointer to user data to pass to the
                                  callback function when invoked. */
    );

/** {\bf UpnpSendAction} sends a message to change a state variable
 *  in a service.  This is a synchronous call that does not return until the 
 *  action is complete.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: {\bf ActionUrl} is not a valid URL.
 *      \item {\tt UPNP_E_INVALID_ACTION}: This action is not valid.
 *      \item {\tt UPNP_E_INVALID_DEVICE}: Either {\bf DevUDN} is not a 
 *              valid device.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf ServiceType}, {\bf Action}, or 
 *              {\bf RespNode} is not a valid pointer.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpSendAction(
    IN UpnpClient_Handle Hnd,   /** The handle of the control point 
                                    sending the action. */
    IN const char *ActionURL,   /** The action URL of the service. */
    IN const char *ServiceType, /** The type of the service. */
    IN const char *DevUDN,      /** This parameter is ignored. */
    IN Upnp_Document Action,    /** The DOM document for the action. */
    OUT Upnp_Document *RespNode /** The DOM document for the response 
                                    to the action.  The UPnP library allocates 
                                    this document and the caller needs to free 
                                    it.  */
   );

/** {\bf UpnpSendActionAsync} sends a message to change a state variable
 *  in a service, generating a callback when the operation is complete.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: {\bf ActionUrl} is an valid URL.
 *      \item {\tt UPNP_E_INVALID_DEVICE}: {\bf DevUDN} is an valid device.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf Fun} is not a valid 
 *              callback function or either {\bf ServiceType} or {\bf Act} is 
 *              {\tt NULL}.
 *      \item {\tt UPNP_E_INVALID_ACTION}: This action is not valid.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpSendActionAsync(
    IN UpnpClient_Handle Hnd,   /** The handle of the control point 
                                    sending the action. */
    IN const char *ActionURL,   /** The action URL of the service. */
    IN const char *ServiceType, /** The type of the service. */
    IN const char *DevUDN,      /** This parameter is ignored. */
    IN Upnp_Document Action,    /** The DOM document for the action to 
                                    perform on this device. */
    IN Upnp_FunPtr Fun,         /** Pointer to a callback function to 
                                    be invoked when the operation 
				                    completes. */
    IN const void *Cookie       /** Pointer to user data that to be
                                    passed to the callback when invoked. */
    );

//@} // Control

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        E V E N T I N G                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

///@name Eventing
//@{

/** {\bf UpnpAcceptSubscription} accepts a subscription request and sends
 *  out the current state of the eventable variables for a service.  
 *  The device application should call this function when it receives a 
 *  {\tt UPNP_EVENT_SUBSCRIPTION_REQUEST} callback. This function is sychronous
 *  and generates no callbacks.
 *
 *  {\bf UpnpAcceptSubscription} can be called during the execution of 
 *  a callback function.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *      \item {\tt UPNP_E_INVALID_SERVICE}: The {\bf DevId} {\bf ServId} 
 *              pair refers to an invalid service. 
 *      \item {\tt UPNP_E_INVALID_SID}: The specified subscription ID is not 
 *              valid.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf VarName} or 
 *              {\bf NewVal} is not a valid pointer or {\bf cVariables} is 
 *              less than zero.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpAcceptSubscription(
    IN UpnpDevice_Handle Hnd, /** The handle of the device. */
    IN const char *DevID,     /** The device ID of the subdevice of the
                                  service generating the event. */
    IN const char *ServID,    /** The unique service identifier of the service
                                  generating the event. */
    IN const char **VarName,  /** Pointer to an array of event variables. */
    IN const char **NewVal,   /**  Pointer to an array of values for
			                       the event variables. */
    IN int cVariables,        /** The number of event variables in 
                                  {\bf VarName}. */
    IN Upnp_SID SubsId        /** The subscription ID of the newly
                                  registered control point. */
    );


/** {\bf UpnpAcceptSubscriptionExt} accepts a subscription request and sends
 *  out the current state of the eventable variables for a service.  
 *  The device application should call this function when it receives a 
 *  {\tt UPNP_EVENT_SUBSCRIPTION_REQUEST} callback. This function is sychronous
 *  and generates no callbacks.
 *
 *  {\bf UpnpAcceptSubscriptionExt} can be called during the execution of 
 *  a callback function.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *      \item {\tt UPNP_E_INVALID_SERVICE}: The {\bf DevId} {\bf ServId} 
 *              pair refers to an invalid service. 
 *      \item {\tt UPNP_E_INVALID_SID}: The specified subscription ID is not 
 *              valid.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf VarName} or 
 *              {\bf NewVal} is not a valid pointer or {\bf cVariables} is 
 *              less than zero.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpAcceptSubscriptionExt(
    IN UpnpDevice_Handle Hnd, /** The handle of the device. */
    IN const char *DevID,     /** The device ID of the subdevice of the
                                  service generating the event. */
    IN const char *ServID,    /** The unique service identifier of the service
                                  generating the event. */
    IN Upnp_Document PropSet, /** The DOM document for property set. */

    IN Upnp_SID SubsId        /** The subscription ID of the newly
                                  registered control point. */
    );


/** {\bf UpnpNotify} sends out an event change notification to all
 *  control points subscribed to a particular service.  This function is
 *  synchronous and generates no callbacks.
 *
 *  {\bf UpnpNotify} may be called during a callback function to send out
 *  a notification.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *      \item {\tt UPNP_E_INVALID_SERVICE}: The {\bf DevId} {\bf ServId} 
 *              pair refers to an invalid service.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf VarName} or {\bf NewVal} 
 *               is not a valid pointer or {\bf cVariables} is less than zero.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpNotify(
    IN UpnpDevice_Handle,   /** The handle to the device sending the event. */
    IN const char *DevID,   /** The device ID of the subdevice of the service
                              generating the event. */
    IN const char *ServID,  /** The unique identifier of the service
                              generating the event. */
    IN const char **VarName,/** Pointer to an array of variables that
                              have changed. */
    IN const char **NewVal, /** Pointer to an array of new values for
                              those variables. */
    IN int cVariables       /** The count of variables included in this
                              notification. */
    );


/** {\bf UpnpNotifyExt} sends out an event change notification to all
 *  control points subscribed to a particular service.  This function is
 *  synchronous and generates no callbacks.
 *
 *  {\bf UpnpNotifyExt} may be called during a callback function to send out
 *  a notification.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *      \item {\tt UPNP_E_INVALID_SERVICE}: The {\bf DevId} {\bf ServId} 
 *              pair refers to an invalid service.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf VarName} or {\bf NewVal} 
 *               is not a valid pointer or {\bf cVariables} is less than zero.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpNotifyExt(
    IN UpnpDevice_Handle,       /** The handle to the device sending the event. */
    IN const char *DevID,       /** The device ID of the subdevice of the service
                                    generating the event. */
    IN const char *ServID,      /** The unique identifier of the service
                                    generating the event. */
    IN Upnp_Document PropSet    /** The DOM document for property set. */

    );

/** {\bf UpnpRenewSubscription} renews a subscription that is about to 
 *  expire.  This function is synchronous.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf Timeout} is not a valid pointer.
 *      \item {\tt UPNP_E_INVALID_SID}: The SID being passed to this function 
 *              is not a valid subscription ID.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured. 
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.  
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl}.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error occurred creating a socket.
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher.
 *      \item {\tt UPNP_E_SUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the subscription renew.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpRenewSubscription(
    IN UpnpClient_Handle Hnd, /** The handle of the control point that 
                                  is renewing the subscription. */
    INOUT int *TimeOut,       /** Pointer to a variable containing the 
                                  requested subscription time.  Upon return, 
                                  it contains the actual renewal time. */
    IN Upnp_SID SubsId             /** The ID for the subscription to renew. */
    );

/** {\bf UpnpRenewSubscriptionAsync} renews a subscription that is about
 *  to expire, generating a callback when the operation is complete.
 *
 *  Note that many of the error codes for this function are returned in
 *  the {\tt Upnp_Event_Renewal} structure.  In these cases, the function
 *  returns {\tt UPNP_E_SUCCESS} and the appropriate error code will
 *  be in the {\tt Upnp_Event_Renewal.ErrCode} field in the {\tt Event}
 *  structure passed to the callback.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_SID}: The {\bf SubsId} is not a valid 
 *              subscription ID.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf Fun} is not a valid 
 *              callback function pointer or {\bf Timeout} is less than zero 
 *              but is not {\tt UPNP_INFINITE}.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured (returned in 
 *              the {\tt Upnp_Event_Renewal.ErrCode} field as part of the 
 *              callback).
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket (returned in the {\tt Upnp_Event_Renewal.ErrCode} 
 *              field as part of the callback).
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading  
 *              from a socket (returned in the {\tt Upnp_Event_Renewal.ErrCode 
 *              } field as part of the callback).
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding the socket 
 *              (returned in the {\tt Upnp_Event_Renewal.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl} (returned in the {\tt 
 *              Upnp_Event_Renewal.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error occurred creating socket (
 *              returned in the {\tt Upnp_Event_Renewal.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher (returned in the {\tt 
 *              Upnp_Event_Renewal.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_SUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the subscription request (returned in the {\tt 
 *              Upnp_Event_Renewal.ErrCode} field as part of the callback).
 *    \end{itemize}
 */

EXPORT int UpnpRenewSubscriptionAsync(
    IN UpnpClient_Handle Hnd, /** The handle of the control point that 
                                  is renewing the subscription. */
    IN int TimeOut,           /** The requested subscription time.  The 
                                  actual timeout value is returned when 
                                  the callback function is called. */
    IN Upnp_SID SubsId,            /** The ID for the subscription to renew. */
    IN Upnp_FunPtr Fun,       /** Pointer to a callback function to be 
                                  invoked when the renewal is complete. */
    IN const void *Cookie     /** Pointer to user data passed
                                  to the callback function when invoked. */
    );

/** {\bf UpnpSetMaxSubscriptions} sets the maximum number of subscriptions 
 *  accepted per service. The default value accepts as many as system 
 *  resources allow. If the number of current subscriptions for a service is 
 *  greater than the requested value, the UPnP library accepts no new 
 *  subscriptions or renewals, however, the UPnP library does not remove
 *  any current subscriptions.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *    \end{itemize}
 */

EXPORT int UpnpSetMaxSubscriptions(  
    IN UpnpDevice_Handle Hnd, /** The handle of the device for which 
				  the maximum subscriptions is being set. */
    IN int MaxSubscriptions   /** The maximum number of subscriptions to be 
				  allowed per service. */
    );

/** {\bf UpnpSetMaxSubscriptionTimeOut} sets the maximum time-out accepted
 *  for a subscription request or renewal. The default value accepts the 
 *  time-out set by the control point. If a control point requests a 
 *  subscription time-out less than or equal to the maximum then, the UPnP
 *  library grants the value requested by the control point.  If the time-out 
 *  is greater, the UPnP library returns the maximum value.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid device 
 *              handle.
 *    \end{itemize}
 */

EXPORT int UpnpSetMaxSubscriptionTimeOut(  
    IN UpnpDevice_Handle Hnd,       /** The handle of the device for which 
				        the maximum subscription time-out is 
                                        being set. */
    IN int MaxSubscriptionTimeOut   /** The maximum subscription time-out to 
                                        be accepted. */
    );

/** {\bf UpnpSubscribe} registers a control point to receive event
 *  notifications from another device.  This operation is synchronous.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: {\bf PublisherUrl} is not a valid URL.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf Timeout} is not a valid pointer 
 *              or {\bf SubsId} is {\tt NULL}.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured. 
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl}.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error occurred creating a socket.
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher.
 *      \item {\tt UPNP_E_SUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the subscription request.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpSubscribe(
    IN UpnpClient_Handle Hnd,    /** The handle of the control point. */
    IN const char *PublisherUrl, /** The URL of the service to subscribe to. */
    INOUT int *TimeOut,          /** Pointer to a variable containsing
                                  the requested subscription time.  Upon 
                                  return, it contains the actual 
                                  subscription time returned from the 
				                  service. */
    OUT Upnp_SID SubsId          /** Pointer to a variable to receive the
                                  subscription ID (SID). */
    );

/** {\bf UpnpSubscribeAsync} performs the same operation as
 * {\bf UpnpSubscribe}, but returns immediately and calls the registered
 * callback function when the operation is complete.
 *
 *  Note that many of the error codes for this function are returned in
 *  the {\tt Upnp_Event_Subscribe} structure.  In these cases, the function
 *  returns {\tt UPNP_E_SUCCESS} and the appropriate error code will
 *  be in the {\tt Upnp_Event_Subscribe.ErrCode} field in the {\tt Event}
 *  structure passed to the callback.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_URL}: The {\bf PublisherUrl} is not a valid 
 *              URL.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf TimeOut} or {\bf Fun} 
 *              is not a valid pointer.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured (returned in 
 *              the {\tt Upnp_Event_Subscribe.ErrCode} field as part of the 
 *              callback).
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket (returned in the 
 *              {\tt Upnp_Event_Subscribe.ErrCode} field as part of the 
 *              callback).
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding the socket 
 *              (returned in the {\tt Upnp_Event_Subscribe.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl} (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error occurred creating socket (
 *              returned in the {\tt Upnp_Event_Subscribe.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_SUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the subscription request (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *    \end{itemize}
 */

EXPORT int UpnpSubscribeAsync(
    IN UpnpClient_Handle Hnd,      /** The handle of the control point that
                                       is subscribing. */
    IN const char *PublisherUrl,   /** The URL of the service to subscribe to. */
    IN int TimeOut,                /** The requested subscription time.  Upon
                                       return, it contains the actual
                                       subscription time returned from the
                                       service. */
    IN Upnp_FunPtr Fun,            /** Pointer to the callback function for
                                       this subscribe request. */
    IN const void *Cookie          /** A user data value passed to the
                                       callback function when invoked. */
    );

/** {\bf UpnpUnSubscribe} removes the subscription of  a control point from a 
 * service previously subscribed to using {\bf UpnpSubscribe} or 
 * {\bf UpnpSubscribeAsync}.  This is a synchronous call.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_SID}: The {\bf SubsId} is not a valid 
 *              subscription ID.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured. 
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl}.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error ocurred creating a socket.
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher.
 *      \item {\tt UPNP_E_UNSUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the unsubscribe request (the client is still unsubscribed and 
 *              no longer receives events).
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpUnSubscribe(
    IN UpnpClient_Handle Hnd, /** The handle of the subscribed control 
                                  point. */
    IN Upnp_SID SubsId             /** The ID returned when the 
                                  control point subscribed to the service. */
    );

/** {\bf UpnpUnSubscribeAsync} removes a subscription of a control
 * point from a service previously subscribed to using {\bf
 * UpnpSubscribe} or {\bf UpnpSubscribeAsync}, generating a callback
 * when the operation is complete.
 *
 *  Note that many of the error codes for this function are returned in
 *  the {\tt Upnp_Event_Subscribe} structure.  In these cases, the function
 *  returns {\tt UPNP_E_SUCCESS} and the appropriate error code will
 *  be in the {\tt Upnp_Event_Subscribe.ErrCode} field in the {\tt Event}
 *  structure passed to the callback.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_HANDLE}: The handle is not a valid control 
 *              point handle.
 *      \item {\tt UPNP_E_INVALID_SID}: The {\bf SubsId} is not a valid SID.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf Fun} is not a valid callback 
 *              function pointer.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occured (returned in 
 *              the {\tt Upnp_Event_Subscribe.ErrCode} field as part of the 
 *              callback).
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding the socket 
 *              (returned in the {\tt Upnp_Event_Subscribe.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting to 
 *              {\bf PublisherUrl} (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: An error occurred creating a socket (
 *              returned in the {\tt Upnp_Event_Subscribe.ErrCode} field as 
 *              part of the callback).
 *      \item {\tt UPNP_E_BAD_RESPONSE}: An error occurred in response from 
 *              the publisher (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *      \item {\tt UPNP_E_UNSUBSCRIBE_UNACCEPTED}: The publisher refused 
 *              the subscription request (returned in the {\tt 
 *              Upnp_Event_Subscribe.ErrCode} field as part of the callback).
 *    \end{itemize} */

EXPORT int UpnpUnSubscribeAsync(
    IN UpnpClient_Handle Hnd, /** The handle of the subscribed control 
                                  point. */
    IN Upnp_SID SubsId,            /** The ID returned when the 
                                  control point subscribed to the service. */
    IN Upnp_FunPtr Fun,       /** Pointer to a callback function to be 
                                  called when the operation is complete. */
    IN const void *Cookie     /** Pointer to user data to pass to the
                                  callback function when invoked. */
    );

//@} // Eventing


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        C L I E N T - A P I                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

///@name Control Point HTTP API
//@{

/** {\bf UpnpDownloadUrlItem} downloads a file specified in a URL.
 *  The UPnP library allocates the memory for {\bf outBuf} and the 
 *  application is responsible for freeing this memory.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf url}, {\bf outBuf} 
 *              or {\bf contentType} is not a valid pointer.
 *      \item {\tt UPNP_E_INVALID_URL}: The {\bf url} is not a valid 
 *              URL.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              download this file.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occurred.
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting a 
 *              socket.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: Too many sockets are currently 
 *              allocated.
 *    \end{itemize}
 */

EXPORT int UpnpDownloadUrlItem(
    IN const char *url,          /** URL of an item to download. */
    OUT char **outBuf,           /** Buffer to store the downloaded item. */
    OUT char *contentType        /** HTTP header value content type if 
                                     present. It should be at least 
                                     {\bf LINE_SIZE} bytes in size. */
    );


/** {\bf UpnpDownloadXmlDoc} downloads an XML document specified in a URL.
 *  The UPnP library parses the document and returns it in the from of a 
 *  DOM document. The application is responsible for freeing the DOM document.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_PARAM}: Either {\bf url} or {\bf xmlDoc} 
 *              is not a valid pointer.
 *      \item {\tt UPNP_E_INVALID_DESC}: The XML document was not 
 *              found or it does not contain a valid XML description.
 *      \item {\tt UPNP_E_INVALID_URL}: The {\bf url} is not a valid 
 *              URL.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: There is insufficient resources to 
 *              download the XML document.
 *      \item {\tt UPNP_E_NETWORK_ERROR}: A network error occurred.
 *      \item {\tt UPNP_E_SOCKET_WRITE}: An error or timeout occurred writing 
 *              to a socket.
 *      \item {\tt UPNP_E_SOCKET_READ}: An error or timeout occurred reading 
 *              from a socket.
 *      \item {\tt UPNP_E_SOCKET_BIND}: An error occurred binding a socket.
 *      \item {\tt UPNP_E_SOCKET_CONNECT}: An error occurred connecting the 
 *              socket.
 *      \item {\tt UPNP_E_OUTOF_SOCKET}: Too many sockets are currently 
 *              allocated.
 *    \end{itemize}
 */

EXPORT int UpnpDownloadXmlDoc(
    IN const char *url,          /** URL of the XML document. */
    OUT Upnp_Document *xmlDoc    /** The parsed XML document. */
    );

//@} // Control Point HTTP API

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//                                                                    //
//                    W E B  S E R V E R  A P I                       //
//                                                                    //
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

///@name Web Server API
//@{

/** {\bf UpnpSetWebServerRootDir} sets the document root directory for
 *  the internal web server. This directory is considered the
 *  root directory (i.e. "/") of the web server.
 *
 *  It is also used to activate or deactivate the web server.
 *  To disable the web server, pass {\tt NULL} for {\bf rootDir}; to 
 *  activate, pass a valid directory string.
 *  
 *  Note that this function is not available when the web server is not
 *  compiled into the UPnP library (i.e. make WEB=0).
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *       \item {\tt UPPN_E_SUCCESS}: The operation completed successfully.
 *       \item {\tt UPNP_E_INVALID_ARGUMENT}: {\bf rootDir} is an invalid 
 *               directory.
 *    \end{itemize}
 */
 
EXPORT int UpnpSetWebServerRootDir( 
    IN const char* rootDir  /** Path of the root directory of the web 
                                server. */
    );

//@} // Web Server API

#ifdef __cplusplus
}
#endif // __cplusplus

//@Include: upnpdom/domCif.h
//@Include: tools/upnptools.h
//@Include: tools/config.h

//@} The API

//@} UPnP SDK for Linux

#endif

