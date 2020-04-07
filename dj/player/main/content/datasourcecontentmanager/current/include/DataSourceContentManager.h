//
// DataSourceContentManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DATASOURCECONTENTMANAGER_H_
#define DATASOURCECONTENTMANAGER_H_

#include <content/common/QueryableContentManager.h>

#include <util/tchar/tchar.h>

class IInputStream;
class CDataSourceContentManagerImp;
class IOutputStream;

#define METAKIT_CONTENT_MANAGER_ERROR_ZONE 0x0e

#define MAKE_CCMRESULT(x,y) MAKE_ERESULT(x,METAKIT_CONTENT_MANAGER_ERROR_ZONE,y)

const ERESULT MCM_NO_ERROR              = MAKE_CCMRESULT( SEVERITY_SUCCESS, 0x0000 );
const ERESULT MCM_ERROR                 = MAKE_CCMRESULT( SEVERITY_FAILED,  0x0001 );
const ERESULT MCM_UNKNOWN_METADATA_ID   = MAKE_CCMRESULT( SEVERITY_FAILED,  0x0002 );
const ERESULT MCM_UNKNOWN_METADATA_TYPE = MAKE_CCMRESULT( SEVERITY_FAILED,  0x0003 );


#include <util/datastructures/SimpleMap.h>
#include <util/metakit/mk4.h>
#include <util/metakit/mk4str.h>


//////////////////////////////////////////////////////////////////////////////////////////
//	CDataSourceContentManager
//////////////////////////////////////////////////////////////////////////////////////////

//! The metakit content manager is a concrete implementation of the IContentManager interface.
//! Content records are stored in the metakit database and can be queried by artist, album,
//! and genre.
//! Functions for loading and saving the content manager's state are also provided.
//! This class has its own metadata type that stores title, artist, album, and genre.  More
//! metadata attributes can be added through the AddStoredMetadata function.  Attributes added
//! this way will persist when the database is saved.
//! \warning If used in persistent mode, then it must be created AFTER fat system initialization

class CDataSourceContentManager : public IQueryableContentManager
{
public:
    CDataSourceContentManager(const char* szFilename);
    CDataSourceContentManager();
    ~CDataSourceContentManager();

    bool AddDataSource(int iDataSourceID, const char* szFilename);
    bool AddDataSource(int iDataSourceID);

    //! Tells the content manager to use the given metadata attribute.
    //! \param iAttributeID The ID of the attribute to add.  If this attribute isn't a
    //! default attribute that came with the SDK, then it must have been added to the CMetadataTable.
    //! \param pUnsetValue Value to be used inside the content manager to represent that
    //! the value of this attribute has not been set.  (That is, when GetAttribute() is called,
    //! if the value of the attribute is equal to pUnsetValue, then METADATA_NO_VALUE_SET is returned.)
    //! \retval MCM_UNKNOWN_METADATA_ID The attribute ID isn't in the CMetadataTable.
    //! \retval MCM_UNKNOWN_METADATA_TYPE The attribute has a bad type in the CMetadataTable.
    //! \retval MCM_NO_ERROR The attribute was added successfully.
    ERESULT AddStoredMetadata(int iAttributeID, const void* pUnsetValue);

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

	//! Clears the content manager and deletes its content records.
	void Clear();

    //! Saves the database.
    void Commit();

    //! Saves the database with the given data source ID.
    void CommitFromDataSource(int iDataSourceID);

