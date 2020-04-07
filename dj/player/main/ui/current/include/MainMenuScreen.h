//
// MainMenuScreen.h: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef MAINMENUSCREEN_H_
#define MAINMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CMainMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetMainMenuScreen();

    static void Destroy() {
		if (s_pMainMenuScreen)
			delete s_pMainMenuScreen;
		s_pMainMenuScreen = 0;
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

	CMainMenuScreen(CScreen* pParent = NULL);
	virtual ~CMainMenuScreen();

	// A pointer to the global instance of this class.
	static CMainMenuScreen* s_pMainMenuScreen;	


};

#endif  // MAINMENUSCREEN_H_
