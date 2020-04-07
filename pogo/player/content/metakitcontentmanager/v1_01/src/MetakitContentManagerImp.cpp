//
// MetakitContentManagerImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "MetakitContentManagerImp.h"

#include <content/common/Metadata.h>
#include <content/metadatatable/MetadataTable.h>
#include <datasource/common/DataSource.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <playlist/common/Playlist.h>

#include <string.h> /* tstrncpy */
#include <stdlib.h> /* free */
#include <stdio.h>  /* sprintf */
#include <util/debug/debug.h>
#include <util/diag/diag.h>

DEBUG_USE_MODULE(METAKITCONTENTMANAGER);

//#define USE_BLOCKED

#ifdef USE_BLOCKED
static const char* sc_szTrackSchemaBase = "Tracks[_B[URL:S,ContentID:I,Title:W,ArtistID:I,AlbumID:I,GenreID:I,DataSourceID:I,CodecID:I,RecordPtr:I";
#else   // USE_BLOCKED
static const char* sc_szTrackSchemaBase = "Tracks[URL:S,ContentID:I,Title:W,ArtistID:I,AlbumID:I,GenreID:I,DataSourceID:I,CodecID:I,RecordPtr:I";
#endif  // USE_BLOCKED
static const char* sc_szArtistSchema = "Artists[ArtistID:I,Artist:W]";
static const char* sc_szAlbumSchema = "Albums[AlbumID:I,Album:W]";
static const char* sc_szGenreSchema = "Genres[GenreID:I,Genre:W]";
static const char* sc_szPlaylistSchema = "Playlists[URL:S,ContentID:I,DataSourceID:I,CodecID:I,RecordPtr:I]";

static const int sc_iUnknownArtistKey = 1;
static const TCHAR sc_szUnknownArtist[] = {'<','U','n','k','n','o','w','n','>',0};
static const int sc_iUnknownAlbumKey = 1;
static const TCHAR sc_szUnknownAlbum[] = {'<','U','n','k','n','o','w','n','>',0};
static const int sc_iUnknownGenreKey = 1;
static const TCHAR sc_szUnknownGenre[] = {'<','U','n','k','n','o','w','n','>',0};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitInternalMetadata
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitInternalMetadata : public IMetadata
{
public:

    CMetakitInternalMetadata();
    ~CMetakitInternalMetadata();

    // Add a metadata type to be used by all CMetakitInternalMetadata objects.
    static void AddAttribute(int iAttributeID);
    // Removes all metadata types from the list.
    static void RemoveAllAttributes();

    // IMetadata functions
    IMetadata* Copy() const;
    bool UsesAttribute(int iAttributeID) const;
    ERESULT SetAttribute(int iAttributeID, void* pAttributeValue);
    ERESULT UnsetAttribute(int iAttributeID);
    ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const;
    void ClearAttributes();

    void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false);

private:

typedef SimpleMap<int /* Attribute ID */, int /* Attribute type */> MetadataTypeMap;

    static MetadataTypeMap    sm_smMetadataToUse;

typedef struct md_value_holder_s
{
    int     iAttributeType;
    void*   pAttributeValue;
} md_value_holder_t;
typedef SimpleMap<int /* Attribute ID */, md_value_holder_t> MetadataValueMap;

    MetadataValueMap  m_smMetadataValues;

    ERESULT SetAttribute(int iAttributeID, int iAttributeType, void* pAttributeValue);
    // Frees any memory allocated to hold attribute values.
    // Doesn't clear the attribute list, though.
    void FreeAttributes();

};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitInputStream
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitInputStream : public c4_Stream
{
public:

	CMetakitInputStream(IInputStream* pInputStream)
        : m_pInputStream(pInputStream)
        { }
	~CMetakitInputStream()
        { }

    // Fetch some bytes sequentially
    int Read(void* Buffer, int Count)
    {
        return m_pInputStream->Read(Buffer, Count);
    }
    // Store some bytes sequentially
    bool Write(const void* Buffer, int Count)
    {
        return false;
    }


private:

    IInputStream*   m_pInputStream;
};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitOutputStream
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitOutputStream : public c4_Stream
{
public:

	CMetakitOutputStream(IOutputStream* pOutputStream)
        : m_pOutputStream(pOutputStream)
        { }
	~CMetakitOutputStream()
        { }

    // Fetch some bytes sequentially
    int Read(void* Buffer, int Count)
    {
        return 0;
    }
    // Store some bytes sequentially
    bool Write(const void* Buffer, int Count)
    {
        return m_pOutputStream->Write(Buffer, Count) == Count;
    }


private:

    IOutputStream*  m_pOutputStream;
};



//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitContentManagerImp
//////////////////////////////////////////////////////////////////////////////////////////

int CMetakitContentManagerImp::sm_iNextContentRecordID = 1;
int CMetakitContentManagerImp::sm_iNextArtistID = 2;
int CMetakitContentManagerImp::sm_iNextAlbumID = 2;
int CMetakitContentManagerImp::sm_iNextGenreID = 2;
int CMetakitContentManagerImp::sm_iNextPlaylistID = 1;

#ifdef INCREMENTAL_METAKIT
CMetakitContentManagerImp::CMetakitContentManagerImp(const char* szFilename)
    : m_mkStorage(szFilename, 1),
    m_pContentID ("ContentID"),
    m_pURL ("URL"),
    m_pTitle ("Title"),
    m_pArtistName ("Artist"),
    m_pArtistID ("ArtistID"),
    m_pAlbumName ("Album"),
    m_pAlbumID ("AlbumID"),
    m_pGenreName ("Genre"),
    m_pGenreID ("GenreID"),
    m_pDataSourceID ("DataSourceID"),
    m_pCodecID ("CodecID"),
    m_pRecordPtr ("RecordPtr"),
    m_szTrackSchema(0)
{
    MakeTrackSchema();

#ifdef USE_HASHED

    m_vTracksRaw = m_mkStorage.GetAs(m_szTrackSchema);
#ifdef USE_BLOCKED
    m_vTracksRaw = m_vTracksRaw.Blocked();
#endif  // USE_BLOCKED
    m_vTracksHash = m_mkStorage.GetAs("Tracks_H[_H:I,_R:I]");
    m_vTracks = m_vTracksRaw.Hash(m_vTracksHash, 1);

#else   // USE_HASHED

    m_vTracks = m_mkStorage.GetAs(m_szTrackSchema);
#ifdef USE_BLOCKED
    m_vTracks = m_vTracks.Blocked();
#endif  // USE_BLOCKED

#endif  // USE_HASHED

    m_vArtists = m_mkStorage.GetAs(sc_szArtistSchema);
    m_vAlbums = m_mkStorage.GetAs(sc_szAlbumSchema);
    m_vGenres = m_mkStorage.GetAs(sc_szGenreSchema);
    m_vPlaylists = m_mkStorage.GetAs(sc_szPlaylistSchema);

    // Find new next index values.
    for (int i = 0; i < m_vArtists.GetSize(); ++i)
        if ((int)m_pArtistID(m_vArtists[i]) >= sm_iNextArtistID)
            sm_iNextArtistID = ((int)m_pArtistID(m_vArtists[i])) + 1;
    for (int i = 0; i < m_vAlbums.GetSize(); ++i)
        if ((int)m_pAlbumID(m_vAlbums[i]) >= sm_iNextAlbumID)
            sm_iNextAlbumID = ((int)m_pAlbumID(m_vAlbums[i])) + 1;
    for (int i = 0; i < m_vGenres.GetSize(); ++i)
        if ((int)m_pGenreID(m_vGenres[i]) >= sm_iNextGenreID)
            sm_iNextGenreID = ((int)m_pGenreID(m_vGenres[i])) + 1;
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        if ((int)m_pContentID(m_vTracks[i]) >= sm_iNextContentRecordID)
            sm_iNextContentRecordID = ((int)m_pContentID(m_vTracks[i])) + 1;
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        if ((int)m_pContentID(m_vPlaylists[i]) >= sm_iNextPlaylistID)
            sm_iNextPlaylistID = ((int)m_pContentID(m_vPlaylists[i])) + 1;

    for (int i = 0; i < m_vTracks.GetSize(); ++i)
    {
        CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, (int)m_pContentID(m_vTracks[i]), (const char*)m_pURL(m_vTracks[i]), 0, false);
        m_pRecordPtr(m_vTracks[i]) = (int)pRecord;
    }

    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
    {
        CMetakitPlaylistContentRecord* pRecord = new CMetakitPlaylistContentRecord(this, (int)m_pContentID(m_vPlaylists[i]), false);
        m_pRecordPtr(m_vPlaylists[i]) = (int)pRecord;
    }

    // Add entries for unknown artist, album, and genre props, if they don't already exist.
    if (m_vArtists.Find(m_pArtistID[sc_iUnknownArtistKey]) == -1)
        m_vArtists.Add(m_pArtistName[sc_szUnknownArtist] + m_pArtistID[sc_iUnknownArtistKey]);
    if (m_vAlbums.Find(m_pAlbumID[sc_iUnknownAlbumKey]) == -1)
        m_vAlbums.Add(m_pAlbumName[sc_szUnknownAlbum] + m_pAlbumID[sc_iUnknownAlbumKey]);
    if (m_vGenres.Find(m_pGenreID[sc_iUnknownGenreKey]) == -1)
        m_vGenres.Add(m_pGenreName[sc_szUnknownGenre] + m_pGenreID[sc_iUnknownGenreKey]);

    CMetakitInternalMetadata::AddAttribute(MDA_TITLE);
    CMetakitInternalMetadata::AddAttribute(MDA_ARTIST);
    CMetakitInternalMetadata::AddAttribute(MDA_ALBUM);
    CMetakitInternalMetadata::AddAttribute(MDA_GENRE);
}

