//........................................................................................
//........................................................................................
//.. File Name: InputSelectMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CInputSelectMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef INPUTSELECT_MENUSCREEN_H_
#define INPUTSELECT_MENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CInputSelectMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetInputSelectMenuScreen();

    static void Destroy() {
		if (s_pInputSelectMenuScreen)
			delete s_pInputSelectMenuScreen;
		s_pInputSelectMenuScreen = 0;
    }

    int GetInputSelect();
    void SetInputSelect(int nInput);


protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

private:

	CInputSelectMenuScreen(CScreen* pParent = NULL);
	virtual ~CInputSelectMenuScreen();

	// A pointer to the global instance of this class.
	static CInputSelectMenuScreen* s_pInputSelectMenuScreen;	

    int m_nInputSelect; // 0 = line-in, 1 = mic
};

#endif  // INPUTSELECT_MENUSCREEN_H_
