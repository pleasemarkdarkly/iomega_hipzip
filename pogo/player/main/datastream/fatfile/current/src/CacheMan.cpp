//........................................................................................
//........................................................................................
//.. File Name: CacheMan.cpp															..
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

/*	CCacheMan	*/
#include <main/datastream/fatfile/CacheMan.h>

#include <core/playmanager/PlayManager.h>

#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <datastream/fatfile/FileInputStream.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>
#include <main/datastream/fatfile/ConsumerMsgPump.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/CacheReferee.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/util/filenamestore/FileNameStore.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CACHE_MAN, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE(DBG_CACHE_MAN);

CCacheMan*	CCacheMan::m_pInstance = 0;

CCacheMan* CCacheMan::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CCacheMan();
	return m_pInstance;
}

CCacheMan::CCacheMan() : m_bActive(false), m_nFakeEmpties(0), m_bFillMode(false), m_bConsumerBorrowingEmpty(false), 
                         m_nFakeEmptyBuffers(0), m_nFakeFullBuffers(0), m_bBufferMode(false), m_bBufferingReady(false), 
                         m_bFirstSpinup(true), m_bJustSeekedFromEOF(false), m_bConservativeBuffering(false), 
                         m_nRebufferPastBuffersToFill(0), m_nPastCaches(0)

{
	m_pCacheReferee = new CCacheReferee(BUFFER_COUNT);
    m_lstUnusedBuffers.Clear();
    InitUnusedBufferList();
    m_lstCaches.Clear();
    for (int i = 0; i < MAX_CACHE_COUNT; ++i)
        m_lstCaches.PushBack(new CFileCache);
}

CCacheMan::~CCacheMan()
{
	delete m_pCacheReferee;
}

bool CCacheMan::IsBufferToFill() 
{	
    //DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:IsBufToFill\n");
    bool bAllCachesFull = true;
    CacheListIterator cache;
    int iCache = 0;
    for ( cache = GetCacheHead(); cache != 0; ++cache, ++iCache)
    {
        // if we exhaust the used caches, then we're done.
        if (!(*cache)->GetFileNameRef())
            break;
        // skip over corrupt entries
        if ((*cache)->FileIsCorrupt())
            continue;
        // skip past caches.
        if (iCache < m_nPastCaches)
            continue;
        // fill current if not eof
        // (epg,10/30/2001): TODO: split these out and catch skip buffer cases for current cache.
        if ((iCache == m_nPastCaches) && (!(*cache)->IsEOFBuffered()))
        {
            bAllCachesFull = false;
            break;
        }
        // fill future if not complete
        if ((iCache > m_nPastCaches) && (!(*cache)->IsEOFBuffered() || !(*cache)->IsBOFBuffered()))
        {
            bAllCachesFull = false;
            break;
        }
    }
    if (bAllCachesFull || !(*cache)->GetFileNameRef())
        return false;
    if (!(*cache)->AreAllBuffersFull())
        return true;
	return false;
}

// print out a report of all caches and their contents.
void CCacheMan::ReportOnAllCaches(const char* szCaption)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:Report %s\n", szCaption);
    int iCache = 0;
    for (CacheListIterator cache = GetCacheHead(); cache != 0; ++cache, ++iCache)
        ReportOnCache(*cache, iCache);
}

// print out a report on one cache and its contents.
void CCacheMan::ReportOnCache(CFileCache* pCache, int idx)
{
    if (pCache->GetFileNameRef() != 0)
    {
        DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "#%d: %s\n", idx, pCache->GetFileNameRef()->URL());
        BufferList* plstBuffers = pCache->GetBufferList();
        if (plstBuffers->Size() != 0) 
        {
            for (BufferListIterator buffer = plstBuffers->GetHead(); buffer != plstBuffers->GetEnd(); ++buffer)
            {
                int bufidx = BufferIndexFromLocation(*buffer);
                DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "%02d ", bufidx);
            }
            DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "\n");
        }
    }
}

