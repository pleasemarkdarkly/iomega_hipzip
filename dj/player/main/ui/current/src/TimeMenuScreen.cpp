//
// TimeMenuScreen.cpp: implementation of CTimeMenuScreen class
// danb@fullplaymedia.com 09/28/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/TimeMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>
#include <main/ui/PlayerScreen.h>

#include <core/playmanager/PlayManager.h>

// the global reference for this class
CTimeMenuScreen* CTimeMenuScreen::s_pTimeMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
    { true, true, false, SID_TRACK_ELAPSED, NULL },
    { false, true, false, SID_TRACK_REMAINING, NULL },
};


// This is a singleton class.
CScreen*
CTimeMenuScreen::GetTimeMenuScreen()
{
    if (!s_pTimeMenuScreen) {
        s_pTimeMenuScreen = new CTimeMenuScreen(NULL);
    }
    return s_pTimeMenuScreen;
}


CTimeMenuScreen::CTimeMenuScreen(CScreen* pParent)
: CMenuScreen(pParent, SID_TIME, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
}

CTimeMenuScreen::~CTimeMenuScreen()
{
}


void
CTimeMenuScreen::SetTimeViewMode(TimeViewMode eTimeViewMode)
{
    switch (eTimeViewMode)
    {
    case TRACK_ELAPSED:
        s_menuItems[0].bSelected = true;
        s_menuItems[1].bSelected = false;
        break;
    case TRACK_REMAINING:
        s_menuItems[0].bSelected = false;
        s_menuItems[1].bSelected = true;
        break;
    default:
        s_menuItems[0].bSelected = false;
        s_menuItems[1].bSelected = false;
        break;
    };
}



// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CTimeMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    
    CPlayerScreen* playscreen = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();

    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
    {
        // reject selection, not valid in line-in mode.
        playscreen->HideMenus();
        playscreen->SetMessageText("Time Settings disabled in Line In source", CSystemMessageString::REALTIME_INFO);
        return;
    }

    switch (iMenuIndex)
    {
    case 0:		// Track Elapsed
        SetTimeViewMode(TRACK_ELAPSED);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(TRACK_ELAPSED);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        break;
    case 1:		// Track Remaining
        SetTimeViewMode(TRACK_REMAINING);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetTimeViewMode(TRACK_REMAINING);
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
        break;
        
    default:
        break;
    };
}

