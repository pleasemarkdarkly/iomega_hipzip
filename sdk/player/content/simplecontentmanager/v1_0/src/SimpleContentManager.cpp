//
// SimpleContentManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <content/simplecontentmanager/SimpleContentManager.h>

#include <playlist/common/Playlist.h>

#include <util/debug/debug.h>
#include <stdlib.h> /* free */


DEBUG_MODULE_S(SIMPLECONTENTMANAGER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(SIMPLECONTENTMANAGER);

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimpleContentManager
//////////////////////////////////////////////////////////////////////////////////////////

int CSimpleContentManager::sm_iNextMediaRecordID = 1;
int CSimpleContentManager::sm_iNextPlaylistRecordID = 1;

CSimpleContentManager::CSimpleContentManager(FNCreateMetadata* pfnCreateMetadata)
    : m_pfnCreateMetadata(pfnCreateMetadata)
{
}

CSimpleContentManager::~CSimpleContentManager()
{
    Clear();
}

// Creates an empty metadata record.
IMetadata*
CSimpleContentManager::CreateMetadataRecord() const
{
    if (m_pfnCreateMetadata)
        return (*m_pfnCreateMetadata)();
    else
        return 0;
}

// Called from the event handler when a data source returns a list of records.
//! If pRecords is non-zero, then it will be filled with a list of pointers to the new records.
void
CSimpleContentManager::AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords)
{
    DEBUGP(SIMPLECONTENTMANAGER, DBGLEV_INFO, "Media content size: %d\n", pContentUpdate->media.Size());
    int i = 0;
    while (i < pContentUpdate->media.Size())
    {
        DEBUGP(SIMPLECONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->media[i].szURL);
        bool pAlreadyExists;
        if (pRecords)
            pRecords->PushBack(AddMediaRecord(pContentUpdate->media[i], &pAlreadyExists));
        else
            AddMediaRecord(pContentUpdate->media[i], &pAlreadyExists);
        if (!pContentUpdate->bTwoPass || pAlreadyExists)
        {
            free(pContentUpdate->media[i].szURL);
            pContentUpdate->media.Remove(i);
        }
        else
            ++i;
    }
    DEBUGP(SIMPLECONTENTMANAGER, DBGLEV_INFO, "Playlist content size: %d\n", pContentUpdate->media.Size());
    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
    {
        DEBUGP(SIMPLECONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->playlists[i].szURL);
        AddPlaylistRecord(pContentUpdate->playlists[i]);
        free(pContentUpdate->playlists[i].szURL);
    }
    pContentUpdate->playlists.Clear();

}

// Clears the content manager and deletes its content records.
void
CSimpleContentManager::Clear()
{
    while (!m_slMediaRecords.IsEmpty())
        delete m_slMediaRecords.PopFront();
    while (!m_slPlaylists.IsEmpty())
        delete m_slPlaylists.PopFront();
}

// Returns the number of media records in the content manager.
int
CSimpleContentManager::GetMediaRecordCount()
{
    return m_slMediaRecords.Size();
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CSimpleContentManager::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
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

    CSimpleMediaContentRecord* pRecord = new CSimpleMediaContentRecord(sm_iNextMediaRecordID++, mediaContent.szURL,
        mediaContent.iDataSourceID, mediaContent.iCodecID,
        mediaContent.pMetadata ? mediaContent.pMetadata : CreateMetadataRecord(),
        mediaContent.bVerified);

    m_slMediaRecords.PushBack(pRecord);

    if (pbAlreadyExists)
        *pbAlreadyExists = false;

    return pRecord;
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CSimpleContentManager::DeleteMediaRecord(int iContentRecordID)
{
    MediaRecordIterator it = FindMediaRecordByID(iContentRecordID);
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
CSimpleContentManager::GetMediaRecord(int iContentRecordID)
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
CSimpleContentManager::GetMediaRecord(const char* szURL)
{
    for (MediaRecordIterator it = m_slMediaRecords.GetHead(); it != m_slMediaRecords.GetEnd(); ++it)
        if (!strcmp((*it)->GetURL(), szURL))
            return *it;

    return 0;
}


// Returns the number of playlist records in the content manager.
int
CSimpleContentManager::GetPlaylistRecordCount()
{
    return m_slPlaylists.Size();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CSimpleContentManager::AddPlaylistRecord(playlist_record_t& playlistRecord)
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
CSimpleContentManager::DeletePlaylistRecord(int iContentRecordID)
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
CSimpleContentManager::DeleteRecordsFromDataSource(int iDataSourceID)
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
CSimpleContentManager::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
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
CSimpleContentManager::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
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
CSimpleContentManager::GetAllMediaRecords(MediaRecordList& records) const
{
    ListAllRecords(records);
}

void
CSimpleContentManager::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; it != m_slMediaRecords.GetEnd(); ++it)
        if ((*it)->GetDataSourceID() == iDataSourceID)
            records.PushBack(*it);
}

void
CSimpleContentManager::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (PlaylistRecordIterator it = m_slPlaylists.GetHead(); it != m_slPlaylists.GetEnd(); ++it)
        records.PushBack(*it);
}

void
CSimpleContentManager::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    PlaylistRecordIterator it = m_slPlaylists.GetHead();
    for (; it != m_slPlaylists.GetEnd(); ++it)
        if ((*it)->GetDataSourceID() == iDataSourceID)
            records.PushBack(*it);
}


MediaRecordIterator
CSimpleContentManager::FindMediaRecordByID(int iContentRecordID)
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; (it != m_slMediaRecords.GetEnd()) && ((*it)->GetID() != iContentRecordID); ++it)
        ;
    return it;
}

MediaRecordIterator
CSimpleContentManager::FindMediaRecordByURL(const char* szURL)
{
    MediaRecordIterator it = m_slMediaRecords.GetHead();
    for (; (it != m_slMediaRecords.GetEnd()) && strcmp((*it)->GetURL(), szURL); ++it)
        ;
    return it;
}

void
CSimpleContentManager::ListAllRecords(MediaRecordList& records) const
{
    for (MediaRecordIterator it = m_slMediaRecords.GetHead(); it != m_slMediaRecords.GetEnd(); ++it)
        records.PushBack(*it);
}

PlaylistRecordIterator
CSimpleContentManager::FindPlaylistRecordByID(int iContentRecordID)
{
    PlaylistRecordIterator it = m_slPlaylists.GetHead();
    for (; (it != m_slPlaylists.GetEnd()) && ((*it)->GetID() != iContentRecordID); ++it)
        ;
    return it;
}

PlaylistRecordIterator
CSimpleContentManager::FindPlaylistRecordByURL(const char* szURL)
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