// return the index of the buffer in the physical order, taking the pointer itself.
int CCacheMan::BufferIndexFromLocation(unsigned char* pBuf)
{
    return ( (pBuf - m_BufferSpace) / BUFFER_SIZE );
}

// return a list of file refs that need to be cached.
FileRefList* CCacheMan::GetFileRefsToCache(IPlaylistEntry* pCurrentEntry)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:GetFileRefsToCache\n");
    FileRefList* lstFileNameRefs = new FileRefList;
    lstFileNameRefs->Clear();
    IPlaylistEntry* pEntry = pCurrentEntry;
    CPogoPlaylist* pl = (CPogoPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    int size = pl->GetSize();
    // cover previous tracks, if there are enough entries to warrant it.
    if (size > MAX_CACHE_COUNT)
    {
        m_nPastCaches = PREV_TRACK_CACHE_COUNT;
        for (int i = 0; i < PREV_TRACK_CACHE_COUNT; ++i)
        {
            pEntry = pl->GetPreviousEntry(pEntry,IPlaylist::NORMAL);
            if (!pEntry)
                pEntry = pl->GetTailEntry();
        }
    }
    else
        m_nPastCaches = 0;

    IPlaylistEntry* pFirst = pEntry;
    for (int i = 0; i < MAX_CACHE_COUNT; i++)
    {
        lstFileNameRefs->PushBack(pEntry->GetContentRecord()->GetFileNameRef());
        pEntry = pl->GetNextEntry(pEntry,IPlaylist::NORMAL);
        if (!pEntry)
            pEntry = pl->GetHeadEntry();
        if (pEntry == pFirst)
            break;
    }
    return lstFileNameRefs;
}

// deconfigure any caches not represented in the list of file refs.
void DeconfigureUnneededCaches(FileRefList* plstFileRefs, CacheList* plstCaches)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:DecUnneeded\n");
    for (CacheListIterator cache = plstCaches->GetHead(); cache != plstCaches->GetEnd(); ++cache)
    {
        bool bNeeded = false;
        if ((*cache)->GetFileNameRef())
            for (FileRefListItr file = plstFileRefs->GetHead(); file != plstFileRefs->GetEnd(); ++file)
                //if (!strcmp((*file),(*cache)->GetFileNameRef()))
                if ((*cache)->GetFileNameRef()->IsSame(*file))
                {
                    bNeeded = true;
                    break;
                }
        if (!bNeeded)
            (*cache)->Deconfigure();
    }
}

// assign untaken files to free caches, and order to playlist order.
void CCacheMan::SetCacheFileRefs(FileRefList* plstFileRefs)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:SetFileRefs\n");
    CacheList lstOrdered;
    lstOrdered.Clear();
    for (FileRefListItr file = plstFileRefs->GetHead(); file != plstFileRefs->GetEnd(); ++file)
    {
        bool bFileRefAssigned = false;
        CacheListIterator unused = 0;
        for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache)
        {
            if ((*cache)->GetFileNameRef() == 0)
                unused = cache;
            else if ((*cache)->GetFileNameRef()->IsSame((*file)))
            {
                bFileRefAssigned = true;
                lstOrdered.PushBack(*cache);
                m_lstCaches.Remove(cache);
                if (m_lstCaches.Size() > 0)
                    cache = m_lstCaches.GetHead();
                else
                    cache = 0;
                break;
            }
        }
        if (!bFileRefAssigned)
            if (unused != 0)
            {
                (*unused)->InitForFileNameRef(*file);
                lstOrdered.PushBack(*unused);
                m_lstCaches.Remove(unused);
                unused = NULL;
            }
            else {
                DEBUGP( DBG_CACHE_MAN, DBGLEV_ERROR, "cm: unexpected condition - no unused cache to assign file ref to!\n");
            }
    }
    lstOrdered.Append(m_lstCaches);
    m_lstCaches.Clear();
    m_lstCaches.Append(lstOrdered);
    int iFileRef = 0;
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++iFileRef) {
        if (iFileRef == m_nPastCaches)
        {
            m_itCurrentCache = cache;
            break;
        }
    }
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "current %s\n",(*m_itCurrentCache)->GetFileNameRef()->LongName());
}

