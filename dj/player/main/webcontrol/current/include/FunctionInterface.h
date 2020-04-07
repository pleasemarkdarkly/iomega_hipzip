//
// webinterface.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//


#ifndef FUNCTIONINTERFACE_H_
#define FUNCTIONINTERFACE_H_


#define SD_BOTH 0xFF

typedef int fnHTTPFunctionHandler(int socket, const char * path);

typedef struct s_FunctionTable
{
	char * szFunctionName;
	fnHTTPFunctionHandler *pfnHTTPFunction;
} t_FunctionTable;

#define NUM_WEB_FUNCTIONS 11

extern t_FunctionTable g_FunctionHandlers[NUM_WEB_FUNCTIONS];





#endif	// WEBCONTROL_H_
