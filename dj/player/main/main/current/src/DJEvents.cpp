// DJEvents.cpp: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#include <main/main/DJEvents.h>

#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>              // set_stream_event_data_t definition
#include <core/mediaplayer/MediaPlayer.h>
#include <core/mediaplayer/PlayStream.h>
#include <core/playmanager/PlayManager.h>  // interface to playmanager
#include <core/events/SystemEvents.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <extras/idlecoder/IdleCoder.h>
#include <gui/peg/peg.hpp>
#include <main/cddb/CDDBHelper.h>
#include <main/cddb/CDDBEvents.h>

#include <main/main/AppSettings.h>
#include <main/main/DJHelper.h>
#include <main/main/DJPlayerState.h>
#include <main/main/EventTypes.h>
#include <main/main/FatHelper.h>
#include <main/main/IsoHelper.h>
#include <main/main/ProgressWatcher.h>
#include <main/main/Recording.h>
#include <main/main/RecordingEvents.h>
#include <main/main/UpdateManager.h>

#include <main/ui/PlayerScreen.h>
#include <main/ui/PlaylistLoadEvents.h>
#include <main/ui/LibraryMenuScreen.h>     // for canceling fml queries...
#include <main/ui/common/UserInterface.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/SystemToolsMenuScreen.h> // For updating the image quicker...
#include <main/ui/Messages.h>
#include <main/ui/LineRecEventHandler.h>
#include <main/ui/LineRecEventHandler.h>
#include <main/testharness/testharness.h>

#include <util/diag/diag.h>
#include <util/timer/Timer.h>

#include <_modules.h>

#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/content/simplercontentmanager/SimplerContentManager.h>
#include <content/simplemetadata/SimpleMetadata.h>

#ifndef DISABLE_VOLUME_CONTROL
#include <io/audio/VolumeControl.h>        // interface to volume
#endif // DISABLE_VOLUME_CONTROL

#ifdef LINE_RECORDER_ENABLED
#include <main/main/LineRecorder.h>
#endif

#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#endif

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif

#include <extras/cdmetadata/CDMetadataEvents.h>
#include <extras/cdmetadata/DiskInfo.h>

#ifdef DDOMOD_MAIN_UPNPSERVICES
#include <main/upnpservices/djservices.h>
#define USE_UPNP_SERVICES
#endif

#ifdef DDOMOD_UTIL_TFTP
#include <util/tftp/fstftp.h>
#define USE_TFTP
#endif

#ifdef DDOMOD_MAIN_WEBCONTROL
#include <main/webcontrol/webcontrol.h>
#endif

#ifndef NO_UPNP
#include <main/djupnp/UPnPEvents.h>
#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#include <main/iml/query/QueryResultManager.h>
#include <main/iml/query/QueryResult.h>
#endif  // NO_UPNP

#ifdef DDOMOD_MAIN_UPNPSERVICES
#include <main/upnpservices/DJServiceEvents.h>
#endif  // DDOMOD_MAIN_UPNPSERVICES

#ifdef ENABLE_FIORI_TEST
// Fiori test uses the LED control
#include <main/main/LEDStateManager.h>
#endif  // ENABLE_FIORI_TEST

#ifdef PEG_BITMAP_WRITER
#include <datastream/fatfile/FileOutputStream.h>
#endif  // PEG_BITMAP_WRITER

#ifdef ENABLE_EXTERN_CONTROL
#include <main/externControl/ExternControl.h>
#endif


#include "CDCache.h"

#include <stdio.h>
#include <stdlib.h>
#include <network.h>
#include <io/net/Net.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( EV, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( EV );  // debugging prefix : (8) ev


#ifndef NULL
#define NULL 0
#endif

// fdecl
extern "C"
{
extern SDVOID oem_setsysdate(
    UINT16 tdate,
    UINT16 ttime,
    UTINY  ttime_tenths);
}


void
BreakPointFunc()
{
    DEBUGP( EV, DBGLEV_INFO, "Breakpoint function\n");
//    m_pDJPlayerState->->SaveState();
    print_mem_usage();
}


#ifndef NO_UPNP
static bool g_NetworkServicesStarted = 0;
#endif

CDJEvents::CDJEvents() :
    m_bIgnoreCDScan(true),
    m_bRestoreCDPlaylist(false),
    m_bPowerHeld(false),
    m_bPlayOnCDInsert(false),
    m_bRecordOnCDInsert(false),
    m_bSwitchSourceOnCDInsertion(true)
{
    //
    // Cache a pointer to the player state.
    //
    m_pDJPlayerState = CDJPlayerState::GetInstance();

    m_pUserInterface = NULL;
    //
    // Initialize the playmanager
    //
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = (CDJContentManager*)m_pPlayManager->GetContentManager();
    m_pSimplerContentManager = CSimplerContentManager::GetInstance();

    // Set up the default (playback) playstream
    CRecordingManager::GetInstance();

#ifndef DISABLE_VOLUME_CONTROL
    //
    // Initialize volume control
    //
    m_pVolumeControl = CVolumeControl::GetInstance();
#endif // DISABLE_VOLUME_CONTROL

    //
    // Null out some members
    //
    m_iTrackTime = 0;

    //
    // Intialize the CD cache.
    //
    CProgressWatcher::GetInstance()->SetTask(TASK_LOADING_CD_METADATA_CACHE);
    m_pCDCache = new CCDCache(DISK_METADATA_CACHE_PATH);
    CProgressWatcher::GetInstance()->UnsetTask(TASK_LOADING_CD_METADATA_CACHE);

    //
    // The CD insertion count tracks the number of times the cd has been inserted.
    // It is used to make sure asynchronous CD metadata query results match the current CD in the drive.
    //
    m_uiCDInsertionCount = 0;

}

CDJEvents::~CDJEvents() 
{
#ifdef USE_TFTP
    StopTFTPService();
#endif  // USE_TFTP

    delete m_pCDCache;
}

void
CDJEvents::SetUserInterface( IUserInterface* pUI ) 
{
    DEBUGP( EV, DBGLEV_INFO, "CDJEvents::SetUserInterface()\n");
    m_pUserInterface = pUI;
    m_pDJPlayerState->SetUserInterface(pUI);
    CRecordingManager::GetInstance()->SetUserInterface(pUI);
#ifdef LINE_RECORDER_ENABLED
	CLineRecorder::GetInstance()->SetUserInterface(pUI);
#endif
}

void
CDJEvents::RefreshInterface()
{
    if (m_pUserInterface)
    {
        DEBUGP( EV, DBGLEV_TRACE, "CDJEvents::RefreshInterface()\n");
#ifndef DISABLE_VOLUME_CONTROL
        m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
        m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
        m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
#endif // DISABLE_VOLUME_CONTROL
        m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
        //SynchPlayState();
        m_pUserInterface->SetTrackTime(m_iTrackTime);
        //synch view mode
        m_pUserInterface->SetUIViewMode(m_pDJPlayerState->GetUIViewMode());
    }
}

void
CDJEvents::SynchPlayState()
{
    DEBUGP( EV, DBGLEV_TRACE, "CDJEvents::SynchPlayState()\n");
    if (m_pPlayManager->GetPlayState() == CMediaPlayer::PLAYING)
        m_pUserInterface->NotifyPlaying();
    else if (m_pPlayManager->GetPlayState() == CMediaPlayer::PAUSED)
        m_pUserInterface->NotifyPaused();
    else
    {
        m_iTrackTime = 0;
        m_pUserInterface->SetTrackTime(m_iTrackTime);
        m_pUserInterface->NotifyStopped();
    }
}

// Called after a CD is inserted and ready to play.
// For data CDs, this is after the first pass during content update.
// For audio CDs, it's after the second pass.
// Returns true if everything's fine, false if an error occurred.
bool
CDJEvents::HandleCDInsertion(const cdda_toc_t* pTOC)
{
    // Tell the recording manager.
    DEBUGP( EV, DBGLEV_TRACE, "CDJEvents::HandleCDInsertion()\n");
    CRecordingManager::GetInstance()->NotifyCDInserted(pTOC);

    // Don't switch if we've just powered up and the source is HD.
    if (!m_bSwitchSourceOnCDInsertion)
    {
        m_bRestoreCDPlaylist = false;
        m_bSwitchSourceOnCDInsertion = true;
        return false;
    }

    IPlaylist* pCurrentPlaylist = m_pPlayManager->GetPlaylist();
    DBASSERT( EV, pCurrentPlaylist, "NULL playlist on CD insertion\n" );

retry:
    // If we've just started the system, then ask the dj player state for the last known playlist.
    if (!m_bRestoreCDPlaylist || FAILED(m_pDJPlayerState->LoadCurrentPlaylist()))
    {
        MediaRecordList records;
        m_pContentManager->GetMediaRecords(records, CMK_ALL, CMK_ALL, CMK_ALL, m_pDJPlayerState->GetCDDataSource()->GetInstanceID());
        if (pCurrentPlaylist)
        {
            m_pUserInterface->NotifyPlaylistCleared();
            pCurrentPlaylist->Clear();
            pCurrentPlaylist->AddEntries(records);
            pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
        }
    }

    // Start ripping if the user pressed record to close the CD tray.
    bool bSetSong = true;
    if (m_bRecordOnCDInsert)
    {
        pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0));
        CRecordingManager::GetInstance()->StartRipping();
        if (FAILED(DJSetCurrentOrNext()))
        {
            // All tracks on this CD have been recorded (or are bad), so stop ripping.
            CRecordingManager::GetInstance()->StopRipping(false);
            // Choose a new song to be first in the random ordering.
            if (!pCurrentPlaylist->IsEmpty())
                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(rand() % (pCurrentPlaylist->GetSize() - 1), m_pPlayManager->GetPlaylistMode()));
            pCurrentPlaylist->ReshuffleRandomEntries();
            pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
        }
        else
            bSetSong = false;
    }

    if (bSetSong && FAILED(DJSetCurrentOrNext()))
    {
        // This playlist is completely invalid.
        // If this was a playlist restored from drive, then create a playlist of all tracks on the disk.
        if (m_bRestoreCDPlaylist)
        {
            m_bRestoreCDPlaylist = false;
            goto retry;
        }
        else
        {
            // There are no playable tracks on this CD.
            // Clear the playlist and deconfigure the play manager.
            m_pPlayManager->Deconfigure();
            m_pUserInterface->NotifyPlaylistCleared();
            pCurrentPlaylist->Clear();

            // Turn off the light.
            CRecordingManager::GetInstance()->DisableRecording();
            if (m_bRecordOnCDInsert)
            {
                m_bRecordOnCDInsert = false;
                CRecordingManager::GetInstance()->StopRipping();
            }

            // Tell the user the bad news.
            if (m_pDJPlayerState->GetSource() == CDJPlayerState::CD)
                CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
            else
                m_pUserInterface->SetMessage(LS(SID_NO_TRACKS_FOUND), CSystemMessageString::REALTIME_INFO);
            return false;
        }
    }

    m_bRestoreCDPlaylist = false;
    m_bRecordOnCDInsert = false;

    if (m_bPlayOnCDInsert)
        m_pPlayManager->Play();

    return true;
}