void CCacheMan::MarkDistantCachesCullable()
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:MarkCulls\n");
    int nBuffersUsed = 0;
    int iCache = 0;
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++iCache)
    {
        (*cache)->SetCullable(false);
        // past caches are cullable.
        if (iCache < m_nPastCaches)
            (*cache)->SetCullable(true);
        // if we're too far in the future, cull.
        else if (nBuffersUsed >= BUFFER_COUNT)
            (*cache)->SetCullable(true);
        // count buffers in caches that aren't trivially cullable.
        else
        {
            // if we get to an uninitialized cache, just continue.
            if (!(*cache)->GetFileNameRef())
            {
                (*cache)->SetCullable(true);
                continue;
            }
            bool bOpen = (*cache)->IsFileOpen();
            if (!bOpen)
                (*cache)->OpenFile();
            int nFileLen = (*cache)->FileLength();
            if (nFileLen < 0) nFileLen = 0;
            if (!bOpen)
                (*cache)->CloseFile();
            int nFileBuffers = nFileLen / BUFFER_SIZE;
            if (nFileLen % BUFFER_SIZE)
                ++nFileBuffers;
            nBuffersUsed += nFileBuffers;
            if (nBuffersUsed >= BUFFER_COUNT)
                (*cache)->SetCullable(true);
        }
    }
}

void CCacheMan::ResetSemaphoreCounts()
{
    int nContiguousFull = 0;
    int iCache = 0;
    m_nNonContigFullBuffers = 0;
    CacheListIterator cache;
    for (cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++iCache)
    {
        // past caches are "empty", since they are cullable.
        if (iCache < m_nPastCaches)
            continue;
        // if cache is missing buffers from the BOF, then we're done.
        if (!(*cache)->IsBOFBuffered())
            break;
        // if the cache is at eof, count and continue.
        if ((*cache)->IsEOFBuffered() && (*cache)->IsBOFBuffered())
            nContiguousFull += (*cache)->CountFullBuffers();
        // if the cache isn't full, count and finish.
        else
        {
            nContiguousFull += (*cache)->CountFullBuffers();   
            break;
        }
    }
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:ResetSems %d\n",nContiguousFull);
    m_pCacheReferee->Reset(nContiguousFull);
}

// reshuffle the cache list to match playlist order.
// caller must guarantee buffers and caches aren't being accessed asynchronously.
void CCacheMan::SyncCachesToPlaylistOrder(IPlaylistEntry* pCurrentEntry)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:SyncCaches\n");
    // generate a list of files worth caching
    //ReportOnAllCaches("Before Deconfigure");
    FileRefList* plstFileRefs = GetFileRefsToCache(pCurrentEntry);
    // de-init any currently cached files not on the list
    DeconfigureUnneededCaches(plstFileRefs, &m_lstCaches);
    // assign any unserviced file refs to empty caches, and order the caches to playlist order.
    //ReportOnAllCaches("Before SetCacheFileRefs");
    SetCacheFileRefs(plstFileRefs);

    plstFileRefs->Clear();
    delete plstFileRefs;
    // see which caches have no hope of retaining buffers once we spin up and recycle.
    MarkDistantCachesCullable();
    // at this point we know enough about the caches to deterimine if we can proceed
    if (!IsCurrentCacheValid())
        return;
    // starting from the read point, how many contiguous, full buffers are available
    ResetSemaphoreCounts();
    // check that the current cache either has BOF buffered or a full set of buffers
    //ReportOnAllCaches("Before ReadyCurrent");
    if (!IsCurrentCacheReady())
    {
        ReadyCurrentCache();
        CBufferedFatFileInputStreamImp::GetInstance()->GetProducerMsgPump()->StartProducing();
    }
    else
        (*m_itCurrentCache)->RewindCacheToBOF();
    //ReportOnAllCaches("After Sync");
}

