// main.cpp: test harness to bring up main player
// danc@iobjects.com 07/10/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <_modules.h>

// Interfaces to classes we need to set up
#include <util/eventq/EventQueueAPI.h>
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <devs/keyboard/Keyboard.h>
#include <main/demos/ssi_neo/ui/Keys.h>

// Keys for classes that we want to have loaded
#include <datastream/outfilter/OutFilterKeys.h>
#include <datastream/waveout/WaveOutKeys.h>

// Interfaces to components we instantiate
#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#define USE_CD
#endif

#ifdef DDOMOD_DATASOURCE_FAT
#include <datasource/fatdatasource/FatDataSource.h>
//#define USE_CF
#define USE_HD
#endif

#ifdef DDOMOD_DATASOURCE_NET
#include <datasource/netdatasource/NetDataSource.h>
//#define USE_NET
#endif

#include <playlist/simpleplaylist/SimplePlaylist.h>

#ifdef DDOMOD_CONTENT_CONFIGURABLECONTENTMANAGER
#include <content/configurablecontentmanager/ConfigurableContentManager.h>
#endif
#ifdef DDOMOD_CONTENT_METAKITCONTENTMANAGER
#include <content/metakitcontentmanager/MetakitContentManager.h>
#endif
#ifdef DDOMOD_CONTENT_SIMPLECONTENTMANAGER
#include <content/simplecontentmanager/SimpleContentManager.h>
#endif

#include <main/demos/ssi_neo/ui/PEGUserInterface.h>

#ifdef DDOMOD_GUI_PEG
#include <main/demos/ssi_neo/ui/UI.h>
void PegAppInitialize(PegPresentationManager *pPresentation);

#endif

#include <util/debug/debug.h>
#include <util/tchar/tchar.h>

#include "Events.h"

DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);

// thread data etc
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];
IUserInterface* g_pUserInterface;
CEventQueue* g_pEventQueue;
CEvents* g_pEvents;

#define __DHARMA_KBD_MAP 1

// keymap
#if __DHARMA_KBD_MAP
static unsigned int _press_map[] = 
{
    KEY_MENU,
    KEY_BROWSE,
    KEY_RECORD,
    0,0,0,
    KEY_DOWN,
    KEY_UP,
    KEY_PREVIOUS,
    KEY_NEXT,
    KEY_STOP,
    KEY_PLAY_PAUSE,
    KEY_REFRESH_CONTENT,
    KEY_LIST_BY_ARTIST,
    KEY_LIST_BY_GENRE,
    KEY_LIST_BY_ALBUM
};
static unsigned int _hold_map[]  =
{
    0,
    0,
    0,
    0,0,0,
    KEY_DOWN,
    KEY_UP,
    KEY_PREVIOUS,
    KEY_NEXT,
    0,
    0,
    0,
    0,
    0,
    0
};
static unsigned int _release_map[]  =
{
    0,0,0,0,0,0,0,0,
    KEY_PREVIOUS,
    KEY_NEXT,
    0,0,0,0,0,0
};

#else // __DHARMA_KBD_MAP
// keymap
static unsigned int _press_map[] = 
{
    KEY_BROWSE,
    KEY_DOWN,
    KEY_STOP,
    KEY_PREVIOUS,
    KEY_PLAY_PAUSE,
    KEY_NEXT,
    KEY_RECORD,
    KEY_UP,
    KEY_MENU,
    0,0,0,0,0,0,0
};
static unsigned int _hold_map[]  =
{
    0,
    KEY_DOWN,
    0,
    KEY_PREVIOUS,
    0,
    KEY_NEXT,
    0,
    KEY_UP,
    0,0,0,0,0,0,0,0
};
static unsigned int _release_map[]  =
{
    0,0,0,
    KEY_PREVIOUS,
    0,
    KEY_NEXT,
    0,0,0,0,0,0,0,0,0,0
};
#endif // __DHARMA_KBD_MAP

static key_map_t key_map = 
{
    num_columns  :  8,
    num_rows     :  2,
    num_buttons  : 16,

    repeat_flags : (KEY_REPEAT_ENABLE),
    
    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,
    
    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};


// function prototypes
extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};
void main_event_loop();

