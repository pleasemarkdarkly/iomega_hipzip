// main.cpp: bring up main player
// danb@iobjects.com 11/09/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <_modules.h>
#include <_version.h>

// Interfaces to classes we need to set up
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <main/main/DJPlayerState.h>
#include <util/eventq/EventQueueAPI.h>
#ifdef __DJ
#include <devs/lcd/lcd.h>
#endif  // __DJ

#ifndef DISABLE_VOLUME_CONTROL
#include <io/audio/VolumeControl.h>
#endif // DISABLE_VOLUME_CONTROL

#ifndef NO_UPNP
#include <main/iml/manager/IMLManager.h>
#include <main/djupnp/DJUPnP.h>
#include <network.h>
#include <io/net/Net.h>
#endif  // NO_UPNP

#ifdef DDOMOD_EXTRAS_CDDB
#include <extras/cddb/gn_fs.h>
#include <main/cddb/CDDBHelper.h>
#endif  // DDOMOD_EXTRAS_CDDB

#include <main/main/DJEvents.h>
#include <main/main/DJHelper.h>
#include <main/main/RestoreEvents.h>
#include <main/main/FatHelper.h>    // VerifyOrCreateDirectory
#include <main/main/LEDStateManager.h>
#include <main/testharness/testharness.h>
#include <main/main/ProgressWatcher.h>
#include <util/debug/debug.h>
#include <util/tchar/tchar.h>
#include <util/diag/diag.h>
#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif
#include "CDDirGen.h"

#include <main/ui/RestoreScreen.h>
#include <main/ui/SystemToolsMenuScreen.h>

// Interfaces to components we instantiate
#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#define USE_CD
#endif

#ifdef DDOMOD_DATASOURCE_FAT
#include <datasource/fatdatasource/FatDataSource.h>
#define USE_HD
#endif

#ifdef DDOMOD_DATASOURCE_LINEIN
#include <datasource/lineindatasource/LineInDataSource.h>
#define USE_LINEIN
#endif

#ifdef DDOMOD_DATASOURCE_NET
#include <datasource/netdatasource/NetDataSource.h>
#define USE_NET
#endif

#include <main/playlist/djplaylist/DJPlaylist.h>

#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/content/simplercontentmanager/SimplerContentManager.h>

#ifdef DDOMOD_DEV_KEYBOARD
#include <devs/keyboard/Keyboard.h>
#define USE_KEYBOARD
#endif
#ifdef DDOMOD_DEV_IR_UEI
#include <devs/ir/IR_UEI.h>
#define USE_IR
#endif

// teardown headers
#include <extras/idlecoder/IdleCoder.h>
#include <extras/filecopier/FileCopier.h>
#include <util/timer/Timer.h>
#include <main/iml/manager/IMLManager.h>
#include <main/iml/query/QueryResultManager.h>
#include <main/main/LineRecorder.h>
#include <main/main/Recording.h>
#include <main/main/UpdateManager.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <main/util/update/UpdateApp.h>
#include <main/ui/MainMenuScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/PlaylistSaveScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/SplashScreen.h>
#include <main/ui/YesNoScreen.h>
#include <main/ui/StaticSettingsMenuScreen.h>
#include <main/ui/CDTriageScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/InfoMenuScreen.h>
#include <main/ui/EditScreen.h>
#include <main/ui/EditIPScreen.h>
#include <codec/codecmanager/CodecManager.h>
#include <content/metadatatable/MetaDataTable.h>

//#define ENABLE_PARTIAL_BOOT
#define ENABLE_RESTORE_KEY

#if defined(ENABLE_RESTORE_KEY) || defined(ENABLE_PARTIAL_BOOT)

#include <devs/keyboard/kbd_scan.h>

#if defined(ENABLE_PARTIAL_BOOT) && !defined(__DJ)
#warning "Partial booting only supported on DJ"
#undef ENABLE_PARTIAL_BOOT
#endif

#if defined(ENABLE_RESTORE_KEY)

// Key combinations are defined in the *-keymaps.h files
#if !defined(__DJ) && !defined(__DHARMA_V2)
#undef ENABLE_RESTORE_KEY
#warning "Restore UI only supported on DJ and Dharma 2"
#endif

extern void RestoreDJ();

#endif // ENABLE_RESTORE_KEY


#endif // ENABLE_RESTORE_KEY || ENABLE_PARTIAL_BOOT


#ifdef DDOMOD_GUI_PEG
#include <main/ui/UI.h>
#include <main/ui/PEGUserInterface.h>
#include <main/ui/PEGRestoreUserInterface.h>
#include <main/ui/Keys.h>

void PegAppInitialize(PegPresentationManager *pPresentation);
void PegAppUninitialize(PegPresentationManager *pPresentation);
void PegRestoreAppInitialize(PegPresentationManager *pPresentation, bool bUseSplashScreen);
#endif // DDOMOD_GUI_PEG

#ifdef DDOMOD_MAIN_UPNPSERVICES
#include <main/upnpservices/djservices.h>
#define USE_UPNP_SERVICES
#endif // DDOMOD_MAIN_UPNPSERVICES

