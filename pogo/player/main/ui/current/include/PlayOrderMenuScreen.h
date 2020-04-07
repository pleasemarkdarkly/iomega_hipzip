//........................................................................................
//........................................................................................
//.. File Name: PlayOrderMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CPlayOrderMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef PLAYORDERMENUSCREEN_H_
#define PLAYORDERMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CPlayOrderMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetPlayOrderMenuScreen();

    static void Destroy() {
		if (s_pPlayOrderMenuScreen)
			delete s_pPlayOrderMenuScreen;
		s_pPlayOrderMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};


private:

	CPlayOrderMenuScreen(CScreen* pParent = NULL);
	virtual ~CPlayOrderMenuScreen();

	// A pointer to the global instance of this class.
	static CPlayOrderMenuScreen* s_pPlayOrderMenuScreen;	


};

#endif  // PLAYORDERMENUSCREEN_H_
