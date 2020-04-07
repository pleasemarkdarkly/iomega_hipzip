//
// SimpleGUIUserInterface.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/ui/simplegui/SimpleGUIUserInterface.h>
#include <codec/common/Codec.h>                 // for set_stream_event_data_t
#include <content/common/Metadata.h>            // for IMetadata
#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/font/BmpFont.h>
#include <playlist/common/Playlist.h>           // for IPlaylistEntry
#include <util/debug/debug.h>

#include <stdio.h>      // sprintf

DEBUG_MODULE_S(SGUI, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(SGUI);

extern CBmpFont Verdana12Font;

extern const unsigned char uc_Play_Bitmap[];
extern const unsigned char uc_Pause_Bitmap[];
extern const unsigned char uc_Stop_Bitmap[];

static const char* szDefaultBanner = "iObjects Dadio SDK";

static const int sc_iPlayStateHeight = 17;
static const int sc_iPlayStateWidth = 25;

static const int sc_iCharHeight = 18;
static const int sc_iCharSpacing = 0;

static const int sc_iScreenTop = 5;

CSimpleGUIUserInterface::CSimpleGUIUserInterface() 
    : m_bmpPlay (25, 17, 4, (unsigned char*)uc_Play_Bitmap), 
    m_bmpPause (25, 17, 4, (unsigned char*)uc_Pause_Bitmap),
    m_bmpStop (25, 17, 4, (unsigned char*)uc_Stop_Bitmap)
{
    m_show.AddScreen(&m_screen);

    int iTop = sc_iScreenTop;
    short iLeftMargin = 5;

    // Banner
    m_lblBanner.SetFont(&Verdana12Font);
    m_lblBanner.SetText(szDefaultBanner);
    m_lblBanner.m_clip.ul.x = iLeftMargin;
    m_lblBanner.m_clip.lr.x = 319;
    m_lblBanner.m_clip.ul.y = iTop;
    m_lblBanner.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblBanner.m_start.x = iLeftMargin;
    m_lblBanner.m_start.y = iTop;
    m_lblBanner.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblBanner);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Play
    m_bmpPlay.m_start.x = iLeftMargin;
    m_bmpPlay.m_start.y = iTop;
    m_bmpPlay.m_clip.ul.x = iLeftMargin;
    m_bmpPlay.m_clip.ul.y = iTop;
    m_bmpPlay.m_clip.lr.x = iLeftMargin + sc_iPlayStateWidth;
    m_bmpPlay.m_clip.lr.y = iTop + sc_iPlayStateHeight;
    m_bmpPlay.SetVisible(GUI_FALSE);
    m_screen.AddScreenElem(&m_bmpPlay);

    // Pause
    m_bmpPause.m_start.x = iLeftMargin;
    m_bmpPause.m_start.y = iTop;
    m_bmpPause.m_clip.ul.x = iLeftMargin;
    m_bmpPause.m_clip.ul.y = iTop;
    m_bmpPause.m_clip.lr.x = iLeftMargin + sc_iPlayStateWidth;
    m_bmpPause.m_clip.lr.y = iTop + sc_iPlayStateHeight;
    m_bmpPause.SetVisible(GUI_FALSE);
    m_screen.AddScreenElem(&m_bmpPause);

    // Stop
    m_bmpStop.m_start.x = iLeftMargin;
    m_bmpStop.m_start.y = iTop;
    m_bmpStop.m_clip.ul.x = iLeftMargin;
    m_bmpStop.m_clip.ul.y = iTop;
    m_bmpStop.m_clip.lr.x = iLeftMargin + sc_iPlayStateWidth;
    m_bmpStop.m_clip.lr.y = iTop + sc_iPlayStateHeight;
    m_bmpStop.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_bmpStop);

    // Track time
    m_lblTrackTime.SetFont(&Verdana12Font);
    m_lblTrackTime.SetText("");
    m_lblTrackTime.m_clip.ul.x = 105;
    m_lblTrackTime.m_clip.lr.x = 165;
    m_lblTrackTime.m_clip.ul.y = iTop;
    m_lblTrackTime.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTrackTime.m_start.x = m_lblTrackTime.m_clip.ul.x;
    m_lblTrackTime.m_start.y = m_lblTrackTime.m_clip.ul.y;
    m_lblTrackTime.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTrackTime);

    // Track number
    m_lblTrackNumber.SetFont(&Verdana12Font);
    m_lblTrackNumber.SetText("");
    m_lblTrackNumber.m_clip.ul.x = 200;
    m_lblTrackNumber.m_clip.lr.x = 319;
    m_lblTrackNumber.m_clip.ul.y = iTop;
    m_lblTrackNumber.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTrackNumber.m_start.x = m_lblTrackNumber.m_clip.ul.x;
    m_lblTrackNumber.m_start.y = m_lblTrackNumber.m_clip.ul.y;
    m_lblTrackNumber.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTrackNumber);

    iTop += sc_iPlayStateHeight + sc_iCharSpacing;

    // Title
    m_lblTitleLabel.SetFont(&Verdana12Font);
    m_lblTitleLabel.SetText("Title");
    m_lblTitleLabel.m_clip.ul.x = iLeftMargin;
    m_lblTitleLabel.m_clip.lr.x = 105;
    m_lblTitleLabel.m_clip.ul.y = iTop;
    m_lblTitleLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTitleLabel.m_start.x = m_lblTitleLabel.m_clip.ul.x;
    m_lblTitleLabel.m_start.y = m_lblTitleLabel.m_clip.ul.y;
    m_lblTitleLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTitleLabel);

    m_lblTitleValue.SetFont(&Verdana12Font);
    m_lblTitleValue.SetText("");
    m_lblTitleValue.m_clip.ul.x = 105;
    m_lblTitleValue.m_clip.lr.x = 319;
    m_lblTitleValue.m_clip.ul.y = iTop;
    m_lblTitleValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTitleValue.m_start.x = m_lblTitleValue.m_clip.ul.x;
    m_lblTitleValue.m_start.y = m_lblTitleValue.m_clip.ul.y;
    m_lblTitleValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTitleValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Artist
    m_lblArtistLabel.SetFont(&Verdana12Font);
    m_lblArtistLabel.SetText("Artist");
    m_lblArtistLabel.m_clip.ul.x = iLeftMargin;
    m_lblArtistLabel.m_clip.lr.x = 105;
    m_lblArtistLabel.m_clip.ul.y = iTop;
    m_lblArtistLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblArtistLabel.m_start.x = m_lblArtistLabel.m_clip.ul.x;
    m_lblArtistLabel.m_start.y = m_lblArtistLabel.m_clip.ul.y;
    m_lblArtistLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblArtistLabel);

    m_lblArtistValue.SetFont(&Verdana12Font);
    m_lblArtistValue.SetText("");
    m_lblArtistValue.m_clip.ul.x = 105;
    m_lblArtistValue.m_clip.lr.x = 319;
    m_lblArtistValue.m_clip.ul.y = iTop;
    m_lblArtistValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblArtistValue.m_start.x = m_lblArtistValue.m_clip.ul.x;
    m_lblArtistValue.m_start.y = m_lblArtistValue.m_clip.ul.y;
    m_lblArtistValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblArtistValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Album
    m_lblAlbumLabel.SetFont(&Verdana12Font);
    m_lblAlbumLabel.SetText("Album");
    m_lblAlbumLabel.m_clip.ul.x = iLeftMargin;
    m_lblAlbumLabel.m_clip.lr.x = 105;
    m_lblAlbumLabel.m_clip.ul.y = iTop;
    m_lblAlbumLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblAlbumLabel.m_start.x = m_lblAlbumLabel.m_clip.ul.x;
    m_lblAlbumLabel.m_start.y = m_lblAlbumLabel.m_clip.ul.y;
    m_lblAlbumLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblAlbumLabel);

    m_lblAlbumValue.SetFont(&Verdana12Font);
    m_lblAlbumValue.SetText("");
    m_lblAlbumValue.m_clip.ul.x = 105;
    m_lblAlbumValue.m_clip.lr.x = 319;
    m_lblAlbumValue.m_clip.ul.y = iTop;
    m_lblAlbumValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblAlbumValue.m_start.x = m_lblAlbumValue.m_clip.ul.x;
    m_lblAlbumValue.m_start.y = m_lblAlbumValue.m_clip.ul.y;
    m_lblAlbumValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblAlbumValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Genre
    m_lblGenreLabel.SetFont(&Verdana12Font);
    m_lblGenreLabel.SetText("Genre");
    m_lblGenreLabel.m_clip.ul.x = iLeftMargin;
    m_lblGenreLabel.m_clip.lr.x = 105;
    m_lblGenreLabel.m_clip.ul.y = iTop;
    m_lblGenreLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblGenreLabel.m_start.x = m_lblGenreLabel.m_clip.ul.x;
    m_lblGenreLabel.m_start.y = m_lblGenreLabel.m_clip.ul.y;
    m_lblGenreLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblGenreLabel);

    m_lblGenreValue.SetFont(&Verdana12Font);
    m_lblGenreValue.SetText("");
    m_lblGenreValue.m_clip.ul.x = 105;
    m_lblGenreValue.m_clip.lr.x = 319;
    m_lblGenreValue.m_clip.ul.y = iTop;
    m_lblGenreValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblGenreValue.m_start.x = m_lblGenreValue.m_clip.ul.x;
    m_lblGenreValue.m_start.y = m_lblGenreValue.m_clip.ul.y;
    m_lblGenreValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblGenreValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Playlist mode
    m_lblPlaylistModeLabel.SetFont(&Verdana12Font);
    m_lblPlaylistModeLabel.SetText("Playlist mode");
    m_lblPlaylistModeLabel.m_clip.ul.x = iLeftMargin;
    m_lblPlaylistModeLabel.m_clip.lr.x = 105;
    m_lblPlaylistModeLabel.m_clip.ul.y = iTop;
    m_lblPlaylistModeLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblPlaylistModeLabel.m_start.x = m_lblPlaylistModeLabel.m_clip.ul.x;
    m_lblPlaylistModeLabel.m_start.y = m_lblPlaylistModeLabel.m_clip.ul.y;
    m_lblPlaylistModeLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblPlaylistModeLabel);

    m_lblPlaylistModeValue.SetFont(&Verdana12Font);
    m_lblPlaylistModeValue.SetText("");
    m_lblPlaylistModeValue.m_clip.ul.x = 105;
    m_lblPlaylistModeValue.m_clip.lr.x = 319;
    m_lblPlaylistModeValue.m_clip.ul.y = iTop;
    m_lblPlaylistModeValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblPlaylistModeValue.m_start.x = m_lblPlaylistModeValue.m_clip.ul.x;
    m_lblPlaylistModeValue.m_start.y = m_lblPlaylistModeValue.m_clip.ul.y;
    m_lblPlaylistModeValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblPlaylistModeValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Volume
    m_lblVolumeLabel.SetFont(&Verdana12Font);
    m_lblVolumeLabel.SetText("Volume");
    m_lblVolumeLabel.m_clip.ul.x = iLeftMargin;
    m_lblVolumeLabel.m_clip.lr.x = 105;
    m_lblVolumeLabel.m_clip.ul.y = iTop;
    m_lblVolumeLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblVolumeLabel.m_start.x = m_lblVolumeLabel.m_clip.ul.x;
    m_lblVolumeLabel.m_start.y = m_lblVolumeLabel.m_clip.ul.y;
    m_lblVolumeLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblVolumeLabel);

    m_lblVolumeValue.SetFont(&Verdana12Font);
    m_lblVolumeValue.SetText("");
    m_lblVolumeValue.m_clip.ul.x = 105;
    m_lblVolumeValue.m_clip.lr.x = 319;
    m_lblVolumeValue.m_clip.ul.y = iTop;
    m_lblVolumeValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblVolumeValue.m_start.x = m_lblVolumeValue.m_clip.ul.x;
    m_lblVolumeValue.m_start.y = m_lblVolumeValue.m_clip.ul.y;
    m_lblVolumeValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblVolumeValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Bass
    m_lblBassLabel.SetFont(&Verdana12Font);
    m_lblBassLabel.SetText("Bass");
    m_lblBassLabel.m_clip.ul.x = iLeftMargin;
    m_lblBassLabel.m_clip.lr.x = 105;
    m_lblBassLabel.m_clip.ul.y = iTop;
    m_lblBassLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblBassLabel.m_start.x = m_lblBassLabel.m_clip.ul.x;
    m_lblBassLabel.m_start.y = m_lblBassLabel.m_clip.ul.y;
    m_lblBassLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblBassLabel);

    m_lblBassValue.SetFont(&Verdana12Font);
    m_lblBassValue.SetText("");
    m_lblBassValue.m_clip.ul.x = 105;
    m_lblBassValue.m_clip.lr.x = 319;
    m_lblBassValue.m_clip.ul.y = iTop;
    m_lblBassValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblBassValue.m_start.x = m_lblBassValue.m_clip.ul.x;
    m_lblBassValue.m_start.y = m_lblBassValue.m_clip.ul.y;
    m_lblBassValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblBassValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Treble
    m_lblTrebleLabel.SetFont(&Verdana12Font);
    m_lblTrebleLabel.SetText("Treble");
    m_lblTrebleLabel.m_clip.ul.x = iLeftMargin;
    m_lblTrebleLabel.m_clip.lr.x = 105;
    m_lblTrebleLabel.m_clip.ul.y = iTop;
    m_lblTrebleLabel.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTrebleLabel.m_start.x = m_lblTrebleLabel.m_clip.ul.x;
    m_lblTrebleLabel.m_start.y = m_lblTrebleLabel.m_clip.ul.y;
    m_lblTrebleLabel.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTrebleLabel);

    m_lblTrebleValue.SetFont(&Verdana12Font);
    m_lblTrebleValue.SetText("");
    m_lblTrebleValue.m_clip.ul.x = 105;
    m_lblTrebleValue.m_clip.lr.x = 319;
    m_lblTrebleValue.m_clip.ul.y = iTop;
    m_lblTrebleValue.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblTrebleValue.m_start.x = m_lblTrebleValue.m_clip.ul.x;
    m_lblTrebleValue.m_start.y = m_lblTrebleValue.m_clip.ul.y;
    m_lblTrebleValue.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblTrebleValue);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    // Message
    m_lblMessage.SetFont(&Verdana12Font);
    m_lblMessage.SetText("");
    m_lblMessage.m_clip.ul.x = iLeftMargin;
    m_lblMessage.m_clip.lr.x = 319;
    m_lblMessage.m_clip.ul.y = iTop;
    m_lblMessage.m_clip.lr.y = iTop + sc_iCharHeight;
    m_lblMessage.m_start.x = m_lblMessage.m_clip.ul.x;
    m_lblMessage.m_start.y = m_lblMessage.m_clip.ul.y;
    m_lblMessage.SetVisible(GUI_TRUE);
    m_screen.AddScreenElem(&m_lblMessage);
    iTop += sc_iCharHeight + sc_iCharSpacing;

    m_screen.SetVisible(GUI_TRUE);

    m_show.Clear();
    m_show.Draw();
}

