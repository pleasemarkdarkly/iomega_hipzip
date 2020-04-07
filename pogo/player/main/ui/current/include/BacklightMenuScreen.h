//........................................................................................
//........................................................................................
//.. File Name: BacklightMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBacklightMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef BACKLIGHTMENUSCREEN_H_
#define BACKLIGHTMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CBacklightMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetBacklightMenuScreen();

    static void Destroy() {
		if (s_pBacklightMenuScreen)
			delete s_pBacklightMenuScreen;
		s_pBacklightMenuScreen = 0;
    }

    bool GetBacklightOn();
    void SetBacklightOn(bool bOn);


protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

private:

	CBacklightMenuScreen(CScreen* pParent = NULL);
	virtual ~CBacklightMenuScreen();

	// A pointer to the global instance of this class.
	static CBacklightMenuScreen* s_pBacklightMenuScreen;	

    bool m_bBacklightOn;
};

#endif  // BACKLIGHTMENUSCREEN_H_
