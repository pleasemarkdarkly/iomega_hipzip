//
// MiniCDMgr.cpp
//
// Manages CD tray state, gets media status, generates media events (if needed)
// because I like CDDataSource, but there are too many problems with it being mixed into
// the media player
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//


#include "MiniCDMgr.h"

#include <string.h>     /* memset */
#include <stdlib.h>     /* calloc */
#include <stdio.h>      /* sprintf */
#include <dirent.h>

#include <core/events/SystemEvents.h>   // event types
#include <cyg/error/codes.h>
#include <cyg/fileio/fileio.h>
#include <devs/storage/ata/atadrv.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

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


DEBUG_MODULE_S( MINICDMGR, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE(MINICDMGR);

static CMiniCDMgr *s_pMiniCDMgr = NULL;

// If REMOVE_BAD_TRACKS is defined, then tracks that fail to be set during the second pass of
// a two pass update will be removed from the update structure.
// This is optional because doing so will change the track count from the first pass to the second
// pass, which screws up data CD metadata caching.
//#define REMOVE_BAD_TRACKS

#define SHOW_RESULT( _fn, _res ) \
DEBUG(MINICDMGR, DBGLEV_ERROR, "<FAIL>: " #_fn "() returned %d %s\n", _res, _res<0?strerror(errno):"");

CMiniCDMgr::CMiniCDMgr( cyg_io_handle_t hCD, const char* szDeviceName, 	const char* szMountDirectory, bool bStartMediaStatusTimer ) :
		m_bStartMediaStatusTimer(bStartMediaStatusTimer),
		m_hCD(hCD),		
		m_bTrayOpen(false),   // Assume the tray starts closed.  May change this later.
		m_bNoMedia(false),
		m_iLastUnitStatus(-ENOMED),
		m_bMediaBad(false)
{

	memset((void*)&m_toc, 0, sizeof(m_toc));

    GetMediaGeometry();
   
	m_szDeviceName = (char*)malloc(strlen(szDeviceName) + 1);
    DBASSERT(MINICDMGR, m_szDeviceName, "Unable to allocate device name string");
    strcpy(m_szDeviceName, szDeviceName);

    m_szMountDirectory = (char*)malloc(strlen(szMountDirectory) + 1);
    DBASSERT(MINICDMGR, m_szMountDirectory, "Unable to allocate mount directory name string");
    strcpy(m_szMountDirectory, szMountDirectory);
    
	register_timer(CheckMediaStatusCB, (void*)this, TIMER_MILLISECONDS(1000), -1, &m_TimerHandle);
    if (m_bStartMediaStatusTimer)
        resume_timer(m_TimerHandle);
}

CMiniCDMgr::~CMiniCDMgr()
{

#ifdef ENABLE_ISOFS
    DEBUGP(MINICDMGR, DBGLEV_INFO, "cdds: <INFO>: Unmounting\n");
    int err = umount(m_szMountDirectory);
    if (err < 0)
        SHOW_RESULT(unmount, err);
    //	mem_stats();
#endif  // ENABLE_ISOFS

    SuspendMediaStatusTimer();
    unregister_timer(m_TimerHandle);    
}

void
CMiniCDMgr::CheckMediaStatusCB(void* arg)
{
    //  DEBUGP(MINICDMGR, DBGLEV_INFO, "Checking media status\n");
    ((CMiniCDMgr*)arg)->GetMediaStatus(true);
}

// Queries the media status to see if a disc has been removed or inserted.
// TODO since this call is made from the timer, review whether 'try_put_event' (nonblocking)
//       should be used in places of 'put_event' (potentially blocking)

// New GetMediaStatus only saves -ENOMED and ENOERR as last events, and only generates
// events when transitioning between the two - all other events are ignored.
// Timeout is still in there - does it do anything?

int CMiniCDMgr::GetMediaGeometry() 
{
    cyg_uint32 len = sizeof( m_dgGeometry );
    Cyg_ErrNo err = cyg_io_get_config( m_hCD, IO_BLK_GET_CONFIG_GEOMETRY, (void*) &m_dgGeometry, &len );
    if( err < 0 ) {
        return -1;
    }
    return 0;
}

CMiniCDMgr*
CMiniCDMgr::GetInstance()
{


	if(s_pMiniCDMgr == NULL)
	{
		char *szDeviceName = "/dev/cda/";

		DEBUGP(MINICDMGR, DBGLEV_INFO, "%s: Opening device %s\n", __FUNCTION__, szDeviceName);

		// Mount the drive.
		cyg_io_handle_t	hCD;
		Cyg_ErrNo ret = cyg_io_lookup(szDeviceName, &hCD);

		if (ret == -ENOENT)
		{
			DEBUGP(MINICDMGR, DBGLEV_ERROR, "Could not open %s; No such device\n", szDeviceName);
			return 0;
		}
		else
		{
			DEBUGP(MINICDMGR, DBGLEV_INFO, "Opened device %s: %d\n", szDeviceName, ret);
		}

		cyg_uint32 len = 1;
		ret = cyg_io_set_config(hCD, IO_BLK_SET_CONFIG_POWER_UP, 0, &len);
		if (ret != ENOERR)
		{
			DEBUGP(MINICDMGR, DBGLEV_ERROR, "Could not turn on device %s\n", szDeviceName);
			return 0;
		}
		else
		{
			DEBUGP(MINICDMGR, DBGLEV_INFO, "Powered device %s: %d\n", szDeviceName, ret);
		}

		s_pMiniCDMgr = new CMiniCDMgr(hCD, szDeviceName, "/", true);
		
	}

	return s_pMiniCDMgr;
	
}

Cyg_ErrNo
CMiniCDMgr::GetMediaStatus(bool bSendEvent)
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
				DEBUGP(MINICDMGR, DBGLEV_INFO, "cdds: &&& Media changed &&&\n");
				GetMediaGeometry();
				if (bSendEvent)
				{
					m_bNoMedia = false;
					// Send a message to the UI.
					put_event(EVENT_MEDIA_INSERTED, 0);
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
					DEBUGP(MINICDMGR, DBGLEV_INFO, "cdds: &&& No media &&&\n");
					m_bNoMedia = true;  // Only send one media removed message.
					// Send a message to the UI.
					put_event(EVENT_MEDIA_REMOVED, 0);
				}
			}

			m_iLastUnitStatus = err;

			break;

		case -ETIME:
			
			// 5/5/02 dc- assume timeout on media sense is bad media
			if( !m_bMediaBad && bSendEvent) 
			{
				m_bMediaBad = true;
			//	put_event(EVENT_MEDIA_BAD, 0);
			}

			break;

		default:
			break;
    }

    return err;
}


