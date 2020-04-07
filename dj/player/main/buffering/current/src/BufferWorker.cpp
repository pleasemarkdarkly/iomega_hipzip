//........................................................................................
//........................................................................................
//.. File Name: BufferWorker.cpp															..
//.. Date: 01/25/2001																	..
//.. Author(s): Eric Gibbs																..
//.. Description of content:															..
//.. Usage:																				..
//.. Last Modified By: Eric Gibbs	ericg@fullplaymedia.com									..	
//.. Modification date: 03/27/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

/*	CBufferWorker	*/
#include <main/buffering/BufferWorker.h>
#include <main/buffering/BufferDebug.h>

#include <core/playmanager/PlayManager.h>

#include <playlist/common/Playlist.h>
#include <datastream/fatfile/FileInputStream.h>
#include <main/buffering/WriterMsgPump.h>
#include <main/buffering/ReaderMsgPump.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/BufferAccountant.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/DJConfig.h>
#include <main/buffering/DocOrderAuthority.h>
#include <main/buffering/BufferDebug.h>
#include <datasource/fatdatasource/FatDataSource.h>
// (epg,4/11/2002): TODO: integrate into optional component
//#include <main/util/filenamestore/FileNameStore.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_WORKER, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_WORKER);  // debugging prefix : (71) bw

CBufferWorker::CBufferWorker() : m_bAwake(false), m_bReaderUsingPrevBlock(false), 
                         m_nFakeInvalidBlocks(0), m_nFakeValidBlocks(0), 
                         m_nReAnchorPastBlocks(0)
{
    // explain debug print conventions
    PrintDebugKey();
    DEBUGP( DBG_BUF_WORKER, DBGLEV_HIGH_LEVEL_TRACE, "bw:WkrCtor\n"); 

    m_pDocOrderAuthority = new CDocOrderAuthority;
	m_pAccountant = new CBufferAccountant(BUFFER_BLOCKS);
    m_lstUnusedBlocks.Clear();
    m_lstDocuments.Clear();

    for (int i = 0; i < BUFFER_MAX_DOCS; ++i)
    {
        CBufferDocument* pDoc = new CBufferDocument;
        pDoc->SetWorker(this);
        m_lstDocuments.PushBack(pDoc);
    }   
}

CBufferWorker::~CBufferWorker()
{
	delete m_pAccountant;
    delete m_pDocOrderAuthority;
    // this gets allocated in the factory, but I don't care to make a parallel DestroyWorker fn there...
    delete [] m_bfBlockRoom;
    m_bfBlockRoom = NULL;
}

bool CBufferWorker::IsBlockToFill() 
{	
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:IBTW\n"); 
    bool bAllValid = true;
    DocumentListIterator document;
    int iDocument = 0;
    for ( document = GetDocumentHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        // skip past caches.
        if (iDocument < m_nPastDocs)
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:skip %d past..\n",iDocument); 
            continue;
        }
        // skip over corrupt entries
        if ((*document)->IsSourceCorrupt())
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:skip %d corrupt..\n",iDocument); 
            continue;
        }
        // if we exhaust the used caches, then we're done.
        if (!(*document)->GetSourceUrl())
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:%d no url, sleep.\n",iDocument); 
            break;
        }
        if ((iDocument == m_nPastDocs) && (!(*document)->SourceAtDocEnd()))
        {
            // current doc is not to the end of the file yet.
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:FCrnt%d.\n",iDocument); 
            bAllValid = false;
            break;
        }
        if (iDocument == m_nPastDocs && (*document)->SourceAtDocEnd())
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:crnt eof\n",iDocument); 
            continue;
        }
        if ((iDocument > m_nPastDocs) && (!(*document)->SourceAtDocEnd() || !(*document)->HaveDocStartData()))
        {
            // a future doc is incomplete.
            DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:FFut%d.\n",iDocument); 
            bAllValid = false;
            break;
        }
    }
    if (bAllValid || !(*document)->GetSourceUrl())
    {
        // (epg,9/12/2002): if all are valid, then the iterator may be NULL, so can't take the Url in that case.
        if (!bAllValid)
        {
            char* url = (*document)->GetSourceUrl();
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:allValid %s, url %s => sleep\n",
            bAllValid?"True":"False",
            (url)?url:"No Url"); 
        }
        else
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:all valid, sleep\n"); 

        // nothing to fill if all docs are ok, or we're past valid documents in the list (no url)
        return false;
    }
    if (!bAllValid && !(*document)->AreAllBlocksValid())
    {
        // some blocks in the selected doc are invalid, so fill them.
        DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:%d HI\n",iDocument); 
        return true;
    }

    /*
    else if (iDocument == m_nPastDocs)
        (*document)->PrintStateReport("the doc that put us to sleep");
    */
    // all blocks must be ready
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:nf2\n"); 
	return false;
}

