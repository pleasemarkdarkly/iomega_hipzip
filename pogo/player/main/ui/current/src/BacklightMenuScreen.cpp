//........................................................................................
//........................................................................................
//.. File Name: BacklightMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBacklightMenuScreen class					..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/BacklightMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/PlayerScreen.h>

extern "C" {
#include <devs/lcd/lcd.h>
}

// the global reference for this class
CBacklightMenuScreen* CBacklightMenuScreen::s_pBacklightMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ NULL, NULL, SID_BACKLIGHT_ON, false},
	{ NULL, NULL, SID_BACKLIGHT_OFF, false},
};


// This is a singleton class.
CScreen*
CBacklightMenuScreen::GetBacklightMenuScreen()
{
	if (!s_pBacklightMenuScreen) {
		s_pBacklightMenuScreen = new CBacklightMenuScreen(NULL);
	}
	return s_pBacklightMenuScreen;
}


CBacklightMenuScreen::CBacklightMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true), m_bBacklightOn(false)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_BACKLIGHT));
}

CBacklightMenuScreen::~CBacklightMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CBacklightMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case 0:		// Backlight On
#ifdef LCD_BACKLIGHT
            g_pEvents->EnableBacklight();
#endif
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
		case 1:		// Backlight Off
#ifdef LCD_BACKLIGHT
            g_pEvents->DisableBacklight();
#endif
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
            break;
	};
}

bool CBacklightMenuScreen::GetBacklightOn()
{
    return m_bBacklightOn;
}

void CBacklightMenuScreen::SetBacklightOn(bool bOn)
{
    m_bBacklightOn = bOn;
}
