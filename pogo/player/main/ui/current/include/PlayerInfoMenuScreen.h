//........................................................................................
//........................................................................................
//.. File Name: PlayerInfoMenuScreen.h															..
//.. Date: 10/01/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CPlayerInfoMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 10/01/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef PLAYERINFOMENUSCREEN_H_
#define PLAYERINFOMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <main/main/Version.h>

class CPlayerInfoMenuScreen : public CDynamicMenuScreen
{
public:
	static CScreen* GetPlayerInfoMenuScreen();

    static void Destroy() {
		if (s_pPlayerInfoMenuScreen)
			delete s_pPlayerInfoMenuScreen;
		s_pPlayerInfoMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:

	CPlayerInfoMenuScreen(CScreen* pParent = NULL);
	~CPlayerInfoMenuScreen();

	// A pointer to the global instance of this class.
	static CPlayerInfoMenuScreen* s_pPlayerInfoMenuScreen;	

	TCHAR m_pszPlayerVersion[VERSION_NUM_SIZE];
	TCHAR m_pszPlayerBuild[VERSION_NUM_SIZE];
	TCHAR m_pszInstallerVersion[VERSION_NUM_SIZE];
};

#endif  // PLAYERINFOMENUSCREEN_H_
