#include <util/debug/debug.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <playlist/common/Playlist.h>
#include <main/datastream/fatfile/CacheReferee.h>

#include <main/datastream/fatfile/FileCache.h>

#include <datastream/fatfile/FileInputStream.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FILE_CACHE, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_FILE_CACHE);

#define __tolower(a) (a>='A'&&a<='Z' ? a + ('a'-'A') : a)

CFileCache::CFileCache(int nBuffers) : 	
							m_nStartChar(0),
							m_nChars (0),
							m_nBuffers (nBuffers),
							m_nFileBookmark(0),
							m_nBufferBookmark(0),
							m_nReadBufferIndex(-1),
							m_nBuffersToSpare(BUFFERS_TO_SPARE),
							m_nPartialIndex(-1),
							m_nPartialChars(-1),
							m_nSkipStart(-1),
							m_nSkipExtent(0),
							m_bSkipEOF(false),
							m_nSkipEOFChars(0),
							m_aBuffers(0),
							m_bExtended(false),
							m_bValid(false),
							m_bEOF(false),
							m_bFileCorrupt(false),
							m_bHardErrorEncountered(false),
							m_bID3v1LookupDone(false),
							m_pNext(0),
							m_pPrev(0)
{
	m_nNextFullBuffer = new CCircIndex(0, m_nBuffers-1);
	m_nNextEmptyBuffer = new CCircIndex(0, m_nBuffers-1);
	m_pFile = new CFatFileInputStream();
}

CFileCache::~CFileCache()
{
	delete m_nNextFullBuffer;
	delete m_nNextEmptyBuffer;
	delete m_pFile;
}

void CFileCache::InitFromFile(const char* szFilename)
{
	DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO, "FC:Cache at %d initting to %s\n",(int) this, szFilename);
	m_pFile->Open(szFilename);
	m_bEOF = false;
	m_bID3v1LookupDone = false;
	m_bFileCorrupt = false;
	m_bHardErrorEncountered = false;
	m_nStartChar = 0;
	m_nChars = 0;
	m_nNextEmptyBuffer->Set(0);
	m_nNextFullBuffer->Set(0);
	m_nPartialChars = -1;
	m_nPartialIndex = -1;
	m_nSkipStart = -1;
	m_nSkipExtent = 0;
	m_bSkipEOF = false;
	m_nPartialIndex = -1;	
	m_nStartChar = 0;
	m_nChars = 0;
	m_bValid = true;
	m_nFileBookmark = 0;	
	m_nBufferBookmark = 0;	
	m_nReadBufferIndex = -1;
}

void CFileCache::UnloadFile()
{
	m_pFile->Close();
	m_bValid = false;
}

void CFileCache::Clear()
{
	// remove a cache to make way for new
	m_nChars = 0;
	m_nNextFullBuffer->Set(0);
	m_nNextEmptyBuffer->Set(0);
}

void CFileCache::Rewind()
{
	m_nNextFullBuffer->Set(0);
	m_nFileBookmark = 0;	
	m_nBufferBookmark = 0;	
	m_nReadBufferIndex = -1;
}	

CFatFileInputStream* CFileCache::GetFile()
{ 
    return m_pFile; 
}

bool CFileCache::FileIsCorrupt()
{
    return m_bFileCorrupt;
}

bool CFileCache::FileAtEOF()
{
    return m_bEOF;
}

void CFileCache::SetFileEOF()
{
    m_bEOF = true;
}

// calculate the highest file-offset currently in memory.
int CFileCache::CacheCeiling()
{
    return m_nChars + m_nStartChar - 1;
}

