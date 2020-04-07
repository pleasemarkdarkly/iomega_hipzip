// FileInputStream.cpp: file input wrapper for CFatFile
// danc@fullplaymedia.com 07/09/01
// (c) Fullplay Media Systems

#include <main/buffering/BufferInStream.h>
#include <main/buffering/BufferInStreamImp.h>
#include <datastream/fatfile/FatFile.h>
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>
#include <main/buffering/BufferDebug.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <new.h>    // placement new

#include <util/debug/debug.h>          // debugging hooks
DEBUG_USE_MODULE(DBG_BUF_ISTREAM);  // debugging prefix : (65) bss
REGISTER_INPUTSTREAM(CBufferingRef,FATFILE_BUFFEREDINPUT_ID);

CBuffering* CBuffering::m_pInstance = 0;

CBuffering* CBuffering::GetInstance()
{
    if (!m_pInstance)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:create\n"); 
        m_pInstance = new CBuffering();
    }
    return m_pInstance;
}

void CBuffering::Destroy()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:destroy\n"); 
    delete m_pInstance;
    m_pInstance = NULL;
}

CBuffering::CBuffering() : m_nCurrentContentId(0)
{
    // (epg,5/17/2002): todo: imp can be non-singleton, this has moved up to this class with the addition of the bufferingRef.
    // remove this linkage in favor of straight hierarchy.
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:bufferingRef  switcher ctor\n"); 
    m_pImp = CBufferingImp::GetInstance();
}

bool CBuffering::IsContentBuffered(int nContentId)
{
#if DEBUG_LEVEL > 0
    if (nContentId != m_nCurrentContentId && nContentId != m_nNextContentId)
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "BSS:checking unknown id %d\n",nContentId); 
#endif
    return (nContentId == m_nCurrentContentId);
}


CBuffering::~CBuffering() 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:dtor\n"); 
}

ERESULT CBuffering::Open( int nContentId, const char* Source ) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bis:open\n"); 
    return ENOERR;
}

ERESULT CBuffering::Close(int nContentId) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bss:close\n"); 
    return m_pImp->Close(IsContentBuffered(nContentId));
}

int CBuffering::Read( int nContentId, void* Block, int Count ) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bis:read\n"); 
    return m_pImp->Read(IsContentBuffered(nContentId),Block,Count);
}

int CBuffering::Ioctl( int nContentId, int Key, void* Value )
{
    return ENOERR;
}

int CBuffering::Seek( int nContentId, eInputSeekPos SeekOrigin, int Offset ) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bis:sk\n"); 
    return m_pImp->Seek(IsContentBuffered(nContentId),SeekOrigin, Offset);
}

int CBuffering::Length(int nContentId) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bis:len\n"); 
    return m_pImp->Length(IsContentBuffered(nContentId));
}

int CBuffering::Position(int nContentId) 
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bis:pos\n"); 
    return m_pImp->Position(IsContentBuffered(nContentId));
}

// decide which stream object is less busy, and return it.
IInputStream* CBuffering::CreateInputStream( IMediaContentRecord* mcr )
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bis:create input stream\n"); 

    IPlaylist* pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    IPlaylistEntry* pCrntEntry = pPlaylist->GetCurrentEntry();
    DBASSERT( DBG_BUF_ISTREAM, pCrntEntry!=NULL, "BIS:NoCrntPlaylistEntry\n"); 

    int nStreamId = mcr->GetID();

    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "stream id %d\n",nStreamId); 
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "stream name %s\n",mcr->GetURL()); 

    int nCrntId = pCrntEntry->GetContentRecord()->GetID();
    m_nCurrentContentId = nCrntId;


    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "crnt id %d\n",nCrntId); 
    IPlaylistEntry* pNextEntry = pPlaylist->GetNextEntry(pCrntEntry,CPlayManager::GetInstance()->GetPlaylistMode());
    int nNextId;
    if (pNextEntry)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "get next id\n"); 
        nNextId = pNextEntry->GetContentRecord()->GetID();
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "next id %d\n",nNextId); 
    }
    else
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "no next entry\n"); 
        nNextId = 0;
    }
    m_nNextContentId = nNextId;

    CBufferingRef* pReference = NULL;
    if (nStreamId == nCrntId)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "crnt requested, setsong\n"); 
        if (SetSong(mcr))
        {
            m_nCurrentContentId = nCrntId;
            
            pReference = new CBufferingRef;
            pReference->m_nContentId = nStreamId;
        }
        else
        {
            m_nCurrentContentId = -1;
        }
    }
    else if (nStreamId == nNextId)
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bss:opening simple stream\n"); 
        if (m_pImp->OpenSimpleStream(pNextEntry->GetContentRecord()->GetURL()))
        {
            pReference = new CBufferingRef;
            pReference->m_nContentId = nStreamId;
        }
        else
        {
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bss:open ss failed\n"); 
        }
    }
    else
    {
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bss:unknown id requested %d\n",nStreamId); 
    }
    m_nNextContentId = nNextId;    

    return pReference;
}

bool CBuffering::SetSong( IMediaContentRecord* mcr )
{
    return m_pImp->SetSong(mcr);
}