// Opens the CD tray and notifies all the appropriate handlers.
void
CDJEvents::OpenCDTray()
{
    if( m_pDJPlayerState->GetSource() == CDJPlayerState::CD )
    {
        // Don't wait for the CD to unmount and messages to be passed back and forth.
        // Remove all CD content from the playlist and clear the screen.
        CRecordingManager::GetInstance()->NotifyCDRemoved();
        m_pPlayManager->Deconfigure();
        m_pUserInterface->ClearTrack();
        m_pUserInterface->NotifyPlaylistCleared();
        m_pPlayManager->GetPlaylist()->Clear();
    }

    // Ignore incoming CD content scan events.
    m_bIgnoreCDScan = true;

    // Clear the CD update variables
    CUpdateManager::GetInstance()->Reset();

    // Remove all the CD records from the data source
    m_pPlayManager->GetContentManager()->DeleteRecordsFromDataSource(m_pDJPlayerState->GetCDDataSource()->GetInstanceID());

    m_pUserInterface->NotifyMediaRemoved(m_pDJPlayerState->GetCDDataSource()->GetInstanceID());
#ifdef DDOMOD_DJ_BUFFERING
    // stop accessing cd files.  must be after playmanager deconfiguration, so that read requests stop
    // before they become unsupported by buffering (else lock possible)
    CBuffering::GetInstance()->NotifyCDRemoved();
#endif

    DEBUGP( EV , DBGLEV_TRACE, "ej:open\n",m_pDJPlayerState->GetSource()); 
    m_pUserInterface->NotifyCDTrayOpened();
    m_pDJPlayerState->GetCDDataSource()->OpenTray();
    m_pDJPlayerState->SetCDState(CDJPlayerState::NONE);
    m_pCDCache->NotifyCDRemoved();
    CDataSourceManager::GetInstance()->NotifyMediaRemoved(m_pDJPlayerState->GetCDDataSource()->GetInstanceID());

    // check to see if the user wants to auto play the cd on insert
    if (m_pDJPlayerState->GetUIPlayCDWhenInserted())
        m_bPlayOnCDInsert = true;
    else
        m_bPlayOnCDInsert = false;
    m_bRecordOnCDInsert = false;

}

// Called whenever a stream stops playing.
// Use this chance to do time-consuming operations.
void
CDJEvents::OnStop()
{
    CCDDBQueryManager::GetInstance()->SaveCache();
    CommitUpdates();
}


// A metadata creation function for the simple content manager.
static IMetadata* CreateSimpleMetadata()
{
    DEBUGP( EV, DBGLEV_TRACE, "CDJEvents::CreateSimpleMetadata()\n");
    return new CSimpleMetadata;
}


// Uncomment to estimate bytes/record after a content scan.
//#define PROFILE_MEM_USAGE

#ifdef PROFILE_MEM_USAGE
#include <cyg/infra/diag.h>
static unsigned long s_fordblks;
#endif  // PROFILE_MEM_USAGE

//#define PROFILE_SCAN_TIME

#ifdef PROFILE_SCAN_TIME
static cyg_tick_count_t s_tickContentScanStart;
static cyg_tick_count_t s_tickMetadataScanStart;
#endif  // PROFILE_SCAN_TIME


