//
// CDDataSourceImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "CDDataSourceImp.h"

#include <string.h>     /* memset */
#include <stdlib.h>     /* calloc */
#include <stdio.h>      /* sprintf */
#include <dirent.h>

#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>
#include <content/common/ContentManager.h>
#include <content/common/Metadata.h>
#include <core/events/SystemEvents.h>   // event types
#include <core/playmanager/PlayManager.h>
#include <cyg/error/codes.h>
#include <cyg/fileio/fileio.h>
#include <datastream/cddafile/CDDAInputStream.h>
#include <datastream/isofile/IsoFileInputStream.h>
#include <devs/storage/ata/atadrv.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

//#define PROF
#ifdef PROF
#include <cyg/kernel/kapi.h>
#endif

DEBUG_USE_MODULE(CDDATASOURCE);

// If REMOVE_BAD_TRACKS is defined, then tracks that fail to be set during the second pass of
// a two pass update will be removed from the update structure.
// This is optional because doing so will change the track count from the first pass to the second
// pass, which screws up data CD metadata caching.
//#define REMOVE_BAD_TRACKS

// If CALC_FREEDB_ID, then the freedb ID will be calculated when a disc is inserted.
#define CALC_FREEDB_ID

#define MAX_TRACKS 99

typedef struct
{
    unsigned char reserved1;
    unsigned char flags;
    unsigned char track;
    unsigned char reserved2;
    signed char start_MSB;
    unsigned char start_1;
    unsigned char start_2;
    unsigned char start_LSB;
} scsi_toc_t;


#define SHOW_RESULT( _fn, _res ) \
DEBUG(CDDATASOURCE, DBGLEV_ERROR, "<FAIL>: " #_fn "() returned %d %s\n", _res, _res<0?strerror(errno):"");

CCDDataSourceImp::CCDDataSourceImp(CCDDataSource* pParent, cyg_io_handle_t hCD,
        const char* szDeviceName, const char* szMountDirectory,
        bool bStartMediaStatusTimer)
    : IDataSource(CD_DATA_SOURCE_CLASS_ID),
      m_pParent(pParent),
      m_iDataSessionCount(0),m_iAudioSessionCount(0),
      m_iDataSourceID(0),
      m_bStartMediaStatusTimer(bStartMediaStatusTimer),
      m_hCD(hCD),
      m_eRefreshMode(DSR_ONE_PASS_WITH_METADATA),
      m_iUpdateChunkSize(0),
      m_uiMaxTrackCount(0),
	  m_iLastUnitStatus(-ENOMED),
      m_bNoMedia(false),
      m_bMediaBad(false),
      m_bTrayOpen(false)   // Assume the tray starts closed.  May change this later.
{
    GetMediaGeometry();

    m_szDeviceName = (char*)malloc(strlen(szDeviceName) + 1);
    DBASSERT(CDDATASOURCE, m_szDeviceName, "Unable to allocate device name string");
    strcpy(m_szDeviceName, szDeviceName);

    m_szMountDirectory = (char*)malloc(strlen(szMountDirectory) + 1);
    DBASSERT(CDDATASOURCE, m_szMountDirectory, "Unable to allocate mount directory name string");
    strcpy(m_szMountDirectory, szMountDirectory);

    m_szFilePrefix = (char*)malloc(strlen(szDeviceName) + 7 /* "file://" */ + 1);
    DBASSERT(CDDATASOURCE, m_szFilePrefix, "Unable to allocate file prefix string");
    strcpy(m_szFilePrefix, "file://");
    //    strcat(m_szFilePrefix, szDeviceName);
    m_szFileSystemPoint = NULL;

    // Initialize the TOC.
    memset((void*)&m_toc, 0, sizeof(m_toc));
}

CCDDataSourceImp::~CCDDataSourceImp()
{
#ifdef ENABLE_ISOFS
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: <INFO>: Unmounting\n");
    int err = umount(m_szMountDirectory);
    if (err < 0)
        SHOW_RESULT(unmount, err);
    //	mem_stats();
#endif  // ENABLE_ISOFS

    SuspendMediaStatusTimer();
    unregister_timer(m_TimerHandle);
    free(m_szDeviceName);
    free(m_szMountDirectory);
    free(m_szFilePrefix);
    if( m_szFileSystemPoint ) {
        free( m_szFileSystemPoint );
    }
}

//! Called by the data source manager to assign an ID to this data source.
void
CCDDataSourceImp::SetInstanceID(int iDataSourceID)
{
    m_iDataSourceID = iDataSourceID;

    /*
    if( GetMediaStatus( true ) == ENOERR ) {
        for(int i = 0; i < 50; i++)
        {
            switch(FetchTOC())
            {
                case 2: // Unit either has no disc or is still loading one
                case 4: // Unit either has no disc or is still loading one
                    cyg_thread_delay(50);
                    break;
                case 0:
                    i = 999;
                    break;
                default:
                    ClearTOC();
                    return;
            }
        }
    }
    */

    register_timer(CheckMediaStatusCB, (void*)this, TIMER_MILLISECONDS(1000), -1, &m_TimerHandle);
    if (m_bStartMediaStatusTimer)
        resume_timer(m_TimerHandle);
}

void
CCDDataSourceImp::CheckMediaStatusCB(void* arg)
{
    //  DEBUGP(CDDATASOURCE, DBGLEV_INFO, "Checking media status\n");
    ((CCDDataSourceImp*)arg)->GetMediaStatus(true);
}

// Queries the media status to see if a disc has been removed or inserted.
// TODO since this call is made from the timer, review whether 'try_put_event' (nonblocking)
//       should be used in places of 'put_event' (potentially blocking)

// New GetMediaStatus only saves -ENOMED and ENOERR as last events, and only generates
// events when transitioning between the two - all other events are ignored.
// Timeout is still in there - does it do anything?

Cyg_ErrNo
CCDDataSourceImp::GetMediaStatus(bool bSendEvent)
{
    Cyg_ErrNo err = ENOERR;
    cyg_uint32 len = sizeof(cyg_uint8);
	
    err = cyg_io_get_config(m_hCD, IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);

	switch(err)
	{
		case ENOERR:
			
			if(m_iLastUnitStatus == -ENOMED)
			{
		        m_bMediaBad = false;
				umount(m_szMountDirectory);
				DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: &&& Media changed &&&\n");
				GetMediaGeometry();
				if (bSendEvent)
				{
					m_bNoMedia = false;
					// Send a message to the UI.
					put_event(EVENT_MEDIA_INSERTED, (void*)GetInstanceID());
				}
			}

			m_iLastUnitStatus = err;

			break;

		case -ENOMED:
			
			if(m_iLastUnitStatus == ENOERR)
			{
				m_bMediaBad = false;
				umount(m_szMountDirectory);

				if (!m_bNoMedia && bSendEvent)
				{
					DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: &&& No media &&&\n");
					m_bNoMedia = true;  // Only send one media removed message.
					// Send a message to the UI.
					put_event(EVENT_MEDIA_REMOVED, (void*)GetInstanceID());
				}
			}

			m_iLastUnitStatus = err;

			break;

		case -ETIME:
			
			// 5/5/02 dc- assume timeout on media sense is bad media
			if( !m_bMediaBad && bSendEvent) 
			{
				m_bMediaBad = true;
				put_event(EVENT_MEDIA_BAD, (void*)GetInstanceID());
			}

			break;

		default:
			break;
    }

    return err;
}

#define SUSPEND_TIMEOUT_MAX 1000
#define SUSPEND_RESET_THRESHOLD 5
#define SUSPEND_LOOP_DELAY 20
bool
CCDDataSourceImp::SuspendDrive( const char* szName )
{

	int iTestPower = SUSPEND_RESET_THRESHOLD;
	int iTotalWait = 0;
    cyg_io_handle_t hCD;
    cyg_io_lookup( szName, &hCD );
    cyg_uint32 len = sizeof(len);
    //	SuspendMediaStatusTimer();

    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_SLEEP, 0, &len) == ENOERR) 
    {
        DEBUG(CDDATASOURCE, DBGLEV_INFO, "CD suspend\n");
        //        cyg_io_set_config(m_hCD, IO_ATA_SET_CONFIG_BUS_LOCK, 0, &len);

		while(iTestPower > 0 && iTotalWait < SUSPEND_TIMEOUT_MAX)
		{
			if(0xFF != cyg_io_get_config( hCD, IO_ATAPI_GET_POWER_STATUS, NULL , 0 ))
			{				
				iTestPower--;		
			}
			else
			{
				DEBUG(CDDATASOURCE, DBGLEV_ERROR, "CD suspend retry\n");
				iTestPower = SUSPEND_RESET_THRESHOLD;
				cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_SLEEP, 0, &len);					
			}

			cyg_thread_delay(SUSPEND_LOOP_DELAY);
			iTotalWait+=SUSPEND_LOOP_DELAY;
		}


        return true;
    }
    else
    {
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "CD suspend failed\n");
        return false;
    }	

}

