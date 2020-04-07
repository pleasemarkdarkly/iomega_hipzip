//
// File Name: DataSource.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DATASOURCE_H_
#define DATASOURCE_H_

class IContentManager;
class IContentRecord;
class IInputStream;
class IOutputStream;
class IPlaylist;
typedef struct content_record_update_s content_record_update_t;

#include <util/eresult/eresult.h>

//! The data source is an abstract representation of a provider of content.
//! It has methods for obtaining a list of content on the data source, fetching metadata
//! for that content, and creating input streams for a given piece of content.
class IDataSource
{
public:

    IDataSource(int iClassID)
        { }
    virtual ~IDataSource()
        { }

    //! The refresh mode specifies how a list of content is to be obtained from the data source.
    typedef enum RefreshMode
        {
            DSR_DEFAULT = 0,            //!< The default method for the data source, chosen from one of the settings below.
            DSR_ONE_PASS,               //!< List all available entries in one pass without fetching metadata.
            DSR_ONE_PASS_WITH_METADATA, //!< List all available entries in one pass and try to fetch metadata
            //!< for each entry.
            DSR_TWO_PASS                //!< List all available entries in one pass without fetching metadata.
            //!< The play manager will then call GetMetadata for each new entry
            //!< in the content manager.
        };
#define DS_DEFAULT_CHUNK_SIZE -1    //!< Use the default update chunk size for the data source.

    //! Returns an ID unique to the type of data source instantiated by the object.
    virtual int GetClassID() const = 0;
    //! Returns an ID unique to the instantiation of the object.
    virtual int GetInstanceID() const = 0;

    //! Copies the string the data source uses to prefix its URLs into the given string provided.
    virtual bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const = 0;

    //! Sets the default refresh mode of the data source.
    //! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
    virtual void SetDefaultRefreshMode(RefreshMode mode) = 0;
    //! Returns the current default refresh mode of the data source.
    virtual RefreshMode GetDefaultRefreshMode() const = 0;

    //! Sets the default update chunk size of the data source
    //! This value is the maximum number of records that should be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records should be sent in one single message.
    //! DS_DEFAULT_CHUNK_SIZE tells the data source to use its original default chunk size.
    virtual void SetDefaultUpdateChunkSize(int iUpdateChunkSize) = 0;
    //! Returns the default update chunk size of the data source
    //! This value is the maximum number of records that will be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records will be sent in one single message.
    virtual int GetDefaultUpdateChunkSize() const = 0;


    //! Asks the source to pass content updates lists through the event system that
    //! contain all of the content it can access.
    //! \param usScanID The scan ID of this refresh cycle, assigned by the data source manager.
    //! \param mode Specifies if metadata should be retrieved in one pass, two passes, or not at all.
    //! \param iUpdateChunkSize Specifies how many media records to send back at a time.
    //! If iUpdateChunkSize is zero, then all records are sent back at once.
    virtual ERESULT ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize) = 0;

    //! Retrieves metadata for each media content record in the passed-in list.
    virtual void GetContentMetadata(content_record_update_t* pContentUpdate) = 0;

    //! Asks the source to open this URL for reading.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IInputStream for this file type.
    virtual IInputStream* OpenInputStream(const char* szURL) = 0;

    //! Asks the source to open this URL for writing.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IOutputStream for this file type.
    virtual IOutputStream* OpenOutputStream(const char* szURL) = 0;

    //! Asks the source the length of the media serial number, if available.
    //! Returns 0 if no serial number is available, or the length in bytes.
    virtual int GetSerialNumberLength() const = 0;

    //! Get the serial number from the media and copy it into the buffer.
    //! Returns the number of bytes copied, or -1 if an error was occurred.
    virtual int GetSerialNumber( char* pBuffer, int iBufferLen ) const = 0;

    //! Determine whether or not the datasource supports prebuffering for the specified
    //!  track
    virtual bool QueryCanPrebuffer( const IContentRecord* pRecord ) const = 0;
    
protected:

    friend class CDataSourceManager;

    //! Called by the data source manager to assign an ID to this data source.
    virtual void SetInstanceID(int iDataSourceID) = 0;
};

#endif	// DATASOURCE_H_