// simple check - is this an audio cd?
bool
CMiniCDMgr::IsAudioCD()
{
	ClearTOC();

	if(FetchTOC() == ENOERR)
	{
		int i = 0;
		for (; i < m_toc.entries; i++)
			if (m_toc.entry_list[i].audio_p)
				return true;
	}

	return false;

}

void
CMiniCDMgr::ClearTOC()
{
    
    if (m_toc.entry_list)
        free((void*)m_toc.entry_list);
    memset((void*)&m_toc, 0, sizeof(m_toc));
 
}


Cyg_ErrNo
CMiniCDMgr::DoPacket(unsigned char *command, unsigned char *data, int iDataLength,int dir)
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
            DEBUG(MINICDMGR, DBGLEV_WARNING, "Request Sense failed %x\n", RSStatus);
        }
        DEBUGP(MINICDMGR, DBGLEV_WARNING, "%s line %d ASC: %x ASCQ: %x\n", __FUNCTION__, __LINE__, RequestSenseData[12],
               RequestSenseData[13]);
        Status = RequestSenseData[12];
    }
    return Status;
}

Cyg_ErrNo
CMiniCDMgr::FetchTOC()
{
    // dc- this probably shouldn't get here, but if it does, be sure to error our
    if ( m_bNoMedia )
        return -ENOMED;
   
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

    /* how many tracks on this disc? */
    int first = packet_data[2];
    int tracks = packet_data[3] - first + 1;

    if( tracks > MAX_TRACKS ) {
        // bad toc
        return -EIO;
    }


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
                return -EIO;
            }
            
            m_toc.entry_list[i].lba_startsector = lbaaddr;
            if (i > 0) {
                m_toc.entry_list[i - 1].lba_length = lbaaddr - m_toc.entry_list[i - 1].lba_startsector;

                // malformed TOC
                if( m_toc.entry_list[i-1].lba_length < 0 ) {
                    return -EIO;
                }
            }

        } else {
            return ret;
        }
    }

    // End with the leadout info.
    memcpy(packet_command, packet_command_TOC, 12);
    packet_command[6] = 0xAA;
    ret = DoPacket(packet_command, packet_data, 12, ATAPI_DATA_IN);
    if (ret == ENOERR)
    {
        long lbaaddr = packet_data[9] * 60 * 75 + packet_data[10] * 75 + packet_data[11] - 150;
        m_toc.entry_list[tracks - 1].lba_length = lbaaddr - m_toc.entry_list[tracks - 1].lba_startsector;

           // malformed TOC
        if( m_toc.entry_list[tracks-1].lba_length < 0 )
            return -EIO;
    } else {
        return ret;
    }

    return(ENOERR);
}

