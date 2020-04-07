//
// DataSourceContentManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/content/datasourcecontentmanager/DataSourceContentManager.h>

#include "DataSourceContentManagerImp.h"
#include "DataSourceContentManagerMetadata.h"

#include <content/metadatatable/MetadataTable.h>
#include <util/debug/debug.h>

#include <stdio.h>  /* sprintf */
#include <stdlib.h> /* malloc/free */

DEBUG_MODULE_S(DATASOURCECONTENTMANAGER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DATASOURCECONTENTMANAGER);


static const char* sc_szTrackSchemaBase = "Tracks[URL:S,ContentID:I,Title:W,ArtistID:I,AlbumID:I,GenreID:I,DataSourceID:I,CodecID:I,RecordPtr:I";

//////////////////////////////////////////////////////////////////////////////////////////
//	CDataSourceContentManager
//////////////////////////////////////////////////////////////////////////////////////////

CDataSourceContentManager::CDataSourceContentManager(const char* szFilename)
    : m_szTrackSchema(0)
{
    MakeTrackSchema();
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        m_vDS[i] = 0;

    CDataSourceContentManagerMetadata::AddAttribute(MDA_TITLE);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_ARTIST);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_ALBUM);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_GENRE);
}

CDataSourceContentManager::CDataSourceContentManager()
    : m_szTrackSchema(0)
{
    MakeTrackSchema();
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        m_vDS[i] = 0;

    CDataSourceContentManagerMetadata::AddAttribute(MDA_TITLE);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_ARTIST);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_ALBUM);
    CDataSourceContentManagerMetadata::AddAttribute(MDA_GENRE);
}

CDataSourceContentManager::~CDataSourceContentManager()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        delete m_vDS[i];

    for (int i = 0; i < m_propMap.Size(); ++i)
    {
        md_property_holder_t prop;
        int key;
        if (m_propMap.GetEntry(i, &key, &prop))
        {
            delete prop.pProp;
            free(prop.szAttributeName);
            if (prop.iAttributeType == MDT_TCHAR)
                free(prop.pUnsetValue);
        }
    }
    delete [] m_szTrackSchema;
    CDataSourceContentManagerMetadata::RemoveAllAttributes();
}

bool
CDataSourceContentManager::AddDataSource(int iDataSourceID, const char* szFilename)
{
    if (!iDataSourceID || (iDataSourceID >= MAX_DATA_SOURCE_COUNT) || m_vDS[iDataSourceID - 1])
        return false;

    m_vDS[iDataSourceID - 1] = new CDataSourceContentManagerImp(this, szFilename);
    return true;
}

bool
CDataSourceContentManager::AddDataSource(int iDataSourceID)
{
    if (!iDataSourceID || (iDataSourceID >= MAX_DATA_SOURCE_COUNT) || m_vDS[iDataSourceID - 1])
        return false;

    m_vDS[iDataSourceID - 1] = new CDataSourceContentManagerImp(this);
    return true;
}

ERESULT
CDataSourceContentManager::AddStoredMetadata(int iAttributeID, const void* pUnsetValue)
{
    int iAttributeType = CMetadataTable::GetInstance()->FindMetadataTypeByID(iAttributeID);
    if (iAttributeType == MDT_INVALID_TYPE)
        return MCM_UNKNOWN_METADATA_ID;

    c4_Property* pNewProp;
    void* pUnsetValueCopy;
    char* szName = (char*)malloc(8 /* "UserProp" */ + 4 /* ID */ + 1);
    sprintf(szName, "UserProp%04d", iAttributeID);
    switch (iAttributeType)
    {
        case MDT_INT:
            pNewProp = new c4_IntProp(szName);
            pUnsetValueCopy = const_cast<void*>(pUnsetValue);
            break;

        case MDT_TCHAR:
            pNewProp = new c4_TStringProp(szName);
            pUnsetValueCopy = tstrdup((TCHAR*)pUnsetValue);
            break;

        default:
            free(szName);
            return MCM_UNKNOWN_METADATA_TYPE;
    }

    md_property_holder_t newProp(iAttributeID, iAttributeType, pUnsetValueCopy, szName, pNewProp);
    m_propMap.AddEntry(iAttributeID, newProp);
    MakeTrackSchema();

    CDataSourceContentManagerMetadata::AddAttribute(iAttributeID);

    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->AddStoredMetadata(iAttributeID, pUnsetValue);

    return MCM_NO_ERROR;

}

