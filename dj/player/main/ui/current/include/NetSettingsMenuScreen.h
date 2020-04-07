//
// NetSettingsMenuScreen.h: implementation of CNetSettingsMenuScreen class
// chuckf@fullplaymedia.com 04/18/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef NETSETTINGSMENUSCREEN_H_
#define NETSETTINGSMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

class CNetSettingsMenuScreen : public CDynamicMenuScreen
{
public:
	static CScreen* GetNetSettingsMenuScreen();

    static void Destroy() {
		if (s_pNetSettingsMenuScreen)
			delete s_pNetSettingsMenuScreen;
		s_pNetSettingsMenuScreen = 0;
    }

    void Draw();

    int GetSelectedItem() { return m_iCurrentNetSettingIndex; }
    
protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

    bool MenuItemSelected(int iMenuIndex);
    bool MenuItemSelectable(int iMenuIndex) { return true; };
    const TCHAR* MenuItemCaption(int iMenuIndex);

    int m_iCurrentNetSettingIndex;

private:

	CNetSettingsMenuScreen(CScreen* pParent = NULL);
	virtual ~CNetSettingsMenuScreen();

	// A pointer to the global instance of this class.
	static CNetSettingsMenuScreen* s_pNetSettingsMenuScreen;	
};

#endif  // SYSTEMTOOLSMENUSCREEN_H_
