//
// FatDataSourceImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "FatDataSourceImp.h"

#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */
#include <ctype.h>      /* isspace */
#include <errno.h>

#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>
#include <content/common/ContentManager.h>
#include <content/common/Metadata.h>
#include <core/events/SystemEvents.h>  // event types
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

#include <core/playmanager/PlayManager.h>

// For media change
#include <cyg/io/io.h>
#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

DEBUG_USE_MODULE(FATDATASOURCE);


CFatDataSourceImp::CFatDataSourceImp(int iDriveNumber, cyg_io_handle_t hDrive, bool bMediaInitialized, bool bStartMediaStatusTimer)
    : IDataSource(FAT_DATA_SOURCE_CLASS_ID),
      m_iDataSourceID(0),
      m_iDriveNumber(iDriveNumber),
      m_hDrive(hDrive),
      m_bMediaInitialized(bMediaInitialized),
      m_eRefreshMode(DSR_ONE_PASS_WITH_METADATA),
      m_iUpdateChunkSize(0),
      m_uiMaxTrackCount(0),
      m_bStartMediaStatusTimer(bStartMediaStatusTimer)
{
    sprintf(m_szContentRootDir, "%c:", 'a' + iDriveNumber);
    GetMediaGeometry();

    if( bStartMediaStatusTimer ) {
        register_timer(CheckMediaStatusCB, (void*)this, TIMER_MILLISECONDS(1000), -1, &m_TimerHandle);
        resume_timer(m_TimerHandle);
    }
}

CFatDataSourceImp::~CFatDataSourceImp()
{
    pc_system_close(m_iDriveNumber);
}

//! Sets the default refresh mode of the data source.
//! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
void
CFatDataSourceImp::SetDefaultRefreshMode(IDataSource::RefreshMode mode)
{
    if (mode == DSR_DEFAULT)
        m_eRefreshMode = DSR_ONE_PASS_WITH_METADATA;
    else
        m_eRefreshMode = mode;
}

#define tolower(ch)    ( ((ch) >= 'A') && ((ch) <= 'Z') ? (ch) - 'A' + 'a' : ch )

//! Sets the root directory for content scans.
bool
CFatDataSourceImp::SetContentRootDirectory(const char* szRootDir)
{
    DBASSERT(FATDATASOURCE, szRootDir, "Null root directory specified\n");

    // Make sure the root dir name isn't too long.
    int len = strlen(szRootDir);

    const char *pch;
    // Check for "a:/blah" format.
    if ((len > 1) && (szRootDir[1] == ':'))
    {
        // Make sure this is the right drive.
        if (tolower(szRootDir[0]) - 'a' != m_iDriveNumber)
        {
            DEBUG(FATDATASOURCE, DBGLEV_WARNING, "Wrong drive letter: %s\n", szRootDir);
            return false;
        }
        pch = szRootDir + 2;
        len -= 2;
    }
    else
        pch = szRootDir;

    // Check for "/blah" format.
    if (*pch == '/')
    {
        ++pch;
        --len;
    }

    if (len + 3 /* a:/ */ >= EMAXPATH) 
    {
        DEBUG(FATDATASOURCE, DBGLEV_WARNING, "Directory name too long: %s\n", szRootDir);
        return false;
    }

    // Construct "a:/blah" format.
    m_szContentRootDir[2] = '/';
    m_szContentRootDir[3] = '\0';
    strcat(m_szContentRootDir, pch);

    // If there's a trailing slash then remove it.
    if (m_szContentRootDir[len + 3 /* a:/ */ - 1] == '/')
        m_szContentRootDir[len + 3 /* a:/ */ - 1] = '\0';

    return true;
}

#undef tolower

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CFatDataSourceImp::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    if (iMaxLength <= 10)
        return false;

    sprintf(szRootURLPrefix, "file://%c:/", 'A' + m_iDriveNumber);
    return true;
}

//! Copies the string the data source uses to prefix its URLs for content scans
//! into the given string provided.
bool
CFatDataSourceImp::GetContentRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    if ((unsigned)iMaxLength <= 7 + strlen(m_szContentRootDir))
        return false;

    sprintf(szRootURLPrefix, "file://%s", m_szContentRootDir);
    return true;
}

