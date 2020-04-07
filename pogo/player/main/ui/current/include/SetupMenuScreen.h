//........................................................................................
//........................................................................................
//.. File Name: SetupMenuScreen.h															..
//.. Date: 09/21/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CSetupMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/26/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef SETUPMENUSCREEN_H_
#define SETUPMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CSetupMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetSetupMenuScreen();

    static void Destroy() {
		if (s_pSetupMenuScreen)
			delete s_pSetupMenuScreen;
		s_pSetupMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);


private:

	CSetupMenuScreen(CScreen* pParent = NULL);
	virtual ~CSetupMenuScreen();

	// A pointer to the global instance of this class.
	static CSetupMenuScreen* s_pSetupMenuScreen;	


};

#endif  // SETUPMENUSCREEN_H_