// print out a report of all caches and their contents.
void CBufferWorker::ReportOnAllDocuments(char* szCaption)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:AllDocReport %s\n", szCaption);
    int iDocument = 0;
    for (DocumentListIterator document = GetDocumentHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_INFO, " *Document %d active: %s\n",iDocument, document == m_itrActiveDoc ? "true" : "false"); 
        ReportOnDocument(*document, iDocument);
    }
}

// print out a report on one document and its contents.
void CBufferWorker::ReportOnDocument(CBufferDocument* pDoc, int idx)
{
    if (pDoc->GetSourceUrl() != 0)
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "doc %d (%s): \n",idx,pDoc->GetSourceUrl()); 
        BlockList* plstBlocks = pDoc->GetBlockList();
        if (plstBlocks->Size() != 0) 
        {
            for (BlockListIterator buffer = plstBlocks->GetHead(); buffer != plstBlocks->GetEnd(); ++buffer)
            {
                int bufidx = BlockIndexFromLocation(*buffer);
                DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "%02d ", bufidx);
            }
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "\n");
        }
    }
}

// return the index of the buffer in the physical order, taking the pointer itself.
int CBufferWorker::BlockIndexFromLocation(unsigned char* pBuf)
{
    return ( (pBuf - m_bfBlockRoom) / BUFFER_BLOCK_BYTES );
}

StringList* CBufferWorker::GetUrlsToDocument(CDocOrderAuthority* pDocOrderAuth, IMediaContentRecord* mcr)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:GetDocUrls\n"); 
    StringList* plstUrls = new StringList;
    int nPast = DJInBufferConfig()->nMaxPastDocs;
    int nFuture = DJInBufferConfig()->nMaxDocs - nPast - 1;
    pDocOrderAuth->GetOrdering(plstUrls,nPast,nFuture,mcr);
    return plstUrls;
}

// deconfigure any caches not represented in the list of file refs.
void DetachUnneededSources(StringList* plstUrls, DocumentList* plstDocuments)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:DetachUnneededSources\n");
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "playlist urls:\n"); 
#if DEBUG_LEVEL > 0
    int i = 0;
    for (StringListIterator itrUrl = plstUrls->GetHead(); itrUrl != plstUrls->GetEnd(); ++itrUrl)
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "%d %s\n",++i,(*itrUrl)); 
    }
#endif
    bool bUsed[plstUrls->Size()];
    for (int j = 0; j < plstUrls->Size(); ++j)
        bUsed[j] = 0;
    for (DocumentListIterator document = plstDocuments->GetHead(); document != plstDocuments->GetEnd(); ++document)
    {
        bool bNeeded = false;
        if ((*document)->GetSourceUrl())
        {
            int j = 0;
            for (StringListIterator url = plstUrls->GetHead(); url != plstUrls->GetEnd(); ++url, ++j)
                //if (!strcmp((*url),(*document)->GetSourceUrl()))
                if (!strcmp((*document)->GetSourceUrl(),(*url)) && !bUsed[j])
                {
                    //DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:keep %s\n",(*document)->GetSourceUrl()); 
                    bNeeded = true;
                    bUsed[j] = true;
                    break;
                }
        }
        if (!bNeeded)
        {
            //DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:detach %s\n",(*document)->GetSourceUrl()); 
            (*document)->DetachSource();
        }
    }
}