bool
CCDDataSourceImp::WakeupDrive( const char* szName )
{
    cyg_uint32 len = sizeof(len);
    cyg_io_handle_t hCD;
    cyg_io_lookup( szName, &hCD );
    
    // cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_RESET, 0, &len);

    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_WAKEUP, 0, &len) == ENOERR) 
    {
        DEBUG(CDDATASOURCE, DBGLEV_INFO, "CD wakeup\n");
        //  		GetInstance()->ResumeMediaStatusTimer();
        return true;
    }
    else
    {
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "CD wakeup failed\n");
        //		GetInstance()->ResumeMediaStatusTimer();
        return false;
    }	


}

//! Pause the media status test thread.
void
CCDDataSourceImp::SuspendMediaStatusTimer()
{
    suspend_timer(m_TimerHandle);
}

//! Resume the media status test thread.
void
CCDDataSourceImp::ResumeMediaStatusTimer()
{
    resume_timer(m_TimerHandle);
}

//! Sets the default refresh mode of the data source.
//! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
void
CCDDataSourceImp::SetDefaultRefreshMode(IDataSource::RefreshMode mode)
{
    if (mode == DSR_DEFAULT)
        m_eRefreshMode = DSR_ONE_PASS_WITH_METADATA;
    else
        m_eRefreshMode = mode;
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CCDDataSourceImp::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    if (strlen(m_szFilePrefix) >= (unsigned)iMaxLength)
        return false;
    else
    {
        strcpy(szRootURLPrefix, m_szFilePrefix);
        return true;
    }
}


//! Returns a pointer to the record of the audio track at the specified zero-based index.
//! If no audio track record is found then 0 is returned.
IMediaContentRecord*
CCDDataSourceImp::GetAudioTrackRecord(IContentManager* pContentManager, int index)
{
    DBASSERT(CDDATASOURCE, pContentManager, "Invalid content manager pointer\n");

    int count = 0;
    int i = 0;
    for (; i < m_toc.entries; i++)
        if (m_toc.entry_list[i].audio_p)
            if (++count > index)
                break;

    char szURL[128];
    sprintf(szURL, "%scdda?%d", m_szFilePrefix, i + 1);
    return pContentManager->GetMediaRecord(szURL);
}

//! Returns the zero-based index of the given track if it is a CD audio track
int
CCDDataSourceImp::GetAudioTrackIndex(const char* szURL) const 
{
    // Verify that the track came from this data source.
    if (strncmp(szURL, m_szFilePrefix, strlen(m_szFilePrefix)))
        return -1;

    // Use the URL to determine if this is a CDDA track.
    if (char* pchCDDAToken = strstr(szURL, "cdda?"))
    {
        int iTableIndex = atoi(pchCDDAToken + 5) - 1;
        if ((iTableIndex >= 0) && (iTableIndex < m_toc.entries) && (m_toc.entry_list[iTableIndex].audio_p))
        {
            // Skip the data tracks when counting the index.
            int index = 0;
            while (--iTableIndex >= 0)
                if (m_toc.entry_list[iTableIndex].audio_p)
                    ++index;
            return index;
        }
    }
    return -1;
}