ERESULT
CFatDataSourceImp::ListAllEntries(unsigned short usScanID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    if( !m_bMediaInitialized ) {
        if( !pc_system_init( m_iDriveNumber ) ) {
            put_event(EVENT_CONTENT_UPDATE_ERROR, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));
            return MAKE_ERESULT(SEVERITY_FAILED, 0, 0);
        }
        m_bMediaInitialized = true;
    }

    put_event(EVENT_CONTENT_UPDATE_BEGIN, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

    RefreshMode eUseMode = mode == DSR_DEFAULT ? m_eRefreshMode : mode;
    RefreshContentThreadFunc(usScanID, eUseMode, iUpdateChunkSize == DS_DEFAULT_CHUNK_SIZE ? m_iUpdateChunkSize : iUpdateChunkSize);

    put_event(EVENT_CONTENT_UPDATE_END, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

    if (eUseMode == DSR_TWO_PASS)
        put_event(EVENT_CONTENT_METADATA_UPDATE_BEGIN, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

    return MAKE_ERESULT(SEVERITY_SUCCESS, 0, 0);
}

void
CFatDataSourceImp::AddMediaRecordToVectorAndUpdate(media_record_info_t& mediaContent, content_record_update_t** ppContentUpdate, int iUpdateChunkSize)
{
    (*ppContentUpdate)->media.PushBack(mediaContent);
    if (iUpdateChunkSize &&
        (((*ppContentUpdate)->media.Size() + (*ppContentUpdate)->playlists.Size()) >= iUpdateChunkSize))
    {
        content_record_update_t* pContentUpdate = new content_record_update_t;
        pContentUpdate->iDataSourceID = GetInstanceID();
        pContentUpdate->usScanID = (*ppContentUpdate)->usScanID;
        pContentUpdate->bTwoPass = (*ppContentUpdate)->bTwoPass;
        put_event(EVENT_CONTENT_UPDATE, (void*)*ppContentUpdate);
        *ppContentUpdate = pContentUpdate;
    }
}

void
CFatDataSourceImp::AddPlaylistRecordToVectorAndUpdate(playlist_record_t& playlistRecord, content_record_update_t** ppContentUpdate, int iUpdateChunkSize)
{
    (*ppContentUpdate)->playlists.PushBack(playlistRecord);
    if (iUpdateChunkSize &&
        (((*ppContentUpdate)->media.Size() + (*ppContentUpdate)->playlists.Size()) >= iUpdateChunkSize))
    {
        content_record_update_t* pContentUpdate = new content_record_update_t;
        pContentUpdate->iDataSourceID = GetInstanceID();
        pContentUpdate->bTwoPass = (*ppContentUpdate)->bTwoPass;
        put_event(EVENT_CONTENT_UPDATE, (void*)*ppContentUpdate);
        *ppContentUpdate = pContentUpdate;
    }
}

void
CFatDataSourceImp::CheckMediaStatusCB(void* arg)
{
    //  DEBUGP(FATDATASOURCE, DBGLEV_INFO, "Checking media status\n");
    ((CFatDataSourceImp*)arg)->GetMediaStatus(true);
}

// Queries the media status to see if a disc has been removed or inserted.
Cyg_ErrNo
CFatDataSourceImp::GetMediaStatus(bool bSendEvent)
{
    Cyg_ErrNo err = ENOERR;
    cyg_uint32 len = sizeof(cyg_uint8);
	
    err = cyg_io_get_config(m_hDrive, IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);
    if (err == -ENOMED)
    {
        if (m_bMediaInitialized && bSendEvent)
        {
            DEBUGP(FATDATASOURCE, DBGLEV_INFO, "&&& No media &&&\n");
            m_bMediaInitialized = false;  // Only send one media removed message.
            // Send a message to the UI.
            put_event(EVENT_MEDIA_REMOVED, (void*)GetInstanceID());
        }
    }
    else if (err == -EMEDCHG)
    {
        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "&&& Media changed &&&\n");
        GetMediaGeometry();
        if (bSendEvent)
        {
            m_bMediaInitialized = true;
            // Send a message to the UI.
            put_event(EVENT_MEDIA_INSERTED, (void*)GetInstanceID());
        }
    }
    else if (err == -ESLEEP)
    {
        /* TODO Drive sleep not yet supported on Dar */
    }
    return err;
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream*
CFatDataSourceImp::OpenInputStream(const char* szURL)
{
    DBASSERT(FATDATASOURCE, szURL, "Opening null URL");
    DEBUGP(FATDATASOURCE, DBGLEV_INFO, "OpenInputStream: %s\n", szURL);

    CFatFileInputStream* pFFIS = new CFatFileInputStream();
    if (FAILED(pFFIS->Open(szURL + 7)))
    {
        delete pFFIS;
        return 0;
    }
    else
        return pFFIS;
}

//! Asks the source to open this URL for writing.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CFatDataSourceImp::OpenOutputStream(const char* szURL)
{
    CFatFileOutputStream* pFFOS = new CFatFileOutputStream();
    if (FAILED(pFFOS->Open(szURL + 7)))
    {
        delete pFFOS;
        return 0;
    }
    else
        return pFFOS;
}

//! Asks the source the length of the media serial number, if available.
//! This function actually isn't const since we cache the serial number.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CFatDataSourceImp::GetSerialNumberLength() const
{
    // fail if no media
    if( !m_bMediaInitialized ) return -1;

    return m_dgGeometry.serial_len;
}

//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CFatDataSourceImp::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    // fail if no media
    if( !m_bMediaInitialized ) return -1;
    
    // this implicitly forces a geometry refresh
    if( iBufferLen < GetSerialNumberLength() ) {
        return -1;
    }

    // copy the serial number if available
    if( m_dgGeometry.serial_len ) {
        memcpy( (void*) pBuffer, m_dgGeometry.serial_num, m_dgGeometry.serial_len );
    }
    
    return m_dgGeometry.serial_len;
}

//! Cache the media geometry from the block device
int
CFatDataSourceImp::GetMediaGeometry() 
{
    cyg_uint32 len = sizeof( m_dgGeometry );
    Cyg_ErrNo err = cyg_io_get_config( m_hDrive, IO_BLK_GET_CONFIG_GEOMETRY, (void*) &m_dgGeometry, &len );
    if( err < 0 ) {
        return -1;
    }
    return 0;
}



bool
CFatDataSourceImp::SuspendDrive( const char* szName )
{
    cyg_uint32 len = sizeof(len);
    cyg_io_handle_t hDrive;
    cyg_io_lookup( szName, &hDrive );

    // sync the fat layer with the drive - it should auto _init(0) itself
    // assumption - 0 = first drive...oopsies
    pc_system_close(0);

    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(hDrive, IO_BLK_SET_CONFIG_SLEEP, 0, &len) == ENOERR) 
    {
        DEBUG(FATDATASOURCE, DBGLEV_INFO, "HD suspend\n");        
        return true;
    }
    else
    {
        DEBUG(FATDATASOURCE, DBGLEV_ERROR, "HD suspend failed\n");
        return false;
    }	

}

