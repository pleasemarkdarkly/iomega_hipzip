//
// DJContentManagerImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "DJContentManagerImp.h"
#include "DJContentManagerMetadata.h"

#include <content/common/Metadata.h>
#include <datasource/common/DataSource.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <playlist/common/Playlist.h>

#include <string.h> /* tstrncpy */
#include <stdlib.h> /* free */
#include <util/debug/debug.h>
#include <util/diag/diag.h>

#include <ctype.h>  /* tolower */

// Used for converting URLs to temporary track titles.
#include <main/main/DJPlayerState.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <main/main/FatHelper.h>
#include <main/main/IsoHelper.h>
#include <main/main/ProgressWatcher.h>

// If PROFILE_UPDATES is defined, then times are printed for content addition and deletion.
//#define PROFILE_UPDATES

// If PROFILE_QUERIES is defined, then times are printed for content queries.
//#define PROFILE_QUERIES

// If PROFILE_COMMIT is defined, then the time is printed for committing the database.
//#define PROFILE_COMMIT

// If PROFILE_DELETE is defined, then the time is printed for deleting batches of media records.
//#define PROFILE_DELETE

#if defined(PROFILE_UPDATES) || defined(PROFILE_QUERIES) || defined(PROFILE_COMMIT)
#include <cyg/kernel/kapi.h>
#endif



DEBUG_USE_MODULE(DJCONTENTMANAGER);

static const char* sc_szArtistSchema = "Artists[Artist:W,ArtistID:I]";
static const char* sc_szAlbumSchema = "Albums[Album:W,AlbumID:I]";
static const char* sc_szGenreSchema = "Genres[Genre:W,GenreID:I]";
static const char* sc_szTrackSchema = "Tracks[URL:S,ContentID:I,Title:W,ArtistID:I,AlbumID:I,GenreID:I,AlbumTrackNumber:I,DataSourceID:I,CodecID:I,RecordPtr:I]";
static const char* sc_szPlaylistSchema = "Playlists[URL:S,ContentID:I,DataSourceID:I,CodecID:I,RecordPtr:I]";
static const char* sc_szURLSortSchema = "URLSort[URL:S,SortedIndex:I]";
static const char* sc_szTitleSortSchema = "TitleSort[Title:W,SortedIndex:I,ArtistID:I,AlbumID:I,GenreID:I]";
static const char* sc_szAlbumSortSchema = "AlbumSort[AlbumID:I,AlbumTrackNumber:I,Title:W,SortedIndex:I,ArtistID:I,GenreID:I]";

static const int sc_iUnknownArtistKey = 1;
static const TCHAR sc_szUnknownArtist[] = {'U','n','k','n','o','w','n',' ','A','r','t','i','s','t',0};
static const int sc_iUnknownAlbumKey = 1;
static const TCHAR sc_szUnknownAlbum[] = {'U','n','k','n','o','w','n',' ','A','l','b','u','m',0};
static const int sc_iUnknownGenreKey = 1;
static const TCHAR sc_szUnknownGenre[] = {'U','n','k','n','o','w','n',' ','G','e','n','r','e',0};


//////////////////////////////////////////////////////////////////////////////////////////
//	stricmp
//////////////////////////////////////////////////////////////////////////////////////////
// This is a copy of the stricmp internal to metakit.
static int stricmp(const char* p1, const char* p2)
{
    int c1, c2;

    do
    {
        c1 = *p1++;
        c2 = *p2++;
    } while (c1 != 0 && (c1 == c2 || tolower(c1) == tolower(c2)));
        
    c1 = tolower(c1);
    c2 = tolower(c2);
        
    return c1 - c2;
}


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
//	CDJContentManagerImp
//////////////////////////////////////////////////////////////////////////////////////////

int CDJContentManagerImp::sm_iNextContentRecordID = 1;
int CDJContentManagerImp::sm_iNextArtistID = 2;
int CDJContentManagerImp::sm_iNextAlbumID = 2;
int CDJContentManagerImp::sm_iNextGenreID = 2;
int CDJContentManagerImp::sm_iNextPlaylistID = 1;

CDJContentManagerImp::CDJContentManagerImp(CDJContentManager* pParent, const char* szFilename)
    : m_pParent(pParent),
    m_mkStorage(szFilename, 1),
    m_pContentID ("ContentID"),
    m_pURL ("URL"),
    m_pTitle ("Title"),
    m_pArtistName ("Artist"),
    m_pArtistID ("ArtistID"),
    m_pAlbumName ("Album"),
    m_pAlbumID ("AlbumID"),
    m_pGenreName ("Genre"),
    m_pGenreID ("GenreID"),
    m_pAlbumTrackNumber("AlbumTrackNumber"),
    m_pDataSourceID ("DataSourceID"),
    m_pCodecID ("CodecID"),
    m_pRecordPtr ("RecordPtr"),
    m_pSortedIndex("SortedIndex"),
    m_bSavedToFile(true),
    m_uiUncommittedCommitRecordCount(0),
    m_bDirty(false)
{
    m_pCDDS = CDJPlayerState::GetInstance()->GetCDDataSource();
    m_iCDDSID = CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID();
    m_iFatDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    m_vTracks = m_mkStorage.GetAs(sc_szTrackSchema);

    m_vArtists = m_mkStorage.GetAs(sc_szArtistSchema);
    m_vAlbums = m_mkStorage.GetAs(sc_szAlbumSchema);
    m_vGenres = m_mkStorage.GetAs(sc_szGenreSchema);
    m_vPlaylists = m_mkStorage.GetAs(sc_szPlaylistSchema);

    m_vURLSort = m_mkStorage.GetAs(sc_szURLSortSchema);
    m_vTitleSort = m_mkStorage.GetAs(sc_szTitleSortSchema);
    m_vAlbumSort = m_mkStorage.GetAs(sc_szAlbumSortSchema);

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
        CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, (int)m_pContentID(m_vTracks[i]), (const char*)m_pURL(m_vTracks[i]), 0, false, i);
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

}

CDJContentManagerImp::CDJContentManagerImp(CDJContentManager* pParent)
    : m_pParent(pParent),
    m_pContentID ("ContentID"),
    m_pURL ("URL"),
    m_pTitle ("Title"),
    m_pArtistName ("Artist"),
    m_pArtistID ("ArtistID"),
    m_pAlbumName ("Album"),
    m_pAlbumID ("AlbumID"),
    m_pGenreName ("Genre"),
    m_pGenreID ("GenreID"),
    m_pAlbumTrackNumber("AlbumTrackNumber"),
    m_pDataSourceID ("DataSourceID"),
    m_pCodecID ("CodecID"),
    m_pRecordPtr ("RecordPtr"),
    m_pSortedIndex("SortedIndex"),
    m_bSavedToFile(false),
    m_uiUncommittedCommitRecordCount(0),
    m_bDirty(false)
{
    m_pCDDS = CDJPlayerState::GetInstance()->GetCDDataSource();
    m_iCDDSID = CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID();
    m_iFatDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    m_vTracks = m_mkStorage.GetAs(sc_szTrackSchema);

    m_vArtists = m_mkStorage.GetAs(sc_szArtistSchema);
    m_vAlbums = m_mkStorage.GetAs(sc_szAlbumSchema);
    m_vGenres = m_mkStorage.GetAs(sc_szGenreSchema);
    m_vPlaylists = m_mkStorage.GetAs(sc_szPlaylistSchema);

    m_vURLSort = m_mkStorage.GetAs(sc_szURLSortSchema);
    m_vTitleSort = m_mkStorage.GetAs(sc_szTitleSortSchema);
    m_vAlbumSort = m_mkStorage.GetAs(sc_szAlbumSortSchema);

    // Add entries for unknown artist, album, and genre props.
    m_vArtists.Add(m_pArtistName[sc_szUnknownArtist] + m_pArtistID[sc_iUnknownArtistKey]);
    m_vAlbums.Add(m_pAlbumName[sc_szUnknownAlbum] + m_pAlbumID[sc_iUnknownAlbumKey]);
    m_vGenres.Add(m_pGenreName[sc_szUnknownGenre] + m_pGenreID[sc_iUnknownGenreKey]);
}


CDJContentManagerImp::~CDJContentManagerImp()
{
    Clear();
    m_mkStorage.RemoveAll();
}

// Creates an empty metadata record.
IMetadata*
CDJContentManagerImp::CreateMetadataRecord() const
{
    return m_pParent->CreateMetadataRecord();
}


// Loads content records from a stream.
// Returns true if the state was loaded, false othersise.
bool
CDJContentManagerImp::LoadStateFromStream(IInputStream* pInputStream)
{
    Clear();

    CMetakitInputStream mis(pInputStream);
    m_mkStorage.LoadFrom(mis);

    m_vTracks = m_mkStorage.GetAs(sc_szTrackSchema);

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
        CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, (int)m_pContentID(m_vTracks[i]), (const char*)m_pURL(m_vTracks[i]), 0, false, i);
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
CDJContentManagerImp::SaveStateToStream(IOutputStream* pOutputStream)
{
    CMetakitOutputStream mos(pOutputStream);
    m_mkStorage.SaveTo(mos);
    return true;
}

// Clears the content manager and deletes its content records.
void
CDJContentManagerImp::Clear()
{
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]);
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        delete (IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]);

    m_vTracks.RemoveAll();
    m_vArtists.RemoveAll();
    m_vAlbums.RemoveAll();
    m_vGenres.RemoveAll();
    m_vPlaylists.RemoveAll();

    m_vURLSort.RemoveAll();
    m_vTitleSort.RemoveAll();
    m_vAlbumSort.RemoveAll();

    // Add the unknown values for album, artist, genre
    m_vArtists.Add(m_pArtistName[sc_szUnknownArtist] + m_pArtistID[sc_iUnknownArtistKey]);
    m_vAlbums.Add(m_pAlbumName[sc_szUnknownAlbum] + m_pAlbumID[sc_iUnknownAlbumKey]);
    m_vGenres.Add(m_pGenreName[sc_szUnknownGenre] + m_pGenreID[sc_iUnknownGenreKey]);

    m_bDirty = true;
}

