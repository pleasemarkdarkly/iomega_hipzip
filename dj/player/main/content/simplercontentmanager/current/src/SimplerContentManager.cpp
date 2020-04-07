//
// SimplerContentManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/content/simplercontentmanager/SimplerContentManager.h>

#include "SimplerMetadata.h"

#include <main/content/djcontentmanager/DJContentManager.h>
#include <playlist/common/Playlist.h>

#include <util/debug/debug.h>
#include <stdlib.h> /* free */


DEBUG_MODULE_S(SIMPLERCONTENTMANAGER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(SIMPLERCONTENTMANAGER);

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimplerContentManager
//////////////////////////////////////////////////////////////////////////////////////////

//! The global singleton CSimplerContentManager.
CSimplerContentManager* CSimplerContentManager::s_pInstance = 0;

int CSimplerContentManager::sm_iNextPlaylistRecordID = 1;

//! Returns a pointer to the global cddb query manager.
CSimplerContentManager*
CSimplerContentManager::GetInstance()
{
    if (!s_pInstance)
        s_pInstance = new CSimplerContentManager;
    return s_pInstance;
}

//! Destroy the singleton global cddb query manager.
void
CSimplerContentManager::Destroy()
{
    if (s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = 0;
    }
}


CSimplerContentManager::CSimplerContentManager()
    : m_pDJContentManager(0)
{
}

CSimplerContentManager::~CSimplerContentManager()
{
    Clear();
}

//! Stores a pointer to the DJ content manager, which it uses for generating content record IDs.
void
CSimplerContentManager::SetDJContentManager(CDJContentManager* pDJContentManager)
{
    m_pDJContentManager = pDJContentManager;
}

// Creates an empty metadata record.
IMetadata*
CSimplerContentManager::CreateMetadataRecord() const
{
    return new CSimplerMetadata;
}

// Called from the event handler when a data source returns a list of records.
void
CSimplerContentManager::AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords)
{
    DEBUGP(SIMPLERCONTENTMANAGER, DBGLEV_INFO, "Media content size: %d\n", pContentUpdate->media.Size());
    if (!pContentUpdate->bTwoPass)
    {
        for (int i = 0; i < pContentUpdate->media.Size(); ++i)
        {
            DEBUGP(SIMPLERCONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->media[i].szURL);
            if (pRecords)
                pRecords->PushBack(AddMediaRecordInternal(pContentUpdate->media[i]));
            else
                AddMediaRecordInternal(pContentUpdate->media[i]);
            free(pContentUpdate->media[i].szURL);
        }
        pContentUpdate->media.Clear();
    }
    DEBUGP(SIMPLERCONTENTMANAGER, DBGLEV_INFO, "Playlist content size: %d\n", pContentUpdate->media.Size());
    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
    {
        DEBUGP(SIMPLERCONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->playlists[i].szURL);
        AddPlaylistRecord(pContentUpdate->playlists[i]);
        free(pContentUpdate->playlists[i].szURL);
    }
    pContentUpdate->playlists.Clear();

}

// Clears the content manager and deletes its content records.
void
CSimplerContentManager::Clear()
{
    while (!m_slMediaRecords.IsEmpty())
        delete m_slMediaRecords.PopFront();
    while (!m_slPlaylists.IsEmpty())
        delete m_slPlaylists.PopFront();
}

