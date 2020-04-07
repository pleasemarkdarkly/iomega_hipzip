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
// This is the first demo of the Dadio (TM) SDK.  It demonstrates simple
// playback from a single data source.  Controls are provided for adjusting
// volume, bass, and treble, as well as loading and saving state.
//

#include <cyg/kernel/kapi.h>
#include <_modules.h>

// Interfaces to classes we need to set up
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <devs/keyboard/Keyboard.h>
#include <util/eventq/EventQueueAPI.h>

// Keys for classes that we want to have loaded
#include <datastream/outfilter/OutFilterKeys.h>
#include <datastream/waveout/WaveOutKeys.h>

// Interfaces to components we instantiate
#include <datasource/fatdatasource/FatDataSource.h>
#include <content/simplecontentmanager/SimpleContentManager.h>
#include <content/simplemetadata/SimpleMetadata.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>
#include "Events.h"

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
static cyg_thread   thread[NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

//
// Keymaps
//

// The list of keys to send on 'press' events
static unsigned int press_map[] =
{
    // buttons S101-S106
    KEY_LOAD_STATE,
    0,
    KEY_REFRESH_CONTENT,
    KEY_TREBLE_DOWN,
    KEY_BASS_DOWN,
    KEY_VOLUME_DOWN,
    // buttons S109-S114
    KEY_SAVE_STATE,
    0,
    KEY_PLAYLIST_MODE,
    KEY_TREBLE_UP,
    KEY_BASS_UP,
    KEY_VOLUME_UP,
    // buttons S107, S108, S115, S116
    KEY_NEXT,
    KEY_STOP,
    KEY_PREVIOUS,
    KEY_PLAY_PAUSE
};

static unsigned int hold_map[]  =
{
    0,0,0,
    KEY_TREBLE_DOWN,
    KEY_BASS_DOWN,
    KEY_VOLUME_DOWN,
    0,0,0,
    KEY_TREBLE_UP,
    KEY_BASS_UP,
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
    release_map  : (const unsigned int*const)release_map,
};


// Function prototypes
extern "C" {
    void cyg_user_start(void);
    void thread_entry(cyg_uint32 data);
};

// This is where execution begins.
void cyg_user_start(void) 
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

// A metadata creation function for the media player.
IMetadata* CreateSimpleMetadata()
{
    return new CSimpleMetadata;
}

void thread_entry(cyg_uint32 data)
{
    DBEN(MAIN);

    // Get a pointer to the play manager singleton.
    CPlayManager* pPlayManager = CPlayManager::GetInstance();

    // Create the content manager and add it to the play manager..
    CSimpleContentManager* pCM = new CSimpleContentManager(0);
    pPlayManager->SetContentManager(pCM);

    // Create the helper class for handling events.
    CEvents* pEvents = new CEvents;

    // Create a simple playlist object and set it as the play manager's current playlist.
    pPlayManager->SetPlaylist(new CSimplePlaylist(""));

    // Initialize the keyboard.
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap(&key_map);

    // Tell the media player to create a CSimpleMetadata object for each stream it sets.
    CMediaPlayer::GetInstance()->SetCreateMetadataFunction(CreateSimpleMetadata);
    CMediaPlayer::GetInstance()->SetPlaystream(&ps);

    // Add the hard drive data source to the play manager.
    DEBUG(MAIN, DBGLEV_INFO, "Opening hard drive\n");
    CFatDataSource* pHDDS = CFatDataSource::Open(0);
    if (pHDDS)
    {
        DEBUG(MAIN, DBGLEV_INFO, "Opened hard drive\n");
        pPlayManager->AddDataSource(pHDDS);
        pEvents->SetSettingsURL("file://a:\\settings.ddo");

        // Begin a content update.
        // Don't bother getting metadata since we don't query content based on it.
        pPlayManager->RefreshAllContent(IDataSource::DSR_ONE_PASS);
    }
    else
    {
        // The hard drive couldn't be opened, so this demo is over.
        DEBUG(MAIN, DBGLEV_INFO, "Couldn't open hard drive\n");
        return;
    }

    // Create a demo user interface.
#if defined(DDOMOD_UI_SIMPLEGUI)
    IUserInterface* pUserInterface = new CSimpleGUIUserInterface;
#elif defined(DDOMOD_UI_SERIAL)
    IUserInterface* pUserInterface = new CSerialUserInterface;
#else
#error No user interface defined
#endif

    // Get a handle to the global event queue.
    CEventQueue* pEventQueue = CEventQueue::GetInstance();

    pUserInterface->SetBanner("iObjects Dadio SDK Demo 1");
    pEvents->SetUserInterface(pUserInterface);

    // Load volume, bass, and treble settings.
    pEvents->LoadState();
    // Update the interface with the new settings.
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
