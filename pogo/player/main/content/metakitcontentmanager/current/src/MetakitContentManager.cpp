//
// MetakitContentManager.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/content/metakitcontentmanager/MetakitContentManager.h>

#include "MetakitContentManagerImp.h"

#include <util/debug/debug.h>

DEBUG_MODULE_S(METAKITCONTENTMANAGER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(METAKITCONTENTMANAGER);


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitContentManager
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef INCREMENTAL_METAKIT
CMetakitContentManager::CMetakitContentManager(const char* szFilename)
{
    m_pImp = new CMetakitContentManagerImp(szFilename);
}

#else

CMetakitContentManager::CMetakitContentManager()
{
    m_pImp = new CMetakitContentManagerImp();
}

#endif

CMetakitContentManager::~CMetakitContentManager()
{
    delete m_pImp;
}

ERESULT
CMetakitContentManager::AddStoredMetadata(int iAttributeID, const void* pUnsetValue)
{
    return m_pImp->AddStoredMetadata(iAttributeID, pUnsetValue);
}

// Creates an empty metadata record.
IMetadata*
CMetakitContentManager::CreateMetadataRecord() const
{
    return m_pImp->CreateMetadataRecord();
}

// Loads content records from a stream.
// Returns true if the state was loaded, false othersise.
bool
CMetakitContentManager::LoadStateFromStream(IInputStream* pInputStream)
{
    return m_pImp->LoadStateFromStream(pInputStream);
}

// Saves content records to a stream.
// Returns true if the state was saved, false othersise.
bool
CMetakitContentManager::SaveStateToStream(IOutputStream* pOutputStream)
{
    return m_pImp->SaveStateToStream(pOutputStream);
}

// Clears the content manager and deletes its content records.
void
CMetakitContentManager::Clear()
{
    m_pImp->Clear();
}

// Saves the database.
void
CMetakitContentManager::Commit()
{
    m_pImp->Commit();
}

// Adds a list of media and playlist records to the content manager.
void
CMetakitContentManager::AddContentRecords(content_record_update_t* pContentUpdate)
{
    m_pImp->AddContentRecords(pContentUpdate);
}

void 
CMetakitContentManager::CompareFileSystemUpdate( CFileNameStore* store )
{
    m_pImp->CompareFileSystemUpdate( store );
}

CFileNameStore* 
CMetakitContentManager::GetFileNameStore()
{
    return m_pImp->GetFileNameStore();
}

// Returns the number of media records in the content manager.
int
CMetakitContentManager::GetMediaRecordCount()
{
    return m_pImp->GetMediaRecordCount();
}

// locate the relevant content, merge any new metadata, and return the record.  if not found, return 0.
IMediaContentRecord*
CMetakitContentManager::MergeMetadata(media_record_info_t& mediaContent)
{
    return m_pImp->MergeMetadata(mediaContent);
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CMetakitContentManager::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
    return m_pImp->AddMediaRecord(mediaContent, pbAlreadyExists);
}

// Removes a record from the content manager.
void
CMetakitContentManager::DeleteMediaRecord(int iContentRecordID)
{
    return m_pImp->DeleteMediaRecord(iContentRecordID);
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CMetakitContentManager::GetMediaRecord(int iContentRecordID)
{
    return m_pImp->GetMediaRecord(iContentRecordID);
}

// Returns a pointer to the media content record with the specified URL.
// Returns 0 if no record with a matching URL was found.
IMediaContentRecord* 
CMetakitContentManager::GetMediaRecord(const char* szURL)
{
    return m_pImp->GetMediaRecord(szURL);
}

IMediaContentRecord*
CMetakitContentManager::GetMediaRecord(IFileNameRef* file)
{
    return m_pImp->GetMediaRecord(file);
}

// Returns the number of playlist records in the content manager.
int
CMetakitContentManager::GetPlaylistRecordCount()
{
    return m_pImp->GetPlaylistRecordCount();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CMetakitContentManager::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    return m_pImp->AddPlaylistRecord(playlistRecord);
}

// Removes a playlist record from the content manager.
void
CMetakitContentManager::DeletePlaylistRecord(int iContentRecordID)
{
    return m_pImp->DeletePlaylistRecord(iContentRecordID);
}

// Removes all records from the given data source from the content manager.
void
CMetakitContentManager::DeleteRecordsFromDataSource(int iDataSourceID)
{
    return m_pImp->DeleteRecordsFromDataSource(iDataSourceID);
}

// Marks all records from the given data source as unverified.
void
CMetakitContentManager::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    return m_pImp->MarkRecordsFromDataSourceUnverified(iDataSourceID);
}

// Removes all records from the given data source that are marked as unverified.
void
CMetakitContentManager::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
{
    return m_pImp->DeleteUnverifiedRecordsFromDataSource(iDataSourceID);
}

// Returns the string of the artist with the given key.
// Returns 0 if no artist with a matching key was found.
const TCHAR*
CMetakitContentManager::GetArtistByKey(int iArtistKey) const
{
    return m_pImp->GetArtistByKey(iArtistKey);
}

const TCHAR*
CMetakitContentManager::GetAlbumByKey(int iAlbumKey) const
{
    return m_pImp->GetAlbumByKey(iAlbumKey);
}

const TCHAR*
CMetakitContentManager::GetGenreByKey(int iGenreKey) const
{
    return m_pImp->GetGenreByKey(iGenreKey);
}

int
CMetakitContentManager::GetArtistKey(const TCHAR* szArtist) const
{
    return m_pImp->GetArtistKey(szArtist);
}

int
CMetakitContentManager::GetAlbumKey(const TCHAR* szAlbum) const
{
    return m_pImp->GetAlbumKey(szAlbum);
}

int
CMetakitContentManager::GetGenreKey(const TCHAR* szGenre) const
{
    return m_pImp->GetGenreKey(szGenre);
}

void
CMetakitContentManager::GetAllMediaRecords(MediaRecordList& records) const
{
    return m_pImp->GetAllMediaRecords(records);
}

void
CMetakitContentManager::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    return m_pImp->GetMediaRecordsByDataSourceID(records, iDataSourceID);
}

void
CMetakitContentManager::GetMediaRecords(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    return m_pImp->GetMediaRecords(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
}

void
CMetakitContentManager::GetArtists(ContentKeyValueVector& keyValues,
    int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    return m_pImp->GetArtists(keyValues, iAlbumKey, iGenreKey, iDataSourceID);
}

void
CMetakitContentManager::GetAlbums(ContentKeyValueVector& keyValues,
    int iArtistKey, int iGenreKey, int iDataSourceID) const
{
    return m_pImp->GetAlbums(keyValues, iArtistKey, iGenreKey, iDataSourceID);
}

void
CMetakitContentManager::GetGenres(ContentKeyValueVector& keyValues,
    int iArtistKey, int iAlbumKey, int iDataSourceID) const
{
    return m_pImp->GetGenres(keyValues, iArtistKey, iAlbumKey, iDataSourceID);
}


void
CMetakitContentManager::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    return m_pImp->GetAllPlaylistRecords(records);
}

void
CMetakitContentManager::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    return m_pImp->GetPlaylistRecordsByDataSourceID(records, iDataSourceID);
}


c4_Storage* 
CMetakitContentManager::GetStorage()
{
    return m_pImp->GetStorage();
}