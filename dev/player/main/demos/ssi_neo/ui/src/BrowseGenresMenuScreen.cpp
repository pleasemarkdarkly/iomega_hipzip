//........................................................................................
//........................................................................................
//.. File Name: BrowseGenresMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseGenresMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/BrowseGenresMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <core/playmanager/PlayManager.h>

CBrowseGenresMenuScreen* CBrowseGenresMenuScreen::s_pBrowseGenresMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_GENRES, true, false, false },
	{ &gbEmptyBitmap, SID_GENRES, true, false, false },
	{ &gbEmptyBitmap, SID_GENRES, true, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseGenresMenuScreen::GetBrowseGenresMenuScreen()
{
	if (!s_pBrowseGenresMenuScreen) {
		s_pBrowseGenresMenuScreen = new CBrowseGenresMenuScreen();
	}
	return s_pBrowseGenresMenuScreen;
}


CBrowseGenresMenuScreen::CBrowseGenresMenuScreen()
  : CDynamicMenuScreen(&CBrowseGenresTitlesMenuScreen::GetBrowseGenresTitlesMenuScreen, SID_BROWSE_GENRES)
{
    /*
                        if (m_bRefreshGenres)
                    {
                        m_bRefreshGenres = false;
                    }
                    if (m_Genres.Size())
                    {
                        m_iGenreIndex = (m_iGenreIndex + 1) % m_Genres.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Genre: %s\n", m_Genres[m_iGenreIndex].iKey, TcharToChar(szValue, m_Genres[m_iGenreIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecordsByGenre(mrlTracks, m_Genres[m_iGenreIndex].iKey);

                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Genres[m_iGenreIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();

                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;

    */
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    pQCM->GetGenres(m_Genres);
    SetItemCount(m_Genres.Size());

//    SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

CBrowseGenresMenuScreen::~CBrowseGenresMenuScreen()
{
}


bool 
CBrowseGenresMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
//	return s_menuItems[iMenuIndex].bIsChecked;
    return false;
}

bool 
CBrowseGenresMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
//	return s_menuItems[iMenuIndex].bHasSubmenu;
    return true;
}


const TCHAR* 
CBrowseGenresMenuScreen::MenuItemCaption(int iMenuIndex)
{
    return m_Genres[iMenuIndex].szValue;
	//return LS(s_menuItems[iMenuIndex].wCaptionSID);
}


PegBitmap* 
CBrowseGenresMenuScreen::MenuItemBitmap(int iMenuIndex)
{
   	return &gbEmptyBitmap;
//	return s_menuItems[iMenuIndex].pBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseGenresMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case -1:	// Exit
			Parent()->Add(m_pParent);
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(m_pParent);
			ResetToTop();
			break;

		default:
			// Talk to the child screen and give it the right query info
			Parent()->Add((*m_pGetChildScreenFunc)());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree((*m_pGetChildScreenFunc)());

			break;
	};
}