// returns -1 if event not handled
//          0 if handled and no further processing needed (override)
//          1 if handled and further processing needed
int
CDJEvents::HandleEvent( int key, void* data ) 
{
static int s_iCDTotalTrackCount = 0;
static int s_iCDCurrentTrackCount = 0;
static int s_iHDTotalTrackCount = 0;
static int s_iHDCurrentTrackCount = 0;

    if( !m_pUserInterface ) return -1;
    
    if (CEventRecorder::g_LoggingEnabled)
    {
        if ((key == EVENT_KEY_HOLD)  ||
            (key == EVENT_KEY_PRESS) ||
            (key == EVENT_KEY_RELEASE))
        {
            CEventRecorder::GetInstance()->ProcessEvent(key,data);
        }
    }

    switch( key ) {

        case EVENT_KEY_HOLD:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( EV, DBGLEV_INFO, "Key hold %d\n", keycode );
            
            
            switch( keycode )
            {
				case IR_KEY_MENU:
					DEBUGP( EV, DBGLEV_INFO, "IR_KEY_MENU hold ignored\n", keycode );
					break;
                case IR_KEY_POWER:
                case KEY_POWER:
                    if (m_bDebouncePower)
                    {
                        DEBUGP( EV, DBGLEV_TRACE, "Power hold ignored\n", keycode );
                    }
                    else if (!m_bPowerHeld)
                    {
                        m_bPowerHeld = true;
                        DEBUGP( EV, DBGLEV_INFO, "Power held\n");
                        // power on if we're off, hard power off if we're on
                        if (m_pDJPlayerState->GetPowerState() == CDJPlayerState::HARD_POWER_OFF)
                            m_pDJPlayerState->SetPowerState(CDJPlayerState::POWER_ON);
                        else
                            m_pDJPlayerState->SetPowerState(CDJPlayerState::HARD_POWER_OFF);												
                    }
                    break;

                default:
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = keycode;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_KEY_PRESS:
        {
            unsigned int keycode = (unsigned int)data;
			CDeviceState::GetInstance()->NotifyNotIdle();
            m_pDJPlayerState->NotifyDeviceActive();
            DEBUGP( EV, DBGLEV_INFO, "Key press %d\n", keycode );
            
            
            switch( keycode )
            {
                case IR_KEY_POWER:
                case KEY_POWER:
                    m_bDebouncePower = (DebounceButton(IR_KEY_POWER, 200) || DebounceButton(KEY_POWER, 200));
                    m_bPowerHeld = false;
                    break;

                case KEY_CD_EJECT:
                {
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    if (!m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
                    {
                        // Delay 5 seconds after a close to let the CD mount.
                        if (DebounceButton(KEY_CD_EJECT, 500))
                        {
                            DEBUGP( EV, DBGLEV_INFO, "Ignoring eject keypress\n");
                            return 1;
                        }

                        DEBUGP( EV , DBGLEV_TRACE, "ej:src?\n"); 
                        if (CPlayerScreen::GetPlayerScreen()->GetEventHandlerMode() != ePSLineRecEvents)
                        {
                            OpenCDTray();
                        }
                        else
                        {
                            m_pUserInterface->SetMessage(LS(SID_CANT_EJECT_TRAY_WHILE_RECORDING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
                            DEBUGP( EV , DBGLEV_TRACE, "ej:deny\n"); 
                        }
                    }
                    else
                    {
                        // Delay 3 seconds after an open to let the CD unmount.
                        if (DebounceButton(KEY_CD_EJECT, 300))
                        {
                            DEBUGP( EV, DBGLEV_INFO, "Ignoring eject keypress\n");
                            return 1;
                        }

                        //m_pUserInterface->NotifyMediaInserted(0);
                        //m_pUserInterface->NotifyCDTrayClosed();
                        m_pDJPlayerState->GetCDDataSource()->CloseTray();

                        // check to see if the user wants to auto play the cd on insert
                        if (m_pDJPlayerState->GetUIPlayCDWhenInserted())
                            m_bPlayOnCDInsert = true;
                        else
                            m_bPlayOnCDInsert = false;
                    }
                    break;
                }

                case IR_KEY_PRINT_SCREEN:
                {
#ifdef PEG_BITMAP_WRITER
                    DWORD bmp_len = 0;
                    UCHAR* bmp_buf;
					PegRect Rect;
					Rect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);

                    // do a screen print
                    PegThing pThing;
                    bmp_buf = pThing.Screen()->DumpWindowsBitmap( &bmp_len, Rect);

                    // save it to file
                    CFatFileOutputStream file;
                    if (SUCCEEDED(file.Open(PRINT_SCREEN_FILE)))
                    {
                        file.Write( bmp_buf, bmp_len );
                        file.Close();
                        DEBUGP( EV, DBGLEV_INFO, "Screen Bitmap Dumped to [%s]\n", PRINT_SCREEN_FILE );
                    }
#endif // PEG_BITMAP_WRITER
                    return 1;
                }

                case KEY_BREAK_POINT:
                {
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)IR_KEY_SAVE;
                    pt->MessageQueue()->Push(Mesg);
                    break;
                }

                case KEY_REFRESH_CONTENT:
                {
                    // Fake an append button press on the dharma
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)IR_KEY_ADD;
                    pt->MessageQueue()->Push(Mesg);
                    break;
                }

				case IR_KEY_PLAY:
				case KEY_PLAY:
                {
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    if (m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
                        m_bPlayOnCDInsert = true;

                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)data;
                    pt->MessageQueue()->Push(Mesg);
                    break;
                }

				case IR_KEY_RECORD:
				case KEY_RECORD:
                {
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    if (m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
                        m_bRecordOnCDInsert = true;

                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)data;
                    pt->MessageQueue()->Push(Mesg);
                    break;
                }

#if 0
                // Start teardown
                case IR_KEY_0_space:
                {
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_EXIT;
    				Mesg.pTarget = pt->Presentation();
                    Mesg.iData = 0;
                    pt->MessageQueue()->Push(Mesg);
                    break;
                }
#endif

#ifdef ENABLE_FIORI_TEST
                case KEY_EXIT:
                {
                    // For the fiori test, catch the KEY_EXIT event, and use it as
                    //  a trigger for 'do nothing mode' - oscillate the LED here, use red
                    SetLEDState( HEARTBEAT, true );
                }
#endif
                default:
                    if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
                        break;
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)data;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_KEY_RELEASE:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( EV, DBGLEV_INFO, "Key release %d\n", keycode );
            
            switch( keycode )
            {
                case IR_KEY_POWER:
                case KEY_POWER:
                    if (m_bDebouncePower)
                    {
                        DEBUGP( EV, DBGLEV_TRACE, "Power release ignored\n", keycode );
                    }
                    else if (!m_bPowerHeld)
                    {
                        DEBUGP( EV, DBGLEV_INFO, "Power pressed\n");
                        if (m_pDJPlayerState->GetPowerState() == CDJPlayerState::POWER_ON)
                        {                            		
                            m_pDJPlayerState->SetPowerState(CDJPlayerState::SOFT_POWER_OFF);                            
                        }
                        else
                            m_pDJPlayerState->SetPowerState(CDJPlayerState::POWER_ON);
                    }
                    break;

                default:
                    if ((m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON))
                        break;
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY_RELEASE;
                    Mesg.iData = keycode;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_STREAM_SET:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_SET\n");
#if DEBUG_LEVEL != 0
            print_mem_usage();
#endif

            // don't handle this event if we're not powered on
            if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
            {
                DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_SET Dropped\n");
                return 1;
            }

            set_stream_event_data_t* pSSED = (set_stream_event_data_t*)data;
            // Find the matching content record for this URL in the content manager.
            // If we can't do that, then the source that hosted this content has probably been lost.
            IMediaContentRecord* pCR = m_pContentManager->GetMediaRecord(pSSED->szURL);
            if (!pCR)
                pCR = m_pSimplerContentManager->GetMediaRecord(pSSED->szURL);

            if (pCR)
            {
                // If the event has metadata, then the track is probably from a data CD.
                // Metadata comes from the media player when using data CDs because we may
                // be scanning the data CD concurrently with playback.
                if (pSSED->pMediaPlayerMetadata)
                {
                    if (pCR->GetDataSourceID() == m_pDJPlayerState->GetCDDataSource()->GetInstanceID())
                    {
                        media_record_info_t mri;
                        mri.szURL = pSSED->szURL;
                        mri.bVerified = true;
                        mri.iCodecID = 0;
                        mri.iDataSourceID = pCR->GetDataSourceID();
                        mri.pMetadata = pSSED->pMediaPlayerMetadata->Copy();
                        m_pPlayManager->GetContentManager()->AddMediaRecord(mri);
                        pSSED->pMediaPlayerMetadata = 0;
                    }
                    else
                        // This isn't a data CD, so turn off the media player's metadata retrieval.
                        CMediaPlayer::GetInstance()->SetCreateMetadataFunction(0);
                }

                // Skip this track if it has already been ripped.
                bool bShouldSkip = CRecordingManager::GetInstance()->NotifyStreamSet(pSSED, pCR);
                if (bShouldSkip)
                {
                    if (FAILED(DJNextTrack()))
                    {
                        // We've reached the end of the playlist, what next?
                        if ((m_pDJPlayerState->GetSource() == CDJPlayerState::CD) &&
                            m_pDJPlayerState->GetUIEjectCDAfterRip())
                        {
                            // We've just ripped the entire CD, so eject the CD.
                            OpenCDTray();
                        }
                        else
                        {
                            // Stop ripping/recording and reset the playlist to the first song.
                            if (CRecordingManager::GetInstance()->IsRecording())
                                CRecordingManager::GetInstance()->StopRecording();
                            if (CRecordingManager::GetInstance()->IsRipping())
                                CRecordingManager::GetInstance()->StopRipping();
                            m_pPlayManager->Stop();
                            IPlaylist* pCurrentPlaylist = m_pPlayManager->GetPlaylist();
                            if (pCurrentPlaylist)
                            {
                                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
                                if (FAILED(DJSetCurrentOrNext()))
                                {
                                    // We couldn't set any track in the playlist, so tell the user.
                                    CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                                }
                            }
                        }
                    }
                }
                else
                {
                    // only send the set track message if we intend on playing it.
                    m_pUserInterface->SetTrack(pSSED, pCR);
                }
            }
            else
                DEBUG( EV, DBGLEV_WARNING, "Can't find matching content record for URL %s\n", pSSED->szURL );

            SynchPlayState();
            m_pPlayManager->HandleEvent(key, data);
#if DEBUG_LEVEL != 0
            print_mem_usage();
#endif
            return 1;
        }

        case EVENT_STREAM_AUTOSET:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_AUTOSET\n");
#if DEBUG_LEVEL != 0
            print_mem_usage();
#endif
            // don't handle this event if we're not powered on
            if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
            {
                DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_AUTOSET Dropped\n");
                return 1;
            }

            // Find the next entry with the given URL.
            change_stream_event_data_t* pChangeEvent = (change_stream_event_data_t*)data;
            IPlaylist *pPlaylist = m_pPlayManager->GetPlaylist();
            if (pPlaylist)
            {
                IPlaylistEntry *pCurrentEntry = pPlaylist->GetCurrentEntry();
                if (pCurrentEntry)
                {
                    IPlaylistEntry *pNextEntry = pCurrentEntry;
                    do
                    {
                        pNextEntry = pPlaylist->GetNextEntry(pNextEntry, m_pPlayManager->GetPlaylistMode());
                    } while (pNextEntry && strcmp(pNextEntry->GetContentRecord()->GetURL(), pChangeEvent->szURL));

                    if (pNextEntry != NULL)
                    {
                        // Tell the recording manager that the old stream has finished.
	                    bool bShouldStop = CRecordingManager::GetInstance()->NotifyStreamEnd(pChangeEvent);

                        // Set the current entry, and stop if the recording manager requested it.
                        pPlaylist->SetCurrentEntry(pNextEntry);
                        if (bShouldStop)
                            m_pPlayManager->Stop();

                        // Now tell the recording manager a new stream has been set.
                        IMediaContentRecord* pCR = pNextEntry->GetContentRecord();
                        bool bShouldSkip = CRecordingManager::GetInstance()->NotifyStreamSet(pChangeEvent, pCR);

                        // Try to get the next track ready. No big deal if it fails.
                        IPlaylistEntry* pNE = pPlaylist->GetNextEntry( pNextEntry, m_pPlayManager->GetPlaylistMode() );
                        if( pNE != pNextEntry ) {
                            CMediaPlayer::GetInstance()->SetNextSong(pNE);
                        }

                        // Skip this track if it has already been ripped.
                        if (bShouldSkip)
                        {
                            if (FAILED(DJNextTrack()))
                            {
                                // We've just ripped the entire CD, so eject the CD.
                                if ((m_pDJPlayerState->GetSource() == CDJPlayerState::CD) &&
                                    m_pDJPlayerState->GetUIEjectCDAfterRip())
                                {
                                    OpenCDTray();
                                }
                                else
                                {
                                    // Stop ripping/recording and reset the playlist to the first song.
                                    if (CRecordingManager::GetInstance()->IsRecording())
                                        CRecordingManager::GetInstance()->StopRecording();
                                    if (CRecordingManager::GetInstance()->IsRipping())
                                        CRecordingManager::GetInstance()->StopRipping();
                                    m_pPlayManager->Stop();
                                    IPlaylist* pCurrentPlaylist = m_pPlayManager->GetPlaylist();
                                    if (pCurrentPlaylist)
                                    {
                                        pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
                                        if (FAILED(DJSetCurrentOrNext()))
                                        {
                                            // We couldn't set any track in the playlist, so tell the user.
                                            CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                                        }
                                    }
                                }
                                OnStop();
                            }
                        }
                        else
                        {
                            m_pUserInterface->SetTrack(pChangeEvent, pCR);
                        }
                    }
                }
            }

            delete [] pChangeEvent->szURL;
            delete pChangeEvent->pPreviousStream;
            delete pChangeEvent;

#if DEBUG_LEVEL != 0
            print_mem_usage();
#endif
	        return 1;
        }

        case EVENT_STREAM_PROGRESS:
            //DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_PROGRESS\n");
            m_pUserInterface->SetTrackTime( (int) data );
            return 1;

        case EVENT_STREAM_END:
        case EVENT_STREAM_FAIL:
        {
            bool bRipTrack = CRecordingManager::GetInstance()->IsRippingSingle();
            bool bRipping = CRecordingManager::GetInstance()->IsRipping();
            bool bShouldStop = false;
            if (key == EVENT_STREAM_END)
            {
                DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_END\n");
                bShouldStop = CRecordingManager::GetInstance()->NotifyStreamEnd((change_stream_event_data_t*)data);
            }
            else
            {
                DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_FAIL\n");
                CRecordingManager::GetInstance()->NotifyStreamAbort((change_stream_event_data_t*)data);
            }

            change_stream_event_data_t* pChangeEvent = (change_stream_event_data_t*)data;
            if (pChangeEvent)
            {
                delete [] pChangeEvent->szURL;
                delete pChangeEvent->pPreviousStream;
                delete pChangeEvent;
            }

            ERESULT res = DJNextTrack();

            if (FAILED(res))
            {
                // If we've just ripped the entire CD, then eject the CD.
                if (bRipping && !bRipTrack &&
                    (m_pDJPlayerState->GetSource() == CDJPlayerState::CD) &&
                    m_pDJPlayerState->GetUIEjectCDAfterRip())
                {
                    OpenCDTray();
                }
                // Otherwise, stop playback and set the first song in the playlist.
                else
                {
                    if (CRecordingManager::GetInstance()->IsRecording())
                        CRecordingManager::GetInstance()->StopRecording();
                    if (CRecordingManager::GetInstance()->IsRipping())
                        CRecordingManager::GetInstance()->StopRipping();
                    if (bRipping && !bRipTrack)
                        CRecordingManager::GetInstance()->RestorePlayMode();
                    IPlaylist* pCurrentPlaylist = m_pPlayManager->GetPlaylist();
                    if (pCurrentPlaylist)
                    {
                        pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
                        if (FAILED(DJSetCurrentOrNext()))
                        {
                            // We couldn't set any track in the playlist, so tell the user.
                            CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                        }
                    }
                    m_pPlayManager->Stop();
                }
                OnStop();
            }
            else if (bShouldStop)
            {
                m_pPlayManager->Stop();
                OnStop();
            }
            SynchPlayState();
            return 1;
        }

        case EVENT_STREAM_PLAYING:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_PLAYING\n");
            CRecordingManager::GetInstance()->NotifyStreamPlaying();
            m_pPlayManager->HandleEvent(key, data);
            SynchPlayState();
            return 1;

        case EVENT_STREAM_PAUSED:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_PAUSED\n");
            CRecordingManager::GetInstance()->NotifyStreamPaused();
            m_pPlayManager->HandleEvent(key, data);
            SynchPlayState();
            return 1;

        case EVENT_STREAM_STOPPED:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_STREAM_STOPPED\n");
            CRecordingManager::GetInstance()->NotifyStreamAbort((change_stream_event_data_t*)data);
            m_pPlayManager->HandleEvent(key, data);
            SynchPlayState();
            if (!CRecordingManager::GetInstance()->IsRipping() && 
                    ((m_pPlayManager->GetPlayState() == CMediaPlayer::STOPPED) ||
                    (m_pPlayManager->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)))
                OnStop();
            return 1;

        case EVENT_MEDIA_REMOVED:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_MEDIA_REMOVED\n");

            if (!m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
            {
                OpenCDTray();
            }

            return 1;

        case EVENT_MEDIA_INSERTED:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_MEDIA_INSERTED\n");

            // don't handle this event if we're not powered on
            if (m_pDJPlayerState->GetPowerState() != CDJPlayerState::POWER_ON)
            {
                DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_MEDIA_INSERTED Dropped\n");
                return 1;
            }

            // This is a closed system, so a media insertion message means that the CD drive tray closed.
            // Check the CD data source to see if a CD is in the tray.
            if (m_pDJPlayerState->GetCDDataSource()->GetMediaStatus(false) == ENOERR)
            {
                // Update the DJ state.
                m_pDJPlayerState->SetScanningCD(true);

                // Make sure that the CDDataSource knows that the tray is closed.
                // The user may have pushed the tray closed.
                m_pDJPlayerState->GetCDDataSource()->TrayIsClosed();

                // Increase the CD insertion count.
                ++m_uiCDInsertionCount;

                // Pay attention to CD scanning events.
                m_bIgnoreCDScan = false;

                // if we're in mid line recording, cancel out and record the cd instead.  logic being, they must have injected the
                // tray by hitting record in line-in mode, and so we iterpret this as a cd-rip, if belatedly
                if (CPlayerScreen::GetPlayerScreen()->GetEventHandlerMode() == ePSLineRecEvents)
                {
                    DEBUGP( EV , DBGLEV_TRACE, "ev:canceling recording\n"); 
                    CPlayerScreen::GetPlayerScreen()->GetLineRecEventHandler()->CancelRecording();
                    DEBUGP( EV , DBGLEV_TRACE, "ev:deferring rec...\n"); 
                    m_bRecordOnCDInsert = true;
                }

                // There's a CD in there, so switch to CD mode.
                DEBUGP( EV, DBGLEV_TRACE, "Event: CD Inserted\n");
                if (m_bSwitchSourceOnCDInsertion)
                {
                    m_pDJPlayerState->SetSource(CDJPlayerState::CD, true, false);
                    // Reach into the player screen and change the event handler.
                    // it'd be nice if there was a cleaner interface to do this.
                    CPlayerScreen::GetPlayerScreen()->SetEventHandlerMode(ePSPlaybackEvents);

                    // Stop playback, drop active queries, and clear the playlist.
                    CRecordingManager::GetInstance()->StopRipping();
                    CRecordingManager::GetInstance()->StopRecording();
                    m_pPlayManager->Deconfigure();
                    
                    m_pUserInterface->ClearTrack();
                    m_pPlayManager->GetPlaylist()->Clear();
                }
                m_pUserInterface->NotifyMediaInserted((int)data);

                // Start the refresh ourselves instead of using the default event handler
                // so we can get the scan ID.
                m_usCDScanID = m_pPlayManager->RefreshContent((int)data);
//                m_pPlayManager->HandleEvent(key, data);
            }
            return 1;

        case EVENT_MEDIA_BAD:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_MEDIA_BAD\n");

            // Update the DJ state.
            m_pDJPlayerState->SetScanningCD(false);

            // eject the tray
            OpenCDTray();

            // notify the user
            m_pUserInterface->SetMessage(LS(SID_CANT_READ_CD), CSystemMessageString::REALTIME_INFO);
            return 1;

        case EVENT_CONTENT_UPDATE_BEGIN:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_UPDATE_BEGIN\n");

            int iDataSourceID = GET_DATA_SOURCE_ID(data);

            CMediaPlayer::GetInstance()->SetCreateMetadataFunction(0);
            if (iDataSourceID == m_pDJPlayerState->GetCDDataSource()->GetInstanceID())
            {
                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
                if ((m_bIgnoreCDScan) || (m_usCDScanID != GET_SCAN_ID(data)))
                {
                    DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)GET_SCAN_ID(data));
                    return 1;
                }

                DEBUGP( EV, DBGLEV_TRACE, "Event: Looking For CD Info\n");
                m_pUserInterface->SetMessage(LS(SID_SCANNING_FOR_CD_INFO), CSystemMessageString::STATUS);
                s_iCDTotalTrackCount = 0;
                s_iCDCurrentTrackCount = 0;
            }
            else if (iDataSourceID == m_pDJPlayerState->GetFatDataSource()->GetInstanceID())
            {
                DEBUGP( EV, DBGLEV_INFO, "Event: Scanning HD\n");
                print_mem_usage();
                s_iHDTotalTrackCount = 0;
                s_iHDCurrentTrackCount = 0;

                // Clear the idle coder queue since it will be rebuilt from the file system.
                CIdleCoder::GetInstance()->Halt();
                CIdleCoder::GetInstance()->RemoveAllJobs();

                // Also clear the queue of encoded files waiting to be copied.
                CRecordingManager::GetInstance()->ClearEncodingUpdates();

#ifdef PROFILE_MEM_USAGE
                struct mallinfo mem_info = mallinfo();
                s_fordblks = mem_info.fordblks;
#endif  // PROFILE_MEM_USAGE
#ifdef PROFILE_SCAN_TIME
                s_tickContentScanStart = cyg_current_time();
#endif  // PROFILE_SCAN_TIME
            }
            m_pUserInterface->NotifyContentUpdateBegin(iDataSourceID);
            m_pPlayManager->HandleEvent(key, data);
            return 1;
        }

        case EVENT_CONTENT_UPDATE:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_UPDATE\n");
            content_record_update_t* pContentUpdate = (content_record_update_t*)data;
            
            // If this content update comes from the hard drive, then tell the interface to update its count.
            if (pContentUpdate->iDataSourceID == m_pDJPlayerState->GetFatDataSource()->GetInstanceID())
            {
                s_iHDTotalTrackCount += pContentUpdate->media.Size();
                DEBUGP( EV, DBGLEV_INFO, "Event: %d HD tracks scanned\n", s_iHDTotalTrackCount);
                print_mem_usage();
                m_pUserInterface->NotifyContentUpdate(pContentUpdate->iDataSourceID, 0, s_iHDTotalTrackCount);
            }
            // CD updates come in one batch, instead of chunked like the HD.
            else if (pContentUpdate->iDataSourceID == m_pDJPlayerState->GetCDDataSource()->GetInstanceID())
            {
                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
                if ((m_bIgnoreCDScan) || (m_usCDScanID != pContentUpdate->usScanID))
                {
                    DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)pContentUpdate->usScanID);
                    // Clean up the data structure.
                    for (int i = 0; i < pContentUpdate->media.Size(); ++i)
                    {
                        free(pContentUpdate->media[i].szURL);
                        delete pContentUpdate->media[i].pMetadata;
                    }
                    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
                        free(pContentUpdate->playlists[i].szURL);
                    delete pContentUpdate;

                    return 1;
                }

                s_iCDTotalTrackCount += pContentUpdate->media.Size();
                // Don't cache audio CD metadata.
                if (m_pDJPlayerState->GetCDDataSource()->GetAudioSessionCount() != s_iCDTotalTrackCount)
                {
                    m_pDJPlayerState->SetCDState(CDJPlayerState::DATA);
                    s_iCDCurrentTrackCount = m_pCDCache->NotifyContentUpdate(pContentUpdate, m_pContentManager, 5);
                    if (s_iCDCurrentTrackCount)
                    {
                        // Update the DJ state.
                        m_pDJPlayerState->SetScanningCD(false);
                    }
                    return 1;
                }
                else
                {
                    m_pDJPlayerState->SetCDState(CDJPlayerState::AUDIO);
                    s_iCDCurrentTrackCount = 0;
                }
                m_pUserInterface->NotifyContentUpdate(pContentUpdate->iDataSourceID, 0, s_iCDTotalTrackCount);
            }
            break;
        }

        case EVENT_CONTENT_UPDATE_END:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_UPDATE_END\n");
            m_pPlayManager->HandleEvent(key, data);
            int iDataSourceID = GET_DATA_SOURCE_ID(data);

            if (iDataSourceID == m_pDJPlayerState->GetCDDataSource()->GetInstanceID())
            {
                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
                if ((m_bIgnoreCDScan) || (m_usCDScanID != GET_SCAN_ID(data)))
                {
                    DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)GET_SCAN_ID(data));
                    return 1;
                }

                // Notify the UI so it can recache the top level queries.
                m_pUserInterface->NotifyContentUpdateEnd(iDataSourceID, s_iCDTotalTrackCount);

                // Only print to system message text if we actually have tracks
                if( s_iCDTotalTrackCount > 0 )
                {
                    char szNumber[32];
                    sprintf(szNumber, " %d ", s_iCDTotalTrackCount);
                    TCHAR tszNumber[32];
                    CharToTcharN(tszNumber, szNumber, 31);

                    TCHAR tszMessage[256];
                    tstrcpy(tszMessage, LS(SID_SCANNING_INFO_FOR));
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_CD_TRACKS));
                    m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
                }

                CCDDataSource* pCDDS = m_pDJPlayerState->GetCDDataSource();

                // If the audio track count doesn't equal the total track count, then this is a data CD.
                if (pCDDS->GetAudioSessionCount() < s_iCDTotalTrackCount)
                {
                    CMediaPlayer::GetInstance()->SetCreateMetadataFunction(CreateSimpleMetadata);

                    if (!HandleCDInsertion(pCDDS->GetTOC()))
                        return 1;
                }
            }
            else if (iDataSourceID == m_pDJPlayerState->GetFatDataSource()->GetInstanceID())
            {
                DEBUGP( EV, DBGLEV_INFO, "Event: Finished scanning HD\n");
                print_mem_usage();
#ifdef PROFILE_MEM_USAGE
                struct mallinfo mem_info = mallinfo();
                DEBUGP( EV, DBGLEV_INFO, "Event: Bytes: %d Records: %d Bytes/record: %d\n",
                    s_fordblks - mem_info.fordblks, m_pContentManager->GetMediaRecordCount(), (s_fordblks - mem_info.fordblks) / m_pContentManager->GetMediaRecordCount());
#endif  // PROFILE_MEM_USAGE

#ifdef PROFILE_SCAN_TIME
                s_tickMetadataScanStart = cyg_current_time();
                DEBUGP( EV, DBGLEV_INFO, "Event: Tick count: %d\n", s_tickMetadataScanStart - s_tickContentScanStart);
#endif  // PROFILE_SCAN_TIME

                // Save the registry since the idle coding queue has been rebuilt.
                m_pDJPlayerState->SaveRegistry();

                // Notify the UI so it can recache the top level queries.
                m_pUserInterface->NotifyContentUpdateEnd(iDataSourceID, s_iHDTotalTrackCount);

            }
            return 1;
        }   

        case EVENT_CONTENT_METADATA_UPDATE_BEGIN:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_METADATA_UPDATE_BEGIN\n");

                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
            if ((GET_DATA_SOURCE_ID(data) == m_pDJPlayerState->GetCDDataSource()->GetInstanceID()) &&
                ((m_bIgnoreCDScan) || (m_usCDScanID != GET_SCAN_ID(data))))
            {
                DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)GET_SCAN_ID(data));
                return 1;
            }

            m_pUserInterface->NotifyMetadataUpdateBegin(GET_DATA_SOURCE_ID(data));
            break;
        }

        case EVENT_CONTENT_METADATA_UPDATE:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_METADATA_UPDATE\n");
            content_record_update_t* pContentUpdate = (content_record_update_t*)data;
            TCHAR tszMessage[256];
            if (pContentUpdate->iDataSourceID == m_pDJPlayerState->GetCDDataSource()->GetInstanceID())
            {
                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
                if ((m_bIgnoreCDScan) || (m_usCDScanID != pContentUpdate->usScanID))
                {
                    DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)pContentUpdate->usScanID);
                    // Clean up the data structure.
                    for (int i = 0; i < pContentUpdate->media.Size(); ++i)
                    {
                        free(pContentUpdate->media[i].szURL);
                        delete pContentUpdate->media[i].pMetadata;
                    }
                    for (int i = 0; i < pContentUpdate->playlists.Size(); ++i)
                        free(pContentUpdate->playlists[i].szURL);
                    delete pContentUpdate;

                    return 1;
                }

                s_iCDCurrentTrackCount += pContentUpdate->media.Size();
                {
                    // construct and send our system message
                    char szNumber[32];
                    TCHAR tszNumber[32];
                    tstrcpy(tszMessage, LS(SID_SCANNED));
                    sprintf(szNumber, " %d ", s_iCDCurrentTrackCount);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_OF));
                    sprintf(szNumber, " %d ", s_iCDTotalTrackCount);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_CD_TRACKS));
                    m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
                }

                for (int i = 0; i < pContentUpdate->media.Size(); ++i)
                {
                    TCHAR* temp;
                    if (pContentUpdate->media[i].pMetadata && !IsCDDA(pContentUpdate->media[i].szURL) && (pContentUpdate->media[i].pMetadata->GetAttribute(MDA_TITLE, (void**)&temp) == METADATA_NO_VALUE_SET))
                    {
                        TCHAR tszTitle[256];
                        const char* szFilename = FilenameFromIsoURLInPlace(pContentUpdate->media[i].szURL);
                        // Make two assumptions here:
                        // 1. All filenames will be 256 characters or less (this is currently true for our file system).
                        // 2. All recognized codec extensions are 3 characters long (currently true: MP3, WMA, WAV, RAW).
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszTitle, szFilename, strlen(szFilename) - 4));
                    }
                }
                m_pCDCache->NotifyMetadataUpdate(pContentUpdate);
                m_pUserInterface->NotifyMetadataUpdate(pContentUpdate->iDataSourceID, s_iCDCurrentTrackCount, s_iCDTotalTrackCount);
            }
            else if (pContentUpdate->iDataSourceID == m_pDJPlayerState->GetFatDataSource()->GetInstanceID())
            {
                s_iHDCurrentTrackCount += pContentUpdate->media.Size();
                {
                    // construct and send our system message
                    char szNumber[32];
                    TCHAR tszNumber[32];
                    tstrcpy(tszMessage, LS(SID_SCANNED));
                    sprintf(szNumber, " %d ", s_iHDCurrentTrackCount);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_OF));
                    sprintf(szNumber, " %d ", s_iHDTotalTrackCount);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_HD_TRACKS));
                    m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
                }
                DEBUGP( EV, DBGLEV_TRACE, "%w\n", tszMessage);
                int iRawCodecID = CCodecManager::GetInstance()->FindCodecID("raw");
                for (int i = 0; i < pContentUpdate->media.Size(); ++i)
                {
                    TCHAR* temp;
                    media_record_info_t& mediaRecord = pContentUpdate->media[i];
                    if (mediaRecord.pMetadata && (mediaRecord.pMetadata->GetAttribute(MDA_TITLE, (void**)&temp) == METADATA_NO_VALUE_SET))
                    {
                        TCHAR tszTitle[256];
                        const char* szFilename = FilenameFromURLInPlace(pContentUpdate->media[i].szURL);
                        // Make two assumptions here:
                        // 1. All filenames will be 256 characters or less (this is currently true for our file system).
                        // 2. All recognized codec extensions are 3 characters long (currently true: MP3, WMA, WAV, RAW).
                        pContentUpdate->media[i].pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tszTitle, szFilename, strlen(szFilename) - 4));
                    }
                    // If this track is a raw file that needs to be encoded, then add it to the encoder queue.
                    if (mediaRecord.iCodecID == iRawCodecID)
                    {
                        char szOutURL[256 + 6 /* file:/ */ + 1];
                        STAT stat;
                        pc_stat(const_cast<char*>(FullFilenameFromURLInPlace(mediaRecord.szURL)), &stat);
                        bool bEncode = false;

                        // If the creation time is not zero, then this raw file has an encoded
                        // version ready for copying.
                        if (stat.st_ctime.time)
                        {
                            // Does the encoded file exist?
                            strcpy(szOutURL, mediaRecord.szURL);
                            strcpy(&(szOutURL[strlen(mediaRecord.szURL) - 4]), ".mxx");
                            DEBUGP( EV, DBGLEV_TRACE, "Event: Encoded file ready: Raw: %s Encoded: %s\n", mediaRecord.szURL, szOutURL);

                            if (FileExists(FullFilenameFromURLInPlace(szOutURL)))
                            {
                                // Yep, add it to the queue of files to copy when we get a chance.
                                CRecordingManager::GetInstance()->AddEncodingUpdate(mediaRecord.szURL, szOutURL);
                            }
                            else
                            {
                                // Nope.  Add the raw file to the encoding queue.
                                bEncode = true;
                                oem_setsysdate(stat.st_ctime.date, 0, 0);
                                pc_touch(const_cast<char*>(FullFilenameFromURLInPlace(mediaRecord.szURL)));
                            }
                        }
                        else
                            bEncode = true;

                        // If the creation date is not zero, then this raw file should be
                        // encoded with the bitrate that the date specifies.
                        if (stat.st_ctime.date && bEncode)
                        {
                            if (!stat.st_ctime.time)
                            {
                                strcpy(szOutURL, mediaRecord.szURL);
                                strcpy(&(szOutURL[strlen(mediaRecord.szURL) - 4]), ".mxx");
                            }
                            DEBUGP( EV, DBGLEV_TRACE, "Event: Adding raw file to encode queue: Raw: %s Out: %s Bitrate: %d\n", mediaRecord.szURL, szOutURL, (int)stat.st_ctime.date);
                            CIdleCoder::GetInstance()->Enqueue(mediaRecord.szURL, szOutURL, (int)stat.st_ctime.date);
                        }
                    }
                }
                m_pUserInterface->NotifyMetadataUpdate(pContentUpdate->iDataSourceID, s_iHDCurrentTrackCount, s_iHDTotalTrackCount);
            }

            break;
        }

        case EVENT_CONTENT_METADATA_UPDATE_END:
        {
            DEBUGP( EV, DBGLEV_INFO, "\n\n*******************\nEvent: EVENT_CONTENT_METADATA_UPDATE_END\n");

            int iDataSourceID = GET_DATA_SOURCE_ID(data);
            m_pPlayManager->HandleEvent(key, data);

//            m_pUserInterface->RefreshTrack();
            if (iDataSourceID == m_pDJPlayerState->GetFatDataSource()->GetInstanceID())
            {
                DEBUGP( EV, DBGLEV_INFO, "Event: Finished scanning HD pt. 2\n");
                print_mem_usage();
#ifdef PROFILE_MEM_USAGE
                struct mallinfo mem_info = mallinfo();
                DEBUGP( EV, DBGLEV_INFO, "Event: Bytes: %d Records: %d Bytes/record: %d\n",
                    s_fordblks - mem_info.fordblks, m_pContentManager->GetMediaRecordCount(), (s_fordblks - mem_info.fordblks) / m_pContentManager->GetMediaRecordCount());
#endif  // PROFILE_MEM_USAGE
#ifdef PROFILE_SCAN_TIME
                cyg_tick_count_t tick = cyg_current_time();
                DEBUGP( EV, DBGLEV_INFO, "Event: Tick count: 1st pass: %d 2nd pass: %d Total: %d\n",
                    (int)(s_tickMetadataScanStart - s_tickContentScanStart),
                    (int)(tick - s_tickMetadataScanStart),
                    (int)(tick - s_tickContentScanStart));
#endif  // PROFILE_SCAN_TIME

                CProgressWatcher::GetInstance()->UnsetTask(TASK_REFRESHING_CONTENT);

                m_pDJPlayerState->SetScanningHD(false);

                // Notify the UI so it can recache the top level queries.
                m_pUserInterface->NotifyMetadataUpdateEnd(iDataSourceID, s_iHDTotalTrackCount);
            }

            // If we're in CD mode and an update just finished, then create a new playlist of
            // all the content on the CD and start playback from the beginning.
            CCDDataSource* pCDDS = m_pDJPlayerState->GetCDDataSource();
            if (iDataSourceID == pCDDS->GetInstanceID() && !m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
            {
                m_pCDCache->NotifyMetadataUpdateEnd();

                // If the CD has been ejected or if this scan event is for an older scan ID then ignore the event.
                if ((m_bIgnoreCDScan) || (m_usCDScanID != GET_SCAN_ID(data)))
                {
                    DEBUGP( EV, DBGLEV_INFO, "Ignoring old CD content scan event (current: %d event: %d)\n", (int)m_usCDScanID, (int)GET_SCAN_ID(data));
                    return 1;
                }

                CMediaPlayer::GetInstance()->SetCreateMetadataFunction(0);

    			// This is a cd notification, so check for updates
                if( pCDDS->GetDataSessionCount() )
                {
                    update_t updates = CUpdateManager::GetInstance()->CheckForUpdates(RESTORE_CD_CONFIG_PATH);

                    if( updates != UM_UPDATE_UNAVAIL ) {
                        if( (updates & (UM_UPDATE_FIRMWARE|UM_UPDATE_SYSTEM) ) )
                        {                           
                            m_pUserInterface->SetMessage(LS(SID_NEW_FIRMWARE_VERSION_FOUND_ON_CD), CSystemMessageString::INFO);
                        }

                        if( (updates & UM_UPDATE_CDDBUPDA) )
                        {
                            m_pUserInterface->SetMessage(LS(SID_NEW_CDDB_VERSION_FOUND_ON_CD), CSystemMessageString::INFO);
                        }

                    }
                    else
                    {
                        DEBUGP( EV, DBGLEV_INFO, "No player updates found on CD\n");
                    }
                }

                bool bOneLocalHit = false;  // Remember if there's just one local CDDB hit.
                if (s_iCDCurrentTrackCount)
                {
	                // Get a list of matching disks from cddb.
                    if (pCDDS->GetAudioTrackRecord(m_pContentManager, 0))
                    {
                        m_bLocalHits = false;
                        DiskInfoVector svDisks;
                        // First check the local cache for the disk.
                        const cdda_toc_t* pTOC = pCDDS->GetTOC();
                        if (CCDDBQueryManager::GetInstance()->GetDiskInfoCache(pTOC, svDisks) && !svDisks.IsEmpty())
                        {
                            SetCDRecordMetadata(m_pContentManager, m_pDJPlayerState->GetCDDataSource(), svDisks[0]);
                        }
                        else if (CDDBGetDiskInfoLocal(pTOC, svDisks))
                        {
                            PrintDiskList(svDisks);
                            // If there is exactly one matching disk in cddb, then use that record.
                            if (svDisks.Size() == 1)
                            {
                                SetCDRecordMetadata(m_pContentManager, pCDDS, svDisks[0]);
                                ClearDiskList(svDisks);
                                bOneLocalHit = true;
                            }
                            else if (svDisks.IsEmpty())
                            {
#if defined(DDOMOD_MAIN_CDDB) && defined(CDDB_QUERY_ONLINE)
                                // No hits locally?  Check online.
                                m_pUserInterface->SetMessage(LS(SID_ACCESSING_CDDB_ONLINE), CSystemMessageString::REALTIME_INFO);
                                if (!CCDDBQueryManager::GetInstance()->GetDiskInfoOnlineAsynch(pTOC, m_uiCDInsertionCount))
                                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)m_uiCDInsertionCount);
#else
                                // Send an event if there are no hits for this CD's freedb ID.
                                CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)m_uiCDInsertionCount);
#endif
                            }
                            else
                            {
#if defined(DDOMOD_MAIN_CDDB) && defined(CDDB_QUERY_ONLINE)
                                // If there are multiple local hits, then also check online.
                                m_pUserInterface->SetMessage(LS(SID_ACCESSING_CDDB_ONLINE), CSystemMessageString::REALTIME_INFO);
                                m_bLocalHits = true;
                                CCDDBQueryManager::GetInstance()->GetDiskInfoOnlineAsynch(pTOC, m_uiCDInsertionCount);
#endif
                                // Send an event if there are multiple hits for this CD's freedb ID.
                                cd_multiple_hit_event_t* pEvent = new cd_multiple_hit_event_t;
                                pEvent->iDataSourceID = pCDDS->GetInstanceID();
                                pEvent->uiDiskID = m_uiCDInsertionCount;
                                for (int i = 0; i < svDisks.Size(); ++i)
                                    pEvent->svDisks.PushBack(svDisks[i]);
                                CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_MULTIPLE_HITS, (void*)pEvent);
                                
                                // don't play or record on insert when there are a few sets of metadata to choose from
                                m_bPlayOnCDInsert = false;
                                m_bRecordOnCDInsert = false;
                            }
                        }
