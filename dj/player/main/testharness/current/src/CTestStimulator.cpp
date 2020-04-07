//
// CTEstStimulator.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//



#include <main/testharness/testharness.h>
#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>
#include <util/diag/diag.h>
#include <util/datastructures/SimpleList.h>
#include <fs/fat/sdapi.h>
#include <util/eventq/EventQueueAPI.h>
#include <stdlib.h>
#include <stdio.h>
#include <main/ui/keys.h>
#include <core/events/SystemEvents.h>
#include <main/main/AppSettings.h>

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
static char			s_MonkeyBuffer[128];	//should be plenty of room.
void inject_thread_entry( cyg_uint32 data );

// global list of current events from the current script
static SimpleList<CInjectEvent*> g_slInjectEvents;

// flags for communicating with the inject events thread
static cyg_flag_t g_flagInjectEvents;

// g_flagInjectEvents can take these values:
#define FLAG_INJECT_EVENTS_START	0x01
#define FLAG_INJECT_EVENTS_STOP		0x02
#define FLAG_INJECT_EVENTS_EXIT     0x04
#define FLAG_INJECT_FROM_SCRIPT     0x10
#define FLAG_INJECT_MONKEY			0x20

static int g_iHoldDelay, g_iMaxEventDelay, g_iMaxHoldCount, g_iMinEventDelay, g_iMinHoldCount;

int GetMonkeyKey();
int GetMonkeyHoldCount();
int GetMonkeyDelayTime();

// time to delay between successive "holds" in ticks
#define MONKEY_INTRA_HOLD_DELAY		80

// keys that the monkey can use
#define MAX_MONKEY_KEYS  61

int iCountMonkeyKeys = 0;
// see main/ui/keys.h
int arMonkeySelectedKeys[MAX_MONKEY_KEYS];

static int arMonkeyKeyValues[MAX_MONKEY_KEYS] = {
  KEY_RECORD               ,//       1
  KEY_PLAY                 ,//       2
  KEY_STOP                 ,//       3
  KEY_PAUSE                ,//       4
  KEY_FWD                  ,//       5
  KEY_REW                  ,//       6
  KEY_REFRESH_CONTENT      ,//       7  // dharma board only
  KEY_BREAK_POINT          ,//       8  // dharma board only
  KEY_MENU                 ,//       11
  KEY_SELECT               ,//       12
  KEY_DOWN                 ,//       13
  KEY_UP                   ,//       14
  KEY_EXIT               ,//       15
  KEY_POWER                ,//       16
  KEY_CD_EJECT             ,//       17
  IR_KEY_POWER             ,//       20
  IR_KEY_UP                ,//       21
  IR_KEY_DOWN              ,//       22
  IR_KEY_NEXT              ,//       23
  IR_KEY_PREV              ,//       24
  IR_KEY_SELECT            ,//       25
  IR_KEY_PLAY_MODE         ,//       26
  IR_KEY_ADD               ,//       27
  IR_KEY_SOURCE            ,//       28
  IR_KEY_MENU              ,//       29
  IR_KEY_REW               ,//       30
  IR_KEY_PLAY              ,//       31
  IR_KEY_FWD               ,//       32
  IR_KEY_RECORD            ,//       33
  IR_KEY_STOP              ,//       34
  IR_KEY_PAUSE             ,//       35
  IR_KEY_1_misc            ,//       41
  IR_KEY_2_abc             ,//       42
  IR_KEY_3_def             ,//       43
  IR_KEY_4_ghi             ,//       44
  IR_KEY_5_jkl             ,//       45
  IR_KEY_6_mno             ,//       46
  IR_KEY_7_pqrs            ,//       47
  IR_KEY_8_tuv             ,//       48
  IR_KEY_9_wxyz            ,//       49
  IR_KEY_0_space           ,//       40
  IR_KEY_MUTE              ,//       50
  IR_KEY_SHIFT             ,//       51
  IR_KEY_VOL_UP            ,//       52
  IR_KEY_VOL_DOWN          ,//       53
  IR_KEY_AV_POWER          ,//       54
  IR_KEY_AV_INPUT          ,//       55
  IR_KEY_CHANNEL_UP        ,//       56
  IR_KEY_CHANNEL_DOWN      ,//       57
  IR_KEY_GENRE             ,//       60
  IR_KEY_ARTIST            ,//       61
  IR_KEY_ALBUM             ,//       62
  IR_KEY_PLAYLIST          ,//       63
  IR_KEY_RADIO             ,//       64
  IR_KEY_INFO              ,//       65
  IR_KEY_ZOOM              ,//       66
  IR_KEY_CLEAR             ,//       67
  IR_KEY_EDIT              ,//       68
  IR_KEY_SAVE              ,//       69
  IR_KEY_DELETE            ,//       70
  IR_KEY_EXIT             //       71
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
    m_iFileCount = 0;
    m_iRetryCount = 0;
    m_bRetry = false;
    m_bReproAttempted = false;
	cyg_flag_init(&g_flagInjectEvents);

	cyg_thread_create( 3 /* The same as KEYBOARD_THREAD_PRIORITY */, inject_thread_entry, 0, "event inject thread",
	                   (void*)tstackInjectThread, INJECT_THREAD_STACKSIZE, &threadhInjectThread, &threadInjectThread);
	cyg_thread_resume(threadhInjectThread);

}

