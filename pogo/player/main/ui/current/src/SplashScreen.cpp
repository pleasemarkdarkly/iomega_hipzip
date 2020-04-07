//........................................................................................
//........................................................................................
//.. File Name: SplashScreen.h																..
//.. Date: 09/10/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of CSplashScreen class				..
//.. Usage:The CSplashScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/10/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/SplashScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>

#include <main/main/Version.h>

#include <stdio.h>  // sprintf

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_SPLASH_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_SPLASH_SCREEN);

#define TIMER_SCREEN_CHANGE 411
const int sc_iScreenChangeInterval = 20;

static SplashItemRec s_SplashItem[] = 
{
	{ &gbPogoLogoBitmap, NULL },
	{ &gbFullplayBitmap, NULL },
	{ &gbFullplayBitmap, NULL },
};

CSplashScreen* CSplashScreen::s_pSplashScreen = 0;

CSplashScreen*
CSplashScreen::GetSplashScreen()
{
	if (!s_pSplashScreen)
		s_pSplashScreen = new CSplashScreen();
    return s_pSplashScreen;
}

CSplashScreen::CSplashScreen()
	: CScreen(NULL),
	m_iCurrentScreen(0),
	m_cItems(sizeof(s_SplashItem) / sizeof(SplashItemRec))
{
	BuildScreen();
}

CSplashScreen::~CSplashScreen()
{
}

// Shows screens.
void
CSplashScreen::ShowScreen()
{
    m_bActive = true;
	m_iCurrentScreen = 0;
	m_pParent->Add(this);
	Presentation()->MoveFocusTree(this);
	Draw();
	SetTimer(TIMER_SCREEN_CHANGE, sc_iScreenChangeInterval, sc_iScreenChangeInterval);
}

// Shows screens.
void
CSplashScreen::HideScreen()
{
    m_bActive = false;
	m_iCurrentScreen = 0;
	KillTimer(TIMER_SCREEN_CHANGE);
	if (Parent())
	{
        DEBUGP( DBG_SPLASH_SCREEN, DBGLEV_INFO, "deferring to %x\n",m_pControlScreen); 
		Parent()->Add(m_pControlScreen);
		Parent()->Remove(this);
	}

	Presentation()->MoveFocusTree(m_pControlScreen);
		
}

void
CSplashScreen::Draw()
{
	m_pScreenWindow->SetIcon(s_SplashItem[m_iCurrentScreen].pBitmap);
	if(m_iCurrentScreen > 0 && s_SplashItem[m_iCurrentScreen - 1].pString != NULL)
		Remove(s_SplashItem[m_iCurrentScreen - 1].pString);
	if(s_SplashItem[m_iCurrentScreen].pString != NULL)
		Add(s_SplashItem[m_iCurrentScreen].pString);
	Invalidate(mReal);

	BeginDraw();
	CScreen::Draw();
	EndDraw();
}

SIGNED
CSplashScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
		// ignore all key presses for simplicity
		case PM_KEY:
			return 0;
		case PM_KEY_RELEASE:
			return 0;
		case PM_TIMER:
			
			switch (Mesg.iData)
			{
				case TIMER_SCREEN_CHANGE:
					if(++m_iCurrentScreen < m_cItems)
						Draw();
					else
						HideScreen();
					return 0;

				default:
					return CScreen::Message(Mesg);
			}
			break;

		default:
			return CScreen::Message(Mesg);
	}
}

// Creates all the icons used by the menu screen.
void
CSplashScreen::BuildScreen()
{
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
	int iTextLength = 0, iCaptionLength = 0;
	PegRect ChildRect;

	m_pScreenWindow = new PegIcon(mReal, s_SplashItem[0].pBitmap);
	Add(m_pScreenWindow);

    // local staging strings
    TCHAR tszTemp[VERSION_NUM_SIZE];
    char szTemp[VERSION_NUM_SIZE];
    // version control object
    CVersion* version = CVersion::GetInstance();

	// construct a player version string
    tstrcpy(tszTemp, LS(SID_PLAYER_V));
    sprintf(szTemp," %d.%d",version->PlayerMajor(),version->PlayerMinor());
    CharToTchar( tszTemp + tstrlen(tszTemp), szTemp);
    // populate the peg version string
	ChildRect.Set(mReal.wLeft, mReal.wTop + 48, mReal.wRight, mReal.wTop + 57);
	m_pVersionString = new PegString(ChildRect, tszTemp, 0, FF_NONE | TT_COPY);
	m_pVersionString->SetFont(&FONT_PLAYSCREEN);
	m_pVersionString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	// center the string
	iTextLength = Screen()->TextWidth(m_pVersionString->DataGet(), m_pVersionString->GetFont());
	iCaptionLength = m_pVersionString->mReal.wRight - m_pVersionString->mReal.wLeft;
	if(iTextLength < iCaptionLength)
	{
		ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pVersionString->mReal.wTop, m_pVersionString->mReal.wRight, m_pVersionString->mReal.wBottom);
		m_pVersionString->Resize(ChildRect);
	}
    
    // construct a player copyright year string
    tstrcpy(tszTemp, LS(SID_FULLPLAY_MEDIA_C));
    sprintf(szTemp," %d",version->CopyrightYear());
    CharToTchar( tszTemp + tstrlen(tszTemp), szTemp);
    // populate peg string
	ChildRect.Set(mReal.wLeft, mReal.wTop + 48, mReal.wRight, mReal.wTop + 57);
	m_pCopyrightString = new PegString(ChildRect, tszTemp, 0, FF_NONE | TT_COPY);
	m_pCopyrightString->SetFont(&FONT_PLAYSCREEN);
	m_pCopyrightString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	// center the string
	iTextLength = Screen()->TextWidth(m_pCopyrightString->DataGet(), m_pCopyrightString->GetFont());
	iCaptionLength = m_pCopyrightString->mReal.wRight - m_pCopyrightString->mReal.wLeft;
	if(iTextLength < iCaptionLength)
	{
		ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pCopyrightString->mReal.wTop, m_pCopyrightString->mReal.wRight, m_pCopyrightString->mReal.wBottom);
		m_pCopyrightString->Resize(ChildRect);
	}

	s_SplashItem[1].pString = m_pVersionString;
	s_SplashItem[2].pString = m_pCopyrightString;
}

bool CSplashScreen::IsActive()
{
    return m_bActive;
}

void CSplashScreen::SetControlScreen(CScreen* pScreen) 
{ 
    DEBUGP( DBG_SPLASH_SCREEN, DBGLEV_INFO, "setting screen %x\n",(int)pScreen); 
    m_pControlScreen = pScreen; 
}
