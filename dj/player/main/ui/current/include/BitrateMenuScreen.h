//
// BitrateMenuScreen.h: implementation of CBitrateMenuScreen class
// danb@fullplaymedia.com 12/20/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef BITRATEMENUSCREEN_H_
#define BITRATEMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

class CBitrateMenuScreen : public CDynamicMenuScreen
{
public:
    static CScreen* GetBitrateMenuScreen();

    static void Destroy() {
		if (s_pBitrateMenuScreen)
			delete s_pBitrateMenuScreen;
		s_pBitrateMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	void SetBitrate(int iBitrate);

        int GetSelectedItem() { return m_iCurrentBitrateIndex; }
        
protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

    // The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
    bool MenuItemSelected(int iMenuIndex);
    bool MenuItemSelectable(int iMenuIndex) { return true; };
    const TCHAR* MenuItemCaption(int iMenuIndex);

private:

	CBitrateMenuScreen();
	virtual ~CBitrateMenuScreen();

    int     m_iCurrentBitrateIndex;

	// A pointer to the global instance of this class.
	static CBitrateMenuScreen* s_pBitrateMenuScreen;	

};

#endif  // BITRATEMENUSCREEN_H_