bool
CFatDataSourceImp::WakeupDrive( const char* szName )
{
    cyg_uint32 len = sizeof(len);
    cyg_io_handle_t hDrive;
    cyg_io_lookup( szName, &hDrive );
    
    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(hDrive, IO_BLK_SET_CONFIG_WAKEUP, 0, &len) == ENOERR) 
    {
        DEBUG(FATDATASOURCE, DBGLEV_INFO, "HD wakeup\n");
        cyg_io_set_config(hDrive, IO_BLK_SET_CONFIG_RESET, 0, &len);
        return true;
    }
    else
    {
        DEBUG(FATDATASOURCE, DBGLEV_ERROR, "CD wakeup failed\n");
        return false;
    }	

}

void
CFatDataSourceImp::Sync()
{
    // this probably junks open file handles. too bad we can't use pc_dskflush (not implemented)
    pc_dskclose(0);
    pc_dskopen(0);
}

//
// Static members
//

#define make_string(var,len) ( var = (char*) malloc(sizeof(char)* (len)) )
#define make_wide_string(var,len) ( var = (unsigned short*) malloc(sizeof(unsigned short)* (len)) )
#define free_string(adr) free( adr )
#define shift_upper(a) (a >= 'a' && a <= 'z' ? a - ('a' - 'A') : a )

static void
AddStrippedString(char* szBase, const char* szAddition, int iAdditionLength)
{
    const char* p = szAddition + iAdditionLength - 1;
    while ((p >= szAddition) && isspace(*p))
        --p;
    if (p >= szAddition)
        strncat(szBase, szAddition, p - szAddition + 1);
}

