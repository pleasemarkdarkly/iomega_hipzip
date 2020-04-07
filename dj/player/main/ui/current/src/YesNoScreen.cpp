//
// YesNoScreen.cpp: contains the implementation of the CAlertScreen class
//      This class is derived from the CProgressScreen class, and will be overlayed  ..
//      on the calling screen and give the user the abliity to choose yes or no to      ..
//      an operation                                                                   ..
// danb@fullplaymedia.com 12/18/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/YesNoScreen.h>

#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_YESNO_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_YESNO_SCREEN );

#define TIMEOUT_LENGTH              2000

//extern CPlayScreen* g_pMainWindow;
CYesNoScreen* CYesNoScreen::s_pYesNoScreen = 0;

// This is a singleton class.
CYesNoScreen*
CYesNoScreen::GetInstance()
{
	if (!s_pYesNoScreen) {
		s_pYesNoScreen = new CYesNoScreen(NULL);
	}
	return s_pYesNoScreen;
}

CYesNoScreen::CYesNoScreen(CScreen* pParent)
  : CProgressScreen(pParent),
  m_pfnCB(NULL),
  m_bYes(false)
{
    DEBUGP( DBG_YESNO_SCREEN, DBGLEV_TRACE, "YesNoScreen:Ctor\n");
	BuildScreen();
}

CYesNoScreen::~CYesNoScreen()
{
    DEBUGP( DBG_YESNO_SCREEN, DBGLEV_TRACE, "YesNoScreen:Dtor\n");
}

void
CYesNoScreen::Draw()
{
    DrawScreen();
    CScreen::Draw();
}

SIGNED
CYesNoScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
	case PM_KEY:

        // reset the timer on every key press
        KillTimer(YNS_TIMER_TIMEOUT);
        SetTimer(YNS_TIMER_TIMEOUT, TIMEOUT_LENGTH, 0);

        switch (Mesg.iData)
        {
        case IR_KEY_SELECT:
        case KEY_SELECT:
            DoCallback();
            break;
            
        case IR_KEY_PREV:
        case IR_KEY_UP:
        case KEY_UP:
            m_bYes = true;
            break;
            
        case IR_KEY_NEXT:
        case IR_KEY_DOWN:
        case KEY_DOWN:
            m_bYes = false;
            break;
            
        case IR_KEY_EXIT:
        case KEY_EXIT:
            m_bYes = false;
            DoCallback();
            break;
            
        case IR_KEY_MENU:
        case KEY_MENU:
            m_bYes = false;
            DoCallback();
            return 0;
            
        // ignore these keys
        case IR_KEY_ZOOM:
        case IR_KEY_INFO:
        case IR_KEY_PLAY_MODE:
            return 0;
            
        default:
            return 0;
        }
        break;

    case PM_KEY_RELEASE:
        return 0;

	case PM_TIMER:
		
		switch (Mesg.iData)
		{
		case YNS_TIMER_TIMEOUT:
            if (this == Presentation()->GetCurrentThing()) {
                m_bYes = false;
			    DoCallback();
            }
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
CYesNoScreen::Config(CScreen* pParent, YesNoCallback* pfnCB, bool bYes = false)
{
    m_pfnCB = pfnCB;
    m_bYes = bYes;
    SetTimer(YNS_TIMER_TIMEOUT, TIMEOUT_LENGTH, 0);
    SetParent(pParent);    
}

void
CYesNoScreen::DoCallback()
{
    HideScreen();
   	Presentation()->MoveFocusTree(m_pParent);
    if(m_pfnCB)
        m_pfnCB(m_bYes);
}

void
CYesNoScreen::ForceRedraw()
{
    Invalidate(mReal);
    Draw();
}

void
CYesNoScreen::DrawScreen()
{
    if(m_bYes)
    {
        m_pYesString->SetFont(&FONT_MENUSCREENENTRY_SELECTED);
        m_pNoString->SetFont(&FONT_MENUSCREENENTRY);
        m_pYesLeftArrow->SetIcon(&gbSolidRightArrowBitmap);
        m_pYesRightArrow->SetIcon(&gbSolidLeftArrowBitmap);
        m_pNoLeftArrow->SetIcon(&gbEmptyBitmap);
        m_pNoRightArrow->SetIcon(&gbEmptyBitmap);
    }
    else
    {
        m_pYesString->SetFont(&FONT_MENUSCREENENTRY);
        m_pNoString->SetFont(&FONT_MENUSCREENENTRY_SELECTED);
        m_pYesLeftArrow->SetIcon(&gbEmptyBitmap);
        m_pYesRightArrow->SetIcon(&gbEmptyBitmap);
        m_pNoLeftArrow->SetIcon(&gbSolidRightArrowBitmap);
        m_pNoRightArrow->SetIcon(&gbSolidLeftArrowBitmap);
    }
}
    
void
CYesNoScreen::BuildScreen()
{
    PegRect ChildRect;

    // remove the message text
    Remove(m_pMessageTextString);

	// yes
	ChildRect.Set(mReal.wLeft + 44, mReal.wBottom - 13, mReal.wLeft + 64, mReal.wBottom);
	m_pYesString = new PegString(ChildRect, LS(SID_YES), 0, FF_NONE | TT_COPY );
	m_pYesString->SetFont(&FONT_MENUSCREENENTRY);
	m_pYesString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pYesString);

	// no
	ChildRect.Set(mReal.wRight - 70, mReal.wBottom - 13, mReal.wRight - 53, mReal.wBottom);
	m_pNoString = new PegString(ChildRect, LS(SID_NO), 0, FF_NONE | TT_COPY );
	m_pNoString->SetFont(&FONT_MENUSCREENENTRY_SELECTED);
	m_pNoString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pNoString);

	ChildRect.Set(mReal.wLeft + 34, mReal.wBottom - 9, mReal.wLeft + 40, mReal.wBottom - 1);
	m_pYesLeftArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pYesLeftArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pYesLeftArrow);

	ChildRect.Set(mReal.wLeft + 69, mReal.wBottom - 9, mReal.wLeft + 75, mReal.wBottom - 1);
	m_pYesRightArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pYesRightArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pYesRightArrow);

    ChildRect.Set(mReal.wRight - 83, mReal.wBottom - 9, mReal.wRight - 77 , mReal.wBottom - 1);
	m_pNoLeftArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pNoLeftArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pNoLeftArrow);

    ChildRect.Set(mReal.wRight - 48, mReal.wBottom - 9, mReal.wRight - 42, mReal.wBottom - 1);
	m_pNoRightArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pNoRightArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pNoRightArrow);
}