// assign untaken files to free caches, and order to playlist order.
void CBufferWorker::AttachDocumentSources(StringList* plstUrls)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:AttachDocSources\n");
    DocumentList lstOrdered;
    lstOrdered.Clear();
    for (StringListIterator url = plstUrls->GetHead(); url != plstUrls->GetEnd(); ++url)
    {
        bool bUrlAssigned = false;
        DocumentListIterator unused = 0;
        for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document)
        {
            if ((*document)->GetSourceUrl() == 0)
                unused = document;
            else if (!strcmp((*document)->GetSourceUrl(),(*url)))
            {
                bUrlAssigned = true;
                lstOrdered.PushBack(*document);
                m_lstDocuments.Remove(document);
                if (m_lstDocuments.Size() > 0)
                    document = m_lstDocuments.GetHead();
                else
                    document = 0;
                break;
            }
        }
        if (!bUrlAssigned)
        {
            if (unused != 0)
            {
                DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "attach %s\n",(*url)); 
                (*unused)->AttachFile(*url);
                lstOrdered.PushBack(*unused);
                m_lstDocuments.Remove(unused);
                unused = NULL;
            }
            else
                DEBUGP( DBG_BUF_WORKER, DBGLEV_ERROR, "bw: unexpected condition - no unused document to assign source url (%s) to!\n", (*url));
        }
    }
    // track how many past docs we're dealing with
    m_nPastDocs = DJInBufferConfig()->nMaxPastDocs;
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "order sz %d, pastDocs %d\n",lstOrdered.Size(),m_nPastDocs); 
    if (lstOrdered.Size() < BUFFER_MAX_DOCS)
        m_nPastDocs = 0;
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "pastDocs %d\n",m_nPastDocs); 

    lstOrdered.Append(m_lstDocuments);
    // stick the empty documents onto 
    m_lstDocuments.Clear();
    m_lstDocuments.Append(lstOrdered);
    int iUrl = 0;

    m_itrActiveDoc = NULL;
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++iUrl) {
        if (iUrl == m_nPastDocs)
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "using %d (%s) as crnt\n",iUrl, (*document)->GetSourceUrl()); 
            m_itrActiveDoc = document;
            break;
        }
    }
    DEBUGP( DBG_BUF_WORKER, DBGLEV_HIGH_LEVEL_TRACE, "bf:'%s' active\n",(*m_itrActiveDoc)->GetSourceUrl());
}

void CBufferWorker::SetDistantDocumentsExpendable()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:SetDistantExpendable\n");
    int nBlocksUsed = 0;
    int iDocument = 0;
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:D%d has %d\n",iDocument,(*document)->m_lstBlocks.Size()); 

        // past caches are cullable.
        if (iDocument < m_nPastDocs)
        {
            //DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:past cull %d\n",iDocument); 
            (*document)->CloseSource();
        }
        // if we're too far in the future, cull.
        else if (nBlocksUsed >= BUFFER_BLOCKS)
        {
            //DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:future cull %d\n",iDocument); 
            (*document)->CloseSource();
        }
        // count blocks in caches that aren't trivially cullable.
        else
        {
            // if we get to an uninitialized document, just continue.
            if (!(*document)->GetSourceUrl())
            {
                (*document)->CloseSource();
                continue;
            }
            bool bOpen = (*document)->IsSourceOpen();
            int nFileLen;
            if (bOpen || (*document)->OpenSource())
                // (epg,8/2/2002): now considering all files as essentially infinite until eof.
                nFileLen = ARBITRARILY_LARGE_DOCSIZE;
            else
                // (epg,5/28/2002): if we can't get the length right now, assume it's large so we're prepared
                // if, e.g., a net doc reappears as valid and needs to have some buffers by now.  the downside
                // to this is that we may give out blocks over-easily.  but the flip side is grinding to a halt
                // b/c the current track doesn't have any blocks to work with.
                nFileLen = ARBITRARILY_LARGE_DOCSIZE;
            if (nFileLen < 0) nFileLen = 0;
            if (!bOpen)
                (*document)->CloseSource();
            int nFileBlocks = nFileLen / BUFFER_BLOCK_BYTES;
            if (nFileLen % BUFFER_BLOCK_BYTES)
                ++nFileBlocks;
            // (epg,6/10/2002): add one more block, in case the stated length is a bitter, evil lie.
            ++nFileBlocks;
            nBlocksUsed += nFileBlocks;
            if (nBlocksUsed >= BUFFER_BLOCKS)
            {
                (*document)->CloseSource();
            }

        }
    }
}