#else

CMetakitContentManagerImp::CMetakitContentManagerImp()
    : m_pContentID ("ContentID"),
    m_pURL ("URL"),
    m_pTitle ("Title"),
    m_pArtistName ("Artist"),
    m_pArtistID ("ArtistID"),
    m_pAlbumName ("Album"),
    m_pAlbumID ("AlbumID"),
    m_pGenreName ("Genre"),
    m_pGenreID ("GenreID"),
    m_pDataSourceID ("DataSourceID"),
    m_pCodecID ("CodecID"),
    m_pRecordPtr ("RecordPtr"),
    m_szTrackSchema(0)
{
    MakeTrackSchema();    
    m_vTracksRaw = m_mkStorage.GetAs(m_szTrackSchema);
    m_vTracksHash = m_mkStorage.GetAs("Tracks_H[_H:I,_R:I]");
    m_vTracks = m_vTracksRaw.Hash(m_vTracksHash, 1);
    m_vArtists = m_mkStorage.GetAs(sc_szArtistSchema);
    m_vAlbums = m_mkStorage.GetAs(sc_szAlbumSchema);
    m_vGenres = m_mkStorage.GetAs(sc_szGenreSchema);
    m_vPlaylists = m_mkStorage.GetAs(sc_szPlaylistSchema);

    m_vArtists.Add(m_pArtistName[sc_szUnknownArtist] + m_pArtistID[sc_iUnknownArtistKey]);
    m_vAlbums.Add(m_pAlbumName[sc_szUnknownAlbum] + m_pAlbumID[sc_iUnknownAlbumKey]);
    m_vGenres.Add(m_pGenreName[sc_szUnknownGenre] + m_pGenreID[sc_iUnknownGenreKey]);

    CMetakitInternalMetadata::AddAttribute(MDA_TITLE);
    CMetakitInternalMetadata::AddAttribute(MDA_ARTIST);
    CMetakitInternalMetadata::AddAttribute(MDA_ALBUM);
    CMetakitInternalMetadata::AddAttribute(MDA_GENRE);
}

#endif

CMetakitContentManagerImp::~CMetakitContentManagerImp()
{
    Clear();
    m_mkStorage.RemoveAll();

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
}

void
CMetakitContentManagerImp::MakeTrackSchema()
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
    free(m_szTrackSchema);
    m_szTrackSchema = (char*)malloc(iLen);
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

#ifdef USE_BLOCKED
    strcat(m_szTrackSchema, "]]");
#else   // USE_BLOCKED
    strcat(m_szTrackSchema, "]");
#endif  // USE_BLOCKED

}

ERESULT
CMetakitContentManagerImp::AddStoredMetadata(int iAttributeID, const void* pUnsetValue)
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

#ifdef USE_HASHED

    m_vTracksRaw = m_mkStorage.GetAs(m_szTrackSchema);
#ifdef USE_BLOCKED
    m_vTracksRaw = m_vTracksRaw.Blocked();
#endif  // USE_BLOCKED
    m_vTracksHash = m_mkStorage.GetAs("Tracks_H[_H:I,_R:I]");
    m_vTracks = m_vTracksRaw.Hash(m_vTracksHash, 1);

#else   // USE_HASHED

    m_vTracks = m_mkStorage.GetAs(m_szTrackSchema);
#ifdef USE_BLOCKED
    m_vTracks = m_vTracks.Blocked();
#endif  // USE_BLOCKED

#endif  // USE_HASHED

    CMetakitInternalMetadata::AddAttribute(iAttributeID);

    return MCM_NO_ERROR;
}

ERESULT
CMetakitContentManagerImp::AddUnstoredMetadata(int iAttributeID)
{
    CMetakitInternalMetadata::AddAttribute(iAttributeID);

    return MCM_NO_ERROR;
}


// Creates an empty metadata record.
IMetadata*
CMetakitContentManagerImp::CreateMetadataRecord() const
{
    return new CMetakitInternalMetadata;
}


// Loads content records from a stream.
// Returns true if the state was loaded, false othersise.
bool
CMetakitContentManagerImp::LoadStateFromStream(IInputStream* pInputStream)
{
    Clear();

    CMetakitInputStream mis(pInputStream);
    m_mkStorage.LoadFrom(mis);

    m_vTracksRaw = m_mkStorage.GetAs(m_szTrackSchema);
    m_vTracksHash = m_mkStorage.GetAs("Tracks_H[_H:I,_R:I]");
    m_vTracks = m_vTracksRaw.Hash(m_vTracksHash, 1);

    m_vArtists = m_mkStorage.GetAs(sc_szArtistSchema);
    m_vAlbums = m_mkStorage.GetAs(sc_szAlbumSchema);
    m_vGenres = m_mkStorage.GetAs(sc_szGenreSchema);
    m_vPlaylists = m_mkStorage.GetAs(sc_szPlaylistSchema);

    // Find new next index values.
    for (int i = 0; i < m_vArtists.GetSize(); ++i)
        if ((int)m_pArtistID(m_vArtists[i]) >= sm_iNextArtistID)
            sm_iNextArtistID = ((int)m_pArtistID(m_vArtists[i])) + 1;
    for (int i = 0; i < m_vAlbums.GetSize(); ++i)
        if ((int)m_pAlbumID(m_vAlbums[i]) >= sm_iNextAlbumID)
            sm_iNextAlbumID = ((int)m_pAlbumID(m_vAlbums[i])) + 1;
    for (int i = 0; i < m_vGenres.GetSize(); ++i)
        if ((int)m_pGenreID(m_vGenres[i]) >= sm_iNextGenreID)
            sm_iNextGenreID = ((int)m_pGenreID(m_vGenres[i])) + 1;
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        if ((int)m_pContentID(m_vTracks[i]) >= sm_iNextContentRecordID)
            sm_iNextContentRecordID = ((int)m_pContentID(m_vTracks[i])) + 1;
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        if ((int)m_pContentID(m_vPlaylists[i]) >= sm_iNextPlaylistID)
            sm_iNextPlaylistID = ((int)m_pContentID(m_vPlaylists[i])) + 1;

    for (int i = 0; i < m_vTracks.GetSize(); ++i)
    {
        CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, (int)m_pContentID(m_vTracks[i]), (const char*)m_pURL(m_vTracks[i]), 0, false);
        m_pRecordPtr(m_vTracks[i]) = (int)pRecord;
    }

    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
    {
        CMetakitPlaylistContentRecord* pRecord = new CMetakitPlaylistContentRecord(this, (int)m_pContentID(m_vPlaylists[i]), false);
        m_pRecordPtr(m_vPlaylists[i]) = (int)pRecord;
    }

    return true;
}