// Saves the database.
void
CDJContentManagerImp::Commit()
{
    if (m_bSavedToFile)
    {
#ifdef PROFILE_QUERIES
        cyg_tick_count_t tick = cyg_current_time();
        diag_printf("Commit: Start: %d ticks (%d tracks)\n", (int)tick, m_vTracks.GetSize());
#endif  // PROFILE_QUERIES

        CProgressWatcher* pWatch = CProgressWatcher::GetInstance();
        pWatch->SetTask(TASK_COMMITTING_METAKIT);

        m_mkStorage.Commit(true);
    
        pWatch->UnsetTask((eCurrentTask)(TASK_COMMITTING_METAKIT | TASK_CONTENT_UPDATE));

#ifdef PROFILE_QUERIES
        diag_printf("Commit: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES
    }
    m_uiUncommittedCommitRecordCount = 0;
    m_bDirty = false;
}

//! Saves the database only if it's been changed since the last commit.
//! Returns true if the database was committed, false otherwise.
bool
CDJContentManagerImp::CommitIfDirty()
{
    if (m_bDirty && m_bSavedToFile)
    {
        Commit();
        return true;
    }
    else
        return false;
}

// Adds a list of media and playlist records to the content manager.
void
CDJContentManagerImp::AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords)
{
#ifdef PROFILE_UPDATES
    cyg_tick_count_t tick = cyg_current_time();
    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Start: %d ticks\n", tick);
#endif  // PROFILE_UPDATES
    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Current track database size: %d\n", m_vTracks.GetSize());

    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Media content size: %d\n", pContentUpdate->media.Size());

    // Guess if this is one pass or two pass.
    // This works if the database is cleared between updates.
    // TODO: Be less hacky.
    if (!m_vTracks.GetSize())
        m_bTwoPass = pContentUpdate->bTwoPass;

    // Only add records if this is the first pass of a two-pass update or if this is a one-pass only update.
    if (pContentUpdate->bTwoPass || !m_bTwoPass)
    {
        m_iUpdateIndex = 0;
        int iIndexBase = m_vTracks.GetSize();

        m_vTracks.SetSize(iIndexBase + pContentUpdate->media.Size());
        for (int i = 0; i < pContentUpdate->media.Size(); ++i)
        {
            DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "URL %d: %s\n", i, pContentUpdate->media[i].szURL);
            if (pRecords)
                pRecords->PushBack(AddMediaRecordInternal(pContentUpdate->media[i], i + iIndexBase));
            else
                AddMediaRecordInternal(pContentUpdate->media[i], i + iIndexBase);
            if (!pContentUpdate->bTwoPass)
                free(pContentUpdate->media[i].szURL);
        }
        if (!pContentUpdate->bTwoPass)
            pContentUpdate->media.Clear();
    }
    else
    {
        // Just update, don't add.
        c4_Row rTitleSort;
        c4_Row rAlbumSort;
        for (int i = 0; i < pContentUpdate->media.Size(); ++i, ++m_iUpdateIndex)
        {
            free(pContentUpdate->media[i].szURL);

            IMetadata* pMetadata = pContentUpdate->media[i].pMetadata;

            if (!pMetadata)
                continue;

            TCHAR* szValue;
            unsigned int iValue;
            int iArtistID = sc_iUnknownArtistKey,
                iAlbumID = sc_iUnknownAlbumKey,
                iGenreID = sc_iUnknownGenreKey;
            if (SUCCEEDED(pMetadata->GetAttribute(MDA_ARTIST, (void**)&szValue)))
            {
                iArtistID = AddArtist(szValue);
            }
            m_pArtistID(m_vTracks[m_iUpdateIndex]) = iArtistID;
            m_pArtistID(rTitleSort) = iArtistID;
            m_pArtistID(rAlbumSort) = iArtistID;

            if (SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM, (void**)&szValue)))
            {
                iAlbumID = AddAlbum(szValue);
            }
            m_pAlbumID(m_vTracks[m_iUpdateIndex]) = iAlbumID;
            m_pAlbumID(rTitleSort) = iAlbumID;
            m_pAlbumID(rAlbumSort) = iAlbumID;

            if (SUCCEEDED(pMetadata->GetAttribute(MDA_GENRE, (void**)&szValue)))
            {
                iGenreID = AddGenre(szValue);
            }
            m_pGenreID(m_vTracks[m_iUpdateIndex]) = iGenreID;
            m_pGenreID(rTitleSort) = iGenreID;
            m_pGenreID(rAlbumSort) = iGenreID;

            if (SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&iValue)))
            {
                m_pAlbumTrackNumber(m_vTracks[m_iUpdateIndex]) = iValue;
                m_pAlbumTrackNumber(rAlbumSort) = iValue;
            }
            else
            {
                m_pAlbumTrackNumber(m_vTracks[m_iUpdateIndex]) = 0;
                m_pAlbumTrackNumber(rAlbumSort) = 0;
            }

            if (SUCCEEDED(pMetadata->GetAttribute(MDA_TITLE, (void**)&szValue)))
            {
                m_pTitle(m_vTracks[m_iUpdateIndex]) = szValue;
                m_pTitle(rTitleSort) = szValue;
                m_pTitle(rAlbumSort) = szValue;
            }

            m_pSortedIndex(rTitleSort) = m_iUpdateIndex;
            m_vTitleSort.InsertAt(m_vTitleSort.Search(rTitleSort), rTitleSort);
            m_pSortedIndex(rAlbumSort) = m_iUpdateIndex;
            m_vAlbumSort.InsertAt(m_vAlbumSort.Search(rAlbumSort), rAlbumSort);

            delete pMetadata;
        }

        pContentUpdate->media.Clear();

//        PrintURLSort();
    }

    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Playlist content size: %d\n", pContentUpdate->media.Size());
    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
    {
        DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "URL %d: %s\n", i, pContentUpdate->playlists[i].szURL);
        AddPlaylistRecord(pContentUpdate->playlists[i]);
        free(pContentUpdate->playlists[i].szURL);
    }
    pContentUpdate->playlists.Clear();

    m_bDirty = true;

    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "New track database size: %d\n", m_vTracks.GetSize());

#ifdef PROFILE_UPDATES
    DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Total time: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_UPDATES
//    Commit();
}

// Returns the number of media records in the content manager.
int
CDJContentManagerImp::GetMediaRecordCount()
{
    return m_vTracks.GetSize();
}

// Adds an entry to the content manager.
// Returns a pointer to the content record.
// If pbAlreadyExists is not null, then its value is set to true if the content record with
// the same URL already exists in the content manager, false otherwise.
IMediaContentRecord*
CDJContentManagerImp::AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists)
{
    m_bDirty = true;

    int iRes = FindByURL(mediaContent.szURL);
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

            // Commit the database if the number of records added since the last commit has reached the specified limit.
            if (m_pParent->GetAutoCommitCount() && (++m_uiUncommittedCommitRecordCount >= m_pParent->GetAutoCommitCount()))
            {
                DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Auto commit limit reached (%d), committing db\n", m_uiUncommittedCommitRecordCount);
                Commit();
            }

            return pRecord;
        }
    }

    int index = m_vTracks.GetSize();
    IMediaContentRecord* pCR = AddMediaRecordInternal(mediaContent, index);

    if (pbAlreadyExists)
        *pbAlreadyExists = false;

    // Commit the database if the number of records added since the last commit has reached the specified limit.
    if (m_pParent->GetAutoCommitCount() && (++m_uiUncommittedCommitRecordCount >= m_pParent->GetAutoCommitCount()))
    {
        DEBUG(DJCONTENTMANAGER, DBGLEV_INFO, "Auto commit limit reached (%d), committing db\n", m_uiUncommittedCommitRecordCount);
        Commit();
    }

    return pCR;
}