void CBufferWorker::ResetValidityCounts()
{
    int nContiguousValid = 0;
    int iDocument = 0;
    DocumentListIterator document;
    for (document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        // past caches are "empty", since they are cullable.
        if (iDocument < m_nPastDocs)
            continue;
        
        // if document is missing blocks from the DocStart, then we're done.
        if (!(*document)->HaveDocStartData())
            break;
        // if the document is at eof, count and continue.
        if ((*document)->SourceAtDocEnd() && (*document)->HaveDocStartData())
        {
            nContiguousValid += (*document)->CountValidBlocks();
            continue;
        }
        // if the document isn't full, count and finish.
        else
        {
            nContiguousValid += (*document)->CountValidBlocks();   
            break;
        }
    }
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:ResetSems %d\n",nContiguousValid);
    m_pAccountant->Reset(nContiguousValid);
}

// sync up with the playlist, assuming the current track is inviolate and shouldn't be interrupted.
void CBufferWorker::ResyncDocumentOrder()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:SyncDocuments\n");
    // generate a list of files worth caching
    //ReportOnAllDocuments("Before Deconfigure");
    CDocOrderAuthority doa;
    IPlaylistEntry* pEntry = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();

    DBASSERT( DBG_BUF_WORKER, ( pEntry ) , "bw:4\n"); 
    StringList* plstUrls = GetUrlsToDocument(&doa,pEntry->GetContentRecord());
    // de-init any currently cached files not on the list
    DetachUnneededSources(plstUrls, &m_lstDocuments);
    // assign any unserviced file refs to empty caches, and order the caches to playlist order.
    //ReportOnAllDocuments("Before AttachDocumentSources");
    AttachDocumentSources(plstUrls);

    while (plstUrls->Size() > 0)
        delete [] plstUrls->PopBack();
    delete plstUrls;

    // see which caches have no hope of retaining blocks once we spin up and recycle.
    SetDistantDocumentsExpendable();

    // starting from the read point, how many contiguous, full blocks are available
    ResetValidityCounts();
}

// reshuffle the document list to match playlist order.
// caller must guarantee blocks and caches aren't being accessed asynchronously.
void CBufferWorker::SyncDocumentOrder(IMediaContentRecord* mcr)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:SyncDocuments\n");
    // generate a list of files worth caching
    //ReportOnAllDocuments("Before Deconfigure");
    CDocOrderAuthority doa;
    StringList* plstUrls = GetUrlsToDocument(&doa,mcr);
    // de-init any currently cached files not on the list
    DetachUnneededSources(plstUrls, &m_lstDocuments);
    // assign any unserviced file refs to empty caches, and order the caches to playlist order.
    //ReportOnAllDocuments("Before AttachDocumentSources");
    AttachDocumentSources(plstUrls);

    while (plstUrls->Size() > 0)
        delete [] plstUrls->PopBack();
    delete plstUrls;

    // see which caches have no hope of retaining blocks once we spin up and recycle.
    SetDistantDocumentsExpendable();
    // at this point we know enough about the caches to deterimine if we can proceed
    if (!ActiveDocValid())
        return;
    // starting from the read point, how many contiguous, full blocks are available
    ResetValidityCounts();
    // check that the current document either has DocStart buffered or a full set of blocks
    //ReportOnAllDocuments("Before ReadyCurrent");
    if (!ActiveDocReady())
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:readying active doc\n"); 
        PrepareActiveDoc();
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:starting block writing\n"); 
        CBufferingImp::GetInstance()->GetWriterMsgPump()->StartProducing();
    }
    else
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:doc was ready, jump to start\n"); 
        (*m_itrActiveDoc)->JumpToDocStart();
        CBufferingImp::GetInstance()->GetWriterMsgPump()->StartProducing();
    }
    //ReportOnAllDocuments("After Sync");
}

// return the document most in need of filling.
CBufferDocument* CBufferWorker::GetDocumentToWrite()
{
//    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:GetToFill\n");
    int iDocument = 0;
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        // skip past caches
        if (iDocument < m_nPastDocs)
            continue;
        // skip corrupt caches
        if ((*document)->IsSourceCorrupt())
            continue;
        // fill the current document if not yet DocEnd
        if (((*document) == (*m_itrActiveDoc)) && !(*document)->SourceAtDocEnd())
            return (*document);
        // skip the current document otherwise
        if ((*document) == (*m_itrActiveDoc))
            continue;
        // fill future caches if not fully cached.
        if (!(*document)->SourceAtDocEnd() || !(*document)->HaveDocStartData())
            return (*document);
    }
    DEBUGP( DBG_BUF_WORKER, DBGLEV_ERROR, "bw: unexpected condition - no document to fill!\n");
    return (CBufferDocument*) 0;
}