// Creates an empty metadata record.
IMetadata*
CDataSourceContentManager::CreateMetadataRecord() const
{
    return new CDataSourceContentManagerMetadata;
}

// Loads content records from a stream.
// Returns true if the state was loaded, false othersise.
bool
CDataSourceContentManager::LoadStateFromStream(IInputStream* pInputStream)
{
    return false;
}

// Saves content records to a stream.
// Returns true if the state was saved, false othersise.
bool
CDataSourceContentManager::SaveStateToStream(IOutputStream* pOutputStream)
{
    return false;
}

// Clears the content manager and deletes its content records.
void
CDataSourceContentManager::Clear()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->Clear();
}

// Saves the database.
void
CDataSourceContentManager::Commit()
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->Commit();
}

//! Saves the database with the given data source ID.
void
CDataSourceContentManager::CommitFromDataSource(int iDataSourceID)
{
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->Commit();
}

// Adds a list of media and playlist records to the content manager.
void
CDataSourceContentManager::AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords = 0)
{
    if (pContentUpdate)
    {
        DBASSERT(DATASOURCECONTENTMANAGER, pContentUpdate->iDataSourceID, "Unspecified data source ID\n");
        DBASSERT(DATASOURCECONTENTMANAGER, pContentUpdate->iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", pContentUpdate->iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[pContentUpdate->iDataSourceID - 1], "Uninitialized data source: %d\n", pContentUpdate->iDataSourceID);
        m_vDS[pContentUpdate->iDataSourceID - 1]->AddContentRecords(pContentUpdate, pRecords);
    }
}

// Returns the number of media records in the content manager.
int
CDataSourceContentManager::GetMediaRecordCount()
{
    int count = 0;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            count += m_vDS[i]->GetMediaRecordCount();
    return count;
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CDataSourceContentManager::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
    DBASSERT(DATASOURCECONTENTMANAGER, mediaContent.iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, mediaContent.iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", mediaContent.iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[mediaContent.iDataSourceID - 1], "Uninitialized data source: %d\n", mediaContent.iDataSourceID);
    return m_vDS[mediaContent.iDataSourceID - 1]->AddMediaRecord(mediaContent, pbAlreadyExists);
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDataSourceContentManager::DeleteMediaRecord(int iContentRecordID)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (m_vDS[i]->DeleteMediaRecord(iContentRecordID)))
            return true;
    return false;
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CDataSourceContentManager::GetMediaRecord(int iContentRecordID)
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
CDataSourceContentManager::GetMediaRecord(const char* szURL)
{
    IMediaContentRecord* pRecord;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (pRecord = m_vDS[i]->GetMediaRecord(szURL)))
            return pRecord;
    return 0;
}

// Returns the number of playlist records in the content manager.
int
CDataSourceContentManager::GetPlaylistRecordCount()
{
    int count = 0;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            count += m_vDS[i]->GetPlaylistRecordCount();
    return count;
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CDataSourceContentManager::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    DBASSERT(DATASOURCECONTENTMANAGER, playlistRecord.iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, playlistRecord.iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", playlistRecord.iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[playlistRecord.iDataSourceID - 1], "Uninitialized data source: %d\n", playlistRecord.iDataSourceID);
    return m_vDS[playlistRecord.iDataSourceID - 1]->AddPlaylistRecord(playlistRecord);
}

// Removes a playlist record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDataSourceContentManager::DeletePlaylistRecord(int iContentRecordID)
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (m_vDS[i]->DeletePlaylistRecord(iContentRecordID)))
            return true;
    return false;
}

// Removes all records from the given data source from the content manager.
void
CDataSourceContentManager::DeleteRecordsFromDataSource(int iDataSourceID)
{
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->Clear();
}

// Marks all records from the given data source as unverified.
void
CDataSourceContentManager::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->MarkRecordsFromDataSourceUnverified(iDataSourceID);
}

// Removes all records from the given data source that are marked as unverified.
void
CDataSourceContentManager::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
{
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->DeleteUnverifiedRecordsFromDataSource(iDataSourceID);
}

// Returns the string of the artist with the given key.
// Returns 0 if no artist with a matching key was found.
const TCHAR*
CDataSourceContentManager::GetArtistByKey(int iArtistKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetArtistByKey(iArtistKey)))
            return tsz;
    return 0;
}