// Adds an entry to the content manager at the specified index.
// Returns a pointer to the content record.
IMediaContentRecord*
CDJContentManagerImp::AddMediaRecordInternal(media_record_info_t& mediaContent, int index)
{
    int iTrackID = sm_iNextContentRecordID++;
    CMetakitMediaContentRecord* pRecord = new CMetakitMediaContentRecord(this, iTrackID,
        mediaContent.szURL, mediaContent.pMetadata, mediaContent.bVerified, index);

    c4_Row rTrack;
    m_pURL(rTrack) = mediaContent.szURL;
    int iArtistKey, iAlbumKey, iGenreKey, iAlbumTrackNumber = 0;
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

        mediaContent.pMetadata->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&iAlbumTrackNumber);

        if (SUCCEEDED(mediaContent.pMetadata->GetAttribute(MDA_TITLE, (void**)&szValue)))
        {
            m_pTitle(rTrack) = szValue;
            // Free the memory in the original metadata record.
            mediaContent.pMetadata->UnsetAttribute(MDA_TITLE);
        }

        delete mediaContent.pMetadata;
        mediaContent.pMetadata = 0;

        // Update title and artist sorted views.
        c4_Row rTitleSort;
        c4_Row rAlbumSort;

        m_pArtistID(rTitleSort) = iArtistKey;
        m_pArtistID(rAlbumSort) = iArtistKey;

        m_pAlbumID(rTitleSort) = iAlbumKey;
        m_pAlbumID(rAlbumSort) = iAlbumKey;

        m_pGenreID(rTitleSort) = iGenreKey;
        m_pGenreID(rAlbumSort) = iGenreKey;

        m_pAlbumTrackNumber(rAlbumSort) = iAlbumTrackNumber;

        const TCHAR* szTitle = (const TCHAR*)m_pTitle(rTrack);
        m_pTitle(rTitleSort) = szTitle;
        m_pTitle(rAlbumSort) = szTitle;

        m_pSortedIndex(rTitleSort) = index;
        m_vTitleSort.InsertAt(m_vTitleSort.Search(rTitleSort), rTitleSort);
        m_pSortedIndex(rAlbumSort) = index;
        m_vAlbumSort.InsertAt(m_vAlbumSort.Search(rAlbumSort), rAlbumSort);
    }
    else
    {
        iArtistKey = AddArtist(0);
        iAlbumKey = AddAlbum(0);
        iGenreKey = AddGenre(0);
        TCHAR tmp[256];

        // Convert the URL into a temporary title.
        if (mediaContent.iDataSourceID == m_iCDDSID)
        {
            int iTrackNumber = m_pCDDS->GetAudioTrackIndex(mediaContent.szURL);
            if (iTrackNumber != -1)
            {
                char szTrack[10];
                sprintf(szTrack, "Track %d", iTrackNumber + 1);
                m_pTitle(rTrack) = CharToTcharN(tmp, szTrack, 9);
            }
            else
            {
                m_pTitle(rTrack) = CharToTcharN(tmp, FilenameFromIsoURLInPlace(mediaContent.szURL), 255);
            }
        }
        else if (mediaContent.iDataSourceID == m_iFatDSID)
        {
            m_pTitle(rTrack) = CharToTcharN(tmp, FilenameFromURLInPlace(mediaContent.szURL), 255);
        }
        else
            m_pTitle(rTrack) = CharToTcharN(tmp, mediaContent.szURL, 255);
    }

    m_pArtistID(rTrack) = iArtistKey;
    m_pAlbumID(rTrack) = iAlbumKey;
    m_pGenreID(rTrack) = iGenreKey;
    m_pAlbumTrackNumber(rTrack) = iAlbumTrackNumber;
    m_pDataSourceID(rTrack) = mediaContent.iDataSourceID;
    m_pCodecID(rTrack) = mediaContent.iCodecID;

    m_pContentID(rTrack) = iTrackID;
    m_pRecordPtr(rTrack) = (int)pRecord;

    m_vTracks.SetAtGrow(index, rTrack);

    // Update URL sorted view.
    c4_Row rURLSort;
    m_pURL(rURLSort) = mediaContent.szURL;
    m_pSortedIndex(rURLSort) = index;
    m_vURLSort.InsertAt(m_vURLSort.Search(rURLSort), rURLSort);

    return pRecord;
}

void
CDJContentManagerImp::PrintTracks() const
{
    diag_printf("Tracks:\n");
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
    {
        diag_printf("%d: %w Ar: %d Al: %d Gn: %d\n",
            i,
            (const TCHAR*)m_pTitle(m_vTracks[i]),
            (int)m_pArtistID(m_vTracks[i]),
            (int)m_pAlbumID(m_vTracks[i]),
            (int)m_pGenreID(m_vTracks[i]));
    }
}

void
CDJContentManagerImp::PrintURLSort() const
{
    diag_printf("URL sort:\n");
    for (int i = 0; i < m_vURLSort.GetSize(); ++i)
    {
        diag_printf("%d: %d URL: %s %w\n",
            i,
            (int)m_pSortedIndex(m_vURLSort[i]),
            (const char*)m_pURL(m_vURLSort[i]),
            (const TCHAR*)m_pTitle(m_vTracks[(int)m_pSortedIndex(m_vURLSort[i])]) );
    }
}

void
CDJContentManagerImp::PrintTitleSort() const
{
    diag_printf("Title sort:\n");
    for (int i = 0; i < m_vTitleSort.GetSize(); ++i)
    {
        diag_printf("%d: %d %w Ar: %d Al: %d Gn: %d\n",
            i,
            (int)m_pSortedIndex(m_vTitleSort[i]),
            (const TCHAR*)m_pTitle(m_vTitleSort[i]),
            (int)m_pArtistID(m_vTitleSort[i]),
            (int)m_pAlbumID(m_vTitleSort[i]),
            (int)m_pGenreID(m_vTitleSort[i]));
    }
}

void
CDJContentManagerImp::PrintAlbumSort() const
{
    diag_printf("Album sort:\n");
    for (int i = 0; i < m_vAlbumSort.GetSize(); ++i)
    {
        diag_printf("%d: %d %w Ar: %d Al: %d Gn: %d Tr: %d\n",
            i,
            (int)m_pSortedIndex(m_vAlbumSort[i]),
            (const TCHAR*)m_pTitle(m_vAlbumSort[i]),
            (int)m_pArtistID(m_vAlbumSort[i]),
            (int)m_pAlbumID(m_vAlbumSort[i]),
            (int)m_pGenreID(m_vAlbumSort[i]),
            (int)m_pAlbumTrackNumber(m_vAlbumSort[i]));
    }
}

// Removes a record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDJContentManagerImp::DeleteMediaRecord(int iContentRecordID)
{
    c4_Row rTrack;
    m_pContentID(rTrack) = iContentRecordID;
    int iRes = m_vTracks.Find(rTrack);
    if (iRes != -1)
    {
        DeleteMediaRecordByRow(iRes);
        m_bDirty = true;
        return true;
    }
    return false;
}

//! Removes a record from the content manager.
//! Returns true if the content record was deleted, false if it wasn't found.
//! WARNING: The pContentRecord pointer will be invalid after this operation!
bool
CDJContentManagerImp::DeleteMediaRecord(IMediaContentRecord* pContentRecord)
{
    int iRes = FindByURL(pContentRecord->GetURL());
    if (iRes != -1)
    {
        DeleteMediaRecordByRow(iRes);
        m_bDirty = true;
        return true;
    }
    return false;
}

// Adds an integer in-order to a sorted list of integers.  Duplicates are dropped.
static void AddToIntList(SimpleList<int>& sl, int i)
{
    SimpleListIterator<int> it = sl.GetHead();
    while(it != sl.GetEnd() && (*it) < i)
        ++it;
    if (it == sl.GetEnd() || (*it) > i)
        sl.Insert(i, it);
}

// For qsort.
static int CompareInt(const void* a, const void* b)
{
    return *((int*)a) < *((int*)b) ? -1 : 1;
}

