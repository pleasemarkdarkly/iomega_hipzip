// main.cpp: test harness to bring up main player
// danc@fullplaymedia.com 07/10/01
// (c) Fullplay Media Systems

#include <cyg/kernel/kapi.h>
#include <_modules.h>

// Interfaces to classes we need to set up
#include <util/eventq/EventQueueAPI.h>
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <devs/keyboard/Keyboard.h>
#include <main/ui/Keys.h>
#include <main/main/usb_setup.h>
#include <main/datastream/fatfile/BufferedFileInputStream.h>
#include <main/main/AppSettings.h>
#include <main/main/FatHelper.h>
#include <main/main/Recorder.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <datastream/outfilter/OutFilterKeys.h>
#include <datastream/waveout/WaveOutKeys.h>
#include <main/datasource/fatdatasource/FatDataSource.h>
#include <main/main/ProgressWatcher.h>
#include <main/main/KeyboardMap.h>
#include <stdio.h>
#include <fs/fat/sdapi.h>   // pc_gfirst

#ifdef __POGO
#include <devs/jogdial/JogDial.h>
#include <devs/lock/LockSwitch.h>
#endif

#define USE_HD

#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <main/content/metakitcontentmanager/MetakitContentManager.h>
#include <main/ui/PEGUserInterface.h>
#include <main/ui/UI.h>

#include <util/debug/debug.h>
#include <util/tchar/tchar.h>

#include <main/main/Events.h>

DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);

// thread data etc
#define NTHREADS       1
#define STACKSIZE   8192*4
#define MAIN_THREAD_PRIORITY (9)

static Cyg_Thread*  s_pThread;
static char*        s_pStack;
IUserInterface* g_pUserInterface;
CEventQueue* g_pEventQueue;
CEvents* g_pEvents;

void PegAppInitialize(PegPresentationManager *pPresentation);
extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};
extern CUIMaster* g_pUIMaster;
void main_event_loop();
void SafeMode(char* szReason);

#if 1
void cyg_user_start( void ) 
{
    DBEN( MAIN );
#ifdef __POGO
    // codecs
	#warning RAM codecs enabled! 
    memcpy((void *)0xed0000,(void *)0xe0020000,0x30000); 
#endif
    s_pStack = new char[STACKSIZE+1];
   	s_pThread = new Cyg_Thread( MAIN_THREAD_PRIORITY, // thread priority
								thread_entry,   // entry point
								0,              // data for thread
								"main thread",  // name
								(CYG_ADDRESS)s_pStack, // stack pointer
								STACKSIZE);     // stack size
    s_pThread->resume();
    DBEX( MAIN );
}
#endif

static const unsigned int sDefaultOutputList[] =  { WAVEOUT_KEY, 0,    };
static const unsigned int sDefaultFilterList[] =  { 0 };

const playstream_settings_t ps = 
{
    szStreamName:                 0,   // name of stream 
    OutputList:  sDefaultOutputList,   // output list
    FilterList:  sDefaultFilterList,   // filter list
};

