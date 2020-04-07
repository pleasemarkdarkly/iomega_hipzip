//........................................................................................
//........................................................................................
//.. File Name: PlaylistMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CPlaylistMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/PlaylistMenuScreen.h>
#include <main/main/FatHelper.h>
#include <main/main/PlaylistConstraint.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>
#include <codec/codecmanager/CodecManager.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_PLAYLIST_MENU, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_PLAYLIST_MENU);

CPlaylistMenuScreen* CPlaylistMenuScreen::s_pPlaylistMenuScreen = 0;

// This is a singleton class.
CScreen*
CPlaylistMenuScreen::GetPlaylistMenuScreen()
{
	if (!s_pPlaylistMenuScreen) {
		s_pPlaylistMenuScreen = new CPlaylistMenuScreen();
	}
	return s_pPlaylistMenuScreen;
}


CPlaylistMenuScreen::CPlaylistMenuScreen()
  : CDynamicMenuScreen(NULL, SID_SET)
{
	SetMenuTitle(LS(SID_PLAYLISTS));
}

CPlaylistMenuScreen::~CPlaylistMenuScreen()
{
}

void CPlaylistMenuScreen::Init()
{
    PopulatePlaylistList();
}

void CPlaylistMenuScreen::Minimize()
{
    m_cItems = 0;
    m_lstPlaylistRecords.Clear();
    m_nPlaylistRecord = 0;
    m_itrPlaylistRecord = m_lstPlaylistRecords.GetEnd();
    ResetToTop();
}


bool 
CPlaylistMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    return false;
}

int CPlaylistMenuScreen::SyncItrToIndex(int iMenuIndex)
{
    if (m_itrPlaylistRecord == m_lstPlaylistRecords.GetEnd() )
    {
        m_itrPlaylistRecord = m_lstPlaylistRecords.GetHead();
        m_nPlaylistRecord = 0;
    }
    while (m_nPlaylistRecord < iMenuIndex)
    {
        ++m_itrPlaylistRecord;
        ++m_nPlaylistRecord;
    }
    while (m_nPlaylistRecord > iMenuIndex)
    {
        --m_itrPlaylistRecord;
        --m_nPlaylistRecord;
    }
    if (m_itrPlaylistRecord == m_lstPlaylistRecords.GetEnd())
        return 0;
    return m_nPlaylistRecord;
}

// return the item's caption in a temporary string location, only valid until the next call into this function.
// this assumes the caller will immediately copy the data out of the string and leave it alone thereafter.
const TCHAR* 
CPlaylistMenuScreen::MenuItemCaption(int iMenuIndex)
{
    static TCHAR caption[PLAYLIST_STRING_SIZE];
    if (SyncItrToIndex(iMenuIndex) != iMenuIndex)
    {
        DEBUGP( DBG_PLAYLIST_MENU, DBGLEV_WARNING, "MS:MenuItemCaption failed to sync to menu index\n");        
        return (TCHAR*) 0;
    }
    CharToTchar(caption, (*m_itrPlaylistRecord)->GetFileNameRef()->LongName());
    return caption;
}


PegBitmap* 
CPlaylistMenuScreen::MenuItemBitmap(int iMenuIndex)
{
    return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CPlaylistMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    // if there are no items in the screen, just exit.
    if (iMenuIndex > m_cItems - 1 || iMenuIndex < 0)
        return;
	// use the current selection to set the current query info 
	// as the playlist criteria and exit to the player screen
    if (SyncItrToIndex(iMenuIndex) != iMenuIndex) {
        DEBUGP( DBG_PLAYLIST_MENU, DBGLEV_WARNING, "MS:ProcMenuOpt failed to sync to menu index\n");        
        return;
    }
    CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
    pPC->Constrain();
    pPC->SetPlaylistFileNameRef((*m_itrPlaylistRecord)->GetFileNameRef());
    bool bCrntStillInList;
    // if the playlist won't load, we're all done (failure handler will get us out of menus, etc)
    if (!pPC->UpdatePlaylist(&bCrntStillInList))
        return;
    if (!bCrntStillInList)
        pPC->SyncPlayerToPlaylist();
    CPlayManager::GetInstance()->Play();
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
    Minimize();
	((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CPlaylistMenuScreen::GotoSubMenu(int iMenuIndex)
{
	// right now we don't do anything.  we rely on selecting the item
	// with the play/pause button and the ProcessMenuOption funtion.
}

void
CPlaylistMenuScreen::PopulatePlaylistList()
{
    CPlayManager::GetInstance()->GetContentManager()->GetAllPlaylistRecords(m_lstPlaylistRecords);
    SetItemCount(m_lstPlaylistRecords.Size());
}
