//........................................................................................
//........................................................................................
//.. File Name: BitrateMenuScreen.cpp													..
//.. Date: 09/28/2001																	..
//.. Author(s): Eric Gibbs  															..
//.. Description of content: implementation of CBitrateMenuScreen class					..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/BitrateMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/PlayerScreen.h>
#include <main/main/Recorder.h>

extern "C" {
#include <devs/lcd/lcd.h>
}

// the global reference for this class
CBitrateMenuScreen* CBitrateMenuScreen::s_pBitrateMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
    { NULL, NULL, SID_BITRATE_64, false},
    { NULL, NULL, SID_BITRATE_128, false},
    { NULL, NULL, SID_BITRATE_192, false},
};


// This is a singleton class.
CScreen*
CBitrateMenuScreen::GetBitrateMenuScreen()
{
    if (!s_pBitrateMenuScreen) {
        s_pBitrateMenuScreen = new CBitrateMenuScreen(NULL);
    }
    return s_pBitrateMenuScreen;
}


CBitrateMenuScreen::CBitrateMenuScreen(CScreen* pParent)
: CMenuScreen(pParent, SID_SETUP, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec), true), m_nBitrate(128)
{
    // relocate the screen 
    PegRect newRect = m_pScreenTitle->mReal;
    newRect.wTop = mReal.wTop + 54;
    newRect.wBottom = mReal.wTop + 63;
    m_pScreenTitle->Resize(newRect);
    
    SetMenuTitle(LS(SID_RECORDING_BITRATE));
}

CBitrateMenuScreen::~CBitrateMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CBitrateMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:		// 64
        SetBitrate(64);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        break;
    case 1:		// 128
        SetBitrate(128);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        break;
    case 2:     // 192
        SetBitrate(192);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        break;
    };
}

int CBitrateMenuScreen::GetBitrate()
{
    return m_nBitrate;
}

void CBitrateMenuScreen::SetBitrate(int nBitrate)
{
    if (nBitrate)
        m_nBitrate = nBitrate;
    CRecorder::GetInstance()->SetBitrate(m_nBitrate);
}
