//
// AboutScreen.cpp: implementation of CAboutScreen class
// danb@fullplaymedia.com 07/22/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/AboutScreen.h>

#include <main/ui/PlayerScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>

CAboutScreen* CAboutScreen::s_pAboutScreen = 0;

CAboutScreen*
CAboutScreen::GetAboutScreen()
{
	if (!s_pAboutScreen)
		s_pAboutScreen = new CAboutScreen();
    return s_pAboutScreen;
}


CAboutScreen::CAboutScreen()
	: CScreen(NULL)
{
	BuildScreen();
}


CAboutScreen::~CAboutScreen()
{
}

SIGNED
CAboutScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
	// ignore all key presses for simplicity
	case PM_KEY:

        switch (Mesg.iData)
        {
        case IR_KEY_EXIT:
        case KEY_EXIT:
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
            return 0;
        default:
            GotoPreviousMenu();
			return 0;
        }
        return 0;

	case PM_KEY_RELEASE:
		return 0;
	default:
		return CScreen::Message(Mesg);
	}
}

// Creates all the icons used by the menu screen.
void
CAboutScreen::BuildScreen()
{
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	m_pScreenWindow = new PegIcon(mReal, &gbAboutFullplayBitmap);
	Add(m_pScreenWindow);
}

void
CAboutScreen::GotoPreviousMenu()
{
    if (Parent() != m_pParent)
	    Parent()->Add(m_pParent);
	Parent()->Remove(this);
	Presentation()->MoveFocusTree(m_pParent);
}


