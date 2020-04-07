//
// CTEstStimulator.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//



#include <main/testharness/testharness.h>
#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>
#include <util/datastructures/SimpleList.h>
#include <fs/fat/sdapi.h>
#include <util/eventq/EventQueueAPI.h>
#include <stdlib.h>
#include <stdio.h>
#include <main/ui/keys.h>
#include <core/events/SystemEvents.h>

DEBUG_MODULE_S(TESTSTIMULATOR, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(TESTSTIMULATOR);

class CInjectEvent
{
public:
	CInjectEvent::CInjectEvent(unsigned int iDelayTime, int iKey, int iParam)
		{m_iDelayTime = iDelayTime; m_iKey = iKey; m_iParam = iParam;};
	CInjectEvent::~CInjectEvent() { };
	unsigned int m_iDelayTime;
	int			 m_iKey;
	int          m_iParam;
};

#define INJECT_THREAD_STACKSIZE   8192*4

static cyg_handle_t threadhInjectThread;
static cyg_thread   threadInjectThread;
static char         tstackInjectThread[INJECT_THREAD_STACKSIZE];
void inject_thread_entry( cyg_uint32 data );

// global list of current events from the current script
static SimpleList<CInjectEvent*> g_slInjectEvents;

// flags for communicating with the inject events thread
static cyg_flag_t g_flagInjectEvents;

// g_flagInjectEvents can take these values:
#define FLAG_INJECT_EVENTS_START	0x01
#define FLAG_INJECT_EVENTS_STOP		0x02
#define FLAG_INJECT_FROM_SCRIPT     0x10
#define FLAG_INJECT_MONKEY			0x20

static int g_iHoldDelay, g_iMaxEventDelay, g_iMaxHoldCount, g_iMinEventDelay, g_iMinHoldCount;

int GetMonkeyKey();
int GetMonkeyHoldCount();
int GetMonkeyDelayTime();

// time to delay between successive "holds" in ticks
#define MONKEY_INTRA_HOLD_DELAY		80

// keys that the monkey can use
#define COUNT_MONKEY_KEYS			14
// see main/ui/keys.h
static int arMonkeyKeys[COUNT_MONKEY_KEYS] = {
    KEY_PLAY_PAUSE      ,
    KEY_STOP            ,
    KEY_PREVIOUS        ,
    KEY_NEXT            ,
    KEY_UP              ,
    KEY_DOWN            ,
    KEY_MENU            ,
    KEY_RECORD          ,
    KEY_DIAL_IN         ,
    KEY_DIAL_UP         ,
    KEY_DIAL_DOWN       ,
};

// The global singleton debug router.
CTestStimulator* CTestStimulator::s_pSingleton = 0;



// Returns a pointer to the global test stimulator.
CTestStimulator*
CTestStimulator::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CTestStimulator;
    return s_pSingleton;
}

// Destroy the singleton debug router.
void
CTestStimulator::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}


CTestStimulator::CTestStimulator()
{
	cyg_flag_init(&g_flagInjectEvents);

	cyg_thread_create( 10, inject_thread_entry, 0, "event inject thread",
	                   (void*)tstackInjectThread, INJECT_THREAD_STACKSIZE, &threadhInjectThread, &threadInjectThread);
	cyg_thread_resume(threadhInjectThread);

}

CTestStimulator::~CTestStimulator()
{
	g_slInjectEvents.Clear();
	cyg_thread_kill(threadhInjectThread);
	cyg_flag_destroy(&g_flagInjectEvents);
}


void
CTestStimulator::StartMonkeyTest(int iHoldDelay, int iMaxEventDelay, int iMaxHoldCount, int iMinEventDelay, int iMinHoldCount)
{
	g_iHoldDelay = iHoldDelay;
	g_iMaxEventDelay=iMaxEventDelay;
	g_iMaxHoldCount=iMaxHoldCount;
	g_iMinEventDelay=iMinEventDelay;
	g_iMinEventDelay=iMinHoldCount;

	cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_START | FLAG_INJECT_MONKEY);
}

void
CTestStimulator::StopTest()
{
	DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Signalling event inject thread to stop.\n");
	cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP);
}