//! Ejects the CD.
//! Returns true if the CD was ejected, false otherwise.
bool
CCDDataSourceImp::OpenTray()
{
		
    cyg_uint32 len = sizeof(len);
    bool ret;
    SuspendMediaStatusTimer();
    

    //    if( m_bMediaBad ) {
        // 5/5/02 dc- on bad media assume we need to reset the cd drive
    //        cyg_io_set_config(m_hCD, IO_BLK_SET_CONFIG_RESET, 0, &len);
    //    }
    
    // eject CD tray
    if(cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_TRAY_OPEN, 0, &len) == ENOERR)
    {
        DEBUG(CDDATASOURCE, DBGLEV_INFO, "CD tray opened\n");
        m_bTrayOpen = true;
        ret = true;
    }
    else
    {
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "CD tray open failed\n");
        ret = false;
    }
    ResumeMediaStatusTimer();
    return ret;
}

//! Closes the CD tray.
//! Returns true if the CD tray was closed, false otherwise.
bool
CCDDataSourceImp::CloseTray()
{


	


    cyg_uint32 len = sizeof(len);
    bool ret;
    
    SuspendMediaStatusTimer();
    // eject CD tray
    if(cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_TRAY_CLOSE, 0, &len) == ENOERR)
    {
        DEBUG(CDDATASOURCE, DBGLEV_INFO, "CD tray Closed\n");
        m_bTrayOpen = false;
        ret = true;
    }
    else
    {
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "CD tray Close failed\n");
        ret = false;
    }
    ResumeMediaStatusTimer();

    return ret;
}

//! Toggles the CD tray open/closed.
void
CCDDataSourceImp::ToggleTray()
{
    if (m_bTrayOpen)
        CloseTray();
    else
        OpenTray();
}

ERESULT
CCDDataSourceImp::ListAllEntries(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize)
{
    //    ClearTOC();

    put_event(EVENT_CONTENT_UPDATE_BEGIN, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

    RefreshMode eUseMode = mode == DSR_DEFAULT ? m_eRefreshMode : mode;
    if (RefreshContentThreadFunc(usScanID, eUseMode, iUpdateChunkSize == DS_DEFAULT_CHUNK_SIZE ? m_iUpdateChunkSize : iUpdateChunkSize))
    {
        if (eUseMode == DSR_TWO_PASS)
            put_event(EVENT_CONTENT_METADATA_UPDATE_BEGIN, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

        return MAKE_ERESULT(SEVERITY_SUCCESS, 0, 0);
    }
    else
        return MAKE_ERESULT(SEVERITY_FAILED, 0, 0);
}

// Retrieves metadata for each media content record in the passed-in list.
void
CCDDataSourceImp::GetContentMetadata(content_record_update_t* pContentUpdate)
{
#ifdef PROF
    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: GetContentMetadata: start: %d ticks\n", tick);
#endif  // PROF

    SuspendMediaStatusTimer();

    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
    int i = 0;
    while (!m_bNoMedia && (i < pContentUpdate->media.Size()))
    {
        if (char* pchCDDAToken = strstr(pContentUpdate->media[i].szURL, "cdda?"))
        {
            // CDDA file
            int iTrackIndex = atoi(pchCDDAToken + 5) - 1;
            if ((iTrackIndex >= 0) && (iTrackIndex < m_toc.entries) &&
                (pContentUpdate->media[i].pMetadata = pCM->CreateMetadataRecord()))
            {
                pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)(m_toc.entry_list[iTrackIndex].lba_length * 2352));
                if (ICodec* pCodec = pCodecMgr->FindCodec(pContentUpdate->media[i].iCodecID))
                {
                    CCDDAInputStream cddaIS;
                    if (FAILED(cddaIS.Open(m_pParent, m_toc.entry_list[iTrackIndex].lba_startsector, m_toc.entry_list[iTrackIndex].lba_length))
                        || FAILED(pCodec->GetMetadata(this, pContentUpdate->media[i].pMetadata, &cddaIS)))
                    {
#ifdef REMOVE_BAD_TRACKS
                        // This is a bad track, so remove it from the list.
                        delete pContentUpdate->media[i].pMetadata;
                        free(pContentUpdate->media[i].szURL);
                        pContentUpdate->media.Remove(i);
#else   // REMOVE_BAD_TRACKS
                        ++i;
#endif  // REMOVE_BAD_TRACKS
                        GetMediaStatus(true);
                    }
                    else
                        ++i;
                    delete pCodec;
                }
                else
                    ++i;
            }
        }
        else
        {
            // ISO file
            if( (pContentUpdate->media[i].pMetadata = pCM->CreateMetadataRecord()) )
            {
                const char* szFilename = pContentUpdate->media[i].szURL + strlen(m_szFilePrefix) - 1 /* '/' */;
                if (ICodec* pCodec = pCodecMgr->FindCodec(pContentUpdate->media[i].iCodecID))
                {
                    struct stat sbuf;
                    CIsoFileInputStream isoIS;
                    if (FAILED(isoIS.Open(szFilename))
                        || FAILED(pCodec->GetMetadata(this, pContentUpdate->media[i].pMetadata, &isoIS))
                        || (stat(szFilename, &sbuf) < 0))
                    {
#ifdef REMOVE_BAD_TRACKS
                        // This is a bad track, so remove it from the list.
                        delete pContentUpdate->media[i].pMetadata;
                        free(pContentUpdate->media[i].szURL);
                        pContentUpdate->media.Remove(i);
#else   // REMOVE_BAD_TRACKS
                        ++i;
#endif  // REMOVE_BAD_TRACKS
                        GetMediaStatus(true);
                    }
                    else
                    {
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)sbuf.st_size);
                        ++i;
                    }
                    delete pCodec;
                }
                else
                    ++i;
            }
        }
    }
    if (m_bNoMedia)
    {
        // An error occurred, most likely a CD eject, so clear all records.
        for (int i = 0; i < pContentUpdate->media.Size(); ++i)
        {
            free(pContentUpdate->media[i].szURL);
            delete pContentUpdate->media[i].pMetadata;
        }
        for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
            free(pContentUpdate->playlists[i].szURL);
        delete pContentUpdate;
    }
    else
        put_event(EVENT_CONTENT_METADATA_UPDATE, (void*)pContentUpdate);


    ResumeMediaStatusTimer();

