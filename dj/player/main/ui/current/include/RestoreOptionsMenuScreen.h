//
// RestoreOptionsMenuScreen.h: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 05/14/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef RESTOREOPTIONSMENUSCREEN_H_
#define RESTOREOPTIONSMENUSCREEN_H_

#include <main/ui/MenuScreen.h>
class CRestoreScreen;
class CRestoreOptionsMenuScreen;

class CRestoreOptionsMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetRestoreOptionsMenuScreen();

    static void Destroy() {
		if (s_pRestoreOptionsMenuScreen)
			delete s_pRestoreOptionsMenuScreen;
		s_pRestoreOptionsMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	void FormatConfirm(bool bConfirm);
	static void FormatConfirmCB(bool bConfirm)
	{
		CRestoreOptionsMenuScreen *p;
		p = (CRestoreOptionsMenuScreen*)CRestoreOptionsMenuScreen::GetRestoreOptionsMenuScreen();
		if(p != NULL)
			p->FormatConfirm(bConfirm);
	}

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};

private:
	CRestoreOptionsMenuScreen(CScreen* pParent = NULL);
	virtual ~CRestoreOptionsMenuScreen();

	// A pointer to the global instance of this class.
	static CRestoreOptionsMenuScreen* s_pRestoreOptionsMenuScreen;	
};

#endif  // RESTOREOPTIONSMENUSCREEN_H_
