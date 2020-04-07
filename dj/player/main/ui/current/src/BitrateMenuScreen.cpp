//........................................................................................
//........................................................................................
//.. File Name: BitrateMenuScreen.cpp														..
//.. Date: 12/20/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBitrateMenuScreen class					..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 12/20/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/ui/BitrateMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/ui/UI.h>
#include <main/ui/PlayerScreen.h>

#include <main/main/DJPlayerState.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_BITRATE_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_BITRATE_MENU_SCREEN );

// the global reference for this class
CBitrateMenuScreen* CBitrateMenuScreen::s_pBitrateMenuScreen = 0;

// Currently supported pem bitrates are: 64, 96, 112, 128, 160, 192, 224, 256

// This is a singleton class.
CScreen*
CBitrateMenuScreen::GetBitrateMenuScreen()
{
	if (!s_pBitrateMenuScreen) {
		s_pBitrateMenuScreen = new CBitrateMenuScreen();
	}
	return s_pBitrateMenuScreen;
}


CBitrateMenuScreen::CBitrateMenuScreen()
  : CDynamicMenuScreen(NULL, SID_RECORDING_BITRATE),
    m_iCurrentBitrateIndex(-1)
{
    // dc- This is a race condition, depending on when the UI is initialized vs. the DJPlayerState loads the registry,
    //     this value may or may not be set properly in the player state when this call is made. The workaround for this
    //     is for the player state to 'push' the value to us when it is initialized, which is mildly gross - similar work
    //     is done to notify other UI components that the registry is available.
    SetBitrate(CDJPlayerState::GetInstance()->GetEncodeBitrate());
}

CBitrateMenuScreen::~CBitrateMenuScreen()
{
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CBitrateMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    DEBUGP( DBG_BITRATE_MENU_SCREEN, DBGLEV_INFO, "CBitrateMenuScreen - Set The Encoder Bitrate\n");
    m_iCurrentBitrateIndex = iMenuIndex;
    // show the changes immediately, and do the work afterwards
    Draw();

    switch (iMenuIndex)
    {
    case 0:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(0);
        break;
    case 1:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(256);
        break;
    case 2:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(224);
        break;
    case 3:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(192);
        break;
    case 4:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(160);
        break;
    case 5:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(128);
        break;
    case 6:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(112);
        break;
    case 7:
        CDJPlayerState::GetInstance()->SetEncodeBitrate(96);
        break;
        //    case 8:
        //        CDJPlayerState::GetInstance()->SetEncodeBitrate(64);
        //        break;
    default:
        break;
    };
}

void
CBitrateMenuScreen::SetBitrate(int iBitrate)
{
    SetItemCount(8);
    int iMenuIndex = 0;

    // check and see if this new bitrate is valid
    switch (iBitrate)
    {
    case 0:     iMenuIndex = 0; break;
    case 256:   iMenuIndex = 1; break;
    case 224:   iMenuIndex = 2; break;
    case 192:   iMenuIndex = 3; break;
    case 160:   iMenuIndex = 4; break;
    case 128:   iMenuIndex = 5; break;
    case 112:   iMenuIndex = 6; break;
    case 96:    iMenuIndex = 7; break;
        //    case 64:    iMenuIndex = 8; break;
        break;
    }

    m_iCurrentBitrateIndex = iMenuIndex;
    SetHighlightedIndex( m_iCurrentBitrateIndex );
}

bool
CBitrateMenuScreen::MenuItemSelected(int iMenuIndex)
{
    return (iMenuIndex == m_iCurrentBitrateIndex);
}

const TCHAR* 
CBitrateMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch(iMenuIndex)
    {
    case 0: return LS(SID_1400_KBPS_UNCOMPRESSED);
    case 1: return LS(SID_256_KBPS_MP3);
    case 2: return LS(SID_224_KBPS_MP3);
    case 3: return LS(SID_192_KBPS_MP3);
    case 4: return LS(SID_160_KBPS_MP3);
    case 5: return LS(SID_128_KBPS_MP3);
    case 6: return LS(SID_112_KBPS_MP3);
    case 7: return LS(SID_96_KBPS_MP3);
        //    case 8: return LS(SID_64_KBPS_MP3);
    default: return LS(SID_EMPTY_STRING);
    }
}

