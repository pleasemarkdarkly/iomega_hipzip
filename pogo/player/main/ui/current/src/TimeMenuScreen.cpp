//........................................................................................
//........................................................................................
//.. File Name: TimeMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CTimeMenuScreen class					..
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
#include <main/ui/TimeMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>

#include <main/playlist/pogoplaylist/PogoPlaylist.h>

// the global reference for this class
CTimeMenuScreen* CTimeMenuScreen::s_pTimeMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ NULL, NULL, SID_TRACK_ELAPSED, false},
	{ NULL, NULL, SID_TRACK_REMAINING, false},
	{ NULL, NULL, SID_ALBUM_ELAPSED, false},
	{ NULL, NULL, SID_ALBUM_REMAINING, false},
};


// This is a singleton class.
CScreen*
CTimeMenuScreen::GetTimeMenuScreen()
{
	if (!s_pTimeMenuScreen) {
		s_pTimeMenuScreen = new CTimeMenuScreen(NULL);
	}
	return s_pTimeMenuScreen;
}


CTimeMenuScreen::CTimeMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_TIME));
}

CTimeMenuScreen::~CTimeMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CTimeMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    CPogoPlaylist* playlist = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    CPlayerScreen* playscreen = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
	switch (iMenuIndex)
	{
		case 0:		// Track Elapsed
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(TRACK_ELAPSED);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
		case 1:		// Track Remaining
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(TRACK_REMAINING);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
		case 2:		// Album Elapsed
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(ALBUM_ELAPSED);
            if (!playlist->IsModeAlbumBased())
                playscreen->DecayAlbumTimeDisplay();
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
		case 3:		// Album Elapsed
    	    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(ALBUM_REMAINING);
            if (!playlist->IsModeAlbumBased())
                playscreen->DecayAlbumTimeDisplay();
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
	};
}