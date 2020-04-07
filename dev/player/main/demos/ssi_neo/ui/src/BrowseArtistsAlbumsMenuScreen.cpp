//........................................................................................
//........................................................................................
//.. File Name: BrowseArtistsAlbumsMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseArtistsAlbumsMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseArtistsAlbumsMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <core/playmanager/PlayManager.h>

CBrowseArtistsAlbumsMenuScreen* CBrowseArtistsAlbumsMenuScreen::s_pBrowseArtistsAlbumsMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, true },
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, false },
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseArtistsAlbumsMenuScreen::GetBrowseArtistsAlbumsMenuScreen()
{
	if (!s_pBrowseArtistsAlbumsMenuScreen) {
		s_pBrowseArtistsAlbumsMenuScreen = new CBrowseArtistsAlbumsMenuScreen();
	}
	return s_pBrowseArtistsAlbumsMenuScreen;
}


CBrowseArtistsAlbumsMenuScreen::CBrowseArtistsAlbumsMenuScreen()
  : CDynamicMenuScreen(&CBrowseArtistsAlbumsTitlesMenuScreen::GetBrowseArtistsAlbumsTitlesMenuScreen, SID_BROWSE_ALBUMS)
{
	//SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

void CBrowseArtistsAlbumsMenuScreen::SetArtist(const TCHAR* szArtist)
{
    tstrncpy(m_szArtist, szArtist,PLAYLIST_STRING_SIZE);
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    m_iArtistKey = pQCM->GetArtistKey(m_szArtist);
    pQCM->GetAlbums(m_Albums,m_iArtistKey);
    SetItemCount(m_Albums.Size() + 1);  // one for play all
}

void CBrowseArtistsAlbumsMenuScreen::Minimize()
{
    m_Albums.Clear();
}

CBrowseArtistsAlbumsMenuScreen::~CBrowseArtistsAlbumsMenuScreen()
{
}


bool 
CBrowseArtistsAlbumsMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].bIsChecked;
    return false;
}

bool 
CBrowseArtistsAlbumsMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	if(iMenuIndex == 0) // Play All
		return false;
	else
		//return s_menuItems[iMenuIndex].bHasSubmenu;
        return true;
}


const TCHAR* 
CBrowseArtistsAlbumsMenuScreen::MenuItemCaption(int iMenuIndex)
{
	//if(s_menuItems[iMenuIndex].bSelectAll) // Play All
    if(iMenuIndex == 0 ) // Play All
		return LS(SID_PLAY_ALL);
	else
		//return LS(s_menuItems[iMenuIndex].wCaptionSID);
        return m_Albums[iMenuIndex-1].szValue;
}


PegBitmap* 
CBrowseArtistsAlbumsMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].pBitmap;
    return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseArtistsAlbumsMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	int i = 0;

	switch (iMenuIndex)
	{
		case -1:	// Exit
			Parent()->Add(m_pParent);
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(m_pParent);
			ResetToTop();
			break;


		default:
			//if(s_menuItems[iMenuIndex].bSelectAll) // Play All
            if (iMenuIndex == 0) // Play All
			{
				// Talk to the database to add all the songs to the playlist
				break;
			}

			// Talk to the child screen and give it the right query info
            ((CBrowseArtistsAlbumsTitlesMenuScreen*)CBrowseArtistsAlbumsTitlesMenuScreen::GetBrowseArtistsAlbumsTitlesMenuScreen())->SetArtistAlbum(m_szArtist,m_Albums[iMenuIndex-1].szValue);
			Parent()->Add(CBrowseArtistsAlbumsTitlesMenuScreen::GetBrowseArtistsAlbumsTitlesMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseArtistsAlbumsTitlesMenuScreen::GetBrowseArtistsAlbumsTitlesMenuScreen());
			break;
	};
}