//! Removes the records in the list from the content manager.
//! All records in the list must come from the same data source.
//! When the function is done the records deleted from the front of the list will be gone.
//! \param nRecordsToDelete The number of records to delete from the front of the list.
//! \param pfnCB A callback function for monitoring progress.  It will be called 2 * records.Size() times.
//! \param pUserData Data to be passed to the callback function.
void
CDJContentManagerImp::DeleteMediaRecords(MediaRecordList& records, int nRecordsToDelete, FNDeleteMediaRecordProgressCB* pfnCB, void* pUserData)
{
#ifdef PROFILE_DELETE
    cyg_tick_count_t start_tick = cyg_current_time();
    diag_printf("DeleteMediaRecords: deleting %d records at %d ticks\n", nRecordsToDelete, (int)start_tick);
#endif  // PROFILE_DELETE

    int nRecords = nRecordsToDelete;
    int prog = 0;

    SimpleList<int> slArtistIDs;
    SimpleList<int> slAlbumIDs;
    SimpleList<int> slGenreIDs;

    SimpleVector<int> svIndices(5, nRecordsToDelete);

    while (!records.IsEmpty() && nRecordsToDelete)
    {
        IContentRecord* pCR = records.PopFront();
        --nRecordsToDelete;

        // Find the record in the URL sorted list.
        const char* szURL = pCR->GetURL();
        int l = -1, iFindIndex = m_vURLSort.GetSize();
        while (l + 1 != iFindIndex)
        {
            const int m = (l + iFindIndex) >> 1;
            if (stricmp(szURL, (const char*)m_pURL(m_vURLSort[m])) > 0)
                l = m;
            else
                iFindIndex = m;
        }

        int index;
        if ((iFindIndex < m_vURLSort.GetSize()) && !stricmp(szURL, (const char*)m_pURL(m_vURLSort[iFindIndex])))
            index = (int)m_pSortedIndex(m_vURLSort[iFindIndex]);
        else
        {
            DEBUGP( DJCONTENTMANAGER, DBGLEV_WARNING, "djcm: Unable to find deleted track\n" );
            continue;
        }

        // Add the index in the tracks table to the list of indices to clear out.
        svIndices.PushBack(index);

        // Remove the entry from the URL sorted list.
        m_vURLSort.RemoveAt(iFindIndex);

        int iArtistID = m_pArtistID(m_vTracks[index]);
        int iAlbumID = m_pAlbumID(m_vTracks[index]);
        int iGenreID = m_pGenreID(m_vTracks[index]);

        // We're done with the content record now.
        delete pCR;

        // Find and remove the entry from the title sorted list.
        const TCHAR* szTitle = (const TCHAR*)m_pTitle(m_vTracks[index]);
        l = -1;
        iFindIndex = m_vTitleSort.GetSize();
        while (l + 1 != iFindIndex)
        {
            const int m = (l + iFindIndex) >> 1;
            if (tstricmp(szTitle, (const TCHAR*)m_pTitle(m_vTitleSort[m])) > 0)
                l = m;
            else
                iFindIndex = m;
        }

        while ((iFindIndex < m_vTitleSort.GetSize()) && (index != (int)m_pSortedIndex(m_vTitleSort[iFindIndex])))
            ++iFindIndex;

        DBASSERT( DJCONTENTMANAGER, (iFindIndex != -1) && (iFindIndex < m_vTitleSort.GetSize()), "Unable to find deleted track in title sorted index, index %d table size %d\n", iFindIndex, m_vTitleSort.GetSize() );
        m_vTitleSort.RemoveAt(iFindIndex);

        // Find and remove the entry from the album sorted list.
        c4_Row rAlbumSort;
        m_pAlbumID(rAlbumSort) = iAlbumID;
        m_pAlbumTrackNumber(rAlbumSort) = (int)m_pAlbumTrackNumber(m_vTracks[index]);
        m_pTitle(rAlbumSort) = szTitle;

        iFindIndex = m_vAlbumSort.Search(rAlbumSort);
        while ((iFindIndex < m_vAlbumSort.GetSize()) && (index != (int)m_pSortedIndex(m_vAlbumSort[iFindIndex])))
            ++iFindIndex;

        DBASSERT( DJCONTENTMANAGER, (iFindIndex < m_vAlbumSort.GetSize()) && (index == (int)m_pSortedIndex(m_vAlbumSort[iFindIndex])), "Unable to find deleted track in album sorted index, index %d table size %d\n", iFindIndex, m_vAlbumSort.GetSize() );
        m_vAlbumSort.RemoveAt(iFindIndex);

        // Remember to clear the artist, album, and genre IDs if needed.
        AddToIntList(slArtistIDs, iArtistID);
        AddToIntList(slAlbumIDs, iAlbumID);
        AddToIntList(slGenreIDs, iGenreID);

        // Update progress.
        pfnCB(++prog, pUserData);
    }

    // Move the entries at the end of the table to the first vacant entries from the beginning of the table.
    int iMoveIndex = m_vTracks.GetSize() - 1;
    int iBottomIndex = 0;
    int iTopIndex = svIndices.Size() - 1;
    DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "Start: total: %d bottom: %d top: %d move: %d\n", svIndices.Size(), svIndices[iBottomIndex], svIndices[iTopIndex], iMoveIndex);
    while (iBottomIndex <= iTopIndex)
    {
        // Find the next move index.
        while (iMoveIndex == svIndices[iTopIndex])
        {
            DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "Shifting down move index, %d to %d, top index %d to %d\n", iMoveIndex, iMoveIndex - 1, svIndices[iTopIndex], iTopIndex - 1 > 0 ? svIndices[iTopIndex - 1] : -1);
            --iMoveIndex;
            --iTopIndex;
            if (iTopIndex < iBottomIndex)
            {
                DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "Looks like the end, bottom: %d top: %d\n", iBottomIndex, iTopIndex);
                break;
            }
        }
        if (iTopIndex < iBottomIndex)
        {
            break;
        }

        DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "Moving %d to %d\n", iMoveIndex, svIndices[iBottomIndex]);

        // Update the index in the URL sorted list.
        const char* szURL = (const char*)m_pURL(m_vTracks[iMoveIndex]);
        int l = -1, iFindIndex = m_vURLSort.GetSize();
        while (l + 1 != iFindIndex)
        {
            const int m = (l + iFindIndex) >> 1;
            if (stricmp(szURL, (const char*)m_pURL(m_vURLSort[m])) > 0)
                l = m;
            else
                iFindIndex = m;
        }

        if ((iFindIndex < m_vURLSort.GetSize()) && !stricmp(szURL, (const char*)m_pURL(m_vURLSort[iFindIndex])))
            m_pSortedIndex(m_vURLSort[iFindIndex]) = svIndices[iBottomIndex];
        else
            DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in URL sorted index\n" );

        // Update the index in the title sorted list.
        const TCHAR* szTitle = (const TCHAR*)m_pTitle(m_vTracks[iMoveIndex]);
        l = -1;
        iFindIndex = m_vTitleSort.GetSize();
        while (l + 1 != iFindIndex)
        {
            const int m = (l + iFindIndex) >> 1;
            if (tstricmp(szTitle, (const TCHAR*)m_pTitle(m_vTitleSort[m])) > 0)
                l = m;
            else
                iFindIndex = m;
        }

        while ((iFindIndex < m_vTitleSort.GetSize()) && (iMoveIndex != (int)m_pSortedIndex(m_vTitleSort[iFindIndex])))
            ++iFindIndex;

        DBASSERT( DJCONTENTMANAGER, (iFindIndex != -1) && (iFindIndex < m_vTitleSort.GetSize()), "Unable to find replacement track in title sorted index\n" );
        m_pSortedIndex(m_vTitleSort[iFindIndex]) = svIndices[iBottomIndex];

        // Update the index in the album sorted list.
        c4_Row rAlbumSort;
        m_pAlbumID(rAlbumSort) = (int)m_pAlbumID(m_vTracks[iMoveIndex]);
        m_pAlbumTrackNumber(rAlbumSort) = (int)m_pAlbumTrackNumber(m_vTracks[iMoveIndex]);
        m_pTitle(rAlbumSort) = szTitle;

        iFindIndex = m_vAlbumSort.Search(rAlbumSort);
        while ((iFindIndex < m_vAlbumSort.GetSize()) && (iMoveIndex != (int)m_pSortedIndex(m_vAlbumSort[iFindIndex])))
            ++iFindIndex;

        DBASSERT( DJCONTENTMANAGER, (iFindIndex < m_vAlbumSort.GetSize()) && (iMoveIndex == (int)m_pSortedIndex(m_vAlbumSort[iFindIndex])), "Unable to find replacement track in album sorted index, index %d table size %d\n", iFindIndex, m_vAlbumSort.GetSize() );
        m_pSortedIndex(m_vAlbumSort[iFindIndex]) = svIndices[iBottomIndex];

        // Move the entry from the end of the tracks table to the next empty space from the front of the tracks table.
        m_vTracks[svIndices[iBottomIndex]] = m_vTracks[iMoveIndex];
        ((CMetakitMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[svIndices[iBottomIndex]]))->SetIndex(svIndices[iBottomIndex]);

        // Go to the next free space from the front of the list.
        ++iBottomIndex;
        // Go to the next full space from the end of the list.
        --iMoveIndex;

        // Update progress.
        pfnCB(++prog, pUserData);
    }

    // Remove the now-empty entries from the end of the list.
    DEBUGP(DJCONTENTMANAGER, DBGLEV_CHATTER, "Removing entries %d to %d\n", m_vTracks.GetSize() - svIndices.Size(), m_vTracks.GetSize() - 1);
    m_vTracks.RemoveAt(m_vTracks.GetSize() - svIndices.Size(), svIndices.Size());

    // Check the artists, albums, and genres that were deleted to see if the entries
    // should be flushed from the string tables.
    for (SimpleListIterator<int> it = slArtistIDs.GetHead(); it != slArtistIDs.GetEnd(); ++it)
        FlushArtist(*it);
    for (SimpleListIterator<int> it = slAlbumIDs.GetHead(); it != slAlbumIDs.GetEnd(); ++it)
        FlushAlbum(*it);
    for (SimpleListIterator<int> it = slGenreIDs.GetHead(); it != slGenreIDs.GetEnd(); ++it)
        FlushGenre(*it);

    // Last progress update.
    pfnCB(nRecords * 2, pUserData);

    m_bDirty = true;

#ifdef PROFILE_DELETE
    cyg_tick_count_t end_tick = cyg_current_time();
    diag_printf("DeleteMediaRecords: Total ticks: %d Rec/Tick: %d Tick/Rec: %d\n", (int)(end_tick - start_tick), (int)(nRecords / (int)(end_tick - start_tick)), (int)((end_tick - start_tick) / nRecords));
#endif  // PROFILE_DELETE

}

//! Removes a record from the content manager.
void
CDJContentManagerImp::DeleteMediaRecordByRow(int index)
{
    int iArtistID = m_pArtistID(m_vTracks[index]);
    int iAlbumID = m_pAlbumID(m_vTracks[index]);
    int iGenreID = m_pGenreID(m_vTracks[index]);
    delete (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[index]);

    c4_Row rFind;
    m_pSortedIndex(rFind) = index;

    int iFindIndex = m_vURLSort.Find(rFind);
    DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in URL sorted index\n" );
    m_vURLSort.RemoveAt(iFindIndex);

    iFindIndex = m_vTitleSort.Find(rFind);
    DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in title sorted index\n" );
    m_vTitleSort.RemoveAt(iFindIndex);

    iFindIndex = m_vAlbumSort.Find(rFind);
    DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in album sorted index\n" );
    m_vAlbumSort.RemoveAt(iFindIndex);

    // Move the last entry into the vacant entry and adjust the sorted view indices.
    int iMoveIndex = m_vTracks.GetSize() - 1;

    if (iMoveIndex != index)
    {
        m_pSortedIndex(rFind) = iMoveIndex;

        iFindIndex = m_vURLSort.Find(rFind);
        DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in URL sorted index\n" );
        m_pSortedIndex(m_vURLSort[iFindIndex]) = index;

        iFindIndex = m_vTitleSort.Find(rFind);
        DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in title sorted index\n" );
        m_pSortedIndex(m_vTitleSort[iFindIndex]) = index;

        iFindIndex = m_vAlbumSort.Find(rFind);
        DBASSERT( DJCONTENTMANAGER, iFindIndex != -1, "Unable to find deleted track in album sorted index\n" );
        m_pSortedIndex(m_vAlbumSort[iFindIndex]) = index;

        m_vTracks[index] = m_vTracks[iMoveIndex];
        ((CMetakitMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[index]))->SetIndex(index);
    }

    m_vTracks.RemoveAt(iMoveIndex);

    FlushArtist(iArtistID);
    FlushAlbum(iAlbumID);
    FlushGenre(iGenreID);
}

// Returns a pointer to the media content record with the specified record ID.
// Returns 0 if no record with a matching ID was found.
IMediaContentRecord*
CDJContentManagerImp::GetMediaRecord(int iContentRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iContentRecordID]);
    return iRes != -1 ? (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]) : 0;
}

// Returns a pointer to the media content record with the specified URL.
// Returns 0 if no record with a matching URL was found.
IMediaContentRecord*
CDJContentManagerImp::GetMediaRecord(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? (IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[iRes]) : 0;
}

