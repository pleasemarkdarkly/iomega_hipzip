//
// DJContentManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/content/djcontentmanager/DJContentManager.h>

#include "DJContentManagerImp.h"

#include <content/metadatatable/MetadataTable.h>
#include <util/debug/debug.h>

// If USE_SIMPLE_METADATA is defined then the CSimpleMetadata class will be used to generate
// temporary metadata records during scanning.  If not defined then the CDJContentManagerMetadata
// class is used.
//#define USE_SIMPLE_METADATA
#ifdef USE_SIMPLE_METADATA
#include <content/simplemetadata/SimpleMetadata.h>
#else   // USE_SIMPLE_METADATA
#include "DJContentManagerMetadata.h"
#endif  // USE_SIMPLE_METADATA

#include <stdio.h>  /* sprintf */
#include <stdlib.h> /* malloc/free */

DEBUG_MODULE_S(DJCONTENTMANAGER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DJCONTENTMANAGER);


//////////////////////////////////////////////////////////////////////////////////////////
//	CDJContentManager
//////////////////////////////////////////////////////////////////////////////////////////

CDJContentManager::CDJContentManager(const char* szFilename)
    : m_uiAutoCommitRecordCount(0)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        m_vDS[i] = 0;

#ifndef USE_SIMPLE_METADATA
    CDJContentManagerMetadata::AddAttribute(MDA_TITLE);
    CDJContentManagerMetadata::AddAttribute(MDA_ARTIST);
    CDJContentManagerMetadata::AddAttribute(MDA_ALBUM);
    CDJContentManagerMetadata::AddAttribute(MDA_GENRE);
    CDJContentManagerMetadata::AddAttribute(MDA_ALBUM_TRACK_NUMBER);
#endif  // USE_SIMPLE_METADATA

#ifdef USE_FIXED_ALLOCATOR
    CMetakitMediaContentRecord::InitMem();
#endif  // USE_FIXED_ALLOCATOR
}

CDJContentManager::CDJContentManager()
    : m_uiAutoCommitRecordCount(0)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        m_vDS[i] = 0;

#ifndef USE_SIMPLE_METADATA
    CDJContentManagerMetadata::AddAttribute(MDA_TITLE);
    CDJContentManagerMetadata::AddAttribute(MDA_ARTIST);
    CDJContentManagerMetadata::AddAttribute(MDA_ALBUM);
    CDJContentManagerMetadata::AddAttribute(MDA_GENRE);
    CDJContentManagerMetadata::AddAttribute(MDA_ALBUM_TRACK_NUMBER);
#endif  // USE_SIMPLE_METADATA

#ifdef USE_FIXED_ALLOCATOR
    CMetakitMediaContentRecord::InitMem();
#endif  // USE_FIXED_ALLOCATOR
}

CDJContentManager::~CDJContentManager()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        delete m_vDS[i];

#ifndef USE_SIMPLE_METADATA
    CDJContentManagerMetadata::RemoveAllAttributes();
#endif  // USE_SIMPLE_METADATA
}

bool
CDJContentManager::AddDataSource(int iDataSourceID, const char* szFilename)
{
    if (!iDataSourceID || (iDataSourceID >= MAX_DATA_SOURCE_COUNT) || m_vDS[iDataSourceID - 1])
        return false;

    m_vDS[iDataSourceID - 1] = new CDJContentManagerImp(this, szFilename);
    return true;
}

bool
CDJContentManager::AddDataSource(int iDataSourceID)
{
    if (!iDataSourceID || (iDataSourceID >= MAX_DATA_SOURCE_COUNT) || m_vDS[iDataSourceID - 1])
        return false;

    m_vDS[iDataSourceID - 1] = new CDJContentManagerImp(this);
    return true;
}

//! Sets the number of records that can be added to the content manager before triggering an
//! automatic commit.  This should reduce memory fragmentation.
//! Set a value of 0 to turn this feature off.
//! The default value is 0.
void
CDJContentManager::SetAutoCommitCount(unsigned int uiRecordCount)
{
    m_uiAutoCommitRecordCount = uiRecordCount;
}

//! Gets the next free media content record ID.
int
CDJContentManager::GetNextMediaContentRecordID()
{
    return CDJContentManagerImp::GetNextMediaContentRecordID();
}

// Creates an empty metadata record.
IMetadata*
CDJContentManager::CreateMetadataRecord() const
{
#ifdef USE_SIMPLE_METADATA
    return new CSimpleMetadata;
#else   // USE_SIMPLE_METADATA
    return new CDJContentManagerMetadata;
#endif  // USE_SIMPLE_METADATA
}

// Loads content records from a stream.
// Returns true if the state was loaded, false othersise.
bool
CDJContentManager::LoadStateFromStream(IInputStream* pInputStream)
{
    return false;
}

// Saves content records to a stream.
// Returns true if the state was saved, false othersise.
bool
CDJContentManager::SaveStateToStream(IOutputStream* pOutputStream)
{
    return false;
}

