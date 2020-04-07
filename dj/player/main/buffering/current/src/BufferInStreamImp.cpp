#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/BufferReader.h>
#include <main/buffering/BufferWorker.h>
//#include "DriveInterface.h"
#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h> // memory checking 
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <main/main/FatHelper.h>

#include <main/buffering/DJConfig.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_BUF_ISTREAM, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_ISTREAM);  // debugging prefix : (40) bi
    
// print out a caption and a report on current system memory.
void ReportSystemMemory(char* szCaption);

REGISTER_INPUTSTREAM(CBufferingImp,FATFILE_BUFFEREDINPUT_IMP_ID);

CBufferingImp* CBufferingImp::m_pInstance = 0;

CBufferingImp::CBufferingImp() :
    m_pSimpleStream(NULL),
    m_bSimpleStreamOpen(false),
    m_nSimpleStreamOffset(0),
    m_nCrntStreamId(-1)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:ctor\n");
	// start threads
	m_pReaderMsgPump = new CReaderMsgPump;
	m_pWriterMsgPump = new CWriterMsgPump;
	m_pWriterMsgPump->SetCautiousMode();
    m_pWorker = CBufferFactory::CreateWorker(*(DJInBufferConfig()));

    m_pReaderMsgPump->SetWorker(m_pWorker);
    m_pWriterMsgPump->SetWorker(m_pWorker);
}

CBufferingImp::~CBufferingImp()
{
	DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:dtor\n");
	delete m_pReaderMsgPump;
	delete m_pWriterMsgPump;
    delete m_pWorker;
}
	
// query the playlist to determine what files need to be buffered
void CBufferingImp::NotifyPlaylistOrderChanged(char* szFilename)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:nploc\n");
	m_pWriterMsgPump->PauseWriterThread();
	m_bNearEndOfTrack = false;
    ResumeCautiousWriting();
}

// enter a cautious mode since the decoder is falling behind
void CBufferingImp::NotifyDecodedDataLow()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:nddl\n");
	m_pWriterMsgPump->StopProducing();
	m_pWriterMsgPump->SetCautiousMode();
}

// start refilling blocks since there are few full remaining
void CBufferingImp::NotifyEncodedDataLow() {
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:nedl\n");
	m_pWriterMsgPump->StartProducing();
}

// enter a more aggressive mode since decoding is going well
void CBufferingImp::NotifyDecodedDataHigh()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:nddh\n");
	m_pWriterMsgPump->SetAggressiveMode();
/*  // (epg,10/18/2001): TODO: this was to fill non active caches, but there aren't any now
    if (CBufferWorker::GetInstance()->IsBlockToFill())
		m_pWriterMsgPump->StartProducing();
*/
}

// start filling blocks
void CBufferingImp::ResumeCautiousWriting()
{
    m_pWriterMsgPump->ResumeWriterThread();
	m_pWriterMsgPump->SetCautiousMode();
	m_pWriterMsgPump->StartProducing();
}

// (epg,10/18/2001): TODO: replace stubs
void WakeDrive() { ; }
void SleepDrive() { ; }
bool IsDriveAsleep() { return true; }

bool CBufferingImp::SetSong( IMediaContentRecord* mcr )
{
    // (epg,10/24/2001): this will differ in multifile mode.
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:setsong\n");
    ReportSystemMemory("bi:SS+");
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:current track %s\n",mcr->GetURL()); 
    // pause both active buffering threads in the right order

    // track the current track.  elsewise NotifyStreamSet can short circuit for thinking it is already set to non-current.
    m_nCrntStreamId = mcr->GetID();
    PauseThreads();

    // clean up state for both threads and the document
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:clear read lock\n"); 
    m_pReaderMsgPump->GetReader()->ClearReadBlockLock();

    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:sync doc order\n"); 
    m_pWorker->SyncDocumentOrder(mcr);

    IDataSource* pDataSource = CDataSourceManager::GetInstance()->GetDataSourceByID(mcr->GetDataSourceID()); 
    bool bNetBased = false;
    if (pDataSource)
        bNetBased = (pDataSource->GetClassID() == NET_DATA_SOURCE_CLASS_ID);

    // if we're re-setting a net track, then start over buffering.  the only alternative is to
    // read bytes off the net stream and throw them out, until we catch up to the correct write
    // point.  this is simpler, and not much less efficient.
    if (bNetBased)
    {
        CBufferDocument* doc = (*(m_pWorker->GetActiveDocument()));
        if (!doc->IsSourceOpen() || !doc->WindowAtDocStart() || !doc->SourceOffsetValid())
            doc->InvalidateAndReAnchorWindow(0);
    }
        
    // (epg,5/21/2002): try again in case network track is back from the abyss
    if (bNetBased)
    {
        CBufferDocument* doc = (*(m_pWorker->GetActiveDocument()));
        if (!m_pWorker->ActiveDocValid())
        {
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:resetting net doc as valid\n"); 
            doc->CloseSource();
            doc->Initialize();
            // this will re-mark the file invalid if it fails the open.
            doc->OpenSource();
        }
    }
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:check crnt valid\n"); 
    if (!m_pWorker->ActiveDocValid())
        return false;
    // restart the threads in the correct order
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:res prod\n"); 
    m_pWriterMsgPump->ResumeWriterThread();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:res cons\n"); 
    m_pReaderMsgPump->ResumeReaderThread();
    ReportSystemMemory("bi:SS-");
    return true;
}

