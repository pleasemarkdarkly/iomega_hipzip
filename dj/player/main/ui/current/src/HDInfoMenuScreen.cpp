//
// HDInfoMenuScreen.cpp: implementation of CHDInfoMenuScreen class
// danb@fullplaymedia.com 07/22/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/HDInfoMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/main/AppSettings.h>
#include <main/main/SpaceMgr.h>
#include <main/content/djcontentmanager/DJContentManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <core/playmanager/PlayManager.h>
#include <stdio.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_HD_INFO_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_HD_INFO_SCREEN );

// the global reference for this class
CHDInfoMenuScreen* CHDInfoMenuScreen::s_pHDInfoMenuScreen = 0;

// This is a singleton class.
CScreen*
CHDInfoMenuScreen::GetHDInfoMenuScreen()
{
	if (!s_pHDInfoMenuScreen) {
		s_pHDInfoMenuScreen = new CHDInfoMenuScreen(NULL);
	}
	return s_pHDInfoMenuScreen;
}


CHDInfoMenuScreen::CHDInfoMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_HD_INFO)
{
    SetItemCount(7);
}

CHDInfoMenuScreen::~CHDInfoMenuScreen()
{
}


void
CHDInfoMenuScreen::GetHDInfo()
{
    char szTemp[STR_SIZE];
    TCHAR tszTemp[STR_SIZE];
    CDJContentManager* pDJCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();
    int iFatDataSourceID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    // Free space info
    long long BytesFree = CSpaceMgr::GetInstance()->BytesFromLow();
    if (BytesFree >= (DJ_BYTES_PER_KILOBYTE * DJ_KILOBYTES_PER_MEGABYTE))
        sprintf(szTemp, ": %.2f GB", BytesFree / ((float)(DJ_BYTES_PER_KILOBYTE * DJ_KILOBYTES_PER_MEGABYTE * DJ_MEGABYTES_PER_GIGABYTE)));
    else if (BytesFree >= 1000)
        sprintf(szTemp, ": %.2f MB", BytesFree / ((float)(DJ_BYTES_PER_KILOBYTE * DJ_KILOBYTES_PER_MEGABYTE)));
    else
        sprintf(szTemp, ": %d kB", BytesFree / ((float)(DJ_BYTES_PER_KILOBYTE)));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszFreeDiskSpace, LS(SID_SPACE_REMAINING));
    tstrcat(m_pszFreeDiskSpace, tszTemp);

    // Free time info
    int iBitrate = CDJPlayerState::GetInstance()->GetEncodeBitrate();
    // quick hack for uncompressed bitrate, part 1
    if (iBitrate == 0)
        iBitrate = (44100 * 16 * 2) / DJ_BYTES_PER_KILOBYTE;
    
    long long lBytesPerSecond = (iBitrate * DJ_BYTES_PER_KILOBYTE) / 8;
    long long lSecondsFree    = BytesFree / lBytesPerSecond;
    long long lMinutesFree    = lSecondsFree / 60;
    long long lHoursFree      = lMinutesFree / 60;
    lMinutesFree              = lMinutesFree % 60;

    // quick hack for uncompressed bitrate, part 2
    if (CDJPlayerState::GetInstance()->GetEncodeBitrate() == 0)
        iBitrate = 1400;
    sprintf(szTemp, ": %dh %dm (%d kbps)", (unsigned int)lHoursFree, (unsigned int)lMinutesFree, iBitrate);
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszFreeDiskTime, LS(SID_TIME_REMAINING));
    tstrcat(m_pszFreeDiskTime, tszTemp);
    
    // Tracks
    sprintf(szTemp, ": %d", pDJCM->GetMediaRecordCount(iFatDataSourceID));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszNumberOfTracks, LS(SID_TRACKS));
    tstrcat(m_pszNumberOfTracks, tszTemp);

    // Albums
    sprintf(szTemp, ": %d", pDJCM->GetAlbumCount(iFatDataSourceID));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszNumberOfAlbums, LS(SID_ALBUMS));
    tstrcat(m_pszNumberOfAlbums, tszTemp);

    // Artists
    sprintf(szTemp, ": %d", pDJCM->GetArtistCount(iFatDataSourceID));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszNumberOfArtists, LS(SID_ARTISTS));
    tstrcat(m_pszNumberOfArtists, tszTemp);

    // Genres
    sprintf(szTemp, ": %d", pDJCM->GetGenreCount(iFatDataSourceID));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszNumberOfGenres, LS(SID_GENRES));
    tstrcat(m_pszNumberOfGenres, tszTemp);

    // Playlists
    sprintf(szTemp, ": %d", pDJCM->GetPlaylistRecordCount(iFatDataSourceID));
    CharToTchar(tszTemp, szTemp);
    tstrcpy(m_pszNumberOfPlaylists, LS(SID_PLAYLISTS));
    tstrcat(m_pszNumberOfPlaylists, tszTemp);
}


const TCHAR* 
CHDInfoMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:
        return m_pszFreeDiskSpace;
    case 1:
        return m_pszFreeDiskTime;
    case 2:
        return m_pszNumberOfTracks;
    case 3:
        return m_pszNumberOfAlbums;
    case 4:
        return m_pszNumberOfArtists;
    case 5:
        return m_pszNumberOfGenres;
    case 6:
        return m_pszNumberOfPlaylists;
    default:
        return LS(SID_EMPTY_STRING);
    }
}
