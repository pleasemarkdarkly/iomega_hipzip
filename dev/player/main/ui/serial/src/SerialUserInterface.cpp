//
// SerialUserInterface.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/ui/serial/SerialUserInterface.h>
#include <playlist/common/Playlist.h>           // for IPlaylistEntry
#include <codec/common/Codec.h>                 // for set_stream_event_data_t
#include <cyg/infra/diag.h>                     // for diag_printf
#include <stdio.h>                              // for vsnprintf

static const char* szDefaultBanner = "iObjects Dadio SDK";

CSerialUserInterface::CSerialUserInterface() 
{
    m_szBanner = szDefaultBanner;
}

CSerialUserInterface::~CSerialUserInterface() 
{
}

void CSerialUserInterface::SetBanner(const char* szBanner) 
{
    m_szBanner = szBanner;
}

void CSerialUserInterface::ShowDirections() 
{
    diag_printf("+----------------------------------------+\n");
    diag_printf("| %30s |\n", m_szBanner);
    diag_printf("| %32s |\n", "");
    diag_printf("+----------------------------------------+\n");
    diag_printf("| Button usage:                          |\n");
    diag_printf("+----------------------------------------+\n");
}

void CSerialUserInterface::NotifyPlaying() 
{
    diag_printf("+-------------- Playing -----------------+\n");
}

void CSerialUserInterface::NotifyPaused() 
{
    diag_printf("+--------------- Paused -----------------+\n");
}

void CSerialUserInterface::NotifyStopped() 
{
    diag_printf("+-------------- Stopped -----------------+\n");
}

void CSerialUserInterface::NotifyNextTrack() 
{
    diag_printf("+------------- Next Track ---------------+\n");
}

void CSerialUserInterface::NotifyPreviousTrack() 
{
    diag_printf("+------------- Prev Track ---------------+\n");
}

void CSerialUserInterface::NotifySeekFwd() 
{
    diag_printf("+-------------- Seek Fwd ----------------+\n");
}

void CSerialUserInterface::NotifySeekRew() 
{
    diag_printf("+-------------- Seek Rew ----------------+\n");
}

void CSerialUserInterface::NotifyMediaInserted(int iMediaIndex) 
{
    diag_printf("+--------- Media %d inserted ------------+\n", iMediaIndex);
}

void CSerialUserInterface::NotifyMediaRemoved(int iMediaIndex) 
{
    diag_printf("+---------- Media %d removed ------------+\n", iMediaIndex);
}

void CSerialUserInterface::NotifyStreamEnd() 
{
    diag_printf("+------------ Stream End ----------------+\n");
}

void CSerialUserInterface::SetTrack(set_stream_event_data_t* pSSED, int iTrackCount)
{
    IMediaContentRecord* pContentRecord = pSSED->pPlaylistEntry->GetContentRecord();
    void* pValue;

    diag_printf("+----------------------------------------+\n");
    if (iTrackCount)
        diag_printf("+ Track Index: %3d of %3d                |\n", pSSED->pPlaylistEntry->GetIndex() + 1, iTrackCount);
    else
        diag_printf("+ Track Index: %3d                       |\n", pSSED->pPlaylistEntry->GetIndex() + 1);
    diag_printf("+ URL:    %30s |\n", pContentRecord->GetURL());

    char szScratch[31];
    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_TITLE, &pValue))) ||
        SUCCEEDED(pContentRecord->GetAttribute(MDA_TITLE, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 30);
    else if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_FILE_NAME, &pValue))) ||
             SUCCEEDED(pContentRecord->GetAttribute(MDA_FILE_NAME, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 30);
    else
        strncpy(szScratch, pContentRecord->GetURL(), 30);
    diag_printf("+ Title:  %30s |\n", szScratch);

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_ARTIST, &pValue))) ||
            SUCCEEDED(pContentRecord->GetAttribute(MDA_ARTIST, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 30);
    else
        szScratch[0] = '\0';
    diag_printf("+ Artist: %30s |\n", szScratch);

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_ALBUM, &pValue))) ||
            SUCCEEDED(pContentRecord->GetAttribute(MDA_ALBUM, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 30);
    else
        szScratch[0] = '\0';
    diag_printf("+ Album:  %30s |\n", szScratch);

    if ((pSSED->pMediaPlayerMetadata && SUCCEEDED(pSSED->pMediaPlayerMetadata->GetAttribute(MDA_GENRE, &pValue))) ||
            SUCCEEDED(pContentRecord->GetAttribute(MDA_GENRE, &pValue)))
        TcharToCharN(szScratch, (TCHAR*)pValue, 30);
    else
        szScratch[0] = '\0';
    diag_printf("+ Genre:  %30s |\n", szScratch);

    diag_printf("+----------------------------------------+\n");
}

void CSerialUserInterface::SetTrackTime(int iSeconds) 
{
    //    if((iSeconds % 5) == 0) {
    //        diag_printf("+----------- Track Time: %3d ------------+\n", iSeconds);
    //    }
}

void CSerialUserInterface::SetPlaylistMode(IPlaylist::PlaylistMode mode)
{
    switch (mode)
    {
        case IPlaylist::NORMAL:
            diag_printf("+-------- Playlist mode: normal ---------+\n");
            break;

        case IPlaylist::REPEAT_ALL:
            diag_printf("+-------- Playlist mode: repeat all -----+\n");
            break;

        case IPlaylist::RANDOM:
            diag_printf("+-------- Playlist mode: random ---------+\n");
            break;

        case IPlaylist::REPEAT_RANDOM:
            diag_printf("+-------- Playlist mode: repeat random --+\n");
            break;

        case IPlaylist::REPEAT_TRACK:
            diag_printf("+-------- Playlist mode: repeat track ---+\n");
            break;
    }
}

void CSerialUserInterface::SetVolume(int iVolume)
{
    diag_printf("+--------------- Volume: %2d -------------+\n", iVolume);
}

void CSerialUserInterface::SetBass(int iBass)
{
    diag_printf("+---------------- Bass: %2d --------------+\n", iBass);
}

void CSerialUserInterface::SetTreble(int iTreble)
{
    diag_printf("+--------------- Treble: %2d -------------+\n", iTreble);
}

// Set a one-line text message to be displayed to the user
void CSerialUserInterface::SetMessage(const char* szFormat, ...) 
{
    char szBuf[256];
    va_list va;
    va_start(va, szFormat);
    vsnprintf(szBuf, 256, szFormat, va);
    va_end(va);

    diag_printf("+------------- Message begin ------------+\n");
    diag_printf("%s\n", szBuf);
    diag_printf("+-------------- Message end -------------+\n");
}
