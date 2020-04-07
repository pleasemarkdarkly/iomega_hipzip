//
// File Name: PLSPlaylistFormat.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

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

#define PLS_PLAYLIST_FORMAT_ID  0x64

DEBUG_MODULE_S( PLSD, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( PLSD );

//
// The format of a pls file is as follows:
//
// [playlist]
// File1=filename
// Title1=title
// Length1=seconds
// ...
// FileX=filename
// TitleX=title
// LengthX=seconds
// NumberOfEntries=X
// Version=2
//
// TitleX and LengthX fields are optional.  A length of -1 is used for streams.
// Here's an example to boggle your mind:
//
// [playlist]
// File1=Classical\Beethoven\Fur Elise.mp3
// Title1=Fur Elise
// Length1=179
// File2=Nirvana\Bleach\07. Negative Creep.mp3
// Title2=Nirvana - Negative Creep
// Length2=175
// File3=http://192.168.0.50/temp/Sabotage.wma
// Title3=http://192.168.0.50/temp/Sabotage.wma
// Length3=-1
// File4=The Smiths\Louder Than Bombs\03. Shoplifters Of The World Unite.mp3
// File5=Tool\Undertow\03. Sober.mp3
// Title5=Tool - Sober
// Length5=306
// NumberOfEntries=705
// Version=2
//

static const char* sc_szPLSHeader = "[playlist]";

//#define GET_PLS_METADATA

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

    DEBUGP(PLSD, DBGLEV_TRACE, "pls: %s\n", szLineBuffer);

    return true;
}

static void ConvertSeparators(char* szBuffer)
{
    char* pch = szBuffer;
    while (pch = strchr(szBuffer, '\\'))
        *pch = '/';
}

ERESULT
LoadPLSPlaylist( IMediaRecordInfoVector& records, const char* szURL, int iMaxTracks )
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

    if (!GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE))
    {
        delete pInputStream;
        return PLAYLIST_FORMAT_BAD_FORMAT;
    }

    // Check the header to make sure this is a pls file.
    if (strcmp(LineBuffer, sc_szPLSHeader))
    {
        delete pInputStream;
        return PLAYLIST_FORMAT_BAD_FORMAT;
    }

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

    // Grab an entry.
    bool bNotEOF = GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE);
    while (bNotEOF)
    {
        // The first line in the entry is the filename.
        int dummy;
        char szFilename[BUFFER_SIZE];
        if (sscanf(LineBuffer, "File%d=%[^\0]", &dummy, szFilename) != 2)
        {
            // No match, so read another line and continue.
            bNotEOF = GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE);
            continue;
        }

        DEBUGP(PLSD, DBGLEV_INFO, "pls: File %d: %s\n", dummy, szFilename);

        // Scan for other metadata.
        // Stop when we encounter a line that we don't recognize.
        char szTitle[BUFFER_SIZE] = "";
        int iSeconds = 0;
        while ((bNotEOF = GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE)) > 0)
        {
            if (sscanf(LineBuffer, "Title%d=%[^\0]", &dummy, szTitle) == 2)
            {
                DEBUGP(PLSD, DBGLEV_INFO, "pls: Title %d: %s\n", dummy, szTitle);
                continue;
            }
            else if (sscanf(LineBuffer, "Length%d=%d", &dummy, &iSeconds) == 2)
            {
                DEBUGP(PLSD, DBGLEV_INFO, "pls: Length %d: %d seconds\n", dummy, iSeconds);
                continue;
            }
            else
                break;
        }

        // Create a media record and populate its metadata.
        media_record_info_t mediaContent;
        mediaContent.iCodecID = 0;
        mediaContent.iDataSourceID = iDataSourceID;
#ifdef GET_PLS_METADATA
        mediaContent.pMetadata = pCM->CreateMetadataRecord();

        if (szTitle[0] != '\0')
        {
            TCHAR tszTitle[METADATA_STRING_SIZE];
            mediaContent.pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszTitle, szTitle, METADATA_STRING_SIZE));
        }
        if (iSeconds > 0)
            mediaContent.pMetadata->SetAttribute(MDA_DURATION, (void*)iSeconds);
#else   // GET_PLS_METADATA
        mediaContent.pMetadata = 0;
