//........................................................................................
//........................................................................................
//.. File Name: ToneMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CToneMenuScreen class					..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 12/13/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/ToneMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/PlayerScreen.h>

#include <io/audio/VolumeControl.h>

#include <util/tchar/Tchar.h>
#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_TMS, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_TMS );

// the global reference for this class
CToneMenuScreen* CToneMenuScreen::s_pToneMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ NULL, &gb_MI_EQ_Normal_Bitmap, SID_NORMAL, false},
	{ NULL, &gb_MI_EQ_Classical_Bitmap, SID_CLASSICAL, false},
	{ NULL, &gb_MI_EQ_Jazz_Bitmap, SID_JAZZ, false},
	{ NULL, &gb_MI_EQ_Rock_Bitmap, SID_ROCK, false},
};


// This is a singleton class.
CScreen*
CToneMenuScreen::GetToneMenuScreen()
{
	if (!s_pToneMenuScreen) {
		s_pToneMenuScreen = new CToneMenuScreen(NULL);
	}
	return s_pToneMenuScreen;
}


CToneMenuScreen::CToneMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_TONE));

    CVolumeControl* vc = CVolumeControl::GetInstance();
    vc->SetBassRange(BASS_RANGE);
    vc->SetTrebleRange(TREBLE_RANGE);
}

CToneMenuScreen::~CToneMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.

void
CToneMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    tEqualizerMode eMode = (tEqualizerMode) iMenuIndex;
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    ps->SetEqualizerMode(eMode);
    SetEqualizerMode(eMode);
    ps->HideMenus();
}

void CToneMenuScreen::SetEqualizerMode( tEqualizerMode eMode )
{
	CVolumeControl* vc = CVolumeControl::GetInstance();
    switch (eMode)
	{
		case EQ_NORMAL:
            vc->SetBass(NORMAL_BASS);
            vc->SetTreble(NORMAL_TREBLE);
			break;
		case EQ_CLASSICAL:
            vc->SetBass(CLASSICAL_BASS);
            vc->SetTreble(CLASSICAL_TREBLE);
			break;
		case EQ_JAZZ:
            vc->SetBass(JAZZ_BASS);
            vc->SetTreble(JAZZ_TREBLE);
			break;
		case EQ_ROCK:
            vc->SetBass(ROCK_BASS);
            vc->SetTreble(ROCK_TREBLE);
			break;
	}
}