static char*
MakeLongFilename(const char* szRoot, DSTAT& statobj)
{
    char* filename = 0;
    if(statobj.longFileName[0] != '\0')
    {
        char* p = statobj.longFileName;
        make_string( filename, strlen(szRoot) + strlen( p ) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
		sprintf( filename, "%s%s%s", szRoot, p, statobj.fattribute & ADIRENT ? "/" : "");
    }
	else
    {
        if (!isspace(statobj.fext[0]) && !isspace(statobj.fext[1]) && !isspace(statobj.fext[2]))
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + 1 /* . */ + strlen(statobj.fext) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            strcat(filename, ".");
            AddStrippedString(filename, statobj.fext, 3);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "/");
        }
        else
        {
            make_string( filename, strlen(szRoot) + strlen(statobj.fname) + (statobj.fattribute & ADIRENT ? 1 /* / */ : 0) + 1);
            strcpy(filename, szRoot);
            // Remove trailing spaces.
            AddStrippedString(filename, statobj.fname, 8);
            if (statobj.fattribute & ADIRENT)
                strcat(filename, "/");
        }
    }
    return filename;
}


void
CFatDataSourceImp::RefreshContentThreadFunc(unsigned short usScanID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    if (m_bStartMediaStatusTimer)
        suspend_timer(m_TimerHandle);

    SimpleList<char*> dqueue;
    SimpleList<char*> dqueuepath;
    DSTAT statobj;
    char *mask = 0;

    unsigned int uiTrackCount = 0;

    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
    content_record_update_t* pContentUpdate = new content_record_update_t;
    pContentUpdate->iDataSourceID = GetInstanceID();
    pContentUpdate->usScanID = usScanID;
    pContentUpdate->bTwoPass = mode == DSR_TWO_PASS;

    make_string( mask, 5 + strlen(m_szContentRootDir) );
    sprintf( mask, "%s/*.*", m_szContentRootDir );
    dqueue.PushBack( mask );

    char *fullpath = 0;
    make_string( fullpath, 9 + strlen(m_szContentRootDir) );
    sprintf( fullpath, "file://%s/", m_szContentRootDir );
    dqueuepath.PushBack( fullpath );
    fullpath = 0;

    while (!dqueue.IsEmpty() && (!m_uiMaxTrackCount || (uiTrackCount < m_uiMaxTrackCount)))
    {
        mask = dqueue.PopFront();
        free_string(fullpath);
        fullpath = dqueuepath.PopFront();
        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "%s: mask: %s\n", __FUNCTION__, mask);
        if( pc_gfirst( &statobj, mask ) )
        {
            free_string( mask );
            do
            {
                if( statobj.fname[0] == '.' )
                {
		            continue;
		        }
                else if( statobj.fattribute & ADIRENT )
                {
                    int len = strlen( statobj.path ) + 1 /* \ */ + strlen( statobj.fname )
                        + 1 /* . */ + strlen( statobj.fext ) + 5 /* \*.*\0 */;
                    // dc - make sure we dont push a dir that we can't possible reference files from
                    if( (len + 12) < EMAXPATH )
                    {
                        make_string( mask, len );
                        if (strlen(statobj.fext))
                            sprintf( mask, "%s/%s.%s/*.*", statobj.path, statobj.fname, statobj.fext );
                        else
                            sprintf( mask, "%s/%s/*.*", statobj.path, statobj.fname );

                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "Directory Entry Information***\n");
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.path [%s]\n", statobj.path);
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.longFileName [%s]\n", statobj.longFileName);
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.fname  [%s]\n", statobj.fname);
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.fext  [%s]\n", statobj.fext);
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.pname  [%s]\n", statobj.pname);
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "statobj.pext  [%s]\n", statobj.pext);

                        dqueue.PushBack(mask);
                        char* temppath = MakeLongFilename(fullpath, statobj);
                        dqueuepath.PushBack(temppath);
                    }
                }
                else
                {
                    DEBUGP(FATDATASOURCE, DBGLEV_INFO, "%s: File %s.%s\n", __FUNCTION__, statobj.fname, statobj.fext);
                    if (RegKey codecID = pCodecMgr->FindCodecID(statobj.fext))
                    {
                        media_record_info_t mediaContent;
                        mediaContent.bVerified = true;
                        mediaContent.iDataSourceID = GetInstanceID();
                        mediaContent.iCodecID = codecID;
                        mediaContent.szURL = MakeLongFilename(fullpath, statobj);

            			// get the file time and date
/*
			            aryFileInfo[count].usTime = statobj.fcrttime;
			            aryFileInfo[count].usDate = statobj.fcrtdate;
			            aryFileInfo[count].ubTimeTenths = statobj.ftime_tenths;
*/

                        if (mode == DSR_ONE_PASS_WITH_METADATA && (mediaContent.pMetadata = pCM->CreateMetadataRecord()))
                        {
                            if (mediaContent.pMetadata->UsesAttribute(MDA_FILE_NAME))
                            {
                                TCHAR tszScratch[256];
                                if (statobj.longFileName[0] != '\0')
                                    mediaContent.pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, statobj.longFileName, 256));
                                else
                                {
                                    char szFilename[14];
                                    szFilename[0] = '\0';
                                    AddStrippedString(szFilename, statobj.fname, 8);
                                    strcat(szFilename, ".");
                                    AddStrippedString(szFilename, statobj.fext, 3);
                                    mediaContent.pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, szFilename, 14));
                                }
                            }

                            mediaContent.pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)statobj.fsize);
			
                            if (ICodec* pCodec = pCodecMgr->FindCodec(mediaContent.iCodecID,(void*)m_pCodecWorkspace, CODEC_WORKSPACE_SIZE))
                            {
                                CFatFileInputStream FFIS;
                                if (SUCCEEDED(FFIS.Open(mediaContent.szURL + 7)))
                                {
                                    pCodec->GetMetadata(this, mediaContent.pMetadata, &FFIS);
                                }
                                pCodec->~ICodec();
                                pCodec = NULL;
                            }
                        }
                        else
                            mediaContent.pMetadata = 0;

                        AddMediaRecordToVectorAndUpdate(mediaContent, &pContentUpdate, iUpdateChunkSize);
                        ++uiTrackCount;
            		}
                    else if (unsigned int iPlaylistFormat = pPFM->FindPlaylistFormat(statobj.fext))
                    {
                        playlist_record_t playlistRecord;
                        playlistRecord.bVerified = true;
                        playlistRecord.iDataSourceID = GetInstanceID();
                        playlistRecord.iPlaylistFormatID = iPlaylistFormat;
                        playlistRecord.szURL = MakeLongFilename(fullpath, statobj);
                        AddPlaylistRecordToVectorAndUpdate(playlistRecord, &pContentUpdate, iUpdateChunkSize);
                    }
		        }
            } while (pc_gnext(&statobj) && (!m_uiMaxTrackCount || (uiTrackCount < m_uiMaxTrackCount)));

    	    pc_gdone( &statobj );
	    }
        else
        {
    	    free_string( mask );
    	}
    }
    while (!dqueue.IsEmpty())
        free_string( dqueue.PopFront() );
    free_string(fullpath);

    if (pContentUpdate->media.Size() || pContentUpdate->playlists.Size())
        put_event(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
    else
        delete pContentUpdate;

    if (m_bStartMediaStatusTimer)
        resume_timer(m_TimerHandle);
}