// Returns the number of media records in the content manager.
int
CSimplerContentManager::GetMediaRecordCount()
{
    return m_slMediaRecords.Size();
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CSimplerContentManager::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
/*
    MediaRecordIterator it = FindMediaRecordByURL(mediaContent.szURL);
    if (it != m_slMediaRecords.GetEnd())
    {
        // This record already exists in the content manager, but if the new record is verified
        // as existing on a data source then mark the record inside the content manager as the same.
        if (!(*it)->IsVerified() && mediaContent.bVerified)
            (*it)->SetVerified(mediaContent.bVerified);
        // Merge any new metadata from the content update.
        if (mediaContent.pMetadata)
        {
            (*it)->MergeAttributes(mediaContent.pMetadata);
            delete mediaContent.pMetadata;
            mediaContent.pMetadata = 0;
        }
        if (pbAlreadyExists)
            *pbAlreadyExists = true;
        return *it;
    }
*/

    if (pbAlreadyExists)
        *pbAlreadyExists = false;

    return AddMediaRecordInternal(mediaContent);
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
IMediaContentRecord*
CSimplerContentManager::AddMediaRecordInternal(media_record_info_t& mediaContent)
{
    CSimpleMediaContentRecord* pRecord = new CSimpleMediaContentRecord(m_pDJContentManager->GetNextMediaContentRecordID(), mediaContent.szURL,
        mediaContent.iDataSourceID, mediaContent.iCodecID,
        mediaContent.pMetadata ? mediaContent.pMetadata : CreateMetadataRecord(),
        mediaContent.bVerified);

    m_slMediaRecords.PushBack(pRecord);

    return pRecord;
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CSimplerContentManager::DeleteMediaRecord(int iContentRecordID)
{
    MediaRecordIterator it = FindMediaRecordByID(iContentRecordID);
    if (it != m_slMediaRecords.GetEnd())
    {
        delete m_slMediaRecords.Remove(it);
        return true;
    }
    return false;
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
// WARNING: The pContentRecord pointer will be invalid after this operation!
bool
CSimplerContentManager::DeleteMediaRecord(IMediaContentRecord* pContentRecord)
{
    MediaRecordIterator it = FindMediaRecordByURL(pContentRecord->GetURL());
    if (it != m_slMediaRecords.GetEnd())
    {
        delete m_slMediaRecords.Remove(it);
        return true;
    }
    return false;
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CSimplerContentManager::GetMediaRecord(int iContentRecordID)
{
    MediaRecordIterator it = FindMediaRecordByID(iContentRecordID);
    if (it != m_slMediaRecords.GetEnd())
        return *it;
    else
        return 0;
}

// Returns a pointer to the media content record with the specified URL.
// Returns 0 if no record with a matching URL was found.
IMediaContentRecord*
CSimplerContentManager::GetMediaRecord(const char* szURL)
{
    for (MediaRecordIterator it = m_slMediaRecords.GetHead(); it != m_slMediaRecords.GetEnd(); ++it)
        if (!strcmp((*it)->GetURL(), szURL))
            return *it;

    return 0;
}


// Returns the number of playlist records in the content manager.
int
CSimplerContentManager::GetPlaylistRecordCount()
{
    return m_slPlaylists.Size();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CSimplerContentManager::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    PlaylistRecordIterator it = FindPlaylistRecordByURL(playlistRecord.szURL);
    if (it != m_slPlaylists.GetEnd())
    {
        free(playlistRecord.szURL);
        if (!(*it)->IsVerified() && playlistRecord.bVerified)
            (*it)->SetVerified(playlistRecord.bVerified);
        return *it;
    }

    CSimplePlaylistRecord* pRecord = new CSimplePlaylistRecord(sm_iNextPlaylistRecordID++, playlistRecord.szURL,
        playlistRecord.iDataSourceID, playlistRecord.iPlaylistFormatID, playlistRecord.bVerified);

    m_slPlaylists.PushBack(pRecord);

    return pRecord;
}

// Removes a playlist record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CSimplerContentManager::DeletePlaylistRecord(int iContentRecordID)
{
    PlaylistRecordIterator it = FindPlaylistRecordByID(iContentRecordID);
    if (it != m_slPlaylists.GetEnd())
    {
        delete m_slPlaylists.Remove(it);
        return true;
    }
    return false;
}

// Removes all records from the given data source from the content manager.
void
CSimplerContentManager::DeleteRecordsFromDataSource(int iDataSourceID)
{
    MediaRecordIterator itMedia = m_slMediaRecords.GetHead();
    while (itMedia != m_slMediaRecords.GetEnd())
    {
        if ((*itMedia)->GetDataSourceID() == iDataSourceID)
        {
            MediaRecordIterator temp = itMedia + 1;
            delete m_slMediaRecords.Remove(itMedia);
            itMedia = temp;
        }
        else
            ++itMedia;
    }

    PlaylistRecordIterator itPlaylist = m_slPlaylists.GetHead();
    while (itPlaylist != m_slPlaylists.GetEnd())
    {
        if ((*itPlaylist)->GetDataSourceID() == iDataSourceID)
        {
            PlaylistRecordIterator temp = itPlaylist + 1;
            delete m_slPlaylists.Remove(itPlaylist);
            itPlaylist = temp;
        }
        else
            ++itPlaylist;
    }
}

// Marks all records from the given data source as unverified.
void
CSimplerContentManager::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    for (MediaRecordIterator itMedia = m_slMediaRecords.GetHead();
        itMedia != m_slMediaRecords.GetEnd(); ++itMedia)
    {
        if ((*itMedia)->GetDataSourceID() == iDataSourceID)
            (*itMedia)->SetVerified(false);
    }

    for (PlaylistRecordIterator itPlaylist = m_slPlaylists.GetHead();
        itPlaylist != m_slPlaylists.GetEnd(); ++itPlaylist)
    {
        if ((*itPlaylist)->GetDataSourceID() == iDataSourceID)
            (*itPlaylist)->SetVerified(false);
    }
}

// Removes all records from the given data source that are marked as unverified.
void
CSimplerContentManager::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
{
    MediaRecordIterator itMedia = m_slMediaRecords.GetHead();
    while (itMedia != m_slMediaRecords.GetEnd())
    {
        if (((*itMedia)->GetDataSourceID() == iDataSourceID)
            && !(*itMedia)->IsVerified())
        {
            MediaRecordIterator temp = itMedia + 1;
            delete m_slMediaRecords.Remove(itMedia);
            itMedia = temp;
        }
        else
            ++itMedia;
    }

    PlaylistRecordIterator itPlaylist = m_slPlaylists.GetHead();
    while (itPlaylist != m_slPlaylists.GetEnd())
    {
        if (((*itPlaylist)->GetDataSourceID() == iDataSourceID)
            && !(*itPlaylist)->IsVerified())
        {
            PlaylistRecordIterator temp = itPlaylist + 1;
            delete m_slPlaylists.Remove(itPlaylist);
            itPlaylist = temp;
        }
        else
            ++itPlaylist;
    }
}

void
CSimplerContentManager::GetAllMediaRecords(MediaRecordList& records) const
{
    ListAllRecords(records);
}

void
CSimplerContentManager::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; it != m_slMediaRecords.GetEnd(); ++it)
        if ((*it)->GetDataSourceID() == iDataSourceID)
            records.PushBack(*it);
}

void
CSimplerContentManager::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (PlaylistRecordIterator it = m_slPlaylists.GetHead(); it != m_slPlaylists.GetEnd(); ++it)
        records.PushBack(*it);
}

