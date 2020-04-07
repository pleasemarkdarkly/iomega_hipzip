// Events.cpp: event processing
// ericg@fullplaymedia.com 12/11/01
// (c) Fullplay Media Systems
#include <core/mediaplayer/MediaPlayer.h>
#include <core/events/SystemEvents.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <io/audio/VolumeControl.h>        // interface to volume
#include <fs/fat/sdapi.h>   // pc_unlink
#include <codec/common/Codec.h>              // set_stream_event_data_t definition
#include <datastream/fatfile/FileInputStream.h>     // TEST
#include <datastream/fatfile/FileOutputStream.h>     // TEST
#include <main/content/metakitcontentmanager/MetakitContentManager.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>   // TEST
#include <gui/peg/peg.hpp>

#include <main/main/EventTypes.h>
#include <main/main/AppSettings.h>
#include <main/main/usb_setup.h>
#include <main/main/PlaylistConstraint.h>
#include <main/main/Recorder.h>
#include <main/main/ProgressWatcher.h>
#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <main/ui/UI.h>
#include <main/ui/Strings.hpp>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/datastream/fatfile/Consumer.h>
#include <main/util/filenamestore/FileNameStore.h>

#include <stdio.h>

#ifdef DDOMOD_MAIN_TESTHARNESS
#include <main/testharness/testharness.h>
#endif

#include <main/main/Events.h>
DEBUG_MODULE_S( EV, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(EV);

#ifndef NULL
#define NULL 0
#endif

extern CUIMaster* g_pUIMaster;
extern void ReportSystemMemory(const char* szCaption);
extern CEvents* g_pEvents;
extern bool FileExists(char* url);
extern void VerifyOrCreateSystemDirectory();
extern int OpenHardDrive();
void BreakPointOnKey11();
void SaveRegistryToDisk();
void LoadRegistryFromDisk();

CEvents::CEvents() : m_bIdleShutdownMode(false), m_bBacklightEnabled(false), m_bBacklightOn(false), m_bKbdLocked(false)
{
    m_pUserInterface = NULL;
    m_pPlayManager = CPlayManager::GetInstance();

    m_iTrackTime = 0;
    m_iArtistIndex = m_iAlbumIndex = m_iGenreIndex = 0;
    m_bRefreshArtists = m_bRefreshAlbums = m_bRefreshGenres = m_bRefreshPlaylists = true;
    m_nTimeOfLastActivity = cyg_current_time();
}

CEvents::~CEvents() 
{}

void CEvents::SetUserInterface( IUserInterface* pUI ) 
{
    m_pUserInterface = pUI;
}

void CEvents::LoadState()
{
    DEBUGP( EV, DBGLEV_INFO, "ev:loading settings\n");
    // load the registry from disk
    LoadRegistryFromDisk();
    // volume
    CVolumeControl::GetInstance()->RestoreFromRegistry();
    // player screen
    CPlayerScreen* ps = (CPlayerScreen*) CPlayerScreen::GetPlayerScreen();
    ps->RestoreFromRegistry();
    // recorder
    CRecorder::GetInstance()->RestoreFromRegistry();
    DEBUGP( EV, DBGLEV_INFO, "ev:loading constraints\n");
    // sync playerscreen with volume control
    ps->SetVolume(CVolumeControl::GetInstance()->GetVolume());
    // playlist constraints
    CPlaylistConstraint::GetInstance()->RestoreFromRegistry();
    // the restore routines will add entries if they aren't already in the reg.  save changes to reg to disk.
    SaveRegistryToDisk();
    DEBUGP( EV, DBGLEV_INFO, "ev:settings loaded\n");
}

void CEvents::SaveState()
{
    DEBUGP( EV, DBGLEV_INFO, "Saving settings\n");
    // volume
    CVolumeControl::GetInstance()->SaveToRegistry();
    // player screen
    CPlayerScreen* ps = (CPlayerScreen*) CPlayerScreen::GetPlayerScreen();
    ps->SaveToRegistry();
    // recorder
    CRecorder::GetInstance()->SaveToRegistry();
    // playlist constraints
    CPlaylistConstraint::GetInstance()->SaveToRegistry();
    // save the registry to disk
    SaveRegistryToDisk();
}

void LoadRegistryFromDisk()
{
    CFatFileInputStream file;
    if (SUCCEEDED(file.Open(SAVE_SETTINGS_PATH)))
    {
        CRegistry::GetInstance()->RestoreState( &file );
        file.Close();
    }
    else
    {
        DEBUGP( EV, DBGLEV_INFO, "EV:Failed To Load Reg From Disk\n");
        SaveRegistryToDisk();
    }
}

void SaveRegistryToDisk()
{
    // save the registry to disk
    CFatFileOutputStream file;
    if (SUCCEEDED(file.Open(SAVE_SETTINGS_PATH)))
    {
        CRegistry::GetInstance()->SaveState( &file );
        file.Close();
    }
    else
    {
        DEBUGP( EV, DBGLEV_INFO, "EV:Failed To Save Reg To Disk\n");
    }
}

void CEvents::RefreshInterface()
{
    if (m_pUserInterface)
    {
        CVolumeControl* vc = CVolumeControl::GetInstance();
        m_pUserInterface->SetVolume(vc->GetVolume());
        m_pUserInterface->SetBass(vc->GetBass());
        m_pUserInterface->SetTreble(vc->GetTreble());
        m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
        m_pUserInterface->SetTrackTime(m_iTrackTime);
    }
}

void CEvents::SynchPlayState()
{
    if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        m_pUserInterface->NotifyPlaying();
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PAUSED)
        m_pUserInterface->NotifyPaused();
    else
    {
        m_iTrackTime = 0;
        m_pUserInterface->NotifyStopped();
    }
}

