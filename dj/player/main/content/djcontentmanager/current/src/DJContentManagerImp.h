//
// DJContentManagerImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DJCONTENTMANAGERIMP_H_
#define DJCONTENTMANAGERIMP_H_

#include <main/content/djcontentmanager/DJContentManager.h>
#include <stdarg.h>

typedef c4_WideStringProp c4_TStringProp;
class CDJContentManager;
class CCDDataSource;

//#define USE_FIXED_ALLOCATOR
#ifdef USE_FIXED_ALLOCATOR
#include <util/alloc/FixedAllocator.h>
#endif  // USE_FIXED_ALLOCATOR

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJContentManagerImp
//////////////////////////////////////////////////////////////////////////////////////////

class CDJContentManagerImp : public IQueryableContentManager
{
public:
    CDJContentManagerImp(CDJContentManager* pParent, const char* szFilename);
    CDJContentManagerImp(CDJContentManager* pParent);
    ~CDJContentManagerImp();

    //! Gets the next free media content record ID.
    static int GetNextMediaContentRecordID()
        { return sm_iNextContentRecordID++; }

    // Creates an empty metadata record.
    IMetadata* CreateMetadataRecord() const;

    //! Loads content records from a stream
    //! \param pInputStream An open input stream to read the state from.
    //! \retval true if the state was loaded,
    //! \retval false if an error occured loading state
    bool LoadStateFromStream(IInputStream* pInputStream);

    //! Saves content records to a stream.
    //! \param pOutputStream An open output stream to write the state to.
    //! \retval true if the state was saved,
    //! \retval false if an error occured saving state
    bool SaveStateToStream(IOutputStream* pOutputStream);

	// Clears the content manager and deletes its content records.
	void Clear();

    // Saves the database.
    void Commit();

    //! Saves the database only if it's been changed since the last commit.
    //! Returns true if the database was committed, false otherwise.
    bool CommitIfDirty();