// return the cache most in need of filling.
CFileCache* CCacheMan::GetNextCacheToFill()
{
    //DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:GetToFill\n");
    int iCache = 0;
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++iCache)
    {
        // skip past caches
        if (iCache < m_nPastCaches)
            continue;
        // skip corrupt caches
        if ((*cache)->FileIsCorrupt())
            continue;
        // fill the current cache if not yet EOF
        if (((*cache) == (*m_itCurrentCache)) && !(*cache)->IsEOFBuffered())
            return (*cache);
        // skip the current cache otherwise
        if ((*cache) == (*m_itCurrentCache))
            continue;
        // fill future caches if not fully cached.
        if (!(*cache)->IsEOFBuffered() || !(*cache)->IsBOFBuffered())
            return (*cache);
    }
    DEBUGP( DBG_CACHE_MAN, DBGLEV_ERROR, "cm: unexpected condition - no cache to fill!\n");
    return (CFileCache*) 0;
}

// return a cache iterator associated with the given cache.
CacheListIterator CCacheMan::GetCacheByCache(CFileCache* pCache)
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:GetCbyC\n");
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache)
        if ((*cache) == pCache)
            return CacheListIterator(cache);
    DEBUGP( DBG_CACHE_MAN, DBGLEV_WARNING, "cm: GetCacheByCache couldn't locate cache!\n");
    return m_lstCaches.GetEnd();
}

void CCacheMan::InitUnusedBufferList()
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:InitUnused\n");
    m_lstUnusedBuffers.Clear();
    for (int i = 0; i < BUFFER_COUNT; ++i)
        m_lstUnusedBuffers.PushBack((unsigned char*) m_BufferSpace + (i * BUFFER_SIZE));
}

int CCacheMan::GetFreeBuffers(int nNeeded, BufferList* plstBuffers, bool bPrepend)
{
    int nGiven = m_lstUnusedBuffers.Size();
    if (nGiven > nNeeded)
        nGiven = nNeeded;
    if (bPrepend)
        for (int i = 0; i < nGiven; ++i)
            plstBuffers->PushFront(m_lstUnusedBuffers.PopBack());
    else
        for (int i = 0; i < nGiven; ++i)
            plstBuffers->PushBack(m_lstUnusedBuffers.PopBack());
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:GetFree %d\n",nGiven);
    return nGiven;
}

// count from just before the readpoint forward, counting contiguous buffers that don't need culling
int CCacheMan::CountUsefulBuffers()
{
    int nUsefulBufs = 0;
    // don't cull buffers just before the readpoint
    int nPreReadBufs = (*m_itCurrentCache)->CountPreReadPointChars() / BUFFER_SIZE;
    int nUsefulPreReadBufs;
    if (nPreReadBufs > BUFFERS_EMPTY_AT_SPINDOWN)
        nUsefulPreReadBufs = BUFFERS_EMPTY_AT_SPINDOWN;
    else
        nUsefulPreReadBufs = nPreReadBufs;
    nUsefulBufs += nUsefulPreReadBufs;
    // don't cull buffers in the current cache beyond the readpoint.
    int nFutureBufs = (*m_itCurrentCache)->GetBufferList()->Size() - nPreReadBufs;
    nUsefulBufs += nFutureBufs;
    // don't cull contiguous buffers in close-by future caches.
    for (CacheListIterator cache = m_itCurrentCache + 1; cache != m_lstCaches.GetEnd(); ++cache)
    {
        // if the cache isn't associated with a file, then we're done.
        if (!(*cache)->GetFileNameRef())
            break;
        // if the whole cache is in buffers, count it and continue
        if ((*cache)->IsBOFBuffered() && (*cache)->IsEOFBuffered())
            nUsefulBufs += (*cache)->GetBufferList()->Size();
        // if the beginning of the cache is buffered, count it and stop walking the list.
        else if ((*cache)->IsBOFBuffered())
        {
            nUsefulBufs += (*cache)->GetBufferList()->Size();
            break;
        }
        // if not even the BOF is buffered, don't count it and stop walking.
        else
            break;
        if (nUsefulBufs > BUFFER_COUNT)
            break;
    }
    if (nUsefulBufs < 0)
        nUsefulBufs = 0;
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:tot useful %d\n",nUsefulBufs);
    return nUsefulBufs;
}
    