#ifdef LOOKUP_SYNCH
                        else
                        {
                            // Send an event if there are no hits for this CD's freedb ID.
                            CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)m_uiCDInsertionCount);
                        }
#endif  // LOOKUP_SYNCH
                    }

                    // Keep the UI synchronized with the update
                    m_pUserInterface->NotifyMetadataUpdateEnd(iDataSourceID, s_iCDTotalTrackCount);

                    // Erase the "Looking For CD Info" Status
                    DEBUGP( EV, DBGLEV_TRACE, "Event: Done Looking For CD Info\n");

                    // If the audio track count equals the total track count, then this is an audio CD.
                    if (pCDDS->GetAudioSessionCount() == s_iCDTotalTrackCount)
                    {
                        if (!HandleCDInsertion(pCDDS->GetTOC()))
                            return 1;
                    }
                    // If this is a multisession disk with one local CDDB hit then update the UI.
                    else if (bOneLocalHit)
                    {
                        m_pUserInterface->RefreshTrack();
                    }
                }
                else
                {
                    // There are no audio tracks on this CD, so tell the user the bad news.
                    if (m_pDJPlayerState->GetSource() == CDJPlayerState::CD)
                        CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                    else
                        m_pUserInterface->SetMessage(LS(SID_NO_TRACKS_FOUND), CSystemMessageString::REALTIME_INFO);

                    // Turn off the light.
                    CRecordingManager::GetInstance()->DisableRecording();
                }
            
                // check to see if the user wants to auto play the cd on insert
                if (m_pDJPlayerState->GetUIPlayCDWhenInserted())
                    m_bPlayOnCDInsert = true;
                else
                    m_bPlayOnCDInsert = false;
                
                // Update the DJ state.
                m_pDJPlayerState->SetScanningCD(false);
            }

            return 1;
        }

        case EVENT_CONTENT_UPDATE_ERROR:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_CONTENT_UPDATE_ERROR\n");
            m_pPlayManager->HandleEvent(key, data);
            return 1;

        case EVENT_PLAYLIST_LOAD_BEGIN:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_PLAYLIST_LOAD_BEGIN\n");
            m_pUserInterface->NotifyPlaylistLoadBegin((int)data);
            return 1;

        case EVENT_PLAYLIST_LOAD:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_PLAYLIST_LOAD\n");
            m_pUserInterface->NotifyPlaylistLoadProgess((playlist_load_info_t*)data);
            return 1;

        case EVENT_PLAYLIST_LOAD_END:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_PLAYLIST_LOAD_END\n");
            m_pUserInterface->NotifyPlaylistLoadEnd();
            return 1;

        case EVENT_DELETE_CONTENT:
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_DELETE_CONTENT\n");
            m_pUserInterface->NotifyDeleteContent((content_delete_info_t*)data);
            return 1;

        case EVENT_CD_METADATA_MULTIPLE_HITS:
        {
            DEBUGP(EV, DBGLEV_INFO, "Multiple CD Names Found for cd %d:\n", ((cd_multiple_hit_event_t*)data)->uiDiskID);
            // Does this result match the current CD insertion index?
            if (((cd_multiple_hit_event_t*)data)->uiDiskID != m_uiCDInsertionCount)
            {
                // No, it doesn't.  Fuck it.
                DEBUGP(EV, DBGLEV_INFO, "Doesn't match current cd index %d -- ignoring\n", m_uiCDInsertionCount);
                break;
            }

            m_bPlayOnCDInsert = false;
            m_bRecordOnCDInsert = false;
            m_pUserInterface->CDMultipleMetadata(data);
            return 1;
        }

        case EVENT_CD_METADATA_SELECTED:
        {
            if( m_pDJPlayerState->GetUIPlayCDWhenInserted() ) {
                m_pPlayManager->Play();
            }
            return 1;
        }
        
        case EVENT_CD_METADATA_NO_HITS:
        case EVENT_CD_METADATA_NO_SELECTION:
        {
            DEBUGP(EV, DBGLEV_INFO, "No Matches Found for Compact Disk: %d\n", (int)data);

            // Does this result match the current CD insertion index?
            if ((unsigned int)data != m_uiCDInsertionCount)
            {
                // No, it doesn't.  Fuck it.
                DEBUGP(EV, DBGLEV_INFO, "Doesn't match current cd index %d -- ignoring\n", m_uiCDInsertionCount);
                return 1;
            }
            // Have we already found a few local hits for this disc?
            else if ((key == EVENT_CD_METADATA_NO_HITS) && m_bLocalHits)
            {
                // Yep.  Sometimes things just work out.
                DEBUGP(EV, DBGLEV_INFO, "No online hits for cd index, but there are already local hits -- ignoring\n", m_uiCDInsertionCount);
                return 1;
            }

            m_pUserInterface->SetMessage(LS(SID_NO_INFO_FOUND_FOR_THIS_CD));

            if (CCDDataSource* pCDDS = m_pDJPlayerState->GetCDDataSource())
            {
                // Make up metadata using a CD session number generated by the DJ player state.
                int iCDSessionIndex = m_pDJPlayerState->GetNextCDSession();

                CCDDBDiskInfo* pDiskInfo = new CCDDBDiskInfo(pCDDS->GetTOC());

                // Populate disk info based on the session number.
                PopulateNewDiskInfo(pDiskInfo, iCDSessionIndex, pCDDS->GetAudioSessionCount());

                // Convert the metadata into cddb format.
                pDiskInfo->ConvertMetadataIntoInfo();
                // Add the metadata to the content manager.
                SetCDRecordMetadata(m_pContentManager, pCDDS, pDiskInfo);
#ifdef CDDB_CACHE_RESULTS
                // Add the disk info to the local cache.
                CCDDBQueryManager::GetInstance()->AddDiskToCache(pDiskInfo);
                // Don't save the cache if we're playing, since that will cause audio dropouts.
                if (m_pPlayManager->GetPlayState() != CMediaPlayer::PLAYING)
                    CCDDBQueryManager::GetInstance()->SaveCache();
#endif  // CDDB_CACHE_RESULTS
                delete pDiskInfo;

                // If the current playlist entry is from the CD, then update the play screen
                // with the new metadata.
                if (IPlaylistEntry* pEntry = m_pPlayManager->GetPlaylist()->GetCurrentEntry())
                    if (pEntry->GetContentRecord()->GetDataSourceID() == pCDDS->GetInstanceID())
                    {
                        m_pUserInterface->RefreshTrack();
                    }
            }
            if( m_pDJPlayerState->GetUIPlayCDWhenInserted() ) {
                m_pPlayManager->Play();
            }

            return 1;
        }

        case EVENT_CD_METADATA_ONLINE_LOOKUP_ERROR:
        {
            DEBUGP(EV, DBGLEV_INFO, "CDDB online lookup error: %x\n", (int)data);

            // This message is sent when there's an error with online lookup.
            // This is sent in conjunction with EVENT_CD_METADATA_NO_HITS,
            // so DON'T TAKE ANY ACTION ON RECEIVING THIS EVENT EXCEPT TO SHOW AN ERROR MESSAGE.
            m_pUserInterface->SetMessage(LS(SID_CANT_ACCESS_CDDB_ONLINE));

            return 1;
        }

        case EVENT_CDDB_ONLINE_UPDATE_BEGIN:
        {
            DEBUGP(EV, DBGLEV_TRACE, "Event: CDDB online update begin\n");

            return 1;
        }

        case EVENT_CDDB_ONLINE_UPDATE_END:
        {
            DEBUGP(EV, DBGLEV_TRACE, "Event: CDDB online update end\n");

            m_pUserInterface->NotifyCDDBUpdateEnd();
            return 1;
        }

        case EVENT_CDDB_ONLINE_UPDATE_ERROR:
        {
            DEBUGP(EV, DBGLEV_ERROR, "CDDB online update error: %x\n", (int)data);

            m_pUserInterface->NotifyCDDBUpdateError((int)data);
            return 1;
        }

        case EVENT_CDDB_ONLINE_UPDATE_DOWNLOADING:
        {
            DEBUGP(EV, DBGLEV_TRACE, "Event: CDDB online update: downloading file %d of %d (%x)\n",
                ((int)data) & 0xFF, ((int)data) >> 16, (int)data);

            m_pUserInterface->NotifyCDDBUpdateDownloading(((int)data) & 0xFFFF, ((int)data) >> 16);
            return 1;
        }

        case EVENT_CDDB_ONLINE_UPDATE_PROCESSING:
        {
            DEBUGP(EV, DBGLEV_TRACE, "Event: CDDB online update: processing file %d of %d (%x)\n",
                ((int)data) & 0xFF, ((int)data) >> 16, (int)data);

            m_pUserInterface->NotifyCDDBUpdateProcessing(((int)data) & 0xFFFF, ((int)data) >> 16);
            return 1;
        }

		case EVENT_NETWORK_UP:
		{
            // dc- this guard is actually bad now, since we do things other than UPNP in networking...
#ifndef NO_UPNP
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_NETWORK_UP %x\n",(int)data);
			if (!g_NetworkServicesStarted)
			{
                struct in_addr ipaddr;
                char szIPAddress[20];
                    
                GetInterfaceAddresses( "eth0", (unsigned int*)&ipaddr, NULL );
                strcpy(szIPAddress, inet_ntoa(ipaddr));
                    
                //DEBUGP(MAIN, DBGLEV_INFO, "IP address: %s\n", szIPAddress);
                
                // if this fails, it has probably already started
                if (DJUPnPStartup(szIPAddress, 5431))
                {
                    CNetDataSource * pNDS = m_pDJPlayerState->GetNetDataSource();
                    if (pNDS)
                    {
                        CIMLManager::GetInstance()->SetNetDataSource(pNDS);
                    }
                    else
                    {
                        DEBUGP( EV, DBGLEV_ERROR, "ERROR: Could not obtain *CNetDataSource for CIMLManager\n");
                    }

#ifdef USE_UPNP_SERVICES
                    DJStartup();
#endif  // USE_UPNP_SERVICES

#ifdef DDOMOD_MAIN_WEBCONTROL
                    // For consistency, always start the web control, but if the user has it
                    // disabled then stub out the web page
                    StartWebControlServer();
                    if( !m_pDJPlayerState->GetUIEnableWebControl() )
                    {
                        StopWebControlServer();
                    }
#endif // DDOMOD_MAIN_WEBCONTROL

				}
				g_NetworkServicesStarted = 1;
			} else {
                // It's possible we just have an ip change here. let Upnp know
                struct in_addr ipaddr;
                char szIPAddress[20];
                    
                GetInterfaceAddresses( "eth0", (unsigned int*)&ipaddr, NULL );
                strcpy(szIPAddress, inet_ntoa(ipaddr));
                UpnpHostChange( szIPAddress, 5431 );
            }

#ifdef USE_TFTP
            StartTFTPService();
#endif  // USE_TFTP
            m_pUserInterface->NotifyNetworkUp();
			DJUPnPControlPointRefresh(false);


			break;
		}
		case EVENT_NETWORK_DOWN:
		{
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_NETWORK_DOWN\n");

            // If the current track is on the network, we need to stop playback here, so deconfigure
            // This can happen if the DJ is playing from hd/cd and querying an fml when network connectivity is lost.
            if( m_pDJPlayerState->GetCurrentSongSource() == CDJPlayerState::FML ) {
                m_pPlayManager->Deconfigure();
                m_pUserInterface->ClearTrack();
                SynchPlayState();
            }

			CIMLManager* pIMLManager = CIMLManager::GetInstance();

			m_pUserInterface->NotifyNetworkDown();

            m_pPlayManager->GetPlaylist()->DeleteEntriesFromDataSource((int)data);

            // Only change the source if we were on FML
            if( m_pDJPlayerState->GetSource() == CDJPlayerState::FML ) {
                m_pDJPlayerState->SetSource(CDJPlayerState::HD, true, false);
            }

			if (m_pPlayManager->GetPlaylist()->IsEmpty())
                m_pUserInterface->NotifyPlaylistCleared(true);

			while (pIMLManager->GetIMLCount())
			{
				CIML* pIML = pIMLManager->GetIMLByIndex(0);
                char szScratch[64];
                DEBUGP( EV, DBGLEV_INFO, "Removing Fullplay Media Library: %d: %s\n", pIML->GetDeviceNumber(), TcharToCharN(szScratch, pIML->GetFriendlyName(), 63));

				m_pUserInterface->NotifyIMLRemoved(pIML);
				pIMLManager->RemoveIML(pIML);

                // Defer deleting the fml until the UI has a chance to play with it.
                put_event(EVENT_IML_DELETE_ME, (void*)pIML);
            }

#ifdef USE_TFTP
            StopTFTPService();
#endif  // USE_TFTP
            //            DJUPnPShutdown();
            //            g_NetworkServicesStarted = 0;

			break;
		}