#ifdef ENABLE_SERIAL_CONTROL
#include <main/serialcontrol/serialcontrol.h>
#endif // ENABLE_SERIAL_CONTROL


DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);  // debugging prefix : (25) mn

// Various prototypes
IInputStream* CreateBufferInStream( IMediaContentRecord* mcr );
void StartRestoreUI(PegPresentationManager& present);
void StartRecoveryUI(PegPresentationManager& present);
void NotifyUserHDDFail();

// Local prototypes
void main_event_loop();

extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};

// Global current event handler
CEvents* g_pEvents;

#ifdef USE_KEYBOARD
#if defined(__DJ)
#include "dj-keymaps.h"

#elif defined(__DAR)
#include "dar-keymaps.h"

#elif defined(__DHARMA_V2)
#include "dharmav2-keymaps.h"

#elif defined(__DHARMA)
#include "dharma-keymaps.h"

#else
#err "Unknown keyboard configuration"
#endif
#endif // USE_KEYBOARD

#ifdef USE_IR
#include "irmaps.h"
#endif


// Thread data
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

void cyg_user_start( void ) 
{
    DBEN( MAIN );
    
    cyg_thread_create( UI_THREAD_NORMAL_PRIORITY, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
    
    DBEX( MAIN );
}

// Get/Set main thread priority level
void SetMainThreadPriority(int nPrio)
{
    cyg_thread_set_priority( threadh[0], nPrio );
}
int GetMainThreadPriority()
{
    return cyg_thread_get_priority( threadh[0] );
}

#include <cyg/kernel/thread.hxx>        // Cyg_Thread
#include <cyg/kernel/thread.inl>

static const char* ExceptionStrings[] =
{
    "reset vector (shouldn't happen)",
    "undefined instruction",
    "software interrupt",
    "prefetch abort (failed to load instruction)",
    "data abort (failed to load data)",
    "reserved vector (shouldn't happen)",
    "irq (shouldn't happen)",
    "fiq (shouldn't happen)",
};
static void DJExceptionHandler(CYG_ADDRWORD data, cyg_code number, CYG_ADDRWORD info)
{
	DEBUGP(MAIN, DBGLEV_FATAL, "Exception %d [%s] in thread %s\n", number, ExceptionStrings[number],Cyg_Thread::self()->get_name());

    // we should probably disable interrupts here
    cyg_scheduler_lock();
    //    print_thread_states();
#ifdef ENABLE_LEAK_TRACING
    diag_printf("dumping leak trace\n");
    dump_leak_trace();
#endif

    // Call does not return
    CDJPlayerState::GetInstance()->SafeReset();
}

void thread_entry( cyg_uint32 data ) 
{
#if DEBUG_LEVEL != 0
    print_mem_usage();
#endif
    DBEN( MAIN );

    // Register the exception handler
    cyg_exception_handler *old_handler;
    CYG_ADDRWORD old_data;

    Cyg_Thread::self()->register_exception(
        CYGNUM_HAL_EXCEPTION_MAX, 
        &DJExceptionHandler,
        (CYG_ADDRWORD)0,
        &old_handler,
        &old_data);

#if defined (DDO_VERSION_STR)
    // print the build version number
    DEBUGP( MAIN, DBGLEV_INFO, "   **** Build: ");
    DEBUGP( MAIN, DBGLEV_INFO, DDO_VERSION_STR);
    DEBUGP( MAIN, DBGLEV_INFO, " ****\n");
#endif // DDO_VERSION_STR

#if defined(ENABLE_RESTORE_KEY) || defined(ENABLE_PARTIAL_BOOT)
    // Scan the keyboard
	kbd_scan();
#endif
    
#if defined(ENABLE_PARTIAL_BOOT)
    if( (__kbd_col_state[POWER_ROW] & POWER_COMBO) != POWER_COMBO ) {
        CDJPlayerState::SetDrivesPower(false);

        do {
            // Until we get the power button, wait 50ms and rescan the keyboard
            // Waiting too long here will cause us to miss keystrokes
            cyg_thread_delay(5);
            kbd_scan();
        } while( (__kbd_col_state[POWER_ROW] & POWER_COMBO) != POWER_COMBO );
    }
#endif
    
#if defined(ENABLE_RESTORE_KEY)

    // Check for restore sequence
    bool bRestoreDJ = false;
	if( (__kbd_col_state[RESTORE_ROW] & RESTORE_COMBO) == RESTORE_COMBO )
	    bRestoreDJ = true;
#endif // ENABLE_RESTORE_KEY

	bool bForceEject = false;
	
	if(__kbd_col_state[EJECT_ROW] == EJECT_COMBO)
	    bForceEject = true;


    // turn lcd on to give the user idea that the device is booting
//    SetLEDState(POWERING_ON, true);

    // Initialize PEG
    DEBUGP(MAIN, DBGLEV_INFO, "Initializing PEG\n");
    PegScreen *pScreen = CreatePegScreen();
	PegThing::SetScreenPtr(pScreen);
	
	PegMessageQueue *pMsgQueue = new PegMessageQueue();
	PegThing::SetMessageQueuePtr(pMsgQueue);

   	PegRect rect;
	rect.Set(0, 0, UI_X_SIZE - 1, UI_Y_SIZE - 1);
    PegPresentationManager present(rect);
	PegThing::SetPresentationManagerPtr(&present);
	pScreen->GenerateViewportList(&present);

#ifdef ENABLE_RESTORE_KEY
    // special restore UI
    if(bRestoreDJ)
        PegRestoreAppInitialize(&present, true);
    else
#endif // ENABLE_RESTORE_KEY
        PegAppInitialize(&present);

	pScreen->Invalidate(present.mReal);
	PegMessage NewMessage(PM_DRAW);
	NewMessage.pTarget = &present;
	pMsgQueue->Push(NewMessage);

#ifdef __DJ
    // turn the backlight on
    LCDSetBacklight(LCD_BACKLIGHT_ON);
#endif  // __DJ

#ifdef USE_KEYBOARD
    DEBUGP(MAIN, DBGLEV_INFO, "Initializing Keyboard\n");
    CKeyboard* pK = CKeyboard::GetInstance();
	pK->LockKeyboard();
    pK->SetKeymap( &key_map );
	
#endif // USE_KEYBOARD

#ifdef USE_IR
    DEBUGP(MAIN, DBGLEV_INFO, "Initializing IR\n");
    CIR* pIR = CIR::GetInstance();
    pIR->SetIRMap( &ir_map );
#endif // USE_IR

#ifdef DDOMOD_EXTRAS_CDDB
    // Set the CDDB root directory.
    gnfs_set_root_directory(CDDB_FOLDER);
#endif  // DDOMOD_EXTRAS_CDDB

#ifdef ENABLE_RESTORE_KEY
    // special restore UI
    if(bRestoreDJ)
    {
        StartRestoreUI(present);
        return;
    }
#endif // ENABLE_RESTORE_KEY
    
    // Bring the debug system up as quick as possible; we want its assert handler
    //  online.
    CDebugRouter::GetInstance();

#ifndef DISABLE_VOLUME_CONTROL
    // Initialize volume control & force a custom volume map
    static int s_aryVolume[21] = {
        -96,
        -42, -39, -36,  -33, -30,
        -27, -24, -21,  -18, -15,
        -13, -11,  -9,   -7,  -5,
        -3,  -1,   1,    3,	 5 };
    CVolumeControl::GetInstance()->SetVolumeRange(21, s_aryVolume);
    // Turn the volume up to a default volume
    CVolumeControl::GetInstance()->SetVolume(16);
#endif // DISABLE_VOLUME_CONTROL

    DEBUGP(MAIN, DBGLEV_INFO, "Initializing PlayManager\n");
    CPlayManager* pPlayManager = CPlayManager::GetInstance();

#ifdef USE_HD
    DEBUGP(MAIN, DBGLEV_INFO, "Opening HD\n");
    CFatDataSource* pFatDS = 0;
    if ( (pFatDS = CFatDataSource::Open(0, false)) )
    {
        pFatDS->SetDefaultRefreshMode(IDataSource::DSR_TWO_PASS);
#ifdef MAX_HD_TRACKS
        pFatDS->SetDefaultMaximumTrackCount(MAX_HD_TRACKS);
#endif  // MAX_HD_TRACKS
        pFatDS->SetDefaultUpdateChunkSize(100);
        DEBUGP(MAIN, DBGLEV_INFO, "Adding HD as data source\n");
        pPlayManager->AddDataSource( pFatDS );
        DEBUGP(MAIN, DBGLEV_INFO, "Opened HD\n");

        CDJPlayerState::GetInstance()->SetFatDataSource(pFatDS);

        pFatDS->SetContentRootDirectory(CONTENT_FOLDER);

        bool bContentFolderExists = VerifyOrCreateDirectory(CONTENT_FOLDER);
        // Assert on this, because if it fails nothing good will ever come.
        DBASSERT(MAIN, bContentFolderExists, "Failed to create content folder %s\n", CONTENT_FOLDER);
    }
    else
    {
        DEBUGP(MAIN, DBGLEV_ERROR, "Failed to open HD\n");
        NotifyUserHDDFail();
    }

    CProgressWatcher* pWatch = CProgressWatcher::GetInstance();
    bool bProgressExists = FileExists(SYSTEM_PROGRESS_PATH);
    pWatch->Load();
    eErrorResponse eResponse = pWatch->AnalyzeShutdown();
    pWatch->SetBooting(true);
    pWatch->Save();
    bool bRefreshNeeded = false;
    switch (eResponse)
    {
        case ER_NO_RESPONSE:
            DEBUGP( MAIN, DBGLEV_TRACE, "mn:shutdown ok, continuing normally\n"); 
            break;
        case ER_CLEAR_PLAYLIST:
            DEBUGP( MAIN, DBGLEV_INFO, "mn:deleting current playlist\n"); 
            if (!pc_unlink(CURRENT_PLAYLIST_URL))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del crnt playlist %s\n",CURRENT_PLAYLIST_URL); 
            }
            break;
        case ER_DELETE_CD_METADATA_CACHE:
            if (!pc_unlink(DISK_METADATA_CACHE_PATH))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del cd metadata persist file\n"); 
            }
            break;
        case ER_REBUILD_CD_DIR_CACHE:
        {
            if (!pc_unlink(CD_DIR_NAME_DB_FILE))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del cd dir cache persist file\n"); 
            }
            // Rebuild the directory name cache from the TOC files stored on disk.
            pWatch->SetTask(TASK_REBUILDING_CD_DIR_CACHE);

            CCDDirGen* pCDDirGen = new CCDDirGen(CD_DIR_NAME_DB_FILE, CD_DIR_NAME_BASE);
            pCDDirGen->Rebuild();
            delete pCDDirGen;

            pWatch->UnsetTask(TASK_REBUILDING_CD_DIR_CACHE);
            break;
        }
        case ER_DELETE_CD_DIR_CACHE:
        {
            if (!pc_unlink(CD_DIR_NAME_DB_FILE))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del cd dir cache persist file\n"); 
            }
            // Force a chkdsk.
            CRestoreScreen* pRS = CRestoreScreen::GetInstance();
            present.Add(pRS);
			present.MoveFocusTree(pRS);
			pRS->DoChkdsk();
            present.Remove(pRS);
            break;
        }
        case ER_DELETE_SETTINGS:
            if (!pc_unlink(SAVE_SETTINGS_PATH))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del save settings file\n"); 
            }
        case ER_REFRESH_CONTENT:
            if (!pc_unlink(METAKIT_HD_PERSIST_FILE))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del hd content db file\n"); 
            }
            bRefreshNeeded = true;
            pWatch->SetBootFails(3);
            pWatch->Save();
            break;
        case ER_SCANDISK:
        {
            DEBUGP( MAIN, DBGLEV_INFO, "mn:shutdown recovery system initiating scandisk\n"); 
            pWatch->SetBootFails(4);
            pWatch->Save();
            CRestoreScreen* pRS = CRestoreScreen::GetInstance();
            present.Add(pRS);
			present.MoveFocusTree(pRS);
			pRS->DoChkdsk();
            present.Remove(pRS);
            if (!pc_unlink(METAKIT_HD_PERSIST_FILE))
            {
                DEBUGP( MAIN, DBGLEV_WARNING, "mn:couldn't del hd content db file\n"); 
            }
            bRefreshNeeded = true;
            break;
        }
        case ER_REFORMAT_HDD:
        case ER_NEW_HDD:
            // back up the failure count to allow a clean boot attempt after whatever occurs in the restore app.
            pWatch->SetBootFails(3);
            pWatch->Save();
            // hand this over to the restore UI
            PegAppUninitialize(&present);
            PegRestoreAppInitialize(&present, false);
            StartRecoveryUI(present);
            return;
        default:
            DEBUGP( MAIN, DBGLEV_WARNING, "mn:unexpected watcher error response %d\n",eResponse); 
    }
    pWatch->ClearTasks();