// Returns the number of playlist records in the content manager.
int
CDJContentManagerImp::GetPlaylistRecordCount()
{
    return m_vPlaylists.GetSize();
}

// Adds a playlist record to the content manager.
// Returns a pointer to the record.
IPlaylistContentRecord*
CDJContentManagerImp::AddPlaylistRecord(playlist_record_t& playlistRecord)
{
    m_bDirty = true;

    c4_Row rTrack;
    m_pURL(rTrack) = playlistRecord.szURL;

    int iRes = m_vPlaylists.Search(rTrack);
    if ((iRes < m_vPlaylists.GetSize()) && !stricmp(playlistRecord.szURL, (const char*)m_pURL(m_vPlaylists[iRes])))
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

    m_pDataSourceID(rTrack) = playlistRecord.iDataSourceID;
    m_pCodecID(rTrack) = playlistRecord.iPlaylistFormatID;

    m_pContentID(rTrack) = iPlaylistID;
    m_pRecordPtr(rTrack) = (int)pRecord;

    if (iRes < m_vPlaylists.GetSize())
        m_vPlaylists.InsertAt(iRes, rTrack);
    else
        m_vPlaylists.Add(rTrack);

    return pRecord;
}

// Removes a playlist record from the content manager.
// Returns true if the content record was deleted, false if it wasn't found.
bool
CDJContentManagerImp::DeletePlaylistRecord(int iContentRecordID)
{
    m_bDirty = true;

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
CDJContentManagerImp::DeleteRecordsFromDataSource(int iDataSourceID)
{
    Clear();
}

// Marks all records from the given data source as unverified.
void
CDJContentManagerImp::MarkRecordsFromDataSourceUnverified(int iDataSourceID)
{
    c4_View vDS = m_vTracks.Select(m_pDataSourceID[iDataSourceID]);
    for (int i = 0; i < vDS.GetSize(); ++i)
        ((IMediaContentRecord*)(int)m_pRecordPtr(vDS[i]))->SetVerified(false);
}

// Removes all records from the given data source that are marked as unverified.
void
CDJContentManagerImp::DeleteUnverifiedRecordsFromDataSource(int iDataSourceID)
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
CDJContentManagerImp::GetArtistByKey(int iArtistKey) const
{
    int iRes = m_vArtists.Find(m_pArtistID[iArtistKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pArtistName(m_vArtists[iRes]);
}

const TCHAR*
CDJContentManagerImp::GetAlbumByKey(int iAlbumKey) const
{
    int iRes = m_vAlbums.Find(m_pAlbumID[iAlbumKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pAlbumName(m_vAlbums[iRes]);
}

const TCHAR*
CDJContentManagerImp::GetGenreByKey(int iGenreKey) const
{
    int iRes = m_vGenres.Find(m_pGenreID[iGenreKey]);
    return iRes == -1 ? (TCHAR*)0 : m_pGenreName(m_vGenres[iRes]);
}


// Adds an artist to the table, if it isn't already in there.
// Returns the artist key.
int
CDJContentManagerImp::AddArtist(const TCHAR* szArtist)
{
    if (!szArtist || (szArtist[0] == '\0'))
        return sc_iUnknownArtistKey;

    // Search the artist table for a matching name.
    c4_Row rArtist;
    m_pArtistName(rArtist) = szArtist;
    int row = m_vArtists.Search(rArtist);
    if (row < m_vArtists.GetSize())
    {
        if (!tstricmp((const TCHAR*)m_pArtistName(m_vArtists[row]), szArtist))
        {
            // If the string case is different then use the new version.
            if (tstrcmp((const TCHAR*)m_pArtistName(m_vArtists[row]), szArtist))
                m_pArtistName(m_vArtists[row]) = szArtist;
            return m_pArtistID(m_vArtists[row]);
        }
    }

    // No match, so add a new entry.
    int iArtistID = sm_iNextArtistID++;
    m_pArtistID(rArtist) = iArtistID;
    m_vArtists.InsertAt(row, rArtist);

    return iArtistID;
}

int
CDJContentManagerImp::AddAlbum(const TCHAR* szAlbum)
{
    if (!szAlbum || (szAlbum[0] == '\0'))
        return sc_iUnknownAlbumKey;

    // Search the album table for a matching name.
    c4_Row rAlbum;
    m_pAlbumName(rAlbum) = szAlbum;
    int row = m_vAlbums.Search(rAlbum);
    if (row < m_vAlbums.GetSize())
    {
        if (!tstricmp((const TCHAR*)m_pAlbumName(m_vAlbums[row]), szAlbum))
        {
            // If the string case is different then use the new version.
            if (tstrcmp((const TCHAR*)m_pAlbumName(m_vAlbums[row]), szAlbum))
                m_pAlbumName(m_vAlbums[row]) = szAlbum;
            return m_pAlbumID(m_vAlbums[row]);
        }
    }

    // No match, so add a new entry.
    int iAlbumID = sm_iNextAlbumID++;
    m_pAlbumID(rAlbum) = iAlbumID;
    m_vAlbums.InsertAt(row, rAlbum);

    return iAlbumID;
}

int
CDJContentManagerImp::AddGenre(const TCHAR* szGenre)
{
    if (!szGenre || (szGenre[0] == '\0'))
        return sc_iUnknownGenreKey;

    // Search the genre table for a matching name.
    c4_Row rGenre;
    m_pGenreName(rGenre) = szGenre;
    int row = m_vGenres.Search(rGenre);
    if (row < m_vGenres.GetSize())
    {
        if (!tstricmp((const TCHAR*)m_pGenreName(m_vGenres[row]), szGenre))
        {
            // If the string case is different then use the new version.
            if (tstrcmp((const TCHAR*)m_pGenreName(m_vGenres[row]), szGenre))
                m_pGenreName(m_vGenres[row]) = szGenre;
            return m_pGenreID(m_vGenres[row]);
        }
    }

    // No match, so add a new entry.
    int iGenreID = sm_iNextGenreID++;
    m_pGenreID(rGenre) = iGenreID;
    m_vGenres.InsertAt(row, rGenre);

    return iGenreID;
}

int
CDJContentManagerImp::GetArtistKey(const TCHAR* szArtist) const
{
    int iRes = m_vArtists.Find(m_pArtistName[szArtist]);
    return iRes != -1 ? m_pArtistID(m_vArtists[iRes]) : 0;
}

int
CDJContentManagerImp::GetAlbumKey(const TCHAR* szAlbum) const
{
    int iRes = m_vAlbums.Find(m_pAlbumName[szAlbum]);
    return iRes != -1 ? m_pAlbumID(m_vAlbums[iRes]) : 0;
}

int
CDJContentManagerImp::GetGenreKey(const TCHAR* szGenre) const
{
    int iRes = m_vGenres.Find(m_pGenreName[szGenre]);
    return iRes != -1 ? m_pGenreID(m_vGenres[iRes]) : 0;
}

void
CDJContentManagerImp::GetAllMediaRecords(MediaRecordList& records) const
{
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]));
}

void
CDJContentManagerImp::GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const
{
    for (int i = 0; i < m_vTracks.GetSize(); ++i)
        if (m_pDataSourceID(m_vTracks[i]) == iDataSourceID)
            records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[i]));
}

void
CDJContentManagerImp::GetMediaRecords(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;

    c4_View vFind = m_vTracks.Select(rFind);
    for (int i = 0; i < vFind.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(vFind[i]));
}

void
CDJContentManagerImp::GetMediaRecordsSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID, int iSortNum, va_list vaSort) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;

    c4_View vSort;
    for (int i = 0; i < iSortNum; ++i)
    {
        int id = va_arg(vaSort, int);
        switch (id)
        {
            case MDA_TITLE:
                vSort.AddProperty(m_pTitle);
                break;

            case MDA_ARTIST:
                vSort.AddProperty(m_pArtistID);
                break;

            case MDA_ALBUM:
                vSort.AddProperty(m_pAlbumID);
                break;

            case MDA_GENRE:
                vSort.AddProperty(m_pGenreID);
                break;

            case MDA_ALBUM_TRACK_NUMBER:
                vSort.AddProperty(m_pAlbumTrackNumber);
                break;
        }
    }

#ifdef PROFILE_QUERIES
    cyg_tick_count_t tick = cyg_current_time();
    diag_printf("GetMediaRecordsSorted: Start: %d ticks (%d tracks)\n", (int)tick, m_vTracks.GetSize());
#endif  // PROFILE_QUERIES

    c4_View vFind = m_vTracks.Select(rFind).SortOn(vSort);

#ifdef PROFILE_QUERIES
    diag_printf("GetMediaRecordsSorted: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

    for (int i = 0; i < vFind.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(vFind[i]));
}

void
CDJContentManagerImp::GetMediaRecordsTitleSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;

#ifdef PROFILE_QUERIES
    cyg_tick_count_t tick = cyg_current_time();
    diag_printf("GetMediaRecordsTitleSorted: Start: %d ticks (%d tracks)\n", (int)tick, m_vTracks.GetSize());
#endif  // PROFILE_QUERIES

    c4_View vFind = m_vTitleSort.Select(rFind);

#ifdef PROFILE_QUERIES
    diag_printf("GetMediaRecordsTitleSorted: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

    for (int i = 0; i < vFind.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[m_pSortedIndex(vFind[i])]));
}

