//
// RestoreMenuScreen.h: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 05/13/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef RESTOREMENUSCREEN_H_
#define RESTOREMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CRestoreScreen;
class CRestoreMenuScreen;
class CRestoreMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetRestoreMenuScreen();

    static void Destroy() {
		if (s_pRestoreMenuScreen)
			delete s_pRestoreMenuScreen;
		s_pRestoreMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	void EverythingConfirm(bool bConfirm);
	
	static void EverythingConfirmCB(bool bConfirm)
	{
		CRestoreMenuScreen *p;
		p = (CRestoreMenuScreen*)CRestoreMenuScreen::GetRestoreMenuScreen();
		if(p != NULL)
			p->EverythingConfirm(bConfirm);
	}

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

private:
	CRestoreMenuScreen(CScreen* pParent = NULL);
	virtual ~CRestoreMenuScreen();

	// A pointer to the global instance of this class.
	static CRestoreMenuScreen* s_pRestoreMenuScreen;	
};

#endif  // RESTOREMENUSCREEN_H_
