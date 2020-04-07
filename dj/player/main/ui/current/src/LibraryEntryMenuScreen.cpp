//
// LibraryEntryMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/LibraryEntryMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/SettingsMenuScreen.h>
#include <main/ui/SystemToolsMenuScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/main/DJPlayerState.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>

#ifndef NO_UPNP
#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#endif  // NO_UPNP

DEBUG_MODULE_S( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_LIBRARY_ENTRY_MENU_SCREEN );

// the global reference for this class
CLibraryEntryMenuScreen* CLibraryEntryMenuScreen::s_pLibraryEntryMenuScreen = 0;

// This is a singleton class.
CScreen*
CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen()
{
    if (!s_pLibraryEntryMenuScreen) {
        s_pLibraryEntryMenuScreen = new CLibraryEntryMenuScreen(NULL);
    }
    return s_pLibraryEntryMenuScreen;
}


CLibraryEntryMenuScreen::CLibraryEntryMenuScreen(CScreen* pParent)
    : CDynamicMenuScreen(&CLibraryMenuScreen::GetLibraryMenuScreen, SID_MUSIC_SOURCE)
{
    m_iTopIndex = 1; // default to album
    SetItemCount(5); // default to Hard Disk
}

CLibraryEntryMenuScreen::~CLibraryEntryMenuScreen()
{
}

SIGNED
CLibraryEntryMenuScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
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
    return CDynamicMenuScreen::Message(Mesg);
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CLibraryEntryMenuScreen::GotoSubMenu(int iMenuIndex)
{
    CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();

    // Check for availability of fml source
    if (pLMS->GetBrowseSource() == CDJPlayerState::FML)
    {
        CIML* pIML = pLMS->GetBrowseIML();
        if (!CIMLManager::GetInstance()->IsAvailable(pIML))
        {
            CIMLManager::GetInstance()->SetIMLUnavailable(pIML);
            return;
        }
    }
    
    switch (iMenuIndex)
    {
    case 0: // Browse Menu
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Genres\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::GENRE, true);
        break;
    case 1:
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Artists\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::ARTIST, true);
        break;
    case 2:
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Albums\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::ALBUM, true);
        break;
    case 3:
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Tracks\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::TRACK, true);
        break;
    case 4:
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Playlists\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::PLAYLIST, true);
        break;
    case 5:
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_INFO, "Goto LibraryMenu > Radio\n");
        pLMS->SetBrowseMode(CLibraryMenuScreen::RADIO, true);
        break;
    };

    pLMS->SetConstraints();
    Parent()->Add(pLMS);
    m_pParent->Remove(this);
    Presentation()->MoveFocusTree(pLMS);
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CLibraryEntryMenuScreen::GotoPreviousMenu()
{
    ((CSourceMenuScreen*)CSourceMenuScreen::GetSourceMenuScreen())->RefreshSource(true);
    CDynamicMenuScreen::GotoPreviousMenu();
}

void
CLibraryEntryMenuScreen::NotifyLostCurrentSource()
{
    CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
    switch (pLMS->GetBrowseSource()) //CDJPlayerState::GetInstance()->GetSource())
    {
    case CDJPlayerState::FML:
        // if the fml we're looking at got removed, then we need to let the user know and exit the screen.
        if (this == Presentation()->GetCurrentThing())
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH, LostCurrentSourceCallback);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_FML_NO_LONGER_AVAILABLE));
            Add(CAlertScreen::GetInstance());
        }
        break;
    case CDJPlayerState::CD:
        // if the cd we're looking at got removed, then we need to let the user know and exit the screen.
        if (this == Presentation()->GetCurrentThing())
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH, LostCurrentSourceCallback);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_CD_NO_LONGER_AVAILABLE));
            Add(CAlertScreen::GetInstance());
        }
        break;
        break;
    case CDJPlayerState::HD:
    case CDJPlayerState::LINE_IN:
    default:
        // we shouldn't loose these.  scarry!
        DEBUGP( DBG_LIBRARY_ENTRY_MENU_SCREEN, DBGLEV_ERROR, "lems: lost a source we shouldn't lose!\n");
        break;
    }
}

void
CLibraryEntryMenuScreen::LostCurrentSourceCB()
{
    CPlayerScreen* pPS = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    pPS->HideMenus();
    pPS->Add(CSourceMenuScreen::GetSourceMenuScreen());
    Presentation()->MoveFocusTree(CSourceMenuScreen::GetSourceMenuScreen());
}

void
CLibraryEntryMenuScreen::LostCurrentSourceCallback()
{
    if( s_pLibraryEntryMenuScreen )
        s_pLibraryEntryMenuScreen->LostCurrentSourceCB();    
}

void
CLibraryEntryMenuScreen::SetBrowseMode(CLibraryMenuScreen::eBrowseMode eMode)
{
    switch(eMode)
    {
    case CLibraryMenuScreen::GENRE:
        m_iTopIndex = -1;
        break;
    case CLibraryMenuScreen::ARTIST:
        m_iTopIndex = 0;
        break;
    case CLibraryMenuScreen::ALBUM:
        m_iTopIndex = 1;
        break;
    case CLibraryMenuScreen::TRACK:
        m_iTopIndex = 2;
        break;
    case CLibraryMenuScreen::PLAYLIST:
        m_iTopIndex = 3;
        break;
    case CLibraryMenuScreen::RADIO:
        m_iTopIndex = 4;
        break;
    default:
        m_iTopIndex = -1;
        break;
    }

    // adjust to the source so we know if we can display radio stations or not
    if (((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->GetBrowseSource() == CDJPlayerState::FML)
        s_pLibraryEntryMenuScreen->SetItemCount(6);
    else
        s_pLibraryEntryMenuScreen->SetItemCount(5);
}

const TCHAR* 
CLibraryEntryMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch(iMenuIndex)
    {
    case 0: return LS(SID_GENRES);
    case 1: return LS(SID_ARTISTS);
    case 2: return LS(SID_ALBUMS);
    case 3: return LS(SID_TRACKS);
    case 4: return LS(SID_PLAYLISTS);
    case 5: return LS(SID_RADIO);
    default: return LS(SID_EMPTY_STRING);
    }
}

const TCHAR* 
CLibraryEntryMenuScreen::MenuTitleCaption()
{
    CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();

    switch (pLMS->GetBrowseSource())
    {
    case CDJPlayerState::HD:
        return LS(SID_HARD_DISK);
        break;
    case CDJPlayerState::CD:
        return LS(SID_CD_DRIVE);
        break;
    case CDJPlayerState::LINE_IN:
        return LS(SID_LINE_INPUT);
        break;
    case CDJPlayerState::FML:
        {
            // use the FML's real name if possible
            CIML* pIML = pLMS->GetBrowseIML();
            if (pIML) {
                return pIML->GetFriendlyName();
            }
            else {
                return 0;
            }
        }
        break;
    default:
        break;
    }

    return LS(SID_EMPTY_STRING);
}