CSimpleGUIUserInterface::~CSimpleGUIUserInterface() 
{}

void CSimpleGUIUserInterface::SetBanner(const char* szBanner) 
{
    m_lblBanner.SetText(szBanner);
    m_show.Draw();
}

void CSimpleGUIUserInterface::ShowDirections() 
{
}

void CSimpleGUIUserInterface::NotifyPlaying() 
{
    m_bmpPlay.SetVisible(GUI_TRUE);
    m_bmpPause.SetVisible(GUI_FALSE);
    m_bmpStop.SetVisible(GUI_FALSE);
    m_show.Draw();
}

void CSimpleGUIUserInterface::NotifyPaused() 
{
    m_bmpPlay.SetVisible(GUI_FALSE);
    m_bmpPause.SetVisible(GUI_TRUE);
    m_bmpStop.SetVisible(GUI_FALSE);
    m_show.Draw();
}

void CSimpleGUIUserInterface::NotifyStopped() 
{
    m_bmpPlay.SetVisible(GUI_FALSE);
    m_bmpPause.SetVisible(GUI_FALSE);
    m_bmpStop.SetVisible(GUI_TRUE);

    m_lblTrackTime.SetText(" 0:00");

    m_show.Draw();
}

void CSimpleGUIUserInterface::NotifyNextTrack() 
{
}