#ifdef DDOMOD_EXTRAS_CDDB
    // Initialize CDDB.
    if (!pc_isdir(CDDB_FOLDER))
    {
        DEBUGP(MAIN, DBGLEV_ERROR, "Missing CDDB directory: %s\n", CDDB_FOLDER);
    }

    if (CCDDBQueryManager::GetInstance()->InitializeCDDB() != SUCCESS)
    {
        CRestoreScreen* pRS = CRestoreScreen::GetInstance();
        present.Add(pRS);
		present.MoveFocusTree(pRS);
        pRS->DoChkdsk();

        if (CCDDBQueryManager::GetInstance()->InitializeCDDB() != SUCCESS)
        {
            pRS->DoRestoreCDDB();

            // hand this over to the restore UI
            PegAppUninitialize(&present);
            PegRestoreAppInitialize(&present, false);
            present.Add(pRS);
            StartRecoveryUI(present);
        }
        present.Remove(pRS);
    }
    CCDDBQueryManager::GetInstance()->InitializeCDDB();

#endif  // DDOMOD_EXTRAS_CDDB

    // Create the peg UI early and register it with the DJPS early; this allows us
    //  to keep the DS in sync
    DEBUGP(MAIN, DBGLEV_INFO, "Create peg ui\n");
    IUserInterface* pUserInterface = new CPEGUserInterface;
    CDJPlayerState::GetInstance()->SetUserInterface( pUserInterface );

    DEBUGP(MAIN, DBGLEV_INFO, "Load saved settings\n");
    CDJPlayerState::GetInstance()->LoadState();
    
	if(!pc_isdir(SYSTEM_FOLDER))
	{
        if (!pc_mkdir(SYSTEM_FOLDER))
            DEBUGP(MAIN, DBGLEV_WARNING, "Unable to create system directory %s\n", SYSTEM_FOLDER);
    }