// returns true if successfully performs a backfill in service of a seek to nFileOffset.
// tries to fill in some buffers before the current past-most data, so that we don't have to just
// throw away everything and rebuffer everything.  
bool CFileCache::BackFillForSeek(int nFileOffset)
{
	if (nFileOffset < m_nStartChar)
	{
		// we're seeking back before the part of the file currently buffered.
		// see how many bufs back we're going, to get a sense of backfill-ability
        short nBufsAway = (m_nStartChar - nFileOffset) / BUFFER_SIZE;
		if (((m_nStartChar - nFileOffset) - (nBufsAway * BUFFER_SIZE)) > 0) nBufsAway++;
		short nBufsToGoBack = nBufsAway + m_nBuffersToSpare;
        // don't back up further than the BOF.
        if (nBufsToGoBack*BUFFER_SIZE > m_nStartChar)
			nBufsToGoBack = m_nStartChar / BUFFER_SIZE;
        // if we can save some time by performing a backfill operation, recycling some of the pastmost buffers' data, we'll do so.
		if (nBufsToGoBack < m_nBuffers)
		{
            // notify the cache manager that some of the buffers will need to be counted as "past" data, i.e. before the read point.
            CCacheMan::GetInstance()->SetPastBuffersToFill( nBufsToGoBack - nBufsAway );
            // count full buffers
			short nFullBufs = (m_nChars)/BUFFER_SIZE;                   
			if ((nFullBufs * BUFFER_SIZE) < (m_nChars)) 
                nFullBufs++;	
            // figure out how many of the past-most buffers we'll get to keep, that the producer can "skip" during filling operations.
			m_nSkipStart = m_nNextEmptyBuffer->Get() - nFullBufs;
			if (m_nSkipStart < 0) 
				m_nSkipStart += m_nBuffers;
            // set up where buffer filling will resume
			m_nNextEmptyBuffer->Set(m_nSkipStart - nBufsToGoBack);
			if (m_nNextEmptyBuffer->Get() < 0)													
				m_nNextEmptyBuffer->Set(m_nNextEmptyBuffer->Get() + m_nBuffers);
            // tell the cache how many buffers it can skip, since they're already filled with relevant data.
			m_nSkipExtent = m_nBuffers - nBufsToGoBack - m_nBuffersToSpare;	// first cut. if any are really empty, we'll adjust down.
			if ( m_nSkipExtent > nFullBufs)
				m_nSkipExtent = nFullBufs;
            // if we're skipping the eof buffer, we need to track that so the producer can restore that info later when it skips the eof buf.
            // we can't leave the m_bEOF flag on in the cache, or it won't fill right.  OTOH, we can leave the partial_buffer_chars count alone,
            // and it will just be reactivated as-is.
			short nLastSkipBuffer = m_nSkipStart + m_nSkipExtent - 1;
			if (nLastSkipBuffer >= m_nBuffers)
				nLastSkipBuffer -= m_nBuffers;
			if (( m_bEOF ) && ( nLastSkipBuffer == m_nPartialIndex))
				m_bSkipEOF = true;
			m_bEOF = false;
			m_nPartialIndex = -1;		// we leave m_nPartialChars, since there's no way to recompute it.
            // adjust the read-point counts.
            m_nFileBookmark = nFileOffset;
			m_nBufferBookmark = nFileOffset - ((nFileOffset / BUFFER_SIZE) * BUFFER_SIZE);
			m_nReadBufferIndex = m_nSkipStart - nBufsAway;
			if (m_nReadBufferIndex < 0)
				m_nReadBufferIndex += m_nBuffers;
			m_nStartChar -= nBufsToGoBack * BUFFER_SIZE;
			m_nChars = 0; 
            // perform the seek on the file, so when the producer starts, it'll get the right data
			WakeDrive();
			DEBUG_WAKE_DRIVE;
			if (m_pFile->Seek( IInputStream::SeekStart, m_nStartChar ) != m_nStartChar)
			{
				DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:SimpleSeek couldn't reach the desired point whilst backfilling for a buffered seek\n");
				m_nFileBookmark = -1;
			}
            SleepDrive();
			DEBUG_SLEEP_DRIVE;
            return true;
		}
	}
    return false;
}

bool CFileCache::ForwardFillForSeek(int nFileOffset)
{
	int nCacheCeiling = CacheCeiling();
	short nBufsAway = (nFileOffset - nCacheCeiling) / BUFFER_SIZE;
	int nBufferOffset = nFileOffset % BUFFER_SIZE;
	if ( nBufferOffset > 0) nBufsAway++;
	if (nBufsAway <= m_nBuffersToSpare)
	{
        // set where our new read buffer will be
		m_nReadBufferIndex = m_nNextEmptyBuffer->Get() + nBufsAway - 1;
        // tell the producer how many filled buffers will need to be marked "past" data, i.e., before the read point.
		CCacheMan::GetInstance()->SetPastBuffersToFill( nBufsAway - 1 );
        // adjust the read-point counts.
		m_nFileBookmark = nFileOffset;
		m_nBufferBookmark = nBufferOffset;
        return true;
    }
    return false;
}

// return true if the offset is currently buffered in the cache.  not thread safe.
bool CFileCache::OffsetIsBuffered (int nFileOffset)
{
    return ((nFileOffset >= m_nStartChar) && (nFileOffset <= m_nStartChar + m_nChars));
}

// loop over or under as needed, after an increment or decrement.  *not general enough for
// non-unit additions/subtractions.
void CFileCache::NormalizeReadBufferIndex()
{
    if (m_nReadBufferIndex < 0)
        m_nReadBufferIndex = m_nBuffers-1;
    if (m_nReadBufferIndex == m_nBuffers)
        m_nReadBufferIndex = 0;
}