extern "C" {
    cyg_int32 get_totalmem();
    cyg_int32 get_freemem();
}

void ReportSystemMemory(char* szCaption)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "Mem %s",szCaption);
    struct mallinfo mem_info;		    
	mem_info = mallinfo();
	DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, " Ttl:0x%08x Used:0x%08x Free:0x%08x Max:0x%08x\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

int CBufferingImp::Position(bool bBuffered)
{
    if (bBuffered)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:buffered pos\n"); 
        return (*m_pWorker->GetActiveDocument())->GetReadByte();
    }
    else
    {
        if (!m_pSimpleStream)
            return -1;
        int nPos = m_pSimpleStream->Position();
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:pt pos %d\n"); 
        m_nSimpleStreamOffset = nPos;
        //m_nSimpleStreamOffset = m_pSimpleStream->Position();
        return m_nSimpleStreamOffset;
    }
}

void CBufferingImp::CloseSimpleStream()
{
    if (m_bSimpleStreamOpen)
    {
        m_pSimpleStream->Close();
        delete m_pSimpleStream;
        m_pSimpleStream = NULL;
        m_bSimpleStreamOpen = false;
    }
}

bool CBufferingImp::OpenSimpleStream(const char* szUrl)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "open simple stream\n"); 
    CloseSimpleStream();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "opening\n"); 
    if (!m_bSimpleStreamOpen)
    {
        m_pSimpleStream = CDataSourceManager::GetInstance()->OpenInputStream(szUrl);
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:Open(s)%d=%s\n",(int)m_pSimpleStream,szUrl); 
        m_bSimpleStreamOpen = (m_pSimpleStream != NULL);
    }
    else
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:SS already open\n"); 
    }
    return m_bSimpleStreamOpen;
}

ERESULT CBufferingImp::Open(const char* szFilename)
{
    DBASSERT( DBG_BUF_ISTREAM, (FALSE), "bi:Open Not Supported\n");
    return INPUTSTREAM_ERROR;
}

void CBufferingImp::NotifyStreamSet(IMediaContentRecord* mcr)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:NotifyStreamSet\n");
    int nStreamId = mcr->GetID();
    if (m_nCrntStreamId == nStreamId)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bi:current already set\n"); 
        return;
    }
    m_nCrntStreamId = nStreamId;
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "stream set to %d\n",nStreamId); 
    if (m_bSimpleStreamOpen && m_nSimpleStreamContentId == nStreamId)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bi:Close(s)%d\n",(int)m_pSimpleStream);  
        CloseSimpleStream();
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "setting song to new stream\n"); 

        SetSong(mcr);
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "need to sync offset to match simple stream state?\n"); 
        if (m_nSimpleStreamOffset != 0)
        {
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "seeking to match simple, offset %d\n",m_nSimpleStreamOffset); 
            Seek(true, SeekStart,m_nSimpleStreamOffset);
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "stream %d in buffered mode\n"); 
        }
        else
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "simple stream still at zero offset\n"); 
        
    }
    else
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "simple stream open? %s\n", m_bSimpleStreamOpen ? "true" : "false"); 
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "simple stream id matches? %s\n", (m_nSimpleStreamContentId == nStreamId) ? "yes" : "no"); 
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "ssc id %d\n",m_nSimpleStreamContentId); 
        SetSong(mcr);
    }
}

ERESULT CBufferingImp::Close(bool bBuffered)
{
    if (bBuffered)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bi:closed buffered\n"); 
        return INPUTSTREAM_NO_ERROR;
    }
    else if (m_bSimpleStreamOpen)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bi:closing simple stream\n"); 
        m_bSimpleStreamOpen = false;
        m_nSimpleStreamOffset = 0;
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bi:Close(s)%d\n",(int)m_pSimpleStream);  
        CloseSimpleStream();
    }
    else
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE , "bi:already closed\n"); 
    }
    return INPUTSTREAM_NO_ERROR;
}

void CBufferingImp::NotifyCDRemoved()
{
    bool bDetach = true;
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:notify cd removed\n"); 
    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    if (pl)
    {
        IPlaylistEntry* entry = pl->GetCurrentEntry();
        if (entry)
        {
            int id = entry->GetContentRecord()->GetDataSourceID();
            DBASSERT( DBG_BUF_ISTREAM, ( id > 0 ) , "bi:null ds id\n"); 
            IDataSource* ds = CDataSourceManager::GetInstance()->GetDataSourceByID(id);
            DBASSERT( DBG_BUF_ISTREAM, ( ds != NULL ) , "bi:null ds from id %d\n",id); 
            int classid = ds->GetClassID();
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:active datasource class id %d\n",classid); 
            if (classid != CD_DATA_SOURCE_CLASS_ID)
                bDetach = false;
        }
    }
    if (bDetach)
    {
        DetachFromPlaylist();
    }
    else
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_WARNING, "bi:ignoring cd removed, cd not active.\n"); 
}

