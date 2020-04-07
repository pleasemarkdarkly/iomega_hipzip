//
// LibraryEntryMenuScreen.h: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef LIBRARYENTRYMENUSCREEN_H_
#define LIBRARYENTRYMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/main/DJPlayerState.h>

class CLibraryEntryMenuScreen : public CDynamicMenuScreen
{
public:
	static CScreen* GetLibraryEntryMenuScreen();

    static void Destroy() {
		if (s_pLibraryEntryMenuScreen)
			delete s_pLibraryEntryMenuScreen;
		s_pLibraryEntryMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

    // Notify this screen that the current source was lost
    void NotifyLostCurrentSource();

    void SetBrowseMode(CLibraryMenuScreen::eBrowseMode eMode);

protected:

    // callback that the alert screen calls
    // we've lost the current source, so this callback will
    // hide this screen and put the user in the source menu screen
    void LostCurrentSourceCB();
    static void LostCurrentSourceCallback();

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

    // Called when the user hits the previous button.
    // Acts based upon the currently highlighted menu item.
    void GotoPreviousMenu();

    bool MenuItemHasSubMenu(int iMenuIndex) { return true; };
    bool MenuItemSelected(int iMenuIndex) { return false; };
    bool MenuItemSelectable(int iMenuIndex) { return false; };
    const TCHAR* MenuItemCaption(int iMenuIndex);
    const TCHAR* MenuTitleCaption();

private:

	CLibraryEntryMenuScreen(CScreen* pParent = NULL);
	virtual ~CLibraryEntryMenuScreen();

	// A pointer to the global instance of this class.
	static CLibraryEntryMenuScreen* s_pLibraryEntryMenuScreen;	
};

#endif  // LIBRARYENTRYMENUSCREEN_H_
