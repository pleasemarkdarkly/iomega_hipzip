//
// SourceMenuScreen.h: Browse available content and input sources via db queries
// danb@fullplaymedia.com 09/20/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef SOURCEMENUSCREEN_H_
#define SOURCEMENUSCREEN_H_

//#include <gui/peg/peg.hpp>

#include <main/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <core/playmanager/PlayManager.h>
#include <main/main/DJPlayerState.h>
#ifndef NO_UPNP
#include <main/iml/query/QueryResult.h>
#endif // NO_UPNP

#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/LibraryEntryMenuScreen.h>

class CSourceMenuScreen : public CDynamicMenuScreen
{
public:
    // this is a singleton class
	static CScreen* GetSourceMenuScreen();
    static void Destroy() {
		if (s_pSourceMenuScreen)
			delete s_pSourceMenuScreen;
		s_pSourceMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

    // Hides any visible screens.
    void HideScreen();
    
    // Called so the screen will ask the DJPlayerState for the current source
    void RefreshSource(bool bSynchToBrowseSource = false);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex);
    
	// Called when the user hits the previous button.
	// Acts based upon the currently highlighted menu item.
	void GotoPreviousMenu();

    // The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
    bool MenuItemSelected(int iMenuIndex);
    bool MenuItemSelectable(int iMenuIndex) { return true; };
	const TCHAR* MenuItemCaption(int iMenuIndex);

private:

    CSourceMenuScreen();
	~CSourceMenuScreen();

#ifndef NO_UPNP
    CIMLManager*            m_pIMLManager;
#endif // NO_UPNP

    int     m_iCurrentSourceIndex;

    CDJPlayerState* m_pDJPlayerState;  // what is the current source mode?
    CLibraryMenuScreen* m_pLMS;
    CLibraryEntryMenuScreen* m_pLEMS;

    static CSourceMenuScreen* s_pSourceMenuScreen;

    int  m_iScanningIMLs;
    bool m_bAllIMLsCached;
    bool m_bNetworkUp;
    const TCHAR* BuildScanningString();
};

#endif  // SOURCEMENUSCREEN_H_