#endif  // USE_HD

#ifdef DDOMOD_DJ_BUFFERING
    // Get buffering to allocate the memory it needs now.
    CBuffering::GetInstance();
#endif

#ifdef USE_CD
    DEBUGP(MAIN, DBGLEV_INFO, "Opening CD\n");
    CCDDataSource* pCDDS;
    bool bRefreshCDDS = false;
	
	// do not open with media status timer, it will get stuck
    if ( (pCDDS = CCDDataSource::Open("/dev/cda/", "/",false)) )
    {
        // Set the maximum file length for ISO files, so we don't try to copy something
        // that will overwhelm the fat system.
        // EMAXPATH is 128, but we can handle full paths of up to 256 characters.
        pCDDS->SetMaxFilenameLength(256 - 11 /* a:/c/cd0000 */);

        pPlayManager->AddDataSource( pCDDS );
        DEBUGP(MAIN, DBGLEV_INFO, "Opened CD\n");

        CDJPlayerState::GetInstance()->SetCDDataSource(pCDDS);

        pCDDS->SetDefaultRefreshMode(IDataSource::DSR_TWO_PASS);
#ifdef MAX_CD_TRACKS
        pCDDS->SetDefaultMaximumTrackCount(MAX_CD_TRACKS);
#endif  // MAX_CD_TRACKS
#ifdef MAX_CD_TRACKS
        pCDDS->SetDefaultMaximumDirectoryCount(MAX_CD_DIRECTORIES);
#endif  // MAX_CD_TRACKS
  
		if(bForceEject)
		{
			DEBUGP(MAIN, DBGLEV_ERROR, "CD Eject Forced\n");
			pCDDS->OpenTray();
		}
		else
		{
			// do not poll media status

		    Cyg_ErrNo err = pCDDS->GetMediaStatus(false);
		    DEBUGP(MAIN, DBGLEV_INFO, "CD Media Status %d\n", err);
	

			if (err == -ENOMED)
			{
				DEBUGP(MAIN, DBGLEV_ERROR, "No CD in drive\n");
				pCDDS->CloseTray();
			}
			else if (err == ENOERR)
				bRefreshCDDS = true;
		}

		// defer start of media status
		pCDDS->ResumeMediaStatusTimer();
    }
    else
        DEBUGP(MAIN, DBGLEV_ERROR, "Failed to open CD\n");