void
CTestStimulator::ExecuteScript(char *szFilepath)
{
	unsigned char * szFileContents = 0;

	int fd = po_open(szFilepath,PO_RDONLY,PS_IREAD);

	STAT filestats;
	int statsOK=0;
	
	if (fd>0)
	{
		statsOK = pc_fstat(fd,&filestats);
	}

	DEBUGP(TESTSTIMULATOR, DBGLEV_ERROR,"fd %d  filesize %d\n",fd,filestats.st_size);

	if (fd>0)
	{

		szFileContents = (unsigned char*)malloc(filestats.st_size);

		if (!szFileContents)
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_ERROR,"Could not allocate memory for script file. Abandoning script execution.\n");
		}
		else
		{

			if (po_read(fd,szFileContents,filestats.st_size)==filestats.st_size)
			{
	
				unsigned int iDelayTime;
				int iKey, iParam;
	
				// clear any previous events
				g_slInjectEvents.Clear();
				int res;

				char * pBuf = (char*)szFileContents;

				pBuf = strstr((char*)szFileContents,"#E#");
				while (pBuf)
				{
					pBuf += 3;
	
					res = sscanf(pBuf,"%d %x %x",&iDelayTime,&iKey,&iParam);
								
					if (res ==3) g_slInjectEvents.PushBack(new CInjectEvent(iDelayTime,iKey,iParam));

					pBuf = strstr(pBuf,"#E#");

				}
				DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Script file %s parsed successfully. Starting script execution.\n",szFilepath);
				cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_START | FLAG_INJECT_FROM_SCRIPT);
			}
			else
			{
				DEBUGP(TESTSTIMULATOR, DBGLEV_ERROR,"Read size does not match file stats for %s. Abandoning script execution.\n",szFilepath);
			}

		}
	}
	else
	{
		DEBUGP(TESTSTIMULATOR, DBGLEV_ERROR,"Could not open file or get file information for %s Abandoning script execution.\n",szFilepath);
	}
	if (szFileContents)
		free(szFileContents);
}

void inject_thread_entry( cyg_uint32 data ) 
{
	cyg_flag_value_t flagValue;
	SimpleListIterator<CInjectEvent*> itBlah;

	// variables for the monkey events

	int mkKey;
	int mkHoldCount;
	int mkDelayTime;

	int i;

	while (1)
	{
		// wait for the "start" flag
		flagValue = cyg_flag_wait(&g_flagInjectEvents,FLAG_INJECT_EVENTS_START,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);

		if (flagValue & FLAG_INJECT_FROM_SCRIPT)
		{
			// now start injecting the events from g_slInjectEvents
			// we assume this has been built for us before we are given the start signal
	
			itBlah = g_slInjectEvents.GetHead(); 
	
			
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to start. %d events in queue.\n",g_slInjectEvents.Size());
	
			// wait for the required timeout, or for the stop signal
	
			while ((itBlah != g_slInjectEvents.GetEnd()) && 
					(!cyg_flag_poll(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR)))
			{
				cyg_thread_delay((*itBlah)->m_iDelayTime);
				CEventQueue::GetInstance()->PutEvent((*itBlah)->m_iKey, (void*)(*itBlah)->m_iParam);
				++itBlah;
			}
			// either we are finished, or we were told to stop.  Now go back and wait for the start signal.
		}
		else if (flagValue & FLAG_INJECT_MONKEY)
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to start. Monkey mode.\n");
			// monkey events are random from arMonkeyKeys in this format
			// EVENT_KEY_PRESS  EVENT_KEY_HOLD[0..n] EVENT_KEY_RELEASE

			mkKey = GetMonkeyKey();				
			mkHoldCount = GetMonkeyHoldCount();  
			mkDelayTime = GetMonkeyDelayTime();

			while (!cyg_flag_poll(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR))
			{
				cyg_thread_delay(mkDelayTime);
				CEventQueue::GetInstance()->PutEvent(EVENT_KEY_PRESS, (void*)mkKey);
				cyg_thread_delay(g_iHoldDelay);
				for (i = 0; i < mkHoldCount; i++)
				{
					CEventQueue::GetInstance()->PutEvent(EVENT_KEY_HOLD, (void*)mkKey);
					cyg_thread_delay(g_iHoldDelay);
				}
				CEventQueue::GetInstance()->PutEvent(EVENT_KEY_RELEASE, (void*)mkKey);

				mkKey = GetMonkeyKey();				
				mkHoldCount = GetMonkeyHoldCount();  
				mkDelayTime = GetMonkeyDelayTime();
			}

		}

		DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to stop.\n");
	}

}


// change these functions for more sophisticated distributions


int GetMonkeyKey()
{
	srand(cyg_current_time());
	return arMonkeyKeys[(rand()%COUNT_MONKEY_KEYS)];
}

int GetMonkeyHoldCount()
{
	srand(cyg_current_time());
	return g_iMinHoldCount+(rand()%(g_iMaxHoldCount-g_iMinHoldCount));
}

int GetMonkeyDelayTime()
{
	srand(cyg_current_time());
	return g_iMinEventDelay + (rand()%(g_iMaxEventDelay-g_iMinEventDelay));
}