CTestStimulator::~CTestStimulator()
{
	g_slInjectEvents.Clear();
    cyg_thread_suspend(threadhInjectThread);
    while( !cyg_thread_delete(threadhInjectThread) ) {
        cyg_thread_delay( 1 );
    }
	cyg_flag_destroy(&g_flagInjectEvents);
}


void
CTestStimulator::StartMonkeyTest(int iHoldDelay, int iMaxEventDelay, int iMaxHoldCount, int iMinEventDelay, int iMinHoldCount, unsigned int uEventMask, unsigned int uEventMaskHi)
{

	int i = 0;
	iCountMonkeyKeys = 0;

	for (i = 0; i < 32; i++)
	{
		if ((uEventMask >> i) & 1)
		{
			arMonkeySelectedKeys[iCountMonkeyKeys++] = i;
		}
	}

	for (i = 0; i < 32; i++)
	{
		if ((uEventMaskHi >> i) & 1)
		{
			arMonkeySelectedKeys[iCountMonkeyKeys++] = i+32;
		}
	}

	g_iHoldDelay = iHoldDelay;
	g_iMaxEventDelay=iMaxEventDelay;
	g_iMaxHoldCount=iMaxHoldCount;
	g_iMinEventDelay=iMinEventDelay;
	g_iMinEventDelay=iMinHoldCount;

	CDeviceState::GetInstance()->SetDeviceState(CDeviceState::DOING_MONKEY);

	cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_START | FLAG_INJECT_MONKEY);
}

void
CTestStimulator::StopTest()
{
	DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Signalling event inject thread to stop.\n");
    m_iFileCount = 0;
	cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP);
    if(m_bRetry || m_iRetryCount)
        RemoveRetryFile();
	CDeviceState::GetInstance()->SetDeviceState(CDeviceState::BUSY);
}

void CTestStimulator::RemoveRetryFile()
{
    m_iRetryCount = 0;
    CEventRecorder::GetInstance()->EnableEventLogFile(false);
    UINT16 attribs;
    if ((pc_get_attributes(TEST_RETRY_FILE, &attribs) == YES))
    {
        DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Found Retry - attempting to delete.\n");
        if(!pc_unlink(TEST_RETRY_FILE))
        {
            DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Unable to delete TestFile\n");
        }
        else
        {
            DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "TestFile deleted.\n");
        }
    }
}

void
CTestStimulator::ExecuteScript(const char *szFilepath)
{
	CDeviceState::GetInstance()->SetDeviceState(CDeviceState::RUNNING_SCRIPT);

	unsigned char * szFileContents = 0;

	int fd = po_open(szFilepath,PO_RDONLY,PS_IREAD);

	STAT filestats;
	int statsOK=0;
	
	if (fd>0)
	{
		statsOK = pc_fstat(fd,&filestats);
	}

	DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"fd %d  filesize %d\n",fd,filestats.st_size);

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
                po_close(fd);
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

					//if we get to the word monkey we want to copy this line to a buffer
					//and stop processing this file.  We will monkey after all previous commands
					//have been executed.  We will create a special key value t osignal this.
					if(!strncmp(pBuf, " monkey", 7))
					{
						pBuf += 7;
						char c = *pBuf;
						int i;
						for(i = 0;c != '\n' && c != '\0';i++)
						{
							s_MonkeyBuffer[i] = c;
							c = *(++pBuf);
						}
						s_MonkeyBuffer[i] = '\0';
						g_slInjectEvents.PushBack(new CInjectEvent(0,0,0));
						break;
					}
	
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

void
CTestStimulator::ExecuteMultipleScripts(int iFileCount, bool bRandom, bool bRepeat, int iRetryCount)
{
    if(iFileCount)
    {
        m_iFileCount = iFileCount;
        m_bRandom = bRandom;
        m_bRepeat = bRepeat;
        if(iRetryCount)
        {
            m_bRetry = true;
            CEventRecorder::GetInstance()->EnableEventLogFile(true);
            int h_TestRetryFile;
            char RetryTimes[10];
            sprintf(RetryTimes,"%d",iRetryCount);
            if((h_TestRetryFile = po_open(TEST_RETRY_FILE, PO_TRUNC | PO_WRONLY | PO_CREAT | PO_NOSHAREANY,PS_IWRITE)) != -1)
            {
                po_write(h_TestRetryFile,(UCHAR*)RetryTimes,strlen(RetryTimes));
                po_close(h_TestRetryFile);
                m_bReproAttempted = false;
                DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"created test retry file\n");
            }
        }

        m_iCurrentFile = 0;

        srand(cyg_current_time());

        ExecuteNext();
    }
}