#endif  // NO_UPNP

        case EVENT_IML_DELETE_ME:
        {
            delete (CIML*)data;
            return 1;
        }

#ifndef NO_UPNP
        case EVENT_IML_FOUND:
        {
            iml_found_info_t* pIMLInfo = (iml_found_info_t*)data;
            DEBUGP( EV, DBGLEV_INFO, "Found FML %d - %s\n", pIMLInfo->iDeviceNumber, pIMLInfo->szFriendlyName);
            CIML* pNewIML = new CIML(pIMLInfo->iDeviceNumber, pIMLInfo->szMediaBaseURL, pIMLInfo->szFriendlyName, pIMLInfo->szUDN);
            free(pIMLInfo->szMediaBaseURL);
            free(pIMLInfo->szFriendlyName);
            free(pIMLInfo->szUDN);
            delete pIMLInfo;

            // If the new fml's UDN is missing, then ignore the fml -- it's of no use to us.
            if (pNewIML->GetUDN())
            {
                CIMLManager* pIMLManager = CIMLManager::GetInstance();
                pIMLManager->AddIML(pNewIML);

                m_pUserInterface->NotifyIMLFound();
                
                // Print some debugging info.
                DEBUGP( EV, DBGLEV_INFO, "FML count: %d\n", pIMLManager->GetIMLCount());
                for (int i = 0; i < pIMLManager->GetIMLCount(); i++)
                    if (CIML* pIML = pIMLManager->GetIMLByIndex(i))
                    {
                        char szScratch[64];
                        DEBUGP( EV, DBGLEV_INFO, "  FML %d: %s\n", pIML->GetDeviceNumber(), TcharToCharN(szScratch, pIML->GetFriendlyName(), 63));
                    }
            }
            else
            {
                DJUPnPControlPointRemoveDevice(pNewIML->GetDeviceNumber());
                delete pNewIML;
            }
            break;
        }

        case EVENT_IML_CACHING_BEGIN:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_IML_CACHING_BEGIN\n");
            break;
        }

        case EVENT_IML_CACHING_END:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_IML_CACHING_END\n");
            if (CIML* pIML = CIMLManager::GetInstance()->GetIMLByDeviceNumber((int)data))
            {
                CIMLManager::GetInstance()->NotifyIMLCached(pIML);
				m_pUserInterface->NotifyIMLAvailable(pIML);
            }
            break;
        }

        case EVENT_IML_CACHING_ERROR:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_IML_CACHING_ERROR\n");
            CIMLManager* pIMLManager = CIMLManager::GetInstance();
            if (CIML* pIML = pIMLManager->GetIMLByDeviceNumber((int)data))
            {
                TCHAR tszMessage[256];
                tstrncpy(tszMessage, LS(SID_ERROR_QUERYING_MEDIA_LIBRARY_COLON_), 255);
                m_pUserInterface->SetMessage(tstrncat(tszMessage, pIML->GetFriendlyName(), 255));

                // Tell the system the the IML is no longer available.
				m_pUserInterface->NotifyIMLRemoved(pIML);
                // Remove the iml from the manager's list.
                pIMLManager->RemoveIML(pIML);
                // Defer deleting the fml until the UI has a chance to play with it.
                put_event(EVENT_IML_DELETE_ME, (void*)pIML);
            }
            break;
        }

        case EVENT_IML_BYEBYE:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_IML_BYEBYE\n");
            CIMLManager* pIMLManager = CIMLManager::GetInstance();
            if (CIML* pIML = pIMLManager->GetIMLByDeviceNumber((int)data))
            {
                char szScratch[64];
                DEBUGP( EV, DBGLEV_INFO, "Media Library Lost %d - %s\n", pIML->GetDeviceNumber(), TcharToCharN(szScratch, pIML->GetFriendlyName(), 63));

                // If the FML lost was the current FML, then switch the current data source to HD.
                // dc- the logic here is still flawed. if you have a mixed playlist of FML and HD content and the FML goes byebye,
                //  the source shouldn't be switched just because the current track is lost. Instead, the playlist should be cleared
                //  when the FML is removed, and if there are no tracks in the current playlist _then_ we should switch to HD
                // ecm- I think there's some confusion in terms.  The "current FML" refers to the current browse source --
                // i.e., press "Artist" and you'll see artists from that FML.  If this FML goes away then we have to
                // choose another current data source for browsing.  That's either the hard drive, CD, line-in, or some
                // random fml.  The hard drive makes the most sense since we can mix and match content from fmls and the
                // hard drive without clearing the playlist.

                // This gets done as a result of RefreshSource in SourceMenuScreen via NotifyIMLRemoved, so don't do anything about
                // it here.
                //if (m_pDJPlayerState->GetCurrentIML() == pIML && m_pDJPlayerState->GetSource() == CDJPlayerState::FML)
				//    m_pDJPlayerState->SetSource(CDJPlayerState::HD);

				m_pUserInterface->NotifyIMLRemoved(pIML);
                pIMLManager->RemoveIML(pIML);
                // Defer deleting the fml until the UI has a chance to play with it.
                put_event(EVENT_IML_DELETE_ME, (void*)pIML);
            }
            break;
        }

        case QUERY_LIBRARY_RESULT:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received library query result\n");
            iml_query_library_info_t* pQueryInfo = (iml_query_library_info_t*)data;
            CMediaQueryResult* pQR = (CMediaQueryResult*)CQueryResultManager::GetInstance()->GetQueryResultByID(pQueryInfo->iQueryID);
            if (pQR)
            {
                pQR->ProcessQueryResults(pQueryInfo);
            }
            else
            {
                // If a matching query result is not found, then free the memory allocated for the titles.
                for (int i = 0; i < pQueryInfo->pRecords->Size(); ++i)
                {
                    delete [] (*pQueryInfo->pRecords)[i].szMediaTitle;
                    delete [] (*pQueryInfo->pRecords)[i].szArtistName;
                }
                delete pQueryInfo->pRecords;
            }

            delete pQueryInfo;
            break;
        }

        case QUERY_ARTISTS_RESULT:
        case QUERY_ALBUMS_RESULT:
        case QUERY_GENRES_RESULT:
        case QUERY_PLAYLISTS_RESULT:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received general query result\n");
            iml_query_info_t* pQueryInfo = (iml_query_info_t*)data;
            CGeneralQueryResult* pQR = (CGeneralQueryResult*)CQueryResultManager::GetInstance()->GetQueryResultByID(pQueryInfo->iQueryID);
            if (pQR)
            {
                pQR->ProcessQueryResults(pQueryInfo);
            }
            else
            {
                // If a matching query result is not found, then free the memory allocated for the titles.
                for (int i = 0; i < pQueryInfo->pKeyValues->Size(); ++i)
                    delete [] (*pQueryInfo->pKeyValues)[i].szValue;
                delete pQueryInfo->pKeyValues;
            }

            delete pQueryInfo;
            break;
        }

        case QUERY_RADIO_STATIONS_RESULT:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received radio station query result\n");
            iml_query_radio_stations_info_t* pQueryInfo = (iml_query_radio_stations_info_t*)data;
            CRadioStationQueryResult* pQR = (CRadioStationQueryResult*)CQueryResultManager::GetInstance()->GetQueryResultByID(pQueryInfo->iQueryID);
            if (pQR)
            {
                pQR->ProcessQueryResults(pQueryInfo);
            }
            else
            {
                // If a matching query result is not found, then free the memory allocated for the titles.
                for (int i = 0; i < pQueryInfo->pStations->Size(); ++i)
                {
                    delete [] (*pQueryInfo->pStations)[i].szStationName;
                    delete [] (*pQueryInfo->pStations)[i].szURL;
                }
                delete pQueryInfo->pStations;
            }

            delete pQueryInfo;
            break;
        }

