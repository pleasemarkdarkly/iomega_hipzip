//
// CDDataSource.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CDDATASOURCE_H_
#define CDDATASOURCE_H_

#include <datasource/common/DataSource.h>
#include <io/storage/blk_dev.h>

class IContentManager;
class IInputStream;
class IMediaContentRecord;
class IOutputStream;
typedef struct media_record_info_s media_record_info_t;
typedef struct playlist_record_s playlist_record_t;

typedef struct cdda_toc_entry_s cdda_toc_entry_t;
typedef struct cdda_toc_s
{
    int entries;
    cdda_toc_entry_t    *entry_list;
} cdda_toc_t;

typedef struct cdda_toc_entry_s
{
    int audio_p;
    int channels;
    int copyok_p;
    int preemp_p;
    int track;
    long lba_startsector;
    long lba_length;
} cdda_toc_entry_t;

#define CD_DATA_SOURCE_CLASS_ID  1

//! Implements the IDataSource class for CD drives.
class CCDDataSource : public IDataSource
{
public:

    //! Attempts to open the drive with the given device name.
    //! Returns a pointer to a new CCDataSource object if successful, 0 otherwise.
    //! If bStartMediaStatusTimer is true, then the media status test thread will be
    //! started on creation.
    //! If false, then ResumeMediaStatusTimer must be called to start the check.
    static CCDDataSource* Open(const char* szDeviceName, const char* szMountDirectory,
        bool bStartMediaStatusTimer = true);

    ~CCDDataSource();

    int GetClassID() const
        { return CD_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const;

    //! Queries the media status to see if a disc has been removed or inserted.
    Cyg_ErrNo GetMediaStatus(bool bSendEvent);

    //! Pause/resume the media status test thread.
    void SuspendMediaStatusTimer();
    void ResumeMediaStatusTimer();

    //! Returns a pointer to the table of contents.
    const cdda_toc_t* GetTOC() const;

    //! Returns number of audio sessions on the cd
    int GetAudioSessionCount() const;

    //! Returns number of data sessions on the cd
    int GetDataSessionCount() const;

    //! Sets the default refresh mode of the data source.
    //! If DSR_DEFAULT is passed in, then DSR_ONE_PASS_WITH_METADATA is used.
    void SetDefaultRefreshMode(RefreshMode mode);
    //! Returns the current default refresh mode of the data source.
    RefreshMode GetDefaultRefreshMode() const;

    //! Sets the default update chunk size of the data source
    //! This value is the maximum number of records that should be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records should be sent in one single message.
    //! If DS_DEFAULT_CHUNK_SIZE is passed in, then 0 is used.
    void SetDefaultUpdateChunkSize(int iUpdateChunkSize);
    //! Returns the default update chunk size of the data source
    //! This value is the maximum number of records that will be sent for each EVENT_CONTENT_UPDATE
    //! message.
    //! 0 indicates that all records will be sent in one single message.
    int GetDefaultUpdateChunkSize() const;

    //! Sets the default maximum record count of the data source
    //! This value is the maximum number of records that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    void SetDefaultMaximumTrackCount(unsigned int uiMaxTrackCount);
    //! Returns the default maximum record count of the data source
    //! This value is the maximum number of records that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    unsigned int GetDefaultMaximumTrackCount() const;

    //! Sets the default maximum directory count of the data source
    //! This value is the maximum number of directories that should be scanned in total during a content update.
    //! 0 indicates that there is no maximum
    void SetDefaultMaximumDirectoryCount(unsigned int uiMaxDirectoryCount);
    //! Returns the default maximum directory count of the data source
    //! This value is the maximum number of directories that should be sent in total during a content update.
    //! 0 indicates that there is no maximum
    unsigned int GetDefaultMaximumDirectoryCount() const;

    //! Copies the string the data source uses to prefix its URLs into the given string provided.
    bool GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const;

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
    unsigned int GetDiscID() const;

    //! Reads data from the CD.
    //! \param lba The logical block address to start reading from.
    //! \param sectors The number of sectors to read.
    //! \param buffer A pointer to a pre-allocated buffer to receive the data.
    //! \retval >=0 The number of sectors read.
    //! \retval <0 An error code.
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
    void TrayIsClosed();
    
    //! Returns true if the tray is open, false otherwise.
    bool IsTrayOpen() const;

	//! Suspends and spins down drive
	static bool SuspendDrive( const char* szName );

	//! wakes up drive for reads
	static bool WakeupDrive( const char* szName );

    //! Sets the maximum file length that an ISO file can be when scanning for content.
    //! Files with names longer than this value won't be added to the update list.
    //! 0 indicates that no maximum should be set.
    //! The default value is 0.
    void SetMaxFilenameLength(int iFileLen);
    
    //! Returns the maximum file length that an ISO file can be when scanning for content.
    //! Files with names longer than this value won't be added to the update list.
    //! 0 indicates that no maximum should be set.
    int GetMaxFilenameLength() const;

private:

    CCDDataSource(cyg_io_handle_t hCD, const char* szDeviceName, const char* szMountDirectory,
        bool bStartMediaStatusTimer);

    //! Called by the data source manager to assign an ID to this data source.
    void SetInstanceID(int iDataSourceID);

friend class CCDDataSourceImp;
    CCDDataSourceImp*   m_pImp;
};

#endif	// CDDATASOURCE_H_