void
CDJContentManagerImp::GetMediaRecordsAlbumSorted(MediaRecordList& records,
    int iArtistKey, int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    c4_Row rFind;
    if (iArtistKey != CMK_ALL)
        m_pArtistID(rFind) = iArtistKey;
    if (iAlbumKey != CMK_ALL)
        m_pAlbumID(rFind) = iAlbumKey;
    if (iGenreKey != CMK_ALL)
        m_pGenreID(rFind) = iGenreKey;

#ifdef PROFILE_QUERIES
    cyg_tick_count_t tick = cyg_current_time();
    diag_printf("GetMediaRecordsAlbumSorted: Start: %d ticks (%d tracks)\n", (int)tick, m_vTracks.GetSize());
#endif  // PROFILE_QUERIES

    c4_View vFind = m_vAlbumSort.Select(rFind);

#ifdef PROFILE_QUERIES
    diag_printf("GetMediaRecordsAlbumSorted: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

    for (int i = 0; i < vFind.GetSize(); ++i)
        records.PushBack((IMediaContentRecord*)(int)m_pRecordPtr(m_vTracks[m_pSortedIndex(vFind[i])]));
}


void
CDJContentManagerImp::GetArtists(ContentKeyValueVector& keyValues,
    int iAlbumKey, int iGenreKey, int iDataSourceID) const
{
    // If the query is for everything, then just dump the table.
    if ((iAlbumKey == CMK_ALL) && (iGenreKey == CMK_ALL))
    {
        bool bSkipUnknown = m_vTracks.Find(m_pArtistID[sc_iUnknownArtistKey]) == -1;
        
        for (int i = 0; i < m_vArtists.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pArtistID(m_vArtists[i]),
                szValue : m_pArtistName(m_vArtists[i])
            };

            if ((rec.iKey != sc_iUnknownArtistKey) || !bSkipUnknown)
                keyValues.PushBack(rec);
        }
    }
    else
    {
        c4_Row rFind;
        if (iAlbumKey != CMK_ALL)
            m_pAlbumID(rFind) = iAlbumKey;
        if (iGenreKey != CMK_ALL)
            m_pGenreID(rFind) = iGenreKey;

        c4_View vMediaProj(m_pArtistID);

#ifdef PROFILE_QUERIES
        cyg_tick_count_t tick = cyg_current_time();
        diag_printf("GetArtists: Start: %d ticks\n", tick);
#endif  // PROFILE_QUERIES

        c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vArtists);

#ifdef PROFILE_QUERIES
        diag_printf("GetArtists: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

        for (int i = 0; i < vFind.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pArtistID(vFind[i]),
                szValue : m_pArtistName(vFind[i])
            };
            keyValues.PushBack(rec);
        }
    }
}

void
CDJContentManagerImp::GetAlbums(ContentKeyValueVector& keyValues,
    int iArtistKey, int iGenreKey, int iDataSourceID) const
{
    // If the query is for everything, then just dump the table.
    if ((iArtistKey == CMK_ALL) && (iGenreKey == CMK_ALL))
    {
        bool bSkipUnknown = m_vTracks.Find(m_pAlbumID[sc_iUnknownAlbumKey]) == -1;
        
        for (int i = 0; i < m_vAlbums.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pAlbumID(m_vAlbums[i]),
                szValue : m_pAlbumName(m_vAlbums[i])
            };

            if ((rec.iKey != sc_iUnknownAlbumKey) || !bSkipUnknown)
                keyValues.PushBack(rec);
        }
    }
    else
    {
        c4_Row rFind;
        if (iArtistKey != CMK_ALL)
            m_pArtistID(rFind) = iArtistKey;
        if (iGenreKey != CMK_ALL)
            m_pGenreID(rFind) = iGenreKey;

        c4_View vMediaProj(m_pAlbumID);

#ifdef PROFILE_QUERIES
        cyg_tick_count_t tick = cyg_current_time();
        diag_printf("GetAlbums: Start: %d ticks\n", tick);
#endif  // PROFILE_QUERIES

        c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vAlbums);

#ifdef PROFILE_QUERIES
        diag_printf("GetAlbums: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

        for (int i = 0; i < vFind.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pAlbumID(vFind[i]),
                szValue : m_pAlbumName(vFind[i])
            };

            keyValues.PushBack(rec);
        }
    }
}

void
CDJContentManagerImp::GetGenres(ContentKeyValueVector& keyValues,
    int iArtistKey, int iAlbumKey, int iDataSourceID) const
{
    // If the query is for everything, then just dump the table.
    if ((iArtistKey == CMK_ALL) && (iAlbumKey == CMK_ALL))
    {
        bool bSkipUnknown = m_vTracks.Find(m_pGenreID[sc_iUnknownGenreKey]) == -1;
        
        for (int i = 0; i < m_vGenres.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pGenreID(m_vGenres[i]),
                szValue : m_pGenreName(m_vGenres[i])
            };

            if ((rec.iKey != sc_iUnknownGenreKey) || !bSkipUnknown)
                keyValues.PushBack(rec);
        }
    }
    else
    {
        c4_Row rFind;
        if (iArtistKey != CMK_ALL)
            m_pArtistID(rFind) = iArtistKey;
        if (iAlbumKey != CMK_ALL)
            m_pAlbumID(rFind) = iAlbumKey;

        c4_View vMediaProj(m_pGenreID);

#ifdef PROFILE_QUERIES
        cyg_tick_count_t tick = cyg_current_time();
        diag_printf("GetGenres: Start: %d ticks\n", tick);
#endif  // PROFILE_QUERIES

        c4_View vFind = m_vTracks.Select(rFind).Project(vMediaProj).Unique().Join(vMediaProj, m_vGenres);

#ifdef PROFILE_QUERIES
        diag_printf("GetGenres: End: %d ticks\n", cyg_current_time() - tick);
#endif  // PROFILE_QUERIES

        for (int i = 0; i < vFind.GetSize(); ++i)
        {
            cm_key_value_record_t rec = {
                iKey : m_pGenreID(vFind[i]),
                szValue : m_pGenreName(vFind[i])
            };

            keyValues.PushBack(rec);
        }
    }
}

//! Returns the number of artists in the content manager.
int
CDJContentManagerImp::GetArtistCount() const
{
    return m_vArtists.GetSize() - (m_vTracks.Find(m_pArtistID[sc_iUnknownArtistKey]) == -1 ? 1 : 0);
}

//! Returns the number of albums in the content manager.
int
CDJContentManagerImp::GetAlbumCount() const
{
    return m_vAlbums.GetSize() - (m_vTracks.Find(m_pAlbumID[sc_iUnknownAlbumKey]) == -1 ? 1 : 0);
}

//! Returns the number of genres in the content manager.
int
CDJContentManagerImp::GetGenreCount() const
{
    return m_vGenres.GetSize() - (m_vTracks.Find(m_pGenreID[sc_iUnknownGenreKey]) == -1 ? 1 : 0);
}

//! Returns a pointer to the playlist content record with the specified URL in the given data source.
//! Returns 0 if no record with a matching URL was found.
IPlaylistContentRecord*
CDJContentManagerImp::GetPlaylistRecord(const char* szURL)
{
    int l = -1, u = m_vPlaylists.GetSize();
    while (l + 1 != u)
    {
        const int m = (l + u) >> 1;
        if (stricmp(szURL, (const char*)m_pURL(m_vPlaylists[m])) > 0)
            l = m;
        else
            u = m;
    }

    if ((u < m_vPlaylists.GetSize()) && !stricmp(szURL, (const char*)m_pURL(m_vPlaylists[u])))
        return (IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[u]);
    else
        return 0;
}

void
CDJContentManagerImp::GetAllPlaylistRecords(PlaylistRecordList& records) const
{
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        records.PushBack((IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]));
}

void
CDJContentManagerImp::GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const
{
    for (int i = 0; i < m_vPlaylists.GetSize(); ++i)
        if (m_pDataSourceID(m_vPlaylists[i]) == iDataSourceID)
            records.PushBack((IPlaylistContentRecord*)(int)m_pRecordPtr(m_vPlaylists[i]));
}


// Returns the row index of the media content record with the given URL,
// or -1 if there is no match.
int
CDJContentManagerImp::FindByURL(const char* szURL)
{
    int l = -1, u = m_vURLSort.GetSize();
    while (l + 1 != u)
    {
        const int m = (l + u) >> 1;
        if (stricmp(szURL, (const char*)m_pURL(m_vURLSort[m])) > 0)
            l = m;
        else
            u = m;
    }

    if ((u < m_vURLSort.GetSize()) && !stricmp(szURL, (const char*)m_pURL(m_vURLSort[u])))
        return (int)m_pSortedIndex(m_vURLSort[u]);
    else
        return -1;
}

#ifdef USE_URL_INDEX

const char*
CDJContentManagerImp::GetMediaRecordURL(int iRecordID)
{
    int iRes = m_vTracks.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pURL(m_vTracks[iRes]) : (char*)0;
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordTitle(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pTitle(m_vTracks[iRes]) : (TCHAR*)0;
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordArtist(const char* szURL)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        return GetArtistByKey(m_pArtistID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordAlbum(const char* szURL)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        return GetAlbumByKey(m_pAlbumID(m_vTracks[iRes]));
    else
        return 0;
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordGenre(const char* szURL)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        return GetGenreByKey(m_pGenreID(m_vTracks[iRes]));
    else
        return 0;
}

int
CDJContentManagerImp::GetMediaRecordAlbumTrackNumber(const char* szURL)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        return m_pAlbumTrackNumber(m_vTracks[iRes]);
    else
        return 0;
}

int
CDJContentManagerImp::GetMediaRecordArtistKey(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pArtistID(m_vTracks[iRes]) : 0;
}

int
CDJContentManagerImp::GetMediaRecordAlbumKey(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pAlbumID(m_vTracks[iRes]) : 0;
}

int
CDJContentManagerImp::GetMediaRecordGenreKey(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pGenreID(m_vTracks[iRes]) : 0;
}

int
CDJContentManagerImp::GetMediaRecordDataSourceID(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pDataSourceID(m_vTracks[iRes]) : 0;
}

int
CDJContentManagerImp::GetMediaRecordCodecID(const char* szURL)
{
    int iRes = FindByURL(szURL);
    return iRes != -1 ? m_pCodecID(m_vTracks[iRes]) : 0;
}

void
CDJContentManagerImp::SetMediaRecordURL(int iRecordID, const char* szURL)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        m_pURL(m_vTracks[iRes]) = szURL;
}

