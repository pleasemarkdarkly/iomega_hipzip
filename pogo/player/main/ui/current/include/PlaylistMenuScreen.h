//........................................................................................
//........................................................................................
//.. File Name: PlaylistMenuScreen.h														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CPlaylistMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef PLAYLISTMENUSCREEN_H_
#define PLAYLISTMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <content/common/ContentManager.h>
#include <main/ui/DynamicMenuScreen.h>
#include <util/datastructures/SimpleVector.h>

struct PlaylistFile {
    char* url;
    TCHAR* caption;
};

class CPlaylistMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetPlaylistMenuScreen();
    static void Destroy() {
		if (s_pPlaylistMenuScreen)
			delete s_pPlaylistMenuScreen;
		s_pPlaylistMenuScreen = 0;
    }
    void Init();
    void Minimize();

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:
    typedef SimpleVector<PlaylistFile*> PlaylistFileList;
 
    int SyncItrToIndex(int iMenuIndex);
    void PopulatePlaylistList();
	CPlaylistMenuScreen();
	~CPlaylistMenuScreen();

    PlaylistRecordList m_lstPlaylistRecords;
    PlaylistRecordIterator m_itrPlaylistRecord;
    int m_nPlaylistRecord;
	static CPlaylistMenuScreen* s_pPlaylistMenuScreen;
};

#endif  // PLAYLISTMENUSCREEN_H_
