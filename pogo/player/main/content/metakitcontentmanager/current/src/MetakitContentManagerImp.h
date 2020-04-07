//
// MetakitContentManagerImp.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef METAKITCONTENTMANAGERIMP_H_
#define METAKITCONTENTMANAGERIMP_H_

#include <main/content/metakitcontentmanager/MetakitContentManager.h>

#include <util/datastructures/SimpleMap.h>
#include <util/metakit/mk4.h>
#include <util/metakit/mk4str.h>
#include <util/tchar/tchar.h>

typedef c4_WideStringProp c4_TStringProp;

//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitContentManagerImp
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitContentManagerImp : public IQueryableContentManager
{
public:
#ifdef INCREMENTAL_METAKIT
    CMetakitContentManagerImp(const char* szFilename);
#else
    CMetakitContentManagerImp();
#endif
    ~CMetakitContentManagerImp();

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
    void AddContentRecords(content_record_update_t* pContentUpdate);

    void CompareFileSystemUpdate( CFileNameStore* store );
    CFileNameStore* GetFileNameStore();

	// Returns the number of media records in the content manager.
	int GetMediaRecordCount();

    // locate the relevant content, merge any new metadata, and return the record.  if not found, return 0.
    IMediaContentRecord* MergeMetadata(media_record_info_t& mediaContent);

	// Adds an entry to the content manager.
    // Returns a pointer to the content record.
    // If pbAlreadyExists is not null, then its value is set to true if the content record with
    // the same URL already exists in the content manager, false otherwise.
	IMediaContentRecord* AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists = 0);

	// Removes a record from the content manager.
	void DeleteMediaRecord(int iContentRecordID);

	// Returns a pointer to the media content record with the specified record ID.
    // Returns 0 if no record with a matching ID was found.
	IMediaContentRecord* GetMediaRecord(int iContentRecordID);

	// Returns a pointer to the media content record with the specified URL.
    // Returns 0 if no record with a matching URL was found.
    IMediaContentRecord* GetMediaRecord(const char* szURL);

	IMediaContentRecord* GetMediaRecord(IFileNameRef* file);

	// Returns the number of playlist records in the content manager.
	int GetPlaylistRecordCount();

    // Adds a playlist record to the content manager.
    // Returns a pointer to the record.
	IPlaylistContentRecord* AddPlaylistRecord(playlist_record_t& playlistRecord);

	// Removes a playlist record from the content manager.
	void DeletePlaylistRecord(int iContentRecordID);

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
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;


    void GetArtists(ContentKeyValueVector& keyValues,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;
    void GetAlbums(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;
    void GetGenres(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;

    void GetAllPlaylistRecords(PlaylistRecordList& records) const;
    void GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const;

    c4_Storage* GetStorage();

private:
static int sm_iNextContentRecordID;
static int sm_iNextArtistID;
static int sm_iNextAlbumID;
static int sm_iNextGenreID;
static int sm_iNextPlaylistID;

typedef struct md_property_holder_s
{
    int             iAttributeID;
    int             iAttributeType;
    void*           pUnsetValue;
    char*           szAttributeName;
    c4_Property*    pProp;
    md_property_holder_s(int theAttributeID, int theAttributeType, void* theUnsetValue,
        char* theAttributeName, c4_Property* theProp)
        : iAttributeID(theAttributeID), iAttributeType(theAttributeType), pUnsetValue(theUnsetValue),
        szAttributeName(theAttributeName), pProp(theProp) { }
    md_property_holder_s()
        : iAttributeID(0), iAttributeType(0), pUnsetValue(0),
        szAttributeName(0), pProp(0) { }
} md_property_holder_t;
typedef SimpleMap<int, md_property_holder_t> PropMap;
    PropMap     m_propMap;

    // Adds an artist to the table, if it isn't already in there.
    // Returns the artist key.
    int AddArtist(const TCHAR* szArtist);
    int AddAlbum(const TCHAR* szAlbum);
    int AddGenre(const TCHAR* szGenre);

    c4_Storage  m_mkStorage;
    c4_View     m_vTracks;
    c4_View     m_vArtists;
    c4_View     m_vAlbums;
    c4_View     m_vGenres;
    c4_View     m_vPlaylists;

    c4_IntProp      m_pContentID;
    c4_IntProp      m_ipFileNamePtr;
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

    char*       m_szTrackSchema;
    void MakeTrackSchema();

    IFileNameRef* GetMediaRecordFileNameRef(int iRecordID);
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

    void SetMediaRecordFileNameRef(int iRecordID, IFileNameRef* file);
	void SetMediaRecordTitle(int iRecordID, const TCHAR* szTitle);
	void SetMediaRecordArtist(int iRecordID, const TCHAR* szArtist);
	void SetMediaRecordGenre(int iRecordID, const TCHAR* szGenre);
	void SetMediaRecordAlbum(int iRecordID, const TCHAR* szAlbum);
    void SetMediaRecordDataSourceID(int iRecordID, int iDataSourceID);
    void SetMediaRecordCodecID(int iRecordID, int iCodecID);
    ERESULT SetMediaRecordAttribute(int iRecordID, int iAttributeID, void* pAttributeValue);

    IFileNameRef* GetPlaylistRecordFileNameRef(int iRecordID);
    int GetPlaylistRecordDataSourceID(int iRecordID);
    int GetPlaylistRecordPlaylistFormatID(int iRecordID);

    void SetPlaylistRecordFileNameRef(int iRecordID, IFileNameRef* file);
    void SetPlaylistRecordDataSourceID(int iRecordID, int iDataSourceID);
    void SetPlaylistRecordPlaylistFormatID(int iRecordID, int iPlaylistFormatID);

     // If there are no more tracks by this artist, then remove that artist record.
    void FlushArtist(int iArtistKey);
    void FlushAlbum(int iAlbumKey);
    void FlushGenre(int iGenreKey);

    CFileNameStore* m_pFileNameStore;

friend class CMetakitMediaContentRecord;
friend class CMetakitPlaylistContentRecord;
    
    
    void DebugPrintTrackListing();
};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

class CMetakitMediaContentRecord : public IMediaContentRecord
{
public:
    CMetakitMediaContentRecord(CMetakitContentManagerImp* pManager, int iContentRecordID,
        IMetadata* pMetadata, bool bVerified);
    ~CMetakitMediaContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    IFileNameRef* GetFileNameRef() const;
    char* GetURL() const;
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
    void SetFileNameRef(IFileNameRef*);

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
    CMetakitContentManagerImp* m_pManager;
    IMetadata*  m_pMetadata;
    int         m_iDataSourceID;
    int         m_iCodecID;
    bool        m_bVerified;
    IFileNameRef* m_pFileNameRef;
    int         m_idxFileName;
    ContentRecordStatus m_eStatus;
};


//////////////////////////////////////////////////////////////////////////////////////////
//	CMetakitPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////


class CMetakitPlaylistContentRecord : public IPlaylistContentRecord
{
public:

    CMetakitPlaylistContentRecord(CMetakitContentManagerImp* pManager, int iContentRecordID, bool bVerified);
	~CMetakitPlaylistContentRecord();

    // IContentRecord functions
    int GetID() const
        { return m_iID; }
    IFileNameRef* GetFileNameRef() const;
    char* GetURL() const;
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
    void SetFileNameRef(IFileNameRef*);

private:

    int     m_iID;  // Unique content record ID
    CMetakitContentManagerImp* m_pManager;
    int     m_iPlaylistFormatID;
    bool    m_bVerified;
    IFileNameRef* m_pFileNameRef;
    ContentRecordStatus m_eStatus;
};


#endif	// METAKITCONTENTMANAGERIMP_H_
