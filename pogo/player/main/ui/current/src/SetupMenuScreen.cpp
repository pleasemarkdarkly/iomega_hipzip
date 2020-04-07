//........................................................................................
//........................................................................................
//.. File Name: SetupMenuScreen.cpp														..
//.. Date: 09/26/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CSetupMenuScreen class						..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/26/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/SetupMenuScreen.h>
#include <main/ui/PlayOrderMenuScreen.h>
#include <main/ui/ToneMenuScreen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/BacklightMenuScreen.h>
#include <main/ui/PlayerInfoMenuScreen.h>
#include <main/ui/DiskInfoMenuScreen.h>
#include <main/ui/InputSelectMenuScreen.h>
#include <main/ui/BitrateMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

// the global reference for this class
CSetupMenuScreen* CSetupMenuScreen::s_pSetupMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ &CPlayOrderMenuScreen::GetPlayOrderMenuScreen, NULL, SID_PLAY_ORDER, true},
	{ &CToneMenuScreen::GetToneMenuScreen, NULL, SID_TONE, true},
	{ &CTimeMenuScreen::GetTimeMenuScreen, NULL, SID_TIME, true},
	{ &CBacklightMenuScreen::GetBacklightMenuScreen, NULL, SID_BACKLIGHT, true},
	{ &CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen, NULL, SID_PLAYER_INFO, true},
	{ &CDiskInfoMenuScreen::GetDiskInfoMenuScreen, NULL, SID_DISK_INFO, true},
	{ &CInputSelectMenuScreen::GetInputSelectMenuScreen, NULL, SID_INPUT_SELECT, true},
	{ &CBitrateMenuScreen::GetBitrateMenuScreen, NULL, SID_RECORDING_BITRATE, true},
};


// This is a singleton class.
CScreen*
CSetupMenuScreen::GetSetupMenuScreen()
{
	if (!s_pSetupMenuScreen) {
		s_pSetupMenuScreen = new CSetupMenuScreen(NULL);
	}
	return s_pSetupMenuScreen;
}


CSetupMenuScreen::CSetupMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), false)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);
}

CSetupMenuScreen::~CSetupMenuScreen()
{
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CSetupMenuScreen::GotoSubMenu(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case 0:		// Play Order
			Parent()->Add(CPlayOrderMenuScreen::GetPlayOrderMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CPlayOrderMenuScreen::GetPlayOrderMenuScreen());
			break;
		case 1:		// Tone
			Parent()->Add(CToneMenuScreen::GetToneMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CToneMenuScreen::GetToneMenuScreen());
			break;
		case 2:		// Time
			Parent()->Add(CTimeMenuScreen::GetTimeMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CTimeMenuScreen::GetTimeMenuScreen());
			break;
		case 3:		// Backlight
			Parent()->Add(CBacklightMenuScreen::GetBacklightMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CBacklightMenuScreen::GetBacklightMenuScreen());
			break;
		case 4:		// Player Info
			Parent()->Add(CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen());
			break;
		case 5:		// Disk Info
			Parent()->Add(CDiskInfoMenuScreen::GetDiskInfoMenuScreen());
            ((CDiskInfoMenuScreen*)CDiskInfoMenuScreen::GetDiskInfoMenuScreen())->Init();
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CDiskInfoMenuScreen::GetDiskInfoMenuScreen());
			break;
        case 6:     // Input Select
			Parent()->Add(CInputSelectMenuScreen::GetInputSelectMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CInputSelectMenuScreen::GetInputSelectMenuScreen());
            break;
        case 7:     // Recording Bitrate
			Parent()->Add(CBitrateMenuScreen::GetBitrateMenuScreen());
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(CBitrateMenuScreen::GetBitrateMenuScreen());
            break;
	};
}