void CBuffering::NotifyStreamAutoset()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bss:+NotStrmAset\n"); 
    IPlaylist *pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pPlaylist)
    {
        IPlaylistEntry *pCurrentEntry = pPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IPlaylistEntry* pNextEntry = pPlaylist->GetNextEntry(pCurrentEntry, CPlayManager::GetInstance()->GetPlaylistMode());

            if (pNextEntry)
	            NotifyStreamSet(pNextEntry->GetContentRecord());
            else
                DEBUGP( DBG_BUF_ISTREAM, DBGLEV_WARNING, "BSS:couldn't get next in autoset\n"); 
        }
        else
            DEBUGP( DBG_BUF_ISTREAM, DBGLEV_WARNING, "BSS:couldn't get current in autoset\n"); 
    }
    else
        DEBUGP( DBG_BUF_ISTREAM, DBGLEV_WARNING, "BSS:couldn't get playlist\n"); 
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bss:-NotStrmAset\n"); 
}

void CBuffering::NotifyStreamSet(IMediaContentRecord* mcr)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_HIGH_LEVEL_TRACE, "bss:NotStrmSet\n"); 
    m_nCurrentContentId = mcr->GetID();
    m_nNextContentId = 0;
    m_pImp->NotifyStreamSet(mcr);
}

void CBuffering::NotifyCDRemoved()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:notify cd removed\n"); 
    m_pImp->NotifyCDRemoved();
}


void CBuffering::ResyncWithPlaylist()
{
    m_pImp->ResyncWithPlaylist();
}

// close and detach from all sources
void CBuffering::DetachFromPlaylist()
{
    return m_pImp->DetachFromPlaylist();    
}

bool CBuffering::CanSeek(int nContentId)
{
    return m_pImp->CanSeek(IsContentBuffered(nContentId));
}

void CBuffering::PauseHDAccess()
{
    return m_pImp->PauseHDAccess();
}

void CBuffering::ResumeHDAccess()
{
    return m_pImp->ResumeHDAccess();
}

void CBuffering::Shutdown()
{
    delete m_pImp;
    m_pImp = NULL;
}

void CBuffering::Restart()
{
    DBASSERT( DBG_BUF_ISTREAM, ( m_pImp==0 ) , "bss:tried to restart buffering when already instantiated!\n"); 
    m_pImp = new CBufferingImp;
}
void CBuffering::SetPrebufDefault(int nSeconds)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bss:buf set pre def\n"); 
    return m_pImp->SetPrebufDefault(nSeconds);
}

int CBuffering::GetPrebufDefault()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_INFO, "bss:buf get pre def\n"); 
    return m_pImp->GetPrebufDefault();
}

void CBuffering::CloseFiles()
{
    return m_pImp->CloseFiles();    
}

void CBuffering::SetPrebufCallback(SetIntFn* pfn)
{
    return m_pImp->SetPrebufCallback(pfn);
}


/*
BUFFERING_REF
*/

CBufferingRef::CBufferingRef() : m_nContentId(0)
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_VERBOSE, "bss:bufferingRef ctor\n"); 
}

CBufferingRef::~CBufferingRef()
{
    DEBUGP( DBG_BUF_ISTREAM, DBGLEV_LOW_LEVEL_TRACE, "bss:bufferingRef dtor\n"); 
    Close();
}

//! Open the input stream to the given source.
ERESULT CBufferingRef::Open( const char* Source )
{
    return CBuffering::GetInstance()->Open(m_nContentId, Source);
}

//! Close the input stream if currently open.
ERESULT CBufferingRef::Close()
{
    return CBuffering::GetInstance()->Close(m_nContentId);
}

//! Read the specified number of bytes into Buffer.
int CBufferingRef::Read( void* Buffer, int Count )
{
    return CBuffering::GetInstance()->Read(m_nContentId,Buffer,Count);
}

//! A generic interface for controlling the input stream.
int CBufferingRef::Ioctl( int Key, void* Value )
{
    return CBuffering::GetInstance()->Ioctl(m_nContentId,Key,Value);
}

//! Called by codecs to determine the minimum unit to read
//! in from this input stream. Not all codecs have been
//! updated to use this routine.
int CBufferingRef::GetInputUnitSize()
{
    return CBuffering::GetInstance()->GetInputUnitSize(m_nContentId);
}

//! Allows the input stream to indicate whether or not seek is
//! supported.
bool CBufferingRef::CanSeek() const
{
    return CBuffering::GetInstance()->CanSeek(m_nContentId);
}

//! Attempt to seek on the input stream.
int CBufferingRef::Seek( InputSeekPos SeekOrigin, int Offset )
{
    return CBuffering::GetInstance()->Seek(m_nContentId,(eInputSeekPos)SeekOrigin,Offset);
}

//! Give back our current position within the stream.
int CBufferingRef::Position() const
{
    return CBuffering::GetInstance()->Position(m_nContentId);
}

//! If available, return the length of the stream.
int CBufferingRef::Length() const
{
    return CBuffering::GetInstance()->Length(m_nContentId);
}