#endif  // USE_CD

#ifdef USE_LINEIN
    DEBUGP(MAIN, DBGLEV_INFO, "Opening line in data source\n");
	CLineInDataSource* pLineInDS;
    if (( pLineInDS = new CLineInDataSource ))
    {
        pPlayManager->AddDataSource( pLineInDS );

        DEBUGP(MAIN, DBGLEV_INFO, "Opened line in data source\n");
    }
    else
        DEBUGP(MAIN, DBGLEV_ERROR, "Failed to open line in data source\n");
#endif  // USE_LINEIN

#ifdef USE_NET
    DEBUGP(MAIN, DBGLEV_INFO, "Opening net data source\n");
	CNetDataSource* pNetDS;
    if ( (pNetDS = CNetDataSource::Open()) )
    {
        pPlayManager->AddDataSource( pNetDS );

        CDJPlayerState::GetInstance()->SetNetDataSource(pNetDS);

        DEBUGP(MAIN, DBGLEV_INFO, "Opened net data source\n");
    }
    else
        DEBUGP(MAIN, DBGLEV_ERROR, "Failed to open net data source\n");
#endif  // USE_NET

    // Synch the UI and state settings
    CDJPlayerState::GetInstance()->Initialize();
    
    bool bHDPersistExists = FileExists(METAKIT_HD_PERSIST_FILE);

    CDJContentManager* pCM = new CDJContentManager;
    pCM->AddDataSource(pCDDS->GetInstanceID());
    pWatch->SetTask(TASK_LOADING_METAKIT);
    pCM->AddDataSource(pFatDS->GetInstanceID(), METAKIT_HD_PERSIST_FILE);
    pWatch->UnsetTask(TASK_LOADING_METAKIT);
    pCM->AddDataSource(pNetDS->GetInstanceID());
#ifdef USE_LINEIN
    pCM->AddDataSource(pLineInDS->GetInstanceID());
#endif  // USE_LINEIN
    pCM->SetAutoCommitCount(200);   // The number of records that can be added before a db commit is forced.

    CIMLManager::GetInstance()->SetContentManager(CSimplerContentManager::GetInstance());
    CSimplerContentManager::GetInstance()->SetDJContentManager(pCM);

#ifdef USE_CD
    if (pCDDS)
        pCM->DeleteRecordsFromDataSource(pCDDS->GetInstanceID());
#endif  // USE_CD


#ifdef USE_LINEIN
	if( pLineInDS ) {
        pLineInDS->GenerateEntry( pCM, "Line-In" );
	}
