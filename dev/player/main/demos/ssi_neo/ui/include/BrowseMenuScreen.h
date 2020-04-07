//........................................................................................
//........................................................................................
//.. File Name: BrowseMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseMenuScreen class		 				..
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
#ifndef BROWSEMENUSCREEN_H_
#define BROWSEMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/MenuScreen.h>

class CBrowseMenuScreen : public CMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseMenuScreen();
    static void Destroy() {
		if (s_pBrowseMenuScreen)
			delete s_pBrowseMenuScreen;
		s_pBrowseMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

private:

	CBrowseMenuScreen();
	~CBrowseMenuScreen();

	static CBrowseMenuScreen* s_pBrowseMenuScreen;

	bool		m_bStopDown;	// Keeps track of the pressing of the next and previous buttons
	bool		m_bPrevDown;	// and only goes back to the main menu when both pressed and released.

};

#endif  // BROWSEMENUSCREEN_H_
