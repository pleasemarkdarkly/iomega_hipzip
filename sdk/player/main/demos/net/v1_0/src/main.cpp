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
// This is the third demo of the Dadio (TM) SDK.  It demonstrates simple
// playback from internet streams.  Controls are provided for adjusting
// volume, bass, and treble.
//

#include <pkgconf/system.h>
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
#include <datasource/netdatasource/NetDataSource.h>
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
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

//
// Keymaps
//

// The list of keys to send on 'press' events
static unsigned int press_map[] =
{
    // buttons S101-S106
    0,0,0,
    KEY_TREBLE_DOWN,
    KEY_BASS_DOWN,
    KEY_VOLUME_DOWN,
    // buttons S109-S114
    0,
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
    release_map  : release_map,
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

    // Create a simple playlist object and set it as the play manager's current playlist.
    pPlayManager->SetPlaylist(new CSimplePlaylist(""));

    // Initialize the keyboard.
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap(&key_map);

    // Add the net data sources to the play manager.
    DEBUG(MAIN, DBGLEV_INFO, "Opening net data source interface 0\n");
    CNetDataSource* pNet0DS = CNetDataSource::Open(0, false);
    if (pNet0DS)
    {
        DEBUG(MAIN, DBGLEV_INFO, "Opened net data source interface 0\n");
        pPlayManager->AddDataSource(pNet0DS);
    }
    else
        DEBUG(MAIN, DBGLEV_WARNING, "Couldn't open net data source interface 0\n");
    
    DEBUG(MAIN, DBGLEV_INFO, "Opening net data source interface 1\n");

    CNetDataSource* pNet1DS = CNetDataSource::Open(1, true);
    if (pNet1DS) {
        DEBUG(MAIN, DBGLEV_INFO, "Opened net data source interface 1\n");
        pPlayManager->AddDataSource( pNet1DS );
    }
    else
        DEBUG(MAIN, DBGLEV_WARNING, "Couldn't open net data source interface 1\n");

    // Check whether we have some sort of network connection
    if (!(pNet0DS || pNet1DS))
    {
        // No net data source could be opened, so this demo is over.
        DEBUG(MAIN, DBGLEV_ERROR, "Couldn't open a net data source\n");
        return;
    }

    // Add entries specific to each interface first
    if (pNet0DS)
    {
    }
    if (pNet1DS)
    {
        pNet1DS->GenerateEntry(pCM, "http://169.254.5.47:8000/");
    }
    
    // Add common entries, preference is given to pNet0DS
    CNetDataSource* pNetDS = (pNet0DS ? pNet0DS : pNet1DS);
    
    // http://www.digitallyimported.com 128kbps mp3 stream
    pNetDS->GenerateEntry(pCM, "http://205.188.234.66:8004/");

    // http://www.wolffm.com 128kbps mp3 stream
    pNetDS->GenerateEntry(pCM, "http://205.188.234.37:8044/");

    // http://www.mostlyclassical.com 128kbps mp3 stream
    pNetDS->GenerateEntry(pCM, "http://66.9.105.2:8400/");

    // http://radiostorm.com 128kbps mp3 stream
    pNetDS->GenerateEntry(pCM, "http://66.28.68.37:8088/");

    // http://www.monkeyradio.com 128kbps mp3 stream
    pNetDS->GenerateEntry(pCM, "http://scastsrv2.shoutcast.com:8038/");

    // Wait for the network to finish initialization.
    while (!pNetDS->IsInitialized())
        cyg_thread_delay(100);

    // Tell the media player to create a CSimpleMetadata object for each stream it sets.
    CMediaPlayer::GetInstance()->SetCreateMetadataFunction(CreateSimpleMetadata);
    CMediaPlayer::GetInstance()->SetPlaystream(&ps);

    // Create a playlist of all the net entries generated above.
    MediaRecordList records;
    pCM->GetAllMediaRecords(records);
    pPlayManager->GetPlaylist()->AddEntries(records);
    if (FAILED(pPlayManager->Play()))
    {
        if (SUCCEEDED(pPlayManager->NextTrack()))
            pPlayManager->Play();
        else
        {
            // No streams could be opened, so this demo is over.
            DEBUG(MAIN, DBGLEV_ERROR, "Couldn't open a network stream - check connection?\n");
            return;
        }
    }

    // Create a demo user interface.
#if defined(DDOMOD_UI_SIMPLEGUI)
    IUserInterface* pUserInterface = new CSimpleGUIUserInterface;
#elif defined(DDOMOD_UI_SERIAL)
    IUserInterface* pUserInterface = new CSerialUserInterface;
#else
#error No user interface defined
#endif

    pUserInterface->SetBanner("iObjects Dadio SDK Demo 3");
    
    // Create the helper class for handling events.
    CEvents* pEvents = new CEvents;
    pEvents->SetUserInterface(pUserInterface);
    pEvents->RefreshInterface();
    
    // Get a handle to the global event queue.
    CEventQueue* pEventQueue = CEventQueue::GetInstance();

    // Enter the main event loop.
    do
    {
        unsigned int key;
        void* data;
        pEventQueue->GetEvent(&key, &data);
        pEvents->Event(key, data);

    } while (1);
}