// return a document iterator associated with the given document.
DocumentListIterator CBufferWorker::GetDocumentByDocument(CBufferDocument* pDoc)
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:GetCbyC\n");
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document)
        if ((*document) == pDoc)
            return DocumentListIterator(document);
    DEBUGP( DBG_BUF_WORKER, DBGLEV_WARNING, "bw: GetDocumentByDocument couldn't locate document!\n");
    return m_lstDocuments.GetEnd();
}

void CBufferWorker::InitUnusedBlockList()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_HIGH_LEVEL_TRACE, "bw:InitUnused\n");
    m_lstUnusedBlocks.Clear();
    for (int i = 0; i < BUFFER_BLOCKS; ++i)
        m_lstUnusedBlocks.PushBack((unsigned char*) m_bfBlockRoom + (i * BUFFER_BLOCK_BYTES));
}

int CBufferWorker::GetFreeBlocks(int nNeeded, BlockList* plstBlocks, bool bPrepend)
{
    int nGiven = m_lstUnusedBlocks.Size();
    if (nGiven > nNeeded)
        nGiven = nNeeded;
    if (bPrepend)
    {
        for (int i = 0; i < nGiven; ++i)
            plstBlocks->PushFront(m_lstUnusedBlocks.PopBack());
    }
    else
    {
        for (int i = 0; i < nGiven; ++i)
            plstBlocks->PushBack(m_lstUnusedBlocks.PopBack());
    }
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:GetFree %d\n",nGiven);
    return nGiven;
}

// count from just before the readpoint forward, counting contiguous blocks that don't need culling
int CBufferWorker::CountUsefulBlocks()
{
    int nUsefulBlocks = 0;
    // don't cull blocks just before the readpoint
    int nPreReadBlocks = (*m_itrActiveDoc)->CountOldChars() / BUFFER_BLOCK_BYTES;
    int nUsefulPreReadBlocks;
    if (nPreReadBlocks > RECLAIM_BLOCK_AGE)
        nUsefulPreReadBlocks = RECLAIM_BLOCK_AGE;
    else
        nUsefulPreReadBlocks = nPreReadBlocks;
    nUsefulBlocks += nUsefulPreReadBlocks;
    // don't cull blocks in the current document beyond the readpoint.
    int nFutureBlocks = (*m_itrActiveDoc)->GetBlockList()->Size() - nPreReadBlocks;
    nUsefulBlocks += nFutureBlocks;
    // don't cull contiguous blocks in close-by future caches.
    for (DocumentListIterator document = m_itrActiveDoc + 1; document != m_lstDocuments.GetEnd(); ++document)
    {
        // if the document isn't associated with a file, then we're done.
        if (!(*document)->GetSourceUrl())
            break;
        // if the whole document is in blocks, count it and continue
        if ((*document)->HaveDocStartData() && (*document)->SourceAtDocEnd())
            nUsefulBlocks += (*document)->GetBlockList()->Size();
        // if the beginning of the document is buffered, count it and stop walking the list.
        else if ((*document)->HaveDocStartData())
        {
            nUsefulBlocks += (*document)->GetBlockList()->Size();
            break;
        }
        // if not even the DocStart is buffered, don't count it and stop walking.
        else
            break;
        if (nUsefulBlocks > BUFFER_BLOCKS)
            break;
    }
    if (nUsefulBlocks < 0)
        nUsefulBlocks = 0;
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:tot useful %d\n",nUsefulBlocks);
    return nUsefulBlocks;
}
    
// report on how many blocks the current document is missing.
int CBufferWorker::CurrentDocumentShortfall()
{
    // count the buffer count necessary to get from the current document-root to the DocEnd.  return that, reduced by the count of blocks the document already has.
    int nFutureBlocks = BlockCountFromCharCount((*m_itrActiveDoc)->BufferLength() - (*m_itrActiveDoc)->FirstWindowByte());
    if (nFutureBlocks > BUFFER_BLOCKS)
        nFutureBlocks = BUFFER_BLOCKS;
    return nFutureBlocks - (*m_itrActiveDoc)->GetBlockList()->Size();
}

