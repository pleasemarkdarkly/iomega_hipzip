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

#ifndef URLCONFIG_H
#define URLCONFIG_H
#ifndef _WIN32
//#include <sys/socket.h>
#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */
#include <network.h>	// - ecm
#ifdef __cplusplus
}
#endif	/* __cplusplus */
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <util/upnp/upnpdom/domCif.h>

/* functions available only if INTERNAL_WEB_SERVER is #defined */

////////////////////////////////////////////////////////////
// modifies 'doc''s urlbase to the machine's ip address. The modified
//  doc is added to the web server to be served using the given alias.
// args:
//   doc: dom document whose urlbase is to be modified
//   serverAddress: ip address and port on which miniserver is
//      running
//   alias: a name to be used for the temp; e.g.: "foo.xml"
//   actual_alias: created if there is a name conflict with
//     alias. E.g.: "foo3.xml". actual_alias should be
//     destroyed using free().
//
// Once installed, the alias can be accessed using an url like:
//  "http://34.24.243.23:80/foo3.xml"
//
//
// return codes:
//   UPNP_E_SUCCESS
//   UPNP_E_OUTOF_MEMORY
//   UPNP_E_BAD_URLBASE
//   UPNP_E_CANNOT_CREATE_ALIAS
int Configure_Urlbase( INOUT Upnp_Document doc,
    IN const struct sockaddr_in* serverAddr,
    IN const char* alias, 
    OUT char** actual_alias, 
    OUT char** descURL);


#endif /* URLCONFIG_H */