// report on how many buffers the current cache is missing.
int CCacheMan::CurrentCacheShortfall()
{
    // count the buffer count necessary to get from the current cache-root to the EOF.  return that, reduced by the count of buffers the cache already has.
    int nFutureBufs = BufferCountFromCharCount((*m_itCurrentCache)->FileLength() - (*m_itCurrentCache)->GetStartChar());
    if (nFutureBufs > BUFFER_COUNT)
        nFutureBufs = BUFFER_COUNT;
    return nFutureBufs - (*m_itCurrentCache)->GetBufferList()->Size();
}

void CCacheMan::CullUnneededBuffers()
{
    // figure out how many we should cull.
    int nNeeded = BUFFER_COUNT - m_lstUnusedBuffers.Size();
    int nUsefulBufs = CountUsefulBuffers();
    int nCurrentCacheShortfall = CurrentCacheShortfall();
    // since the consumer is active during this time, I cull one less buffer to avoid culling a useful one b/c of a race condition.
    ++nUsefulBufs;
    nNeeded -= nUsefulBufs;
    if (nNeeded < nCurrentCacheShortfall)
        nNeeded = nCurrentCacheShortfall;
    if (nNeeded > BUFFER_COUNT - BUFFERS_EMPTY_AT_SPINDOWN - BUFFERS_FULL_AT_SPINUP - m_lstUnusedBuffers.Size())
        nNeeded = BUFFER_COUNT - BUFFERS_EMPTY_AT_SPINDOWN - BUFFERS_FULL_AT_SPINUP - m_lstUnusedBuffers.Size();
    if (nNeeded < 1)
        return;
    
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:Cull %d\n",nNeeded);

    // first, free up any past caches.
    for (CacheListIterator cache = m_lstCaches.GetHead(); (*cache) != (*m_itCurrentCache); ++cache)
        if (nNeeded) {
            nNeeded -= (*cache)->GiveUpBuffers(nNeeded);
            if (nNeeded < 0) nNeeded = 0;
        }

    // next, chip into the current file
    if (nNeeded) {
        nNeeded -= (*m_itCurrentCache)->GiveUpBuffers((*m_itCurrentCache)->CountUnneededBuffers(), true);
        if (nNeeded < 0) nNeeded = 0;
    }
    // finally, work back from the future most cache.
    if (nNeeded)
        for (CacheListIterator cache = m_lstCaches.GetTail(); cache != m_lstCaches.GetEnd(); --cache)
        {
            // if somehow we still are trying to cull when we come back around to the current cache, just stop.
            if (cache == m_itCurrentCache)
                break;
            if ((*cache)->IsCullable())
            {
                nNeeded -= (*cache)->GiveUpBuffers(nNeeded);
                if (nNeeded < 0) nNeeded = 0;
                if (!nNeeded)
                    break;
            }
            // caches not marked cullable are guaranteed not to need culling, so the 
            // apparent need will be made up for by the buffers they already hold.
            else
                break;
        }
    if (nNeeded) {
        DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm: %d needed at end of cull\n",nNeeded);       
    }
    //ReportOnAllCaches("After Cull");
}

