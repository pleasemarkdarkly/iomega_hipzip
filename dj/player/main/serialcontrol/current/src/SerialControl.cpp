//
// SerialControl.cpp - the serial control interface to the DJ
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <stdlib.h>
#include <stdio.h>
#include <cyg/fileio/fileio.h>
#include <cyg/infra/diag.h>
#include <stdarg.h>
#include <util/debug/debug.h>
#include <util/diag/diag.h>
#include <cyg/kernel/kapi.h>
#include <util/eventq/EventQueueAPI.h>
#include <main/ui/keys.h>
#include <core/events/SystemEvents.h>
#include <fs/fat/sdapi.h>
#include <main/ui/common/UserInterface.h>
#include <main/ui/UI.h>
#include <main/main/AppSettings.h>

#include <main/externcontrol/ExternControl.h>

#include <util/timer/Timer.h>


DEBUG_MODULE_S(SERIALCONTROL, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(SERIALCONTROL);  

#define SERIALCONTROL_RUNNING 1
#define SERIALCONTROL_NOTRUNNING 0
int g_SerialControlStatus=SERIALCONTROL_NOTRUNNING;

// signalled when the serial control thread needs to shut down
static cyg_flag_t g_flagStopSerial;
static cyg_flag_t g_flagControlCallbackComplete,g_flagControlFromTimerCallbackComplete;

timer_handle_t SerialStatusTimer;

#define SERIALCONTROL_THREAD_STACKSIZE   8192*4
static cyg_handle_t hSerialControlThread;
static cyg_thread   threadSerialControlThread;
static char         tstackSerialControlThread[SERIALCONTROL_THREAD_STACKSIZE];

FILE * fpSerialControlRead;
FILE * fpSerialControlWrite;
#define SERIAL_CR  '\r'
#define SERIAL_LF  '\n'
#define SERIAL_SP  ' '


#define SERIAL_BUF_SIZE 200
static char serialBuf[SERIAL_BUF_SIZE];

// forward declarations
void serialControl_thread_entry( cyg_uint32 data);
int  GetSerialCommand(char * buf, int bufSize);

// module interfaces
int StartSerialControl()
{


	if (g_SerialControlStatus!=SERIALCONTROL_NOTRUNNING)
	{
		DEBUGP(SERIALCONTROL, DBGLEV_ERROR,"Server is already running\n");
		return 0;
	}

	mount("", "/dev", "devfs");
	fpSerialControlRead = fopen("/dev/ser2", "r");

	if (fpSerialControlRead == NULL)
	{
		DEBUGP(SERIALCONTROL, DBGLEV_ERROR,"Could not get file pointer to serial control port\n");
		return 0;
	}  

	fpSerialControlWrite = fopen("/dev/ser2", "w");

	if (fpSerialControlWrite == NULL)
	{
		DEBUGP(SERIALCONTROL, DBGLEV_ERROR,"Could not get file pointer to serial control port\n");
		return 0;
	}  

	cyg_flag_init(&g_flagStopSerial);
	cyg_flag_init(&g_flagControlCallbackComplete);
	cyg_flag_init(&g_flagControlFromTimerCallbackComplete);
	

	cyg_thread_create( 10, serialControl_thread_entry, 0, "serial control thread",
	                   (void*)tstackSerialControlThread, SERIALCONTROL_THREAD_STACKSIZE, &hSerialControlThread, &threadSerialControlThread);
	cyg_thread_resume(hSerialControlThread);

	return 1;
}

int StopSerialControl()
{
	DEBUGP(SERIALCONTROL, DBGLEV_TRACE,"Signalling serial control thread to terminate\n\r");

	cyg_flag_setbits(&g_flagStopSerial,1);

	return 1;
}

void SerialControlResponseHandler(t_ControlReturn crReturnValue, const char * szResponse)
{
	DEBUGP(SERIALCONTROL, DBGLEV_TRACE,"Serial control response handler called\n\r");
	// here is where we get the response text - just put the padding in and send it out the serial port
	if (szResponse == NULL)
	{
		fprintf(fpSerialControlWrite,"!!%d\n\r",(int)crReturnValue);
	}
	else if (crReturnValue == CONTROL_OK)
	{
		fprintf(fpSerialControlWrite,"!#%s\n\r",szResponse);
	}
	else
	{
		fprintf(fpSerialControlWrite,"!!1\n\r");
	}
	cyg_flag_setbits(&g_flagControlCallbackComplete,1);
}

void SerialControlTimerResponseHandler(t_ControlReturn crReturnValue, const char * szResponse)
{
	DEBUGP(SERIALCONTROL, DBGLEV_TRACE,"Serial control response handler called\n\r");
	// here is where we get the response text - just put the padding in and send it out the serial port
	if (szResponse == NULL)
	{
		fprintf(fpSerialControlWrite,"!!%d\n\r",(int)crReturnValue);
	}
	else if (crReturnValue == CONTROL_OK)
	{
		fprintf(fpSerialControlWrite,"!#%s\n\r",szResponse);
	}
	else
	{
		fprintf(fpSerialControlWrite,"!!1\n\r");
	}
	cyg_flag_setbits(&g_flagControlFromTimerCallbackComplete,1);
}

void TimerCallback(void *Arg)
{
	ProcessControlRequest("!getStatus",CONTROL_SERIAL,SerialControlTimerResponseHandler);
	//cyg_flag_wait(&g_flagControlFromTimerCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);
}

// thread entry point
void serialControl_thread_entry( cyg_uint32 data)
{
	DEBUGP(SERIALCONTROL, DBGLEV_ERROR,"Serial control thread running\n");
	int inputLen = 0;
	g_SerialControlStatus=SERIALCONTROL_RUNNING;
	fprintf(fpSerialControlWrite,"!!0\n\r");

	register_timer( TimerCallback, NULL, 10, -1, &SerialStatusTimer);
	suspend_timer(SerialStatusTimer);

    while ( true )
    {
		
		inputLen = GetSerialCommand(serialBuf,sizeof(serialBuf));
		if (inputLen)
		{
			ProcessControlRequest(serialBuf,CONTROL_SERIAL,SerialControlResponseHandler);
			//cyg_flag_wait(&g_flagControlCallbackComplete,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);
		} 
		if (cyg_flag_poll(&g_flagStopSerial,1,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR))
		{
			
			break;
		}
	}

	
	fclose(fpSerialControlWrite);
	fclose(fpSerialControlRead);
	umount("devfs");
}

// helper functions for serial control thread

int GetSerialCommand(char * buf, int bufSize)
{
	memset(buf,0,bufSize);
	int pos = 0;
	char ch = fgetc(fpSerialControlRead);
	while (ch!=SERIAL_CR)
	{
		if (ch!=SERIAL_LF)
		{
			buf[pos] = ch;
			// if we fill the buffer, just overwrite the last character
			if (pos < bufSize-1) pos++;
		}
		ch = fgetc(fpSerialControlRead);
	}
	//fputc((int)SERIAL_CR,fpSerialControl);
	//fputc((int)SERIAL_LF,fpSerialControl);
	return pos;
}
	




