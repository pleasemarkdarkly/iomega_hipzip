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
// $Date: 2000/09/14 21:50:44 $
//

/** @name Optional Tool APIs
 *  The UPnP SDK for Linux contains some additional, optional utility
 *  APIs that can be helpful in writing applications using the SDK.
 *  These additional APIs can be compiled out in order to save code
 *  size in the UPnP library.  Refer to the README for details.
 */

//@{

#ifndef UPNP_TOOLS_H
#define UPNP_TOOLS_H

#include <util/upnp/api/upnp.h>
#include <util/upnp/upnpdom/domCif.h>


#ifndef _WIN32
#define EXPORT
#else
#define EXPORT __declspec(dllexport)
#endif

/** {\bf UpnpResolveURL} combines a base URL and a relative URL into
 *  a single absolute URL.  The memory for {\bf AbsURL} needs to be
 *  allocated by the caller and must be large enough to hold the
 *  {\bf BaseURL} and {\bf RelURL} combined to be safe.
 *
 *  @return An integer representing one of the following:
 *    \begin{itemize}
 *      \item {\tt UPNP_E_SUCCESS}: The operation completed successfully.
 *      \item {\tt UPNP_E_INVALID_PARAM}: {\bf RelURL} is {\tt NULL}.
 *      \item {\tt UPNP_E_INVALID_URL}: The {\bf BaseURL} {\bf RelURL} 
 *              combination does not form a valid URL.
 *      \item {\tt UPNP_E_OUTOF_MEMORY}: Insufficient resources exist to 
 *              complete this operation.
 *    \end{itemize}
 */

EXPORT int UpnpResolveURL(
	IN char * BaseURL,  /** The base URL to combine. */
	IN char * RelURL,   /** The relative URL to {\bf BaseURL}. */
	OUT char * AbsURL   /** Pointer to a buffer to store the 
                                absolute URL. */
	);




/** {\bf UpnpMakeAction} This function creates a action request 
 *  packet based on its input parameter(Status variable name  
 *  and value pair). Any  number of input parameter can be  
 *  passed to this function but every  input variable name 
 *  should have its matching value argument. 
 *   
 *  
 *  @return  NULL if fails, or the action node of Upnp_Document type.
 */

EXPORT Upnp_Document UpnpMakeAction(
	IN char * ActionName,  /** The action name. */
	IN char * ServType,    /** The service type.  */
	IN int NumArg,         /** Number of argument pair to be passed. */  
        IN char * Arg,         /** Status variable name and value pair. */
        IN ...                 /** Other status variable name and value pair. */
	);





/** {\bf UpnpAddToAction} This function creates a action request
 *  packet based on its input parameter(Status variable name
 *  and value pair). This API is specialy suitable to use inside
 *  a loop to add any number input parameter in an existing action.
 *  If no action document exist in the begning then a  Upnp_Document
 *  variable initialized with NULL should be passed as a parameter.
 *
 *  @return  UPNP_E_SUCCESS in case of success.
 */

EXPORT int UpnpAddToAction(
	IN OUT Upnp_Document * ActionDoc, /** The action document node. */
	IN char * ActionName,             /** The action name. */
	IN char * ServType,               /** The service type.  */
	IN char * ArgName,                /** Status variable name */
        IN char * ArgVal                  /** Status variable value  */
 	);








/** {\bf UpnpAddToPropertySet} This function can be used at a place 
 *  where application needs to transfer the status of too many
 *  status variable under a single property set or it can be used 
 *  (Inside a loop) to add some extra status variable in a existing
 *  property set. If no property set exist before then a variable 
 *  initialized with NULL can be passed as its first parameter.
 *  
 *  @return  UPNP_E_SUCCESS in case of success.
 *
 */

EXPORT int UpnpAddToPropertySet(
	IN OUT Upnp_Document * PropSet,   /** The property set document node. */
	IN char * ArgName,                /** Status variable name */  
        IN char * ArgVal                  /** Status variable value  */
	);








/** {\bf UpnpCreatePropertySet} This function creates a property set  
 *  message packet. Any number of input parameter can be passed  
 *  to this function but every  input variable name should have 
 *  its matching value input argument.
 *  
 *  @return  NULL if fails, or the property-set document node.
 *
 */

EXPORT Upnp_Document  UpnpCreatePropertySet(
	IN int NumArg,  /** The Number of argument pair to be passed. */
	IN char* Arg,   /** Status variable name and value pair. */
        IN ...          /** Other input argument. */
	);






/** {\bf UpnpGetErrorMessage} converts a UPnP SDK error code into a 
 *  string error message suitable for display.  The memory returned
 *  from this function should NOT be freed.
 *
 *  @return An ASCII text string representation of the error message
 *    associated with the error code. 
 */

EXPORT const char * UpnpGetErrorMessage(
	int errorcode  /** The UPnP error code to convert. */
	);

//@}

#endif
