//........................................................................................
//........................................................................................
//.. File Name: BrowseAlbumsMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseAlbumsMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseAlbumsMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

#include <core/playmanager/PlayManager.h>

CBrowseAlbumsMenuScreen* CBrowseAlbumsMenuScreen::s_pBrowseAlbumsMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, false },
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, false },
	{ &gbEmptyBitmap, SID_ALBUMS, true, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseAlbumsMenuScreen::GetBrowseAlbumsMenuScreen()
{
	if (!s_pBrowseAlbumsMenuScreen) {
		s_pBrowseAlbumsMenuScreen = new CBrowseAlbumsMenuScreen();
	}
	return s_pBrowseAlbumsMenuScreen;
}


CBrowseAlbumsMenuScreen::CBrowseAlbumsMenuScreen()
  : CDynamicMenuScreen(&CBrowseAlbumsTitlesMenuScreen::GetBrowseAlbumsTitlesMenuScreen, SID_BROWSE_ALBUMS)
{
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    pQCM->GetAlbums(m_Albums);
    //m_Albums = CPlayManager::GetInstance()->GetContentManager()->GetAlbums();
    SetItemCount(m_Albums.Size());

    //	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

CBrowseAlbumsMenuScreen::~CBrowseAlbumsMenuScreen()
{
}


bool 
CBrowseAlbumsMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
    return false;
	//return s_menuItems[iMenuIndex].bIsChecked;
}

bool 
CBrowseAlbumsMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    return true;
	//turn s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CBrowseAlbumsMenuScreen::MenuItemCaption(int iMenuIndex)
{
    return m_Albums[iMenuIndex].szValue;
	//return LS(s_menuItems[iMenuIndex].wCaptionSID);
}


PegBitmap* 
CBrowseAlbumsMenuScreen::MenuItemBitmap(int iMenuIndex)
{
    return &gbEmptyBitmap;
	//return s_menuItems[iMenuIndex].pBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseAlbumsMenuScreen::ProcessMenuOption(int iMenuIndex)
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

