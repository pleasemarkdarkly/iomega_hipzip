//........................................................................................
//........................................................................................
//.. File Name: SetMenuScreen.cpp														..
//.. Date: 09/21/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CSetMenuScreen class						..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/21/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/SetMenuScreen.h>
#include <main/ui/GenreMenuScreen.h>
#include <main/ui/PlaylistMenuScreen.h>
#include <main/ui/FolderMenuScreen.h>
#include <main/main/PlaylistConstraint.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <core/playmanager/PlayManager.h>

// the global reference for this class
CSetMenuScreen* CSetMenuScreen::s_pSetMenuScreen = 0;

#define SET_MENU_ITEM_COUNT 4
static MenuItemRec s_menuItems[] =
{
#if 1
    { &CGenreMenuScreen::GetGenreMenuScreen, NULL, SID_GENRES, true},
	{ &CPlaylistMenuScreen::GetPlaylistMenuScreen, NULL, SID_PLAYLISTS, true},
	{ &CFolderMenuScreen::GetFolderMenuScreen, NULL, SID_FOLDERS, true},
	{ NULL, NULL, SID_PLAY_EVERYTHING, false},
#else
  	{ NULL, NULL, SID_GENRES, true},
	{ NULL, NULL, SID_PLAYLISTS, true},
	{ NULL, NULL, SID_FOLDERS, true},
	{ NULL, NULL, SID_PLAY_EVERYTHING, false},
#endif
};


// This is a singleton class.
CScreen*
CSetMenuScreen::GetSetMenuScreen()
{
	if (!s_pSetMenuScreen) {
		s_pSetMenuScreen = new CSetMenuScreen(NULL);
	}
	return s_pSetMenuScreen;
}


CSetMenuScreen::CSetMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SET, s_menuItems, SET_MENU_ITEM_COUNT,false)
{
	SetMenuTitle(LS(SID_DAN_BOLSTAD));
	SetMenuTitle(NULL);
}

CSetMenuScreen::~CSetMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CSetMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case 0:		// Genre
		case 1:		// Playlists
		case 2:		// Folders
			break;	// Don't do anything for these
		case 3:		// Play Everything
        {
            // load everything into the current playlist
			// deselect all the selected items in the play screen
			// go to the play screen
            CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
            pPC->Constrain();
            bool bCrntStillInList;
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
			((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
			break;
        }
	};
}


// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CSetMenuScreen::GotoSubMenu(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case 0:		// Genre
        {
            CGenreMenuScreen* gms = (CGenreMenuScreen*)CGenreMenuScreen::GetGenreMenuScreen();
            gms->Init();
			Parent()->Add(gms);
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(gms);
			break;
        }
		case 1:		// Playlists
        {
            CPlaylistMenuScreen* pms = (CPlaylistMenuScreen*)CPlaylistMenuScreen::GetPlaylistMenuScreen();
            pms->Init();
			Parent()->Add(pms);
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(pms);
			break;
        }
		case 2:		// Folders
        {
            CFolderMenuScreen* fms = (CFolderMenuScreen*)CFolderMenuScreen::GetFolderMenuScreen();
			Parent()->Add(fms);
            fms->Init();
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(fms);
			break;
        }
		case 3:		// Play Everything
			break;
	};
}