void
CSimplerContentManager::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    PlaylistRecordIterator it = m_slPlaylists.GetHead();
    for (; it != m_slPlaylists.GetEnd(); ++it)
        if ((*it)->GetDataSourceID() == iDataSourceID)
            records.PushBack(*it);
}


MediaRecordIterator
CSimplerContentManager::FindMediaRecordByID(int iContentRecordID)
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; (it != m_slMediaRecords.GetEnd()) && ((*it)->GetID() != iContentRecordID); ++it)
        ;
    return it;
}

MediaRecordIterator
CSimplerContentManager::FindMediaRecordByURL(const char* szURL)
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; (it != m_slMediaRecords.GetEnd()) && strcmp((*it)->GetURL(), szURL); ++it)
        ;
    return it;
}

void
CSimplerContentManager::ListAllRecords(MediaRecordList& records) const
{
    for (MediaRecordIterator it = m_slMediaRecords.GetHead(); it != m_slMediaRecords.GetEnd(); ++it)
        records.PushBack(*it);
}

PlaylistRecordIterator
CSimplerContentManager::FindPlaylistRecordByID(int iContentRecordID)
{
    PlaylistRecordIterator it = m_slPlaylists.GetHead();
    for (; (it != m_slPlaylists.GetEnd()) && ((*it)->GetID() != iContentRecordID); ++it)
        ;
    return it;
}