// Saves content records to a stream.
// Returns true if the state was saved, false othersise.
bool
CMetakitContentManagerImp::SaveStateToStream(IOutputStream* pOutputStream)
{
    CMetakitOutputStream mos(pOutputStream);
    m_mkStorage.SaveTo(mos);
    return true;
}

// Clears the content manager and deletes its content records.
void
CMetakitContentManagerImp::Clear()
{
print_mem_usage();
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]);
print_mem_usage();
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        delete (IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]);
print_mem_usage();

    m_vTracks.RemoveAll();
print_mem_usage();
    m_vArtists.RemoveAll();
    m_vAlbums.RemoveAll();
    m_vGenres.RemoveAll();
    m_vPlaylists.RemoveAll();

    // Add the unknown values for album, artist, genre
    m_vArtists.Add(m_pArtistName[sc_szUnknownArtist] + m_pArtistID[sc_iUnknownArtistKey]);
    m_vAlbums.Add(m_pAlbumName[sc_szUnknownAlbum] + m_pAlbumID[sc_iUnknownAlbumKey]);
    m_vGenres.Add(m_pGenreName[sc_szUnknownGenre] + m_pGenreID[sc_iUnknownGenreKey]);

    sm_iNextContentRecordID = 1;
    sm_iNextArtistID = 2;
    sm_iNextAlbumID = 2;
    sm_iNextGenreID = 2;
    sm_iNextPlaylistID = 1;
}

// Saves the database.
void
CMetakitContentManagerImp::Commit()
{
    m_mkStorage.Commit(true);
}

#define PROF
#ifdef PROF
#include <cyg/kernel/kapi.h>
#endif

// Adds a list of media and playlist records to the content manager.
void
CMetakitContentManagerImp::AddContentRecords(content_record_update_t* pContentUpdate)
{
#ifdef PROF
    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "AddContentRecords: start: %d ticks\n", tick);
#endif  // PROF
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "Current track database size: %d\n", m_vTracks.GetSize());

    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "Media content size: %d\n", pContentUpdate->media.Size());
    int i = 0;
    while (i < pContentUpdate->media.Size())
    {
        DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->media[i].szURL);
        bool pAlreadyExists;
        AddMediaRecord(pContentUpdate->media[i], &pAlreadyExists);
        if (!pContentUpdate->bTwoPass || pAlreadyExists)
        {
            free(pContentUpdate->media[i].szURL);
            pContentUpdate->media.Remove(i);
        }
        else
            ++i;
    }
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "Playlist content size: %d\n", pContentUpdate->media.Size());
    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
    {
        DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "URL %d: %s\n", i, pContentUpdate->playlists[i].szURL);
        AddPlaylistRecord(pContentUpdate->playlists[i]);
        free(pContentUpdate->playlists[i].szURL);
    }
    pContentUpdate->playlists.Clear();

    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "New track database size: %d\n", m_vTracks.GetSize());

#ifdef PROF
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "AddContentRecords: %d ticks\n", cyg_current_time() - tick);
#endif  // PROF
//    Commit();
}

// Returns the number of media records in the content manager.
int
CMetakitContentManagerImp::GetMediaRecordCount()
{
    return m_vTracks.GetSize();
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CMetakitContentManagerImp::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
    int iRes = m_vTracks.Find(m_pURL[mediaContent.szURL]);
    if (iRes != -1)
    {
        IMediaContentRecord* pRecord = (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]);
        if (pRecord)
        {
            // This record already exists in the content manager, but if the new record is verified
            // as existing on a data source then mark the record inside the content manager as the same.
            if (!pRecord->IsVerified())
                pRecord->SetVerified(mediaContent.bVerified);
            // Merge any new metadata from the content update.
            if (mediaContent.pMetadata)
            {
                pRecord->MergeAttributes(mediaContent.pMetadata, true);
                delete mediaContent.pMetadata;
                mediaContent.pMetadata = 0;
            }
            if (pbAlreadyExists)
                *pbAlreadyExists = true;
            return pRecord;
        }
    }

    int iTrackID = sm_iNextContentRecordID++;
    CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, iTrackID,
        mediaContent.szURL, mediaContent.pMetadata, mediaContent.bVerified);

    c4_Row rTrack;
    m_pURL(rTrack) = mediaContent.szURL;
    int iArtistKey, iAlbumKey, iGenreKey;
    if (mediaContent.pMetadata)
    {
        TCHAR* szValue;
        if (SUCCEEDED(mediaContent.pMetadata->GetAttribute(MDA_ARTIST, (void**)&szValue)))
        {
            iArtistKey = AddArtist(szValue);
            // Free the memory in the original metadata record.
            mediaContent.pMetadata->UnsetAttribute(MDA_ARTIST);
        }
        else
            iArtistKey = AddArtist(0);
        if (SUCCEEDED(mediaContent.pMetadata->GetAttribute(MDA_ALBUM, (void**)&szValue)))
        {
            iAlbumKey = AddAlbum(szValue);
            // Free the memory in the original metadata record.
            mediaContent.pMetadata->UnsetAttribute(MDA_ALBUM);
        }
        else
            iAlbumKey = AddAlbum(0);
        if (SUCCEEDED(mediaContent.pMetadata->GetAttribute(MDA_GENRE, (void**)&szValue)))
        {
            iGenreKey = AddGenre(szValue);
            // Free the memory in the original metadata record.
            mediaContent.pMetadata->UnsetAttribute(MDA_GENRE);
        }
        else
            iGenreKey = AddGenre(0);
        if (SUCCEEDED(mediaContent.pMetadata->GetAttribute(MDA_TITLE, (void**)&szValue)))
        {
            m_pTitle(rTrack) = szValue;
            // Free the memory in the original metadata record.
            mediaContent.pMetadata->UnsetAttribute(MDA_TITLE);
        }
    }
    else
    {
        iArtistKey = AddArtist(0);
        iAlbumKey = AddAlbum(0);
        iGenreKey = AddGenre(0);
    }

    for (int i = 0; i < m_propMap.Size(); ++i)
    {
        md_property_holder_t prop;
        int key;
        if (m_propMap.GetEntry(i, &key, &prop))
        {
            switch (prop.iAttributeType)
            {
                case MDT_TCHAR:
                {
                    TCHAR* szValue;
                    if (mediaContent.pMetadata && SUCCEEDED(mediaContent.pMetadata->GetAttribute(prop.iAttributeID, (void**)&szValue)))
                    {
                        (*((c4_TStringProp*)prop.pProp))(rTrack) = szValue;
                        // Free the memory in the original metadata record.
                        mediaContent.pMetadata->UnsetAttribute(prop.iAttributeID);
                    }
                    else
                        (*((c4_TStringProp*)prop.pProp))(rTrack) = (TCHAR*)prop.pUnsetValue;
                    break;
                }
                case MDT_INT:
                {
                    int iValue;
                    if (mediaContent.pMetadata && SUCCEEDED(mediaContent.pMetadata->GetAttribute(prop.iAttributeID, (void**)&iValue)))
                    {
                        (*((c4_IntProp*)prop.pProp))(rTrack) = iValue;
                    }
                    else
                        (*((c4_IntProp*)prop.pProp))(rTrack) = (int)prop.pUnsetValue;
                    break;
                }

                default:
                    break;
            }
        }
    }


    m_pArtistID(rTrack) = iArtistKey;
    m_pAlbumID(rTrack) = iAlbumKey;
    m_pGenreID(rTrack) = iGenreKey;
    m_pDataSourceID(rTrack) = mediaContent.iDataSourceID;
    m_pCodecID(rTrack) = mediaContent.iCodecID;

    m_pContentID(rTrack) = iTrackID;
    m_pRecordPtr(rTrack) = (int)pRecord;

    m_vTracks.Add(rTrack);

    if (pbAlreadyExists)
        *pbAlreadyExists = false;

    return pRecord;
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CMetakitContentManagerImp::DeleteMediaRecord(int iContentRecordID)
{
    c4_Row rTrack;
    m_pContentID(rTrack) = iContentRecordID;
    int iRes = m_vTracks.Find(rTrack);
    if (iRes != -1)
    {
        int iArtistID = m_pArtistID(m_vTracks[iRes]);
        int iAlbumID = m_pAlbumID(m_vTracks[iRes]);
        int iGenreID = m_pGenreID(m_vTracks[iRes]);
        delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]);

        m_vTracks.RemoveAt(iRes);

        FlushArtist(iArtistID);
        FlushAlbum(iAlbumID);
        FlushGenre(iGenreID);

        return true;
    }
    return false;
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CMetakitContentManagerImp::GetMediaRecord(int iContentRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iContentRecordID]);
    return iRes != -1 ? (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]) : 0;
}