#endif  // NO_UPNP

        case EVENT_IDLE_CODING_FINISH:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_IDLE_CODING_FINISH\n");
            idle_coder_finish_t* pEvent = (idle_coder_finish_t*)data;
            CRecordingManager::GetInstance()->NotifyEncodingFinished(pEvent->szInURL, pEvent->szOutURL);
            delete [] pEvent->szInURL;
            delete [] pEvent->szOutURL;
            delete pEvent;
            // Paranoia -- we should always be stopped when idle coding finishes, but routing
            // commit calls through a common function helps us all.
            CommitUpdatesIfSafe();
            // Save the idle coder's new state.
            CIdleCoder::GetInstance()->SaveToRegistry();
            m_pDJPlayerState->SaveRegistry();
            return 1;
        }

        case EVENT_FILE_COPIER_ERROR:
        {
            bool bRipTrack = CRecordingManager::GetInstance()->IsRippingSingle();
            bool bRipping = CRecordingManager::GetInstance()->IsRipping();
            bool bRecording = CRecordingManager::GetInstance()->IsRecording();
            if( bRipping || bRecording ) {
                CRecordingManager::GetInstance()->StopRipping(false);
                CRecordingManager::GetInstance()->StopRecording(false);
                if (bRipping && !bRipTrack)
                    CRecordingManager::GetInstance()->RestorePlayMode();
                m_pPlayManager->Stop();

                // Notify the user of the error
                m_pUserInterface->SetMessage(LS(SID_FAILED_TO_COPY_LAST_TRACK), CSystemMessageString::REALTIME_INFO);
                return 1;
            }
        }
        case EVENT_FILE_COPIER_FINISH:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_FILE_COPIER_FINISH\n");
            // If in ripping mode, then manually advance to the next track.
            bool bRipTrack = CRecordingManager::GetInstance()->IsRippingSingle();
            bool bRipping = CRecordingManager::GetInstance()->IsRipping();
            
            if (key == EVENT_FILE_COPIER_FINISH)
                CRecordingManager::GetInstance()->NotifyCopyingFinished((file_copier_finish_t*)data);

            if (bRipping)
            {
                if (FAILED(DJNextTrack()))
                {
                    // If we've just ripped the entire CD, then eject the CD.
                    if (!bRipTrack &&
                        (m_pDJPlayerState->GetSource() == CDJPlayerState::CD) &&
                        m_pDJPlayerState->GetUIEjectCDAfterRip())
                    {
                        OpenCDTray();
                    }
                    else
                    {
                        // We've reached the end of the playlist in single-track rip mode,
                        // so cycle to the beginning of the playlist.
                        CRecordingManager::GetInstance()->StopRipping();
                        if (bRipping && !bRipTrack)
                            CRecordingManager::GetInstance()->RestorePlayMode();
                        IPlaylist* pCurrentPlaylist = m_pPlayManager->GetPlaylist();
                        if (pCurrentPlaylist)
                        {
                            pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
                            if (FAILED(DJSetCurrentOrNext()))
                            {
                                // We couldn't set any track in the playlist, so tell the user.
                                CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                            }
                        }
                        m_pPlayManager->Stop();
                    }
                }
            }
            return 1;
        }

        case EVENT_DELETE_PARTIAL_RECORDING:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_DELETE_PARTIAL_RECORDING\n");
            CRecordingManager::GetInstance()->DeletePartialRecording((char*)data);
            return 1;
        }

        case EVENT_CLEAR_CD_CACHE:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received clear CD cache request\n");

            // Clear the cache of all disk info.
            CCDDBQueryManager::GetInstance()->ClearCache();

            // Clear the data CD cache, too.
            m_pCDCache->Clear();
            m_pCDCache->Commit();

            return 1;
        }

