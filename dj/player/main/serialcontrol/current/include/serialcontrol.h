//
// serialcontrol.h
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems, Inc. All rights reserved
//


#ifndef SERIALCONTROL_H_
#define SERIALCONTROL_H_


int StartSerialControl();

int StopSerialControl();

void SerialServiceCallback(int cookie);


extern timer_handle_t SerialStatusTimer;


#endif	// SERIALCONTROL_H_
