//
// AlertScreen.cpp: contains the implementation of the CAlertScreen class
// danb@fullplaymedia.com 03/13/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/AlertScreen.h>

#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_ALERT_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_ALERT_SCREEN );

//extern CPlayScreen* g_pMainWindow;
CAlertScreen* CAlertScreen::s_pAlertScreen = 0;

// This is a singleton class.
CAlertScreen*
CAlertScreen::GetInstance()
{
	if (!s_pAlertScreen) {
		s_pAlertScreen = new CAlertScreen(NULL);
	}
	return s_pAlertScreen;
}

CAlertScreen::CAlertScreen(CScreen* pParent)
  : CProgressScreen(pParent),
  m_pfnTimeoutCB(NULL),
  m_pfnCancelCB(NULL)
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:Ctor\n");
	BuildScreen();
}

CAlertScreen::~CAlertScreen()
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:Dtor\n");
}

void
CAlertScreen::HideScreen()
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:HideScreen\n");
    KillTimer(AS_TIMER_TIMEOUT);
    m_pfnCancelCB = NULL;
    m_pfnTimeoutCB = NULL;
    m_pScreenTitle->DataSet(NULL);
    m_pActionTextString->DataSet(NULL);
    m_pMessageTextString->DataSet(NULL);
    ResetProgressBar();
	if (m_pParent)
    {
	    if (this == Presentation()->GetCurrentThing())
            Presentation()->MoveFocusTree(m_pParent);
		m_pParent->Remove(this);
    }
}

SIGNED
CAlertScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:Message\n");
	switch (Mesg.wType)
	{
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        case IR_KEY_EXIT:
        case KEY_EXIT:
            // If a cancel callback has been set, then call the function and hide the screen.
            if (m_pfnCancelCB)
            {
                (*m_pfnCancelCB)();
                HideScreen();
            }
            return 0;
            
        default:
            return 0;
        }
        return 0;
        
    case PM_KEY_RELEASE:
        return 0;

	case PM_TIMER:
		
		switch (Mesg.iData)
		{
		case AS_TIMER_TIMEOUT:
            DoCallback();
			return 0;

        default:
            return CScreen::Message(Mesg);
    	}

    default:
        return CScreen::Message(Mesg);
	}

    ForceRedraw();
    return 0;
}


void
CAlertScreen::Config(CScreen* pParent, int iTimeout, FNTimeoutCallback* pfnCB)
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:Config2\n");
    m_pScreenTitle->DataSet(NULL);
    m_pActionTextString->DataSet(NULL);
    m_pMessageTextString->DataSet(NULL);
    ResetProgressBar();
    m_pfnTimeoutCB = pfnCB;
    SetParent(pParent);
    if(iTimeout > 0)
        SetTimer(AS_TIMER_TIMEOUT, iTimeout, 0);
    else
        KillTimer(AS_TIMER_TIMEOUT);
}


void
CAlertScreen::DoCallback()
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:DoCallback\n");
    if(m_pfnTimeoutCB)
        m_pfnTimeoutCB();
    HideScreen();
}


void
CAlertScreen::ForceRedraw()
{
    DEBUGP( DBG_ALERT_SCREEN, DBGLEV_TRACE, "AlertScreen:ForceRedraw\n");
    Invalidate(mReal);
    Draw();
}


void
CAlertScreen::BuildScreen()
{
}
