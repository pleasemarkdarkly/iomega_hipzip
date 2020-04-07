//........................................................................................
//........................................................................................
//.. File Name: DJPPlaylistFormat.cpp													..
//.. Date: 12/27/2001																	..
//.. Author(s): Edward Miller															..
//.. Description of content: interface for the CDJPPlaylistFormat class	 				..
//.. Usage: This is a part of the IObjects Direct Streaming Services Library			..
//.. Last Modified By: Ed Miller	edwardm@iobjects.com								..	
//.. Modification date: 12/27/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
//
// The DJ playlist format takes advantage of the fact that the DJ has one HD, one CD, and
// countless fpmls.
// The first number on a line specifies the data source where the file is located.
// 1 = CD, 2 = HD, 3 = fpml
// The second number specifies the codec of the file.
// 1 = PCM, 2 = MP3, 3 = WMA, 4 = WAV
//
// CD and HD entries are in the following format:
// [Data source ID] [Codec ID] [URL]
//
// fpml entries are as follows:
// 3 [Codec ID] [URL]
// [Title]
// [Artist]
//

#include <codec/codecmanager/CodecManager.h>
#include <content/common/ContentManager.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <main/content/simplercontentmanager/SimplerContentManager.h>
#include <main/main/DJPlayerState.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/common/PlaylistFormat.h>
#include <util/debug/debug.h>

#include <stdio.h>  // sscanf, sprintf
#include <stdlib.h> // malloc

#define DJP_PLAYLIST_FORMAT_ID  0x63

