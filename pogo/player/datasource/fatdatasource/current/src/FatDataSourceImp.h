//
// FatDataSourceImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef FATDATASOURCEIMP_H_
#define FATDATASOURCEIMP_H_

#include <datasource/fatdatasource/FatDataSource.h>
#include <util/timer/Timer.h>
#include <io/storage/blk_dev.h>
#include <fs/fat/sdapi.h>

// TODO make sure this is appropriate for the codecs we are using. some codecs may need more
// space
#define CODEC_WORKSPACE_SIZE (32*1024)

//! Implements the IDataSource class for storage devices that use the FAT file system.
class CFatDataSourceImp : public IDataSource
{
public:

    CFatDataSourceImp(int iDriveNumber, cyg_io_handle_t hDrive, bool bMediaInitialized = true, bool bStartMediaStatusTimer=true);
    ~CFatDataSourceImp();

    int GetClassID() const
        { return FAT_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const
        { return m_iDataSourceID; }

    //! Queries the media status to see if a disc has been removed or inserted.
    Cyg_ErrNo GetMediaStatus(bool bSendEvent);

    //! Sets the default refresh mode of the data source.
    //! If DSR_DEFAULT is passed in, then DSR_ONE_PASS_WITH_METADATA is used.
    void SetDefaultRefreshMode(RefreshMode mode);
    //! Returns the current default refresh mode of the data source.
    RefreshMode GetDefaultRefreshMode() const
        { return m_eRefreshMode; }

    //! Sets the default update chunk size of the data source
    //! This value is the maximum number of records that should be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records should be sent in one single message.
    //! If DS_DEFAULT_CHUNK_SIZE is passed in, then 0 is used.
    void SetDefaultUpdateChunkSize(int iUpdateChunkSize)
        { m_iUpdateChunkSize = iUpdateChunkSize == DS_DEFAULT_CHUNK_SIZE ? 0 : iUpdateChunkSize; }
    //! Returns the default update chunk size of the data source
    //! This value is the maximum number of records that will be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records will be sent in one single message.
    int GetDefaultUpdateChunkSize() const
        { return m_iUpdateChunkSize; }

    //! Sets the default maximum record count of the data source
    //! This value is the maximum number of records that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    void SetDefaultMaximumTrackCount(unsigned int uiMaxTrackCount)
        { m_uiMaxTrackCount = uiMaxTrackCount; }
    //! Returns the default maximum record count of the data source
    //! This value is the maximum number of records that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    unsigned int GetDefaultMaximumTrackCount() const
        { return m_uiMaxTrackCount; }

    //! Sets the root directory for content scans.
    bool SetContentRootDirectory(const char* szRootDir);
    //! Gets the root directory for content scans.
    const char* GetContentRootDirectory() const
        { return m_szContentRootDir; }

    //! Copies the string the data source uses to prefix its URLs into the given string provided.
    bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;
    //! Copies the string the data source uses to prefix its URLs for content scans
    //! into the given string provided.
    bool GetContentRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;

    //! Asks the source to pass content updates lists through the event system that
    //! contain all of the content it can access.
    //! \param usScanID The scan ID of this refresh cycle, assigned by the data source manager.
    //! \param mode Specifies if metadata should be retrieved in one pass, two passes, or not at all.
    //! \param iUpdateChunkSize Specifies how many media records to send back at a time.
    //! If iUpdateChunkSize is zero, then all records are sent back at once.
    ERESULT ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize);

    //! Retrieves metadata for each media content record in the passed-in list.
    void GetContentMetadata(content_record_update_t* pContentUpdate);

    //! Asks the source to open this URL for reading.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(const char* szURL);

    //! Asks the source to open this URL for writing.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(const char* szURL);

    //! Asks the source the length of the media serial number, if available.
    //! Returns 0 if no serial number is available, or the length in bytes.
    int GetSerialNumberLength() const;

    //! Get the serial number from the media and copy it into the buffer.
    //! Returns the number of bytes copied, or -1 if an error was occurred.
    int GetSerialNumber( char* pBuffer, int iBufferLen ) const;
    
    //! Indicates whether or not the datasource supports prebuffering.
    bool QueryCanPrebuffer( const IContentRecord* pRecord ) const
        { return false; }

    //! Called by the data source manager to assign an ID to this data source.
    void SetInstanceID(int iDataSourceID)
        { m_iDataSourceID = iDataSourceID; }

	//! Suspends and spins down drive
	static bool SuspendDrive( const char* szName );

	//! wakes up drive for reads
	static bool WakeupDrive( const char* szName );

    //! Flushes fat to disk
    static void Sync();

private:

    int m_iDataSourceID;    //!< ID assigned by the data source manager for this data source.
    int m_iDriveNumber;     //!< Drive description.
    cyg_io_handle_t m_hDrive;
    bool m_bMediaInitialized;   //!< True if the drive has been mounted.  False if there's no media inserted
                                //!< or if there's media present that wasn't able to be mounted.
    RefreshMode m_eRefreshMode; //!< Default setting to use when refreshing content.
    int     m_iUpdateChunkSize; //!< Default setting to use when refreshing content.
    unsigned int    m_uiMaxTrackCount;  //!< Maximum number of records to read during a content scan.
    char    m_szContentRootDir[EMAXPATH];   //!< Root directory for content scans.

    //! Updates the cached drive_geometry_t structure
    int GetMediaGeometry();
    drive_geometry_t m_dgGeometry;        //!< Information about the physical device geometry, including serial number
    
    //! Traverses the given directory and all subdirectories, adding files with recognized extensions to the playlist.
    void RefreshContentThreadFunc(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize);
    void AddMediaRecordToVectorAndUpdate(media_record_info_t& mediaContent, content_record_update_t** ppContentUpdate, int iUpdateChunkSize);
    void AddPlaylistRecordToVectorAndUpdate(playlist_record_t& playlistRecord, content_record_update_t** ppContentUpdate, int iUpdateChunkSize);

    // Used for checking media status at regular intervals.
    timer_handle_t  m_TimerHandle;
    static void CheckMediaStatusCB(void* arg);
    bool            m_bNoMedia;
    bool            m_bStartMediaStatusTimer;

    char m_pCodecWorkspace[CODEC_WORKSPACE_SIZE]  __attribute__((aligned(4)));
};

#endif	// FATDATASOURCEIMP_H_