#endif  // GET_PLS_METADATA

        if (szFilename[0] == '\\')
        {
            // Absolute path
            mediaContent.szURL = (char*)malloc(strlen(DSAbsolutePrefix) + strlen(szFilename));
            strcpy(mediaContent.szURL , DSAbsolutePrefix);
            ConvertSeparators(szFilename + 1);
            strcat(mediaContent.szURL , szFilename + 1);
        }
        else if (IDataSource* pDS = CDataSourceManager::GetInstance()->GetDataSourceByURL(szFilename))
        {
            mediaContent.szURL = (char*)malloc(strlen(szFilename) + 1);
            strcpy(mediaContent.szURL, szFilename);
            mediaContent.iDataSourceID = pDS->GetInstanceID();
        }
        else
        {
            // Local path
            mediaContent.szURL = (char*)malloc(strlen(DSLocalPrefix) + strlen(szFilename) + 1);
            strcpy(mediaContent.szURL, DSLocalPrefix);
            ConvertSeparators(szFilename);
            strcat(mediaContent.szURL, szFilename);
        }

        DEBUGP(PLSD, DBGLEV_TRACE, "pls: URL: %s\n", mediaContent.szURL);

        mediaContent.bVerified = false;

        records.PushBack(mediaContent);

        // Stop loading if we've reached the maximum number of tracks to load.
        if (iMaxTracks && (++iTracksRead == iMaxTracks))
            break;
    }

    delete pInputStream;

    return PLAYLIST_FORMAT_NO_ERROR;
}

static ERESULT
WriteTitleAndDuration(int index, IMediaContentRecord* pRecord, IOutputStream* pOutputStream)
{
    char szFileBuffer[METADATA_STRING_SIZE + 20];

    // Get the title, first by trying the title, then the filename, then the URL.
    TCHAR* tszTitle = 0;
    char szTitle[METADATA_STRING_SIZE];
    if (SUCCEEDED(pRecord->GetAttribute(MDA_TITLE, (void**)&tszTitle)))
    {
        TcharToCharN(szTitle, tszTitle, METADATA_STRING_SIZE);
        sprintf(szFileBuffer, "Title%d=%s\n", index, szTitle);
        int iToWrite = strlen(szFileBuffer);
        if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
            return PLAYLIST_FORMAT_WRITE_ERROR;
        else
            return PLAYLIST_FORMAT_NO_ERROR;
    }

    int iDuration = 0;
    if (SUCCEEDED(pRecord->GetAttribute(MDA_DURATION, (void**)iDuration)))
    {
        sprintf(szFileBuffer, "Length%d:%s\n", index, iDuration);
        int iToWrite = strlen(szFileBuffer);
        if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
            return PLAYLIST_FORMAT_WRITE_ERROR;
        else
            return PLAYLIST_FORMAT_NO_ERROR;
    }
    // dc- possibly incorrect
    return PLAYLIST_FORMAT_NO_ERROR;
}

ERESULT
SavePLSPlaylist( const char* szURL, IPlaylist* pPlaylist )
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
    sprintf(szFileBuffer, "%s\n", sc_szPLSHeader);
    int iToWrite = strlen(szFileBuffer);
    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
    {
        delete pOutputStream;
        return PLAYLIST_FORMAT_WRITE_ERROR;
    }
    int i = 0;
    for (; i < pPlaylist->GetSize(); ++i)
    {
        if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(i))
        {
            if (IMediaContentRecord* pRecord = pEntry->GetContentRecord())
            {
                if (pRecord->GetDataSourceID() == iOutputDSID)
                {
                    // Cut off the URL prefix since this record comes from the same data source.
                    sprintf(szFileBuffer, "File%d=%s\n", i + 1, pRecord->GetURL() + iOutputDSLen);
                    iToWrite = strlen(szFileBuffer);
                    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
                    {
                        delete pOutputStream;
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                    }
                }
                else
                {
                    // This record comes from a different data source than what we're writing to,
                    // so use the whole URL.
                    sprintf(szFileBuffer, "File%d=%s\n", i + 1, pRecord->GetURL());
                    iToWrite = strlen(szFileBuffer);
                    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
                    {
                        delete pOutputStream;
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                    }
                }
                ERESULT eres = WriteTitleAndDuration(i + 1, pRecord, pOutputStream);
                if (FAILED(eres))
                {
                    delete pOutputStream;
                    return eres;
                }
            }
        }
    }

    // Finish up with the number of entries and the version number.
    sprintf(szFileBuffer, "NumberOfEntries=%d\nVersion=2\n", i);
    iToWrite = strlen(szFileBuffer);
    if (pOutputStream->Write((void*)szFileBuffer, iToWrite) != iToWrite)
    {
        delete pOutputStream;
        return PLAYLIST_FORMAT_WRITE_ERROR;
    }

    delete pOutputStream;
    return PLAYLIST_FORMAT_NO_ERROR;
}

REGISTER_PLAYLIST_FORMAT( "Winamp PLS playlist format", PLS_PLAYLIST_FORMAT_ID, LoadPLSPlaylist, SavePLSPlaylist, "pls" );