void
CDJContentManagerImp::SetMediaRecordTitle(const char* szURL, const TCHAR* szTitle)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        SetMediaRecordTitle(iRes, szTitle);
}

void
CDJContentManagerImp::SetMediaRecordArtist(const char* szURL, const TCHAR* szArtist)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        SetMediaRecordArtist(iRes, szArtist);
}

void
CDJContentManagerImp::SetMediaRecordAlbum(const char* szURL, const TCHAR* szAlbum)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        SetMediaRecordAlbum(iRes, szAlbum);
}

void
CDJContentManagerImp::SetMediaRecordGenre(const char* szURL, const TCHAR* szGenre)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        SetMediaRecordGenre(iRes, szGenre);
}

void
CDJContentManagerImp::SetMediaRecordAlbumTrackNumber(const char* szURL, int iAlbumTrackNumber)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        SetMediaRecordAlbumTrackNumber(iRes, iAlbumTrackNumber);
}

void
CDJContentManagerImp::SetMediaRecordDataSourceID(const char* szURL, int iDataSourceID)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        m_pDataSourceID(m_vTracks[iRes]) = iDataSourceID;
}

void
CDJContentManagerImp::SetMediaRecordCodecID(const char* szURL, int iCodecID)
{
    int iRes = FindByURL(szURL);
    if (iRes != -1)
        m_pCodecID(m_vTracks[iRes]) = iCodecID;
}

#else   // USE_URL_INDEX

const char*
CDJContentManagerImp::GetMediaRecordURL(int iRowIndex)
{
    return (const char*)m_pURL(m_vTracks[iRowIndex]);
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordTitle(int iRowIndex)
{
    return (const TCHAR*)m_pTitle(m_vTracks[iRowIndex]);
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordArtist(int iRowIndex)
{
    return (const TCHAR*)GetArtistByKey(m_pArtistID(m_vTracks[iRowIndex]));
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordAlbum(int iRowIndex)
{
    return (const TCHAR*)GetAlbumByKey(m_pAlbumID(m_vTracks[iRowIndex]));
}

const TCHAR*
CDJContentManagerImp::GetMediaRecordGenre(int iRowIndex)
{
    return (const TCHAR*)GetGenreByKey(m_pGenreID(m_vTracks[iRowIndex]));
}

int
CDJContentManagerImp::GetMediaRecordAlbumTrackNumber(int iRowIndex)
{
    return (int)m_pAlbumTrackNumber(m_vTracks[iRowIndex]);
}

int
CDJContentManagerImp::GetMediaRecordArtistKey(int iRowIndex)
{
    return (int)m_pArtistID(m_vTracks[iRowIndex]);
}

int
CDJContentManagerImp::GetMediaRecordAlbumKey(int iRowIndex)
{
    return (int)m_pAlbumID(m_vTracks[iRowIndex]);
}

int
CDJContentManagerImp::GetMediaRecordGenreKey(int iRowIndex)
{
    return (int)m_pGenreID(m_vTracks[iRowIndex]);
}

int
CDJContentManagerImp::GetMediaRecordDataSourceID(int iRowIndex)
{
    return (int)m_pDataSourceID(m_vTracks[iRowIndex]);
}

int
CDJContentManagerImp::GetMediaRecordCodecID(int iRowIndex)
{
    return (int)m_pCodecID(m_vTracks[iRowIndex]);
}

void
CDJContentManagerImp::SetMediaRecordURL(int iRowIndex, const char* szURL)
{
    m_pURL(m_vTracks[iRowIndex]) = szURL;
}

void
CDJContentManagerImp::SetMediaRecordTitle(int iRowIndex, const TCHAR* szTitle)
{
    m_pTitle(m_vTracks[iRowIndex]) = szTitle;

    if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
    {
        int index = GetTitleSortedIndex(iRowIndex);
        m_pTitle(m_vTitleSort[index]) = szTitle;
        RepositionTitleSortedRow(index);
        index = GetAlbumSortedIndex(iRowIndex);
        m_pTitle(m_vAlbumSort[index]) = szTitle;
        RepositionAlbumSortedRow(index);
    }
}

void
CDJContentManagerImp::SetMediaRecordArtist(int iRowIndex, const TCHAR* szArtist)
{
    int iArtistID = AddArtist(szArtist);
    int iOldArtistID = m_pArtistID(m_vTracks[iRowIndex]);
    if (iArtistID != iOldArtistID)
    {
        m_pArtistID(m_vTracks[iRowIndex]) = iArtistID;
        if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
        {
            m_pArtistID(m_vTitleSort[GetTitleSortedIndex(iRowIndex)]) = iArtistID;
            m_pArtistID(m_vAlbumSort[GetAlbumSortedIndex(iRowIndex)]) = iArtistID;
        }
        // If there are no more tracks by this artist, then remove that artist record.
        FlushArtist(iOldArtistID);
    }
}

void
CDJContentManagerImp::SetMediaRecordAlbum(int iRowIndex, const TCHAR* szAlbum)
{
    int iAlbumID = AddAlbum(szAlbum);
    int iOldAlbumID = m_pAlbumID(m_vTracks[iRowIndex]);
    if (iAlbumID != iOldAlbumID)
    {
        m_pAlbumID(m_vTracks[iRowIndex]) = iAlbumID;
        if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
        {
            m_pAlbumID(m_vTitleSort[GetTitleSortedIndex(iRowIndex)]) = iAlbumID;
            int index = GetAlbumSortedIndex(iRowIndex);
            m_pAlbumID(m_vAlbumSort[index]) = iAlbumID;

            RepositionAlbumSortedRow(index);
        }

        // If there are no more tracks on this album, then remove that album record.
        FlushAlbum(iOldAlbumID);
    }
}

void
CDJContentManagerImp::SetMediaRecordGenre(int iRowIndex, const TCHAR* szGenre)
{
    int iGenreID = AddGenre(szGenre);
    int iOldGenreID = m_pGenreID(m_vTracks[iRowIndex]);
    if (iGenreID != iOldGenreID)
    {
        m_pGenreID(m_vTracks[iRowIndex]) = iGenreID;
        if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
        {
            m_pGenreID(m_vTitleSort[GetTitleSortedIndex(iRowIndex)]) = iGenreID;
            m_pGenreID(m_vAlbumSort[GetAlbumSortedIndex(iRowIndex)]) = iGenreID;
        }

        // If there are no more tracks in this genre, then remove that genre record.
        FlushGenre(iOldGenreID);
    }
}

void
CDJContentManagerImp::SetMediaRecordAlbumTrackNumber(int iRowIndex, int iAlbumTrackNumber)
{
    m_pAlbumTrackNumber(m_vTracks[iRowIndex]) = iAlbumTrackNumber;
    if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
    {
        int index = GetAlbumSortedIndex(iRowIndex);
        if (m_pAlbumTrackNumber(m_vAlbumSort[index]) != iAlbumTrackNumber)
        {
            m_pAlbumTrackNumber(m_vAlbumSort[index]) = iAlbumTrackNumber;
            RepositionAlbumSortedRow(index);
        }
    }
}

void
CDJContentManagerImp::SetMediaRecordDataSourceID(int iRowIndex, int iDataSourceID)
{
    m_pDataSourceID(m_vTracks[iRowIndex]) = iDataSourceID;
}

void
CDJContentManagerImp::SetMediaRecordCodecID(int iRowIndex, int iCodecID)
{
    m_pCodecID(m_vTracks[iRowIndex]) = iCodecID;
}

#endif  // USE_URL_INDEX

void
CDJContentManagerImp::MergeMediaRecordAttributes(int iRowIndex, const IMetadata* pMetadata, bool bOverwrite)
{
    TCHAR* tszValue;
    bool bResortTitle = false, bResortAlbum = false;
    int iTitleSortedRow = GetTitleSortedIndex(iRowIndex);
    int iAlbumSortedRow = GetAlbumSortedIndex(iRowIndex);

    // Don't reposition indices while a content update is still in progress.
    bool bTableFull = m_vTracks.GetSize() == m_vTitleSort.GetSize();

    // Update title.
    if (SUCCEEDED(pMetadata->GetAttribute(MDA_TITLE, (void**)&tszValue)))
    {
        m_pTitle(m_vTracks[iRowIndex]) = tszValue;
        if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
        {
            m_pTitle(m_vTitleSort[iTitleSortedRow]) = tszValue;
            m_pTitle(m_vAlbumSort[iAlbumSortedRow]) = tszValue;
            bResortTitle = true;
            bResortAlbum = true;
        }
    }

    // Update artist.
    if ((bOverwrite || (((int)m_pArtistID(m_vTracks[iRowIndex])) == sc_iUnknownArtistKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_ARTIST, (void**)&tszValue)))
    {
        int iArtistID = AddArtist(tszValue);
        int iOldArtistID = m_pArtistID(m_vTracks[iRowIndex]);
        if (iArtistID != iOldArtistID)
        {
            m_pArtistID(m_vTracks[iRowIndex]) = iArtistID;
            if (bTableFull)
            {
                m_pArtistID(m_vTitleSort[iTitleSortedRow]) = iArtistID;
                m_pArtistID(m_vAlbumSort[iAlbumSortedRow]) = iArtistID;
            }
            // If there are no more tracks by this artist, then remove that artist record.
            FlushArtist(iOldArtistID);
        }
    }

    // Update album.
    if ((bOverwrite || (((int)m_pAlbumID(m_vTracks[iRowIndex])) == sc_iUnknownAlbumKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM, (void**)&tszValue)))
    {
        int iAlbumID = AddAlbum(tszValue);
        int iOldAlbumID = m_pAlbumID(m_vTracks[iRowIndex]);
        if (iAlbumID != iOldAlbumID)
        {
            m_pAlbumID(m_vTracks[iRowIndex]) = iAlbumID;
            if (bTableFull)
            {
                m_pAlbumID(m_vTitleSort[iTitleSortedRow]) = iAlbumID;
                m_pAlbumID(m_vAlbumSort[iAlbumSortedRow]) = iAlbumID;
                bResortTitle = true;
                bResortAlbum = true;
            }
            // If there are no more tracks on this album, then remove that album record.
            FlushAlbum(iOldAlbumID);
        }
    }

    // Update genre.
    if ((bOverwrite || (((int)m_pGenreID(m_vTracks[iRowIndex])) == sc_iUnknownGenreKey)) && SUCCEEDED(pMetadata->GetAttribute(MDA_GENRE, (void**)&tszValue)))
    {
        int iGenreID = AddGenre(tszValue);
        int iOldGenreID = m_pGenreID(m_vTracks[iRowIndex]);
        if (iGenreID != iOldGenreID)
        {
            m_pGenreID(m_vTracks[iRowIndex]) = iGenreID;
            if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
            {
                m_pGenreID(m_vTitleSort[iTitleSortedRow]) = iGenreID;
                m_pGenreID(m_vAlbumSort[iAlbumSortedRow]) = iGenreID;
            }
            // If there are no more tracks in this genre, then remove that genre record.
            FlushGenre(iOldGenreID);
        }
    }

    // Update album track number.
    int iAlbumTrackNumber;
    if ((bOverwrite || (((int)m_pAlbumTrackNumber(m_vTracks[iRowIndex])) == 0)) && SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&iAlbumTrackNumber)))
    {
        m_pAlbumTrackNumber(m_vTracks[iRowIndex]) = iAlbumTrackNumber;
        if (m_vTracks.GetSize() == m_vTitleSort.GetSize())
        {
            if (m_pAlbumTrackNumber(m_vAlbumSort[iAlbumSortedRow]) != iAlbumTrackNumber)
            {
                m_pAlbumTrackNumber(m_vAlbumSort[iAlbumSortedRow]) = iAlbumTrackNumber;
                bResortAlbum = true;
            }
        }
    }

    // If the sort columns for the title sorted table or album sorted table have changed
    // then reposition the rows.
    if (bResortTitle)
        RepositionTitleSortedRow(iTitleSortedRow);
    if (bResortAlbum)
        RepositionAlbumSortedRow(iAlbumSortedRow);
}

const char*
CDJContentManagerImp::GetPlaylistRecordURL(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pURL(m_vPlaylists[iRes]) : (char*)0;
}

int
CDJContentManagerImp::GetPlaylistRecordDataSourceID(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pDataSourceID(m_vPlaylists[iRes]) : 0;
}

int
CDJContentManagerImp::GetPlaylistRecordPlaylistFormatID(int iRecordID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    return iRes != -1 ? m_pCodecID(m_vPlaylists[iRes]) : 0;
}

void
CDJContentManagerImp::SetPlaylistRecordURL(int iRecordID, const char* szURL)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pURL(m_vPlaylists[iRes]) = szURL;
}