void cyg_user_start( void ) 
{
    DBEN( MAIN );
    
    cyg_thread_create( 10, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
    
    DBEX( MAIN );
}

const playstream_settings_t ps = 
{
    WAVEOUT_KEY,
    0,//    filter_list,
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

#include <content/simplemetadata/SimpleMetadata.h>
IMetadata* CreateSimpleMetadata()
{
    return new CSimpleMetadata;
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

	pScreen->Invalidate(present.mReal);
	PegMessage NewMessage(PM_DRAW);
	NewMessage.pTarget = &present;
	pMsgQueue->Push(NewMessage);


    
    CKeyboard* pK = CKeyboard::GetInstance();
    pK->SetKeymap( &key_map );

    CPlayManager* pPlayManager = CPlayManager::GetInstance();

#ifdef USE_CF
    DEBUG(MAIN, DBGLEV_INFO, "Opening cf card\n");
    if (CFatDataSource* pFat2DS = CFatDataSource::Open(1))
    {
        pFat2DS->SetDefaultRefreshMode(IDataSource::DSR_TWO_PASS);
        pPlayManager->AddDataSource( pFat2DS );
        DEBUG(MAIN, DBGLEV_INFO, "Opened cf card\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open cf card\n");
#endif  // USE_CF

#ifdef USE_HD
    DEBUG(MAIN, DBGLEV_INFO, "Opening hard drive\n");
    if (CFatDataSource* pFatDS = CFatDataSource::Open(0))
    {
        pFatDS->SetDefaultRefreshMode(IDataSource::DSR_TWO_PASS);
        pPlayManager->AddDataSource( pFatDS );
        DEBUG(MAIN, DBGLEV_INFO, "Opened hard drive\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open hard drive\n");
#endif  // USE_HD

#ifdef USE_CD
    DEBUG(MAIN, DBGLEV_INFO, "Opening CD\n");
    if (CCDDataSource* pCDDS = CCDDataSource::Open("/dev/cda/", "/"))
    {
        pPlayManager->AddDataSource( pCDDS );
        DEBUG(MAIN, DBGLEV_INFO, "Opened CD\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open CD\n");
#endif  // USE_CD

#ifdef USE_NET
    DEBUG(MAIN, DBGLEV_INFO, "Opening net data source\n");
    if (CNetDataSource* pNetDS = CNetDataSource::Open())
    {
        pPlayManager->AddDataSource( pNetDS );

        pNetDS->GenerateEntry( pCM, "http://192.168.0.50/temp/ElliotSmith-SayYes.mp3" );
        pNetDS->GenerateEntry( pCM, "http://192.168.0.50/temp/Sabotage.wma" );
        DEBUG(MAIN, DBGLEV_INFO, "Opened net data source\n");
    }
    else
        DEBUG(MAIN, DBGLEV_ERROR, "Failed to open net data source\n");
#endif  // USE_NET

#ifdef DDOMOD_CONTENT_METAKITCONTENTMANAGER
#if INCREMENTAL_METAKIT
    CMetakitContentManager* pCM = new CMetakitContentManager("a:/mkpersist.dat");
#else
    CMetakitContentManager* pCM = new CMetakitContentManager( );
#endif
#endif
#ifdef DDOMOD_CONTENT_CONFIGURABLECONTENTMANAGER

#if INCREMENTAL_METAKIT
    CConfigurableContentManager* pCM = new CConfigurableContentManager("a:/mkpersist.dat");
#else
    CConfigurableContentManager* pCM = new CConfigurableContentManager( );
#endif
    pCM->AddStoredMetadata(MDA_ALBUM_TRACK_NUMBER, (void*)0);
    pCM->AddStoredMetadata(MDA_COMMENT, (void*)"");
    pCM->AddStoredMetadata(MDA_HAS_DRM, (void*)2);
#endif
#ifdef DDOMOD_CONTENT_SIMPLECONTENTMANAGER
    CSimpleContentManager* pCM = new CSimpleContentManager( );
#endif

    pPlayManager->SetContentManager( pCM );
    pPlayManager->SetPlaylist( new CSimplePlaylist("") );

    CMediaPlayer::GetInstance()->SetPlaystream( &ps );
    CMediaPlayer::GetInstance()->SetCreateMetadataFunction( CreateSimpleMetadata );

    g_pUserInterface = new CPEGUserInterface;
    g_pUserInterface->SetBanner("SDK SSI Neo");
    g_pEventQueue = CEventQueue::GetInstance();
    g_pEvents = new CEvents;
    g_pEvents->SetUserInterface( g_pUserInterface );
    g_pEvents->LoadState();
    g_pEvents->RefreshInterface();

   	present.Execute();
}


void PegIdleFunction()
{
    unsigned int key;
    void* data;
    PegThing* pt = 0;
    if (g_pEventQueue->TimedGetEvent( &key, &data, 10))
        g_pEvents->Event(key, data);
	pt->MessageQueue()->TimerTick();
}