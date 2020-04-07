//........................................................................................
//........................................................................................
//.. File Name: BrowseAlbumsTitlesMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseAlbumsTitlesMenuScreen class		 				..
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
#ifndef BROWSEALBUMSTITLESMENUSCREEN_H_
#define BROWSEALBUMSTITLESMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>

class CBrowseAlbumsTitlesMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseAlbumsTitlesMenuScreen();
    static void Destroy() {
		if (s_pBrowseAlbumsTitlesMenuScreen)
			delete s_pBrowseAlbumsTitlesMenuScreen;
		s_pBrowseAlbumsTitlesMenuScreen = 0;
    }

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

	CBrowseAlbumsTitlesMenuScreen();
	~CBrowseAlbumsTitlesMenuScreen();

	static CBrowseAlbumsTitlesMenuScreen* s_pBrowseAlbumsTitlesMenuScreen;
};

#endif  // BROWSEALBUMSTITLESMENUSCREEN_H_
