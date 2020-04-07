//........................................................................................
//........................................................................................
//.. File Name: PlayOrderMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CPlayOrderMenuScreen class					..
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
#include <main/ui/PlayOrderMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <core/playmanager/PlayManager.h>
#include <main/ui/PlayerScreen.h>

// the global reference for this class
CPlayOrderMenuScreen* CPlayOrderMenuScreen::s_pPlayOrderMenuScreen = 0;

// (epg,11/28/2001): TODO: impl add'l modes, incorp icons
//extern PegBitmap gb_MI_Shuffle_Album_Bitmap;
//extern PegBitmap gb_MI_Repeat_Shuffle_Album_Bitmap;

static MenuItemRec s_menuItems[] =
{
	{ NULL, &gb_MI_Normal_Bitmap, SID_NORMAL, false},
	{ NULL, &gb_MI_Shuffle_Bitmap, SID_RANDOM, false},
	{ NULL, &gb_MI_Repeat_Bitmap, SID_REPEAT_ALL, false},
	{ NULL, &gb_MI_Repeat_Suffle_Bitmap, SID_REPEAT_RANDOM, false},
	{ NULL, &gb_MI_Album_Bitmap, SID_ALBUM_ORDER, false},
    { NULL, &gb_MI_Repeat_Album_Bitmap, SID_REPEAT_ALBUM_ORDER, false},
    { NULL, &gb_MI_Shuffle_Album_Bitmap, SID_RANDOM_ALBUM_ORDER, false},
    { NULL, &gb_MI_Repeat_Shuffle_Album_Bitmap, SID_REPEAT_RANDOM_ALBUM_ORDER, false},
};


// This is a singleton class.
CScreen*
CPlayOrderMenuScreen::GetPlayOrderMenuScreen()
{
	if (!s_pPlayOrderMenuScreen) {
		s_pPlayOrderMenuScreen = new CPlayOrderMenuScreen(NULL);
	}
	return s_pPlayOrderMenuScreen;
}


CPlayOrderMenuScreen::CPlayOrderMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_PLAY_ORDER));
}

CPlayOrderMenuScreen::~CPlayOrderMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CPlayOrderMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    CPogoPlaylist* pPP = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
	switch (iMenuIndex)
	{
		case 0:		// set normal playlist mode
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::NORMAL);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(0);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
        }
		case 1:		// random
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::PRAGMATIC_RANDOM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(1);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
        }
		case 2:		// repeat all
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::REPEAT_ALL);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(4);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
        }
		case 3:		// repeat rand
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::REPEAT_RANDOM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(5);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
        }
        case 4:     // album
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::ALBUM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(2);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
		    break;
        }
        case 5:     // repeat album
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::REPEAT_ALBUM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(6);
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
		    break;
        }
        case 6:     // random album
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::RANDOM_ALBUM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(3);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        }
        case 7:     // repeat random album
        {
            pPP->SetPogoPlaylistMode(CPogoPlaylist::REPEAT_RANDOM_ALBUM);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetPlayModeIcon(7);
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        }
	}
}