#endif

    DEBUGP(MAIN, DBGLEV_INFO, "Refresh Content\n");
    if ((!bHDPersistExists && bProgressExists) || bRefreshNeeded)
    {
        DEBUGP(MAIN, DBGLEV_INFO, "Refreshing HD content\n");
        CDJPlayerState::GetInstance()->SetScanningHD(true);
        pWatch->SetTask(TASK_REFRESHING_CONTENT);
        pPlayManager->RefreshContent( pFatDS->GetInstanceID() );
        bRefreshNeeded = true;
    }

    // Always refresh the CD content on startup.
    unsigned short usCDScanID = 999;
    if (bRefreshCDDS && pCDDS)
    {
        DEBUGP(MAIN, DBGLEV_INFO, "Refreshing CD content\n");
        usCDScanID = pPlayManager->RefreshContent( pCDDS->GetInstanceID() );
    }

    DEBUGP(MAIN, DBGLEV_INFO, "Create content manager\n");
    pPlayManager->SetContentManager( pCM );
    DEBUGP(MAIN, DBGLEV_INFO, "Create playlist\n");

    pPlayManager->SetPlaylist( new CDJPlaylist("") );

    // configure the media player to load the codecs into sram
    //CMediaPlayer::GetInstance()->SetCodecPool( (unsigned int *)CODEC_SRAM_POOL_ADDRESS, CODEC_SRAM_POOL_SIZE );

#ifdef DDOMOD_DJ_BUFFERING
    // config mediaplayer to use buffering.
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif

    // set the src blending threshold
    if (!SUCCEEDED(CMediaPlayer::GetInstance()->SetSRCBlending( SRC_BLENDING_THRESHOLD ))) {
        DEBUGP( MAIN, DBGLEV_INFO, "SRC blending threshold %d rejected\n",SRC_BLENDING_THRESHOLD); 
    }

    // Create the event handler and associate the UI with it
    DEBUGP(MAIN, DBGLEV_INFO, "Create event queue\n");
    g_pEvents = new CDJEvents;
    g_pEvents->SetUserInterface( pUserInterface );

    // Set the CD content scan ID so the initial content scan won't be rejected.
    if (usCDScanID != 999)
        ((CDJEvents*)g_pEvents)->SetCDScanID( usCDScanID );

#ifdef USE_UPNP_SERVICES
	UPnPSetUserInterface(pUserInterface);
#endif  // USE_UPNP_SERVICES