PlaylistRecordIterator
CSimplerContentManager::FindPlaylistRecordByURL(const char* szURL)
{
    PlaylistRecordIterator it = m_slPlaylists.GetHead();
    for (; (it != m_slPlaylists.GetEnd()) && strcmp((*it)->GetURL(), szURL); ++it)
        ;
    return it;
}

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimpleMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

CSimpleMediaContentRecord::CSimpleMediaContentRecord(int iID, char* szURL, int iDataSourceID, int iCodecID,
    IMetadata* pMetadata, bool bVerified)
    : m_iID(iID),
    m_iDataSourceID(iDataSourceID),
    m_iCodecID(iCodecID),
    m_pMetadata(pMetadata),
    m_bVerified(bVerified),
    m_eStatus(CR_OKAY)
{
    m_szURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szURL, szURL);
}

CSimpleMediaContentRecord::~CSimpleMediaContentRecord()
{
    free(m_szURL);
    delete m_pMetadata;
}

const TCHAR*
CSimpleMediaContentRecord::GetTitle() const
{
    TCHAR* szValue;
    if (m_pMetadata && SUCCEEDED(m_pMetadata->GetAttribute(MDA_TITLE, (void**)&szValue)))
        return szValue;
    else
        return 0;
}

const TCHAR*
CSimpleMediaContentRecord::GetAlbum() const
{
    TCHAR* szValue;
    if (m_pMetadata && SUCCEEDED(m_pMetadata->GetAttribute(MDA_ALBUM, (void**)&szValue)))
        return szValue;
    else
        return 0;
}

const TCHAR*
CSimpleMediaContentRecord::GetGenre() const
{
    TCHAR* szValue;
    if (m_pMetadata && SUCCEEDED(m_pMetadata->GetAttribute(MDA_GENRE, (void**)&szValue)))
        return szValue;
    else
        return 0;
}

const TCHAR*
CSimpleMediaContentRecord::GetArtist() const
{
    TCHAR* szValue;
    if (m_pMetadata && SUCCEEDED(m_pMetadata->GetAttribute(MDA_ARTIST, (void**)&szValue)))
        return szValue;
    else
        return 0;
}

void
CSimpleMediaContentRecord::SetTitle(const TCHAR* szTitle)
{
    if (m_pMetadata)
        m_pMetadata->SetAttribute(MDA_TITLE, (void*)szTitle);
}

void
CSimpleMediaContentRecord::SetAlbum(const TCHAR* szAlbum)
{
    if (m_pMetadata)
        m_pMetadata->SetAttribute(MDA_ALBUM, (void*)szAlbum);
}

void
CSimpleMediaContentRecord::SetGenre(const TCHAR* szGenre)
{
    if (m_pMetadata)
        m_pMetadata->SetAttribute(MDA_GENRE, (void*)szGenre);
}

void
CSimpleMediaContentRecord::SetArtist(const TCHAR* szArtist)
{
    if (m_pMetadata)
        m_pMetadata->SetAttribute(MDA_ARTIST, (void*)szArtist);
}

void
CSimpleMediaContentRecord::SetDataSourceID(int iDataSourceID)
{
    m_iDataSourceID = iDataSourceID;
}

void
CSimpleMediaContentRecord::SetCodecID(int iCodecID)
{
    m_iCodecID = iCodecID;
}


//////////////////////////////////////////////////////////////////////////////////////////
//	CSimplePlaylistRecord
//////////////////////////////////////////////////////////////////////////////////////////

CSimplePlaylistRecord::CSimplePlaylistRecord(int iID, char* szURL, int iDataSourceID, int iPlaylistFormatID, bool bVerified)
    : m_iID(iID),
    m_iDataSourceID(iDataSourceID),
    m_iPlaylistFormatID(iPlaylistFormatID),
    m_bVerified(bVerified),
    m_eStatus(CR_OKAY)
{
    m_szURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szURL, szURL);
}

CSimplePlaylistRecord::~CSimplePlaylistRecord()
{
    free(m_szURL);
}
