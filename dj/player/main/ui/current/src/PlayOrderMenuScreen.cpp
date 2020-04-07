//
// PlayOrderMenuScreen.cpp: implementation of PlayOrderMenuScreen class
// danb@fullplaymedia.com 09/28/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/PlayOrderMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>
#include <main/ui/PlayerScreen.h>
#include <main/main/Recording.h>

// the global reference for this class
CPlayOrderMenuScreen* CPlayOrderMenuScreen::s_pPlayOrderMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
    { true, true, false, SID_NORMAL, NULL },
    { false, true, false, SID_RANDOM, NULL },
    { false, true, false, SID_REPEAT, NULL },
    { false, true, false, SID_REPEAT_RANDOM, NULL },
};


// This is a singleton class.
CScreen*
CPlayOrderMenuScreen::GetPlayOrderMenuScreen()
{
	if (!s_pPlayOrderMenuScreen) {
		s_pPlayOrderMenuScreen = new CPlayOrderMenuScreen(NULL);
	}
	return s_pPlayOrderMenuScreen;
}


CPlayOrderMenuScreen::CPlayOrderMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_PLAY_MODE, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
}

CPlayOrderMenuScreen::~CPlayOrderMenuScreen()
{
}

void
CPlayOrderMenuScreen::SetPlayMode(IPlaylist::PlaylistMode ePlaylistMode)
{
    // unselect all of the options
    for (int i = 0; i < m_cItems; i++)
    {
        s_menuItems[i].bSelected = false;
    }

    switch (ePlaylistMode)
    {
    case IPlaylist::NORMAL:
        s_menuItems[0].bSelected = true;
        SetHighlightedIndex(0);
        break;
    case IPlaylist::RANDOM:
        s_menuItems[1].bSelected = true;
        SetHighlightedIndex(1);
        break;
    case IPlaylist::REPEAT_ALL:
        s_menuItems[2].bSelected = true;
        SetHighlightedIndex(2);
        break;
    case IPlaylist::REPEAT_RANDOM:
        s_menuItems[3].bSelected = true;
        SetHighlightedIndex(3);
        break;
    default:
        break;
    };
}

int
CPlayOrderMenuScreen::GetSelectedItem()
{
    for (int i = 0; i < m_cItems; ++i)
    {
        if (s_menuItems[i].bSelected)
            return i;
    }
    return 0;
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CPlayOrderMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    // the user can't change the play order from normal while ripping an entire cd
    if (CRecordingManager::GetInstance()->IsRipping() && !CRecordingManager::GetInstance()->IsRippingSingle())
    {
        CPlayerScreen::GetPlayerScreen()->SetMessageText(LS(SID_CANT_CHANGE_PLAY_MODE_WHILE_FAST_RECORDING));
    }
    // the user can't change the play order from normal while in line-in mode
    else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
    {
        CPlayerScreen::GetPlayerScreen()->SetMessageText(LS(SID_CANT_CHANGE_PLAY_MODE_OF_LINE_INPUT_SOURCE));
    }
    else
    {
	    switch (iMenuIndex)
	    {
		    case 0:		// set normal playlist mode
            {
                SetPlayMode(IPlaylist::NORMAL);
                CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
                CPlayerScreen::GetPlayerScreen()->SetPlayModeTextByPlaylistMode(IPlaylist::NORMAL);
			    break;
            }
		    case 1:		// random
            {
                SetPlayMode(IPlaylist::RANDOM);
                CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::RANDOM);
                CPlayerScreen::GetPlayerScreen()->SetPlayModeTextByPlaylistMode(IPlaylist::RANDOM);
			    break;
            }
		    case 2:		// repeat all
            {
                SetPlayMode(IPlaylist::REPEAT_ALL);
                CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::REPEAT_ALL);
                CPlayerScreen::GetPlayerScreen()->SetPlayModeTextByPlaylistMode(IPlaylist::REPEAT_ALL);
			    break;
            }
		    case 3:		// repeat rand
            {
                SetPlayMode(IPlaylist::REPEAT_RANDOM);
                CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::REPEAT_RANDOM);
                CPlayerScreen::GetPlayerScreen()->SetPlayModeTextByPlaylistMode(IPlaylist::REPEAT_RANDOM);
			    break;
            }
	    }
    }

    Draw();
}