    //! Adds a list of media and playlist records to the content manager.
    //! If pRecords is non-zero, then it will be filled with a list of pointers to the new records.
    //! WARNING: This function has been optimized for the content update cycle.
    //! IT IS NOT FOR GENERAL PURPOSE USE!  Use AddMediaRecord instead.
    void AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords = 0);

	//! Returns the number of media records in the content manager.
	int GetMediaRecordCount();

	//! Adds an entry to the content manager.
    //! Returns a pointer to the content record.
    //! Whatever the media_record_info_t's pMetadata field points to becomes property of
    //! the content manager after this call, and cannot be used again by the caller.
    //! If pbAlreadyExists is not null, then its value is set to true if the content record with
    //! the same URL already exists in the content manager, false otherwise.
	IMediaContentRecord* AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists = 0);

	//! Removes a record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
	bool DeleteMediaRecord(int iContentRecordID);

	//! Returns a pointer to the media content record with the specified record ID.
    //! Returns 0 if no record with a matching ID was found.
	IMediaContentRecord* GetMediaRecord(int iContentRecordID);

	//! Returns a pointer to the media content record with the specified URL.
    //! Returns 0 if no record with a matching URL was found.
	IMediaContentRecord* GetMediaRecord(const char* szURL);

	//! Returns the number of playlist records in the content manager.
	int GetPlaylistRecordCount();

    //! Adds a playlist record to the content manager.
    //! Returns a pointer to the record.
	IPlaylistContentRecord* AddPlaylistRecord(playlist_record_t& playlistRecord);

	//! Removes a playlist record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
	bool DeletePlaylistRecord(int iContentRecordID);

    //! Removes all records from the given data source from the content manager.
    void DeleteRecordsFromDataSource(int iDataSourceID);

    //! Marks all records from the given data source as unverified.
    void MarkRecordsFromDataSourceUnverified(int iDataSourceID);

    //! Removes all records from the given data source that are marked as unverified.
    void DeleteUnverifiedRecordsFromDataSource(int iDataSourceID);

    //! Returns the string of the artist with the given key.
    //! Returns 0 if no artist with a matching key was found.
    const TCHAR* GetArtistByKey(int iArtistKey) const;
    //! Returns the string of the album with the given key.
    //! Returns 0 if no album with a matching key was found.
    const TCHAR* GetAlbumByKey(int iAlbumKey) const;
    //! Returns the string of the genre with the given key.
    //! Returns 0 if no genre with a matching key was found.
    const TCHAR* GetGenreByKey(int iGenreKey) const;

    //! Returns the artist key that matches the artist string passed in, or 0 if no matching artist was found.
    int GetArtistKey(const TCHAR* szArtist) const;
    //! Returns the album key that matches the album string passed in, or 0 if no matching album was found.
    int GetAlbumKey(const TCHAR* szAlbum) const;
    //! Returns the genre key that matches the genre string passed in, or 0 if no matching genre was found.
    int GetGenreKey(const TCHAR* szGenre) const;

    //! Appends all media content records in the manager to the record list passed in.
    void GetAllMediaRecords(MediaRecordList& records) const;
    //! Appends all media content records from the given data source in the manager to the record list passed in.
    void GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const;
    //! Searches the content manager for media records that match the artist, album, genre, and data source
    //! keys.
    //! \param records List used to append matching records.
    void GetMediaRecords(MediaRecordList& records,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;

    //! Searches the content manager for media records that match the artist, album, genre, and data source
    //! keys.
    //! \param records List used to append matching records.
    //! \param iSortNum The number of attribute IDs to be used for sorting.
    //! \param ... List of attribute IDs to sort on.
    void GetMediaRecordsSorted(MediaRecordList& records,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL,
        int iSortNum = 0,
        ...) const;


    //! Searches the content manager for artists that have tracks with the specified album, genre,
    //! and data source keys.
    //! \param keyValues List used to append matching artist key/value pairs.
    void GetArtists(ContentKeyValueVector& keyValues,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;
    //! Searches the content manager for albums that have tracks with the specified artist, genre,
    //! and data source keys.
    //! \param keyValues List used to append matching album key/value pairs.
    void GetAlbums(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;
    //! Searches the content manager for genre that have tracks with the specified artist, album,
    //! and data source keys.
    //! \param keyValues List used to append matching genre key/value pairs.
    void GetGenres(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const;

    //! Appends all playlist content records in the manager to the record list passed in.
    void GetAllPlaylistRecords(PlaylistRecordList& records) const;
    //! Appends all playlist content records from the given data source in the manager to the record list passed in.
    void GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const;


    // Should be moved from public, if a true implementation class is ever created.

    const char* GetTrackSchema() const
        { return m_szTrackSchema; }

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

private:

#define MAX_DATA_SOURCE_COUNT   5

    CDataSourceContentManagerImp*   m_vDS[MAX_DATA_SOURCE_COUNT];

    PropMap     m_propMap;


    char*   m_szTrackSchema;
    void MakeTrackSchema();

public:

    PropMap* GetPropertyMap()
        { return &m_propMap; }

};


#endif	// DATASOURCECONTENTMANAGER_H_
