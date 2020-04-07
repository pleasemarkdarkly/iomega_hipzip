//
// File Name: DJUPnP.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DJUPNP_H_
#define DJUPNP_H_

#include <util/upnp/api/upnp.h>

bool DJUPnPStartup(const char* szIPAddress, int iPort);
void DJUPnPShutdown();

//! Clear the current global device list and issue new search
//! requests to build it up again from scratch.
bool DJUPnPControlPointRefresh(bool bClear = true);

int DJUPNPSendAction(int service, int iDeviceNumber, char *actionname, char **param_name, char **param_val, int param_count, Upnp_FunPtr pCallbackFunction = 0, const void *pCookie = 0);

//! Remove a device
int DJUPnPControlPointRemoveDevice(int iDeviceNumber);

#endif	// DJUPNP_H_
