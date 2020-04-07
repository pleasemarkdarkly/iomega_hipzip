//
// ContentManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Both media files and playlist files are tracked by a content manager in the
//! Dadio(tm) system.  The IContentManager interface describes functions for
//! simple addition and retrieval of records.  The IQueryableContentManager
//! interface adds functions for creating lists of media records based on
//! artist, album, and genre metadata.
/** \addtogroup ContentManager Content Manager */
//@{

#ifndef CONTENTMANAGER_H_
#define CONTENTMANAGER_H_

#include <content/common/Metadata.h>
#include <util/datastructures/SimpleList.h>
#include <util/datastructures/SimpleVector.h>
#include <util/tchar/tchar.h>

class IContentRecord;
class IDataSource;
class IMediaContentRecord;
class IPlaylist;
class IPlaylistContentRecord;

//////////////////////////////////////////////////////////////////////////////////////////
//	Content record update structs
//////////////////////////////////////////////////////////////////////////////////////////

//! Used for passing information about a track available from the data source to
//! the content manager (and back to the data source manager for metadata retrieval
//! in a two-pass update).
typedef struct media_record_info_s
{
    char*   szURL;          //!< The unique URL of this media record
    bool    bVerified;      //!< True if this record has been verified to exist on the data source
    int     iDataSourceID;  //!< ID of the data source that hosts the content
    int     iCodecID;       //!< ID of the matching codec of this content
    IMetadata*  pMetadata;  //!< Associated metadata; can be 0
} media_record_info_t;

typedef SimpleVector<media_record_info_t> IMediaRecordInfoVector;

//! Used for passing information about a playlist file available from the data source to
//! the content manager.
typedef struct playlist_record_s
{
    char*   szURL;          //!< The unique URL of this playlist record
    bool    bVerified;      //!< True if this record has been verified to exist on the data source
    int     iDataSourceID;  //!< ID of the data source that hosts the playlist
    int	iPlaylistFormatID;  //!< ID of the matching playlist format
} playlist_record_t;

typedef SimpleVector<playlist_record_t> IPlaylistRecordInfoVector;

//! Used for passing information about the media and playlist records available from a data source to
//! the content manager.
typedef struct content_record_update_s
{
    int                         iDataSourceID;  //!< Data source that supplied the content
    bool                        bTwoPass;       //!< Pass-through option specified by the type of search.
                                                //!< If true, then when AddContentRecords is called
                                                //!< the content manager should prune the media record info
                                                //!< vector to contain only records whose metadata needs retrieving.
    IMediaRecordInfoVector      media;          //!< List of media content records to add/update
    IPlaylistRecordInfoVector   playlists;      //!< List of playlist content records to add/update
    unsigned short              usScanID;       //!< ID assigned by the data source manager to this scan cycle
} content_record_update_t;

typedef SimpleList<IMediaContentRecord*> MediaRecordList;
typedef SimpleListIterator<IMediaContentRecord*> MediaRecordIterator;

typedef SimpleList<IPlaylistContentRecord*> PlaylistRecordList;
typedef SimpleListIterator<IPlaylistContentRecord*> PlaylistRecordIterator;

//////////////////////////////////////////////////////////////////////////////////////////
//	IContentManager
//////////////////////////////////////////////////////////////////////////////////////////

//! The IContentManager provides record tracking for media content accross several
//! data sources.
class IContentManager
{
public:

	virtual ~IContentManager()
        { }

    //! Creates an empty metadata record.
    virtual IMetadata* CreateMetadataRecord() const = 0;

	//! Clears the content manager and deletes its content records.
	virtual void Clear() = 0;

    //! Adds a list of media and playlist records to the content manager.
    //! If pRecords is non-zero, then it will be filled with a list of pointers to the new records.
    virtual void AddContentRecords(content_record_update_t* pContentUpdate, MediaRecordList* pRecords = 0) = 0;

	//! Returns the number of media records in the content manager.
	virtual int GetMediaRecordCount() = 0;

    //! Adds a media entry to the content manager.
    //! Returns a pointer to the content record.
    //! Whatever the media_record_info_t's pMetadata field points to becomes property of
    //! the content manager after this call, and cannot be used again by the caller.
    //! If pbAlreadyExists is not null, then its value is set to true if the content record with
    //! the same URL already exists in the content manager, false otherwise.
    virtual IMediaContentRecord* AddMediaRecord(media_record_info_t& mediaContent, bool* pbAlreadyExists = 0) = 0;

    //! Removes a media record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
    virtual bool DeleteMediaRecord(int iContentRecordID) = 0;

	//! Removes a record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
    //! WARNING: The pContentRecord pointer will be invalid after this operation!
	virtual bool DeleteMediaRecord(IMediaContentRecord* pContentRecord) = 0;

	//! Returns a pointer to the media content record with the specified record ID.
    //! Returns 0 if no record with a matching ID was found.
	virtual IMediaContentRecord* GetMediaRecord(int iContentRecordID) = 0;

