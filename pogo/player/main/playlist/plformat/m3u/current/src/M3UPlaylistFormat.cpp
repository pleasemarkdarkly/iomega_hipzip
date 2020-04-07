//........................................................................................
//........................................................................................
//.. File Name: M3UPlaylistFormat.cpp													..
//.. Date: 8/8/2001																	    ..
//.. Author(s): Edward Miller															..
//.. Description of content: m3u playlist format loading and saving functions           ..
//.. Usage: This is a part of the IObjects Direct Streaming Services Library			..
//.. Last Modified By: Ed Miller	edwardm@fullplaymedia.com								..	
//.. Modification date: 8/8/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
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
#include <main/util/filenamestore/FileNameStore.h>
#include <main/content/metakitcontentmanager/MetakitContentManager.h>

#include <stdio.h>  // sscanf, sprintf
#include <stdlib.h>  // malloc

#define M3U_PLAYLIST_FORMAT_ID  0x62

DEBUG_MODULE_S( M3UD, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( M3UD );

static const char* sc_M3UHeader = "#EXTM3U";
static const char* sc_M3UInfoPrefix = "#EXTINF:";

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
            iCurFileBufPos = 0;
            if ((iFileBufferBytes = pInputStream->Read(szFileBuffer, iMaxFileBytes)) <= 0)
            {
                *pchLineBuffer = '\0';
                return pchLineBuffer - szLineBuffer;
            }
        }
        while ((iCurFileBufPos < iFileBufferBytes) && (szFileBuffer[iCurFileBufPos] != '\n'))
        {
            if ((*pchLineBuffer++ = szFileBuffer[iCurFileBufPos++]) == '\r')
                --pchLineBuffer;
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
LoadM3UPlaylist( const char* szURL, IPlaylist* pPlaylist, bool bVerifyContent )
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
    CMetakitContentManager* pMCM = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();

    int iCurFileBufPos = 0, iFileBufferBytes = 0;

    if (GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE) <= 0)
    {
        return PLAYLIST_FORMAT_BAD_FORMAT;
    }

    // Only handle extended m3u files.
    if (strcmp(LineBuffer, sc_M3UHeader))
        return PLAYLIST_FORMAT_BAD_FORMAT;

    // Construct a prefix string for files from this data source.
#define METADATA_STRING_SIZE 256
    char DSLocalPrefix[METADATA_STRING_SIZE];
    strcpy(DSLocalPrefix, szURL);
    char* pch;
    if (pch = strrchr(DSLocalPrefix, '\\'))
        *++pch = '\0';
    char DSAbsolutePrefix[METADATA_STRING_SIZE];
    strcpy(DSAbsolutePrefix, szURL);
    if (pch = strchr(DSAbsolutePrefix, '\\'))
        *++pch = '\0';

    int iLineSize;
    while ((iLineSize = GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE)) > 0)
    {
        media_record_info_t mediaContent;
        mediaContent.iCodecID = 0;
        mediaContent.iDataSourceID = iDataSourceID;
        mediaContent.pMetadata = pMCM->CreateMetadataRecord();
        // Check for the info line.
        if (!strncmp(LineBuffer, sc_M3UInfoPrefix, strlen(sc_M3UInfoPrefix)))
        {
            int iSeconds;
            char Title[BUFFER_SIZE];
            if (sscanf(LineBuffer, "#EXTINF:%d,%[^\0]", &iSeconds, Title) == 2)
            {
                DEBUGP(M3UD, DBGLEV_INFO, "Title: %s Length: %d:%02d\n", Title, iSeconds / 60, iSeconds % 60);
                TCHAR tszTitle[METADATA_STRING_SIZE];
                mediaContent.pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszTitle, Title, METADATA_STRING_SIZE));
            }

            if ((iLineSize = GetLine(pInputStream, szFileBuffer, BUFFER_SIZE, iCurFileBufPos, iFileBufferBytes, LineBuffer, BUFFER_SIZE)) <= 0)
            {
                // Couldn't read the line that contains the file name, so skip this entry.
                delete mediaContent.pMetadata;
                continue;
            }
        }

        if (LineBuffer[0] == '\\')
        {
            // Absolute path
            char temp[strlen(DSAbsolutePrefix) + strlen(LineBuffer)+1];
            strcpy(temp, DSAbsolutePrefix);
            strcat(temp, LineBuffer + 1);
            IFileNameRef* file = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->GetRefByURL(temp);
            //mediaContent.szURL = (char*)malloc(strlen(DSAbsolutePrefix) + strlen(LineBuffer));
            mediaContent.pFilename = file;
        }
        else if (IDataSource* pDS = CDataSourceManager::GetInstance()->GetDataSourceByURL(LineBuffer))
        {
            char temp [strlen(LineBuffer) + 1];
            strcpy(temp, LineBuffer);
            //mediaContent.szURL = (char*)malloc(strlen(LineBuffer) + 1);
            IFileNameRef* file = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->GetRefByURL(temp);
            mediaContent.pFilename = file;
            mediaContent.iDataSourceID = pDS->GetInstanceID();
        }
        else
        {
            // Local path
            char temp [strlen(DSLocalPrefix) + strlen(LineBuffer) + 1];
            strcpy(temp, DSLocalPrefix);
            strcat(temp, LineBuffer);
            IFileNameRef* file = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->GetRefByURL(temp);
            mediaContent.pFilename = file;
        }

        mediaContent.bVerified = true;
        if (bVerifyContent)
        {
            bool bExists = false;
            IMediaContentRecord* pRecord = pMCM->MergeMetadata(mediaContent);  // Merge metadata.
            if (pRecord != NULL)
                pPlaylist->AddEntry(pRecord);
        }
        else if (IMediaContentRecord* pRecord = pMCM->AddMediaRecord(mediaContent))
        {
            pPlaylist->AddEntry(pRecord);
        }
        else
        {
            delete mediaContent.pMetadata;
        }
        //free(mediaContent.szURL);
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
        strncpy(szTitle, pRecord->GetFileNameRef()->LongName(), 256);

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
                    char* url = pRecord->GetFileNameRef()->URL();
                    sprintf(szFileBuffer, "%s\n",  url + iOutputDSLen);
                    if (pRecord->GetFileNameRef()->DynamicAlloc())
                        delete [] url;
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
                    char* url = pRecord->GetFileNameRef()->URL();
                    sprintf(szFileBuffer, "%s\n", url);
                    if (pRecord->GetFileNameRef()->DynamicAlloc())
                        delete [] url;
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