#include <stdlib.h>
static void
mem_stats(void)
{
	struct mallinfo mem_info;		    
	mem_info = mallinfo();
	diag_printf("Memory system: Total=0x%08x Used = 0x%08x Free=0x%08x Max=0x%08x\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

bool KeyboardLocked() {
#ifdef __POGO
    return IsLocked();
#else
    return false;
#endif
}

void VerifyOrCreateSystemDirectory()
{
    VerifyOrCreateDirectory(SYSTEM_DIRECTORY_PATH);
    HideFile(SYSTEM_DIRECTORY_PATH);
}

int OpenHardDrive()
{
    DEBUGP(MAIN, DBGLEV_INFO, "Opening hard drive...\n");
    if (CFatDataSource* pFatDS = CFatDataSource::Open(0))
    {
        pFatDS->SetDefaultRefreshMode(IDataSource::DSR_ONE_PASS_WITH_METADATA);
        CPlayManager::GetInstance()->AddDataSource( pFatDS );
        DEBUGP(MAIN, DBGLEV_INFO, "Hdd ready.\n");
        return 1;
    }
    else
    {
        DEBUGP(MAIN, DBGLEV_ERROR, "Failed to open hard drive\n");
        return 0;
    }
}

const playstream_settings_t* PogoCreatePlayStream(IMediaContentRecord*)
{
    return (const playstream_settings_t*) 0;    
}

IInputStream* CreateBufferedInputStream( IMediaContentRecord* mcr )
{
    return CBufferedFatFileInputStream::GetInstance()->CreateInputStream(mcr);
}

#include <content/configurablemetadata/ConfigurableMetadata.h>

void thread_entry( cyg_uint32 data ) 
{
    DBEN( MAIN );

   	PegRect rect;
	rect.Set(0, 0, UI_X_SIZE - 1, UI_Y_SIZE - 1);

	PegScreen *pScreen = CreatePegScreen();
    PegThing::SetScreenPtr(pScreen);

	// create the PEG message Queue:
	PegMessageQueue *pMsgQueue = new PegMessageQueue();
    PegThing::SetMessageQueuePtr(pMsgQueue);

    PegPresentationManager present(rect);
	PegThing::SetPresentationManagerPtr(&present);
	pScreen->GenerateViewportList(&present);

    PegAppInitialize(&present);

    if (KeyboardLocked())
    {
        CSplashScreen::GetSplashScreen()->HideScreen();
        CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(g_pUIMaster,CSystemMessageScreen::PLAYER_LOCKED_POWOFF);
        while(1);
    }

	pScreen->Invalidate(present.mReal);
	PegMessage NewMessage(PM_DRAW);
	NewMessage.pTarget = &present;
	pMsgQueue->Push(NewMessage);
    
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap( &key_map );

#ifdef __POGO
    CJogDial* pJ = CJogDial::GetInstance();
    pJ->SetKeymap(KEY_DIAL_DOWN, KEY_DIAL_UP);
#endif

    CPlayManager* pPlayManager = CPlayManager::GetInstance();

    int iRes = OpenHardDrive();
    if (!SUCCEEDED(iRes))
        SafeMode("Unable to access disk");


    // check system files
    VerifyOrCreateSystemDirectory();
    bool bUpdateNeeded = !FileExists(METAKIT_PERSIST_PATH);

    // check whether we shutdown cleanly
    CProgressWatcher* pProgressWatcher = CProgressWatcher::GetInstance();
    pProgressWatcher->Load();
    eCurrentTask eTask = pProgressWatcher->GetLastTask();
    int nBootFails = pProgressWatcher->GetBootFails();    
    if (pProgressWatcher->WasBooting())
        ++nBootFails;
    if (nBootFails > BOOT_FAILURE_SAFEMODE_THRESHOLD)
        SafeMode("Unable to complete startup");
    switch (eTask) 
    {
        case TASK_GENERAL:
            break;
        case TASK_LOADING_METAKIT:
        case TASK_COMMITTING_METAKIT:
            DEBUGP( MAIN, DBGLEV_INFO, "mn:del metakit\n"); 
            pc_unlink(METAKIT_PERSIST_PATH);
            bUpdateNeeded = true;
            break;
        case TASK_LOADING_SETTINGS:
            DEBUGP( MAIN, DBGLEV_INFO, "mn:del settings\n"); 
            pc_unlink(SAVE_SETTINGS_PATH);
            break;
        case TASK_REFRESHING_CONTENT:
            DEBUGP( MAIN, DBGLEV_INFO, "mn:update needed\n"); 
            bUpdateNeeded = true;
            break;
    }

    // if debugging, set 'del' to true at runtime to delete system files.
    bool del = false;
    if (del)
        pc_unlink(METAKIT_PERSIST_PATH);
    if (del)
    {
        pc_unlink(SAVE_SETTINGS_PATH);
        pc_unlink(SYSTEM_DIRECTORY_PATH);
        pc_rmdir(SYSTEM_DIRECTORY_PATH);
    }

    // note that we're booting
    pProgressWatcher->SetBooting(true);
    pProgressWatcher->SetCurrentTask(TASK_LOADING_METAKIT);
    pProgressWatcher->Save();

    CMetakitContentManager* pCM = new CMetakitContentManager(METAKIT_PERSIST_PATH);

    pCM->AddStoredMetadata(MDA_DURATION, (void*)0);
    pCM->AddStoredMetadata(MDA_ALBUM_TRACK_NUMBER, (void*)0);

    // set the working content manager in the play manager
    DEBUGP(MAIN, DBGLEV_INFO, "Create content manager\n");
    pPlayManager->SetContentManager( pCM );
    // de serialize the file name store
    CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->DeSerialize();

    // note that we're committing metakit
    pProgressWatcher->SetCurrentTask(TASK_COMMITTING_METAKIT);
    // commit mk to update pointer fields updated during deserialization of the filename store
    ((CMetakitContentManager*) CPlayManager::GetInstance()->GetContentManager())->Commit();
    
    // run a content update if there was no metakit persist file
    if (bUpdateNeeded)
    {
        pProgressWatcher->SetCurrentTask(TASK_REFRESHING_CONTENT);
        pProgressWatcher->Save();
        // create a tchar string to show on the system message screen.
        DEBUGP(MAIN, DBGLEV_INFO, "Refresh Content\n");
        TCHAR msg[strlen("Finding New Files")+1];
        CharToTchar(msg,"Finding New Files");
        CSystemMessageScreen* sysmsg = CSystemMessageScreen::GetSystemMessageScreen();
        CSplashScreen* splash = CSplashScreen::GetSplashScreen();
        CPlayerScreen* player = (CPlayerScreen*) CPlayerScreen::GetPlayerScreen();
        // queue the system message screen after the splash screen.
        DEBUGP( MAIN, DBGLEV_INFO, "setting splash control to sysmsg %x\n",(int)sysmsg); 
        splash->SetControlScreen(sysmsg);
        sysmsg->SetParent(player);
        sysmsg->SetScreenData(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT,msg);
        pPlayManager->RefreshAllContent( IDataSource::DSR_ONE_PASS_WITH_METADATA, CONTENT_UPDATE_CHUNK_SIZE );
    }
    // instantiate playlist object
    DEBUGP(MAIN, DBGLEV_INFO, "Create playlist\n");
    pPlayManager->SetPlaylist( new CPogoPlaylist("") );
    DEBUGP(MAIN, DBGLEV_INFO, "Init playstream\n");
    CPlayManager::GetInstance()->SetPlaystream( &ps, PogoCreatePlayStream );
    // init the mediaplayer's callback member to use pogo buffering.
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferedInputStream );
    // setup MP to use sram-based codec instances.
    CMediaPlayer::GetInstance()->SetCodecPool( (unsigned int *)CODEC_SRAM_POOL_ADDRESS, CODEC_SRAM_POOL_SIZE );
    // set the src blending threshold
    if (!SUCCEEDED(CMediaPlayer::GetInstance()->SetSRCBlending( SRC_BLENDING_THRESHOLD ))) {
        DEBUGP( MAIN, DBGLEV_INFO, "SRC blending threshold %d rejected\n",SRC_BLENDING_THRESHOLD); 
    }
    // instantiate user interface
    DEBUGP(MAIN, DBGLEV_INFO, "Create peg ui\n");
    g_pUserInterface = new CPEGUserInterface;
    DEBUGP(MAIN, DBGLEV_INFO, "Create event queue\n");
    // instantiate event processing
    g_pEventQueue = CEventQueue::GetInstance();
    g_pEvents = new CEvents;
    g_pEvents->SetUserInterface( g_pUserInterface );
    // load saved settings
    DEBUGP(MAIN, DBGLEV_INFO, "Load saved settings\n");
    pProgressWatcher->SetCurrentTask(TASK_LOADING_SETTINGS);
    pProgressWatcher->Save();
    g_pEvents->LoadState();
    // if we didn't fire off a content update, then we're done booting.
    if (!bUpdateNeeded)
    {
        // begin watching activity levels to perform idle shutdown (this will happen after the end of the content update if one is running
        g_pEvents->EnterIdleShutdownMode();
        // all critical tasks are done, so tell the progress watcher to relax.
        pProgressWatcher->SetBootFails(0);
        pProgressWatcher->SetCurrentTask(TASK_GENERAL);
        pProgressWatcher->SetBooting(false);
        pProgressWatcher->Save();
    }
    // start the UI
    DEBUGP(MAIN, DBGLEV_INFO, "Refresh interface\n");
    g_pEvents->RefreshInterface();
    DEBUGP(MAIN, DBGLEV_INFO, "Start presentation\n");
    InitUSB();

   	present.Execute();
}

void DisplayWarning(char* szWarning)
{
    TCHAR tszWarning[PLAYLIST_STRING_SIZE];
    CharToTchar(tszWarning,szWarning);
    CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(g_pUIMaster, CSystemMessageScreen::TEXT_MESSAGE,tszWarning);
}

// notify the user that there was an abnormal shutdown, and display a repercussion string
void NotifyBadShutdown(char* szRepercussion)
{
    char szWarning[PLAYLIST_STRING_SIZE];
    sprintf (szWarning, "Abnormal shutdown. %s", szRepercussion);
    DisplayWarning(szWarning);
}

// bring up usb, notify the user, display a reason string, and wait.
void SafeMode(char* szReason)
{
    // warn user that safe mode is required
    char szWarning[PLAYLIST_STRING_SIZE];
    sprintf (szWarning, "%s.  Entering safe mode.",szReason);
    CSplashScreen::GetSplashScreen()->HideScreen();
    DisplayWarning(szWarning);
    strcpy (szWarning, "Safe Mode : USB Ready.");
    TCHAR tszWarning[PLAYLIST_STRING_SIZE];
    CharToTchar(tszWarning, szWarning);
    CSystemMessageScreen::GetSystemMessageScreen()->SetScreenData(CSystemMessageScreen::TEXT_MSG_NO_TIMEOUT, tszWarning);
    while (1);
}

void PegIdleFunction()
{
    unsigned int key;
    void* data;
    PegThing* pt = 0;
    g_pEvents->TestForRecentActivity();
    if (g_pEventQueue->TimedGetEvent( &key, &data, 10))
        g_pEvents->Event(key, data);
	pt->MessageQueue()->TimerTick();
}