// Returns a pointer to the media content record with the specified URL.
// Returns 0 if no record with a matching URL was found.
IMediaContentRecord*
CMetakitContentManagerImp::GetMediaRecord(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]) : 0;
}

// Returns the number of playlist records in the content manager.
int
CMetakitContentManagerImp::GetPlaylistRecordCount()
{
    return m_vPlaylists.GetSize();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CMetakitContentManagerImp::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    int iRes = m_vPlaylists.Find(m_pURL[playlistRecord.szURL]);
    if (iRes != -1)
    {
        IPlaylistContentRecord* pRecord = (IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[iRes]);
        if (pRecord)
        {
            if (!pRecord->IsVerified())
                pRecord->SetVerified(playlistRecord.bVerified);
            return pRecord;
        }
    }

    int iPlaylistID = sm_iNextPlaylistID++;
    CMetakitPlaylistContentRecord* pRecord = new CMetakitPlaylistContentRecord(this, iPlaylistID, playlistRecord.bVerified);

    c4_Row rTrack;
    m_pURL(rTrack) = playlistRecord.szURL;
    m_pDataSourceID(rTrack) = playlistRecord.iDataSourceID;
    m_pCodecID(rTrack) = playlistRecord.iPlaylistFormatID;

    m_pContentID(rTrack) = iPlaylistID;
    m_pRecordPtr(rTrack) = (int)pRecord;

    m_vPlaylists.Add(rTrack);

    return pRecord;
}

// Removes a playlist record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CMetakitContentManagerImp::DeletePlaylistRecord(int iContentRecordID)
{
    c4_Row rTrack;
    m_pContentID(rTrack) = iContentRecordID;
    int iRes = m_vPlaylists.Find(rTrack);
    if (iRes != -1)
    {
        delete (IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[iRes]);
        m_vPlaylists.RemoveAt(iRes);
        return true;
    }
    return false;
}

// Removes all records from the given data source from the content manager.
void
CMetakitContentManagerImp::DeleteRecordsFromDataSource(int iDataSourceID)
{
    int i = 0, start = -1, count = 0;
    SimpleVector<int> artistIDs, albumIDs, genreIDs;
    while (i < m_vTracks.GetSize())
    {
        if (m_pDataSourceID(m_vTracks[i]) == iDataSourceID)
        {
            if (start == -1)
                start = i;
            ++count;
            int iArtistID = m_pArtistID(m_vTracks[i]);
            int iAlbumID = m_pAlbumID(m_vTracks[i]);
            int iGenreID = m_pGenreID(m_vTracks[i]);
            delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]);

//            m_vTracks.RemoveAt(i);

            if ((iArtistID != sc_iUnknownAlbumKey) && (artistIDs.Find(iArtistID) == -1))
                artistIDs.PushBack(iArtistID);
            if ((iAlbumID != sc_iUnknownAlbumKey) && (albumIDs.Find(iAlbumID) == -1))
                albumIDs.PushBack(iAlbumID);
            if ((iGenreID != sc_iUnknownAlbumKey) && (genreIDs.Find(iGenreID) == -1))
                genreIDs.PushBack(iGenreID);
            ++i;
        }
        else
        {
            if (start != -1)
            {
                m_vTracks.RemoveAt(start, count);
                i = start;
                start = -1;
            }
            else
                ++i;
        }
    }
    if (start != -1)
    {
        m_vTracks.RemoveAt(start, count);
    }

    while (!artistIDs.IsEmpty())
        FlushArtist(artistIDs.PopBack());
    while (!albumIDs.IsEmpty())
        FlushAlbum(albumIDs.PopBack());
    while (!genreIDs.IsEmpty())
        FlushGenre(genreIDs.PopBack());

    i = 0;
    while (i < m_vPlaylists.GetSize())
    {
        if (m_pDataSourceID(m_vPlaylists[i]) == iDataSourceID)
            DeletePlaylistRecord(m_pContentID(m_vPlaylists[i]));
        else
            ++i;
    }
}

// Marks all records from the given data source as unverified.
void
CMetakitContentManagerImp::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    c4_View vDS = m_vTracks.Select(m_pDataSourceID[iDataSourceID]);
    for (int i = 0; i < vDS.GetSize(); ++i)
        ((IMediaContentRecord*)(int)m_pRecordPtr(vDS[i]))->SetVerified(false);
}

// Removes all records from the given data source that are marked as unverified.
void
CMetakitContentManagerImp::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
{
    int i = 0;
    SimpleVector<int> artistIDs, albumIDs, genreIDs;
    while (i < m_vTracks.GetSize())
    {
        if ((m_pDataSourceID(m_vTracks[i]) == iDataSourceID) &&
            !(((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]))->IsVerified()))
        {
            int iArtistID = m_pArtistID(m_vTracks[i]);
            int iAlbumID = m_pAlbumID(m_vTracks[i]);
            int iGenreID = m_pGenreID(m_vTracks[i]);
            delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]);

            m_vTracks.RemoveAt(i);

            if ((iArtistID != sc_iUnknownAlbumKey) && (artistIDs.Find(iArtistID) == -1))
                artistIDs.PushBack(iArtistID);
            if ((iAlbumID != sc_iUnknownAlbumKey) && (albumIDs.Find(iAlbumID) == -1))
                albumIDs.PushBack(iAlbumID);
            if ((iGenreID != sc_iUnknownAlbumKey) && (genreIDs.Find(iGenreID) == -1))
                genreIDs.PushBack(iGenreID);
        }
        else
            ++i;
    }
    while (!artistIDs.IsEmpty())
        FlushArtist(artistIDs.PopBack());
    while (!albumIDs.IsEmpty())
        FlushAlbum(albumIDs.PopBack());
    while (!genreIDs.IsEmpty())
        FlushGenre(genreIDs.PopBack());

    i = 0;
    while (i < m_vPlaylists.GetSize())
    {
        if ((m_pDataSourceID(m_vPlaylists[i]) == iDataSourceID) &&
            !(((IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]))->IsVerified()))
        {
            DeletePlaylistRecord(m_pContentID(m_vPlaylists[i]));
        }
        else
            ++i;
    }
}

