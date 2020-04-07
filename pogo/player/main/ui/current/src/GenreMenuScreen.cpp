//........................................................................................
//........................................................................................
//.. File Name: GenreMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CGenreMenuScreen class	 				..
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
#include <main/ui/GenreMenuScreen.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/PlaylistConstraint.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

// externs from BrowseMenuScreen, used for sorting
extern int CompareContentKeyValues(const void* a, const void* b);
extern void SortContentKeyValueVector(ContentKeyValueVector& keyValues);

CGenreMenuScreen* CGenreMenuScreen::s_pGenreMenuScreen = 0;

// This is a singleton class.
CScreen*
CGenreMenuScreen::GetGenreMenuScreen()
{
	if (!s_pGenreMenuScreen) {
		s_pGenreMenuScreen = new CGenreMenuScreen();
	}
	return s_pGenreMenuScreen;
}


CGenreMenuScreen::CGenreMenuScreen()
  : CDynamicMenuScreen(NULL, SID_SET)
{
	SetMenuTitle(LS(SID_GENRES));
}

void CGenreMenuScreen::Init()
{
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    if (!m_Genres.Size())
    {
        pQCM->GetGenres(m_Genres);
        if (m_Genres.Size() > 1)
            SortContentKeyValueVector(m_Genres);
        SetItemCount(m_Genres.Size());
    }
}

void CGenreMenuScreen::Minimize()
{
    m_cItems = 0;
    m_Genres.Clear();
    ResetToTop();
}

CGenreMenuScreen::~CGenreMenuScreen()
{
}

bool 
CGenreMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    return true;
}


const TCHAR* 
CGenreMenuScreen::MenuItemCaption(int iMenuIndex)
{
    return m_Genres[iMenuIndex].szValue;
}


PegBitmap* 
CGenreMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CGenreMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    if (m_cItems == 0)
        return;
	// use the current selection to set the current query info 
	// as the playlist criteria and exit to the player screen
	CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
    pPC->Constrain(CMK_ALL, CMK_ALL, m_Genres[iMenuIndex].iKey);
    bool bCrntStillInList;
    pPC->UpdatePlaylist(&bCrntStillInList);
    if (!bCrntStillInList)
        pPC->SyncPlayerToPlaylist();
    CPlayManager::GetInstance()->Play();
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
}

// Called when the user hits the next button.
// Moves to the Artist screen, constrained by the selected genre.
void
CGenreMenuScreen::GotoSubMenu(int iMenuIndex)
{
    if (m_cItems == 0)
        return;
    CBrowseMenuScreen* bms = (CBrowseMenuScreen*)CBrowseMenuScreen::GetBrowseMenuScreen();
    bms->SetParent(this);
    bms->SetConstraints(m_Genres[iMenuIndex].iKey);
    bms->SetBrowseMode(CBrowseMenuScreen::ARTIST);
    Parent()->Add(bms);
    Presentation()->MoveFocusTree(bms);
    this->HideScreen();
}

void
CGenreMenuScreen::GotoPreviousMenu()
{
    Minimize();
   	Parent()->Add(m_pParent);
	Parent()->Remove(this);
	Presentation()->MoveFocusTree(m_pParent);
	return;
}