void CBufferingImp::PauseThreads()
{
   // pause both active buffering threads (order dependant)
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:pause cons\n");
    m_pReaderMsgPump->PauseReaderThread();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:pause prod\n"); 
    m_pWriterMsgPump->PauseWriterThread();
}

void CBufferingImp::ResumeThreads()
{
    // restart the threads in reverse order of pausing
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:res prod\n"); 
    m_pWriterMsgPump->ResumeWriterThread();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:res cons\n"); 
    m_pReaderMsgPump->ResumeReaderThread();
}

void CBufferingImp::ResyncWithPlaylist()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:resync\n");
    PauseThreads();
    if (m_pSimpleStream != NULL && m_bSimpleStreamOpen && m_pSimpleStream->CanSeek())
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "BI:closing seekable simple stream\n"); 
        CloseSimpleStream();
    }
    m_pWorker->ResyncDocumentOrder();
    ResumeThreads();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:resync done\n"); 
}

// close and detach from all sources
void CBufferingImp::DetachFromPlaylist()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:detaching\n");
    PauseThreads();

    // release the 
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:clr BRL\n"); 
    m_pReaderMsgPump->GetReader()->ClearReadBlockLock();

    m_pWorker->DetachAllSources();
    
    ResumeThreads();
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bi:detach done\n"); 

}

bool CBufferingImp::CanSeek(bool bBuffered)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:CanSeek?\n"); 

    // (epg,9/13/2002): can't rely on current playlist entry in this context.
    if (bBuffered)
    {
        DocumentListIterator it = m_pWorker->GetActiveDocument();
        if (it != NULL)
        {
            char* url = (*it)->GetSourceUrl();
            if (url != NULL)
            {
                int dsid = CDataSourceManager::GetInstance()->GetDataSourceByURL(url)->GetClassID();
                if (dsid == NET_DATA_SOURCE_CLASS_ID)
                    return false;
                else
                    return true;
            }
        }
        return false;
    }
    else
    {
        if (m_pSimpleStream)
            return m_pSimpleStream->CanSeek();
        return false;
    }
}

int CBufferingImp::Length(bool bBuffered) const 
{ 
    int nRet;
    if (bBuffered)
    {
        nRet = (*m_pWorker->GetActiveDocument())->Length(); 
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:buffered len is %d\n",nRet); 
        return nRet;
    }
    else
    {
        if (m_pSimpleStream && m_bSimpleStreamOpen)
        {
            nRet = m_pSimpleStream->Length();
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bi:straight len is %d\n",nRet); 
            return nRet; 
        }
        else
            DBASSERT( DBG_BUF_ISTREAM, ( 1<0 ) , "bi:simple stream not open in Length\n"); 
    }
    return 0; // Make the compiler happy
}

void CBufferingImp::PauseHDAccess()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bi:pauseHD\n"); 
    m_pWriterMsgPump->PauseWriterThread();
    m_pWorker->CloseSeekableSources();
}

void CBufferingImp::ResumeHDAccess()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bi:resumeHD\n"); 
    m_pWriterMsgPump->ResumeWriterThread();
}

void CBufferingImp::SetPrebufDefault(int nSeconds)
{
    return GetReaderMsgPump()->GetReader()->SetPrebufDefault(nSeconds);
}

int CBufferingImp::GetPrebufDefault()
{
    return GetReaderMsgPump()->GetReader()->GetPrebufDefault();
}

// puse threads, close all files, and then resume threads.  then if eg fml files go away, they won't reopen.
void CBufferingImp::CloseFiles()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bi:closing files\n"); 
    PauseThreads();
    m_pWorker->CloseAllSources();
    ResumeThreads();
}

int CBufferingImp::Seek(bool bBuffered, eInputSeekPos Origin, int Offset)
{
    if (bBuffered)
        return m_pReaderMsgPump->Seek(Origin,Offset);
    else
    {
        m_nSimpleStreamOffset = m_pSimpleStream->Seek((IInputStream::InputSeekPos)Origin,Offset);
        return m_nSimpleStreamOffset;
    }
}

int CBufferingImp::Read(bool bBuffered, void* pBlock, int dwBytes)
{
    if (bBuffered)
	    return m_pReaderMsgPump->Read(pBlock,dwBytes);
    else
    {
        int nRead = m_pSimpleStream->Read(pBlock,dwBytes);
        m_nSimpleStreamOffset += nRead;
        return nRead;
    }
}

void CBufferingImp::SetPrebufCallback(SetIntFn* pfn)
{
    return m_pReaderMsgPump->GetReader()->SetPrebufCallback(pfn);
}
