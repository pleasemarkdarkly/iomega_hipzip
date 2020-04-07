//........................................................................................
//........................................................................................
//.. File Name: SystemMessageScreen.h																..
//.. Date: 10/02/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of CSystemMessageScreen class				..
//.. Usage:The CSystemMessageScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 10/02/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/ui/SystemMessageScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>

#include <stdlib.h>   // exit

#include <main/main/Events.h>

#define TIMER_SCREEN_CHANGE 999
const int sc_iScreenChangeInterval = 40;

extern CEvents* g_pEvents;

CSystemMessageScreen* CSystemMessageScreen::s_pSystemMessageScreen = 0;

CSystemMessageScreen*
CSystemMessageScreen::GetSystemMessageScreen()
{
	if (!s_pSystemMessageScreen)
		s_pSystemMessageScreen = new CSystemMessageScreen();
    return s_pSystemMessageScreen;
}


CSystemMessageScreen::CSystemMessageScreen()
	: CScreen(NULL)
{
	BuildScreen();
}


CSystemMessageScreen::~CSystemMessageScreen()
{
}


// Shows screens.
void
CSystemMessageScreen::ShowScreen(CScreen* pScreen, eSystemMessage eSysMesg, const TCHAR* pszMesg)
{
	// there has to be a screen to get back to...	
	if(!pScreen)
		return;  	
	SetParent(pScreen);
	SetScreenData(eSysMesg, pszMesg);
	Presentation()->Add(this);
	Presentation()->MoveFocusTree(this);
	Draw();
	SetTimer(TIMER_SCREEN_CHANGE, sc_iScreenChangeInterval, sc_iScreenChangeInterval);
}


void
CSystemMessageScreen::SetScreenData(eSystemMessage eSysMesg, const TCHAR* pszMesg)
{
	PegRect ChildRect;
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wLeft, mReal.wTop);
	PegBitmap* pBitmap = &gbEmptyBitmap;

	m_eSysMesg = eSysMesg;

	// set up what message we want to show.
	switch(eSysMesg)
	{
		case SHUTTING_DOWN:
			m_pString->DataSet(LS(SID_SHUTTING_DOWN));
			break;
		case INVALID_PLAYLIST:
			m_pString->DataSet(LS(SID_INVALID_PLAYLIST));
			break;
		case TEXT_MESSAGE:
		default:
			m_pString->DataSet(pszMesg);
			break;
	}

	// place the icon correctly
	m_pIcon->SetIcon(pBitmap);
	m_pIcon->Resize(ChildRect);

	// center the string
	int iTextLength = Screen()->TextWidth(m_pString->DataGet(), m_pString->GetFont());
	int iCaptionLength = m_StringRect.wRight - m_StringRect.wLeft;
	if(iTextLength < iCaptionLength)
		ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_StringRect.wTop, m_StringRect.wRight, m_StringRect.wBottom);
	else
		ChildRect = m_StringRect;
	m_pString->Resize(ChildRect);
}


// Shows screens.
void
CSystemMessageScreen::HideScreen()
{
	// todo:  this has the potential to do weird things...  what if the partent screen of this
	// window goes away while this screen is active?   eg, what if a 
	KillTimer(TIMER_SCREEN_CHANGE);
	Presentation()->Remove(this);
	Presentation()->MoveFocusTree(m_pParent);
		
}


void
CSystemMessageScreen::Draw()
{
	BeginDraw();
	CScreen::Draw();
	EndDraw();
}


SIGNED
CSystemMessageScreen::Message(const PegMessage &Mesg)
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
					if(m_eSysMesg == LOW_POWER)
					{
						SetScreenData(SHUTTING_DOWN);
						Draw();
					}
					else if(m_eSysMesg == SHUTTING_DOWN)
					{
						KillTimer(TIMER_SCREEN_CHANGE);
						// save player state to disk
                                                g_pEvents->SaveState();
                                                // shutdown the player.
                                                // (epg,11/1/2001): TODO: impl
                                                exit(0);
					}
					else if(m_eSysMesg == USB_CONNECTED)
					{
                                            // this timer is more for the other modes, but it is convenient to just 
                                            // set it for all modes and kill it here for usb.
						KillTimer(TIMER_SCREEN_CHANGE);
                                        }
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
CSystemMessageScreen::BuildScreen()
{
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
	PegRect ChildRect;

	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wLeft, mReal.wTop);
	m_pIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pIcon);

	m_StringRect.Set(mReal.wLeft, mReal.wTop + 20, mReal.wRight, mReal.wTop + 29);
	m_pString = new PegString(m_StringRect, 0, 0, FF_NONE | TT_COPY);
	m_pString->SetFont(&FONT_PLAYSCREEN);
	m_pString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pString);
}
