//
// SplashScreen.cpp: contains the implementation of the CSplashScreen class
// danb@fullplaymedia.com 09/10/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/SplashScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <main/main/LEDStateManager.h>

const int sc_iScreenChangeInterval = 150;

/*
static SplashItemRec s_SplashItem[] = {
	{ &gbFullplayLogoBitmap, NULL },
	{ &gbIobjectsLogoBitmap, NULL },
	{ &gbIobjectsLogoBitmap, NULL },
};
*/

// show the fullplay text logo during hard start up
static SplashItemRec s_SplashItem[] = {
	{ &gbFullplayBitmap, NULL },
//	{ &gbEmptyBitmap, NULL },
//	{ &gbEmptyBitmap, NULL },
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
	m_cItems(sizeof(s_SplashItem) / sizeof(SplashItemRec)),
	m_iCurrentScreen(0)
{
	BuildScreen();
}


CSplashScreen::~CSplashScreen()
{
    // TODO since version and copyright screens aren't Add()'d to this, we have to delete them manually
    delete m_pVersionString;
    delete m_pCopyrightString;
	KillTimer(SS_TIMER_SCREEN_CHANGE);
}


// Shows screens.
void
CSplashScreen::ShowScreen()
{
	m_iCurrentScreen = 0;
	m_pParent->Add(this);
	Presentation()->MoveFocusTree(this);
	Draw();
	SetTimer(SS_TIMER_SCREEN_CHANGE, sc_iScreenChangeInterval, sc_iScreenChangeInterval);
}

// Shows screens.
void
CSplashScreen::HideScreen()
{
    //SetLEDState(POWERING_ON, false);
	m_iCurrentScreen = 0;
	KillTimer(SS_TIMER_SCREEN_CHANGE);
	if (Parent())
	{
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

    // turn the led orange
    //SetLEDState(POWERING_ON, true);

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
				case SS_TIMER_SCREEN_CHANGE:
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

	// todo: get the player verion number
	ChildRect.Set(mReal.wLeft, mReal.wTop + 46, mReal.wRight, mReal.wTop + 60);
	m_pVersionString = new PegString(ChildRect, LS(SID_PLAYER_V), 0, FF_NONE | TT_COPY);
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

	ChildRect.Set(mReal.wLeft, mReal.wTop + 46, mReal.wRight, mReal.wTop + 60);
	m_pCopyrightString = new PegString(ChildRect, LS(SID_FULLPLAY_MEDIA_C), 0, FF_NONE | TT_COPY);
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

	//s_SplashItem[1].pString = m_pVersionString;
	//s_SplashItem[2].pString = m_pCopyrightString;
}
