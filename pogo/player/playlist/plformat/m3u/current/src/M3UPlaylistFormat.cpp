//........................................................................................
//........................................................................................
//.. File Name: M3UPlaylistFormat.cpp													..
//.. Date: 8/8/2001																	    ..
//.. Author(s): Edward Miller															..
//.. Description of content: m3u playlist format loading and saving functions           ..
//.. Usage: This is a part of the IObjects Direct Streaming Services Library			..
//.. Last Modified By: Ed Miller	edwardm@iobjects.com								..	
//.. Modification date: 8/8/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <content/common/ContentManager.h>
#include <core/playmanager/PlayManager.h>   // to get content manager
#include <datasource/common/DataSource.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/common/PlaylistFormat.h>
#include <util/debug/debug.h>
#include <util/tchar/tchar.h>
#include <_modules.h>

#include <stdio.h>  // sscanf, sprintf
#include <stdlib.h>  // malloc

#define M3U_PLAYLIST_FORMAT_ID  0x62

DEBUG_MODULE_S( M3UD, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( M3UD );

static const char* sc_M3UHeader = "#EXTM3U";
static const char* sc_M3UInfoPrefix = "#EXTINF:";

//#define GET_M3U_METADATA

// Gets the next line from the file and puts it into the line buffer.
// Returns false on EOF.
static bool GetLine(IInputStream* pInputStream,
    char* szFileBuffer, int iMaxFileBytes,
    int& iCurFileBufPos, int& iFileBufferBytes,
    char* szLineBuffer, int iMaxLineBytes)
{
    char* pchLineBuffer = szLineBuffer;
    do
    {
        if (iCurFileBufPos >= iFileBufferBytes)
        {
            iCurFileBufPos = 0;
            if ((iFileBufferBytes = pInputStream->Read(szFileBuffer, iMaxFileBytes)) <= 0)
            {
                *pchLineBuffer = '\0';
                return (pchLineBuffer != szLineBuffer);
            }
        }
        while ((iCurFileBufPos < iFileBufferBytes) && (szFileBuffer[iCurFileBufPos] != '\n'))
        {
            if ((*pchLineBuffer++ = szFileBuffer[iCurFileBufPos++]) == '\r')
                --pchLineBuffer;
            // If the line is longer than the buffer then assume this is a bad file.
            if (pchLineBuffer - szLineBuffer >= iMaxLineBytes)
                return false;
        }
    } while (szFileBuffer[iCurFileBufPos] != '\n');

    *pchLineBuffer = '\0';
    ++iCurFileBufPos;

    DEBUGP(M3UD, DBGLEV_TRACE, "m3u: %s\n", szLineBuffer);

    return true;
}

static void ConvertSeparators(char* szBuffer)
{
    char* pch = szBuffer;
    while (pch = strchr(szBuffer, '\\'))
        *pch = '/';
}

ERESULT
LoadM3UPlaylist( IMediaRecordInfoVector& records, const char* szURL, int iMaxTracks )
{
    IInputStream* pInputStream = CDataSourceManager::GetInstance()->OpenInputStream(szURL);
    if (!pInputStream)
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;

    IDataSource* pDataSource = CDataSourceManager::GetInstance()->GetDataSourceByURL(szURL);
    if (!pDataSource)
    {
        delete pInputStream;
        return PLAYLIST_FORMAT_BAD_URL;
    }
    int iDataSourceID = pDataSource->GetInstanceID();

static const int BUFFER_SIZE = 1024;
    char szFileBuffer[BUFFER_SIZE];
    char LineBuffer[BUFFER_SIZE];
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    MediaRecordList mrlTracks;

    int iCurFileBufPos = 0, iFileBufferBytes = 0;

    // Construct a prefix string for files from this data source.
#define METADATA_STRING_SIZE 256
    char DSLocalPrefix[METADATA_STRING_SIZE];
    strcpy(DSLocalPrefix, szURL);
    char* pch;
    ConvertSeparators(DSLocalPrefix);
//   if (pch = strrchr(DSLocalPrefix, '\\'))
    if( (pch = strrchr(DSLocalPrefix, '/')) )
        *++pch = '\0';
    char DSAbsolutePrefix[METADATA_STRING_SIZE];
    strcpy(DSAbsolutePrefix, szURL);
    ConvertSeparators(DSAbsolutePrefix);
//    if (pch = strchr(DSAbsolutePrefix, '\\'))
    if( (pch = strstr(DSAbsolutePrefix, "//")) )
        if( (pch = strchr(pch + 2, '/')) )
            *++pch = '\0';

    records.SetBlockSize(500);
    int iTracksRead = 0;

    while ((GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE)) > 0)
    {
        media_record_info_t mediaContent;
        mediaContent.iCodecID = 0;
        mediaContent.iDataSourceID = iDataSourceID;
        mediaContent.pMetadata = 0;
        // Is this an info line?
        if (LineBuffer[0] == '#')
        {
#ifdef GET_M3U_METADATA
            // Check for the info line.
            if (!strncmp(LineBuffer, sc_M3UInfoPrefix, strlen(sc_M3UInfoPrefix)))
            {
                if (!mediaContent.pMetadata)
                    mediaContent.pMetadata = pCM->CreateMetadataRecord();
                int iSeconds;
                char szTitle[BUFFER_SIZE];
                if (sscanf(LineBuffer, "#EXTINF:%d,%[^\0]", &iSeconds, szTitle) == 2)
                {
                    DEBUGP(M3UD, DBGLEV_INFO, "m3u: Title: %s Length: %d:%02d\n", szTitle, iSeconds / 60, iSeconds % 60);
                    TCHAR tszTitle[METADATA_STRING_SIZE];
                    mediaContent.pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszTitle, szTitle, METADATA_STRING_SIZE));
                }
            }
#endif  // GET_M3U_METADATA
            continue;
        }

        if (LineBuffer[0] == '\\')
        {
            // Absolute path
            mediaContent.szURL = (char*)malloc(strlen(DSAbsolutePrefix) + strlen(LineBuffer));
            strcpy(mediaContent.szURL , DSAbsolutePrefix);
            ConvertSeparators(LineBuffer + 1);
            strcat(mediaContent.szURL , LineBuffer + 1);
        }
        else if (IDataSource* pDS = CDataSourceManager::GetInstance()->GetDataSourceByURL(LineBuffer))
        {
            mediaContent.szURL = (char*)malloc(strlen(LineBuffer) + 1);
            strcpy(mediaContent.szURL, LineBuffer);
            mediaContent.iDataSourceID = pDS->GetInstanceID();
        }
        else
        {
            // Local path
            mediaContent.szURL = (char*)malloc(strlen(DSLocalPrefix) + strlen(LineBuffer) + 1);
            strcpy(mediaContent.szURL, DSLocalPrefix);
            ConvertSeparators(LineBuffer);
            strcat(mediaContent.szURL, LineBuffer);
        }

        DEBUGP(M3UD, DBGLEV_INFO, "m3u: URL: %s\n", mediaContent.szURL);

        mediaContent.bVerified = false;

        records.PushBack(mediaContent);

        // Stop loading if we've reached the maximum number of tracks to load.
        if (iMaxTracks && (++iTracksRead == iMaxTracks))
            break;
    }

    delete pInputStream;

    return PLAYLIST_FORMAT_NO_ERROR;
}