#ifdef DDOMOD_MAIN_UPNPSERVICES
        case EVENT_CLEAR_CONTENT:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received clear content request\n");

            m_pPlayManager->Deconfigure();
            m_pUserInterface->NotifyPlaylistCleared(true);
            m_pPlayManager->GetPlaylist()->Clear();
            m_pContentManager->Clear();

            return 1;
        }

        case EVENT_REFRESH_CONTENT:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received refresh content request\n");

            m_pPlayManager->RefreshAllContent( );

            return 1;
        }

        case EVENT_RESET_CDDB:
        {
            DEBUGP( EV, DBGLEV_INFO, "Received reset CDDB request\n");
            CCDDBQueryManager::GetInstance()->Reset();

            return 1;
        }

#endif  // DDOMOD_MAIN_UPNPSERVICES

        case EVENT_LINERECORDER_CONTENT_ADDITION:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_LINERECORDER_CONTENT_ADDITION\n");
#ifdef LINE_RECORDER_ENABLED
            // add the new records to the content manager
            CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);
            media_record_info_t* update = (media_record_info_t*)data;
            m_pContentManager->AddMediaRecord( *update );
            free(update->szURL);
            delete update;
            // create a new playlist of the current session and park the current track at the track just recorded
            DEBUGP( EV , DBGLEV_TRACE, "ev:linerec update done\n"); 
            MediaRecordList recs;
            char szAlbum[32];
            TCHAR tszAlbum[32];
            CLineRecorder::GetInstance()->GetSessionName(szAlbum);
            int iAlbum = m_pContentManager->GetAlbumKey(CharToTchar(tszAlbum,szAlbum));
            m_pContentManager->GetMediaRecords(recs,CMK_ALL,iAlbum);
            IPlaylist* pPlaylist = m_pPlayManager->GetPlaylist();
            pPlaylist->Clear();
            pPlaylist->AddEntries(recs);
            // hopefully the last track is consistently the most recently recorded.
            pPlaylist->SetCurrentEntry(pPlaylist->GetEntry(pPlaylist->GetSize()-1));
            // unless we're still busily recording, set the most recent song into the player core.
            if (CPlayerScreen::GetPlayerScreen()->GetEventHandlerMode() != ePSLineRecEvents)
                m_pPlayManager->SetSong(pPlaylist->GetCurrentEntry());
