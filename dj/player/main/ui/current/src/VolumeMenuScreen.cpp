//
// VolumeMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/14/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/VolumeMenuScreen.h>

#ifndef DISABLE_VOLUME_CONTROL
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>
#include <main/ui/PlayerScreen.h>

#include <main/main/DJPlayerState.h>
#include <io/audio/VolumeControl.h>        // interface to volume

#include <stdio.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_VOLUME_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_VOLUME_MENU_SCREEN );

// the global reference for this class
CVolumeMenuScreen* CVolumeMenuScreen::s_pVolumeMenuScreen = 0;

// This is a singleton class.
CScreen*
CVolumeMenuScreen::GetVolumeMenuScreen()
{
	if (!s_pVolumeMenuScreen) {
		s_pVolumeMenuScreen = new CVolumeMenuScreen();
	}
	return s_pVolumeMenuScreen;
}


CVolumeMenuScreen::CVolumeMenuScreen()
	: CDynamicMenuScreen(NULL, SID_VOLUME)
{
}

CVolumeMenuScreen::~CVolumeMenuScreen()
{
}

void
CVolumeMenuScreen::Draw()
{
    SetItemCount(CVolumeControl::GetInstance()->GetVolumeRange());
    m_iTopIndex = m_cItems - CVolumeControl::GetInstance()->GetVolume() - 2;
    // make sure we're not pointing at something too low
    if (m_iTopIndex < -1)
        m_iTopIndex = -1;
    DEBUGP( DBG_VOLUME_MENU_SCREEN, DBGLEV_TRACE, "CVolumeMenuScreen::Draw\n  m_iTopIndex = %d\n  Volume = %d\n", m_iTopIndex, CVolumeControl::GetInstance()->GetVolume());
	CDynamicMenuScreen::Draw();
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CVolumeMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    CPlayerScreen::GetPlayerScreen()->HideMenus();
}

const TCHAR* 
CVolumeMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if (iMenuIndex > m_cItems || iMenuIndex < 0)
        return LS(SID_EMPTY_STRING);
    char pszCharVolumeValue[10];
    TCHAR pszVolumeValue[10];
    sprintf(pszCharVolumeValue, "%d", m_cItems - iMenuIndex - 1); // invert the numbers
    CharToTchar(pszVolumeValue, pszCharVolumeValue);
    tstrcpy(m_pszVolumeValue, pszVolumeValue);
    return m_pszVolumeValue;
}

// Notification from the scrolling list that the list has scrolled up.
void
CVolumeMenuScreen::NotifyScrollUp()
{
    CVolumeControl::GetInstance()->VolumeUp();
    Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CVolumeMenuScreen::NotifyScrollDown()
{
    CVolumeControl::GetInstance()->VolumeDown();
    Draw();
}

#endif // DISABLE_VOLUME_CONTROL

