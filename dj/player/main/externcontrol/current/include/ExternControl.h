//
// ExternControl.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//


#ifndef EXTERNCONTROL_H_
#define EXTERNCONTROL_H_


// bitmask flags defining the type of control

#define CONTROL_SERIAL     1
#define CONTROL_HTTPGET    2
#define CONTROL_SOAPACTION 4

#define CONTROL_ALL        CONTROL_SERIAL | CONTROL_HTTPGET | CONTROL_SOAPACTION

typedef enum
{
	CONTROL_OK=0,
	CONTROL_ERR_SYNTAX=1,
	CONTROL_ERR_PARAM=2,
	CONTROL_ERR_STATE=3
} t_ControlReturn;


typedef void t_fnControlResponseHandler(t_ControlReturn crReturnValue, const char * szResponse);

void ProcessControlRequest(const char * szRequest, int flControlType, t_fnControlResponseHandler * fnResponseHandler);


void ControlServiceCallback(int cookie);



#endif	// EXTERNCONTROL_H_
