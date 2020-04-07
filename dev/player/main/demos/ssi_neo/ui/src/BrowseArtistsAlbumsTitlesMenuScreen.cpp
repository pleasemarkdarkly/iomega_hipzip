//........................................................................................
//........................................................................................
//.. File Name: BrowseArtistsAlbumsTitlesMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseArtistsAlbumsTitlesMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseArtistsAlbumsTitlesMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <core/playmanager/PlayManager.h>

CBrowseArtistsAlbumsTitlesMenuScreen* CBrowseArtistsAlbumsTitlesMenuScreen::s_pBrowseArtistsAlbumsTitlesMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_TITLES, false, false, true },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false },
	{ &gbEmptyBitmap, SID_TITLES, false, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseArtistsAlbumsTitlesMenuScreen::GetBrowseArtistsAlbumsTitlesMenuScreen()
{
	if (!s_pBrowseArtistsAlbumsTitlesMenuScreen) {
		s_pBrowseArtistsAlbumsTitlesMenuScreen = new CBrowseArtistsAlbumsTitlesMenuScreen();
	}
	return s_pBrowseArtistsAlbumsTitlesMenuScreen;
}

CBrowseArtistsAlbumsTitlesMenuScreen::CBrowseArtistsAlbumsTitlesMenuScreen()
  : CDynamicMenuScreen(NULL, SID_BROWSE_TITLES)
{
//	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

void CBrowseArtistsAlbumsTitlesMenuScreen::SetArtistAlbum(const TCHAR* szArtist, const TCHAR* szAlbum)
{
    tstrncpy(m_szArtist, szArtist, PLAYLIST_STRING_SIZE);
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    m_iAlbumKey = pQCM->GetAlbumKey(szAlbum);
    m_iArtistKey = pQCM->GetArtistKey(szArtist);
    pQCM->GetMediaRecords(m_mrlTracks,m_iArtistKey,m_iAlbumKey);
/*    
    CPlayManager::GetInstance()->GetContentManager()->GetMediaRecordsByAlbum(m_mrlTracks,m_iAlbumKey);
    for (MediaRecordIterator it = m_mrlTracks.GetHead(); it != m_mrlTracks.GetEnd(); )
    {
        if (tstrcmp((*it)->GetArtist(),m_szArtist))
        {
            m_mrlTracks.Remove(it);
            it = m_mrlTracks.GetHead();
        }
        else
            ++it;
    }
*/
    m_iIterIndex = 0;
    m_itTrack = m_mrlTracks.GetHead();
    SetItemCount(m_mrlTracks.Size() + 1);  // one extra for play all
}

void CBrowseArtistsAlbumsTitlesMenuScreen::Minimize()
{
    m_mrlTracks.Clear();
}

CBrowseArtistsAlbumsTitlesMenuScreen::~CBrowseArtistsAlbumsTitlesMenuScreen()
{
}


bool 
CBrowseArtistsAlbumsTitlesMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].bIsChecked;
    // TODO: checking and stuff!
    return false;
}

bool 
CBrowseArtistsAlbumsTitlesMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].bHasSubmenu;
    return false;
}


const TCHAR* 
CBrowseArtistsAlbumsTitlesMenuScreen::MenuItemCaption(int iMenuIndex)
{
	//if(s_menuItems[iMenuIndex].bSelectAll) // Play All
    if (iMenuIndex == 0)
		return LS(SID_PLAY_ALL);
	else
    {
        // the grossness of keeping around an iterator and then just moving it to the desired entry is, I hope,
        // offset by the alternative grossness of starting a new iterator at the beginning of the list on each
        // lookup.  this should be an advantage if access is consecutive.
        while ( m_iIterIndex > iMenuIndex-1)
        {
            --m_iIterIndex;
            --m_itTrack;
        }
        while ( m_iIterIndex < iMenuIndex-1 )
        {
            ++m_iIterIndex;
            ++m_itTrack;
        }
        void* pData;
        (*m_itTrack)->GetAttribute(MDA_TITLE,&pData);
        return (TCHAR*) pData;
        //return LS(s_menuItems[iMenuIndex].wCaptionSID);
    }
}


PegBitmap* 
CBrowseArtistsAlbumsTitlesMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].pBitmap;
    return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseArtistsAlbumsTitlesMenuScreen::ProcessMenuOption(int iMenuIndex)
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
            if (iMenuIndex == 0)
			{
				//for(i = iMenuIndex + 1; i < m_cItems; i++)
				//	s_menuItems[i].bIsChecked = true;
				ForceRedraw();
				break;
			}

			// add or remove the file from the current playlist
            /*
			if(s_menuItems[iMenuIndex].bIsChecked)
			{
				s_menuItems[iMenuIndex].bIsChecked = false;
			}
			else
			{
				s_menuItems[iMenuIndex].bIsChecked = true;
			}
            */
			ForceRedraw();
			break;
	};
}

