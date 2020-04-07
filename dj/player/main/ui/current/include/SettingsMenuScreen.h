//........................................................................................
//........................................................................................
//.. File Name: SettingsMenuScreen.h															..
//.. Date: 09/21/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CSettingsMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 12/14/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef SETTINGSMENUSCREEN_H_
#define SETTINGSMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CSettingsMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetSettingsMenuScreen();

    static void Destroy() {
		if (s_pSettingsMenuScreen)
			delete s_pSettingsMenuScreen;
		s_pSettingsMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

private:

	CSettingsMenuScreen(CScreen* pParent = NULL);
	virtual ~CSettingsMenuScreen();

	// A pointer to the global instance of this class.
	static CSettingsMenuScreen* s_pSettingsMenuScreen;	


};

#endif  // SETTINGSMENUSCREEN_H_