// award any unused buffers to current and future caches.
void CCacheMan::DistributeBuffersToCaches()
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:DistrBufs\n");
    //ReportOnAllCaches("Before DistributeToCaches");
    int iCache = 0;
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++iCache)
    {
        if (iCache < m_nPastCaches)
        {
            // if a past file exists and is open, then close it.
            if ((*cache)->GetFileNameRef() && (*cache)->IsFileOpen())
                (*cache)->CloseFile();
            // don't give it any caches, since it is in the past.
            continue;
        }
        // stop distribution if source is empty
        if (m_lstUnusedBuffers.Size() == 0)
            break;
        // the current cache doesn't need BOF support, just future buffers.
        // award all available buffers
        if (iCache == m_nPastCaches)
        {
            (*cache)->OpenFile();
            if ((*cache)->FileLength() == 0)
            {
                (*cache)->SetFileCorrupt(true);
                (*cache)->SetFileEOF();
                (*cache)->CloseFile();
                continue;
            }
            int nCharsToBuffer = (*cache)->FileLength() - (*cache)->GetStartChar();
            int nBufsNeeded = BufferCountFromCharCount(nCharsToBuffer);
            if ((*cache)->GetBufferList()->Size() < nBufsNeeded)
            {
                GetFreeBuffers(nBufsNeeded - (*cache)->GetBufferList()->Size(),(*cache)->GetBufferList());
                (*cache)->FixNextEmptyBuffer();
                if ((*cache)->GetBufferList()->Size() == nBufsNeeded)
                    (*cache)->SetPartialBufferIndex(nBufsNeeded - 1);
            }
        }
        else
        {   if (!(*cache)->GetFileNameRef())
                break;
            if ((*cache)->FileIsCorrupt())
                continue;
            (*cache)->RewindCacheToBOF();
            bool bOpen = (*cache)->IsFileOpen();
            if (!bOpen)
                (*cache)->OpenFile();
            int nCharsToBuffer = (*cache)->FileLength();
            if (nCharsToBuffer == 0)
            {
                (*cache)->SetFileCorrupt(true);
                (*cache)->SetFileEOF();
                (*cache)->CloseFile();
                continue;
            }
            if (!bOpen)
                (*cache)->CloseFile();
            int nBufsNeeded = BufferCountFromCharCount(nCharsToBuffer);
            if ((*cache)->GetBufferList()->Size() < nBufsNeeded)
            {
                GetFreeBuffers(nBufsNeeded - (*cache)->GetBufferList()->Size(),(*cache)->GetBufferList());
                (*cache)->FixNextEmptyBuffer();
            }
        }
    }
    ReportOnAllCaches("After DistributeToCaches");
}

bool CCacheMan::IsCurrentCacheReady()
{
    DEBUGP( DBG_CACHE_MAN, DBGLEV_TRACE, "cm:IsCurrRdy\n");
    if ((*m_itCurrentCache)->IsBOFBuffered() && (*m_itCurrentCache)->CountFullBuffers() > BUFFERS_FULL_AT_SPINUP)
        return true;
    return false;
}

void CCacheMan::ReadyCurrentCache()
{
    CFileCache* pCurrent = (*m_itCurrentCache);
    // rewind the cache, especially in case we're looping on one file and need to cull out future buffers.
    pCurrent->RewindCacheToBOF();
    CullUnneededBuffers();
    // rewind again, in case there were no buffers avail before
    pCurrent->RewindCacheToBOF();

    if (!pCurrent->IsEOFBuffered())
    {
        pCurrent->OpenFile();
        int nFileSize = pCurrent->FileLength();
        int nBufsNeeded = BufferCountFromCharCount(nFileSize);
        if (pCurrent->GetBufferList()->Size() < nBufsNeeded) {
            GetFreeBuffers(nBufsNeeded - pCurrent->GetBufferList()->Size(),pCurrent->GetBufferList());
            pCurrent->FixNextEmptyBuffer();
        }
    }
}

void CCacheMan::CloseAllFiles()
{
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache)
        (*cache)->CloseFile();
}

bool CCacheMan::IsCurrentCacheValid()
{
    int i = 0;
    for (CacheListIterator cache = m_lstCaches.GetHead(); cache != m_lstCaches.GetEnd(); ++cache, ++i)
    {
        if (i < m_nPastCaches)
            continue;
        if (i == m_nPastCaches)
            return (!(*cache)->FileIsCorrupt());
    }
}