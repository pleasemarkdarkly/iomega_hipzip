// main.cpp: test harness to bring up main player
//  this is supposed to be a demo of how to perform various functions.
//  it can be used as the basis of your app, just clean out the parts you dont need.
// danc@iobjects.com 07/10/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <_modules.h>

//////////////////////////
// Interfaces to classes we need to set up
//
#include <core/playmanager/PlayManager.h>
#include <util/eventq/EventQueueAPI.h>

//////////////////////////
// Input interfaces
//
#ifdef DDOMOD_DEV_KEYBOARD
#include <devs/keyboard/Keyboard.h>
#define USE_KBD
#endif
#ifdef DDOMOD_DEV_IR
#include <devs/ir/ir.h>
//#define USE_IR
#endif

#include <datastream/waveout/WaveOutKeys.h>

//////////////////////////
// Figure out which data sources we need
//
#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#define USE_CD
#endif
#ifdef DDOMOD_DATASOURCE_FAT
#include <datasource/fatdatasource/FatDataSource.h>
#define USE_HD
//#define USE_CF  // we try to open this, ignore it if it fails
#endif
#ifdef DDOMOD_DATASOURCE_NET
#include <datasource/netdatasource/NetDataSource.h>
//#define USE_NET
#endif


//////////////////////////
// Determine the content mgr
//
#ifdef DDOMOD_CONTENT_METAKITCONTENTMANAGER
#include <content/metakitcontentmanager/MetakitContentManager.h>
#define USE_METAKIT_CM
#endif
#ifdef DDOMOD_CONTENT_SIMPLECONTENTMANAGER
#include <content/simplecontentmanager/SimpleContentManager.h>
#define USE_SIMPLE_CM
#endif

//////////////////////////
// Determine the UI
//
#ifdef DDOMOD_UI_SERIAL
#include <main/ui/serial/SerialUserInterface.h>
#endif
#ifdef DDOMOD_UI_SIMPLEGUI
#include <main/ui/simplegui/SimpleGUIUserInterface.h>
#endif

/////////////////////////
// Any other components we want
//
#include <playlist/simpleplaylist/SimplePlaylist.h>

/////////////////////////
// Basic utilities
//
#include <util/debug/debug.h>
#include <util/tchar/tchar.h>

#include "Events.h"

//////////////////
// Set up a debug module
//
DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);

#ifdef USE_KBD

// Set up a keyboard keymap
// The press_map indicates what event to fire when a button
// is initially pressed
static unsigned int kbd_press_map[] = 
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
// hold_map is the event to fire when a button is being held;
//  this is for repeat events
static unsigned int kbd_hold_map[]  =
{
    0,0,0,0,0,
    KEY_VOLUME_DOWN,
    0,0,0,0,0,
    KEY_VOLUME_UP,
    0,0,0,0
};
// release_map is the event to fire when a button is released
static unsigned int kbd_release_map[]  =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static key_map_t key_map = 
{
#ifdef __DHARMA_V2
    num_columns  :  3,
    num_rows     :  6,
    num_buttons  : 16,
#else
    num_columns  :  8,
    num_rows     :  2,
    num_buttons  : 16,
#endif
    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)kbd_press_map,
    hold_map     : (const unsigned int*const)kbd_hold_map,
    release_map  : kbd_release_map
};

#endif // USE_KBD

#ifdef USE_IR
// Set up the IR map
static unsigned int ir_press_map[] =
{
    KEY_PLAY_PAUSE,
    KEY_STOP,
    KEY_PREVIOUS,
    KEY_NEXT,
    KEY_VOLUME_UP,
    KEY_VOLUME_DOWN,
    KEY_LIST_BY_ARTIST,
    KEY_LIST_BY_GENRE,
    KEY_LIST_BY_ALBUM,
    KEY_LIST_BY_PLAYLIST,
    KEY_PLAYLIST_MODE,
    KEY_REFRESH_CONTENT,
    KEY_SAVE_PLAYLIST,
    KEY_LOAD_PLAYLIST,
    KEY_SAVE_STATE,
    KEY_LOAD_STATE,
    KEY_TREBLE_UP,
    KEY_TREBLE_DOWN,
    KEY_BASS_UP,
    KEY_BASS_DOWN,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
};
static unsigned int ir_hold_map[] =
{
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0,0,0,0,0,
    0,0
};
static ir_map_t ir_map =
{
    num_buttons :  32,
    repeat_flags:  (IR_REPEAT_ENABLE),
    filter_start:   5,
    filter_rate :   3,
    press_map   :  (const unsigned int*const)ir_press_map,
    hold_map    :  (const unsigned int*const)ir_hold_map
};
#endif // USE_IR


