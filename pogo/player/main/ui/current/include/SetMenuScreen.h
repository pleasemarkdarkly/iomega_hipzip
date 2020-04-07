//........................................................................................
//........................................................................................
//.. File Name: SetMenuScreen.h															..
//.. Date: 09/21/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CSetMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/21/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef SETMENUSCREEN_H_
#define SETMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CSetMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetSetMenuScreen();

    static void Destroy() {
		if (s_pSetMenuScreen)
			delete s_pSetMenuScreen;
		s_pSetMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);

protected:

	// Called when the user hits the play/pause button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

private:

	CSetMenuScreen(CScreen* pParent = NULL);
	virtual ~CSetMenuScreen();

	// A pointer to the global instance of this class.
	static CSetMenuScreen* s_pSetMenuScreen;	


};

#endif  // SETMENUSCREEN_H_
