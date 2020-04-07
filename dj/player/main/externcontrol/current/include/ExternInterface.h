//
// ExternInterface.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//


#ifndef EXTERNINTERFACE_H_
#define EXTERNINTERFACE_H_

#include <main/externcontrol/ExternControl.h>

typedef t_ControlReturn fnCommandFunctionHandler(const char * szRequest, char ** szResponse );

typedef struct s_FunctionTable
{
	char * szFunctionName;
	fnCommandFunctionHandler *pfnCommandHandler;
	unsigned int  flControlMap;
} t_FunctionTable;

#define NUM_CONTROL_FUNCTIONS 25


extern t_FunctionTable g_ExternFunctionHandlers[NUM_CONTROL_FUNCTIONS];





#endif	// EXTERNINTERFACE_H_