    // Adds a list of media and playlist records to the content manager.
    void AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords);

	// Returns the number of media records in the content manager.
	int GetMediaRecordCount();

	// Adds an entry to the content manager.
    // Returns a pointer to the content record.
    // If pbAlreadyExists is not null, then its value is set to true if the content record with
    // the same URL already exists in the content manager, false otherwise.
	IMediaContentRecord* AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists);

	// Removes a record from the content manager.
    // Returns true if the content record was deleted, false if it wasn't found.
	bool DeleteMediaRecord(int iContentRecordID);

	// Removes a record from the content manager.
    // Returns true if the content record was deleted, false if it wasn't found.
    // WARNING: The pContentRecord pointer will be invalid after this operation!
	bool DeleteMediaRecord(IMediaContentRecord* pContentRecord);

    //! Removes the records in the list from the content manager.
    //! All records in the list must come from the same data source.
    //! When the function is done the records deleted from the front of the list will be gone.
    //! \param nRecordsToDelete The number of records to delete from the front of the list.
    //! \param pfnCB A callback function for monitoring progress.  It will be called 2 * records.Size() times.
    //! \param pUserData Data to be passed to the callback function.
    void DeleteMediaRecords(MediaRecordList& records, int nRecordsToDelete, FNDeleteMediaRecordProgressCB* pfnCB, void* pUserData);

	// Returns a pointer to the media content record with the specified record ID.
    // Returns 0 if no record with a matching ID was found.
	IMediaContentRecord* GetMediaRecord(int iContentRecordID);

	// Returns a pointer to the media content record with the specified URL.
    // Returns 0 if no record with a matching URL was found.
	IMediaContentRecord* GetMediaRecord(const char* szURL);

	// Returns the number of playlist records in the content manager.
	int GetPlaylistRecordCount();

    // Adds a playlist record to the content manager.
    // Returns a pointer to the record.
	IPlaylistContentRecord* AddPlaylistRecord(playlist_record_t& playlistRecord);

	// Removes a playlist record from the content manager.
    // Returns true if the content record was deleted, false if it wasn't found.
	bool DeletePlaylistRecord(int iContentRecordID);

    // Removes all records from the given data source from the content manager.
    void DeleteRecordsFromDataSource(int iDataSourceID);

    // Marks all records from the given data source as unverified.
    void MarkRecordsFromDataSourceUnverified(int iDataSourceID);

    // Removes all records from the given data source that are marked as unverified.
    void DeleteUnverifiedRecordsFromDataSource(int iDataSourceID);

    // Returns the string of the artist with the given key.
    // Returns 0 if no artist with a matching key was found.
    const TCHAR* GetArtistByKey(int iArtistKey) const;
    const TCHAR* GetAlbumByKey(int iAlbumKey) const;
    const TCHAR* GetGenreByKey(int iGenreKey) const;

    int GetArtistKey(const TCHAR* szArtist) const;
    int GetAlbumKey(const TCHAR* szAlbum) const;
    int GetGenreKey(const TCHAR* szGenre) const;

    void GetAllMediaRecords(MediaRecordList& records) const;
    void GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const;
    void GetMediaRecords(MediaRecordList& records,
        int iArtistKey,
        int iAlbumKey,
        int iGenreKey,
        int iDataSourceID) const;
    void GetMediaRecordsSorted(MediaRecordList& records,
        int iArtistKey,
        int iAlbumKey,
        int iGenreKey,
        int iDataSourceID,
        int iSortNum,
        va_list vaSort) const;
    void GetMediaRecordsTitleSorted(MediaRecordList& records,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;
    void GetMediaRecordsAlbumSorted(MediaRecordList& records,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;


    void GetArtists(ContentKeyValueVector& keyValues,
        int iAlbumKey,
        int iGenreKey,
        int iDataSourceID) const;
    void GetAlbums(ContentKeyValueVector& keyValues,
        int iArtistKey,
        int iGenreKey,
        int iDataSourceID) const;
    void GetGenres(ContentKeyValueVector& keyValues,
        int iArtistKey,
        int iAlbumKey,
        int iDataSourceID) const;

    //! Returns the number of artists in the content manager.
    int GetArtistCount() const;
    //! Returns the number of albums in the content manager.
    int GetAlbumCount() const;
    //! Returns the number of genres in the content manager.
    int GetGenreCount() const;

	//! Returns a pointer to the playlist content record with the specified URL in the given data source.
    //! Returns 0 if no record with a matching URL was found.
	IPlaylistContentRecord* GetPlaylistRecord(const char* szURL);

    void GetAllPlaylistRecords(PlaylistRecordList& records) const;
    void GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const;

private:

static int sm_iNextContentRecordID;
static int sm_iNextArtistID;
static int sm_iNextAlbumID;
static int sm_iNextGenreID;
static int sm_iNextPlaylistID;

	// Adds an entry to the content manager at the specified index.
    // Returns a pointer to the content record.
	IMediaContentRecord* AddMediaRecordInternal(media_record_info_t& mediaContent, int index);

	//! Removes a record from the content manager.
	void DeleteMediaRecordByRow(int index);

    // Adds an artist to the table, if it isn't already in there.
    // Returns the artist key.
    int AddArtist(const TCHAR* szArtist);
    int AddAlbum(const TCHAR* szAlbum);
    int AddGenre(const TCHAR* szGenre);

    CDJContentManager*  m_pParent;

    c4_Storage  m_mkStorage;
    c4_View     m_vTracks;
    c4_View     m_vArtists;
    c4_View     m_vAlbums;
    c4_View     m_vGenres;
    c4_View     m_vPlaylists;

    c4_IntProp      m_pContentID;
    c4_StringProp   m_pURL;
    c4_TStringProp  m_pTitle;
    c4_TStringProp  m_pArtistName;
    c4_IntProp      m_pArtistID;
    c4_TStringProp  m_pAlbumName;
    c4_IntProp      m_pAlbumID;
    c4_TStringProp  m_pGenreName;
    c4_IntProp      m_pGenreID;
    c4_IntProp      m_pAlbumTrackNumber;
    c4_IntProp      m_pDataSourceID;
    c4_IntProp      m_pCodecID;
    c4_IntProp      m_pRecordPtr;

    c4_View         m_vURLSort;
    c4_View         m_vTitleSort;
    c4_View         m_vAlbumSort;
    c4_IntProp      m_pSortedIndex;

    bool            m_bSavedToFile; // True if this db is saved to file, false if it only used per-run.
    unsigned int    m_uiUncommittedCommitRecordCount;   // The number of records that have been added since the last Commit().
    bool            m_bDirty;       // True if the database has been changed since the last commit.

    // Used for converting URLs to temporary track titles.
    CCDDataSource*  m_pCDDS;
    int             m_iCDDSID;
    int             m_iFatDSID;

    int     m_iUpdateIndex;
    bool    m_bTwoPass;

    // Returns the row index of the media content record with the given URL,
    // or -1 if there is no match.
    int FindByURL(const char* szURL);

//#define USE_URL_INDEX
#ifdef USE_URL_INDEX

    const char* GetMediaRecordURL(int iRecordID);
    const TCHAR* GetMediaRecordTitle(const char* szURL);
    const TCHAR* GetMediaRecordArtist(const char* szURL);
    const TCHAR* GetMediaRecordAlbum(const char* szURL);
    const TCHAR* GetMediaRecordGenre(const char* szURL);
	int GetMediaRecordAlbumTrackNumber(const char* szURL);
    int GetMediaRecordArtistKey(const char* szURL);
    int GetMediaRecordAlbumKey(const char* szURL);
    int GetMediaRecordGenreKey(const char* szURL);
    int GetMediaRecordDataSourceID(const char* szURL);
    int GetMediaRecordCodecID(const char* szURL);

    void SetMediaRecordURL(int iRecordID, const char* szURL);
	void SetMediaRecordTitle(const char* szURL, const TCHAR* szTitle);
	void SetMediaRecordArtist(const char* szURL, const TCHAR* szArtist);
	void SetMediaRecordGenre(const char* szURL, const TCHAR* szGenre);
	void SetMediaRecordAlbum(const char* szURL, const TCHAR* szAlbum);
	void SetMediaRecordAlbumTrackNumber(const char* szURL, int iAlbumTrackNumber);
    void SetMediaRecordDataSourceID(const char* szURL, int iDataSourceID);
    void SetMediaRecordCodecID(const char* szURL, int iCodecID);

    void MergeMediaRecordAttributes(const char* szURL, const IMetadata* pMetadata, bool bOverwrite);

#endif  // USE_URL_INDEX

    const char* GetMediaRecordURL(int iRowIndex);
    const TCHAR* GetMediaRecordTitle(int iRowIndex);
    const TCHAR* GetMediaRecordArtist(int iRowIndex);
    const TCHAR* GetMediaRecordAlbum(int iRowIndex);
    const TCHAR* GetMediaRecordGenre(int iRowIndex);
	int GetMediaRecordAlbumTrackNumber(int iRowIndex);
    int GetMediaRecordArtistKey(int iRowIndex);
    int GetMediaRecordAlbumKey(int iRowIndex);
    int GetMediaRecordGenreKey(int iRowIndex);
    int GetMediaRecordDataSourceID(int iRowIndex);
    int GetMediaRecordCodecID(int iRowIndex);

    void SetMediaRecordURL(int iRowIndex, const char* szURL);
	void SetMediaRecordTitle(int iRowIndex, const TCHAR* szTitle);
	void SetMediaRecordArtist(int iRowIndex, const TCHAR* szArtist);
	void SetMediaRecordGenre(int iRowIndex, const TCHAR* szGenre);
	void SetMediaRecordAlbum(int iRowIndex, const TCHAR* szAlbum);
	void SetMediaRecordAlbumTrackNumber(int iRowIndex, int iAlbumTrackNumber);
    void SetMediaRecordDataSourceID(int iRowIndex, int iDataSourceID);
    void SetMediaRecordCodecID(int iRowIndex, int iCodecID);

    void MergeMediaRecordAttributes(int iRowIndex, const IMetadata* pMetadata, bool bOverwrite);

    const char* GetPlaylistRecordURL(int iRecordID);
    int GetPlaylistRecordDataSourceID(int iRecordID);
    int GetPlaylistRecordPlaylistFormatID(int iRecordID);

    void SetPlaylistRecordURL(int iRecordID, const char* szURL);
    void SetPlaylistRecordDataSourceID(int iRecordID, int iDataSourceID);
    void SetPlaylistRecordPlaylistFormatID(int iRecordID, int iPlaylistFormatID);

     // If there are no more tracks by this artist, then remove that artist record.
    void FlushArtist(int iArtistKey);
    void FlushAlbum(int iAlbumKey);
    void FlushGenre(int iGenreKey);

    // Given a row index in the track table, return the row index in the sorted table.
    int GetTitleSortedIndex(int row) const;
    int GetAlbumSortedIndex(int row) const;

    // Repositions a given row in the sorted table.
    void RepositionTitleSortedRow(int row);
    void RepositionAlbumSortedRow(int row);

    // For debugging.
    void PrintTracks() const;
    void PrintURLSort() const;
    void PrintTitleSort() const;
    void PrintAlbumSort() const;

friend class CMetakitMediaContentRecord;
friend class CMetakitPlaylistContentRecord;

};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitMediaContentRecord : public IMediaContentRecord
#ifdef USE_FIXED_ALLOCATOR
    , public FixedAllocator<CMetakitMediaContentRecord, 11000>
#endif  // USE_FIXED_ALLOCATOR
{
public:

    CMetakitMediaContentRecord(CDJContentManagerImp* pManager, int iContentRecordID,
        const char* szURL, IMetadata* pMetadata, bool bVerified, int index);
    ~CMetakitMediaContentRecord();

    int GetIndex() const
        { return m_index; }
    void SetIndex(int index)
        { m_index = index; }

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const;
    int GetDataSourceID() const;
    // We've ditched verified/unverified updates in the DJ, so this can be optimized.
    bool IsVerified() const
        { return true; }
    void SetVerified(bool bVerified)
        { }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IMediaContentRecord functions
    const TCHAR* GetTitle() const;
    const TCHAR* GetAlbum() const;
    const TCHAR* GetGenre() const;
    const TCHAR* GetArtist() const;
	int GetAlbumTrackNumber() const;
    int GetCodecID() const;

    int GetArtistKey() const;
    int GetAlbumKey() const;
    int GetGenreKey() const;

	void SetTitle(const TCHAR* szTitle);
	void SetAlbum(const TCHAR* szAlbum);
	void SetGenre(const TCHAR* szGenre);
	void SetArtist(const TCHAR* szArtist);
	void SetAlbumTrackNumber(int iAlbumTrackNumber);
	void SetDataSourceID(int iDataSourceID);
	void SetCodecID(int iCodecID);

    // IMetadata functions
    IMetadata* Copy() const;
    bool UsesAttribute(int iAttributeID) const;
    ERESULT SetAttribute(int iAttributeID, void* pAttributeValue);
    ERESULT UnsetAttribute(int iAttributeID) { return METADATA_NOT_USED; };
    ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const;
    void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false);
    void ClearAttributes() { };

protected:

    int         m_iID;      // Unique content record ID
    int         m_index;    // Row index in tracks table.
#ifdef USE_URL_INDEX
    char*       m_szURL;
#endif  // USE_URL_INDEX
    CDJContentManagerImp* m_pManager;
    ContentRecordStatus m_eStatus;

};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////


class CMetakitPlaylistContentRecord : public IPlaylistContentRecord
{
public:

    CMetakitPlaylistContentRecord(CDJContentManagerImp* pManager, int iContentRecordID, bool bVerified);
	~CMetakitPlaylistContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const;
	void SetDataSourceID(int iDataSourceID);
	int GetDataSourceID() const;
    // We've ditched verified/unverified updates in the DJ, so this can be optimized.
    bool IsVerified() const
        { return true; }
    void SetVerified(bool bVerified)
        { }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IPlaylistContentRecord functions
    void SetPlaylistFormatID(int iPlaylistFormatID);
	int GetPlaylistFormatID() const;

private:

    int     m_iID;  // Unique content record ID
    CDJContentManagerImp* m_pManager;
    ContentRecordStatus m_eStatus;
};


#endif	// DJCONTENTMANAGERIMP_H_
