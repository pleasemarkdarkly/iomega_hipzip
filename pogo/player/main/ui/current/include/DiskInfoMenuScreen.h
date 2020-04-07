//........................................................................................
//........................................................................................
//.. File Name: DiskInfoMenuScreen.h															..
//.. Date: 10/01/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CDiskInfoMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 10/01/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef DISKINFOMENUSCREEN_H_
#define DISKINFOMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

#define MAX_DISKINFO_CHARS 25

class CDiskInfoMenuScreen : public CDynamicMenuScreen
{
public:
	static CScreen* GetDiskInfoMenuScreen();

    static void Destroy() {
		if (s_pDiskInfoMenuScreen)
			delete s_pDiskInfoMenuScreen;
		s_pDiskInfoMenuScreen = 0;
    }

    // computes and displays the disk info.
    void Init();

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

	CDiskInfoMenuScreen(CScreen* pParent = NULL);
	~CDiskInfoMenuScreen();

	// A pointer to the global instance of this class.
	static CDiskInfoMenuScreen* s_pDiskInfoMenuScreen;	

	TCHAR m_pszDiskFree[MAX_DISKINFO_CHARS];    // Free: 20,000 MB
	TCHAR m_pszFileCount[MAX_DISKINFO_CHARS];   // Files: 10,000 Files
	TCHAR m_pszFolderCount[MAX_DISKINFO_CHARS]; // Folders: 10,000
};

#endif  // DISKINFOMENUSCREEN_H_
