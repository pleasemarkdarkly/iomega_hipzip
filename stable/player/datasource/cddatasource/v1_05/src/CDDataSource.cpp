//
// CDDataSource.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <datasource/cddatasource/CDDataSource.h>
#include "CDDataSourceImp.h"
#include <devs/storage/ata/atadrv.h>

#include <util/debug/debug.h>

DEBUG_MODULE_S(CDDATASOURCE, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(CDDATASOURCE);

CCDDataSource*
CCDDataSource::Open(const char* szDeviceName, const char* szMountDirectory, bool bStartMediaStatusTimer)
{
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "%s: Opening device %s\n", __FUNCTION__, szDeviceName);

    // Mount the drive.
    cyg_io_handle_t	hCD;
    Cyg_ErrNo ret = cyg_io_lookup(szDeviceName, &hCD);

    if (ret == -ENOENT)
    {
        DEBUGP(CDDATASOURCE, DBGLEV_ERROR, "Could not open %s; No such device\n", szDeviceName);
        return 0;
    }
    else
    {
        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "Opened device %s: %d\n", szDeviceName, ret);
    }

    cyg_uint32 len = 1;
    // this gets unset if the drive is put to sleep
    ret = cyg_io_set_config(hCD, IO_ATAPI_SET_CONFIG_FEATURES, 0, &len);
    // this happens every time
    ret = cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_POWER_UP, 0, &len);
    if (ret != ENOERR)
    {
        DEBUGP(CDDATASOURCE, DBGLEV_ERROR, "Could not turn on device %s\n", szDeviceName);
        return 0;
    }
    else
    {
        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "Powered device %s: %d\n", szDeviceName, ret);
    }

    return new CCDDataSource(hCD, szDeviceName, szMountDirectory, bStartMediaStatusTimer);
}

CCDDataSource::CCDDataSource(cyg_io_handle_t hCD, const char* szDeviceName, const char* szMountDirectory,
    bool bStartMediaStatusTimer)
    : IDataSource(CD_DATA_SOURCE_CLASS_ID)
{
    m_pImp = new CCDDataSourceImp(this, hCD, szDeviceName, szMountDirectory, bStartMediaStatusTimer);
}

CCDDataSource::~CCDDataSource()
{
    delete m_pImp;
}

//! Called by the data source manager to assign an ID to this data source.
void
CCDDataSource::SetInstanceID(int iDataSourceID)
{
    m_pImp->SetInstanceID(iDataSourceID);
}

// Queries the media status to see if a disc has been removed or inserted.
Cyg_ErrNo
CCDDataSource::GetMediaStatus(bool bSendEvent)
{
    return m_pImp->GetMediaStatus(bSendEvent);
}

//! Pause the media status test thread.
void
CCDDataSource::SuspendMediaStatusTimer()
{
    m_pImp->SuspendMediaStatusTimer();
}

//! Resume the media status test thread.
void
CCDDataSource::ResumeMediaStatusTimer()
{
    m_pImp->ResumeMediaStatusTimer();
}

//! Returns a pointer to the table of contents.
const cdda_toc_t*
CCDDataSource::GetTOC() const
{
    return m_pImp->GetTOC();
}

int
CCDDataSource::GetAudioSessionCount() const 
{
    return m_pImp->GetAudioSessionCount();
}

int
CCDDataSource::GetDataSessionCount() const
{
    return m_pImp->GetDataSessionCount();
}

int
CCDDataSource::GetInstanceID() const
{
    return m_pImp->GetInstanceID();
}

//! Sets the default refresh mode of the data source.
//! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
void
CCDDataSource::SetDefaultRefreshMode(IDataSource::RefreshMode mode)
{
    m_pImp->SetDefaultRefreshMode(mode);
}

//! Returns the current default refresh mode of the data source.
IDataSource::RefreshMode
CCDDataSource::GetDefaultRefreshMode() const
{
    return m_pImp->GetDefaultRefreshMode();
}

//! Sets the default update chunk size of the data source
//! This value is the maximum number of records that should be sent for each EVENT_CONTENT_UPDATE
//! message.
//! 0 indicates that all records should be sent in one single message.
//! If DS_DEFAULT_CHUNK_SIZE is passed in, then 0 is used.
void
CCDDataSource::SetDefaultUpdateChunkSize(int iUpdateChunkSize)
{
    m_pImp->SetDefaultUpdateChunkSize(iUpdateChunkSize);
}

//! Returns the default update chunk size of the data source
//! This value is the maximum number of records that will be sent for each EVENT_CONTENT_UPDATE
//! message.
//! 0 indicates that all records will be sent in one single message.
int
CCDDataSource::GetDefaultUpdateChunkSize() const
{
    return m_pImp->GetDefaultUpdateChunkSize();
}

