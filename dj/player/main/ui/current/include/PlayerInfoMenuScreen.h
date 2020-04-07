//
// PlayerInfoMenuScreen.cpp: implementation of CPlayerInfoMenuScreen class
// danb@fullplaymedia.com 10/01/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef PLAYERINFOMENUSCREEN_H_
#define PLAYERINFOMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

#define STR_SIZE 128

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
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	const TCHAR* MenuItemCaption(int iMenuIndex);

private:

	CPlayerInfoMenuScreen(CScreen* pParent = NULL);
	~CPlayerInfoMenuScreen();

	// A pointer to the global instance of this class.
	static CPlayerInfoMenuScreen* s_pPlayerInfoMenuScreen;	

	TCHAR m_pszPlayerVersion[STR_SIZE];
	TCHAR m_pszPlayerIP[STR_SIZE];
	TCHAR m_pszPlayerMAC[STR_SIZE];
	TCHAR m_pszPlayerCDDB[STR_SIZE];
    TCHAR m_pszPlayerFreeDiskSpace[STR_SIZE];
};

#endif  // PLAYERINFOMENUSCREEN_H_
