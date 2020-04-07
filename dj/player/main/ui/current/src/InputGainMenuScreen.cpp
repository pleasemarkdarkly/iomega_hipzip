//
// InputGainMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/14/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/InputGainMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>
#include <main/ui/PlayerScreen.h>

#include <main/main/DJPlayerState.h>
#include <io/audio/VolumeControl.h>        // interface to volume

#include <stdio.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_INPUT_GAIN_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_INPUT_GAIN_MENU_SCREEN );

// the global reference for this class
CInputGainMenuScreen* CInputGainMenuScreen::s_pInputGainMenuScreen = 0;

// This is a singleton class.
CScreen*
CInputGainMenuScreen::GetInputGainMenuScreen()
{
	if (!s_pInputGainMenuScreen) {
		s_pInputGainMenuScreen = new CInputGainMenuScreen();
	}
	return s_pInputGainMenuScreen;
}


CInputGainMenuScreen::CInputGainMenuScreen()
	: CDynamicMenuScreen(NULL, SID_LINE_INPUT_GAIN)
{
    SetItemCount(CLineRecorder::GetInstance()->GetGainRange() + 1);
}

CInputGainMenuScreen::~CInputGainMenuScreen()
{
}

void
CInputGainMenuScreen::Draw()
{
	CDynamicMenuScreen::Draw();
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CInputGainMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    CLineRecorder::GetInstance()->SetGain((m_cItems - 1) - iMenuIndex);
    Draw();
}

const TCHAR* 
CInputGainMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if (iMenuIndex > m_cItems || iMenuIndex < 0)
        return LS(SID_EMPTY_STRING);
    char pszCharVolumeValue[10];
    TCHAR pszVolumeValue[10];
    sprintf(pszCharVolumeValue, "+%ddB", m_cItems - iMenuIndex - 1); // invert the numbers
    CharToTchar(pszVolumeValue, pszCharVolumeValue);
    tstrcpy(m_pszVolumeValue, pszVolumeValue);
    return m_pszVolumeValue;
}

bool
CInputGainMenuScreen::MenuItemSelected(int iMenuIndex)
{
    return (((m_cItems - 1) - iMenuIndex) == CLineRecorder::GetInstance()->GetGain());
}