#endif
            return 0;
        }
        case EVENT_SPACE_LOW:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_SPACE_LOW\n");
            if (CLineRecorder::GetInstance()->InSession())
                CPlayerScreen::GetPlayerScreen()->GetLineRecEventHandler()->HandleSpaceLow();
            return 0;
        }
        case EVENT_RECORDING_TIMELIMIT:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_RECORDING_TIMELIMIT\n");
            if( CLineRecorder::GetInstance()->InSession() ) {
                // Normally we could use the CheckSpaceAndTrackLimits() call here, but that's no good because we haven't added the
                //  current track to the content manager. instead, we have to query the content manager and see if it was at LIMIT-1
                //  tracks; if so, we are done
                //                if( CRecordingManager::GetInstance()->CheckSpaceAndTrackLimits() == RECORDING_NO_ERROR ) {
                if( m_pContentManager->GetMediaRecordCount( m_pDJPlayerState->GetFatDataSource()->GetInstanceID() ) < (MAX_HD_TRACKS-1) ) {
                    CPlayerScreen::GetPlayerScreen()->GetLineRecEventHandler()->JumpToNextFile();
                }
                else {
                    CPlayerScreen::GetPlayerScreen()->GetLineRecEventHandler()->StopRecording();
                }
            }
            
            return 0;
        }

        case EVENT_SYSTEM_SHUTDOWN:
        {
            DEBUGP( EV, DBGLEV_TRACE, "Event: EVENT_SYSTEM_SHUTDOWN\n");
#ifdef DDOMOD_DJ_BUFFERING
            // stop accessing files so we don't inadvertantly wake up drives
            CBuffering::GetInstance()->DetachFromPlaylist();
#endif
            // actually shut down the system/drives/etc
            m_pDJPlayerState->ShutdownSystem();
            break;
        }
#ifdef ENABLE_EXTERN_CONTROL
		case EVENT_CONTROL_SERVICE_REQUEST: 
		{
			ControlServiceCallback((int)data);
			return 0;
		}
#endif //ENABLE_EXTERN_CONTROL

#ifdef DDOMOD_MAIN_WEBCONTROL
		case EVENT_WEBCONTROL_SERVICE_REQUEST:
		{
			WebControlCallback((int)data);
			return 0;
		}
#endif //DDOMOD_MAIN_WEBCONTROL
        default:
            m_pPlayManager->HandleEvent(key, data);
            return -1;
    };

    m_pPlayManager->HandleEvent(key, data);
    return -1;
}
