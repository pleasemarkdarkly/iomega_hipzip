//........................................................................................
//........................................................................................
//.. File Name: BrowseTitlesMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseTitlesMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseTitlesMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

CBrowseTitlesMenuScreen* CBrowseTitlesMenuScreen::s_pBrowseTitlesMenuScreen = 0;

// todo: this structure is for testing only...
// the real info should be from the content manager
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


// This is a singleton class.
CScreen*
CBrowseTitlesMenuScreen::GetBrowseTitlesMenuScreen()
{
	if (!s_pBrowseTitlesMenuScreen) {
		s_pBrowseTitlesMenuScreen = new CBrowseTitlesMenuScreen();
	}
	return s_pBrowseTitlesMenuScreen;
}


CBrowseTitlesMenuScreen::CBrowseTitlesMenuScreen()
  : CDynamicMenuScreen(NULL, SID_BROWSE_TITLES)
{
	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
}

CBrowseTitlesMenuScreen::~CBrowseTitlesMenuScreen()
{
}


bool 
CBrowseTitlesMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bIsChecked;
}

bool 
CBrowseTitlesMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CBrowseTitlesMenuScreen::MenuItemCaption(int iMenuIndex)
{
	if(s_menuItems[iMenuIndex].bSelectAll) // Play All
		return LS(SID_PLAY_ALL);
	else
		return LS(s_menuItems[iMenuIndex].wCaptionSID);
}


PegBitmap* 
CBrowseTitlesMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].pBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseTitlesMenuScreen::ProcessMenuOption(int iMenuIndex)
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