//! Sets the default maximum record count of the data source
//! This value is the maximum number of records that should be sent in total during a content update.
//! 0 indicates that there is no maximum
void
CCDDataSource::SetDefaultMaximumTrackCount(unsigned int uiMaxTrackCount)
{
    m_pImp->SetDefaultMaximumTrackCount(uiMaxTrackCount);
}

//! Returns the default maximum record count of the data source
//! This value is the maximum number of records that should be sent in total during a content update.
//! 0 indicates that there is no maximum
unsigned int
CCDDataSource::GetDefaultMaximumTrackCount() const
{
    return m_pImp->GetDefaultMaximumTrackCount();
}

//! Sets the default maximum directory count of the data source
//! This value is the maximum number of directories that should be scanned in total during a content update.
//! 0 indicates that there is no maximum
void
CCDDataSource::SetDefaultMaximumDirectoryCount(unsigned int uiMaxDirectoryCount)
{
    m_pImp->SetDefaultMaximumDirectoryCount(uiMaxDirectoryCount);
}

//! Returns the default maximum directory count of the data source
//! This value is the maximum number of directories that should be sent in total during a content update.
//! 0 indicates that there is no maximum
unsigned int
CCDDataSource::GetDefaultMaximumDirectoryCount() const
{
    return m_pImp->GetDefaultMaximumDirectoryCount();
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CCDDataSource::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    return m_pImp->GetRootURLPrefix(szRootURLPrefix, iMaxLength);
}


ERESULT
CCDDataSource::ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize)
{
    return m_pImp->ListAllEntries(usScanID, mode, iUpdateChunkSize);
}

// Retrieves metadata for each media content record in the passed-in list.
void
CCDDataSource::GetContentMetadata(content_record_update_t* pContentUpdate)
{
    return m_pImp->GetContentMetadata(pContentUpdate);
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream*
CCDDataSource::OpenInputStream(const char* szURL)
{
    return m_pImp->OpenInputStream(szURL);
}

//! Asks the source the length of the media serial number, if available.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CCDDataSource::GetSerialNumberLength() const
{
    return m_pImp->GetSerialNumberLength();
}


//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CCDDataSource::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    return m_pImp->GetSerialNumber( pBuffer, iBufferLen );
}

bool
CCDDataSource::QueryCanPrebuffer( const IContentRecord* pRecord ) const
{
    return m_pImp->QueryCanPrebuffer( pRecord );
}

unsigned int
CCDDataSource::GetDiscID() const 
{
    return m_pImp->GetDiscID();
}

long
CCDDataSource::Read(long lba, long sectors, void *buffer)
{
    return m_pImp->Read(lba, sectors, buffer);
}

//! Returns a pointer to the record of the audio track at the specified zero-based index.
//! If no audio track record is found then 0 is returned.
IMediaContentRecord*
CCDDataSource::GetAudioTrackRecord(IContentManager* pContentManager, int index)
{
    return m_pImp->GetAudioTrackRecord(pContentManager, index);
}

//! Returns the index of the given track if it is a cd audio track
int
CCDDataSource::GetAudioTrackIndex(const char* szURL) const
{
    return m_pImp->GetAudioTrackIndex(szURL);
}

//! Ejects the CD.
//! Returns true if the CD was ejected, false otherwise.
bool
CCDDataSource::OpenTray()
{
    return m_pImp->OpenTray( );
}

//! Closes the CD tray.
//! Returns true if the CD tray was closed, false otherwise.
bool
CCDDataSource::CloseTray()
{
    return m_pImp->CloseTray( );
}

//! suspends cd hardware
bool
CCDDataSource::SuspendDrive( const char* szName )
{
    return CCDDataSourceImp::SuspendDrive( szName );
}

//! resumes cd hardware
bool
CCDDataSource::WakeupDrive( const char* szName )
{
    return CCDDataSourceImp::WakeupDrive( szName );
}


//! Toggles the CD tray open/closed.
void
CCDDataSource::ToggleTray()
{
    m_pImp->ToggleTray( );
}

//! Tell this Data Source that the tray has been closed outside of it's control.
//! Most likely the CD tray was pushed closed by the user.
void
CCDDataSource::TrayIsClosed()
{
    m_pImp->TrayIsClosed();
}
    
//! Returns true if the tray is open, false otherwise.
bool
CCDDataSource::IsTrayOpen() const
{
    return m_pImp->IsTrayOpen();
}

//! Sets the maximum filename length that an ISO file can be when scanning for content.
//! Files with names longer than this value won't be added to the update list.
//! 0 indicates that no maximum should be set.
//! The default value is 0.
void
CCDDataSource::SetMaxFilenameLength(int iFileLen)
{
    m_pImp->SetMaxFilenameLength(iFileLen);
}

//! Returns the maximum filename length that an ISO file can be when scanning for content.
//! Files with names longer than this value won't be added to the update list.
//! 0 indicates that no maximum should be set.
int
CCDDataSource::GetMaxFilenameLength() const
{
    return m_pImp->GetMaxFilenameLength();
}