ERESULT
WriteTitleAndDuration(IMediaContentRecord* pRecord, IOutputStream* pOutputStream)
{
    char szFileBuffer[512];

    // Get the title, first by trying the title, then the filename, then the URL.
    TCHAR* tszTitle = 0;
    char szTitle[256];
    if (SUCCEEDED(pRecord->GetAttribute(MDA_TITLE, (void**)&szTitle)))
        TcharToCharN(szTitle, tszTitle, 256);
    else if (SUCCEEDED(pRecord->GetAttribute(MDA_FILE_NAME, (void**)&szTitle)))
        TcharToCharN(szTitle, tszTitle, 256);
    else
        strncpy(szTitle, pRecord->GetURL(), 256);

    int iDuration = 0;
    pRecord->GetAttribute(MDA_DURATION, (void**)iDuration);

    sprintf(szFileBuffer, "%s %d,%s\n", sc_M3UInfoPrefix, iDuration, szTitle);
    int iToWrite = strlen(szFileBuffer);
    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
        return PLAYLIST_FORMAT_WRITE_ERROR;
    else
        return PLAYLIST_FORMAT_NO_ERROR;
}

ERESULT
SaveM3UPlaylist( const char* szURL, IPlaylist* pPlaylist )
{
    IOutputStream* pOutputStream = CDataSourceManager::GetInstance()->OpenOutputStream(szURL);
    if (!pOutputStream)
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;

    IDataSource* pOutputDS = CDataSourceManager::GetInstance()->GetDataSourceByURL(szURL);
    if (!pOutputDS)
    {
        delete pOutputStream;
        return PLAYLIST_FORMAT_BAD_URL;
    }
    int iOutputDSID = pOutputDS->GetInstanceID();
    char szOutputDSPrefix[64];
    pOutputDS->GetRootURLPrefix(szOutputDSPrefix, 64);
    int iOutputDSLen = strlen(szOutputDSPrefix);

    char szFileBuffer[512];
    sprintf(szFileBuffer, "%s\n", sc_M3UHeader);
    int iToWrite = strlen(szFileBuffer);
    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
        return PLAYLIST_FORMAT_WRITE_ERROR;
    for (int i = 0; i < pPlaylist->GetSize(); ++i)
    {
        if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(i))
        {
            if (IMediaContentRecord* pRecord = pEntry->GetContentRecord())
            {
                if (pRecord->GetDataSourceID() == iOutputDSID)
                {
                    ERESULT eres = WriteTitleAndDuration(pRecord, pOutputStream);
                    if (FAILED(eres))
                    {
                        delete pOutputStream;
                        return eres;
                    }

                    // Cut off the URL prefix since this record comes from the same data source.
                    sprintf(szFileBuffer, "%s\n", pRecord->GetURL() + iOutputDSLen);
                    iToWrite = strlen(szFileBuffer);
                    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
                    {
                        delete pOutputStream;
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                    }
                }
                else
                {
                    ERESULT eres = WriteTitleAndDuration(pRecord, pOutputStream);
                    if (FAILED(eres))
                    {
                        delete pOutputStream;
                        return eres;
                    }

                    // This record comes from a different data source than what we're writing to,
                    // so use the whole URL.
                    sprintf(szFileBuffer, "%s\n", pRecord->GetURL());
                    iToWrite = strlen(szFileBuffer);
                    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
                    {
                        delete pOutputStream;
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                    }
                }
            }
        }
    }
    delete pOutputStream;
    return PLAYLIST_FORMAT_NO_ERROR;
}

REGISTER_PLAYLIST_FORMAT( "Winamp M3U playlist format", M3U_PLAYLIST_FORMAT_ID, LoadM3UPlaylist, SaveM3UPlaylist, "m3u" );
