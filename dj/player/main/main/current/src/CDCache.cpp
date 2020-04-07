//
// CDCache.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "CDCache.h"

#include <content/common/ContentManager.h>
#include <core/events/SystemEvents.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <main/main/DJPlayerState.h>
#include <main/main/ProgressWatcher.h>
#include <util/debug/debug.h>

DEBUG_MODULE_S(CDCACHE, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(CDCACHE);

#define CD_CACHE_DB_FORMAT "CDs[ID:I,TrackCount:I,URLs[URL:S],Tracks[URL:S,Title:W,Artist:W,Album:W,Genre:W,AlbumTrackNumber:I]]"

CCDCache::CCDCache(const char* szDBFilename, int iMinTrackCount = 20, int iURLMatchCount = 10)
    : m_mkCDCache(szDBFilename, 1),
    mc_iMinTrackCount(iMinTrackCount),
    mc_iURLMatchCount(iURLMatchCount),
    m_iRow(-1),
    m_bNew(false),
    m_bAbortCache(false)
{
    DBASSERT(CDCACHE, iMinTrackCount >= iURLMatchCount, "Minimum track count is greater than the required match count\n");
    m_vCDCache = m_mkCDCache.GetAs(CD_CACHE_DB_FORMAT);
}

CCDCache::~CCDCache()
{
}

//! Called after the initial CD content update.
//! Checks the cache to find a matching CD.
//! If a match isn't found, then content updates are sent to the content manager
//! in the chunk size specified.
int
CCDCache::NotifyContentUpdate(content_record_update_t* pContentUpdate, IContentManager* pContentManager, int iUpdateChunkSize)
{
    CPlayManager* pPM = CPlayManager::GetInstance();

    m_bAbortCache = false;

    if (pContentUpdate->media.Size() <= mc_iMinTrackCount)
    {
        DEBUG(CDCACHE, DBGLEV_INFO, "Only %d tracks, so not caching\n", pContentUpdate->media.Size());
        m_bNew = false;
        pPM->HandleEvent(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
        return 0;
    }


    c4_StringProp pURL("URL");
    c4_IntProp pDiskID("ID"), pTrackCount("TrackCount");
    c4_ViewProp pTracks("Tracks");

    int iDiskID = CDJPlayerState::GetInstance()->GetCDDataSource()->GetDiscID();
    DEBUG(CDCACHE, DBGLEV_INFO, "Checking cache for CD 0x%x, tracks: %d\n", iDiskID, pContentUpdate->media.Size());
    m_iRow = m_vCDCache.Find(pDiskID[iDiskID] + pTrackCount[pContentUpdate->media.Size()]);
    while (m_iRow != -1)
    {
        // Match based on url compares.
        c4_StringProp pURL("URL");
        c4_View vTracks = pTracks(m_vCDCache[m_iRow]);

        if (vTracks.GetSize() != pTrackCount(m_vCDCache[m_iRow]))
        {
            DEBUG(CDCACHE, DBGLEV_ERROR, "Corrupt cache entry in row %d: Track count: %d vs %d\n", m_iRow, vTracks.GetSize(), (int)pTrackCount(m_vCDCache[m_iRow]));
            m_vCDCache.RemoveAt(m_iRow);
            m_iRow = m_vCDCache.Find(pDiskID[iDiskID] + pTrackCount[pContentUpdate->media.Size()], m_iRow);
        }
        else
        {
            int step = pContentUpdate->media.Size() / mc_iURLMatchCount;
            bool bMatch = true;
            for (int i = 0; i < mc_iURLMatchCount; ++i)
            {
                if (strcmp(pContentUpdate->media[i * step].szURL, (const char*)pURL(vTracks[i * step])))
                {
                    DEBUG(CDCACHE, DBGLEV_INFO, "Near miss in row %d:\n%s vs\n%s\n", m_iRow, pContentUpdate->media[i * step].szURL, (const char*)pURL(vTracks[i * step]));
                    bMatch = false;
                    break;
                }
            }
            if (bMatch)
            {
                DEBUG(CDCACHE, DBGLEV_INFO, "Match found in row %d\n", m_iRow);
                m_bNew = false;

                c4_WideStringProp pTitle("Title"), pArtist("Artist"), pAlbum("Album"), pGenre("Genre");
                c4_IntProp pAlbumTrackNumber("AlbumTrackNumber");
                for (int i = 0; i < vTracks.GetSize(); ++i)
                {

                    DEBUG(CDCACHE, DBGLEV_TRACE, "Track %d: %s\n%w\n%w\n%w\n%w\n", i, (const char*)pURL(vTracks[i]),
                        (const TCHAR*)pTitle(vTracks[i]), (const TCHAR*)pArtist(vTracks[i]),
                        (const TCHAR*)pAlbum(vTracks[i]), (const TCHAR*)pGenre(vTracks[i]));

                    if ( (pContentUpdate->media[i].pMetadata = pContentManager->CreateMetadataRecord()) )
                    {
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_TITLE, (void*)(const TCHAR*)pTitle(vTracks[i]));
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_ARTIST, (void*)(const TCHAR*)pArtist(vTracks[i]));
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_ALBUM, (void*)(const TCHAR*)pAlbum(vTracks[i]));
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_GENRE, (void*)(const TCHAR*)pGenre(vTracks[i]));
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_ALBUM_TRACK_NUMBER, (void*)(int)pAlbumTrackNumber(vTracks[i]));
                    }
                }
                pContentUpdate->bTwoPass = false;
                pPM->HandleEvent(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
                return vTracks.GetSize();
            }
            m_iRow = m_vCDCache.Find(pDiskID[iDiskID] + pTrackCount[pContentUpdate->media.Size()], m_iRow + 1);
        }
    }
    DEBUG(CDCACHE, DBGLEV_INFO, "No match found\n");
    m_iRow = m_vCDCache.Add(pDiskID[iDiskID] + pTrackCount[pContentUpdate->media.Size()]);
    m_bNew = true;
    m_iTotalTracks = pContentUpdate->media.Size();
    m_iCurrentTrackCount = 0;
    c4_View vTracks = pTracks(m_vCDCache[m_iRow]);
    vTracks.SetSize(m_iTotalTracks);

    // Split the monolithic update record into smaller bite-sized chunks for
    // metadata retrieval.
    int iTotalChunkSize = 0;
    while (pContentUpdate->media.Size() - iTotalChunkSize > iUpdateChunkSize)
    {
        content_record_update_t* pContentUpdateChunk = new content_record_update_t;
        pContentUpdateChunk->bTwoPass = true;
        pContentUpdateChunk->iDataSourceID = pContentUpdate->iDataSourceID;
        pContentUpdateChunk->usScanID = pContentUpdate->usScanID;
        for (int i = 0; i < iUpdateChunkSize; ++i)
            pContentUpdateChunk->media.PushBack(pContentUpdate->media[iTotalChunkSize++]);
        pPM->HandleEvent(EVENT_CONTENT_UPDATE, (void*)pContentUpdateChunk);
    }
    if ((pContentUpdate->media.Size() > iTotalChunkSize) || !pContentUpdate->playlists.IsEmpty())
    {
        pContentUpdate->media.Remove(0, iTotalChunkSize);
        pPM->HandleEvent(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
    }

    return 0;
}

