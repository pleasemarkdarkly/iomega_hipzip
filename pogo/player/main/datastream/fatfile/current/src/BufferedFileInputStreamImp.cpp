#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/Consumer.h>
//#include "DriveInterface.h"
#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h> // memory checking 
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datasource/datasourcemanager/DataSourceManager.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_BUF_FIS_IMP, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_BUF_FIS_IMP);
    
// print out a caption and a report on current system memory.
void ReportSystemMemory(const char* szCaption);

REGISTER_INPUTSTREAM(CBufferedFatFileInputStreamImp,FATFILE_BUFFEREDINPUT_IMP_ID);

CBufferedFatFileInputStreamImp* CBufferedFatFileInputStreamImp::m_pInstance = 0;

CBufferedFatFileInputStreamImp::CBufferedFatFileInputStreamImp()
{
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:ctor\n");
	// start threads
	m_pConsumerMsgPump = new CConsumerMsgPump;
	m_pProducerMsgPump = new CProducerMsgPump;
	m_pProducerMsgPump->SetCautiousMode();
	m_pCacheMan = CCacheMan::GetInstance();
}

CBufferedFatFileInputStreamImp::~CBufferedFatFileInputStreamImp()
{
	DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:dtor\n");
	delete m_pConsumerMsgPump;
	delete m_pProducerMsgPump;
	m_pCacheMan->Destroy();
}
	
// query the playlist to determine what files need to be buffered
void CBufferedFatFileInputStreamImp::NotifyPlaylistOrderChanged(const char* szFilename)
{
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:nploc\n");
	m_pProducerMsgPump->PauseProducerThread();
	m_bNearEndOfTrack = false;
    // (epg,10/25/2001): TODO: incorrect under cfb
    ResumeCautiousBuffering();
}

// enter a cautious mode since the decoder is falling behind
void CBufferedFatFileInputStreamImp::NotifyDecodedDataLow()
{
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:nddl\n");
	m_pProducerMsgPump->StopProducing();
	m_pProducerMsgPump->SetCautiousMode();
}

// start refilling buffers since there are few full remaining
void CBufferedFatFileInputStreamImp::NotifyEncodedDataLow() {
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:nedl\n");
	m_pProducerMsgPump->StartProducing();
}

// enter a more aggressive mode since decoding is going well
void CBufferedFatFileInputStreamImp::NotifyDecodedDataHigh()
{
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:nddh\n");
	m_pProducerMsgPump->SetAggressiveMode();
/*  // (epg,10/18/2001): TODO: this was to fill non active caches, but there aren't any now
    if (CCacheMan::GetInstance()->IsBufferToFill())
		m_pProducerMsgPump->StartProducing();
*/
}

// start filling buffers
void CBufferedFatFileInputStreamImp::ResumeCautiousBuffering()
{
    m_pProducerMsgPump->ResumeProducerThread();
	m_pProducerMsgPump->SetCautiousMode();
	m_pProducerMsgPump->StartProducing();
}


// (epg,10/18/2001): TODO: replace stubs
void WakeDrive() { ; }
void SleepDrive() { ; }
bool IsDriveAsleep() { return true; }


bool CBufferedFatFileInputStreamImp::SetSong( IMediaContentRecord* mcr )
{
    // (epg,10/24/2001): this will differ in multifile mode.
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_TRACE, "bis:setsong\n");
    ReportSystemMemory("Enter bis:setsong");
    // pause both active buffering threads in the right order
     DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "pause cons\n");
    m_pConsumerMsgPump->PauseConsumerThread();
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "pause prod\n"); 
    m_pProducerMsgPump->PauseProducerThread();
    // clean up state for both threads and the cache
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "clr BRL\n"); 
    m_pConsumerMsgPump->GetConsumer()->ClearReadBufferLock();
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "sync caches\n"); 
    CCacheMan::GetInstance()->SyncCachesToPlaylistOrder(CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry());
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "check crnt valid\n"); 
    if (!CCacheMan::GetInstance()->IsCurrentCacheValid())
        return false;
    // (epg,10/25/2001): TODO: replace removed IS creation
    // restart the threads in the correct order
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "res prod\n"); 
    m_pProducerMsgPump->ResumeProducerThread();
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "res cons\n"); 
    m_pConsumerMsgPump->ResumeConsumerThread();
    ReportSystemMemory("Exit bis:setsong");
    return true;
}

extern "C" {
    cyg_int32 get_totalmem();
    cyg_int32 get_freemem();
}

void ReportSystemMemory(const char* szCaption)
{
    DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, "Mem at %s :",szCaption);
    struct mallinfo mem_info;		    
	mem_info = mallinfo();
	DEBUGP( DBG_BUF_FIS_IMP, DBGLEV_INFO, " (Ttl 0x%08x, Used 0x%08x, Free=0x%08x, Max=0x%08x)\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

