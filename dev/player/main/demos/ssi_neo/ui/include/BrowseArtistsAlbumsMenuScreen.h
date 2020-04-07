//........................................................................................
//........................................................................................
//.. File Name: BrowseArtistsAlbumsMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseArtistsAlbumsMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef BROWSEARTISTSALBUMSMENUSCREEN_H_
#define BROWSEARTISTSALBUMSMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class CBrowseArtistsAlbumsMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseArtistsAlbumsMenuScreen();
    static void Destroy() {
		if (s_pBrowseArtistsAlbumsMenuScreen)
			delete s_pBrowseArtistsAlbumsMenuScreen;
		s_pBrowseArtistsAlbumsMenuScreen = 0;
    }

    void SetArtist(const TCHAR* szArtist);
    void Minimize();

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemIsChecked(int iMenuIndex);
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:
    int m_iArtistKey;
    TCHAR m_szArtist[PLAYLIST_STRING_SIZE];
    ContentKeyValueVector m_Albums;
    int m_iAlbumIndex;

	CBrowseArtistsAlbumsMenuScreen();
	~CBrowseArtistsAlbumsMenuScreen();

	static CBrowseArtistsAlbumsMenuScreen* s_pBrowseArtistsAlbumsMenuScreen;
};

#endif  // BROWSEARTISTSALBUMSMENUSCREEN_H_