#ifdef PROF
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: GetContentMetadata: %d ticks\n", cyg_current_time() - tick);
#endif  // PROF
}


void
CCDDataSourceImp::AddMediaRecordToVectorAndUpdate(media_record_info_t& mediaContent, content_record_update_t** ppContentUpdate, int iUpdateChunkSize)
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
CCDDataSourceImp::AddPlaylistRecordToVectorAndUpdate(playlist_record_t& playlistRecord, content_record_update_t** ppContentUpdate, int iUpdateChunkSize)
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

bool
CCDDataSourceImp::RefreshContentThreadFunc(unsigned short usScanID, RefreshMode mode, int iUpdateChunkSize)
{
    suspend_timer(m_TimerHandle);

    ClearTOC();
    for(int i = 0; i < 50; i++)
    {
        switch(FetchTOC())
        {
            case 2: // Unit either has no disc or is still loading one
            case 4: // Unit either has no disc or is still loading one
                cyg_thread_delay(50);
                break;
            case 0:
                i = 999;
                break;
            case -EIO:  // malformed TOC
            default:
                ClearTOC();
                resume_timer(m_TimerHandle);
                put_event(EVENT_CONTENT_UPDATE_END, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));
                put_event(EVENT_CONTENT_METADATA_UPDATE_END, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));
                DBTR( CDDATASOURCE );
                return false;
        }
    }

    DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "track  length [mm:ss.ff]    begin [mm:ss.ff]        audio copy pre ch\n");
    DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "======================================================================\n");

    m_iAudioSessionCount = 0;
    m_iDataSessionCount  = 0;
    
    int total = 0;
    int iDataTrackIndex = -1;
    bool    bAudioTrackExists = false;
    RegKey pcmCodecID = CCodecManager::GetInstance()->FindCodecID("cda");
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    // The PCM codec can only return stream info metadata, so check before opening a stream.
    bool bGetPCMMetadata = false;
    ICodec* pPCMCodec = 0;
    if (mode == DSR_ONE_PASS_WITH_METADATA)
    {
        IMetadata* pMetadata = pCM->CreateMetadataRecord();
        if (pMetadata &&
                (pMetadata->UsesAttribute(MDA_DURATION) ||
                pMetadata->UsesAttribute(MDA_SAMPLING_FREQUENCY) ||
                pMetadata->UsesAttribute(MDA_CHANNELS) ||
                pMetadata->UsesAttribute(MDA_BITRATE) ||
                pMetadata->UsesAttribute(MDA_FILE_SIZE)))
        {
            bGetPCMMetadata = true;
            pPCMCodec = CCodecManager::GetInstance()->FindCodec(pcmCodecID);
        }
        delete pMetadata;
    }
    content_record_update_t* pContentUpdate = new content_record_update_t;
    pContentUpdate->usScanID = usScanID;
    pContentUpdate->bTwoPass = mode == DSR_TWO_PASS;
    pContentUpdate->iDataSourceID = GetInstanceID();

    for(int i=0;i<m_toc.entries;i++)
    {
        if(m_toc.entry_list[i].audio_p)
        {
            bAudioTrackExists = true;
            m_iAudioSessionCount++;
            
            DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "%3d.  %7ld [%02d:%02d.%02d]  ",
                   i,
                   m_toc.entry_list[i].lba_length,
                   (int)(m_toc.entry_list[i].lba_length/(60*75)),
                   (int)(m_toc.entry_list[i].lba_length/75%60),
                   (int)(m_toc.entry_list[i].lba_length%75));
            DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "%7ld [%02d:%02d.%02d]         %s  %s  %s  %d\n",
                   m_toc.entry_list[i].lba_startsector,
                   (int)(m_toc.entry_list[i].lba_startsector/(60*75)),
                   (int)(m_toc.entry_list[i].lba_startsector/75%60),
                   (int)(m_toc.entry_list[i].lba_startsector%75),
                   "yes"                                  ,
                   (m_toc.entry_list[i].copyok_p?"yes":" no"),
                   (m_toc.entry_list[i].preemp_p?"yes":" no"),
                   m_toc.entry_list[i].channels);

            if (m_toc.entry_list[i].preemp_p)
                DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "*** Pre-emphasis bit found ***\n");

            total+=m_toc.entry_list[i].lba_length;

            if (pcmCodecID)
            {
                media_record_info_t mediaContent;
                mediaContent.bVerified = true;
                mediaContent.iDataSourceID = GetInstanceID();
                mediaContent.iCodecID = pcmCodecID;
                mediaContent.pMetadata = 0;
                mediaContent.szURL = (char*)malloc(strlen(m_szFilePrefix) + 10 /* cdda?xxxx */);
                sprintf(mediaContent.szURL, "%scdda?%d", m_szFilePrefix, i + 1);
                if (bGetPCMMetadata)
                {
                    if (mediaContent.pMetadata || (mediaContent.pMetadata = pCM->CreateMetadataRecord()))
                    {
                        mediaContent.pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)(m_toc.entry_list[i].lba_length * 2352));
                        CCDDAInputStream cddaIS;
                        if (SUCCEEDED(cddaIS.Open(m_pParent, m_toc.entry_list[i].lba_startsector, m_toc.entry_list[i].lba_length)))
                            pPCMCodec->GetMetadata(this, mediaContent.pMetadata, &cddaIS);
                    }
                }
                AddMediaRecordToVectorAndUpdate(mediaContent, &pContentUpdate, iUpdateChunkSize);
            }
        }
        else
        {
            DEBUGP(CDDATASOURCE, DBGLEV_TRACE, "%3d.  %7ld             %7ld                    %s  %s   \n",
                   i,
                   m_toc.entry_list[i].lba_length,
                   m_toc.entry_list[i].lba_startsector,
                   " no",
                   (m_toc.entry_list[i].copyok_p?"yes":" no"));
            iDataTrackIndex = i;
            ++m_iDataSessionCount;
        }
    }
    delete pPCMCodec;

