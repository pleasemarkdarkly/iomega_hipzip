//
// main.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//
// This is the second demo of the Dadio (TM) SDK.  It demonstrates content
// loading/saving as well as construction of playlists based on artist/album/genre.
//

#include <cyg/kernel/kapi.h>
#include <_modules.h>

// Interfaces to classes we need to set up
#include <util/eventq/EventQueueAPI.h>
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <devs/keyboard/Keyboard.h>

// Keys for classes that we want to have loaded
#include <datastream/outfilter/OutFilterKeys.h>
#include <datastream/waveout/WaveOutKeys.h>

// Interfaces to components we instantiate
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <content/metakitcontentmanager/MetakitContentManager.h>
#include "Events.h"

// Data sources can be added in the module file and included/excluded by
// using the following #defines.
#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#define USE_CD  // Comment this out to exclude the CD from the demo
#endif
#ifdef DDOMOD_DATASOURCE_FAT
#include <datasource/fatdatasource/FatDataSource.h>
#define USE_CF  // Comment this out to exclude the compact flash from the demo
#define USE_HD  // Comment this out to exclude the hard drive from the demo
#endif

// The simple GUI and serial UI can be switched in the modules file.
#if defined(DDOMOD_UI_SIMPLEGUI)
#include <main/ui/simplegui/SimpleGUIUserInterface.h>
#elif defined(DDOMOD_UI_SERIAL)
#include <main/ui/serial/SerialUserInterface.h>
#endif

#include <util/debug/debug.h>

// Set up the debug module used for the main app.
DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);

// Thread data
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

//
// Keymaps
//
static unsigned int press_map[] =
{
    // buttons S101-S106
    KEY_LOAD_STATE,
    KEY_LOAD_PLAYLIST,
    KEY_REFRESH_CONTENT,
    KEY_LIST_BY_PLAYLIST,
    KEY_LIST_BY_GENRE,
    KEY_VOLUME_DOWN,
    // buttons S109-S114
    KEY_SAVE_STATE,
    KEY_SAVE_PLAYLIST,
    KEY_PLAYLIST_MODE,
    KEY_LIST_BY_ALBUM,
    KEY_LIST_BY_ARTIST,
    KEY_VOLUME_UP,
    // buttons S107, S108, S115, S116
    KEY_NEXT,
    KEY_STOP,
    KEY_PREVIOUS,
    KEY_PLAY_PAUSE
};

static unsigned int hold_map[]  =
{
    0,0,0,0,0,
    KEY_VOLUME_DOWN,
    0,0,0,0,0,
    KEY_VOLUME_UP,
    0,0,0,0
};

// The list of keys to send release events.
static unsigned int release_map[]  =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static key_map_t key_map = 
{
    num_columns  :  3,
    num_rows     :  6,
    num_buttons  : 16,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)press_map,
    hold_map     : (const unsigned int*const)hold_map,
    release_map  : release_map,
};


// Function prototypes
extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};

// This is where execution begins.
void cyg_user_start( void ) 
{
    DBEN(MAIN);
    
    cyg_thread_create(9, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume(threadh[0]);
    
    DBEX(MAIN);
}

// Playstream settings for the media player.
const playstream_settings_t ps = 
{
    WAVEOUT_KEY,
    0,//    filter_list,
};

void thread_entry(cyg_uint32 data)
{
    DBEN(MAIN);

    // Get a pointer to the play manager singleton.
    CPlayManager* pPlayManager = CPlayManager::GetInstance();

    // Create the content manager and add it to the play manager..
    CMetakitContentManager* pCM = new CMetakitContentManager();
    pPlayManager->SetContentManager(pCM);

    // Create the helper class for handling events.
    CEvents* pEvents = new CEvents;

    // Create a simple playlist object and set it as the play manager's current playlist.
    pPlayManager->SetPlaylist(new CSimplePlaylist(""));

    // Initialize the keyboard.
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap(&key_map);

    //
    // Add data sources to the play manager.
    //
#ifdef USE_CF
    DEBUG(MAIN, DBGLEV_INFO, "Opening cf card\n");
    if (CFatDataSource* pCFDS = CFatDataSource::Open(1))
    {
        // Keep the chunk size down so we can show progress as content is scanned.
        pCFDS->SetDefaultUpdateChunkSize(20);

        pPlayManager->AddDataSource(pCFDS);
        DEBUG(MAIN, DBGLEV_INFO, "Opened cf card\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open cf card\n");
#endif  // USE_CF

#ifdef USE_HD
    DEBUG(MAIN, DBGLEV_INFO, "Opening hard drive\n");
    if (CFatDataSource* pHDDS = CFatDataSource::Open(0))
    {
        // Keep the chunk size down so we can show progress as content is scanned.
        pHDDS->SetDefaultUpdateChunkSize(20);

        pPlayManager->AddDataSource(pHDDS);

        // Use the hard drive to store volume/bass/treble levels, the content
        // database, and playlists.
        pEvents->SetSettingsURL("file://a:\\settings.ddo");
        pEvents->SetContentStateURL("file://a:\\content.dat");
        pEvents->SetPlaylistURL("file://a:\\playlist.dpl");

        DEBUG(MAIN, DBGLEV_INFO, "Opened hard drive\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open hard drive\n");
#endif  // USE_HD

#ifdef USE_CD
    DEBUG(MAIN, DBGLEV_INFO, "Opening CD\n");
    if (CCDDataSource* pCDDS = CCDDataSource::Open("/dev/cda/", "/"))
    {
        pPlayManager->AddDataSource(pCDDS);
        DEBUG(MAIN, DBGLEV_INFO, "Opened CD\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open CD\n");
#endif  // USE_CD

    CMediaPlayer::GetInstance()->SetPlaystream(&ps);

    // Create a demo user interface.
#if defined(DDOMOD_UI_SIMPLEGUI)
    IUserInterface* pUserInterface = new CSimpleGUIUserInterface;
#elif defined(DDOMOD_UI_SERIAL)
    IUserInterface* pUserInterface = new CSerialUserInterface;
#else
#error No user interface defined
#endif

    pUserInterface->SetBanner("iObjects Dadio SDK Demo 2");

    // Get a handle to the global event queue.
    CEventQueue* pEventQueue = CEventQueue::GetInstance();

    pEvents->SetUserInterface(pUserInterface);
    pEvents->RefreshInterface();

    // Enter the main event loop.
    do
    {
        unsigned int key;
        void* data;
        pEventQueue->GetEvent(&key, &data);
        pEvents->Event(key, data);

    } while (1);
}
