//........................................................................................
//........................................................................................
//.. File Name: BrowseGenresMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseGenresMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef BROWSEGENRESMENUSCREEN_H_
#define BROWSEGENRESMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class CBrowseGenresMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseGenresMenuScreen();
    static void Destroy() {
		if (s_pBrowseGenresMenuScreen)
			delete s_pBrowseGenresMenuScreen;
		s_pBrowseGenresMenuScreen = 0;
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
    ContentKeyValueVector m_Genres;

	CBrowseGenresMenuScreen();
	~CBrowseGenresMenuScreen();

	static CBrowseGenresMenuScreen* s_pBrowseGenresMenuScreen;
};

#endif  // BROWSEGENRESMENUSCREEN_H_