void CBufferWorker::ReclaimDeadBlocks()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_LOW_LEVEL_TRACE, "bw:reclaimDead\n");
    
    // figure out how many we should cull.
    int nNeeded                   = BUFFER_BLOCKS - m_lstUnusedBlocks.Size();
    int nUsefulBlocks             = CountUsefulBlocks();
    int nCurrentDocumentShortfall = CurrentDocumentShortfall();
    
    // since the consumer is active during this time, I cull one less buffer to avoid culling a useful one b/c of a race condition.
    ++nUsefulBlocks;
    nNeeded -= nUsefulBlocks;
    if (nNeeded < nCurrentDocumentShortfall)
        nNeeded = nCurrentDocumentShortfall;
    if (nNeeded > BUFFER_BLOCKS - RECLAIM_BLOCK_AGE - NET_BUFFER_WAKE_THRESHOLD - m_lstUnusedBlocks.Size())
        nNeeded = BUFFER_BLOCKS - RECLAIM_BLOCK_AGE - NET_BUFFER_WAKE_THRESHOLD - m_lstUnusedBlocks.Size();
    if (nNeeded < 1)
        return;
    
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:Cull %d\n",nNeeded);

    // first, free up any past caches.
    for (DocumentListIterator document = m_lstDocuments.GetHead(); (*document) != (*m_itrActiveDoc); ++document)
        if (nNeeded) {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:culling past\n"); 
            nNeeded -= (*document)->GiveUpBlocks(nNeeded);
            if (nNeeded < 0) nNeeded = 0;
        }

    // next, chip into the current file
    if (nNeeded) {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:culling active!\n"); 
        nNeeded                  -= (*m_itrActiveDoc)->GiveUpBlocks((*m_itrActiveDoc)->CountDeadBlocks(), true);
        if (nNeeded < 0) nNeeded  = 0;
    }
    
    // finally, work back from the future most document.
    if (nNeeded)
        for (DocumentListIterator document = m_lstDocuments.GetTail(); document != m_lstDocuments.GetEnd(); --document)
        {
            // if somehow we still are trying to cull when we come back around to the current document, just stop.
            if (document == m_itrActiveDoc)
                break;
            // (epg,9/13/2002): I was avoiding culling documents that I had figured to be unneeding of culling (because
            // of their proximity to the user's listening point), but I think that seeking around can throw this off, so it's
            // better to just cull what we need without such distinctions.  The worst thing this can do is throw some
            // data away from a future document, but in a responsible manner (proper accounting et al).  And it can't
            // affect the current doc, so I think it's a clear win.  The previous system could wedge, I believe, b/c
            // the future doc could squirrel away lots of blocks, starving the current track.
            nNeeded -= (*document)->GiveUpBlocks(nNeeded);
            if (nNeeded < 0) nNeeded = 0;
            if (!nNeeded)
                break;
        }
    if (nNeeded) {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw: %d needed at end of cull\n",nNeeded);       
    }
    //ReportOnAllDocuments("After Cull");
}