#ifdef ALWAYS_LOG_DEBUG
	CDebugRouter::GetInstance()->CreateLogFile();

    // reprint some important info for the log files
    {
		DEBUGP( MAIN, DBGLEV_INFO, "Log File started during boot\n");
        DEBUGP( MAIN, DBGLEV_INFO, "   ****\n");
        char szTempBuild[128];
           strcpy(szTempBuild, "  Build: ");
#if defined (DDO_VERSION_STR)
        strcat(szTempBuild, DDO_VERSION_STR);
#endif // DDO_VERSION_STR
        DEBUGP( MAIN, DBGLEV_INFO, "%s\n", szTempBuild);

        // grab the ip and the mac
        struct in_addr ip;
        char mac[6];
        GetInterfaceAddresses( "eth0", (unsigned int*)&ip, mac );
        DEBUGP( MAIN, DBGLEV_INFO, "     IP: %s\n", inet_ntoa(ip) );
        DEBUGP( MAIN, DBGLEV_INFO, "    MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[0],mac[1],mac[2],mac[3],mac[4],mac[5] );
        DEBUGP( MAIN, DBGLEV_INFO, "   ****\n");
    }
#endif

    // Restore the previous playlist and data source.
    pWatch->SetTask(TASK_RESTORING_PLAYLIST);
    
    if (bRefreshCDDS && CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
    {
        // The last source was the CD and there's a CD in the drive, so tell the 
        // event manager to load the CD playlist after the initial scan is complete.
        DEBUG(MAIN, DBGLEV_INFO, "Restoring CD playlist\n");
        ((CDJEvents*)g_pEvents)->SetRestoreCDPlaylist();
    }
    else
    {
        // If the last source wasn't CD, then default to HD.
        DEBUG(MAIN, DBGLEV_INFO, "Using default HD playlist\n");
        CDJPlayerState::GetInstance()->SetSource(CDJPlayerState::HD);
    }

    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::HD)
    {
        // If the last source was HD, then restore the HD playlist.
        DEBUG(MAIN, DBGLEV_INFO, "Restoring HD playlist\n");
        bool bHDPlaylistRestored = false;
        if (SUCCEEDED(CDJPlayerState::GetInstance()->LoadCurrentPlaylist()))
        {
            IPlaylistEntry* pCurrentEntry = pPlayManager->GetPlaylist()->GetCurrentEntry();
            if (pCurrentEntry)
            {
                if (SUCCEEDED(DJSetCurrentOrNext(true)))
                    bHDPlaylistRestored = true;
                else
                    DEBUG(MAIN, DBGLEV_ERROR, "Error restoring HD playlist\n");
            }
        }
        if (bHDPlaylistRestored)
        {
            if (bRefreshCDDS)
                ((CDJEvents*)g_pEvents)->SetSwitchSourceOnCDInsertion(false);
        }
        else
            CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
    }
    pWatch->UnsetTask(TASK_RESTORING_PLAYLIST);

    DEBUGP(MAIN, DBGLEV_INFO, "Refresh interface\n");
    g_pEvents->RefreshInterface();

    // turn lcd off since we're basically done booting
//    SetLEDState(POWERING_ON, false);

    pWatch->SetBooting(false);
    pWatch->SetBootFails(0);
    pWatch->Save();
	pK->UnlockKeyboard();

#ifdef ENABLE_SERIAL_CONTROL
	StartSerialControl();
#endif

#ifdef DDOMOD_EXTRAS_CDDB
    // Do a fake CDDB online lookup to force initialization.
    DiskInfoVector svDisks;
    cdda_toc_t toc;
    toc.entries = 0;
    toc.entry_list = 0;
    CDDBGetDiskInfoOnline(&toc, svDisks);
    ClearDiskList(svDisks);
#endif  // DDOMOD_EXTRAS_CDDB

    // Get rid of the splash screen
    CSplashScreen::GetSplashScreen()->HideScreen();
    
    // If we're rescanning the HD then show the progress screen.
    if (bRefreshNeeded)
    {
        CSystemToolsMenuScreen* pSTMS = (CSystemToolsMenuScreen*)CSystemToolsMenuScreen::GetSystemToolsMenuScreen();
        CPlayerScreen::GetPlayerScreen()->Add(pSTMS);
        CPlayerScreen::GetPlayerScreen()->Presentation()->MoveFocusTree(pSTMS);
        pSTMS->StartHDScan(true);
    }

    // This should be the last thing before starting the presentation
#ifdef DDOMOD_MAIN_TESTHARNESS
	DEBUGP(MAIN, DBGLEV_INFO, "Check for Test File\n");
	CTestStimulator::GetInstance()->RunStartupScript();
#endif

    // Tell the event queue that we are the one picking off the events
    CEventQueue::GetInstance()->SetReader(cyg_thread_self());
    
    DEBUGP(MAIN, DBGLEV_INFO, "Start presentation\n");
   	present.Execute();

    // If code gets to this location, it's because a PM_EXIT was received by peg, and the above
    //  routine (which normally runs forever) returned. Treat this as a teardown situation and
    //  try to mop up memory usage.
    DEBUGP(MAIN, DBGLEV_WARNING, "Presentation exited, starting teardown\n");
#if DEBUG_LEVEL != 0
    print_mem_usage();

    // kill all the ui objects, hopefully
    CMainMenuScreen::Destroy();
    CPlayerScreen::Destroy();
    CAlertScreen::Destroy();
    CStaticSettingsMenuScreen::Destroy();
    CPlaylistSaveScreen::Destroy();
    CEditScreen::Destroy();
    CEditIPScreen::Destroy();
    CInfoMenuScreen::Destroy();
    CYesNoScreen::Destroy();
    CSplashScreen::Destroy();
    CCDTriageScreen::Destroy();
    CQuickBrowseMenuScreen::Destroy();

    CUpdateApp::Destroy();
    CUpdateManager::Destroy();
    CProgressWatcher::Destroy();
    ShutdownLED();

    CIMLManager::Destroy();
    CQueryResultManager::Destroy();

    // These need to be cleaned up before the fat data source deinits the HD.
    delete pCM;
    CRecordingManager::Destroy();   // deletes the CD directory name generator cache, which closes a file handle
    delete g_pEvents;   // deletes the data CD cache, which closes a file handle

    // this is probably very order dependent
    CDJPlayerState* pDJPS = CDJPlayerState::GetInstance();
    delete pDJPS->GetCDDataSource();
    delete pDJPS->GetFatDataSource();
    delete pDJPS->GetNetDataSource();
    UpnpFinish();
    DJUPnPShutdown();

    // shut down testharness code
    CTestStimulator::Destroy();
    CDeviceState::Destroy();
    CEventRecorder::Destroy();
    CDebugRouter::Destroy();
    
    CCDDBQueryManager::Destroy();
    CIdleCoder::Destroy();
    CFileCopier::Destroy();
    CLineRecorder::Destroy();

    CIMLManager::Destroy();
    CDataSourceManager::Destroy();
    CSimplerContentManager::Destroy();

    CMetadataTable::Destroy();
    // below needs a Destroy() routine since the destructor is fucking private.
    //    delete CCodecManager::GetInstance();
    
    pDJPS->Destroy();
    delete pUserInterface;

    delete pLineInDS;

    // junk the peg message queue
    delete pMsgQueue;
    
    pPlayManager->Destroy();
    CMediaPlayer::GetInstance()->Destroy();
#ifdef DDOMOD_DJ_BUFFERING
    CBuffering::Destroy();
#endif
    CEventQueue::Destroy();

#ifdef USE_KEYBOARD
    CKeyboard::Destroy();
#endif
#ifdef USE_IR
    CIR::Destroy();
#endif
    
    shutdown_timers();
    
    // now that we have torn everything down, print mem usage and thread states
    print_mem_usage();
    print_thread_states();
#if defined(ENABLE_LEAK_TRACING)
    // if leak tracing is enabled then dump some stats
    dump_leak_trace();
#endif // ENABLE_LEAK_TRACING

    // technically after a proper teardown we could recurse into thread_entry to bring the system
    // back up, but it's better to not try that...
    
#endif // DEBUG_LEVEL != 0
}