// Returns the string of the artist with the given key.
// Returns 0 if no artist with a matching key was found.
const TCHAR*
CMetakitContentManagerImp::GetArtistByKey(int iArtistKey) const
{
    int iRes = m_vArtists.Find(m_pArtistID[iArtistKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pArtistName(m_vArtists[iRes]);
}

const TCHAR*
CMetakitContentManagerImp::GetAlbumByKey(int iAlbumKey) const
{
    int iRes = m_vAlbums.Find(m_pAlbumID[iAlbumKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pAlbumName(m_vAlbums[iRes]);
}

const TCHAR*
CMetakitContentManagerImp::GetGenreByKey(int iGenreKey) const
{
    int iRes = m_vGenres.Find(m_pGenreID[iGenreKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pGenreName(m_vGenres[iRes]);
}


// Adds an artist to the table, if it isn't already in there.
// Returns the artist key.
int
CMetakitContentManagerImp::AddArtist(const TCHAR* szArtist)
{
    if (!szArtist || (szArtist[0] == '\0'))
        return sc_iUnknownArtistKey;

    int iArtistKey = GetArtistKey(szArtist);
    if (iArtistKey)
        return iArtistKey;

    c4_Row rArtist;
    m_pArtistName(rArtist) = szArtist;
    int iArtistID = sm_iNextArtistID++;
    m_pArtistID(rArtist) = iArtistID;
    m_vArtists.Add(rArtist);

    return iArtistID;
}

int
CMetakitContentManagerImp::AddAlbum(const TCHAR* szAlbum)
{
    if (!szAlbum || (szAlbum[0] == '\0'))
        return sc_iUnknownAlbumKey;

    int iAlbumKey = GetAlbumKey(szAlbum);
    if (iAlbumKey)
        return iAlbumKey;

    c4_Row rAlbum;
    m_pAlbumName(rAlbum) = szAlbum;
    int iAlbumID = sm_iNextAlbumID++;
    m_pAlbumID(rAlbum) = iAlbumID;
    m_vAlbums.Add(rAlbum);

    return iAlbumID;
}

int
CMetakitContentManagerImp::AddGenre(const TCHAR* szGenre)
{
    if (!szGenre || (szGenre[0] == '\0'))
        return sc_iUnknownGenreKey;

    int iGenreKey = GetGenreKey(szGenre);
    if (iGenreKey)
        return iGenreKey;

    c4_Row rGenre;
    m_pGenreName(rGenre) = szGenre;
    int iGenreID = sm_iNextGenreID++;
    m_pGenreID(rGenre) = iGenreID;
    m_vGenres.Add(rGenre);

    return iGenreID;
}

int
CMetakitContentManagerImp::GetArtistKey(const TCHAR* szArtist) const
{
    int iRes = m_vArtists.Find(m_pArtistName[szArtist]);
    return iRes != -1 ? m_pArtistID(m_vArtists[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetAlbumKey(const TCHAR* szAlbum) const
{
    int iRes = m_vAlbums.Find(m_pAlbumName[szAlbum]);
    return iRes != -1 ? m_pAlbumID(m_vAlbums[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetGenreKey(const TCHAR* szGenre) const
{
    int iRes = m_vGenres.Find(m_pGenreName[szGenre]);
    return iRes != -1 ? m_pGenreID(m_vGenres[iRes]) : 0;
}

void
CMetakitContentManagerImp::GetAllMediaRecords(MediaRecordList& records) const
{
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]));
}

void
CMetakitContentManagerImp::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        if (m_pDataSourceID(m_vTracks[i]) == iDataSourceID)
            records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]));
}

void
CMetakitContentManagerImp::GetMediaRecords(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;
    if (iDataSourceID != CMK_ALL)
        m_pDataSourceID(rFind) = iDataSourceID;

    c4_View vFind = m_vTracks.Select(rFind);
    for (int i = 0; i < vFind.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(vFind[i]));
}

void
CMetakitContentManagerImp::GetArtists(ContentKeyValueVector& keyValues,
    int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;
    if (iDataSourceID != CMK_ALL)
        m_pDataSourceID(rFind) = iDataSourceID;

    c4_View vMediaProj(m_pArtistID);

    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetArtists: start: %d ticks\n", tick);
    c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vArtists);
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetArtists: select: %d ticks\n", cyg_current_time() - tick);
    for (int i = 0; i < vFind.GetSize(); ++i)
    {
        cm_key_value_record_t rec = {
            iKey : m_pArtistID(vFind[i]),
            szValue : m_pArtistName(vFind[i])
        };
        keyValues.PushBack(rec);
    }
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetArtists: build array: %d ticks\n", cyg_current_time() - tick);
}

void
CMetakitContentManagerImp::GetAlbums(ContentKeyValueVector& keyValues,
    int iArtistKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;
    if (iDataSourceID != CMK_ALL)
        m_pDataSourceID(rFind) = iDataSourceID;

    c4_View vMediaProj(m_pAlbumID);

    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetAlbums: start: %d ticks\n", tick);
    c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vAlbums);
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetAlbums: select: %d ticks\n", cyg_current_time() - tick);
    for (int i = 0; i < vFind.GetSize(); ++i)
    {
        cm_key_value_record_t rec = {
            iKey : m_pAlbumID(vFind[i]),
            szValue : m_pAlbumName(vFind[i])
        };

        keyValues.PushBack(rec);
    }
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetAlbums: build array: %d ticks\n", cyg_current_time() - tick);
}

void
CMetakitContentManagerImp::GetGenres(ContentKeyValueVector& keyValues,
    int iArtistKey, int iAlbumKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iDataSourceID != CMK_ALL)
        m_pDataSourceID(rFind) = iDataSourceID;

    c4_View vMediaProj(m_pGenreID);

    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetGenres: start: %d ticks\n", tick);
    c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vGenres);
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetGenres: select: %d ticks\n", cyg_current_time() - tick);
    for (int i = 0; i < vFind.GetSize(); ++i)
    {
        cm_key_value_record_t rec = {
            iKey : m_pGenreID(vFind[i]),
            szValue : m_pGenreName(vFind[i])
        };

        keyValues.PushBack(rec);
    }
    DEBUGP(METAKITCONTENTMANAGER, DBGLEV_INFO, "GetGenres: build array: %d ticks\n", cyg_current_time() - tick);
}


void
CMetakitContentManagerImp::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        records.PushBack((IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]));
}

void
CMetakitContentManagerImp::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        if (m_pDataSourceID(m_vPlaylists[i]) == iDataSourceID)
            records.PushBack((IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]));
}

#ifdef USE_HASHED