// award any unused blocks to current and future caches.
void CBufferWorker::DistributeFreeBlocks()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:DistFreeBlocks\n");
    //ReportOnAllDocuments("Before DistributeToDocuments");
    int iDocument = 0;
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++iDocument)
    {
        // m_nPastDocs is the count of past docs to keep in in m_lstDocuments.  In other words, m_lstDocuments is
        // laid out as GetHead(),...,(m_nPastDocs - 1),m_nPastDocs,...,GetEnd(), where m_nPastDocs is the place of
        // the current doc.
        if (iDocument < m_nPastDocs)
        {
            // if a past file exists and is open, then close it.
            if ((*document)->GetSourceUrl() && (*document)->IsSourceOpen())
                (*document)->CloseSource();
            
            // don't give it any caches, since it is in the past.
            continue;
        }
        
        // stop distribution if source is empty
        if (m_lstUnusedBlocks.Size() == 0)
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:all distributed\n"); 
            break;
        }
        
        // the current document doesn't need DocStart support, just future blocks.
        // award all available blocks
        bool bCurrentDocument = (iDocument == m_nPastDocs);
        if (bCurrentDocument)
        {
            // (epg,9/12/2002): don't distro to an already complete document.  normally this 
            // would be mostly ok, but if the OpenSource fails, the already buffered document
            // would be discarded as corrupt, which would be silly.
            if (!(*document)->SourceAtDocEnd())
            {
                DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:distr to crnt %d\n",iDocument);

                // GetFreeBlocks gets blocks from m_lstUnusedBlocks and puts them onto (*document)->GetBlockList(),
                // in other words, adds free blocks to the end of the document list so that the document can continue
                // to write out data.
                GetFreeBlocks(BUFFER_BLOCKS - (*document)->GetBlockList()->Size(),(*document)->GetBlockList());
                (*document)->SyncNextInvalidBlock();
            }
            else
            {
                DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:skip crnt, already eof\n"); 
            }
        }
        // Future document
        else
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:distr noncrnt %d, jumping to start\n",iDocument); 
            if (!(*document)->GetSourceUrl())
                continue;

            (*document)->JumpToDocStart();

            // unless doc at end, assume infinite needs.
            if (!(*document)->SourceAtDocEnd())
            {
                GetFreeBlocks(BUFFER_BLOCKS - (*document)->GetBlockList()->Size(),(*document)->GetBlockList());
                (*document)->SyncNextInvalidBlock();
            }
        }
    }
//    ReportOnAllDocuments("After DistributeToDocuments");
}

bool CBufferWorker::ActiveDocReady()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:IsCurrRdy\n");
    if ((*m_itrActiveDoc)->HaveDocStartData() && (*m_itrActiveDoc)->CountValidBlocks() > NET_BUFFER_WAKE_THRESHOLD)
    {
        return true;
    }
    return false;
}

void CBufferWorker::PrepareActiveDoc()
{
    CBufferDocument* pCurrent = (*m_itrActiveDoc);
    // rewind the document, especially in case we're looping on one file and need to cull out future blocks.
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "jump to doc start\n"); 
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:prepping active doc\n"); 
    pCurrent->JumpToDocStart();
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "reclaim dead\n"); 
    ReclaimDeadBlocks();
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "rejump to start\n"); 
    // rewind again, in case there were no blocks avail before
    pCurrent->JumpToDocStart();

    if (!pCurrent->SourceAtDocEnd())
    {
        // (epg,9/12/2002): continue even if we fail to open the input stream.
        //if (pCurrent->OpenSource())
        pCurrent->OpenSource();
        //{
            int nFileSize = pCurrent->BufferLength();
            int nBlocksNeeded = BlockCountFromCharCount(nFileSize);
            // add one in case the length is inaccurate.  if we actually start writing in the last owned block, then
            // we will know the length is wrong and to prepare for extended buffering.
            if (pCurrent->GetBlockList()->Size() < nBlocksNeeded) {
                GetFreeBlocks(nBlocksNeeded - pCurrent->GetBlockList()->Size(),pCurrent->GetBlockList());
                pCurrent->SyncNextInvalidBlock();
            }
        //}
        //else
        //    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:couldn't open source in prepActive\n"); 
    }
}

void CBufferWorker::CloseAllSources()
{
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document)
        (*document)->CloseSource();
}

void CBufferWorker::CloseSeekableSources()
{
    DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:CloseSeekableSources\n"); 
    
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document)
    {
        DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:checking sources\n"); 
        if ((*document)->CanSourceSeek())
        {
            DEBUGP( DBG_BUF_WORKER, DBGLEV_VERBOSE, "bw:closing fat source\n"); 
            (*document)->CloseSource();
        }
    }
}

bool CBufferWorker::ActiveDocValid()
{
    int i = 0;
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document, ++i)
    {
        if (i < m_nPastDocs)
        {
            continue;
        }
        if (i == m_nPastDocs)
        {
            return (!(*document)->IsSourceCorrupt());
        }
    }
    return false;
}

void CBufferWorker::DetachAllSources()
{
    for (DocumentListIterator document = m_lstDocuments.GetHead(); document != m_lstDocuments.GetEnd(); ++document)
        (*document)->DetachSource();
}
