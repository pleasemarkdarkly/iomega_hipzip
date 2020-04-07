//........................................................................................
//........................................................................................
//.. File Name: StaticSettingsMenuScreen.h															..
//.. Date: 10/01/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CStaticSettingsMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 10/01/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef STATICSETTINGSMENUSCREEN_H_
#define STATICSETTINGSMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

#define STR_SIZE 128

class CStaticSettingsMenuScreen : public CDynamicMenuScreen
{
public:
	static CScreen* GetStaticSettingsMenuScreen();

    static void Destroy() {
		if (s_pStaticSettingsMenuScreen)
			delete s_pStaticSettingsMenuScreen;
		s_pStaticSettingsMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex) {};

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	const TCHAR* MenuItemCaption(int iMenuIndex);

    void EditItemInFocus();
    void EditItemInFocusCB(bool bSave);
    static void EditItemInFocusCallback(bool bSave);

private:

	CStaticSettingsMenuScreen(CScreen* pParent = NULL);
	~CStaticSettingsMenuScreen();

	// A pointer to the global instance of this class.
	static CStaticSettingsMenuScreen* s_pStaticSettingsMenuScreen;	

	TCHAR m_pszPlayerIP[STR_SIZE];
	TCHAR m_pszPlayerGateway[STR_SIZE];
	TCHAR m_pszPlayerSubnet[STR_SIZE];
    TCHAR m_pszPlayerDNS[STR_SIZE];

    bool m_bSettingsHaveChanged;
};

#endif  // STATICSETTINGSMENUSCREEN_H_