DEBUG_MODULE_S( DJPD, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( DJPD );


static const int FILE_BUFFER_SIZE = 65536 / 2;
static const int LINE_BUFFER_SIZE = 1024;
static const int METADATA_STRING_SIZE = 256;

static char s_buffer[FILE_BUFFER_SIZE];
static char* s_bufptr;

#define DJP_ENTRY_COUNT "#count:"

//#define PROFILE_TIME

#ifdef PROFILE_TIME
#include <cyg/kernel/kapi.h>
static cyg_tick_count_t s_tickContentScanStart;
static cyg_tick_count_t s_tickMetadataScanStart;
#endif  // PROFILE_TIME


static int GetLine(IInputStream* pInputStream,
    char* szFileBuffer, int iMaxFileBytes,
    int& iCurFileBufPos, int& iFileBufferBytes,
    char* szLineBuffer, int iMaxLineBytes)
{

    char* pchLineBuffer = szLineBuffer;
    do
    {
        if (iCurFileBufPos >= iFileBufferBytes)
        {
            DEBUGP(DJPD, DBGLEV_TRACE, "Buffer empty, reading %d bytes\n", iMaxFileBytes);
            iCurFileBufPos = 0;
            if ((iFileBufferBytes = pInputStream->Read(szFileBuffer, iMaxFileBytes)) <= 0)
            {
                DEBUGP(DJPD, DBGLEV_TRACE, "Error reading %d bytes\n", iMaxFileBytes);
                *pchLineBuffer = '\0';
                return pchLineBuffer - szLineBuffer;
            }
            DEBUGP(DJPD, DBGLEV_TRACE, "Read %d of %d bytes\n", iFileBufferBytes, iMaxFileBytes);
        }
        while ((iCurFileBufPos < iFileBufferBytes) && (szFileBuffer[iCurFileBufPos] != '\n'))
        {
            *pchLineBuffer++ = szFileBuffer[iCurFileBufPos++];
            // If the line is longer than the buffer then assume this is a bad file.
            if (pchLineBuffer - szLineBuffer >= iMaxLineBytes)
                return -1;
        }
    } while (szFileBuffer[iCurFileBufPos] != '\n');

    *pchLineBuffer = '\0';
    ++iCurFileBufPos;
    return pchLineBuffer - szLineBuffer;
}


ERESULT
LoadDJPPlaylist( IMediaRecordInfoVector& records, const char* szURL, int iMaxTracks )
{
    IInputStream* pInputStream = CDataSourceManager::GetInstance()->OpenInputStream(szURL);
    if (!pInputStream)
    {
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;
    }
    if (pInputStream->Length() == 0)
    {
        // That was quick.
        delete pInputStream;
        return PLAYLIST_FORMAT_NO_ERROR;
    }

    int iCDDSID = CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID();
    int iFatDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    int iNetDSID = CDJPlayerState::GetInstance()->GetNetDataSource()->GetInstanceID();

    RegKey pcmCodecID = CCodecManager::GetInstance()->FindCodecID("cda");
    RegKey mp3CodecID = CCodecManager::GetInstance()->FindCodecID("mp3");
    RegKey wmaCodecID = CCodecManager::GetInstance()->FindCodecID("wma");
    RegKey wavCodecID = CCodecManager::GetInstance()->FindCodecID("wav");

    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();

    char LineBuffer[LINE_BUFFER_SIZE];
    int iCurFileBufPos = 0, iFileBufferBytes = 0;

#ifdef PROFILE_TIME
    cyg_tick_count_t tickStart = cyg_current_time();
#endif  // PROFILE_TIME

    int iLineSize;
    int iEntryCount = 0;

    // Get the entry count.
    iLineSize = GetLine(pInputStream, s_buffer, FILE_BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, LINE_BUFFER_SIZE);
    if (iLineSize > 0)
    {
        if (!strncmp(LineBuffer, DJP_ENTRY_COUNT, strlen(DJP_ENTRY_COUNT)))
        {
            iEntryCount = atoi(&LineBuffer[strlen(DJP_ENTRY_COUNT)]);
            iLineSize = GetLine(pInputStream, s_buffer, FILE_BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, LINE_BUFFER_SIZE);
        }
    }

    if (iEntryCount)
        if (iMaxTracks && (iMaxTracks < iEntryCount))
            records.SetBlockSize(iMaxTracks);
        else
            records.SetBlockSize(iEntryCount);
    else
        records.SetBlockSize(500);

    int iTracksRead = 0;
    while (iLineSize)
    {
        DEBUGP(DJPD, DBGLEV_TRACE, "Line: %s\n", LineBuffer);
        media_record_info_t mediaContent;
        mediaContent.pMetadata = 0;
        if ((LineBuffer[0] >= '1') && (LineBuffer[0] <= '3'))
        {
            // Check codec ID.
            if (LineBuffer[2] == '1')
                mediaContent.iCodecID = pcmCodecID;
            else if (LineBuffer[2] == '2')
                mediaContent.iCodecID = mp3CodecID;
            else if (LineBuffer[2] == '3')
                mediaContent.iCodecID = wmaCodecID;
            else if (LineBuffer[2] == '4')
                mediaContent.iCodecID = wavCodecID;
            else
            {
                DEBUG(DJPD, DBGLEV_WARNING, "Unknown codec: %c (Line: %s)\n", LineBuffer[2], LineBuffer);
                goto NextLine;
            }

            // Check datasource ID.
            if (LineBuffer[0] == '1')
                mediaContent.iDataSourceID = iCDDSID;
            else if (LineBuffer[0] == '2')
                mediaContent.iDataSourceID = iFatDSID;
            else if (LineBuffer[0] == '3')
            {
                mediaContent.iDataSourceID = iNetDSID;
                mediaContent.pMetadata = CSimplerContentManager::GetInstance()->CreateMetadataRecord();

                // The second line is the title.
                TCHAR tszScratch[METADATA_STRING_SIZE];
                char MDBuffer[LINE_BUFFER_SIZE];
                if ((iLineSize = GetLine(pInputStream, s_buffer, FILE_BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, MDBuffer, LINE_BUFFER_SIZE)) <= 0)
                {
                    DEBUG(DJPD, DBGLEV_ERROR, "Unable to read entry for URL: %s (Line: %s)\n", LineBuffer, MDBuffer);
                    goto NextLine;
                }
                DEBUGP(DJPD, DBGLEV_TRACE, "Line: %s\n", LineBuffer);
                mediaContent.pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszScratch, MDBuffer, METADATA_STRING_SIZE - 1));

                // The third line is the artist.
                if ((iLineSize = GetLine(pInputStream, s_buffer, FILE_BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, MDBuffer, LINE_BUFFER_SIZE)) <= 0)
                {
                    DEBUG(DJPD, DBGLEV_ERROR, "Unable to read entry for URL: %s (Line: %s)\n", LineBuffer, MDBuffer);
                    goto NextLine;
                }
                DEBUGP(DJPD, DBGLEV_TRACE, "Line: %s\n", LineBuffer);
                mediaContent.pMetadata->SetAttribute(MDA_ARTIST, (void*)CharToTcharN(tszScratch, MDBuffer, METADATA_STRING_SIZE - 1));
            }
            else
            {
                DEBUG(DJPD, DBGLEV_WARNING, "Unknown data source: %c (Line: %s)\n", LineBuffer[0], LineBuffer);
                goto NextLine;
            }
        }
        else
        {
            DEBUG(DJPD, DBGLEV_WARNING, "Unknown entry type: %s\n", LineBuffer);
            goto NextLine;
        }

        mediaContent.bVerified = false;
        mediaContent.szURL = (char*)malloc(strlen(&LineBuffer[4]) + 1);
        strcpy(mediaContent.szURL, &LineBuffer[4]);

        records.PushBack(mediaContent);

        // Stop loading if we've reached the maximum number of tracks to load.
        if (iMaxTracks && (++iTracksRead == iMaxTracks))
            break;

NextLine:
        iLineSize = GetLine(pInputStream, s_buffer, FILE_BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, LINE_BUFFER_SIZE);
    }

    delete pInputStream;

