//
// CustomizeMenuScreen.cpp: implementation of CCustomizeMenuScreen class
// danb@fullplaymedia.com 07/23/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/CustomizeMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/PlayerScreen.h>
#include <main/main/DJPlayerState.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_CUSTOMIZE_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_CUSTOMIZE_SCREEN );

#define LCD_BRIGHTNESS 0
//#define LCD_BRIGHTNESS 1

// the global reference for this class
CCustomizeMenuScreen* CCustomizeMenuScreen::s_pCustomizeMenuScreen = 0;

// This is a singleton class.
CScreen*
CCustomizeMenuScreen::GetCustomizeMenuScreen()
{
	if (!s_pCustomizeMenuScreen) {
		s_pCustomizeMenuScreen = new CCustomizeMenuScreen(NULL);
	}
	return s_pCustomizeMenuScreen;
}

CCustomizeMenuScreen::CCustomizeMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_CUSTOMIZATIONS)
{
    SetItemCount(8 + LCD_BRIGHTNESS);
}

CCustomizeMenuScreen::~CCustomizeMenuScreen()
{
}

void
CCustomizeMenuScreen::RefreshInfo()
{
    CDJPlayerState* pDJPS = CDJPlayerState::GetInstance();

    // eject cd
    tstrcpy(m_pszEjectCD, LS(SID_EJECT_CD_TRAY_AFTER_FAST_RECORD));
    tstrcat(m_pszEjectCD, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIEjectCDAfterRip())
        tstrcat(m_pszEjectCD, LS(SID_YES));
    else
        tstrcat(m_pszEjectCD, LS(SID_NO));

    // extended char
    tstrcpy(m_pszExtendedChars, LS(SID_ENABLE_EXTENDED_CHAR_EDITING));
    tstrcat(m_pszExtendedChars, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIEnableExtChars())
        tstrcat(m_pszExtendedChars, LS(SID_YES));
    else
        tstrcat(m_pszExtendedChars, LS(SID_NO));

    // web control
    tstrcpy(m_pszWebControl, LS(SID_ENABLE_WEB_CONTROL));
    tstrcat(m_pszWebControl, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIEnableWebControl())
        tstrcat(m_pszWebControl, LS(SID_YES));
    else
        tstrcat(m_pszWebControl, LS(SID_NO));

    // track number in title
    tstrcpy(m_pszTrackNumber, LS(SID_SHOW_TRACK_NUMBER_IN_TITLE));
    tstrcat(m_pszTrackNumber, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIShowTrackNumInTitle())
        tstrcat(m_pszTrackNumber, LS(SID_YES));
    else
        tstrcat(m_pszTrackNumber, LS(SID_NO));

    // album with artist name
    tstrcpy(m_pszAlbumWithArtistName, LS(SID_SHOW_ALBUM_WITH_ARTIST_NAME));
    tstrcat(m_pszAlbumWithArtistName, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIShowAlbumWithArtist())
        tstrcat(m_pszAlbumWithArtistName, LS(SID_YES));
    else
        tstrcat(m_pszAlbumWithArtistName, LS(SID_NO));

    // auto play a cd on insertion
    tstrcpy(m_pszPlayCDWhenInserted, LS(SID_PLAY_CD_WHEN_INSERTED));
    tstrcat(m_pszPlayCDWhenInserted, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIPlayCDWhenInserted())
        tstrcat(m_pszPlayCDWhenInserted, LS(SID_YES));
    else
        tstrcat(m_pszPlayCDWhenInserted, LS(SID_NO));

    // text scroll speed
    tstrcpy(m_pszTextScrollSpeed, LS(SID_SET_TEXT_SCROLL_SPEED));
    tstrcat(m_pszTextScrollSpeed, LS(SID_COLON_SPACE));
    if (pDJPS->GetUITextScrollSpeed() == CDJPlayerState::FAST)
        tstrcat(m_pszTextScrollSpeed, LS(SID_FAST));
    else if (pDJPS->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
        tstrcat(m_pszTextScrollSpeed, LS(SID_SLOW));
    else
        tstrcat(m_pszTextScrollSpeed, LS(SID_OFF));

    // enable led
    tstrcpy(m_pszEnableRecordLED, LS(SID_ENABLE_RECORD_LED));
    tstrcat(m_pszEnableRecordLED, LS(SID_COLON_SPACE));
    if (pDJPS->GetUIEnableRecordLED())
        tstrcat(m_pszEnableRecordLED, LS(SID_YES));
    else
        tstrcat(m_pszEnableRecordLED, LS(SID_NO));

    // lcd brightness
#if LCD_BRIGHTNESS
    tstrcpy(m_pszLCDBrightness, LS(SID_SET_LCD_BRIGHTNESS));
    tstrcat(m_pszLCDBrightness, LS(SID_COLON_SPACE));
    if (pDJPS->GetUILCDBrightness() == CDJPlayerState::BRIGHT)
        tstrcat(m_pszLCDBrightness, LS(SID_BRIGHT));
    else
        tstrcat(m_pszLCDBrightness, LS(SID_DIM));
#endif
}

void
CCustomizeMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
    CDJPlayerState* pDJPS = CDJPlayerState::GetInstance();

    switch (iMenuIndex)
    {
    case 0:
        pDJPS->SetUIEjectCDAfterRip(!pDJPS->GetUIEjectCDAfterRip());
        break;
    case 1:
        pDJPS->SetUIEnableExtChars(!pDJPS->GetUIEnableExtChars());
        break;
    case 2:
        pDJPS->SetUIEnableWebControl(!pDJPS->GetUIEnableWebControl());
        break;
    case 3:
        pDJPS->SetUIShowTrackNumInTitle(!pDJPS->GetUIShowTrackNumInTitle());
        pPS->RefreshCurrentTrackMetadata();
        break;
    case 4:
        pDJPS->SetUIShowAlbumWithArtist(!pDJPS->GetUIShowAlbumWithArtist());
        pPS->RefreshCurrentTrackMetadata();
        break;
    case 5:
        pDJPS->SetUIPlayCDWhenInserted(!pDJPS->GetUIPlayCDWhenInserted());
        break;
    case 6:
        if (pDJPS->GetUITextScrollSpeed() == CDJPlayerState::FAST)
            pDJPS->SetUITextScrollSpeed(CDJPlayerState::SLOW);
        else if (pDJPS->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
            pDJPS->SetUITextScrollSpeed(CDJPlayerState::OFF);
        else
            pDJPS->SetUITextScrollSpeed(CDJPlayerState::FAST);
        break;
    case 7:
        pDJPS->SetUIEnableRecordLED(!pDJPS->GetUIEnableRecordLED());
        break;
#if LCD_BRIGHTNESS
    case 8:
        if (pDJPS->GetUILCDBrightness() == CDJPlayerState::BRIGHT)
            pDJPS->SetUILCDBrightness(CDJPlayerState::DIM);
        else
            pDJPS->SetUILCDBrightness(CDJPlayerState::BRIGHT);
        break;
#endif
    default:
        break;
    }

    RefreshInfo();
    Draw();
}

const TCHAR* 
CCustomizeMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:
        return m_pszEjectCD;
    case 1:
        return m_pszExtendedChars;
    case 2:
        return m_pszWebControl;
    case 3:
        return m_pszTrackNumber;
    case 4:
        return m_pszAlbumWithArtistName;
    case 5:
        return m_pszPlayCDWhenInserted;
    case 6:
        return m_pszTextScrollSpeed;
    case 7:
        return m_pszEnableRecordLED;
#if LCD_BRIGHTNESS
    case 8:
        return m_pszLCDBrightness;
#endif
    default:
        return LS(SID_EMPTY_STRING);
    }
}