const TCHAR*
CDataSourceContentManager::GetAlbumByKey(int iAlbumKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetAlbumByKey(iAlbumKey)))
            return tsz;
    return 0;
}

const TCHAR*
CDataSourceContentManager::GetGenreByKey(int iGenreKey) const
{
    const TCHAR* tsz;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (tsz = m_vDS[i]->GetGenreByKey(iGenreKey)))
            return tsz;
    return 0;
}

int
CDataSourceContentManager::GetArtistKey(const TCHAR* szArtist) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetArtistKey(szArtist)))
            return key;
    return 0;
}

int
CDataSourceContentManager::GetAlbumKey(const TCHAR* szAlbum) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetAlbumKey(szAlbum)))
            return key;
    return 0;
}

int
CDataSourceContentManager::GetGenreKey(const TCHAR* szGenre) const
{
    int key;
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i] && (key = m_vDS[i]->GetGenreKey(szGenre)))
            return key;
    return 0;
}

void
CDataSourceContentManager::GetAllMediaRecords(MediaRecordList& records) const
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->GetAllMediaRecords(records);
}

void
CDataSourceContentManager::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID , "Unspecified data source ID\n");
    DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
    DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
    m_vDS[iDataSourceID - 1]->GetMediaRecordsByDataSourceID(records, iDataSourceID);
}

void
CDataSourceContentManager::GetMediaRecords(MediaRecordList& records,
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
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetMediaRecords(records, iArtistKey, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDataSourceContentManager::GetMediaRecordsSorted(MediaRecordList& records,
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
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
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
CDataSourceContentManager::GetArtists(ContentKeyValueVector& keyValues,
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
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetArtists(keyValues, iAlbumKey, iGenreKey, iDataSourceID);
    }
}

void
CDataSourceContentManager::GetAlbums(ContentKeyValueVector& keyValues,
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
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetAlbums(keyValues, iArtistKey, iGenreKey, iDataSourceID);
    }
}

void
CDataSourceContentManager::GetGenres(ContentKeyValueVector& keyValues,
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
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
            m_vDS[iDataSourceID - 1]->GetGenres(keyValues, iArtistKey, iAlbumKey, iDataSourceID);
    }
}


void
CDataSourceContentManager::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
        if (m_vDS[i])
            m_vDS[i]->GetAllPlaylistRecords(records);
}

void
CDataSourceContentManager::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    if (!iDataSourceID)
    {
        for (int i = 0; i < MAX_DATA_SOURCE_COUNT; ++i)
            if (m_vDS[i])
                m_vDS[i]->GetPlaylistRecordsByDataSourceID(records, iDataSourceID);
    }
    else
    {
        DBASSERT(DATASOURCECONTENTMANAGER, iDataSourceID <= MAX_DATA_SOURCE_COUNT, "Data source ID %d out-of-range\n", iDataSourceID);
        DBASSERT(DATASOURCECONTENTMANAGER, m_vDS[iDataSourceID - 1], "Uninitialized data source: %d\n", iDataSourceID);
        if (m_vDS[iDataSourceID - 1])
           m_vDS[iDataSourceID - 1]->GetPlaylistRecordsByDataSourceID(records, iDataSourceID);
    }
}
















void
CDataSourceContentManager::MakeTrackSchema()
{
    // Calculate length
    int iLen = strlen(sc_szTrackSchemaBase) + 1 /* ] */ + 1;
    for (int i = 0; i < m_propMap.Size(); ++i)
    {
        md_property_holder_t prop;
        int key;
        if (m_propMap.GetEntry(i, &key, &prop))
        {
            iLen += strlen(prop.szAttributeName) + 3 /* :x, */ + 1;
        }
    }

    // Build the new schema
    delete [] m_szTrackSchema;
    m_szTrackSchema = new char[iLen];
    memset((void*)m_szTrackSchema, 0, iLen);
    strcpy(m_szTrackSchema, sc_szTrackSchemaBase);
    for (int i = 0; i < m_propMap.Size(); ++i)
    {
        md_property_holder_t prop;
        int key;
        if (m_propMap.GetEntry(i, &key, &prop))
        {
            strcat(m_szTrackSchema, ",");
            strcat(m_szTrackSchema, prop.szAttributeName);
            strcat(m_szTrackSchema, ":");
            m_szTrackSchema[strlen(m_szTrackSchema)] = prop.pProp->Type();
        }
    }

    strcat(m_szTrackSchema, "]");

}
