//........................................................................................
//........................................................................................
//.. File Name: BrowseArtistsAlbumsTitlesMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseArtistsAlbumsTitlesMenuScreen class		 				..
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
#ifndef BROWSEARTISTSALBUMSTITLESMENUSCREEN_H_
#define BROWSEARTISTSALBUMSTITLESMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class CBrowseArtistsAlbumsTitlesMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseArtistsAlbumsTitlesMenuScreen();
    static void Destroy() {
		if (s_pBrowseArtistsAlbumsTitlesMenuScreen)
			delete s_pBrowseArtistsAlbumsTitlesMenuScreen;
		s_pBrowseArtistsAlbumsTitlesMenuScreen = 0;
    }

    void SetArtistAlbum(const TCHAR* szArtist, const TCHAR* szAlbum);
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
    TCHAR m_szArtist[PLAYLIST_STRING_SIZE];
    TCHAR m_szAlbum[PLAYLIST_STRING_SIZE];
    int m_iAlbumKey;
    int m_iArtistKey;
    MediaRecordList m_mrlTracks;
    MediaRecordIterator m_itTrack;
    int m_iIterIndex;
    
	CBrowseArtistsAlbumsTitlesMenuScreen();
	~CBrowseArtistsAlbumsTitlesMenuScreen();

	static CBrowseArtistsAlbumsTitlesMenuScreen* s_pBrowseArtistsAlbumsTitlesMenuScreen;
};

#endif  // BROWSEARTISTSALBUMSTITLESMENUSCREEN_H_
