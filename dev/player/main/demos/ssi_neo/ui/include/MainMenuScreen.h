//........................................................................................
//........................................................................................
//.. File Name: MainMenuScreen.h														..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: definition of CMainMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Dan Bolstad  danb@iobjects.com									..	
//.. Modification date: 03/14/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef MAINMENUSCREEN_H_
#define MAINMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#ifdef PEG_UNICODE
#define USE_SETTINGS_MENU
#endif

#include "MenuScreen.h"
//#include "BookmarkMenuScreen.h"
//#include "PlayModeMenuScreen.h"
//#include "EqualizerMenuScreen.h"
//#include "DiskInfoScreen.h"
//#include "PlaylistManagerMenuScreen.h"
#ifdef USE_SETTINGS_MENU
//#include "SettingsMenuScreen.h"
#else	// USE_SETTINGS_MENU
//#include "BacklightMenuScreen.h"
#endif	// USE_SETTINGS_MENU
//#include "SongInfoScreen.h"
//#include "PlayerInfoScreen.h"

class CMainMenuScreen : public CMenuScreen
{
public:
	// this is a singleton class
	//CMainMenuScreen* GetMainMenuScreen();
	static CScreen* GetMainMenuScreen();
    static void Destroy() {
		if (s_pMainMenuScreen)
			delete s_pMainMenuScreen;
		s_pMainMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	// Hides any visible menu screens.
	//void HideMenu();

	// returns a pointer to a string for the title of this screen in other menus
	//TCHAR* GetMenuCaption() = {return LS(m_wScreenTitleSID)};

/*
	// A hack to let the play screen tell the play mode menu screen to update itself.
	CPlayModeMenuScreen* GetPlayModeMenuScreen()
		{ return m_pPlayModeMenuScreen; }

	// A hack to let the play screen tell the song info screen to update itself.
	CSongInfoScreen* GetSongInfoScreen()
		{ return m_pSongInfoScreen; }

	// A hack to tell the disk info screen the free space on the disk.
	CDiskInfoScreen* GetDiskInfoScreen()
		{ return m_pDiskInfoScreen; }

	// A hack to tell the playlist display screen that a playlist has been loaded.
	CPlaylistScreen* GetPlaylistScreen()
		{ return m_pPlaylistManagerMenuScreen->GetPlaylistScreen(); }

	// A hack to let the play screen tell the bookmark screen to update itself.
	CBookmarkMenuScreen* GetBookmarkMenuScreen()
		{ return m_pBookmarkMenuScreen; }
*/
protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

private:

	CMainMenuScreen();
	~CMainMenuScreen();

	static CMainMenuScreen* s_pMainMenuScreen;

	bool		m_bStopDown;	// Keeps track of the pressing of the next and previous buttons
	bool		m_bPrevDown;	// and only goes back to the main menu when both pressed and released.

};

#endif  // MAINMENUSCREEN_H_
