//
// FatDataSource.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/datasource/fatdatasource/FatDataSource.h>
#include "FatDataSourceImp.h"

#include <util/debug/debug.h>

// For media change
#include <errno.h>
#include <fs/fat/sdapi.h>
#include <cyg/io/io.h>
#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

//DEBUG_MODULE_S(FATDATASOURCE, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE);
DEBUG_MODULE_S(FATDATASOURCE, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(FATDATASOURCE);


CFatDataSource*
CFatDataSource::Open(int iDriveNumber)
{
    DEBUGP(FATDATASOURCE, DBGLEV_INFO, "%s: Opening drive %d\n", __FUNCTION__, iDriveNumber);

    bool bMediaPresent = true;
    if ( !pc_system_init(iDriveNumber) && ( errno != ENOMED ) )
    {
        DEBUG(FATDATASOURCE, DBGLEV_ERROR, "pc_system_init failed, errno = %d\n", errno);
        return 0;
    }
    else
    {
        if( errno == ENOMED ) {
            bMediaPresent = false;
        }
        
        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "Opened drive %d (media %s)\n",
               iDriveNumber, (bMediaPresent ? "present" : "absent") );

        // Register the callback
        cyg_io_handle_t hDrive;
        Cyg_ErrNo ret = cyg_io_lookup( block_drive_names[ iDriveNumber ], &hDrive );
        if( ret != -ENOENT ) {
            return new CFatDataSource( iDriveNumber, hDrive, bMediaPresent );
        }
    }
    return 0;
}

CFatDataSource::CFatDataSource(int iDriveNumber, cyg_io_handle_t hDrive, bool bMediaInitialized)
    : IDataSource(FAT_DATA_SOURCE_CLASS_ID)
{
    m_pImp = new CFatDataSourceImp(iDriveNumber, hDrive, bMediaInitialized);
}

CFatDataSource::~CFatDataSource()
{
    delete m_pImp;
}

int
CFatDataSource::GetClassID() const
{
    return FAT_DATA_SOURCE_CLASS_ID;
}

int
CFatDataSource::GetInstanceID() const
{
    return m_pImp->GetInstanceID();
}

//! Sets the default refresh mode of the data source.
//! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
void
CFatDataSource::SetDefaultRefreshMode(IDataSource::RefreshMode mode)
{
    m_pImp->SetDefaultRefreshMode(mode);
}

//! Returns the current default refresh mode of the data source.
IDataSource::RefreshMode
CFatDataSource::GetDefaultRefreshMode() const
{
    return m_pImp->GetDefaultRefreshMode();
}

//! Sets the default update chunk size of the data source
//! This value is the maximum number of records that should be sent for each EVENT_CONTENT_UPDATE
//! message.
//! 0 indicates that all records should be sent in one single message.
//! If DS_DEFAULT_CHUNK_SIZE is passed in, then 0 is used.
void
CFatDataSource::SetDefaultUpdateChunkSize(int iUpdateChunkSize)
{
    m_pImp->SetDefaultUpdateChunkSize(iUpdateChunkSize);
}

//! Returns the default update chunk size of the data source
//! This value is the maximum number of records that will be sent for each EVENT_CONTENT_UPDATE
//! message.
//! 0 indicates that all records will be sent in one single message.
int
CFatDataSource::GetDefaultUpdateChunkSize() const
{
    return m_pImp->GetDefaultUpdateChunkSize();
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CFatDataSource::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    return m_pImp->GetRootURLPrefix(szRootURLPrefix, iMaxLength);
}

ERESULT
CFatDataSource::ListAllEntries(IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    return m_pImp->ListAllEntries(mode, iUpdateChunkSize);
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream*
CFatDataSource::OpenInputStream(const char* szURL)
{
    return m_pImp->OpenInputStream(szURL);
}

//! Asks the source to open this URL for writing.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CFatDataSource::OpenOutputStream(const char* szURL)
{
    return m_pImp->OpenOutputStream(szURL);
}

//! Asks the source the length of the media serial number, if available.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CFatDataSource::GetSerialNumberLength() const
{
    return m_pImp->GetSerialNumberLength();
}

//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CFatDataSource::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    return m_pImp->GetSerialNumber( pBuffer, iBufferLen );
}

// Retrieves metadata for each media content record in the passed-in list.
void
CFatDataSource::GetContentMetadata(content_record_update_t* pContentUpdate)
{
    return m_pImp->GetContentMetadata(pContentUpdate);
}

void 
CFatDataSource::CreateMediaRecordsForNewFiles(FileRefList* lstNewFiles)
{
    m_pImp->CreateMediaRecordsForNewFiles(lstNewFiles);
}

//! Called by the data source manager to assign an ID to this data source.
void
CFatDataSource::SetInstanceID(int iDataSourceID)
{
    m_pImp->SetInstanceID(iDataSourceID);
}