const char*
CMetakitContentManagerImp::GetMediaRecordURL(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pURL(m_vTracks[iRes]) : (char*)0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordTitle(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pTitle(m_vTracks[iRes]) : (TCHAR*)0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordArtist(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        return GetArtistByKey(m_pArtistID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordAlbum(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        return GetAlbumByKey(m_pAlbumID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordGenre(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        return GetGenreByKey(m_pGenreID(m_vTracks[iRes]));
    else
        return 0;
}


int
CMetakitContentManagerImp::GetMediaRecordArtistKey(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pArtistID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordAlbumKey(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pAlbumID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordGenreKey(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pGenreID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordDataSourceID(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pDataSourceID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordCodecID(const char* szURL)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    return iRes != -1 ? m_pCodecID(m_vTracks[iRes]) : 0;
}

ERESULT
CMetakitContentManagerImp::GetMediaRecordAttribute(const char* szURL, int iAttributeID, void** ppAttributeValue)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes == -1)
        return MCM_ERROR;

    md_property_holder_t prop;
    if (m_propMap.FindEntry(iAttributeID, &prop))
    {
        switch (prop.iAttributeType)
        {
            case MDT_TCHAR:
            {
                const TCHAR* tszValue = (*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]);
                if (tstrcmp(tszValue, (TCHAR*)prop.pUnsetValue))
                {
//                    *ppAttributeValue = (void*)(const TCHAR*)(*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]);
                    *ppAttributeValue = (void*)tszValue;
                    return METADATA_NO_ERROR;
                }
                else
                    return METADATA_NO_VALUE_SET;
            }

            case MDT_INT:
            {
                int iValue = (*((c4_IntProp*)prop.pProp))(m_vTracks[iRes]);
                if (iValue != (int)prop.pUnsetValue)
                {
                    *ppAttributeValue = (void*)iValue;
                    return METADATA_NO_ERROR;
                }
                else
                    return METADATA_NO_VALUE_SET;
            }

            default:
                return MCM_UNKNOWN_METADATA_TYPE;
        }
    }
    return METADATA_NOT_USED;
}

void
CMetakitContentManagerImp::SetMediaRecordURL(int iRecordID, const char* szURL)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pURL(m_vTracks[iRes]) = szURL;
}

void
CMetakitContentManagerImp::SetMediaRecordTitle(const char* szURL, const TCHAR* szTitle)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        m_pTitle(m_vTracks[iRes]) = szTitle;
}

void
CMetakitContentManagerImp::SetMediaRecordArtist(const char* szURL, const TCHAR* szArtist)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
    {
        int iArtistID = AddArtist(szArtist);
        int iOldArtistID = m_pArtistID(m_vTracks[iRes]);
        if (iArtistID != iOldArtistID)
        {
            m_pArtistID(m_vTracks[iRes]) = iArtistID;
            // If there are no more tracks by this artist, then remove that artist record.
            FlushArtist(iOldArtistID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordAlbum(const char* szURL, const TCHAR* szAlbum)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
    {
        int iAlbumID = AddAlbum(szAlbum);
        int iOldAlbumID = m_pAlbumID(m_vTracks[iRes]);
        if (iAlbumID != iOldAlbumID)
        {
            m_pAlbumID(m_vTracks[iRes]) = iAlbumID;
            // If there are no more tracks on this album, then remove that album record.
            FlushAlbum(iOldAlbumID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordGenre(const char* szURL, const TCHAR* szGenre)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
    {
        int iGenreID = AddGenre(szGenre);
        int iOldGenreID = m_pGenreID(m_vTracks[iRes]);
        if (iGenreID != iOldGenreID)
        {
            m_pGenreID(m_vTracks[iRes]) = iGenreID;
            // If there are no more tracks in this genre, then remove that genre record.
            FlushGenre(iOldGenreID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordDataSourceID(const char* szURL, int iDataSourceID)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        m_pDataSourceID(m_vTracks[iRes]) = iDataSourceID;
}

void
CMetakitContentManagerImp::SetMediaRecordCodecID(const char* szURL, int iCodecID)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes != -1)
        m_pCodecID(m_vTracks[iRes]) = iCodecID;
}

ERESULT
CMetakitContentManagerImp::SetMediaRecordAttribute(const char* szURL, int iAttributeID, void* pAttributeValue)
{
    int iRes = m_vTracks.Find(m_pURL[szURL]);
    if (iRes == -1)
        return MCM_ERROR;

    md_property_holder_t prop;
    if (m_propMap.FindEntry(iAttributeID, &prop))
    {
        switch (prop.iAttributeType)
        {
            case MDT_TCHAR:
                (*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]) = (TCHAR*)pAttributeValue;
                return METADATA_NO_ERROR;

            case MDT_INT:
            {
                (*((c4_IntProp*)prop.pProp))(m_vTracks[iRes]) = (int)pAttributeValue;
                return METADATA_NO_ERROR;
            }

            default:
                return MCM_UNKNOWN_METADATA_TYPE;
        }
    }

    return METADATA_NOT_USED;
}

#else   // USE_HASHED

const char*
CMetakitContentManagerImp::GetMediaRecordURL(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pURL(m_vTracks[iRes]) : (char*)0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordTitle(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pTitle(m_vTracks[iRes]) : (TCHAR*)0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordArtist(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        return GetArtistByKey(m_pArtistID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordAlbum(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        return GetAlbumByKey(m_pAlbumID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CMetakitContentManagerImp::GetMediaRecordGenre(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        return GetGenreByKey(m_pGenreID(m_vTracks[iRes]));
    else
        return 0;
}


int
CMetakitContentManagerImp::GetMediaRecordArtistKey(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pArtistID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordAlbumKey(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pAlbumID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordGenreKey(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pGenreID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordDataSourceID(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pDataSourceID(m_vTracks[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetMediaRecordCodecID(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pCodecID(m_vTracks[iRes]) : 0;
}

ERESULT
CMetakitContentManagerImp::GetMediaRecordAttribute(int iRecordID, int iAttributeID, void** ppAttributeValue)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes == -1)
        return MCM_ERROR;

    md_property_holder_t prop;
    if (m_propMap.FindEntry(iAttributeID, &prop))
    {
        switch (prop.iAttributeType)
        {
            case MDT_TCHAR:
            {
                const TCHAR* tszValue = (*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]);
                if (tstrcmp(tszValue, (TCHAR*)prop.pUnsetValue))
                {
//                    *ppAttributeValue = (void*)(const TCHAR*)(*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]);
                    *ppAttributeValue = (void*)tszValue;
                    return METADATA_NO_ERROR;
                }
                else
                    return METADATA_NO_VALUE_SET;
            }

            case MDT_INT:
            {
                int iValue = (*((c4_IntProp*)prop.pProp))(m_vTracks[iRes]);
                if (iValue != (int)prop.pUnsetValue)
                {
                    *ppAttributeValue = (void*)iValue;
                    return METADATA_NO_ERROR;
                }
                else
                    return METADATA_NO_VALUE_SET;
            }

            default:
                return MCM_UNKNOWN_METADATA_TYPE;
        }
    }
    return METADATA_NOT_USED;
}

void
CMetakitContentManagerImp::SetMediaRecordURL(int iRecordID, const char* szURL)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pURL(m_vTracks[iRes]) = szURL;
}

void
CMetakitContentManagerImp::SetMediaRecordTitle(int iRecordID, const TCHAR* szTitle)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pTitle(m_vTracks[iRes]) = szTitle;
}

void
CMetakitContentManagerImp::SetMediaRecordArtist(int iRecordID, const TCHAR* szArtist)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
    {
        int iArtistID = AddArtist(szArtist);
        int iOldArtistID = m_pArtistID(m_vTracks[iRes]);
        if (iArtistID != iOldArtistID)
        {
            m_pArtistID(m_vTracks[iRes]) = iArtistID;
            // If there are no more tracks by this artist, then remove that artist record.
            FlushArtist(iOldArtistID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordAlbum(int iRecordID, const TCHAR* szAlbum)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
    {
        int iAlbumID = AddAlbum(szAlbum);
        int iOldAlbumID = m_pAlbumID(m_vTracks[iRes]);
        if (iAlbumID != iOldAlbumID)
        {
            m_pAlbumID(m_vTracks[iRes]) = iAlbumID;
            // If there are no more tracks on this album, then remove that album record.
            FlushAlbum(iOldAlbumID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordGenre(int iRecordID, const TCHAR* szGenre)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
    {
        int iGenreID = AddGenre(szGenre);
        int iOldGenreID = m_pGenreID(m_vTracks[iRes]);
        if (iGenreID != iOldGenreID)
        {
            m_pGenreID(m_vTracks[iRes]) = iGenreID;
            // If there are no more tracks in this genre, then remove that genre record.
            FlushGenre(iOldGenreID);
        }
    }
}

void
CMetakitContentManagerImp::SetMediaRecordDataSourceID(int iRecordID, int iDataSourceID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pDataSourceID(m_vTracks[iRes]) = iDataSourceID;
}

void
CMetakitContentManagerImp::SetMediaRecordCodecID(int iRecordID, int iCodecID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pCodecID(m_vTracks[iRes]) = iCodecID;
}

ERESULT
CMetakitContentManagerImp::SetMediaRecordAttribute(int iRecordID, int iAttributeID, void* pAttributeValue)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    if (iRes == -1)
        return MCM_ERROR;

    md_property_holder_t prop;
    if (m_propMap.FindEntry(iAttributeID, &prop))
    {
        switch (prop.iAttributeType)
        {
            case MDT_TCHAR:
                (*((c4_TStringProp*)prop.pProp))(m_vTracks[iRes]) = (TCHAR*)pAttributeValue;
                return METADATA_NO_ERROR;

            case MDT_INT:
            {
                (*((c4_IntProp*)prop.pProp))(m_vTracks[iRes]) = (int)pAttributeValue;
                return METADATA_NO_ERROR;
            }

            default:
                return MCM_UNKNOWN_METADATA_TYPE;
        }
    }

    return METADATA_NOT_USED;
}

#endif  // USE_HASHED

const char*
CMetakitContentManagerImp::GetPlaylistRecordURL(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pURL(m_vPlaylists[iRes]) : (char*)0;
}

int
CMetakitContentManagerImp::GetPlaylistRecordDataSourceID(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pDataSourceID(m_vPlaylists[iRes]) : 0;
}

int
CMetakitContentManagerImp::GetPlaylistRecordPlaylistFormatID(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pCodecID(m_vPlaylists[iRes]) : 0;
}

void
CMetakitContentManagerImp::SetPlaylistRecordURL(int iRecordID, const char* szURL)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pURL(m_vPlaylists[iRes]) = szURL;
}

void
CMetakitContentManagerImp::SetPlaylistRecordDataSourceID(int iRecordID, int iDataSourceID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pDataSourceID(m_vPlaylists[iRes]) = iDataSourceID;
}

void
CMetakitContentManagerImp::SetPlaylistRecordPlaylistFormatID(int iRecordID, int iPlaylistFormatID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pCodecID(m_vPlaylists[iRes]) = iPlaylistFormatID;
}

// If there are no more tracks by this artist, then remove that artist record.
void
CMetakitContentManagerImp::FlushArtist(int iArtistKey)
{
    if (iArtistKey != sc_iUnknownArtistKey)
    {
        int iRes = m_vTracks.Find(m_pArtistID[iArtistKey]);
        if (iRes == -1)
        {
            iRes = m_vArtists.Find(m_pArtistID[iArtistKey]);
            if (iRes != -1)
                m_vArtists.RemoveAt(iRes);
        }
    }
}

void
CMetakitContentManagerImp::FlushAlbum(int iAlbumKey)
{
    if (iAlbumKey != sc_iUnknownAlbumKey)
    {
        int iRes = m_vTracks.Find(m_pAlbumID[iAlbumKey]);
        if (iRes == -1)
        {
            iRes = m_vAlbums.Find(m_pAlbumID[iAlbumKey]);
            if (iRes != -1)
                m_vAlbums.RemoveAt(iRes);
        }
    }
}

void
CMetakitContentManagerImp::FlushGenre(int iGenreKey)
{
    if (iGenreKey != sc_iUnknownGenreKey)
    {
        int iRes = m_vTracks.Find(m_pGenreID[iGenreKey]);
        if (iRes == -1)
        {
            iRes = m_vGenres.Find(m_pGenreID[iGenreKey]);
            if (iRes != -1)
                m_vGenres.RemoveAt(iRes);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

CMetakitMediaContentRecord::CMetakitMediaContentRecord(CMetakitContentManagerImp* pManager, int iContentRecordID,
        const char* szURL, IMetadata* pMetadata, bool bVerified)
    : m_iID(iContentRecordID),
    m_pManager(pManager),
    m_iDataSourceID(0),
    m_iCodecID(0),
    m_bVerified(bVerified),
    m_eStatus(CR_OKAY)
{
    m_pMetadata = pMetadata ? pMetadata : pManager->CreateMetadataRecord();
    m_szURL = new char[strlen(szURL) + 1];
    strcpy(m_szURL, szURL);
}

CMetakitMediaContentRecord::~CMetakitMediaContentRecord()
{
    delete m_pMetadata;
    delete [] m_szURL;
}

const char*
CMetakitMediaContentRecord::GetURL() const
{
#ifdef USE_HASHED
    return m_szURL;
#else   // USE_HASHED
    return m_pManager->GetMediaRecordURL(GetID());
#endif  // USE_HASHED
}

const TCHAR*
CMetakitMediaContentRecord::GetTitle() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordTitle(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordTitle(GetID());
#endif  // USE_HASHED
}

const TCHAR*
CMetakitMediaContentRecord::GetArtist() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordArtist(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordArtist(GetID());
#endif  // USE_HASHED
}

const TCHAR*
CMetakitMediaContentRecord::GetAlbum() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordAlbum(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordAlbum(GetID());
#endif  // USE_HASHED
}

const TCHAR*
CMetakitMediaContentRecord::GetGenre() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordGenre(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordGenre(GetID());
#endif  // USE_HASHED
}

int
CMetakitMediaContentRecord::GetDataSourceID() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordDataSourceID(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordDataSourceID(GetID());
#endif  // USE_HASHED
}

int
CMetakitMediaContentRecord::GetCodecID() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordCodecID(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordCodecID(GetID());
#endif  // USE_HASHED
}

int
CMetakitMediaContentRecord::GetArtistKey() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordArtistKey(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordArtistKey(GetID());
#endif  // USE_HASHED
}

int
CMetakitMediaContentRecord::GetAlbumKey() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordAlbumKey(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordAlbumKey(GetID());
#endif  // USE_HASHED
}

int
CMetakitMediaContentRecord::GetGenreKey() const
{
#ifdef USE_HASHED
    return m_pManager->GetMediaRecordGenreKey(m_szURL);
#else   // USE_HASHED
    return m_pManager->GetMediaRecordGenreKey(GetID());
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetTitle(const TCHAR* szTitle)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordTitle(m_szURL, szTitle);
#else   // USE_HASHED
    m_pManager->SetMediaRecordTitle(GetID(), szTitle);
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetAlbum(const TCHAR* szAlbum)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordAlbum(m_szURL, szAlbum);
#else   // USE_HASHED
    m_pManager->SetMediaRecordAlbum(GetID(), szAlbum);
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetGenre(const TCHAR* szGenre)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordGenre(m_szURL, szGenre);
#else   // USE_HASHED
    m_pManager->SetMediaRecordGenre(GetID(), szGenre);
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetArtist(const TCHAR* szArtist)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordArtist(m_szURL, szArtist);
#else   // USE_HASHED
    m_pManager->SetMediaRecordArtist(GetID(), szArtist);
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetDataSourceID(int iDataSourceID)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordDataSourceID(m_szURL, iDataSourceID);
#else   // USE_HASHED
    m_pManager->SetMediaRecordDataSourceID(GetID(), iDataSourceID);
#endif  // USE_HASHED
}

void
CMetakitMediaContentRecord::SetCodecID(int iCodecID)
{
#ifdef USE_HASHED
    m_pManager->SetMediaRecordCodecID(m_szURL, iCodecID);
#else   // USE_HASHED
    m_pManager->SetMediaRecordCodecID(GetID(), iCodecID);
#endif  // USE_HASHED
}

IMetadata*
CMetakitMediaContentRecord::Copy() const
{
    IMetadata* pCopy = m_pManager->CreateMetadataRecord();
    pCopy->MergeAttributes(this, true);
    return pCopy;
}


bool
CMetakitMediaContentRecord::UsesAttribute(int iAttributeID) const
{
    if ((iAttributeID == MDA_TITLE) ||
            (iAttributeID == MDA_ARTIST) ||
            (iAttributeID == MDA_ALBUM) ||
            (iAttributeID == MDA_GENRE))
        return true;
    return m_pMetadata->UsesAttribute(iAttributeID);
}

ERESULT
CMetakitMediaContentRecord::SetAttribute(int iAttributeID, void* pAttributeValue)
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
            SetTitle((TCHAR*)pAttributeValue);
            return METADATA_NO_ERROR;

        case MDA_ARTIST:
            SetArtist((TCHAR*)pAttributeValue);
            return METADATA_NO_ERROR;

        case MDA_ALBUM:
            SetAlbum((TCHAR*)pAttributeValue);
            return METADATA_NO_ERROR;

        case MDA_GENRE:
            SetGenre((TCHAR*)pAttributeValue);
            return METADATA_NO_ERROR;

        default:
#ifdef USE_HASHED
            if (FAILED(m_pManager->SetMediaRecordAttribute(m_szURL, iAttributeID, pAttributeValue)))
#else   // USE_HASHED
            if (FAILED(m_pManager->SetMediaRecordAttribute(m_iID, iAttributeID, pAttributeValue)))
#endif  // USE_HASHED
                return m_pMetadata->SetAttribute(iAttributeID, pAttributeValue);
            else
                return METADATA_NO_ERROR;
    }
}

ERESULT
CMetakitMediaContentRecord::GetAttribute(int iAttributeID, void** ppAttributeValue) const
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
        {
            *ppAttributeValue = (void*)GetTitle();
            if (tstrlen((TCHAR*)*ppAttributeValue))
                return METADATA_NO_ERROR;
            else
                return METADATA_NO_VALUE_SET;
        }

        case MDA_ARTIST:
        {
            *ppAttributeValue = (void*)GetArtist();
            return METADATA_NO_ERROR;
        }

        case MDA_ALBUM:
        {
            *ppAttributeValue = (void*)GetAlbum();
            return METADATA_NO_ERROR;
        }

        case MDA_GENRE:
        {
            *ppAttributeValue = (void*)GetGenre();
            return METADATA_NO_ERROR;
        }

        default:
        {
#ifdef USE_HASHED
            if (FAILED(m_pManager->GetMediaRecordAttribute(m_szURL, iAttributeID, ppAttributeValue)))
#else   // USE_HASHED
            if (FAILED(m_pManager->GetMediaRecordAttribute(m_iID, iAttributeID, ppAttributeValue)))
#endif  // USE_HASHED
                return m_pMetadata->GetAttribute(iAttributeID, ppAttributeValue);
            else
                return METADATA_NO_ERROR;
        }
    }
}

void
CMetakitMediaContentRecord::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite)
{
    TCHAR* tszValue;
    if (SUCCEEDED(pMetadata->GetAttribute(MDA_TITLE, (void**)&tszValue)))
        SetTitle(tszValue);
    if ((bOverwrite || (GetArtistKey() == sc_iUnknownArtistKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_ARTIST, (void**)&tszValue)))
        SetArtist(tszValue);
    if ((bOverwrite || (GetAlbumKey() == sc_iUnknownAlbumKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM, (void**)&tszValue)))
        SetAlbum(tszValue);
    if ((bOverwrite || (GetGenreKey() == sc_iUnknownGenreKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_GENRE, (void**)&tszValue)))
        SetGenre(tszValue);
    for (int i = 0; i < m_pManager->m_propMap.Size(); ++i)
    {
        CMetakitContentManagerImp::md_property_holder_t prop;
        int iAttributeID;
        if (m_pManager->m_propMap.GetEntry(i, &iAttributeID, &prop))
        {
            void* pOriginalAttributeValue, *pAttributeValue;
#ifdef USE_HASHED
            if ((bOverwrite || FAILED(m_pManager->GetMediaRecordAttribute(m_szURL, iAttributeID, &pOriginalAttributeValue))) && SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pAttributeValue)))
                m_pManager->SetMediaRecordAttribute(m_szURL, iAttributeID, pAttributeValue);
#else   // USE_HASHED
            if ((bOverwrite || FAILED(m_pManager->GetMediaRecordAttribute(m_iID, iAttributeID, &pOriginalAttributeValue))) && SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pAttributeValue)))
//            if (SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pAttributeValue)))
                m_pManager->SetMediaRecordAttribute(m_iID, iAttributeID, pAttributeValue);
#endif  // USE_HASHED
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////


CMetakitPlaylistContentRecord::CMetakitPlaylistContentRecord(CMetakitContentManagerImp* pManager, int iContentRecordID, bool bVerified)
    : m_iID(iContentRecordID),
    m_pManager(pManager),
    m_iPlaylistFormatID(0),
    m_bVerified(bVerified),
    m_eStatus(CR_OKAY)
{
}

CMetakitPlaylistContentRecord::~CMetakitPlaylistContentRecord()
{
}

const char*
CMetakitPlaylistContentRecord::GetURL() const
{
    return m_pManager->GetPlaylistRecordURL(GetID());
}

void
CMetakitPlaylistContentRecord::SetDataSourceID(int iDataSourceID)
{
    m_pManager->SetPlaylistRecordDataSourceID(GetID(), iDataSourceID);
}

int
CMetakitPlaylistContentRecord::GetDataSourceID() const
{
    return m_pManager->GetPlaylistRecordDataSourceID(GetID());
}

void
CMetakitPlaylistContentRecord::SetPlaylistFormatID(int iPlaylistFormatID)
{
    m_pManager->SetPlaylistRecordPlaylistFormatID(GetID(), iPlaylistFormatID);
}

int
CMetakitPlaylistContentRecord::GetPlaylistFormatID() const
{
    return m_pManager->GetPlaylistRecordPlaylistFormatID(GetID());
}









////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////





















CMetakitInternalMetadata::MetadataTypeMap CMetakitInternalMetadata::sm_smMetadataToUse;

CMetakitInternalMetadata::CMetakitInternalMetadata()
{
}

CMetakitInternalMetadata::~CMetakitInternalMetadata()
{
    FreeAttributes();
}

// Add a metadata type to be used by all CMetakitInternalMetadata objects.
void CMetakitInternalMetadata::AddAttribute(int iAttributeID)
{
    int iAttributeType = CMetadataTable::GetInstance()->FindMetadataTypeByID(iAttributeID);
    if (iAttributeType != MDT_INVALID_TYPE)
    {
        sm_smMetadataToUse.AddEntry(iAttributeID, iAttributeType);
    }
}

// Removes all metadata types from the list.
void CMetakitInternalMetadata::RemoveAllAttributes()
{
    sm_smMetadataToUse.Clear();
}

// IMetadata functions
IMetadata* CMetakitInternalMetadata::Copy() const
{
    CMetakitInternalMetadata* pCopy = new CMetakitInternalMetadata;
    pCopy->MergeAttributes(this, true);
    return pCopy;
}

bool CMetakitInternalMetadata::UsesAttribute(int iAttributeID) const
{
    int iAttributeType;
    if (sm_smMetadataToUse.FindEntry(iAttributeID, &iAttributeType))
        return true;
    else
        return false;
}

ERESULT CMetakitInternalMetadata::SetAttribute(int iAttributeID, void* pAttributeValue)
{
    int iAttributeType;
    if (sm_smMetadataToUse.FindEntry(iAttributeID, &iAttributeType))
        return SetAttribute(iAttributeID, iAttributeType, pAttributeValue);
    else
        return METADATA_NOT_USED;
}

ERESULT CMetakitInternalMetadata::UnsetAttribute(int iAttributeID)
{
    md_value_holder_t mdValue;
    if (m_smMetadataValues.RemoveEntryByKey(iAttributeID, &mdValue))
    {
        if (mdValue.iAttributeType == MDT_TCHAR)
            free(mdValue.pAttributeValue);
        return METADATA_NO_ERROR;
    }
    else
    {
        // No value found, so check if the value is not used or not set.
        if (sm_smMetadataToUse.FindEntry(iAttributeID, &mdValue.iAttributeType))
            return METADATA_NO_VALUE_SET;
        else
            return METADATA_NOT_USED;
    }
}

ERESULT CMetakitInternalMetadata::GetAttribute(int iAttributeID, void** ppAttributeValue) const
{
    md_value_holder_t mdValue;
    if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
    {
        *ppAttributeValue = mdValue.pAttributeValue;
        return METADATA_NO_ERROR;
    }
    else
    {
        // No value found, so check if the value is not used or not set.
        if (sm_smMetadataToUse.FindEntry(iAttributeID, &mdValue.iAttributeType))
            return METADATA_NO_VALUE_SET;
        else
            return METADATA_NOT_USED;
    }
}

void CMetakitInternalMetadata::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false)
{
    for (int i = 0; i < sm_smMetadataToUse.Size(); ++i)
    {
        int iAttributeID;
        int iAttributeType;
        if (sm_smMetadataToUse.GetEntry(i, &iAttributeID, &iAttributeType))
        {
            // If we're not in overwrite mode, then check to see if a value exists before setting.
            if (!bOverwrite)
            {
                md_value_holder_t mdValue;
                if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
                    continue;
            }

            void* pAttributeValue;
            if (SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pAttributeValue)))
                SetAttribute(iAttributeID, iAttributeType, pAttributeValue);
        }
    }
}

void CMetakitInternalMetadata::ClearAttributes()
{
    FreeAttributes();
    m_smMetadataValues.Clear();
}

ERESULT CMetakitInternalMetadata::SetAttribute(int iAttributeID, int iAttributeType, void* pAttributeValue)
{
    switch (iAttributeType)
    {
        case MDT_INT:
        {
            // No memory is allocated to store this value, so go ahead and write
            // a record without checking to see if a previous value has already been stored.
            md_value_holder_t mdValue;
            mdValue.iAttributeType = iAttributeType;
            mdValue.pAttributeValue = pAttributeValue;
            m_smMetadataValues.AddEntry(iAttributeID, mdValue);
            return METADATA_NO_ERROR;
        }

        case MDT_TCHAR:
        {
            // If a value for this entry has already been stored, then first free that memory.
            md_value_holder_t mdValue;
            if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
                free(mdValue.pAttributeValue);
            else
                mdValue.iAttributeType = iAttributeType;
            mdValue.pAttributeValue = (void*)tstrdup((TCHAR*)pAttributeValue);
            m_smMetadataValues.AddEntry(iAttributeID, mdValue);
            return METADATA_NO_ERROR;
        }
    }
    return METADATA_ERROR;
}

// Frees any memory allocated to hold attribute values.
// Doesn't clear the attribute list, though.
void CMetakitInternalMetadata::FreeAttributes()
{
    // Free any stored strings.
    for (int i = 0; i < m_smMetadataValues.Size(); ++i)
    {
        int key;
        md_value_holder_t mdValue;
        if (m_smMetadataValues.GetEntry(i, &key, &mdValue) && (mdValue.iAttributeType == MDT_TCHAR))
            free(mdValue.pAttributeValue);
    }
}