void CSimpleGUIUserInterface::NotifyPreviousTrack() 
{
}

void CSimpleGUIUserInterface::NotifySeekFwd() 
{
}

void CSimpleGUIUserInterface::NotifySeekRew() 
{
}

void CSimpleGUIUserInterface::NotifyMediaInserted(int iMediaIndex) 
{
    SetMessage("Media %d inserted", iMediaIndex);
}

void CSimpleGUIUserInterface::NotifyMediaRemoved(int iMediaIndex) 
{
    SetMessage("Media %d removed", iMediaIndex);
}

void CSimpleGUIUserInterface::NotifyStreamEnd() 
{
    SetMessage("Stream end");
}

void CSimpleGUIUserInterface::SetTrack(set_stream_event_data_t* pSSED, int iTrackCount)
{
    IMediaContentRecord* pContentRecord = pSSED->pPlaylistEntry->GetContentRecord();
    char szScratch[256];
    void* pValue;

    DEBUGP(SGUI, DBGLEV_INFO, "********************************************************\n");
    DEBUGP(SGUI, DBGLEV_INFO, "URL: %s\n", pContentRecord->GetURL());
    DEBUGP(SGUI, DBGLEV_INFO, "Sampling frequency: %d hz\n", pSSED->streamInfo.SamplingFrequency);
    DEBUGP(SGUI, DBGLEV_INFO, "Bitrate: %d bps\n", pSSED->streamInfo.Bitrate);
    DEBUGP(SGUI, DBGLEV_INFO, "Channels: %d\n", pSSED->streamInfo.Channels);
    DEBUGP(SGUI, DBGLEV_INFO, "Duration: %2d:%02d\n", pSSED->streamInfo.Duration / 60, pSSED->streamInfo.Duration % 60);
    DEBUGP(SGUI, DBGLEV_INFO, "********************************************************\n");

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_TITLE, &pValue))) ||
        SUCCEEDED(pContentRecord->GetAttribute(MDA_TITLE, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 256);
    else if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_FILE_NAME, &pValue))) ||
             SUCCEEDED(pContentRecord->GetAttribute(MDA_FILE_NAME, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 256);
    else
        strncpy(szScratch, pContentRecord->GetURL(), 256);
    m_lblTitleValue.SetText(szScratch);

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_ARTIST, &pValue))) ||
        SUCCEEDED(pContentRecord->GetAttribute(MDA_ARTIST, &pValue)))
    {
        TcharToCharN(szScratch, (TCHAR*)pValue, 256);
        m_lblArtistValue.SetText(szScratch);
    }
    else
        m_lblArtistValue.SetText("");

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_ALBUM, &pValue))) ||
        SUCCEEDED(pContentRecord->GetAttribute(MDA_ALBUM, &pValue)))
    {
        TcharToCharN(szScratch, (TCHAR*)pValue, 256);
        m_lblAlbumValue.SetText(szScratch);
    }
    else
        m_lblAlbumValue.SetText("");

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_GENRE, &pValue))) ||
        SUCCEEDED(pContentRecord->GetAttribute(MDA_GENRE, &pValue)))
    {
        TcharToCharN(szScratch, (TCHAR*)pValue, 256);
        m_lblGenreValue.SetText(szScratch);
    }
    else
        m_lblGenreValue.SetText("");

    if (iTrackCount)
        sprintf(szScratch, "Track %d of %d", pSSED->pPlaylistEntry->GetIndex() + 1, iTrackCount);
    else
        sprintf(szScratch, "Track %d", pSSED->pPlaylistEntry->GetIndex() + 1);

    m_lblTrackNumber.SetText(szScratch);

    m_lblTrackTime.SetText(" 0:00");

    m_show.Draw();
}

