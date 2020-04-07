//........................................................................................
//........................................................................................
//.. File Name: InputSelectMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CInputSelectMenuScreen class					..
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
#include <main/ui/InputSelectMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/PlayerScreen.h>

extern "C" {
#include <devs/lcd/lcd.h>
}

// (epg,2/5/2002): todo: stubs
void ADCSelectLineIn() {;}
void ADCSelectMic() {;}

// the global reference for this class
CInputSelectMenuScreen* CInputSelectMenuScreen::s_pInputSelectMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ NULL, NULL, SID_INPUT_LINE_IN, false},
	{ NULL, NULL, SID_INPUT_MIC, false},
};


// This is a singleton class.
CScreen*
CInputSelectMenuScreen::GetInputSelectMenuScreen()
{
	if (!s_pInputSelectMenuScreen) {
		s_pInputSelectMenuScreen = new CInputSelectMenuScreen(NULL);
	}
	return s_pInputSelectMenuScreen;
}


CInputSelectMenuScreen::CInputSelectMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true), m_nInputSelect(0)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_INPUT_SELECT));
}

CInputSelectMenuScreen::~CInputSelectMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CInputSelectMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case 0:		// InputSelect On
            SetInputSelect(0);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
		case 1:		// InputSelect Off
            SetInputSelect(1);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
            break;
	};
}

// 0 = line-in, 1 = mic
int CInputSelectMenuScreen::GetInputSelect()
{
    return m_nInputSelect;
}

void CInputSelectMenuScreen::SetInputSelect(int nInput)
{
    m_nInputSelect = nInput;
    if (nInput == 0)
        ADCSelectLineIn();
    else
        ADCSelectMic();
}