//////////////////
// Our thread data
//
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

///////////////////
// C-style functions
//
extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};

///////////////////
// eCos entry point
//
void cyg_user_start( void ) 
{
    DBEN( MAIN );
    
    cyg_thread_create( 9, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
    
    DBEX( MAIN );
}

///////////////////
// main thread
//
void thread_entry( cyg_uint32 data ) 
{
    DBEN( MAIN );

    
#ifdef USE_KBD
    // Set up keyboard
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap( &key_map );
#endif // USE_KBD

#ifdef USE_IR
    // Set up IR interface
    CIR* pIR = CIR::GetInstance();
    pIR->SetIRMap( &ir_map );
#endif // USE_IR


    ////////////////////////
    // Create and configure any general objects we have
    //
    CPlayManager* pPlayManager = CPlayManager::GetInstance();

#ifdef USE_METAKIT_CM
    CMetakitContentManager* pCM = new CMetakitContentManager( );
#elif defined(USE_SIMPLE_CM)
    CSimpleContentManager* pCM = new CSimpleContentManager( );
#else
#error You must build in a content manager
#endif

    pPlayManager->SetContentManager( pCM );
    pPlayManager->SetPlaylist( new CSimplePlaylist("") );

    ////////////////////////
    // Create data sources
    //
#ifdef USE_HD
    DEBUG(MAIN, DBGLEV_INFO, "Opening hard drive\n");
    if (CFatDataSource* pFatDS = CFatDataSource::Open(0))
    {
        pPlayManager->AddDataSource( pFatDS );
        DEBUG(MAIN, DBGLEV_INFO, "Opened hard drive\n");
    }
    else {
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open hard drive\n");
    }
#endif  // USE_HD

#ifdef USE_CD
    DEBUG(MAIN, DBGLEV_INFO, "Opening CD\n");
    if (CCDDataSource* pCDDS = CCDDataSource::Open("/dev/cda/", "/"))
    {
        pPlayManager->AddDataSource( pCDDS );
        DEBUG(MAIN, DBGLEV_INFO, "Opened CD\n");
    }
    else {
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open CD\n");
    }
#endif  // USE_CD

#ifdef USE_NET
    DEBUG(MAIN, DBGLEV_INFO, "Opening net\n");
    if (CNetDataSource* pNetDS = CNetDataSource::Open())
    {
        pPlayManager->AddDataSource( pNetDS );

        pNetDS->GenerateEntry( pCM, "http://192.168.0.50/temp/ElliotSmith-SayYes.mp3" );
        pNetDS->GenerateEntry( pCM, "http://192.168.0.50/temp/Sabotage.wma" );
        DEBUG(MAIN, DBGLEV_INFO, "Opened net data source\n");
    }
    else {
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open net data source\n");
    }
#endif  // USE_NET


    /////////////////
    // Create a UI
    //
#ifdef DDOMOD_UI_SERIAL
    IUserInterface* pUserInterface = new CSerialUserInterface;
#elif defined(DDOMOD_UI_SIMPLEGUI)
    IUserInterface* pUserInterface = new CSimpleGUIUserInterface;
#else
#error No user interface defined
#endif
    pUserInterface->SetBanner("iObjects Dadio SDK");
    

    ////////////////
    // Create an event queue and event handler
    //
    CEventQueue* pEventQueue = CEventQueue::GetInstance();

    CEvents* pEvents = new CEvents;
    pEvents->SetUserInterface( pUserInterface );

    
    ////////////////
    // Force a content refresh
    //
    pPlayManager->RefreshAllContent( );

    ////////////////
    // Pump events for the rest of our life
    do
    {
        unsigned int key;
        void* data;
        pEventQueue->GetEvent(&key, &data);
        pEvents->Event(key, data);
    } while (1);
}