#ifdef PROFILE_TIME
    cyg_tick_count_t tickEnd = cyg_current_time();
    diag_printf("%s: Tick count: %d\n", __FUNCTION__, (int)(tickEnd - tickStart));
#endif  // PROFILE_TIME

    return PLAYLIST_FORMAT_NO_ERROR;
}


// Write out any remaining data in the buffer to the file.
static ERESULT
FlushWriteBuffer(IOutputStream* pOutputStream)
{
    if (pOutputStream->Write((const void*)s_buffer, s_bufptr - s_buffer) != s_bufptr - s_buffer)
    {
        DEBUG(DJPD, DBGLEV_WARNING, "Error in writing %d bytes to playlist file\n", s_bufptr - s_buffer);
        delete pOutputStream;
        return PLAYLIST_FORMAT_WRITE_ERROR;
    }
    s_bufptr = s_buffer;
    return PLAYLIST_FORMAT_NO_ERROR;
}

// Writes a line of output to the buffer.  If this would cause a buffer overflow,
// then the buffer is first flushed to file.
static ERESULT
WriteLine(IOutputStream* pOutputStream, const char* szBuffer, int iToWrite)
{
    DEBUGP(DJPD, DBGLEV_TRACE, "Writing line: %s", szBuffer);
    if (FILE_BUFFER_SIZE - (s_bufptr - s_buffer) < iToWrite)
    {
        ERESULT err = FlushWriteBuffer(pOutputStream);
        if (FAILED(err))
            return err;
    }
    memcpy((void*)s_bufptr, (void*)szBuffer, iToWrite);
    s_bufptr += iToWrite;
    return PLAYLIST_FORMAT_NO_ERROR;
}

#include <main/playlist/djplaylist/DJPlaylist.h>