void CSimpleGUIUserInterface::ClearTrack()
{
    m_lblTitleValue.SetText("");
    m_lblArtistValue.SetText("");
    m_lblAlbumValue.SetText("");
    m_lblGenreValue.SetText("");

    m_lblTrackNumber.SetText("");
    m_lblTrackTime.SetText(" 0:00");

    m_show.Draw();
}


void CSimpleGUIUserInterface::SetTrackTime(int iSeconds) 
{
    // don't redraw unless we absolutely have to
    if( iSeconds != m_iCurrentTrackTime ) {
        char szScratch[7];
        sprintf(szScratch, "%2d:%02d", iSeconds / 60, iSeconds % 60);
        m_lblTrackTime.SetText(szScratch);
	m_show.Draw();
        m_iCurrentTrackTime = iSeconds;
    }
}

void CSimpleGUIUserInterface::SetPlaylistMode(IPlaylist::PlaylistMode mode)
{
    switch (mode)
    {
        case IPlaylist::NORMAL:
            m_lblPlaylistModeValue.SetText("Normal");
            break;

        case IPlaylist::REPEAT_ALL:
            m_lblPlaylistModeValue.SetText("Repeat all");
            break;

        case IPlaylist::RANDOM:
            m_lblPlaylistModeValue.SetText("Random");
            break;

        case IPlaylist::REPEAT_RANDOM:
            m_lblPlaylistModeValue.SetText("Repeat random");
            break;

        case IPlaylist::REPEAT_TRACK:
            m_lblPlaylistModeValue.SetText("Repeat track");
            break;
    }
    m_show.Draw();
}

void CSimpleGUIUserInterface::SetVolume(int iVolume)
{
    char szScratch[3];
    sprintf(szScratch, "%2d", iVolume);
    m_lblVolumeValue.SetText(szScratch);
    m_show.Draw();
}

void CSimpleGUIUserInterface::SetBass(int iBass)
{
    char szScratch[3];
    sprintf(szScratch, "%2d", iBass);
    m_lblBassValue.SetText(szScratch);
    m_show.Draw();
}

void CSimpleGUIUserInterface::SetTreble(int iTreble)
{
    char szScratch[3];
    sprintf(szScratch, "%2d", iTreble);
    m_lblTrebleValue.SetText(szScratch);
    m_show.Draw();
}

void CSimpleGUIUserInterface::SetMessage(const char* szFormat, ...) 
{
    char szBuf[256];
    va_list va;
    va_start(va, szFormat);
    vsnprintf(szBuf, 256, szFormat, va);
    va_end(va);

    m_lblMessage.SetText(szBuf);
    m_show.Draw();
}
