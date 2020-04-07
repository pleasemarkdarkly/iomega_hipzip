//........................................................................................
//........................................................................................
//.. File Name: MainMenuScreen.cpp														..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: implementation of CMainMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Ed Miller	edwardm@iobjects.com								..
//.. Modification date: 10/18/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include "MainMenuScreen.h"

//#include "PlayScreen.h"
#include "Bitmaps.h"
#include "Keys.h"
#include "Strings.hpp"
#include "UI.h"

//extern CPlayScreen* g_pMainWindow;
CMainMenuScreen* CMainMenuScreen::s_pMainMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
	{ NULL, &gbEmptyBitmap, SID_NEO_JUKEBOX, false, false },
	{ NULL, &gbEmptyBitmap, SID_FM_TUNER, false, false },
	{ NULL, &gbEmptyBitmap, SID_VOICE_RECORDER, false, false },
	{ &CPlaylistManagerMenuScreen::GetPlaylistManagerMenuScreen, &gbEmptyBitmap, SID_PLAYLIST_MANAGER, true, false },
	{ &CPlayModeMenuScreen::GetPlayModeMenuScreen, &gbEmptyBitmap, SID_PLAY_MODE, true, false },
	{ &CEqualizerMenuScreen::GetEqualizerMenuScreen, &gbEmptyBitmap, SID_EQUALIZER, true, false },
	{ &CSettingsMenuScreen::GetSettingsMenuScreen, &gbEmptyBitmap, SID_SETTINGS, true, false },
	{ &CSongInfoScreen::GetSongInfoScreen, &gbEmptyBitmap, SID_SONG_INFO, true, false },
	{ &CDiskManagerMenuScreen::GetDiskManagerMenuScreen, &gbEmptyBitmap, SID_DISK_MANAGER, true, false },
	{ &CPlayerInfoScreen::GetPlayerInfoScreen, &gbEmptyBitmap, SID_PLAYER_INFO, true, false }
};


// This is a singleton class.
CScreen*
CMainMenuScreen::GetMainMenuScreen()
{
	if (!s_pMainMenuScreen) {
		s_pMainMenuScreen = new CMainMenuScreen();
	}
	return s_pMainMenuScreen;
}


CMainMenuScreen::CMainMenuScreen()
  : CMenuScreen(0, SID_MENU, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec)),
  	m_bStopDown(false),
	m_bPrevDown(false)
{
}

CMainMenuScreen::~CMainMenuScreen()
{
}


SIGNED
CMainMenuScreen::Message(const PegMessage &Mesg)
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
CMainMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	switch (iMenuIndex)
	{
		case -1:	// Exit
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CPlayerScreen::GetPlayerScreen());
			ResetToTop();
			break;

		case 0:		// Jukebox
			// if we're not already in the jukebox screen
			if (Parent() != CPlayerScreen::GetPlayerScreen())
			{
				// stop all the other types of playback
				Parent()->Remove(this);
				Presentation()->Add(CPlayerScreen::GetPlayerScreen());
			}
			else
				Parent()->Remove(this);

			Presentation()->MoveFocusTree(CPlayerScreen::GetPlayerScreen());
			ResetToTop();
			break;
/*
		case 1:		// FM Tuner
			Parent()->Add(CFMTunerScreen::GetFMTunerScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CFMTunerScreen::GetFMTunerScreen());
			ResetToTop();
			break;

		case 2:		// Voice Recorder
			Parent()->Add(CVoiceRecorderScreen::GetVoiceRecorderScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CVoiceRecorderScreen::GetVoiceRecorderScreen());
			ResetToTop();
			break;
*/
		case 3:		// Playlist manager
			Parent()->Add(CPlaylistManagerMenuScreen::GetPlaylistManagerMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CPlaylistManagerMenuScreen::GetPlaylistManagerMenuScreen());
			break;

		case 4:		// Play mode
			Parent()->Add(CPlayModeMenuScreen::GetPlayModeMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CPlayModeMenuScreen::GetPlayModeMenuScreen());
			break;

		case 5:		// Equalizer
			Parent()->Add(CEqualizerMenuScreen::GetEqualizerMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CEqualizerMenuScreen::GetEqualizerMenuScreen());
			break;

		case 6:		// Settings
			Parent()->Add(CSettingsMenuScreen::GetSettingsMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CSettingsMenuScreen::GetSettingsMenuScreen());
			break;

		case 7:		// Song info
			Parent()->Add(CSongInfoScreen::GetSongInfoScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CSongInfoScreen::GetSongInfoScreen());
			break;

		case 8:		// Disk Manager
			Parent()->Add(CDiskManagerMenuScreen::GetDiskManagerMenuScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CDiskManagerMenuScreen::GetDiskManagerMenuScreen());
			break;

		case 9:		// Player info
			Parent()->Add(CPlayerInfoScreen::GetPlayerInfoScreen());
			Parent()->Remove(this);
			Presentation()->MoveFocusTree(CPlayerInfoScreen::GetPlayerInfoScreen());
			break;
	};
}

