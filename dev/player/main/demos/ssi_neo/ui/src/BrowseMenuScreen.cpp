//........................................................................................
//........................................................................................
//.. File Name: BrowseMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseMenuScreen class	 				..
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
#include <main/demos/ssi_neo/ui/BrowseMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

CBrowseMenuScreen* CBrowseMenuScreen::s_pBrowseMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ &CBrowseArtistsMenuScreen::GetBrowseArtistsMenuScreen, &gbEmptyBitmap, SID_ARTISTS, true, false },
	{ &CBrowseAlbumsMenuScreen::GetBrowseAlbumsMenuScreen, &gbEmptyBitmap, SID_ALBUMS, true, false },
	{ &CBrowseTitlesMenuScreen::GetBrowseTitlesMenuScreen, &gbEmptyBitmap, SID_TITLES, true, false },
	{ &CBrowseGenresMenuScreen::GetBrowseGenresMenuScreen, &gbEmptyBitmap, SID_GENRES, true, false },
	{ &CBrowseFoldersMenuScreen::GetBrowseFoldersMenuScreen, &gbEmptyBitmap, SID_FOLDERS, true, false },
	{ &CBrowseRecordingsMenuScreen::GetBrowseRecordingsMenuScreen, &gbEmptyBitmap, SID_RECORDINGS, true, false }
};


// This is a singleton class.
CScreen*
CBrowseMenuScreen::GetBrowseMenuScreen()
{
	if (!s_pBrowseMenuScreen) {
		s_pBrowseMenuScreen = new CBrowseMenuScreen();
	}
	return s_pBrowseMenuScreen;
}


CBrowseMenuScreen::CBrowseMenuScreen()
  : CMenuScreen(0, SID_BROWSE_MENUS, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec)),
  	m_bStopDown(false),
	m_bPrevDown(false)
{
}

CBrowseMenuScreen::~CBrowseMenuScreen()
{
}


SIGNED
CBrowseMenuScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
		case PM_KEY:

			switch (Mesg.iData)
			{
				case KEY_STOP:
					m_bStopDown = true;
					return CScrollingListScreen::Message(Mesg);

				case KEY_PREVIOUS:
					m_bPrevDown = true;
					return CScrollingListScreen::Message(Mesg);

                case KEY_BROWSE:
					Parent()->Remove(this);
					Presentation()->MoveFocusTree(CPlayerScreen::GetPlayerScreen());
					ResetToTop();
					return CScrollingListScreen::Message(Mesg);

				default:
					return CMenuScreen::Message(Mesg);
			}
			break;

		case PM_KEY_RELEASE:
			switch (Mesg.iData)
			{
				case KEY_STOP:
					if (m_bStopDown)
					{
						m_bStopDown = m_bPrevDown = false;
						Parent()->Remove(this);
						Presentation()->MoveFocusTree(CPlayerScreen::GetPlayerScreen());
						ResetToTop();
					}
					return CScrollingListScreen::Message(Mesg);

				case KEY_PREVIOUS:
					if (m_bPrevDown)
					{
						m_bStopDown = m_bPrevDown = false;
						Parent()->Remove(this);
						Presentation()->MoveFocusTree(CPlayerScreen::GetPlayerScreen());
						ResetToTop();
					}
					return CScrollingListScreen::Message(Mesg);
                
				default:
					return CMenuScreen::Message(Mesg);
			}
			break;

		default:
			return CMenuScreen::Message(Mesg);
	}
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case -1:	// Exit
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(m_pParent);
			ResetToTop();
			break;

		case 0:		// Browse by Artist
			Parent()->Add(CBrowseArtistsMenuScreen::GetBrowseArtistsMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseArtistsMenuScreen::GetBrowseArtistsMenuScreen());
			break;

		case 1:		// Browse by Album
			Parent()->Add(CBrowseAlbumsMenuScreen::GetBrowseAlbumsMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseAlbumsMenuScreen::GetBrowseAlbumsMenuScreen());
			break;

		case 2:		// Browse by Title
			Parent()->Add(CBrowseTitlesMenuScreen::GetBrowseTitlesMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseTitlesMenuScreen::GetBrowseTitlesMenuScreen());
			break;

		case 3:		// Browse by Genre
			Parent()->Add(CBrowseGenresMenuScreen::GetBrowseGenresMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseGenresMenuScreen::GetBrowseGenresMenuScreen());
			break;

		case 4:		// Browse by Folders
			Parent()->Add(CBrowseFoldersMenuScreen::GetBrowseFoldersMenuScreen());
            ((CBrowseFoldersMenuScreen*)CBrowseFoldersMenuScreen::GetBrowseFoldersMenuScreen())->Init();
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseFoldersMenuScreen::GetBrowseFoldersMenuScreen());
			break;

		case 5:		// Browse by Recordings
			Parent()->Add(CBrowseRecordingsMenuScreen::GetBrowseRecordingsMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CBrowseRecordingsMenuScreen::GetBrowseRecordingsMenuScreen());
			break;
	};
}