void
CTestStimulator::ExecuteNext()
{
    if(m_iRetryCount)
    {
        m_iRetryCount--;
        DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Begin Repro Attempt\n");
        ExecuteScript(EVENT_LOG_FILE);
        return;
    }
    if(m_bReproAttempted)
    {
        m_bReproAttempted = false;
        RunStartupScript();
        return;
    }
    if(!m_iFileCount)
        return;
    if(m_bRandom)
    {
        m_iCurrentFile = rand()%m_iFileCount + 1;
    }
    else
    {
        if(m_iCurrentFile < m_iFileCount)
            m_iCurrentFile++;
        else
        {
            if(m_bRepeat)
                m_iCurrentFile = 1;
            else
            {
                if(m_bRetry)
                    RemoveRetryFile();
                m_iCurrentFile = 0;
                m_iFileCount = 0;
                return;
            }
        }
    }

    char szTestScript[EMAXPATH];
    sprintf(szTestScript, "%s/Script-%d.txt",SYSTEM_FOLDER,m_iCurrentFile);
    ExecuteScript(szTestScript);
}


void CTestStimulator::RunStartupScript()
{
	UINT16 attribs;
    if ((pc_get_attributes(TEST_RETRY_FILE, &attribs) == YES))
    {
        DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Test Retry File found\n");
        unsigned char* szFileContents = 0;
		int fd = po_open(TEST_RETRY_FILE,PO_RDONLY,PS_IREAD);
		STAT filestats;
		int statsOK = pc_fstat(fd,&filestats);
		szFileContents = (unsigned char*)malloc(filestats.st_size + 1);
		po_read(fd,szFileContents,filestats.st_size);
		po_close(fd);
		szFileContents[filestats.st_size] = '\0';
        int NumRetry;

        const char* szFilename = EVENT_LOG_FILE;
        // If this file doesn't parse the way we want it to, assume it's just an event sequence
        if (sscanf((char*)szFileContents,"%d",&NumRetry) < 1)
        {
            NumRetry = 1;
            szFilename = TEST_RETRY_FILE;
        }
        
        DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Attempting to repro %d times\n", NumRetry);
        m_iRetryCount = NumRetry - 1;
        DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Begin Repro Attempt\n");
        m_bReproAttempted = true;
        ExecuteScript(szFilename);
        RemoveRetryFile();
    }
	else if ((pc_get_attributes(TEST_RERUN_FILE, &attribs) == YES))
	{
		DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "TestFile found\n");
		unsigned char* szFileContents = 0;
		int fd = po_open(TEST_RERUN_FILE,PO_RDONLY,PS_IREAD);
		STAT filestats;
		int statsOK = pc_fstat(fd,&filestats);
		szFileContents = (unsigned char*)malloc(filestats.st_size + 1);
		po_read(fd,szFileContents,filestats.st_size);
		po_close(fd);
		szFileContents[filestats.st_size] = '\0';
		if(!strncmp("monkey",(char*)szFileContents,6))
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Repro monkey Test\n");
			int iHoldDelay,iMaxEventDelay,iMaxHoldCount,iMinEventDelay,iMinHoldCount;
			unsigned int uEventMask, uEventMaskHi;
			char Dummy[10];
			if (sscanf((char*)szFileContents,"%s %d %d %d %d %d %x %x",Dummy,&iHoldDelay,&iMaxEventDelay,&iMaxHoldCount,&iMinEventDelay,&iMinHoldCount,&uEventMask,&uEventMaskHi)==8)
			{
				StartMonkeyTest(iHoldDelay,iMaxEventDelay,iMaxHoldCount,iMinEventDelay,iMinHoldCount,uEventMask,uEventMaskHi);
			}
		}
		else if(!strncmp("script",(char*)szFileContents,6))
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Repro script Test\n");
			char FileName[EMAXPATH];
			char Dummy[10];
			if (sscanf((char*)szFileContents,"%s %s",Dummy,FileName) == 2)
			{
				ExecuteScript(FileName);
			}
		}
        else if(!strncmp("multi",(char*)szFileContents,5))
        {
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Repro multi Test\n");
            int iNumFiles, iRandom, iRepeat, iRetrys;
			char Dummy[10];
			if (sscanf((char*)szFileContents,"%s %d %d %d %d",Dummy,&iNumFiles,&iRandom,&iRepeat,&iRetrys) == 5)
			{
				ExecuteMultipleScripts(iNumFiles, iRandom, iRepeat, iRetrys);
			}
		}
		else
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "Unknown type of Test.cfg %s\n", szFileContents);
            // since this file is bad, unlink it
            pc_unlink( TEST_RERUN_FILE );
		}
		if(szFileContents)
			free(szFileContents);
	}
	else
	{
		DEBUGP(TESTSTIMULATOR, DBGLEV_INFO, "No TestFile found\n");
	}
}

