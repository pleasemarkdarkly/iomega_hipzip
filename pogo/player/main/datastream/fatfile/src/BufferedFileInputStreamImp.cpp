#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
//#include "DriveInterface.h"
#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <cyg/kernel/kapi.h>
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>

#include <datastream/fatfile/FileInputStream.h>

extern "C" void* cyg_libc_get_malloc_pool(void);

REGISTER_INPUTSTREAM(CBufferedFatFileInputStreamImp,FATFILE_BUFFEREDINPUT_IMP_ID);

CBufferedFatFileInputStreamImp* CBufferedFatFileInputStreamImp::m_pInstance = 0;

CBufferedFatFileInputStreamImp::CBufferedFatFileInputStreamImp()
{
	// set up the cache area
	m_pCache = new CFileCache(CACHE_SIZE);
	m_pCache->m_aBuffers = new unsigned char*[BUFFER_COUNT];
	for (int k = 0; k < BUFFER_COUNT; k++)
		m_pCache->m_aBuffers[k] = m_BufferSpace + k * BUFFER_SIZE;
	// start threads
	m_pConsumerMsgPump = new CConsumerMsgPump;
	m_pProducerMsgPump = new CProducerMsgPump;
	m_pProducerMsgPump->SetCautiousMode();
	m_pCacheMan = CCacheMan::GetInstance();
}

CBufferedFatFileInputStreamImp::~CBufferedFatFileInputStreamImp()
{
	delete [] m_pCache->m_aBuffers;	
	delete m_pCache;
	delete m_pConsumerMsgPump;
	delete m_pProducerMsgPump;
	m_pCacheMan->Destroy();
}

CBufferedFatFileInputStreamImp* CBufferedFatFileInputStreamImp::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CBufferedFatFileInputStreamImp;
	return m_pInstance;
}

void CBufferedFatFileInputStreamImp::Destroy()
{
	delete m_pInstance;
}

CFileCache* CBufferedFatFileInputStreamImp::GetCache()
{
	return m_pCache;
}

int CBufferedFatFileInputStreamImp::SetActiveFile(IPlaylistEntry* pEntry)
{
	NotifyPlaylistOrderChanged(pEntry);
    return 0;
}
	
int CBufferedFatFileInputStreamImp::Seek(InputSeekPos Origin, int Offset)
{
	return m_pConsumerMsgPump->Seek(Origin,Offset);
}

int CBufferedFatFileInputStreamImp::Read(void* pBuffer,int dwBytes)
{
	return m_pConsumerMsgPump->Read(pBuffer,dwBytes);
}

// query the playlist to determine what files need to be buffered
void CBufferedFatFileInputStreamImp::NotifyPlaylistOrderChanged(IPlaylistEntry* pCurrentEntry)
{
	m_pProducerMsgPump->PauseProducerThread();
    IPlaylist* pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
	bool bNewCurrent = true;
	m_bNearEndOfTrack = false;

    if (m_pCache->m_bValid)
		m_pCache->UnloadFile();

    ResumeCautiousBuffering();
}

// enter a cautious mode since the decoder is falling behind
void CBufferedFatFileInputStreamImp::NotifyDecodedDataLow()
{
	m_pProducerMsgPump->ExitFillMode();
	m_pProducerMsgPump->SetCautiousMode();
}

// start refilling buffers since there are few full remaining
void CBufferedFatFileInputStreamImp::NotifyEncodedDataLow() {
	m_pProducerMsgPump->StartProducing();
}

// enter a more aggressive mode since decoding is going well
void CBufferedFatFileInputStreamImp::NotifyDecodedDataHigh()
{
	m_pProducerMsgPump->SetAggressiveMode();
/*  // (epg,10/18/2001): TODO: this was to fill non active caches, but there aren't any now
    if (CCacheMan::GetInstance()->IsBufferToFill())
		m_pProducerMsgPump->StartProducing();
*/
}

// start filling buffers
void CBufferedFatFileInputStreamImp::ResumeCautiousBuffering()
{
    ResumeCautiousBuffering();
}

ERESULT CBufferedFatFileInputStreamImp::Close()
{
    return INPUTSTREAM_NO_ERROR;
}

int CBufferedFatFileInputStreamImp::Position() const
{
    return m_pCache->m_nFileBookmark;
}

int CBufferedFatFileInputStreamImp::Ioctl(int, void*)
{ return 0; }

int CBufferedFatFileInputStreamImp::Length() const 
{ 
    return m_pCache->m_pFile->Length(); 
}

// (epg,10/18/2001): TODO: replace stubs
void WakeDrive() { ; }
void SleepDrive() { ; }
bool IsDriveAsleep() { return true; }

// (epg,10/19/2001): empty stub to avoid a compiler warning
void NoOp() { return; }

