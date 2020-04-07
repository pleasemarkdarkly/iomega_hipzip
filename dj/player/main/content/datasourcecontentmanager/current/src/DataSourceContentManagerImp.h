//
// DataSourceContentManagerImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DATASOURCECONTENTMANAGERIMP_H_
#define DATASOURCECONTENTMANAGERIMP_H_

#include <main/content/datasourcecontentmanager/DataSourceContentManager.h>
#include <stdarg.h>

typedef c4_WideStringProp c4_TStringProp;
class CDataSourceContentManager;

//////////////////////////////////////////////////////////////////////////////////////////
//	CDataSourceContentManagerImp
//////////////////////////////////////////////////////////////////////////////////////////

class CDataSourceContentManagerImp : public IQueryableContentManager
{
public:
    CDataSourceContentManagerImp(CDataSourceContentManager* pParent, const char* szFilename);
    CDataSourceContentManagerImp(CDataSourceContentManager* pParent);
    ~CDataSourceContentManagerImp();

    ERESULT AddStoredMetadata(int iAttributeID, const void* pUnsetValue);
    ERESULT AddUnstoredMetadata(int iAttributeID);

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

    // Adds an artist to the table, if it isn't already in there.
    // Returns the artist key.
    int AddArtist(const TCHAR* szArtist);
    int AddAlbum(const TCHAR* szAlbum);
    int AddGenre(const TCHAR* szGenre);

    CDataSourceContentManager*  m_pParent;

    c4_Storage  m_mkStorage;
    c4_View     m_vTracksRaw;
    c4_View     m_vTracksHash;
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
    c4_IntProp      m_pDataSourceID;
    c4_IntProp      m_pCodecID;
    c4_IntProp      m_pRecordPtr;

    int     m_iUpdateIndex;
    bool    m_bTwoPass;

    char*       m_szTrackSchema;
    void MakeTrackSchema();

//#define USE_HASHED
#ifdef USE_HASHED
    const char* GetMediaRecordURL(int iRecordID);
    const TCHAR* GetMediaRecordTitle(const char* szURL);
    const TCHAR* GetMediaRecordArtist(const char* szURL);
    const TCHAR* GetMediaRecordAlbum(const char* szURL);
    const TCHAR* GetMediaRecordGenre(const char* szURL);
    int GetMediaRecordArtistKey(const char* szURL);
    int GetMediaRecordAlbumKey(const char* szURL);
    int GetMediaRecordGenreKey(const char* szURL);
    int GetMediaRecordDataSourceID(const char* szURL);
    int GetMediaRecordCodecID(const char* szURL);
    ERESULT GetMediaRecordAttribute(const char* szURL, int iAttributeID, void** ppAttributeValue);

    void SetMediaRecordURL(int iRecordID, const char* szURL);
	void SetMediaRecordTitle(const char* szURL, const TCHAR* szTitle);
	void SetMediaRecordArtist(const char* szURL, const TCHAR* szArtist);
	void SetMediaRecordGenre(const char* szURL, const TCHAR* szGenre);
	void SetMediaRecordAlbum(const char* szURL, const TCHAR* szAlbum);
    void SetMediaRecordDataSourceID(const char* szURL, int iDataSourceID);
    void SetMediaRecordCodecID(const char* szURL, int iCodecID);
    ERESULT SetMediaRecordAttribute(const char* szURL, int iAttributeID, void* pAttributeValue);
#else   // USE_HASHED
    const char* GetMediaRecordURL(int iRecordID);
    const TCHAR* GetMediaRecordTitle(int iRecordID);
    const TCHAR* GetMediaRecordArtist(int iRecordID);
    const TCHAR* GetMediaRecordAlbum(int iRecordID);
    const TCHAR* GetMediaRecordGenre(int iRecordID);
    int GetMediaRecordArtistKey(int iRecordID);
    int GetMediaRecordAlbumKey(int iRecordID);
    int GetMediaRecordGenreKey(int iRecordID);
    int GetMediaRecordDataSourceID(int iRecordID);
    int GetMediaRecordCodecID(int iRecordID);
    ERESULT GetMediaRecordAttribute(int iRecordID, int iAttributeID, void** ppAttributeValue);

    void SetMediaRecordURL(int iRecordID, const char* szURL);
	void SetMediaRecordTitle(int iRecordID, const TCHAR* szTitle);
	void SetMediaRecordArtist(int iRecordID, const TCHAR* szArtist);
	void SetMediaRecordGenre(int iRecordID, const TCHAR* szGenre);
	void SetMediaRecordAlbum(int iRecordID, const TCHAR* szAlbum);
    void SetMediaRecordDataSourceID(int iRecordID, int iDataSourceID);
    void SetMediaRecordCodecID(int iRecordID, int iCodecID);
    ERESULT SetMediaRecordAttribute(int iRecordID, int iAttributeID, void* pAttributeValue);
#endif  // USE_HASHED

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

friend class CMetakitMediaContentRecord;
friend class CMetakitPlaylistContentRecord;

};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitMediaContentRecord : public IMediaContentRecord
{
public:
    CMetakitMediaContentRecord(CDataSourceContentManagerImp* pManager, int iContentRecordID,
        const char* szURL, IMetadata* pMetadata, bool bVerified);
    ~CMetakitMediaContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const;
    int GetDataSourceID() const;
    bool IsVerified() const
        { return m_bVerified; }
    void SetVerified(bool bVerified)
        { m_bVerified = bVerified; }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IMediaContentRecord functions
    const TCHAR* GetTitle() const;
    const TCHAR* GetAlbum() const;
    const TCHAR* GetGenre() const;
    const TCHAR* GetArtist() const;
    int GetCodecID() const;

    int GetArtistKey() const;
    int GetAlbumKey() const;
    int GetGenreKey() const;

	void SetTitle(const TCHAR* szTitle);
	void SetAlbum(const TCHAR* szAlbum);
	void SetGenre(const TCHAR* szGenre);
	void SetArtist(const TCHAR* szArtist);
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

    int         m_iID;  // Unique content record ID
    char*       m_szURL;
    CDataSourceContentManagerImp* m_pManager;
    IMetadata*  m_pMetadata;
    int         m_iDataSourceID;
    int         m_iCodecID;
    bool        m_bVerified;
    ContentRecordStatus m_eStatus;
};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////


class CMetakitPlaylistContentRecord : public IPlaylistContentRecord
{
public:

    CMetakitPlaylistContentRecord(CDataSourceContentManagerImp* pManager, int iContentRecordID, bool bVerified);
	~CMetakitPlaylistContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    const char* GetURL() const;
	void SetDataSourceID(int iDataSourceID);
	int GetDataSourceID() const;
    bool IsVerified() const
        { return m_bVerified; }
    void SetVerified(bool bVerified)
        { m_bVerified = bVerified; }
    ContentRecordStatus GetStatus() const
        { return m_eStatus; }
    void SetStatus(ContentRecordStatus status)
        { m_eStatus = status; }

    // IPlaylistContentRecord functions
    void SetPlaylistFormatID(int iPlaylistFormatID);
	int GetPlaylistFormatID() const;

private:

    int     m_iID;  // Unique content record ID
    CDataSourceContentManagerImp* m_pManager;
    int     m_iPlaylistFormatID;
    bool    m_bVerified;
    ContentRecordStatus m_eStatus;
};


#endif	// DATASOURCECONTENTMANAGERIMP_H_