// Retrieves metadata for each media content record in the passed-in list.
void
CFatDataSourceImp::GetContentMetadata(content_record_update_t* pContentUpdate)
{
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
    for (int i = 0; i < pContentUpdate->media.Size(); ++i)
    {
        DSTAT statobj;
        if ( pc_gfirst( &statobj, pContentUpdate->media[i].szURL + 7))
        {
            if ( (pContentUpdate->media[i].pMetadata = pCM->CreateMetadataRecord()) )
            {
                if (ICodec* pCodec = pCodecMgr->FindCodec(pContentUpdate->media[i].iCodecID,(void*)m_pCodecWorkspace, CODEC_WORKSPACE_SIZE))
                {
                    pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)statobj.fsize);
                    CFatFileInputStream FFIS;
                    if (SUCCEEDED(FFIS.Open(pContentUpdate->media[i].szURL + 7)))
                    {
                        pCodec->GetMetadata(this, pContentUpdate->media[i].pMetadata, &FFIS);
                    }

                    if (pContentUpdate->media[i].pMetadata->UsesAttribute(MDA_FILE_NAME))
                    {
                        TCHAR tszScratch[256];
                        if (statobj.longFileName[0] != '\0')
                            pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, statobj.longFileName, 256));
                        else
                        {
                            char szFilename[14];
                            szFilename[0] = '\0';
                            AddStrippedString(szFilename, statobj.fname, 8);
                            strcat(szFilename, ".");
                            AddStrippedString(szFilename, statobj.fext, 3);
                            pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, szFilename, 14));
                        }
                    }
                    pCodec->~ICodec();
                    pCodec = NULL;
                }
            }
    	    pc_gdone( &statobj );
        }
    }
    put_event(EVENT_CONTENT_METADATA_UPDATE, (void*)pContentUpdate);
}