// shift through the buffers to a new read point, maintaining vital counts and synchronizers along the way.
// this isn't thread safe, that responsibility is left to the caller.
// this is only intended for a shift outside the current buffer.  inbuffer shifts are inline.
// params:	nCharsToShift	= number of chars to shift
//			iUnitDirection	= signed unit, indicating direction (i.e., +1 or -1)
//          bBorrowingEmpty = if true, we intend to shift to the location without full semaphore counting, to avoid pausing the producer.
bool CFileCache::ShiftReadPoint(int nFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingEmpty)
{
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CCacheReferee* pRef = pCacheMan->GetCacheReferee();

    DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:ShiftReadPoint(toshift=%d,dir=%d,borrow=%d\n",nCharsToShift,iUnitDirection,bBorrowingEmpty);
	// figure out how many chars to skip in the current buffer
    int nToShiftInCurrent;
	if (iUnitDirection > 0)
		nToShiftInCurrent = BUFFER_SIZE - m_nBufferBookmark;
	else
		nToShiftInCurrent = m_nBufferBookmark;
    // move out of the current buffer
	m_nReadBufferIndex += iUnitDirection;
    NormalizeReadBufferIndex();
	
    // release the current buffer back into the appropriate empty/full count.
    // (epg,8/9/2000): if we're moving forward and had previously borrowed an empty, then right the count by skipping a post to empty.
	if (pCacheMan->IsBorrowingEmpty() && iUnitDirection > 0)
		pCacheMan->SetBorrowingEmpty(false);
	else
		(iUnitDirection>0)?pRef->PutEmpty():pRef->PutFull();
	// (epg,8/9/2000): if we're moving back and previously borrowed an empty (but not just now), then wait for an extra empty to right the count.
	if (pCacheMan->IsBorrowingEmpty() && iUnitDirection < 0 && !bBorrowingEmpty)
	{
		pCacheMan->SetBorrowingEmpty(false);
		(iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty();
	}

    // update position counters to account for portion of the current buffer skipped over
	m_nFileBookmark += nToShiftInCurrent * iUnitDirection;
	nCharsToShift -= nToShiftInCurrent;
	m_nBufferBookmark = 0;
    // skip over the centerspan of whole buffers
	while (nCharsToShift > BUFFER_SIZE)								
	{
		if ((iUnitDirection>0)?pRef->PeekFull():pRef->PeekEmpty() > 0) 
			if ((iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty() < 0) {
				DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:trywait failed with %d chars yet to shift, m_nReadBufferIndex=%d\n",nCharsToShift,m_nReadBufferIndex);
			}
		else {
			DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:none to get with %d chars yet to shift, m_nReadBufferIndex=%d\n",nCharsToShift,m_nReadBufferIndex);
		}
        // consume the character to-skip count
		nCharsToShift -= BUFFER_SIZE;
        // move the buffer index
		m_nReadBufferIndex += iUnitDirection;
        NormalizeReadBufferIndex();
        // update the file offset count
        m_nFileBookmark += BUFFER_SIZE * iUnitDirection;
        // update the empty/full semaphore
		(iUnitDirection>0)?pRef->PutEmpty():pRef->PutFull();
	}
	// skip into the new buffer to the correct place
	// if we're 'borrowing' an empty, then we're using it outside the classic accounting system with the idea of giving it back asap.  therefore
	// we won't actually wait on the semaphore, but rather remember that we owe one to the system.
	if (!bBorrowingEmpty && (iUnitDirection>0)?pRef->PeekFull():pRef->PeekEmpty() > 0)
		if ((iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty() < 0) {
			DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:last trywait failed\n");
		}
 	else {
		DEBUGP( DBG_FILE_CACHE, DBGLEV_INFO,"C:last trywait failed 2\n");
	}
	m_nFileBookmark += nCharsToShift * iUnitDirection;					
	m_nBufferBookmark = (iUnitDirection > 0) ? nCharsToShift : (BUFFER_SIZE - nCharsToShift);
    // succeed
	return true;
}

// return true if successfully resets the cache to a new location in the file, throwing out all previously buffered data.
bool CFileCache::FullRebufferForSeek(int nFileOffset)
{
    // clear skip buffer state
	m_nSkipStart = -1;
	m_nSkipExtent = 0;
    // figure out where we'll start buffering from	
    int nStartOffset;
	nStartOffset = nFileOffset - (BUFFERS_TO_SPARE * BUFFER_SIZE) - 10;
	if (nStartOffset < 0) nStartOffset = 0;
    // threshold the start offset to the nearest buffer boundary, rounding towards the BOF.
    short nSkipBufferCount;
    nSkipBufferCount = nStartOffset / BUFFER_SIZE;
	nStartOffset = nSkipBufferCount * BUFFER_SIZE;
    // set cache manager to fill some buffers as "past" data, i.e. before the readpoint.
    int nPastBufs = (nFileOffset - nStartOffset) / BUFFER_SIZE;
    CCacheMan::GetInstance()->SetPastBuffersToFill(nPastBufs);
	// configure the cache's readpoint counters
    m_nFileBookmark = nFileOffset;
	m_nBufferBookmark = nFileOffset - (nFileOffset / BUFFER_SIZE) * BUFFER_SIZE;
	m_nReadBufferIndex = nPastBufs;
    // set cache positional counters, buffering will start at first buffer.
    m_nStartChar = nStartOffset;
	m_nChars = 0;
	m_bEOF = false;
	m_nPartialIndex = -1;
	m_nPartialChars = 0;
	m_nNextEmptyBuffer->Set(0);
    // seek the file to the new producer point	
	bool bFailed = false;
	WakeDrive();
	DEBUG_WAKE_DRIVE;
	if (m_pFile->Seek(IInputStream::SeekStart, nStartOffset) != nStartOffset)
		return false;
	SleepDrive();
	DEBUG_SLEEP_DRIVE;
    // succeed
    return true;
}