// Clears the content manager and deletes its content records.
void
CDJContentManager::Clear()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->Clear();
}

// Saves the database.
void
CDJContentManager::Commit()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->Commit();
}

//! Saves the database only if it's been changed since the last commit.
//! Returns true if the database was committed, false otherwise.
bool
CDJContentManager::CommitIfDirty()
{
    bool ret = false;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            ret = (m_vDS[i]->CommitIfDirty() || ret);

    return ret;
}

// Adds a list of media and playlist records to the content manager.
void
CDJContentManager::AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords = 0)
{
    if (pContentUpdate)
    {
        DBASSERT(DJCONTENTMANAGER, pContentUpdate->iDataSourceID, "Unspecified data source ID\n");
        DBASSERT(DJCONTENTMANAGER, pContentUpdate->iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", pContentUpdate->iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[pContentUpdate->iDataSourceID - 1], "Uninitialized data source: %d\n", pContentUpdate->iDataSourceID);
        m_vDS[pContentUpdate->iDataSourceID - 1]->AddContentRecords(pContentUpdate, pRecords);
    }
}

// Returns the number of media records in the content manager.
int
CDJContentManager::GetMediaRecordCount()
{
    int count = 0;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            count += m_vDS[i]->GetMediaRecordCount();
    return count;
}

//! Returns the number of media records in the content manager from a given data source.
int
CDJContentManager::GetMediaRecordCount(int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetMediaRecordCount();
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CDJContentManager::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
    DBASSERT(DJCONTENTMANAGER, mediaContent.iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, mediaContent.iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", mediaContent.iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[mediaContent.iDataSourceID - 1], "Uninitialized data source: %d\n", mediaContent.iDataSourceID);
    return m_vDS[mediaContent.iDataSourceID - 1]->AddMediaRecord(mediaContent, pbAlreadyExists);
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDJContentManager::DeleteMediaRecord(int iContentRecordID)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (m_vDS[i]->DeleteMediaRecord(iContentRecordID)))
            return true;
    return false;
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
// WARNING: The pContentRecord pointer will be invalid after this operation!
bool
CDJContentManager::DeleteMediaRecord(IMediaContentRecord* pContentRecord)
{
    DBASSERT(DJCONTENTMANAGER, pContentRecord->GetDataSourceID(), "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, pContentRecord->GetDataSourceID() <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", pContentRecord->GetDataSourceID());
    DBASSERT(DJCONTENTMANAGER, m_vDS[pContentRecord->GetDataSourceID() - 1], "Uninitialized data source: %d\n", pContentRecord->GetDataSourceID());
    return m_vDS[pContentRecord->GetDataSourceID() - 1]->DeleteMediaRecord(pContentRecord);
}

//! Removes the records in the list from the content manager.
//! All records in the list must come from the same data source.
//! When the function is done the records deleted from the front of the list will be gone.
//! \param nRecordsToDelete The number of records to delete from the front of the list.
//! \param pfnCB A callback function for monitoring progress.  It will be called 2 * records.Size() times.
//! \param pUserData Data to be passed to the callback function.
void
CDJContentManager::DeleteMediaRecords(MediaRecordList& records, int nRecordsToDelete, FNDeleteMediaRecordProgressCB* pfnCB, void* pUserData)
{
    if (!records.IsEmpty())
    {
        DBASSERT(DJCONTENTMANAGER, (*records.GetHead())->GetDataSourceID(), "Unspecified data source ID\n");
        DBASSERT(DJCONTENTMANAGER, (*records.GetHead())->GetDataSourceID() <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", (*records.GetHead())->GetDataSourceID());
        DBASSERT(DJCONTENTMANAGER, m_vDS[(*records.GetHead())->GetDataSourceID() - 1], "Uninitialized data source: %d\n", (*records.GetHead())->GetDataSourceID());
        m_vDS[(*records.GetHead())->GetDataSourceID() - 1]->DeleteMediaRecords(records, nRecordsToDelete, pfnCB, pUserData);
    }
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CDJContentManager::GetMediaRecord(int iContentRecordID)
{
    IMediaContentRecord* pRecord;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (pRecord = m_vDS[i]->GetMediaRecord(iContentRecordID)))
            return pRecord;
    return 0;
}

// Returns a pointer to the media content record with the specified URL.
// Returns 0 if no record with a matching URL was found.
IMediaContentRecord*
CDJContentManager::GetMediaRecord(const char* szURL)
{
    IMediaContentRecord* pRecord;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (pRecord = m_vDS[i]->GetMediaRecord(szURL)))
            return pRecord;
    return 0;
}

// Returns the number of playlist records in the content manager.
int
CDJContentManager::GetPlaylistRecordCount()
{
    int count = 0;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            count += m_vDS[i]->GetPlaylistRecordCount();
    return count;
}

//! Returns the number of playlist records in the content manager from a given data source.
int
CDJContentManager::GetPlaylistRecordCount(int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetPlaylistRecordCount();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CDJContentManager::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    DBASSERT(DJCONTENTMANAGER, playlistRecord.iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, playlistRecord.iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", playlistRecord.iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[playlistRecord.iDataSourceID - 1], "Uninitialized data source: %d\n", playlistRecord.iDataSourceID);
    return m_vDS[playlistRecord.iDataSourceID - 1]->AddPlaylistRecord(playlistRecord);
}

// Removes a playlist record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDJContentManager::DeletePlaylistRecord(int iContentRecordID)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (m_vDS[i]->DeletePlaylistRecord(iContentRecordID)))
            return true;
    return false;
}

// Removes all records from the given data source from the content manager.
void
CDJContentManager::DeleteRecordsFromDataSource(int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->Clear();
}

// Marks all records from the given data source as unverified.
void
CDJContentManager::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->MarkRecordsFromDataSourceUnverified(iDataSourceID);
}

// Removes all records from the given data source that are marked as unverified.
void
CDJContentManager::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->DeleteUnverifiedRecordsFromDataSource(iDataSourceID);
}

