//........................................................................................
//........................................................................................
//.. File Name: GenreMenuScreen.h														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CGenreMenuScreen class		 				..
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
#ifndef GENREMENUSCREEN_H_
#define GENREMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/ui/DynamicMenuScreen.h>

#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class CGenreMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetGenreMenuScreen();
    static void Destroy() {
		if (s_pGenreMenuScreen)
			delete s_pGenreMenuScreen;
		s_pGenreMenuScreen = 0;
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

    // Called when the user hits the previous button.
	// Minimizes memory usage and returns to the parent screen.
    void GotoPreviousMenu();

    // The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:
    ContentKeyValueVector m_Genres;

	CGenreMenuScreen();
	~CGenreMenuScreen();

	static CGenreMenuScreen* s_pGenreMenuScreen;
};

#endif  // GENREMENUSCREEN_H_