//diag_printf("*** Pause -- Press <Eject> ***\n");
//cyg_thread_delay(1000);
//diag_printf("*** Too late ***\n");

#ifdef ENABLE_ISOFS
    if (iDataTrackIndex >= 0)
    {
        if( m_szFileSystemPoint ) {
            free( m_szFileSystemPoint );
            m_szFileSystemPoint = NULL;
        }
        if( m_toc.entry_list[iDataTrackIndex].lba_startsector > 0 ) {
            // we have a data track that isn't the lead one, so we need to
            // specify the lba
            m_szFileSystemPoint = (char*)malloc( strlen("cd9660") + 7 ); // max

            // TODO: figure out why i need 11400 here. it makes it work though
//            sprintf( m_szFileSystemPoint, "cd9660:%d", (int)(m_toc.entry_list[iDataTrackIndex].lba_startsector + 11400) );
            sprintf( m_szFileSystemPoint, "cd9660:%d", (int)(m_toc.entry_list[iDataTrackIndex].lba_startsector) );
        }
        else {
            m_szFileSystemPoint = (char*) malloc( strlen("cd9660") + 1 );
            strcpy( m_szFileSystemPoint, "cd9660");
        }
        
        // Mount the drive.
        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Mounting\n");
        int n = 0;
        int iErr;
        do
        {
            iErr = mount( m_szDeviceName, m_szMountDirectory, m_szFileSystemPoint );
            if (iErr < 0)
            {
                SHOW_RESULT(mount, iErr);
                cyg_thread_delay(10);
            }
        } while (iErr && (n++ < 50));

        if (!iErr)
        {
            DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Mounted\n");
            if (!PopulateCDFileList(m_szMountDirectory, &pContentUpdate, mode, iUpdateChunkSize))
            {
                // An error occurred, most likely a CD eject, so clear all records.
                for (int i = 0; i < pContentUpdate->media.Size(); ++i)
                {
                    free(pContentUpdate->media[i].szURL);
                    delete pContentUpdate->media[i].pMetadata;
                }
                pContentUpdate->media.Clear();
                for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
                    free(pContentUpdate->playlists[i].szURL);
                pContentUpdate->playlists.Clear();
                delete pContentUpdate;

                resume_timer(m_TimerHandle);

                put_event(EVENT_CONTENT_UPDATE_ERROR, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

                return false;
            }
        }
    }

#endif  // ENABLE_ISOFS

    if (pContentUpdate->media.Size() || pContentUpdate->playlists.Size())
    {
        put_event(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
    }
    else
        delete pContentUpdate;

    resume_timer(m_TimerHandle);

    put_event(EVENT_CONTENT_UPDATE_END, (void*)MAKE_DATA_SCAN_ID(GetInstanceID(), usScanID));

    return true;
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream*
CCDDataSourceImp::OpenInputStream(const char* szURL)
{
    DBASSERT(CDDATASOURCE, szURL, "Opening null URL");
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: OpenInputStream: %s\n", szURL);

    // Verify that the track came from this data source.
    if (strncmp(szURL, m_szFilePrefix, strlen(m_szFilePrefix)))
        return 0;

    // Use the URL to determine if this is a CDDA track.
    if (char* pchCDDAToken = strstr(szURL, "cdda?"))
    {
        CCDDAInputStream* pStream = 0;
        int iTrackIndex = atoi(pchCDDAToken + 5) - 1;
        if (iTrackIndex >= 0)
        {
            DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: CDDA index: %d\n", iTrackIndex);
            pStream = new CCDDAInputStream;

            // Open stream
            if (FAILED(pStream->Open(m_pParent, m_toc.entry_list[iTrackIndex].lba_startsector, m_toc.entry_list[iTrackIndex].lba_length)))
            {
                delete pStream;

                DEBUG(CDDATASOURCE, DBGLEV_ERROR, "Unable to open CDDA track %d\n", iTrackIndex);
                return 0;
            }
        }
        return pStream;
    }
    else
    {
        CIsoFileInputStream* pStream = new CIsoFileInputStream;
        if (FAILED(pStream->Open(szURL + strlen(m_szFilePrefix) - 1 /* '/' */)))
        {
            delete pStream;

            DEBUG(CDDATASOURCE, DBGLEV_ERROR, "Unable to open ISO file %s\n", szURL);
            return 0;
        }

        return pStream;
    }

    return 0;
}

//! Asks the source the length of the media serial number, if available.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CCDDataSourceImp::GetSerialNumberLength() const
{
    if( m_bNoMedia ) return -1;

    return m_dgGeometry.serial_len;
}


//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CCDDataSourceImp::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    // fail if no media
    if( m_bNoMedia ) return -1;
    
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

int CCDDataSourceImp::GetMediaGeometry() 
{
    cyg_uint32 len = sizeof( m_dgGeometry );
    Cyg_ErrNo err = cyg_io_get_config( m_hCD, IO_BLK_GET_CONFIG_GEOMETRY, (void*) &m_dgGeometry, &len );
    if( err < 0 ) {
        return -1;
    }
    return 0;
}

bool CCDDataSourceImp::QueryCanPrebuffer( const IContentRecord* pRecord ) const
{
    // Only prebuffer audio CDs
    return (bool)(strstr(pRecord->GetURL(), "cdda?") != NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CALC_FREEDB_ID
static int
CDDBSum(int n)
{
    int	ret;

    /* For backward compatibility this algorithm must not change */

    ret = 0;

    while (n > 0) {
	ret = ret + (n % 10);
	n = n / 10;
    }

    return (ret);
}
#endif  // CALC_FREEDB_ID


Cyg_ErrNo
CCDDataSourceImp::FetchTOC()
{
    // dc- this probably shouldn't get here, but if it does, be sure to error our
    if ( m_bNoMedia )
        return -ENOMED;
    
    DBEN(CDDATASOURCE);

    Cyg_ErrNo       ret;
    unsigned char   packet_command[12];
    unsigned char   packet_data[12];
    unsigned char   packet_command_TOC[12] = {0x43,2,0,0,0,0,0,0,12,0,0,0};
//    unsigned char   packet_command_TOC[12] = {0x43,0,0,0,0,0,0,0,12,0,0,0};
    scsi_toc_t      *stoc = (scsi_toc_t *)(packet_data + 4);

    // set up packet command to grab TOC geometry before anything else
    memcpy(packet_command, packet_command_TOC, 12);

    ret = DoPacket(packet_command, packet_data, 12, ATAPI_DATA_IN);
    // this is a hack to swallow the retry after a UNIT ATTENTION that usually follows an ATA reset.
    if (ret != ENOERR)
    {
        ret = DoPacket(packet_command, packet_data, 12, ATAPI_DATA_IN);
        if (ret != ENOERR)
            return ret;
    }

    DBTR(CDDATASOURCE);

    /* how many tracks on this disc? */
    int first = packet_data[2];
    int tracks = packet_data[3] - first + 1;

    if( tracks > MAX_TRACKS || tracks < 1 ) {
        // bad toc
        return -EIO;
    }

    DBTR(CDDATASOURCE);

    m_toc.entries = tracks;
    m_toc.entry_list = (cdda_toc_entry_t *)calloc(tracks, sizeof(cdda_toc_entry_t));

    unsigned int Sum = 0, Total;
    unsigned int FirstM = 0, FirstS = 0;
    for (int i = 0; i < tracks; ++i)
    {
        memcpy(packet_command, packet_command_TOC, 12);
        packet_command[6] = i + first;
		
        ret = DoPacket(packet_command, packet_data, 12, ATAPI_DATA_IN);

        if (ret == ENOERR)
        {
            m_toc.entry_list[i].channels = (stoc->flags & 8 ? 4 : 2);
            m_toc.entry_list[i].audio_p = (stoc->flags & 4 ? 0 : 1);
            m_toc.entry_list[i].copyok_p = (stoc->flags & 2 ? 1 : 0);
            m_toc.entry_list[i].preemp_p = (stoc->flags & 1 ? 1 : 0);
            m_toc.entry_list[i].track = stoc->track;

            long lbaaddr = packet_data[9] * 60 * 75 + packet_data[10] * 75 + packet_data[11] - 150;

            // malformed TOC
            if( lbaaddr < 0 ) {
                DBTR(CDDATASOURCE);
                return -EIO;
            }
            
            m_toc.entry_list[i].lba_startsector = lbaaddr;
            if (i > 0) {
                m_toc.entry_list[i - 1].lba_length = lbaaddr - m_toc.entry_list[i - 1].lba_startsector;

                // malformed TOC
                if( m_toc.entry_list[i-1].lba_length < 0 ) {
                    DBTR(CDDATASOURCE);
                    return -EIO;
                }
            }

#ifdef CALC_FREEDB_ID
            if (i == 0)
            {
                FirstM = packet_data[9];
                FirstS = packet_data[10];
            }
            Sum += CDDBSum(packet_data[9] * 60 + packet_data[10]);
#endif
        } else {
            DBTR(CDDATASOURCE);
            return ret;
        }
    }

#ifdef CALC_FREEDB_ID
    unsigned int LeadM = 0, LeadS = 0;
#endif
    // End with the leadout info.
    memcpy(packet_command, packet_command_TOC, 12);
    packet_command[6] = 0xAA;
    ret = DoPacket(packet_command, packet_data, 12, ATAPI_DATA_IN);
    if (ret == ENOERR)
    {
        long lbaaddr = packet_data[9] * 60 * 75 + packet_data[10] * 75 + packet_data[11] - 150;
        m_toc.entry_list[tracks - 1].lba_length = lbaaddr - m_toc.entry_list[tracks - 1].lba_startsector;

        LeadM = packet_data[9];
        LeadS = packet_data[10];

        // malformed TOC
        if( m_toc.entry_list[tracks-1].lba_length < 0 ) {
            DBTR(CDDATASOURCE);
            return -EIO;
        }
    } else {
        DBTR(CDDATASOURCE);        
        return ret;
    }
    
#ifdef CALC_FREEDB_ID
    Total = ((LeadM * 60) + LeadS) - ((FirstM * 60) + FirstS);
    m_uiDiskID = ((Sum % 0xff) << 24 | Total << 8 | tracks);
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: My discID: %08x\n", m_uiDiskID);
#endif

#ifdef ENABLE_PARANOIA
    DisableReadCache();
#endif  // ENABLE_PARANOIA

    DBEX(CDDATASOURCE);

    return(ENOERR);
}

void
CCDDataSourceImp::ClearTOC()
{
    DBEN(CDDATASOURCE);
    if (m_toc.entry_list)
        free((void*)m_toc.entry_list);
    memset((void*)&m_toc, 0, sizeof(m_toc));
    m_uiDiskID = 0;
    DBEX(CDDATASOURCE);
}

Cyg_ErrNo
CCDDataSourceImp::DoPacket(unsigned char *command, unsigned char *data, int iDataLength,int dir)
{
    ATAPICommand_T Cmd;
    cyg_uint32 Length;
    int Status;

    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags = dir; //(dir | ATAPI_AUTO_SENSE);
    Cmd.Data = (char *)data;
    Cmd.DataLength = iDataLength;
    Cmd.Timeout = 100 * 30; /* 30s */
	
    memcpy(&Cmd._SCSICmd, command, 12);
    Cmd.SCSICmd = &Cmd._SCSICmd;
    Cmd.SCSICmdLength = 12;
	
    Length = sizeof(Cmd);
    Status = cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &Cmd, &Length);
#if 0   /* Comment this out for now.  The Status variable was being lost, so the caller couldn't tell the command failed. */
    if (Status) {
        static const unsigned char RequestSenseData[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        static const unsigned char packet_command_RequestSense[12]={0x03,0,0,0,14,0,0,0,0,0,0,0};	
		

        memset(&Cmd, 0, sizeof(Cmd));
        Cmd.Flags = ATAPI_DATA_IN;
        Cmd.Data = (char *)RequestSenseData;
        Cmd.DataLength = sizeof(RequestSenseData);
        Cmd.Timeout = 100 * 30; /* 30s */

        memcpy(&Cmd._SCSICmd, packet_command_RequestSense, sizeof(packet_command_RequestSense));
        Cmd.SCSICmd = &Cmd._SCSICmd;
        Cmd.SCSICmdLength = 12;

        Length = sizeof(Cmd);
        int RSStatus = cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &Cmd, &Length);
        if (RSStatus) {
            DEBUG(CDDATASOURCE, DBGLEV_WARNING, "Request Sense failed %x\n", RSStatus);
        }
        DEBUGP(CDDATASOURCE, DBGLEV_WARNING, "%s line %d ASC: %x ASCQ: %x\n", __FUNCTION__, __LINE__, RequestSenseData[12],
               RequestSenseData[13]);
        Status = RequestSenseData[12];
    }
#endif
    return Status;
}

int
CCDDataSourceImp::DisableReadCache ()
{
    Cyg_ErrNo ret=0;
    unsigned char packet_command[12]={0x55,0x10,0,0,0,0,0,0,20,0, 0,0};
    unsigned char mode[20]={0,0x0a,0,0,0,0,0,0,
                            0x08,0x0a,0x01,0x00,
                            0x00,0x00,0x00,0x00,
                            0x00,0x00,0x00,0x00};
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Disabling ATA look-ahead\n");
    cyg_uint32 len = 1;
    ret=cyg_io_set_config(m_hCD,IO_BLK_SET_CONFIG_LOOKAHEAD_DISABLE,0,&len);
    if(ret)
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "Look-ahead disable FAILED: %d.\n",ret);
        
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Disabling read cache (ATAPI MODE SENSE(10), page 0x8)\n");
    ret=DoPacket(packet_command,mode,20,ATAPI_DATA_OUT);
    if(ret)
        DEBUG(CDDATASOURCE, DBGLEV_ERROR, "Read cache diable FAILED: %d.\n",ret);
    return(ret);
}

long
CCDDataSourceImp::Read (long lba, long sectors, void *buffer)
{
    Cyg_ErrNo ret=0;
    unsigned char packet_command[12];
    static const unsigned char packet_command_READMMC[12]={0xbe,0,0,0,0,0,0,0,0,0xf8,0,0};
    int iErrCount = 0;

    // assumption: lba and sectors are 24bit values (probably safe)
    
    //    while(sectors)
    {
        memcpy(packet_command,packet_command_READMMC,12);
        packet_command[3]= (lba >>16) & 0xff;
        packet_command[4]= (lba >>8) & 0xff;
        packet_command[5]= lba & 0xff;
        packet_command[6]= (sectors>>16) & 0xff;
        packet_command[7]= (sectors>> 8) & 0xff;
        packet_command[8]= sectors & 0xff;
        
        ret=DoPacket(packet_command,(unsigned char *)buffer,sectors*2352,ATAPI_DATA_IN);
        if(ret==ENOERR){
            return(sectors);
        }
                
        if (++iErrCount > 5)
        {
            sectors>>=1;
            iErrCount = 0;
        }
    }

    // If we get here, we had an error. Assume the cd has fingerprints/scratch/whatever that will prevent
    //  us from reading it, and let the UI know
    m_bMediaBad = true;
    put_event(EVENT_MEDIA_BAD, (void*)GetInstanceID());
    
    if(ret<0){
        return(ret);
    }
        
    return(-EIO);
}

//==========================================================================

#define make_string(var,len) ( var = (char*) malloc(sizeof(char)* (len)) )
#define free_string(adr) free( adr )

bool
CCDDataSourceImp::PopulateCDFileList(char* szTopDirName,
    content_record_update_t** ppContentUpdate, RefreshMode mode, int iUpdateChunkSize)
{
#ifdef ENABLE_ISOFS

#ifdef PROF
    cyg_tick_count_t tick = cyg_current_time();
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: PopulateCDFileList: start: %d ticks\n", tick);
#endif

    SimpleList<char*> dqueue;
    char *mask = 0, *full_filename = 0;
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();

    int prefix_len = strlen(m_szFilePrefix);
    unsigned int uiTrackCount = 0;
    unsigned int uiDirectoryCount = 0;
    unsigned int uiDirectoriesScannedCount = 0;

    make_string(mask, strlen(szTopDirName) + 1);
    strcpy(mask, szTopDirName);
  
    dqueue.PushBack(mask);

    Cyg_ErrNo err = ENOERR;
    while ((err == ENOERR) && !dqueue.IsEmpty() && (!m_uiMaxTrackCount || (uiTrackCount < m_uiMaxTrackCount)))
    {
        mask = dqueue.PopFront();
        DIR * dirp = opendir(mask);
        if (dirp)
        {
            /* Check each entry in this directory */
            struct dirent * dentry;
            while(!m_bNoMedia && (dentry = readdir(dirp)) && (!m_uiMaxTrackCount || (uiTrackCount < m_uiMaxTrackCount)))
            {
                if (dentry->d_name[0] != '.')
                {
                    /* Build full filename */
                    int len = strlen(mask) + 1 /* '/' */ + strlen(dentry->d_name) + 1 /* '\0' */;
                    make_string(full_filename, len);
                    if (mask[0])
                    {
                        strcpy(full_filename, mask);
                        if (!(mask[0] == '/' && mask[1] == 0))
                            strcat(full_filename, "/");
                    }
                    strcat(full_filename, dentry->d_name);
		        
                    /* Find out what type of file this is */
                    struct stat sbuf;
                    if (stat(full_filename, &sbuf) >= 0)
                    {
                        if (S_ISDIR(sbuf.st_mode))
                        {
                            /* This is a directory */

                            /* Obey our directory limit */
                            if (!m_uiMaxDirectoryCount || (uiDirectoryCount < m_uiMaxDirectoryCount))
                            {
                                dqueue.PushBack(full_filename);
                                uiDirectoryCount++;
                            }
                            continue; /* Don't free full_filename just yet since it's a directory */
                        }
                        else if (S_ISREG(sbuf.st_mode))
                        {
                            /* This is a regular file */
			                
                            /* If it matches an extension */
                            len = strlen(full_filename);
                            if (m_iMaxFilenameLength && (len > m_iMaxFilenameLength))
                            {
                                DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Ignoring file of len %d: %s\n", len, full_filename);
                            }
                            else if (RegKey codecID = pCodecMgr->FindCodecID(&full_filename[len - 3]))
                            {
                                media_record_info_t mediaContent;
                                mediaContent.bVerified = true;
                                mediaContent.iDataSourceID = GetInstanceID();
                                mediaContent.iCodecID = codecID;

                                mediaContent.szURL = (char*)malloc(prefix_len + len);
                                strcpy(mediaContent.szURL, m_szFilePrefix);
                                strcat(mediaContent.szURL, full_filename + 1);

                                bool bAddTrack = false;

                                if ((mode == DSR_ONE_PASS_WITH_METADATA) && (mediaContent.pMetadata = pCM->CreateMetadataRecord()))
                                {
                                    if (ICodec* pCodec = pCodecMgr->FindCodec(mediaContent.iCodecID))
                                    {
                                        CIsoFileInputStream IFIS;
                                        if (SUCCEEDED(IFIS.Open(full_filename)) &&
                                            SUCCEEDED(pCodec->GetMetadata(this, mediaContent.pMetadata, &IFIS)))
                                        {
                                            bAddTrack = true;
                                        }
                                        else
                                        {
                                            // The codec couldn't get metadata, which most likely means that this is a bad track.
                                            // Save ourselves some hassle and don't add it to the content manager.
                                            DEBUG(CDDATASOURCE, DBGLEV_WARNING, "Failed to get metadata on file %s\n", mediaContent.szURL);
                                        }
                                        delete pCodec;
                                    }
                                    TCHAR tszScratch[256];
                                    mediaContent.pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, dentry->d_name, 256));
                                    mediaContent.pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)sbuf.st_size);
                                }
                                else
                                {
                                    mediaContent.pMetadata = 0;
                                    bAddTrack = true;
                                }

                                if (bAddTrack)
                                {
                                    AddMediaRecordToVectorAndUpdate(mediaContent, ppContentUpdate, iUpdateChunkSize);
                                    if (++uiTrackCount % 100 == 0)
                                        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Tracks Found %d\n", uiTrackCount);
                                }
                                else
                                {
                                    free(mediaContent.szURL);
                                    delete mediaContent.pMetadata;
                                }
                            }
                            else if (unsigned int iPlaylistFormat = pPFM->FindPlaylistFormat(&full_filename[len - 3]))
                            {
                                playlist_record_t playlistRecord;
                                playlistRecord.bVerified = true;
                                playlistRecord.iDataSourceID = GetInstanceID();
                                playlistRecord.iPlaylistFormatID = iPlaylistFormat;
                                playlistRecord.szURL = (char*)malloc(strlen(m_szFilePrefix) + len);
                                strcpy(playlistRecord.szURL, m_szFilePrefix);
                                strcat(playlistRecord.szURL, full_filename + 1);
                                AddPlaylistRecordToVectorAndUpdate(playlistRecord, ppContentUpdate, iUpdateChunkSize);
                            }
                        }
                        else
                        {
                            /* Not a directory or a file, ignore */
                        }
                    }
                    else
                    {
                        /* stat() failed, ignore this entry */

                        if (!m_bNoMedia)
                        {
                            /* GetMediaStatus, but close the dirp if the media has been removed or changed. */
                            cyg_uint32 len = sizeof(cyg_uint8);
	                        
                            err = cyg_io_get_config(m_hCD, IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);
                        }
                        else
                        {
                            err = -ENOMED;
                        }
                    }
                    free_string(full_filename);
                }
                else
                {
                    /* Skip "." and ".." directories */
                }
            }
	    	    
            /* No more entries in this directory */
            closedir(dirp);

            /* Post an event stating how many directories we've scanned (in chunks of 20) */
            if (++uiDirectoriesScannedCount % 20 == 0)
            {
                DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: Directories Scanned %d\n", uiDirectoriesScannedCount);
            }
        }
        else
        {
            if (!m_bNoMedia)
                err = GetMediaStatus(true);
            else
            {
                err = -ENOMED;
            }
        }

        /* Could not open this directory */
        free_string(mask);
    }

    // did we reached our track limit?
    if (m_uiMaxTrackCount && (uiTrackCount >= m_uiMaxTrackCount))
    {
        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: CD Track Limit of %d was reached\n", m_uiMaxTrackCount);
    }

    // did we reached our directory limit?
    if (m_uiMaxDirectoryCount && (uiDirectoryCount >= m_uiMaxDirectoryCount))
    {
        DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: CD Track Limit of %d was reached\n", m_uiMaxDirectoryCount);
    }

    if (err != ENOERR)
    {
        // Cleanup
        while (!dqueue.IsEmpty())
            free_string(dqueue.PopFront());

        if (!m_bNoMedia)
        {
            if (err == -ENOMED)
            {
                // Calling umount crashes the system at this point.  Odd, that.
//                umount(m_szMountDirectory);
                DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: &&& No media &&&\n");
                m_bNoMedia = true;  // Only send one media removed message.
                // Send a message to the UI.
                put_event(EVENT_MEDIA_REMOVED, (void*)GetInstanceID());
            }
            else if (err == -EMEDCHG)
            {
//                umount(m_szMountDirectory);
                DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: &&& Media changed &&&\n");
                GetMediaGeometry();
                m_bNoMedia = false;
                // Send a message to the UI.
                put_event(EVENT_MEDIA_INSERTED, (void*)GetInstanceID());
            }
        }

        return false;
    }

#ifdef PROF
    DEBUGP(CDDATASOURCE, DBGLEV_INFO, "cdds: PopulateCDFileList: %d ticks\n", cyg_current_time() - tick);
#endif  // PROF

    return true;

#endif  // ENABLE_ISOFS
}

//==========================================================================