void
CDJContentManagerImp::SetPlaylistRecordDataSourceID(int iRecordID, int iDataSourceID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pDataSourceID(m_vPlaylists[iRes]) = iDataSourceID;
}

void
CDJContentManagerImp::SetPlaylistRecordPlaylistFormatID(int iRecordID, int iPlaylistFormatID)
{
    int iRes = m_vPlaylists.Find(m_pContentID[iRecordID]);
    if (iRes != -1)
        m_pCodecID(m_vPlaylists[iRes]) = iPlaylistFormatID;
}

// If there are no more tracks by this artist, then remove that artist record.
void
CDJContentManagerImp::FlushArtist(int iArtistKey)
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
CDJContentManagerImp::FlushAlbum(int iAlbumKey)
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
CDJContentManagerImp::FlushGenre(int iGenreKey)
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

// Given a row index in the track table, return the row index in the sorted table.
int
CDJContentManagerImp::GetTitleSortedIndex(int row) const 
{
    return m_vTitleSort.Find(m_pSortedIndex[row]);
}

// Given a row index in the track table, return the row index in the sorted table.
int
CDJContentManagerImp::GetAlbumSortedIndex(int row) const
{
    return m_vAlbumSort.Find(m_pSortedIndex[row]);
}

// Repositions a given row in the sorted table.
void
CDJContentManagerImp::RepositionTitleSortedRow(int row)
{
    c4_Row rCopy = m_vTitleSort[row];
    m_vTitleSort.RemoveAt(row);
    m_vTitleSort.InsertAt(m_vTitleSort.Search(rCopy), rCopy);
}

// Repositions a given row in the sorted table.
void
CDJContentManagerImp::RepositionAlbumSortedRow(int row)
{
    c4_Row rCopy = m_vAlbumSort[row];
    m_vAlbumSort.RemoveAt(row);
    m_vAlbumSort.InsertAt(m_vAlbumSort.Search(rCopy), rCopy);
}


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

CMetakitMediaContentRecord::CMetakitMediaContentRecord(CDJContentManagerImp* pManager, int iContentRecordID,
        const char* szURL, IMetadata* pMetadata, bool bVerified, int index)
    : m_iID(iContentRecordID),
    m_index(index),
    m_pManager(pManager),
    m_eStatus(CR_OKAY)
{
//    delete pMetadata;
#ifdef USE_URL_INDEX
    m_szURL = new char[strlen(szURL) + 1];
    strcpy(m_szURL, szURL);
#endif  // USE_URL_INDEX
}

CMetakitMediaContentRecord::~CMetakitMediaContentRecord()
{
#ifdef USE_URL_INDEX
    delete [] m_szURL;
#endif  // USE_URL_INDEX
}

const char*
CMetakitMediaContentRecord::GetURL() const
{
#ifdef USE_URL_INDEX
    return m_szURL;
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordURL(m_index);
#endif  // USE_URL_INDEX
}

const TCHAR*
CMetakitMediaContentRecord::GetTitle() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordTitle(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordTitle(m_index);
#endif  // USE_URL_INDEX
}

const TCHAR*
CMetakitMediaContentRecord::GetArtist() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordArtist(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordArtist(m_index);
#endif  // USE_URL_INDEX
}

const TCHAR*
CMetakitMediaContentRecord::GetAlbum() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbum(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbum(m_index);
#endif  // USE_URL_INDEX
}

const TCHAR*
CMetakitMediaContentRecord::GetGenre() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordGenre(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordGenre(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetAlbumTrackNumber() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbumTrackNumber(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbumTrackNumber(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetDataSourceID() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordDataSourceID(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordDataSourceID(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetCodecID() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordCodecID(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordCodecID(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetArtistKey() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordArtistKey(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordArtistKey(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetAlbumKey() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbumKey(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordAlbumKey(m_index);
#endif  // USE_URL_INDEX
}

int
CMetakitMediaContentRecord::GetGenreKey() const
{
#ifdef USE_URL_INDEX
    return m_pManager->GetMediaRecordGenreKey(m_szURL);
#else   // USE_URL_INDEX
    return m_pManager->GetMediaRecordGenreKey(m_index);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetTitle(const TCHAR* szTitle)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordTitle(m_szURL, szTitle);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordTitle(m_index, szTitle);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetAlbum(const TCHAR* szAlbum)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordAlbum(m_szURL, szAlbum);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordAlbum(m_index, szAlbum);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetGenre(const TCHAR* szGenre)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordGenre(m_szURL, szGenre);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordGenre(m_index, szGenre);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetArtist(const TCHAR* szArtist)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordArtist(m_szURL, szArtist);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordArtist(m_index, szArtist);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetAlbumTrackNumber(int iAlbumTrackNumber)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordAlbumTrackNumber(m_szURL, iAlbumTrackNumber);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordAlbumTrackNumber(m_index, iAlbumTrackNumber);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetDataSourceID(int iDataSourceID)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordDataSourceID(m_szURL, iDataSourceID);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordDataSourceID(m_index, iDataSourceID);
#endif  // USE_URL_INDEX
}

void
CMetakitMediaContentRecord::SetCodecID(int iCodecID)
{
#ifdef USE_URL_INDEX
    m_pManager->SetMediaRecordCodecID(m_szURL, iCodecID);
#else   // USE_URL_INDEX
    m_pManager->SetMediaRecordCodecID(m_index, iCodecID);
#endif  // USE_URL_INDEX
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
            (iAttributeID == MDA_GENRE) ||
            (iAttributeID == MDA_ALBUM_TRACK_NUMBER))
        return true;
    else
        return false;
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

        case MDA_ALBUM_TRACK_NUMBER:
            SetAlbumTrackNumber((int)pAttributeValue);
            return METADATA_NO_ERROR;

        default:
            return METADATA_NOT_USED;
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

        case MDA_ALBUM_TRACK_NUMBER:
        {
            *ppAttributeValue = (void*)GetAlbumTrackNumber();
            return METADATA_NO_ERROR;
        }

        default:
        {
            return METADATA_NOT_USED;
        }
    }
}

void
CMetakitMediaContentRecord::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite)
{
#ifdef USE_URL_INDEX
    m_pManager->MergeMediaRecordAttributes(m_szURL, pMetadata, bOverwrite);
#else   // USE_URL_INDEX
    m_pManager->MergeMediaRecordAttributes(m_index, pMetadata, bOverwrite);
#endif  // USE_URL_INDEX
}

//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////


CMetakitPlaylistContentRecord::CMetakitPlaylistContentRecord(CDJContentManagerImp* pManager, int iContentRecordID, bool bVerified)
    : m_iID(iContentRecordID),
    m_pManager(pManager),
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

