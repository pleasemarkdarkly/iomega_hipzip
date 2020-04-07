//
// CEventLogger.cpp
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
#include <fs/fat/sdapi.h>
#include <main/main/AppSettings.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef ENABLE_EVENT_LOGGING

// The global singleton debug router.
CEventRecorder* CEventRecorder::s_pSingleton = 0;

// flag to check if we are enabling debug routing at run time
bool CEventRecorder::g_LoggingEnabled = 0;

static char* arMonkeyKeyNames[] = {
  "KEY_RECORD"               ,//       1
  "KEY_PLAY"                 ,//       2
  "KEY_STOP"                 ,//       3
  "KEY_PAUSE"                ,//       4
  "KEY_FWD"                  ,//       5
  "KEY_REW"                  ,//       6
  "KEY_REFRESH_CONTENT"      ,//       7  // dharma board only
  "KEY_BREAK_POINT"          ,//       8  // dharma board only
  ""                         ,//       9  // space holder
  ""                         ,//       10  // space holder
  "KEY_MENU"                 ,//       11
  "KEY_SELECT"               ,//       12
  "KEY_DOWN"                 ,//       13
  "KEY_UP"                   ,//       14
  "KEY_EXIT"               ,//       15
  "KEY_POWER"                ,//       16
  "KEY_CD_EJECT"             ,//       17
  ""                         ,//       18  // space holder
  ""                         ,//       19  // space holder
  "IR_KEY_POWER"             ,//       20
  "IR_KEY_UP"                ,//       21
  "IR_KEY_DOWN"              ,//       22
  "IR_KEY_NEXT"              ,//       23
  "IR_KEY_PREV"              ,//       24
  "IR_KEY_SELECT"            ,//       25
  "IR_KEY_PLAY_MODE"         ,//       26
  "IR_KEY_ADD"               ,//       27
  "IR_KEY_SOURCE"            ,//       28
  "IR_KEY_MENU"              ,//       29
  "IR_KEY_REW"               ,//       30
  "IR_KEY_PLAY"              ,//       31
  "IR_KEY_FWD"               ,//       32
  "IR_KEY_RECORD"            ,//       33
  "IR_KEY_STOP"              ,//       34
  "IR_KEY_PAUSE"             ,//       35
  ""                         ,//       36  // space holder
  ""                         ,//       37  // space holder
  ""                         ,//       38  // space holder
  ""                         ,//       39  // space holder
  "IR_KEY_0_space"           ,//       40
  "IR_KEY_1_misc"            ,//       41
  "IR_KEY_2_abc"             ,//       42
  "IR_KEY_3_def"             ,//       43
  "IR_KEY_4_ghi"             ,//       44
  "IR_KEY_5_jkl"             ,//       45
  "IR_KEY_6_mno"             ,//       46
  "IR_KEY_7_pqrs"            ,//       47
  "IR_KEY_8_tuv"             ,//       48
  "IR_KEY_9_wxyz"            ,//       49
  "IR_KEY_MUTE"              ,//       50
  "IR_KEY_SHIFT"             ,//       51
  "IR_KEY_VOL_UP"            ,//       52
  "IR_KEY_VOL_DOWN"          ,//       53
  "IR_KEY_AV_POWER"          ,//       54
  "IR_KEY_AV_INPUT"          ,//       55
  "IR_KEY_CHANNEL_UP"        ,//       56
  "IR_KEY_CHANNEL_DOWN"      ,//       57
  "IR_KEY_ABC_UP"            ,//       58
  "IR_KEY_ABC_DOWN"          ,//       59  
  "IR_KEY_GENRE"             ,//       60
  "IR_KEY_ARTIST"            ,//       61
  "IR_KEY_ALBUM"             ,//       62
  "IR_KEY_PLAYLIST"          ,//       63
  "IR_KEY_RADIO"             ,//       64
  "IR_KEY_INFO"              ,//       65
  "IR_KEY_ZOOM"              ,//       66
  "IR_KEY_CLEAR"             ,//       67
  "IR_KEY_EDIT"              ,//       68
  "IR_KEY_SAVE"              ,//       69
  "IR_KEY_DELETE"            ,//       70
  "IR_KEY_EXIT"            ,//       71
  ""                         ,//       72  // space holder
  ""                         ,//       73  // space holder
  ""                         ,//       74  // space holder
  ""                         ,//       75  // space holder
  ""                         ,//       76  // space holder
  ""                         ,//       77  // space holder
  ""                         ,//       78  // space holder
  ""                         ,//       79  // space holder
  "IR_KEY_PRINT_SCREEN"       //       80
};

// Returns a pointer to the global debug router.
CEventRecorder*
CEventRecorder::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CEventRecorder;
    return s_pSingleton;
}

// Destroy the singleton debug router.
void
CEventRecorder::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}


CEventRecorder::CEventRecorder()
{
	m_LastEventTime = 0;
}

CEventRecorder::~CEventRecorder()
{

}

void 
CEventRecorder::ProcessEvent(int key, void* data )
{
	unsigned int uCurrentTime = cyg_current_time();
	unsigned int uTimeElapsed = uCurrentTime - m_LastEventTime;
	m_LastEventTime = uCurrentTime;

    // work around the debug system to force output
#if DEBUG_LEVEL != 0
	_debug_handler("EL",0,0,"\n#E# %d %x %x\n%s\n",uTimeElapsed,key,(unsigned int)data,arMonkeyKeyNames[(unsigned int)data - 1]);
#endif
    if (m_fdCurrentOutputFile >= 0)
    {
        char EventBuffer[128];
        sprintf(EventBuffer,"#E# %d %x %x\n",uTimeElapsed,key,(unsigned int)data);
        po_write(m_fdCurrentOutputFile,(UCHAR*)EventBuffer,strlen(EventBuffer));
        //have to flush every time
        po_flush(m_fdCurrentOutputFile);
    }
}



void
CEventRecorder::EnableEventLogging(bool bEnable)
{
		
	CEventRecorder::g_LoggingEnabled = bEnable;
	m_LastEventTime = cyg_current_time();

}

void
CEventRecorder::EnableEventLogFile(bool bEnable)
{
    if (m_fdCurrentOutputFile >= 0)
	{
	    po_close(m_fdCurrentOutputFile);
	}
    if (bEnable)
    {
	    m_fdCurrentOutputFile = po_open(EVENT_LOG_FILE,PO_TRUNC | PO_WRONLY | PO_CREAT | PO_NOSHAREANY,PS_IWRITE);
	    if (m_fdCurrentOutputFile < 0)
	    {
		    diag_printf("\nFailed to open Event Log File.\n");
	    }
    }
}

#endif


