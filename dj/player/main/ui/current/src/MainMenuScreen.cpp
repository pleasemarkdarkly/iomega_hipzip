//
// MainMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/MainMenuScreen.h>
#include <main/ui/LibraryEntryMenuScreen.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/SettingsMenuScreen.h>
#include <main/ui/SystemToolsMenuScreen.h>
#include <main/ui/PlayerInfoMenuScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/AlertScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_MAIN_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_MAIN_MENU_SCREEN );

// the global reference for this class
CMainMenuScreen* CMainMenuScreen::s_pMainMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
//    { false, false, true, SID_MUSIC_LIBRARY, &CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen },
    { false, false, true, SID_MUSIC_SOURCES, &CSourceMenuScreen::GetSourceMenuScreen },
    { false, false, true, SID_JUKEBOX_SETTINGS, &CSettingsMenuScreen::GetSettingsMenuScreen },
    { false, false, true, SID_SYSTEM_TOOLS, &CSystemToolsMenuScreen::GetSystemToolsMenuScreen },
    { false, false, true, SID_PLAYER_INFO, &CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen },
};


// This is a singleton class.
CScreen*
CMainMenuScreen::GetMainMenuScreen()
{
	if (!s_pMainMenuScreen) {
		s_pMainMenuScreen = new CMainMenuScreen(NULL);
	}
	return s_pMainMenuScreen;
}


CMainMenuScreen::CMainMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_MENU, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
}

CMainMenuScreen::~CMainMenuScreen()
{
}

SIGNED
CMainMenuScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        case IR_KEY_PREV:
        case IR_KEY_MENU:
        case KEY_MENU:
            ResetToTop();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
            return 0;
        case IR_KEY_SELECT:
        case KEY_SELECT:
            GotoSubMenu(GetHighlightedIndex());
            return 0;
        default:
            break;
        }
        break;
        
    default:
        break;
    }
    return CMenuScreen::Message(Mesg);
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CMainMenuScreen::GotoSubMenu(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:
        DEBUGP( DBG_MAIN_MENU_SCREEN, DBGLEV_INFO, "Goto MusicSourceMS\n");
        ((CSourceMenuScreen*)CSourceMenuScreen::GetSourceMenuScreen())->RefreshSource();
        Parent()->Add(CSourceMenuScreen::GetSourceMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CSourceMenuScreen::GetSourceMenuScreen());
        break;
    case 1:
        DEBUGP( DBG_MAIN_MENU_SCREEN, DBGLEV_INFO, "Goto JukeboxSettingsMS\n");
        Parent()->Add(CSettingsMenuScreen::GetSettingsMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CSettingsMenuScreen::GetSettingsMenuScreen());
        break;
    case 2:
        DEBUGP( DBG_MAIN_MENU_SCREEN, DBGLEV_INFO, "Goto SystemToolsMS\n");
        Parent()->Add(CSystemToolsMenuScreen::GetSystemToolsMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CSystemToolsMenuScreen::GetSystemToolsMenuScreen());
        return;
    case 3:
        DEBUGP( DBG_MAIN_MENU_SCREEN, DBGLEV_INFO, "Goto PlayerInfoMS\n");
        Parent()->Add(CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen());
        return;
    };
}
