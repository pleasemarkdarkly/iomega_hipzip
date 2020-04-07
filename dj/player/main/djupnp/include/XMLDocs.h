//
// File Name: XMLDocs.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef XMLDOCS_H_
#define XMLDOCS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define IML_SERVICE_SERVCOUNT   1
#define IML_SERVICE_1_VARCOUNT  0
#define IML_SERVICE_2_VARCOUNT  0

#define IML_SERVICE				0

/* This should be the maximum VARCOUNT from above */
#define IML_MAXVARS IML_SERVICE_2_VARCOUNT

extern char g_szIMLDeviceType[];
extern char *g_szIMLServiceType[];
extern char *g_szIMLServiceName[];

extern char *g_szIMLVarName[IML_SERVICE_SERVCOUNT][IML_MAXVARS];
extern char IMLVarCount[];

#ifdef __cplusplus
}
#endif

#endif	// XMLDOCS_H_
