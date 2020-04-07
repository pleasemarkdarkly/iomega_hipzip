//
// serialinterface.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//


#ifndef SERIALINTERFACE_H_
#define SERIALINTERFACE_H_

typedef enum
{
	SH_RET_OK_RESPONDED=-1,
	SH_RET_OK=0,
	SH_RET_ERR_SYNTAX=1,
	SH_RET_ERR_PARAM=2,
	SH_RET_ERR_STATE=3
} t_SFHReturn;

#define NO_PARAM -1

typedef t_SFHReturn fnSerialFunctionHandler(FILE * fp, int param);

typedef struct s_FunctionTable
{
	char * szFunctionName;
	fnSerialFunctionHandler *pfnSerialFunction;
} t_FunctionTable;

#define NUM_SERIAL_FUNCTIONS 21

extern t_FunctionTable g_SerialFunctionHandlers[NUM_SERIAL_FUNCTIONS];





#endif	// SERIALINTERFACE_H_