ERESULT
SaveDJPPlaylist( const char* szURL, IPlaylist* pPlaylist )
{
    CDJPlaylist* pDJPlaylist = (CDJPlaylist*)pPlaylist;

    int iCDDSID = CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID();
    int iFatDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    RegKey pcmCodecID = CCodecManager::GetInstance()->FindCodecID("cda");
    RegKey mp3CodecID = CCodecManager::GetInstance()->FindCodecID("mp3");
    RegKey wmaCodecID = CCodecManager::GetInstance()->FindCodecID("wma");
    RegKey wavCodecID = CCodecManager::GetInstance()->FindCodecID("wav");

    if (IOutputStream* pOutputStream = CDataSourceManager::GetInstance()->OpenOutputStream(szURL))
    {

#ifdef PROFILE_TIME
        cyg_tick_count_t tickStart = cyg_current_time();
#endif  // PROFILE_TIME

        // Reset the buffer pointer.
        s_bufptr = s_buffer;

        // Write the number of entries in the playlist.
        char szBuffer[LINE_BUFFER_SIZE];
        sprintf(szBuffer, "%s%d\n", DJP_ENTRY_COUNT, pPlaylist->GetSize());
        if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
            return PLAYLIST_FORMAT_WRITE_ERROR;

        const CDJPlaylist::PlaylistEntryList* playlist = pDJPlaylist->GetEntryList();

        for (CDJPlaylist::PlaylistIterator it = playlist->GetHead(); it != playlist->GetEnd(); ++it)
        {
            if (IMediaContentRecord* pRecord = (*it)->GetContentRecord())
            {
                char chCodec = pRecord->GetCodecID() == mp3CodecID ? '2' :
                            pRecord->GetCodecID() == wmaCodecID ? '3' :
                            pRecord->GetCodecID() == wavCodecID ? '4' :
                            '1';
                if (pRecord->GetDataSourceID() == iCDDSID)
                {
                    sprintf(szBuffer, "1 %c %s\n",
                        chCodec,
                        pRecord->GetURL());
                    if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                }
                else if (pRecord->GetDataSourceID() == iFatDSID)
                {
                    sprintf(szBuffer, "2 %c %s\n",
                        chCodec,
                        pRecord->GetURL());
                    if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                }
                else
                {
                    TCHAR* pch;
                    char szTitle[METADATA_STRING_SIZE], szArtist[METADATA_STRING_SIZE];
                    if (SUCCEEDED(pRecord->GetAttribute(MDA_TITLE, (void**)&pch)))
                    {
                        TcharToCharN(szTitle, pch, METADATA_STRING_SIZE - 1);
                        if (SUCCEEDED(pRecord->GetAttribute(MDA_ARTIST, (void**)&pch)))
                        {
                            sprintf(szBuffer, "3 %c %s\n",
                                chCodec,
                                pRecord->GetURL());
                            if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
                                return PLAYLIST_FORMAT_WRITE_ERROR;
                            sprintf(szBuffer, "%s\n", szTitle);
                            if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
                                return PLAYLIST_FORMAT_WRITE_ERROR;
                            sprintf(szBuffer, "%s\n", TcharToCharN(szArtist, pch, METADATA_STRING_SIZE - 1));
                            if (FAILED(WriteLine(pOutputStream, szBuffer, strlen(szBuffer))))
                                return PLAYLIST_FORMAT_WRITE_ERROR;
                        }
                        else
                        {
                            DEBUG(DJPD, DBGLEV_WARNING, "Unable to get artist for track %s\n", pRecord->GetURL());
                        }
                    }
                    else
                    {
                        DEBUG(DJPD, DBGLEV_WARNING, "Unable to get title for track %s\n", pRecord->GetURL());
                    }
                }
            }
        }

        // Write any remaining data to file.
        FlushWriteBuffer(pOutputStream);

#ifdef PROFILE_TIME
        cyg_tick_count_t tickEnd = cyg_current_time();
        diag_printf("%s: Tick count: %d Track count: %d Tracks per tick: %d\n", __FUNCTION__,
            (int)(tickEnd - tickStart),
            pPlaylist->GetSize(),
            pPlaylist->GetSize() / (int)(tickEnd - tickStart));
#endif  // PROFILE_TIME

        delete pOutputStream;
        return PLAYLIST_FORMAT_NO_ERROR;
    }
    else
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;

}

REGISTER_PLAYLIST_FORMAT( "DJ playlist format", DJP_PLAYLIST_FORMAT_ID, LoadDJPPlaylist, SaveDJPPlaylist, "djp" );