void PegIdleFunction()
{
    unsigned int key;
    void* data;
    PegThing* pt = 0;
    CEventQueue* pEventQueue = CEventQueue::GetInstance();
    if (pEventQueue->TimedGetEvent( &key, &data, 1))
    {
        g_pEvents->HandleEvent(key, data);
    }
	pt->MessageQueue()->TimerTick();
}

#ifdef DDOMOD_DJ_BUFFERING
IInputStream* CreateBufferInStream( IMediaContentRecord* mcr )
{
    return CBuffering::GetInstance()->CreateInputStream(mcr);
}
#endif

void NotifyUserHDDFail()
{
    // (epg,7/29/2002): todo:
    DEBUGP( MAIN, DBGLEV_WARNING, "mn:hd failure\n"); 
}

void StartRestoreUI(PegPresentationManager& present)
{
    CKeyboard* pK = CKeyboard::GetInstance();
    
    DEBUGP(MAIN, DBGLEV_INFO, "*\n *\n  * Starting Restore UI\n *\n*\n");
    DEBUGP(MAIN, DBGLEV_INFO, "Create CPEGRestoreUserInterface\n");
    DEBUGP(MAIN, DBGLEV_INFO, "Create CRestoreEvents\n");

    // start datasources.  removed from CRestoreScreen ctor so that RS can be used from
    // outside the restore ui without interfering with normal datasource usage.

    // Open up our data sources and register them with the DSM. Supporting online updates
    // would require adding the NetDataSource to the DSM here
    // note that fat has to be added first due to a bug in how DSM::OpenByURL routines work
    // This always takes a long time
    CFatDataSource* pFatDS = CFatDataSource::Open(0, false);
    if( !pFatDS ) {
        DEBUGP( MAIN, DBGLEV_ERROR, "Can't open fat data source, about to bail\n");
    }
	else
	{
        CDataSourceManager::GetInstance()->AddDataSource( pFatDS );
	}

    // This only takes a long time if a CD is inserted
    CCDDataSource* pCDDS = CCDDataSource::Open( CD_DEVICE_NAME, CD_MOUNT_DIR, false);
    if( !pCDDS ) {
        DEBUGP( MAIN, DBGLEV_ERROR, "Can't open cd data source, about to bail\n");
    }
	else
	{
        CDataSourceManager::GetInstance()->AddDataSource( pCDDS );
    }

    // Show the UI now that the time consuming tasks above are done
    CSplashScreen::GetSplashScreen()->HideScreen();
    
    // continue bringing up the ui.
    IUserInterface* pUserInterface = new CPEGRestoreUserInterface;
    g_pEvents = new CRestoreEvents;
    g_pEvents->SetUserInterface( pUserInterface );

    DEBUGP(MAIN, DBGLEV_INFO, "Refresh interface\n");
    g_pEvents->RefreshInterface();

    DEBUGP(MAIN, DBGLEV_INFO, "Start presentation\n");
	pK->UnlockKeyboard();
    present.Execute();				
    return;
}

void StartRecoveryUI(PegPresentationManager& present)
{
    CKeyboard* pK = CKeyboard::GetInstance();
    
    DEBUGP(MAIN, DBGLEV_INFO, "*\n *\n  * Starting Restore UI\n *\n*\n");
    DEBUGP(MAIN, DBGLEV_INFO, "Create CPEGRestoreUserInterface\n");
    DEBUGP(MAIN, DBGLEV_INFO, "Create CRestoreEvents\n");

    // start datasources.  removed from CRestoreScreen ctor so that RS can be used from
    // outside the restore ui without interfering with normal datasource usage.

    // Open up our data sources and register them with the DSM. Supporting online updates
    // would require adding the NetDataSource to the DSM here
    CCDDataSource* pCDDS = CCDDataSource::Open( CD_DEVICE_NAME, CD_MOUNT_DIR, true);
    if( !pCDDS ) {
        DEBUGP( MAIN, DBGLEV_ERROR, "Can't open cd data source, about to bail\n");
    }
    CDataSourceManager::GetInstance()->AddDataSource( pCDDS );

    // continue bringing up the ui.
    IUserInterface* pUserInterface = new CPEGRestoreUserInterface;
    g_pEvents = new CRestoreEvents;
    g_pEvents->SetUserInterface( pUserInterface );

    DEBUGP(MAIN, DBGLEV_INFO, "Refresh interface\n");
    g_pEvents->RefreshInterface();

    put_event(EVENT_RESTORE_KICKSTART, 0);
    
    DEBUGP(MAIN, DBGLEV_INFO, "Start presentation\n");
	pK->UnlockKeyboard();
    present.Execute();				
    return;
}
