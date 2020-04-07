//........................................................................................
//........................................................................................
//.. File Name: BrowseRecordingsMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseRecordingsMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseRecordingsMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

CBrowseRecordingsMenuScreen* CBrowseRecordingsMenuScreen::s_pBrowseRecordingsMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_RECORDINGS, false, false, true },
	{ &gbEmptyBitmap, SID_RECORDINGS, false, false, false },
	{ &gbEmptyBitmap, SID_RECORDINGS, false, false, false },
	{ &gbEmptyBitmap, SID_RECORDINGS, false, false, false }
};


// This is a singleton class.
CScreen*
CBrowseRecordingsMenuScreen::GetBrowseRecordingsMenuScreen()
{
	if (!s_pBrowseRecordingsMenuScreen) {
		s_pBrowseRecordingsMenuScreen = new CBrowseRecordingsMenuScreen();
	}
	return s_pBrowseRecordingsMenuScreen;
}


CBrowseRecordingsMenuScreen::CBrowseRecordingsMenuScreen()
  : CDynamicMenuScreen(NULL, SID_BROWSE_RECORDINGS)
{
	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

CBrowseRecordingsMenuScreen::~CBrowseRecordingsMenuScreen()
{
}


bool 
CBrowseRecordingsMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bIsChecked;
}

bool 
CBrowseRecordingsMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CBrowseRecordingsMenuScreen::MenuItemCaption(int iMenuIndex)
{
	if(s_menuItems[iMenuIndex].bSelectAll) // Play All
		return LS(SID_PLAY_ALL);
	else
		return LS(s_menuItems[iMenuIndex].wCaptionSID);
}


PegBitmap* 
CBrowseRecordingsMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].pBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseRecordingsMenuScreen::ProcessMenuOption(int iMenuIndex)
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
			if(s_menuItems[iMenuIndex].bSelectAll) // Play All
			{
				for(i = iMenuIndex + 1; i < m_cItems; i++)
					s_menuItems[i].bIsChecked = true;
				ForceRedraw();
				break;
			}

			// add or remove the file from the current playlist
			if(s_menuItems[iMenuIndex].bIsChecked)
			{
				s_menuItems[iMenuIndex].bIsChecked = false;
			}
			else
			{
				s_menuItems[iMenuIndex].bIsChecked = true;
			}
			ForceRedraw();
			break;
	};
}
