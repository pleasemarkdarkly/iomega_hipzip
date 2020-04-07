//........................................................................................
//........................................................................................
//.. File Name: DPLPlaylistFormat.cpp													..
//.. Date: 7/30/2001																	..
//.. Author(s): Edward Miller															..
//.. Description of content: interface for the CDPLPlaylistFormat class	 				..
//.. Usage: This is a part of the IObjects Direct Streaming Services Library			..
//.. Last Modified By: Ed Miller	edwardm@fullplaymedia.com								..	
//.. Modification date: 7/30/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#include <content/common/ContentManager.h>     // metadata struct
#include <core/playmanager/PlayManager.h>   // to get content manager
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/common/PlaylistFormat.h>
#include <util/debug/debug.h>
#include <main/util/filenamestore/FileNameStore.h>

#include <stdio.h>  // sscanf, sprintf
#include <stdlib.h> // malloc

#define DPL_PLAYLIST_FORMAT_ID  0x61

DEBUG_MODULE_S( DPLD, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( DPLD );

static const int BUFFER_SIZE = 1024;

ERESULT
LoadDPLPlaylist( const char* szURL, IPlaylist* pPlaylist, bool bVerifyContent )
{
    IInputStream* pInputStream = CDataSourceManager::GetInstance()->OpenInputStream(szURL);
    if (!pInputStream)
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;

    char szBuffer[BUFFER_SIZE];
    char LineBuffer[BUFFER_SIZE];
    char URLBuffer[BUFFER_SIZE];
    char* pFN = LineBuffer;
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();

    while (int iRead = pInputStream->Read(szBuffer, BUFFER_SIZE))
    {
        DEBUGP(DPLD, DBGLEV_INFO, "Read %d bytes\n", iRead);
        char* pBuf = szBuffer;
        while (pBuf - szBuffer < iRead)
        {
            while ((pBuf - szBuffer < iRead) && (*pBuf != '\n'))
            {
                *pFN++ = *pBuf++;
                // If the filename is longer than the buffer then assume this is a bad file.
                if (pFN - LineBuffer >= BUFFER_SIZE)
                {
                    delete pInputStream;
                    return PLAYLIST_FORMAT_BAD_FORMAT;
                }
            }
            if ((pBuf - szBuffer < iRead) && (*pBuf == '\n'))
            {
                *pFN = '\0';
                ++pBuf;
                DEBUGP(DPLD, DBGLEV_INFO, "Line: %s\n", LineBuffer);
                media_record_info_t mediaContent;
                mediaContent.pMetadata = 0;     // No extra metadata is extracted from this playlist format.
                if (sscanf(LineBuffer, "%d %d %[^\0]", &mediaContent.iDataSourceID, &mediaContent.iCodecID, URLBuffer) != 3)
                    continue;
                if (bVerifyContent && !pCM->GetMediaRecord(URLBuffer))
                {
                    if (!pCM->GetMediaRecord(URLBuffer))
                    {
                        pFN = LineBuffer;
                        continue;
                    }
                    else
                        mediaContent.bVerified = true;
                }
                else
                    mediaContent.bVerified = false;
                IFileNameRef* file = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->GetRefByURL(URLBuffer);
                mediaContent.pFilename = file;
                pFN = LineBuffer;
                if (IMediaContentRecord* pRecord = pCM->AddMediaRecord(mediaContent))
                    pPlaylist->AddEntry(pRecord);
            }
        }
    }

    delete pInputStream;

    return PLAYLIST_FORMAT_NO_ERROR;
}

#define PLAYLIST_STRING_SIZE	256

ERESULT
SaveDPLPlaylist( const char* szURL, IPlaylist* pPlaylist )
{
    if (IOutputStream* pOutputStream = CDataSourceManager::GetInstance()->OpenOutputStream(szURL))
    {
        for (int i = 0; i < pPlaylist->GetSize(); ++i)
        {
            if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(i))
            {
                if (IMediaContentRecord* pRecord = pEntry->GetContentRecord())
                {
                    char szBuffer[BUFFER_SIZE];
                    sprintf(szBuffer, "%d %d %s\n", pRecord->GetDataSourceID(), pRecord->GetCodecID(), pRecord->GetFileNameRef()->URL());
                    int iToWrite = strlen(szBuffer);
                    if (pOutputStream->Write((void*)szBuffer, iToWrite) != iToWrite)
                    {
                        delete pOutputStream;
                        return PLAYLIST_FORMAT_WRITE_ERROR;
                    }
                }
            }
        }
        delete pOutputStream;
        return PLAYLIST_FORMAT_NO_ERROR;
    }
    else
        return PLAYLIST_FORMAT_FILE_OPEN_ERROR;

}

REGISTER_PLAYLIST_FORMAT( "Dadio playlist format", DPL_PLAYLIST_FORMAT_ID, LoadDPLPlaylist, SaveDPLPlaylist, "dpl" );