	//! Returns a pointer to the media content record with the specified URL.
    //! Returns 0 if no record with a matching URL was found.
	virtual IMediaContentRecord* GetMediaRecord(const char* szURL) = 0;

	//! Returns the number of playlist records in the content manager.
	virtual int GetPlaylistRecordCount() = 0;

    //! Adds a playlist record to the content manager.
    //! Returns a pointer to the record.
	virtual IPlaylistContentRecord* AddPlaylistRecord(playlist_record_t& playlistRecord) = 0;

	//! Removes a playlist record from the content manager.
    //! Returns true if the content record was deleted, false if it wasn't found.
	virtual bool DeletePlaylistRecord(int iContentRecordID) = 0;

    //! Removes all records from the given data source from the content manager.
    virtual void DeleteRecordsFromDataSource(int iDataSourceID) = 0;

    //! Marks all records from the given data source as unverified.
    virtual void MarkRecordsFromDataSourceUnverified(int iDataSourceID) = 0;

    //! Removes all records from the given data source that are marked as unverified.
    virtual void DeleteUnverifiedRecordsFromDataSource(int iDataSourceID) = 0;

    //! Appends all media content records in the manager to the record list passed in.
    virtual void GetAllMediaRecords(MediaRecordList& records) const = 0;
    //! Appends all media content records from the given data source in the manager to the record list passed in.
    virtual void GetMediaRecordsByDataSourceID(MediaRecordList& records, int iDataSourceID) const = 0;
    //! Appends all playlist content records in the manager to the record list passed in.
    virtual void GetAllPlaylistRecords(PlaylistRecordList& records) const = 0;
    //! Appends all playlist content records from the given data source in the manager to the record list passed in.
    virtual void GetPlaylistRecordsByDataSourceID(PlaylistRecordList& records, int iDataSourceID) const = 0;

};

//////////////////////////////////////////////////////////////////////////////////////////
//	IContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

//! The base class for both media content records and playlist content records.
//! The content record has an ID unique across the content manager, a URL unique across
//! the system, and the ID of the data source it's stored on.  Content records are also
//! considered verified if they've been found to reside on a data source (as opposed to
//! solely existing in the database from a previous run).  They also have a status which
//! specifies their ability to be run through a codec for playback.
class IContentRecord
{
public:

    //! Describes the playability status of this content record.
typedef enum ContentRecordStatus {
        CR_OKAY = 0,        /*!< The content record is either good or hasn't been tested. \hideinitializer */
        CR_DRM_FAILURE,     /*!< The content record can't be played because of licensing issues. */
        CR_DECODE_ERROR,    /*!< Errors in decoding prevent this track from being played. */
        CR_NO_CODEC         /*!< No matching codec for this content record was found. */
    };

	virtual ~IContentRecord()
        { }

    //! Returns the ID of the content record, which must be unique across records in the manager.
	virtual int GetID() const = 0;

    //! Returns the unique URL of the content record.
    virtual const char* GetURL() const = 0;
    //! Returns the ID of the data source this content record comes from.
	virtual int GetDataSourceID() const = 0;

    //! Returns true if the file has been verified as existing on its data source.
    virtual bool IsVerified() const = 0;
    //! Set this content records as verified (i.e., existing on a data source) or unverified.
    virtual void SetVerified(bool bVerified) = 0;

    //! Gets the status of the content record.
    virtual ContentRecordStatus GetStatus() const = 0;
    //! Sets the status of the content record.
    //! Called primarily from the media player during SetSong.
    //! This is separate from verified/unverified so that bad content can be left in the content
    //! manager after refreshes (instead of flushing bad content and adding it again on the refresh).
    virtual void SetStatus(ContentRecordStatus status) = 0;
};


//////////////////////////////////////////////////////////////////////////////////////////
//	IMediaContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

//! A subclass of both IContentRecord and IMetadata, used for representing tracks
//! that can be decoded by the media player.  In addition to the functionality
//! of the IContentRecord and IMetadata classes, the media content record also
//! retains a codec ID.
class IMediaContentRecord : public IContentRecord, public IMetadata
{
public:

	virtual ~IMediaContentRecord()
        { }

    //! Returns the ID of the codec used to decode this track.
	virtual int GetCodecID() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
//	IPlaylistContentRecord
//////////////////////////////////////////////////////////////////////////////////////////

//! A subclass of IContentRecord, used for representing playlist files that can
//! can be loaded by the playlist format manager.  In addition to the functionality
//! of the IContentRecord class, the playlist content record also
//! retains a playlist format ID.
class IPlaylistContentRecord : public IContentRecord
{
public:

	virtual ~IPlaylistContentRecord()
        { }

    //! Returns the ID of the playlist format used to load/save this playlist.
	virtual int GetPlaylistFormatID() const = 0;

};



//@}

#endif	// CONTENTMANAGER_H_
