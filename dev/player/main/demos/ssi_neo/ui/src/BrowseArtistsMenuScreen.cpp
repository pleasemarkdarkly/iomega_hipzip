//........................................................................................
//........................................................................................
//.. File Name: BrowseArtistsMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseArtistsMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/BrowseArtistsMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <core/playmanager/PlayManager.h>

CBrowseArtistsMenuScreen* CBrowseArtistsMenuScreen::s_pBrowseArtistsMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_ARTISTS, true, false, false },
	{ &gbEmptyBitmap, SID_ARTISTS, true, false, false },
	{ &gbEmptyBitmap, SID_ARTISTS, true, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseArtistsMenuScreen::GetBrowseArtistsMenuScreen()
{
	if (!s_pBrowseArtistsMenuScreen) {
		s_pBrowseArtistsMenuScreen = new CBrowseArtistsMenuScreen();
	}
	return s_pBrowseArtistsMenuScreen;
}


CBrowseArtistsMenuScreen::CBrowseArtistsMenuScreen()
  : CDynamicMenuScreen(&CBrowseArtistsAlbumsMenuScreen::GetBrowseArtistsAlbumsMenuScreen, SID_BROWSE_ARTISTS)
{
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    pQCM->GetArtists(m_Artists);
/*
                        m_bRefreshArtists = false;
                    }
                    if (m_Artists.Size())
                    {
                        m_iArtistIndex = (m_iArtistIndex + 1) % m_Artists.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Artist: %s\n", m_Artists[m_iArtistIndex].iKey, TcharToChar(szValue, m_Artists[m_iArtistIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecordsByArtist(mrlTracks, m_Artists[m_iArtistIndex].iKey);
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Artists[m_iArtistIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();
                        m_pUserInterface->NotifyPlaying();
  */  
    SetItemCount(m_Artists.Size());
}

CBrowseArtistsMenuScreen::~CBrowseArtistsMenuScreen()
{
}


bool 
CBrowseArtistsMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
    return false;
	//return s_menuItems[iMenuIndex].bIsChecked;
}

bool 
CBrowseArtistsMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return true;
}


const TCHAR* 
CBrowseArtistsMenuScreen::MenuItemCaption(int iMenuIndex)
{
    return m_Artists[iMenuIndex].szValue;
    //return LS(s_menuItems[iMenuIndex].wCaptionSID);
}


PegBitmap* 
CBrowseArtistsMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseArtistsMenuScreen::ProcessMenuOption(int iMenuIndex)
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
            ((CBrowseArtistsAlbumsMenuScreen*)CBrowseArtistsAlbumsMenuScreen::GetBrowseArtistsAlbumsMenuScreen())->SetArtist(m_Artists[iMenuIndex].szValue);
			Parent()->Add(CBrowseArtistsAlbumsMenuScreen::GetBrowseArtistsAlbumsMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseArtistsAlbumsMenuScreen::GetBrowseArtistsAlbumsMenuScreen());

			break;
	};
}