bool
CMiniCDMgr::SuspendDrive()
{

    cyg_uint32 len = sizeof(len);
	SuspendMediaStatusTimer();

    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(m_hCD, IO_BLK_SET_CONFIG_SLEEP, 0, &len) == ENOERR) 
    {
        DEBUG(MINICDMGR, DBGLEV_INFO, "CD suspend\n");
        return true;
    }
    else
    {
        DEBUG(MINICDMGR, DBGLEV_ERROR, "CD suspend failed\n");
        return false;
    }	

}

bool
CMiniCDMgr::WakeupDrive()
{


    cyg_uint32 len = sizeof(len);

    // put drive in idle (spin up, electronics on)
    if(cyg_io_set_config(m_hCD, IO_BLK_SET_CONFIG_WAKEUP, 0, &len) == ENOERR) 
    {
        DEBUG(MINICDMGR, DBGLEV_INFO, "CD wakeup\n");
  		ResumeMediaStatusTimer();
        return true;
    }
    else
    {
        DEBUG(MINICDMGR, DBGLEV_ERROR, "CD wakeup failed\n");
		ResumeMediaStatusTimer();
        return false;
    }	

}

//! Pause the media status test thread.
void
CMiniCDMgr::SuspendMediaStatusTimer()
{
    suspend_timer(m_TimerHandle);
}

//! Resume the media status test thread.
void
CMiniCDMgr::ResumeMediaStatusTimer()
{
    resume_timer(m_TimerHandle);
}


//! Ejects the CD.
//! Returns true if the CD was ejected, false otherwise.
bool
CMiniCDMgr::OpenTray()
{
    cyg_uint32 len = sizeof(len);
    bool ret;
    SuspendMediaStatusTimer();
    
    if( m_bMediaBad ) {
        // 5/5/02 dc- on bad media assume we need to reset the cd drive
        cyg_io_set_config(m_hCD, IO_BLK_SET_CONFIG_RESET, 0, &len);
    }
    // eject CD tray
    if(cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_TRAY_OPEN, 0, &len) == ENOERR)
    {
        DEBUG(MINICDMGR, DBGLEV_INFO, "CD tray opened\n");
        m_bTrayOpen = true;
        ret = true;
    }
    else
    {
        DEBUG(MINICDMGR, DBGLEV_ERROR, "CD tray open failed\n");
        ret = false;
    }
    ResumeMediaStatusTimer();
    return ret;
}

//! Closes the CD tray.
//! Returns true if the CD tray was closed, false otherwise.
bool
CMiniCDMgr::CloseTray()
{
    cyg_uint32 len = sizeof(len);
    bool ret;
    
    SuspendMediaStatusTimer();
    // eject CD tray
    if(cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_TRAY_CLOSE, 0, &len) == ENOERR)
    {
        DEBUG(MINICDMGR, DBGLEV_INFO, "CD tray Closed\n");
        m_bTrayOpen = false;
        ret = true;
    }
    else
    {
        DEBUG(MINICDMGR, DBGLEV_ERROR, "CD tray Close failed\n");
        ret = false;
    }
    ResumeMediaStatusTimer();

	return ret;
}

//! Toggles the CD tray open/closed.
void
CMiniCDMgr::ToggleTray()
{
    if (m_bTrayOpen)
        CloseTray();
    else
        OpenTray();
}
