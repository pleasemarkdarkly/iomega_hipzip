//
// CDDataSourceImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CDDATASOURCEIMP_H_
#define CDDATASOURCEIMP_H_

#include <datasource/cddatasource/CDDataSource.h>
#include <io/storage/blk_dev.h>
#include <util/timer/Timer.h>

class CCDDataSourceImp : public IDataSource
{
public:

    CCDDataSourceImp(CCDDataSource* pParent, cyg_io_handle_t hCD,
        const char* szDeviceName, const char* szMountDirectory,
        bool bStartMediaStatusTimer);
    ~CCDDataSourceImp();

    //! Called by the data source manager to assign an ID to this data source.
    void SetInstanceID(int iDataSourceID);

    int GetClassID() const
        { return CD_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const
        { return m_iDataSourceID; }

    //! Queries the media status to see if a disc has been removed or inserted.
    Cyg_ErrNo GetMediaStatus(bool bSendEvent);

    //! Pause/resume the media status test thread.
    void SuspendMediaStatusTimer();
    void ResumeMediaStatusTimer();

    //! Returns a pointer to the table of contents.
    const cdda_toc_t* GetTOC() const
        { return &m_toc; }

    //! Returns number of audio sessions on the cd
    int GetAudioSessionCount() const
        { return m_iAudioSessionCount; }

    //! Returns number of data sessions on the cd
    int GetDataSessionCount() const
        { return m_iDataSessionCount;  }
    
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

    //! Sets the default maximum directory count of the data source
    //! This value is the maximum number of directories that should be scanned in total during a content update.
    //! 0 indicates that there is no maximum
    void SetDefaultMaximumDirectoryCount(unsigned int uiMaxDirectoryCount)
        { m_uiMaxDirectoryCount = uiMaxDirectoryCount; }
    //! Returns the default maximum directory count of the data source
    //! This value is the maximum number of directories that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    unsigned int GetDefaultMaximumDirectoryCount() const
        { return m_uiMaxDirectoryCount; }

    //! Copies the string the data source uses to prefix its URLs into the given string provided.
    bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;
    //! Given a URL from this data source, this function creates a URL prefix for the local path.
    bool GetLocalURLPrefix(const char* szURL, char* szLocalURLPrefix, int iMaxLength) const
        { return false; }

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
    IOutputStream* OpenOutputStream(const char* szURL)
        { return 0; }   // Not supported

    //! Asks the source the length of the media serial number, if available.
    //! Returns 0 if no serial number is available, or the length in bytes.
    int GetSerialNumberLength() const;

    //! Get the serial number from the media and copy it into the buffer.
    //! Returns the number of bytes copied, or -1 if an error was occurred.
    int GetSerialNumber( char* pBuffer, int iBufferLen ) const;

    //! Indicates whether or not the datasource supports prebuffering.
    bool QueryCanPrebuffer( const IContentRecord* pRecord ) const;

    //! Returns the calculated disc ID for the CD
    unsigned int GetDiscID() const
        {  return m_uiDiskID; }
        
    long Read(long lba, long sectors, void *buffer);

    //! Returns a pointer to the record of the audio track at the specified zero-based index.
    //! If no audio track record is found then 0 is returned.
    IMediaContentRecord* GetAudioTrackRecord(IContentManager* pContentManager, int index);

    //! Returns the zero-based index of the given track if it is a CD audio track
    int GetAudioTrackIndex(const char* szURL) const;

    //! Ejects the CD.
    //! Returns true if the CD was ejected, false otherwise.
    bool OpenTray();

    //! Closes the CD tray.
    //! Returns true if the CD tray was closed, false otherwise.
    bool CloseTray();

    //! Toggles the CD tray open/closed.
    void ToggleTray();

    //! Tell this Data Source that the tray has been closed outside of it's control.
    //! Most likely the CD tray was pushed closed by the user.
    void TrayIsClosed()
        { m_bTrayOpen = false; }
    
    //! Returns true if the tray is open, false otherwise.
    bool IsTrayOpen() const
        { return m_bTrayOpen; }

	//! Suspends and spins down drive
	static bool SuspendDrive( const char* szName );

	//! wakes up drive for reads
	static bool WakeupDrive( const char* szName );

    //! Sets the maximum filename length that an ISO file can be when scanning for content.
    //! Files with names longer than this value won't be added to the update list.
    //! 0 indicates that no maximum should be set.
    //! The default value is 0.
    void SetMaxFilenameLength(int iFileLen)
        { m_iMaxFilenameLength = iFileLen; }
    
    //! Returns the maximum filename length that an ISO file can be when scanning for content.
    //! Files with names longer than this value won't be added to the update list.
    //! 0 indicates that no maximum should be set.
    int GetMaxFilenameLength() const
        { return m_iMaxFilenameLength; }

    
private:

    CCDDataSource* m_pParent;

    int     m_iDataSessionCount;
    int     m_iAudioSessionCount;
    
    int     m_iDataSourceID;
    bool    m_bStartMediaStatusTimer;

    // Disc/drive description
    char*           m_szDeviceName;
    char*           m_szMountDirectory;
    char*           m_szFileSystemPoint;
    char*           m_szFilePrefix;
    cyg_io_handle_t	m_hCD;      //!< Handle to the CD drive.
    cdda_toc_t      m_toc;      //!< Record that stores the CD's table of contents.
    RefreshMode m_eRefreshMode; //!< Default setting to use when refreshing content.
    int     m_iUpdateChunkSize; //!< Default setting to use when refreshing content.
    unsigned int    m_uiMaxTrackCount;  //!< Maximum number of records to read during a content scan.
    unsigned int    m_uiMaxDirectoryCount;  //!< Maximum number of directories to scan during a content scan.
    unsigned int	m_uiDiskID; //!< CDDB disc ID.
    int m_iMaxFilenameLength;   //!< Maximum allowable ISO filename length to use in content scans.

    drive_geometry_t m_dgGeometry;
    
    
    Cyg_ErrNo FetchTOC();
    void ClearTOC();
    int DisableReadCache();
    Cyg_ErrNo DoPacket(unsigned char *command, unsigned char *data, int iDataLength, int dir);

    bool RefreshContentThreadFunc(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize);
    void AddMediaRecordToVectorAndUpdate(media_record_info_t& mediaContent, content_record_update_t** ppContentUpdate, int iUpdateChunkSize);
    void AddPlaylistRecordToVectorAndUpdate(playlist_record_t& playlistRecord, content_record_update_t** ppContentUpdate, int iUpdateChunkSize);

    // Traverses the given directory and all subdirectories, adding files with recognized extensions to the playlist.
    bool PopulateCDFileList(char* szTopDirName, content_record_update_t** ppContentUpdate, RefreshMode mode, int iUpdateChunkSize);

    // Obtains the media geometry and caches it locally
    int GetMediaGeometry();
    
    // Used for checking media status at regular intervals.
    timer_handle_t  m_TimerHandle;
    static void CheckMediaStatusCB(void* arg);
	int				m_iLastUnitStatus;
    bool            m_bNoMedia;
    bool            m_bMediaBad;
    bool            m_bTrayOpen;        //!< True if the CD tray is open, false otherwise.
};

#endif	// CDDATASOURCEIMP_H_