void
CCDCache::NotifyMetadataUpdate(content_record_update_t* pContentUpdate)
{
    if (m_bNew && !m_bAbortCache)
    {
        // This is a new disc, so add its metadata to the database.
        c4_WideStringProp pTitle("Title"), pArtist("Artist"), pAlbum("Album"), pGenre("Genre");
        c4_IntProp pAlbumTrackNumber("AlbumTrackNumber");
        c4_StringProp pURL("URL");
        c4_ViewProp pTracks("Tracks");
        c4_View vTracks = pTracks(m_vCDCache[m_iRow]);

        for (int i = 0; i < pContentUpdate->media.Size(); ++i)
        {
            const TCHAR* szTitle = 0, *szArtist = 0, *szAlbum = 0, *szGenre = 0;
            int iAlbumTrackNumber = 0;
            static const TCHAR szNoValue[1] = { '\0' };

            if (FAILED(pContentUpdate->media[i].pMetadata->GetAttribute(MDA_TITLE, (void**)&szTitle)))
                szTitle = szNoValue;
            if (FAILED(pContentUpdate->media[i].pMetadata->GetAttribute(MDA_ARTIST, (void**)&szArtist)))
                szArtist = szNoValue;
            if (FAILED(pContentUpdate->media[i].pMetadata->GetAttribute(MDA_ALBUM, (void**)&szAlbum)))
                szAlbum = szNoValue;
            if (FAILED(pContentUpdate->media[i].pMetadata->GetAttribute(MDA_GENRE, (void**)&szGenre)))
                szGenre = szNoValue;
            pContentUpdate->media[i].pMetadata->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&iAlbumTrackNumber);

            DEBUG(CDCACHE, DBGLEV_TRACE, "Adding track %d: %s\n%w\n%w\n%w\n%w\n", i, pContentUpdate->media[i].szURL,
                szTitle, szArtist, szAlbum, szGenre);

            int row = m_iCurrentTrackCount + i;
            pURL(vTracks[row]) = pContentUpdate->media[i].szURL;
            pTitle(vTracks[row]) = szTitle;
            pArtist(vTracks[row]) = szArtist;
            pAlbum(vTracks[row]) = szAlbum;
            pGenre(vTracks[row]) = szGenre;
            pAlbumTrackNumber(vTracks[row]) = iAlbumTrackNumber;
        }

        // Update the track count.
        m_iCurrentTrackCount += pContentUpdate->media.Size();
    }
}