// Returns the string of the artist with the given key.
// Returns 0 if no artist with a matching key was found.
const TCHAR*
CDJContentManager::GetArtistByKey(int iArtistKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetArtistByKey(iArtistKey)))
            return tsz;
    return 0;
}

const TCHAR*
CDJContentManager::GetAlbumByKey(int iAlbumKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetAlbumByKey(iAlbumKey)))
            return tsz;
    return 0;
}

const TCHAR*
CDJContentManager::GetGenreByKey(int iGenreKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetGenreByKey(iGenreKey)))
            return tsz;
    return 0;
}

int
CDJContentManager::GetArtistKey(const TCHAR* szArtist) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetArtistKey(szArtist)))
            return key;
    return 0;
}

int
CDJContentManager::GetAlbumKey(const TCHAR* szAlbum) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetAlbumKey(szAlbum)))
            return key;
    return 0;
}

int
CDJContentManager::GetGenreKey(const TCHAR* szGenre) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetGenreKey(szGenre)))
            return key;
    return 0;
}

void
CDJContentManager::GetAllMediaRecords(MediaRecordList& records) const
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->GetAllMediaRecords(records);
}

void
CDJContentManager::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->GetMediaRecordsByDataSourceID(records, iDataSourceID);
}

void
CDJContentManager::GetMediaRecords(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetMediaRecords(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetMediaRecords(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDJContentManager::GetMediaRecordsSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID, int iSortNum, ...) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
            {
                va_list ap;
                va_start(ap, iSortNum);
                m_vDS[i]->GetMediaRecordsSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID, iSortNum, ap);
                va_end(ap);
            }
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
        {
            va_list ap;
            va_start(ap, iSortNum);
            m_vDS[iDataSourceID - 1]->GetMediaRecordsSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID, iSortNum, ap);
            va_end(ap);
        }
    }

}

void
CDJContentManager::GetMediaRecordsTitleSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetMediaRecordsTitleSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetMediaRecordsTitleSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDJContentManager::GetMediaRecordsAlbumSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetMediaRecordsAlbumSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetMediaRecordsAlbumSorted(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDJContentManager::GetArtists(ContentKeyValueVector& keyValues,
    int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetArtists(keyValues, iAlbumKey, iGenreKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetArtists(keyValues, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDJContentManager::GetAlbums(ContentKeyValueVector& keyValues,
    int iArtistKey, int iGenreKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetAlbums(keyValues, iArtistKey, iGenreKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetAlbums(keyValues, iArtistKey, iGenreKey, iDataSourceID);
    }
}

void
CDJContentManager::GetGenres(ContentKeyValueVector& keyValues,
    int iArtistKey, int iAlbumKey, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetGenres(keyValues, iArtistKey, iAlbumKey, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetGenres(keyValues, iArtistKey, iAlbumKey, iDataSourceID);
    }
}

//! Returns the number of artists in the content manager from a given data source.
int
CDJContentManager::GetArtistCount(int iDataSourceID) const
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetArtistCount();
}

//! Returns the number of albums in the content manager from a given data source.
int
CDJContentManager::GetAlbumCount(int iDataSourceID) const
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetAlbumCount();
}

//! Returns the number of genres in the content manager from a given data source.
int
CDJContentManager::GetGenreCount(int iDataSourceID) const
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetGenreCount();
}

//! Returns a pointer to the playlist content record with the specified URL in the given data source.
//! Returns 0 if no record with a matching URL was found.
IPlaylistContentRecord*
CDJContentManager::GetPlaylistRecord(const char* szURL, int iDataSourceID)
{
    DBASSERT(DJCONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    return m_vDS[iDataSourceID - 1]->GetPlaylistRecord(szURL);
}

void
CDJContentManager::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->GetAllPlaylistRecords(records);
}

void
CDJContentManager::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetPlaylistRecordsByDataSourceID(records, iDataSourceID);
    }
    else
    {
        DBASSERT(DJCONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DJCONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
           m_vDS[iDataSourceID - 1]->GetPlaylistRecordsByDataSourceID(records, iDataSourceID);
    }
}