void inject_thread_entry( cyg_uint32 data ) 
{
	cyg_flag_value_t flagValue;
	SimpleListIterator<CInjectEvent*> itBlah;

	// variables for the monkey events

	int mkKey;
	int mkHoldCount;
	int mkDelayTime;

    bool bMonkeyFound = false;

	int i;

	while (1)
	{
		// wait for the "start" flag
		flagValue = cyg_flag_wait(&g_flagInjectEvents,FLAG_INJECT_EVENTS_START|FLAG_INJECT_EVENTS_EXIT,
            CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);

		if (flagValue & FLAG_INJECT_FROM_SCRIPT)
		{
			// now start injecting the events from g_slInjectEvents
			// we assume this has been built for us before we are given the start signal
	
			itBlah = g_slInjectEvents.GetHead(); 
	
			
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to start. %d events in queue.\n",g_slInjectEvents.Size());
			CDeviceState::GetInstance()->SetDeviceState(CDeviceState::RUNNING_SCRIPT);
	
			// wait for the required timeout, or for the stop signal
	
			bool bStartMonkey = false;
			while ((itBlah != g_slInjectEvents.GetEnd()) && 
					(!cyg_flag_poll(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR)))
			{
				if((*itBlah)->m_iKey == 0)
				{
					diag_printf("monkey key (0) found\n");
					bStartMonkey = true;
					break;
				}
				else
				{
					cyg_thread_delay((*itBlah)->m_iDelayTime);
					CEventQueue::GetInstance()->PutEvent((*itBlah)->m_iKey, (void*)(*itBlah)->m_iParam);
					++itBlah;
				}
			}
            
			if(bStartMonkey)
			{
                bMonkeyFound = true;
				cyg_flag_setbits(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP);
				int iHoldDelay, iMaxEventDelay, iMaxHoldCount, iMinEventDelay, iMinHoldCount;
				unsigned int uEventMask, uEventMaskHi;
				sscanf(s_MonkeyBuffer, "%d %d %d %d %d %u %u", &iMinEventDelay, &iMaxEventDelay, &iMinHoldCount, &iMaxHoldCount, &iHoldDelay, &uEventMask, &uEventMaskHi);
				CTestStimulator::GetInstance()->StartMonkeyTest(iHoldDelay, iMaxEventDelay, iMaxHoldCount, iMinEventDelay, iMinHoldCount, uEventMask, uEventMaskHi);
			}
				
			//CDeviceState::GetInstance()->SetDeviceState(CDeviceState::BUSY);
			// either we are finished, or we were told to stop.  Now go back and wait for the start signal.
		}
		else if (flagValue & FLAG_INJECT_MONKEY)
		{
			DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to start. Monkey mode.\n");
			CDeviceState::GetInstance()->SetDeviceState(CDeviceState::DOING_MONKEY);

			// monkey events are random from arMonkeyKeys in this format
			// EVENT_KEY_PRESS  EVENT_KEY_HOLD[0..n] EVENT_KEY_RELEASE

			mkKey = GetMonkeyKey();				
			mkHoldCount = GetMonkeyHoldCount();  
			mkDelayTime = GetMonkeyDelayTime();

			while (!cyg_flag_poll(&g_flagInjectEvents,FLAG_INJECT_EVENTS_STOP,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR))
			{
                print_mem_usage();
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
        else if( flagValue & FLAG_INJECT_EVENTS_EXIT )
        {
            DEBUGP(TESTSTIMULATOR, DBGLEV_WARNING, "Event inject thread signalled to exit.\n");
            break;
        }

		DEBUGP(TESTSTIMULATOR, DBGLEV_INFO,"Event inject thread signalled to stop.\n");
		CDeviceState::GetInstance()->SetDeviceState(CDeviceState::IDLE);
        if(!bMonkeyFound)
        {
            CTestStimulator::GetInstance()->ExecuteNext();
        }

	}
}


// change these functions for more sophisticated distributions


int GetMonkeyKey()
{
	srand(cyg_current_time());
	return arMonkeyKeyValues[arMonkeySelectedKeys[(rand()%iCountMonkeyKeys)]];
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








