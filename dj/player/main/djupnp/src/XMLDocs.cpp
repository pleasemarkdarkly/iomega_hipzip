//
// File Name: XMLDocs.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/djupnp/XMLDocs.h>

char g_szIMLDeviceType[] = "urn:www-fullplaymedia-com:device:medialibrary:1";
char *g_szIMLServiceType[] = {"urn:www-fullplaymedia-com:service:mls:1"};
char *g_szIMLServiceName[] = {"Media store service"};

/* Global arrays for storing variable names and counts for MS player services */
char *g_szIMLVarName[IML_SERVICE_SERVCOUNT][IML_MAXVARS] = {
//    {"ModifiedTime","String"}, 
};

char IMLVarCount[IML_SERVICE_SERVCOUNT] = {IML_SERVICE_1_VARCOUNT};