//! Called upon receiving the EVENT_METADATA_UPDATE_END message.
//! If the CD being read is new to the cache, then the cache is committed to disk.
void
CCDCache::NotifyMetadataUpdateEnd()
{
    if (m_bNew)
    {
        if (!m_bAbortCache)
        {
            // If the number of tracks doesn't equal the number of metadata records received,
            // then the CD was probably ejected while reading.
            // If so, then take it out of the cache.
            if (m_iCurrentTrackCount != m_iTotalTracks)
            {
                DEBUG(CDCACHE, DBGLEV_WARNING, "Total tracks on CD: %d  MD read: %d --- aborting CD cache\n", m_iTotalTracks, m_iCurrentTrackCount);
                m_vCDCache.RemoveAt(m_iRow);
            }
            else
            {
                CProgressWatcher::GetInstance()->SetTask(TASK_COMMITTING_CD_METADATA_CACHE);
                Commit();
                CProgressWatcher::GetInstance()->UnsetTask(TASK_COMMITTING_CD_METADATA_CACHE);
            }
        }

        m_bNew = false;

/*
        DEBUG(CDCACHE, DBGLEV_INFO, "** New disk: Row %d: **\n", m_iRow);

        c4_StringProp pURL("URL");
        c4_ViewProp pTracks("Tracks");

        c4_View vTracks = pTracks(m_vCDCache[m_iRow]);
        for (int i = 0; i < vTracks.GetSize(); ++i)
        {
            DEBUG(CDCACHE, DBGLEV_INFO, "URL %d: %s\n", i, (const char*)pURL(vTracks[i]));
        }
        DEBUG(CDCACHE, DBGLEV_INFO, "\n");
*/

    }
}

//! Called when the CD is ejected.  Aborts any caching in progress.
void
CCDCache::NotifyCDRemoved()
{
    if (m_bNew)
    {
        // Remove the partial entry from the database.
        DEBUG(CDCACHE, DBGLEV_INFO, "Total tracks on CD: %d  MD read: %d --- aborting CD cache\n", m_iTotalTracks, m_iCurrentTrackCount);
        m_vCDCache.RemoveAt(m_iRow);

        m_bNew = false;
    }
    m_bAbortCache = true;
}

//! Saves the database to file.
bool
CCDCache::Commit()
{
    return m_mkCDCache.Commit(true);
}

//! Removes all disk info from the cache.
void
CCDCache::Clear()
{
    m_vCDCache.RemoveAll();
}