// Used for sorting a ContentKeyValueVector.
static bool
CompareKeyValueRecord(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstrcmp(a.szValue, b.szValue) <= 0;
}

// (epg,10/23/2001): TODO: send a message instead of using this linkage
void DumpContent()
{
    CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
    MediaRecordList mrl;
    mcm->GetAllMediaRecords(mrl);
    char temp[256];
    int i = 0;
    for (MediaRecordIterator it = mrl.GetHead(); it != mrl.GetEnd(); ++it)
    {
        void* pdata;
        (*it)->GetAttribute(MDA_ARTIST, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( EV, DBGLEV_FATAL, "track %d has artist '%s'", i++, temp);
        (*it)->GetAttribute(MDA_ALBUM, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( EV, DBGLEV_FATAL, ", album '%s'", temp);
        (*it)->GetAttribute(MDA_GENRE, &pdata);
        TcharToChar(temp,(TCHAR*)pdata);
        DEBUGP( EV, DBGLEV_FATAL, ", genre '%s'", temp);
        DEBUGP( EV, DBGLEV_FATAL, ", DSid '%d'", (*it)->GetDataSourceID());
        DEBUGP( EV, DBGLEV_FATAL, ", url '%s'\n", (*it)->GetFileNameRef()->URL());
    }
}

int CEvents::HandleFilesystemUpdate(int key, void* data)
{
    CFileNameStore* store = (CFileNameStore*) data;
    DEBUGP( EV, DBGLEV_INFO, "Received filesystem update\n");
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenDataByChar(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, "Comparing Files");
    ((IQueryableContentManager*)CPlayManager::GetInstance()->GetContentManager())->CompareFileSystemUpdate( store );
    // commit any changes to the mcm.  this will pick up content deletions, etc.
    CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
    mcm->Commit();
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

int CEvents::HandleStreamSet(int key, void* data)
{
    set_stream_event_data_t* eventData = (set_stream_event_data_t*) data;
    m_pUserInterface->SetTrack(eventData);
    m_pUserInterface->SetMessage(m_sCurrentPlaylistName);
    m_pPlayManager->HandleEvent(key, data);
    return 1;
}

bool CEvents::SetCurrentSong()
{
    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    IPlaylistEntry* entry = pl->GetCurrentEntry();
    // no current? set first.
    if (!entry)
        pl->SetCurrentEntry(entry = pl->GetEntry(0,IPlaylist::NORMAL));
    IPlaylistEntry* first = entry;
    ERESULT res;
    while (entry && FAILED(res = CMediaPlayer::GetInstance()->SetSong(entry)))
    {
        // get a pointer to the next entry
        entry = pl->GetNextEntry(entry,IPlaylist::NORMAL);
        // sync the playlist to the new entry
        pl->SetCurrentEntry(entry);
        if (entry == first)
            entry = 0;
    }
    if (FAILED(res))
    {
        CPlaylistConstraint::GetInstance()->HandleInvalidPlaylist();
        return false;
    }
    return true;
}

int CEvents::HandleStreamEnd(int key, void* data)
{
    m_pUserInterface->NotifyStreamEnd();
    SynchPlayState();
    ERESULT res = m_pPlayManager->HandleEvent(key, data);
    if (res == PM_PLAYLIST_END)
    {
        CPlayManager::GetInstance()->Stop();
        CPogoPlaylist* pl = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
        pl->SetCurrentEntry(pl->GetEntry(0,IPlaylist::NORMAL));
        if (SetCurrentSong() && pl->IsModeRepeating())
            CPlayManager::GetInstance()->Play();
    }
    return 1;
}

int CEvents::HandleMediaRemoved(int key, void* data)
{
    m_pPlayManager->HandleEvent(key, data);
    m_pUserInterface->NotifyMediaRemoved((int)data);
    return 1;
}

int CEvents::HandleMediaInserted(int key, void* data)
{
    m_pPlayManager->HandleEvent(key, data);
    m_pUserInterface->NotifyMediaInserted((int)data);
    return 1;
}

int CEvents::HandleContentUpdateBegin(int key, void* data)
{
    //m_pUserInterface->SetMessage("Starting content update on data source %d", (int)data);
    m_pPlayManager->HandleEvent(key, data);
    return 1;
}

int CEvents::HandleContentUpdateEnd(int key, void* data)
{
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenDataByChar(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, "Saving Database");
    DEBUGP( EV, DBGLEV_INFO, "update finished, saving info\n");
    CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->Serialize();

    CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
    DEBUGP( EV, DBGLEV_INFO, "commiting\n");
    mcm->Commit();
    DEBUGP( EV, DBGLEV_INFO, "commit done\n");
    // if the recorder is in session, then set the album constraint to the newly available key, and then save state.
    CRecorder* rc = CRecorder::GetInstance();
    if (rc->InSession())
    {
        DEBUGP( EV, DBGLEV_INFO, "setting recording playlist constraints, saving state\n");
        char szAlbum[32];
        TCHAR tszAlbum[32];
        int iAlbum = mcm->GetAlbumKey(CharToTchar(tszAlbum,rc->GetSessionName(szAlbum)));
        CPlaylistConstraint::GetInstance()->Constrain(CMK_ALL, iAlbum, CMK_ALL, true);
        SaveState();
    }
    // reload the playlist constraints from file, and attempt to reload the best possible semblance of the old playlist.
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenDataByChar(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, "Restoring Settings");
    DEBUGP( EV, DBGLEV_INFO, "restoring state\n");
    LoadState();
    // if an update goes quickly, then the splash may still be up, and will need to go straight to the player screen.
    if (CSplashScreen::GetSplashScreen()->IsActive())
    {
        DEBUGP( EV, DBGLEV_INFO, "setting splash ctl to player scrn %x\n",(int)CPlayerScreen::GetPlayerScreen()); 
        CSplashScreen::GetSplashScreen()->SetControlScreen(CPlayerScreen::GetPlayerScreen());
    }
    else
    {
        CSystemMessageScreen::GetSystemMessageScreen()->HideScreen();
        g_pUIMaster->Add(CPlayerScreen::GetPlayerScreen());
    }
    DEBUGP( EV, DBGLEV_INFO, "update finished\n");
    // resume watching activity levels to perform idle shutdown
    EnterIdleShutdownMode();
    // let the progress watcher relax
    CProgressWatcher* pProgressWatcher = CProgressWatcher::GetInstance();
    pProgressWatcher->SetBootFails(0);
    pProgressWatcher->SetCurrentTask(TASK_GENERAL);
    pProgressWatcher->SetBooting(false);
    pProgressWatcher->Save();

    return -1;
}

int CEvents::HandleContentMetadataUpdate(int key, void* data)
{
    ReportSystemMemory("md update");
    ((CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

int CEvents::HandleContentMetadataUpdateEnd(int key, void* data)
{
    DEBUGP( EV, DBGLEV_INFO, "metadata update ended, saving MK\n");
    //DumpContent();
    // (epg,10/24/2001): TODO: check necessity of this, given different messaging.  also is more sensitive since in ui thread.
    cyg_thread_delay(100);
    CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
    mcm->Commit();
    DEBUGP( EV, DBGLEV_INFO, "update done, commit done\n");
    //DumpContent();
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

int CEvents::HandleContentUpdateError(int key, void* data)
{
    //m_pUserInterface->SetMessage("Error updating content on data source %d", (int)data);
    m_pPlayManager->HandleEvent(key, data);
    return 1;
}

int CEvents::HandleContentUpdate(int key, void* data)
{
    //DEBUGP( EV, DBGLEV_INFO, "Received content update\n");
    m_bRefreshArtists = true;
    m_bRefreshAlbums = true;
    m_bRefreshGenres = true;
    m_bRefreshPlaylists = true;
    ReportSystemMemory("update");
    m_pPlayManager->HandleEvent(key, data);
    return -1;
}

// stop all activities and serialize all settings to disk, in preparation for either usb connect or shutdown.  
// this won't be performed if shutdown is of a low-power nature.
void CEvents::PrepareToYield()
{
    // stop recording
    DEBUGP( EV, DBGLEV_INFO, "ev:prep yield\n"); 
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    if (ps->IsRecording())
        CRecorder::GetInstance()->StopRecording();
    DEBUGP( EV, DBGLEV_INFO, "saving state from prep to yield\n"); 
    SaveState();
    // if the menus are up, they will interfere with the sys msg scrn display
    ps->HideMenus();
    // shut down playback
    DEBUGP( EV, DBGLEV_INFO, "36\n"); 
    CMediaPlayer::GetInstance()->Deconfigure();
    DEBUGP( EV, DBGLEV_INFO, "37\n"); 
    // halt buffering threads
    CBufferedFatFileInputStreamImp* bis = CBufferedFatFileInputStreamImp::GetInstance();
    DEBUGP( EV, DBGLEV_INFO, "38\n"); 
    cyg_thread_delay(50);
    bis->GetProducerMsgPump()->StopProducing();
    DEBUGP( EV, DBGLEV_INFO, "39\n"); 
    bis->GetProducerMsgPump()->PauseProducerThread();
    DEBUGP( EV, DBGLEV_INFO, "40\n"); 
    CCacheMan::GetInstance()->CloseAllFiles();
    DEBUGP( EV, DBGLEV_INFO, "41\n"); 
    // make sure the mk cm is in a known state before the file is yanked from it.
    CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
    DEBUGP( EV, DBGLEV_INFO, "42\n"); 
    mcm->Commit();
    DEBUGP( EV, DBGLEV_INFO, "43\n"); 
    delete mcm;
    DEBUGP( EV, DBGLEV_INFO, "44\n"); 
    cyg_thread_delay(50);
    pc_dskclose(0);
}

int CEvents::HandleUSBConnect(int key, void* data)
{
    DEBUGP( EV, DBGLEV_INFO, "ev:enter usb\n"); 
    // suspend watching activity levels to perform idle shutdown
    ExitIdleShutdownMode();
    // stop activity and save to disk
    PrepareToYield();
    // bring up the SystemMessageScreen
    CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(CPlayerScreen::GetPlayerScreen(), CSystemMessageScreen::USB_CONNECTED);
    // if the splash screen is still done, have it yield to sysmsg for the "usb conn" screen.
    if (CSplashScreen::GetSplashScreen()->IsActive())
        CSplashScreen::GetSplashScreen()->SetControlScreen(CSystemMessageScreen::GetSystemMessageScreen());

}

int CEvents::HandleUSBDisconnect(int key, void* data)
{
    DEBUGP( EV, DBGLEV_INFO, "ev:exit usb\n"); 
    // put up a message screen to keep the user informed
    TCHAR msg[strlen(CONTENT_UPDATE_INITIAL_SYSMSGSCRN_TEXT)+1];
    CharToTchar(msg,CONTENT_UPDATE_INITIAL_SYSMSGSCRN_TEXT);
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenData(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, msg);
    // clear out buffering state and get the threads running again
    CBufferedFatFileInputStreamImp* bis = CBufferedFatFileInputStreamImp::GetInstance();
    bis->GetConsumerMsgPump()->ResumeConsumerThread();
    bis->GetConsumerMsgPump()->GetConsumer()->ClearReadBufferLock();
    // clear the playerscreen
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->ClearTrack();
    // in case we were recording when we connected, close down the session.
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->ExitRecordingSession();
    // get the hard drive up again
    pc_system_init(0);
    InitUSB();
    // make sure we still have a system directory
    VerifyOrCreateSystemDirectory();
    // remove loaded constraints
    CPlaylistConstraint::GetInstance()->Constrain();
    // clear the playlist
    CPlayManager::GetInstance()->GetPlaylist()->Clear();    
    // restart producer thread.
    bis->GetProducerMsgPump()->ResumeProducerThread();
    // hide the usb screen
    CSystemMessageScreen::GetSystemMessageScreen()->HideScreen();
    // figure out if the metakit file was deleted
    bool bPersistenceFileExists = FileExists(METAKIT_PERSIST_PATH);
    // create a new content manager to live on the new fat instance.
    CMetakitContentManager* mcm = new CMetakitContentManager(METAKIT_PERSIST_PATH);
    mcm->AddStoredMetadata(MDA_DURATION, (void*)0);
    mcm->AddStoredMetadata(MDA_ALBUM_TRACK_NUMBER, (void*)0);
    CPlayManager::GetInstance()->SetContentManager( mcm );
    // if there was a preexisting metakit file, load data from it.
    if (bPersistenceFileExists)
    {
        mcm->GetFileNameStore()->DeSerialize();
        mcm->Commit();
    }
    // run a content update to catch any content changes
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenData(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, LS(SID_FINDING_NEW_FILES));
    CPlayManager::GetInstance()->RefreshAllContent( IDataSource::DSR_ONE_PASS_WITH_METADATA, CONTENT_UPDATE_CHUNK_SIZE );
}

int CEvents::HandleClipDetected(int key, void* data)
{
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HandleClipDetected();
    return -1;
}

// returns -1 if event not handled
//          0 if handled and no further processing needed (override)
//          1 if handled and further processing needed
int CEvents::Event( int key, void* data ) 
{
    if( !m_pUserInterface ) return -1;

    switch( key ) {

        case EVENT_KEY_HOLD:
            return HandleKeyHold(key, data);
        case EVENT_KEY_PRESS:
            return HandleKeyPress(key, data);
        case EVENT_KEY_RELEASE:
            return HandleKeyRelease(key, data);
        case EVENT_FILESYSTEM_UPDATE:
            return HandleFilesystemUpdate(key, data);
        case EVENT_STREAM_SET:
            HandleStreamSet(key, data);
            return 1;
        case EVENT_STREAM_PROGRESS:
            HandleStreamProgress(key, data);
            return 1;
        case EVENT_STREAM_END:
        case EVENT_STREAM_FAIL:
            HandleStreamEnd(key,data);
            return 1;
        case EVENT_MEDIA_REMOVED:
            HandleMediaRemoved(key,data);
            return 1;
        case EVENT_MEDIA_INSERTED:
            HandleMediaInserted(key,data);
            return 1;
        case EVENT_CONTENT_UPDATE_BEGIN:
            HandleContentUpdateBegin(key,data);
            return 1;
        case EVENT_CONTENT_UPDATE_END:
            return HandleContentUpdateEnd(key,data);
        case EVENT_CONTENT_METADATA_UPDATE:
            return HandleContentMetadataUpdate(key, data);
        case EVENT_CONTENT_METADATA_UPDATE_END:
            return HandleContentMetadataUpdateEnd(key, data);
        case EVENT_CONTENT_UPDATE_ERROR:
            HandleContentUpdateError(key,data);
            return 1;
        case EVENT_CONTENT_UPDATE:
            return HandleContentUpdate(key,data);
        case EVENT_USB_CONNECT:
            HandleUSBConnect(key, data);
            return 0;
        case EVENT_USB_DISCONNECT:
            HandleUSBDisconnect(key, data);
            return 0;
        case EVENT_CLIP_DETECTED:
            HandleClipDetected(key, data);
            return 0;
        default:
            m_pPlayManager->HandleEvent(key, data);
            return -1;
    };
}
#ifdef DDOMOD_MAIN_TESTHARNESS

#define TS_HOLD_DELAY 25
#define TS_MIN_EVENT_DELAY 50
#define TS_MAX_EVENT_DELAY 300
#define TS_MIN_HOLD_COUNT 0
#define TS_MAX_HOLD_COUNT 5

void CEvents::StartTestStimulator()
{
    DEBUGP( EV, DBGLEV_INFO, "starting test stim\n"); 
    CTestStimulator::GetInstance()->StartMonkeyTest(TS_HOLD_DELAY, TS_MAX_EVENT_DELAY, TS_MAX_HOLD_COUNT, TS_MIN_EVENT_DELAY, TS_MIN_HOLD_COUNT);
}

void CEvents::StopTestStimulator()
{
    DEBUGP( EV, DBGLEV_INFO, "stopping test stim\n"); 
    CTestStimulator::GetInstance()->StopTest();
}
#endif

void CEvents::TestForRecentActivity()
{
    int nTicks = cyg_current_time();
    if (m_bIdleShutdownMode)
        if (nTicks - m_nTimeOfLastActivity > PLAYER_IDLE_SHUTDOWN_TICKS)
            ShutdownPlayer();
    if (m_bBacklightOn)
        if (nTicks - m_nTimeOfLastActivity > BACKLIGHT_TIMEOUT_TICKS)
        {
#ifdef LCD_BACKLIGHT
            LCDSetBacklight(LCD_BACKLIGHT_OFF);
#endif
            m_bBacklightOn = false;
        }
}

// the player isn't doing anything special, so shutdown if idle for too long.
void CEvents::EnterIdleShutdownMode()
{
    m_nTimeOfLastActivity = cyg_current_time();
    m_bIdleShutdownMode = true;
}

// the player is performing non-interactive operations that shouldn't be subject to idle shutdown.
void CEvents::ExitIdleShutdownMode()
{
    m_bIdleShutdownMode = false;
}

// tidy state and shut down to conserve power
void CEvents::ShutdownPlayer()
{
    // stop activity and save to disk
    PrepareToYield();
    // bring up the SystemMessageScreen.  this will actually perform the power down sequence, since it has a handy timer to manage the delay.
    CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(CPlayerScreen::GetPlayerScreen(), CSystemMessageScreen::SHUTTING_DOWN);
}

extern void SafeMode(char* szReason);
void CEvents::BreakPointOnKey11()
{
    DEBUGP( EV, DBGLEV_TRACE, "Breakpoint function\n");
    
    volatile bool ss = false;
    volatile bool ls = false;
    volatile bool sm = false;
    volatile bool rp = false;
    volatile bool cu = false;
    volatile bool cs = false;
    volatile bool enu = false;
    volatile bool exu = false;
    volatile bool ips = false;

    if (ss)
        g_pEvents->SaveState();
    if (ls)
        g_pEvents->LoadState();
    if (sm)
        SafeMode("Repeated Boot Failure!");
    if (rp)
    {
        CMetakitContentManager* mcm = (CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager();
        mcm->Clear();
        delete mcm;
        pc_unlink(METAKIT_PERSIST_PATH);
        mcm = new CMetakitContentManager(METAKIT_PERSIST_PATH);
        CPlayManager::GetInstance()->SetContentManager( mcm );
    }
    if (cu)
        CPlayManager::GetInstance()->RefreshAllContent( IDataSource::DSR_ONE_PASS_WITH_METADATA, CONTENT_UPDATE_CHUNK_SIZE );
    if (cs)
        ReportSystemMemory("key 11");
    if (enu)
        HandleUSBConnect(0,0);
    if (exu)
        HandleUSBDisconnect(0,0);
    if (ips)
        CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(CPlayerScreen::GetPlayerScreen(), CSystemMessageScreen::INVALID_PLAYLIST);
}

void CEvents::SetKeyboardLock(bool bLocked)
{
    m_bKbdLocked = bLocked;
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetLockIcon(bLocked);
}