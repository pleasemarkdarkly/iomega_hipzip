#include <util/debug/debug.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <playlist/common/Playlist.h>
#include <main/datastream/fatfile/CacheReferee.h>
#include <main/datastream/fatfile/FileCache.h>
#include <datastream/input/InputStream.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <errno.h>

#include <fs/fat/sdapi.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_FILE_CACHE, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE(DBG_FILE_CACHE);

#define __tolower(a) (a>='A'&&a<='Z' ? a + ('a'-'A') : a)
// hard error support constants
#define RE_READ_FUDGE_FACTOR (350)		// don't read right up to the last good char as reported, but rather this far back
#define DEADSPACE_CHARS 32				// insert this many null bytes between sections of good data to demarcate errors to the codec
#define LBA_BLOCK_SIZE (512)

CFileCache::CFileCache() : 	m_bValid(false),
                            m_szURL(0),
                            m_bFileOpen(false),
                            m_bNextEmptyOffEnd(false),
                            m_bNextFullOffEnd(false)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:ctor\n");
	m_nNextFullBuffer = new CCircIndex(0, BUFFER_COUNT);
	m_nNextEmptyBuffer = new CCircIndex(0, BUFFER_COUNT);
    Initialize();
    m_pFile = new CFatFileInputStream;
    m_itNextFullBuffer = 0;
    m_itNextEmptyBuffer = 0;
}

CFileCache::~CFileCache()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:dtor\n");
	delete m_nNextFullBuffer;
	delete m_nNextEmptyBuffer;
	delete m_pFile;
}

void CFileCache::InitForURL(const char* szUrl)
{
	DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:initting to %s\n",szUrl);
	SetURL(szUrl);
    Initialize();
}

void CFileCache::Initialize()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc: init\n");
	m_bEOF = false;
	m_bFileCorrupt = false;
	m_bHardErrorEncountered = false;
    m_nHardErrors = 0;
    m_nLastDeadSpace = -1;
	m_nStartChar = 0;
	m_nChars = 0;
	m_nNextEmptyBuffer->Set(0);
	m_nNextFullBuffer->Set(0);
	m_nPartialChars = -1;
	m_nPartialIndex = -1;
	m_nSkipStart = -1;
	m_nSkipExtent = 0;
	m_bSkipEOF = false;
    m_nSkipEOFChars = 0;
	m_nPartialIndex = -1;	
	m_nChars = 0;
	m_bValid = true;
	m_nFileOffset = 0;	
	m_nBufferOffset = 0;	
    m_bCullable = false;
    m_nFileLength = -1;
}

// give up all buffers and close the file.
void CFileCache::Minimize()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Minimize\n");
    if (m_bFileOpen)
        m_pFile->Close();
    if (m_lstBuffers.Size() > 0)
    {
        CCacheMan::GetInstance()->AppendFreeBuffers(&m_lstBuffers);
        m_lstBuffers.Clear();
    }
    Initialize();
    m_bFileOpen = false;
	m_bValid = false;
}

// minimize, but also detach from the URL.
void CFileCache::Deconfigure()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Deconf\n");
    Minimize();
    delete [] m_szURL;
    m_szURL = 0;
}

void CFileCache::Clear()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Clr\n");
	// remove a cache to make way for new
	m_nChars = 0;
	m_nNextFullBuffer->Set(0);
	m_nNextEmptyBuffer->Set(0);
}

// returns true if successfully performs a backfill in service of a seek to nFileOffset.
// tries to fill in some buffers before the current past-most data, so that we don't have to just
// throw away everything and rebuffer everything.  
bool CFileCache::BackFillForSeek(int nFileOffset)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:BackFill\n");
	if (nFileOffset < m_nStartChar)
	{
		// we're seeking back before the part of the file currently buffered.
		// see how many bufs back we're going, to get a sense of backfill-ability
        short nBufsAway = (m_nStartChar - nFileOffset) / BUFFER_SIZE;
		if (((m_nStartChar - nFileOffset) - (nBufsAway * BUFFER_SIZE)) > 0) nBufsAway++;
		short nBufsToGoBack = nBufsAway + BUFFERS_EMPTY_AT_SPINDOWN;
        // don't back up further than the BOF.
        if (nBufsToGoBack*BUFFER_SIZE > m_nStartChar)
			nBufsToGoBack = m_nStartChar / BUFFER_SIZE;
        // if we can save some time by performing a backfill operation, recycling some of the pastmost buffers' data, we'll do so.
		if (nBufsToGoBack < BUFFER_COUNT)
		{
            DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:BackFill %d\n",nBufsToGoBack);
            // cull unneeded buffers, and then prepend buffers as needed to support the backfill.
            CCacheMan::GetInstance()->CullUnneededBuffers();
            CCacheMan::GetInstance()->GetFreeBuffers(nBufsToGoBack, &m_lstBuffers, true);

            // notify the cache manager that some of the buffers will need to be counted as "past" data, i.e. before the read point.
            CCacheMan::GetInstance()->SetPastBuffersToFill( nBufsToGoBack - nBufsAway );
            // count full buffers
			short nFullBufs = (m_nChars)/BUFFER_SIZE;                   
			if ((nFullBufs * BUFFER_SIZE) < (m_nChars)) 
                nFullBufs++;	
            // figure out how many of the past-most buffers we'll get to keep, that the producer can "skip" during filling operations.
			m_nSkipStart = m_nNextEmptyBuffer->Get() - nFullBufs;
			if (m_nSkipStart < 0) 
				m_nSkipStart += BUFFER_COUNT;
            // set up where buffer filling will resume
			m_nNextEmptyBuffer->Set(m_nSkipStart - nBufsToGoBack);
			if (m_nNextEmptyBuffer->Get() < 0)													
				m_nNextEmptyBuffer->Set(m_nNextEmptyBuffer->Get() + BUFFER_COUNT);
            // tell the cache how many buffers it can skip, since they're already filled with relevant data.
			m_nSkipExtent = BUFFER_COUNT - nBufsToGoBack - BUFFERS_TO_PREBUFFER;	// first cut. if any are really empty, we'll adjust down.
			if ( m_nSkipExtent > nFullBufs)
				m_nSkipExtent = nFullBufs;
            // if we're skipping the eof buffer, we need to track that so the producer can restore that info later when it skips the eof buf.
            // we can't leave the m_bEOF flag on in the cache, or it won't fill right.  OTOH, we can leave the partial_buffer_chars count alone,
            // and it will just be reactivated as-is.
			short nLastSkipBuffer = m_nSkipStart + m_nSkipExtent - 1;
			if (nLastSkipBuffer >= BUFFER_COUNT)
				nLastSkipBuffer -= BUFFER_COUNT;
			if (( m_bEOF ) && ( nLastSkipBuffer == m_nPartialIndex))
				m_bSkipEOF = true;
			m_bEOF = false;
			m_nPartialIndex = -1;		// we leave m_nPartialChars, since there's no way to recompute it.
            // adjust the read-point counts.
            m_nFileOffset = nFileOffset;
			m_nBufferOffset = nFileOffset - ((nFileOffset / BUFFER_SIZE) * BUFFER_SIZE);
            for (int i = 0; i < m_nNextEmptyBuffer->Get() - m_nSkipStart + nBufsAway; ++i)
                DecrementNextFullBuffer();
			m_nStartChar -= nBufsToGoBack * BUFFER_SIZE;
			m_nChars = 0; 
            // perform the seek on the file, so when the producer starts, it'll get the right data
			WakeDrive();
			DEBUG_WAKE_DRIVE;
			if (m_pFile->Seek( IInputStream::SeekStart, m_nStartChar ) != m_nStartChar)
			{
				DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING,"C:SimpleSeek couldn't reach the desired point whilst backfilling for a buffered seek\n");
				m_nFileOffset = -1;
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
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:ForFill\n");
	int nCacheCeiling = CacheCeiling();
	short nBufsAway = (nFileOffset - nCacheCeiling) / BUFFER_SIZE;
	int nBufferOffset = nFileOffset % BUFFER_SIZE;
	if ( nBufferOffset > 0) nBufsAway++;
	if (nBufsAway <= BUFFERS_TO_PREBUFFER)
	{
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:FwdFill %d\n",nBufsAway);
        // cull unneeded buffers and postpend if necessary
        CCacheMan::GetInstance()->CullUnneededBuffers();
        CCacheMan::GetInstance()->DistributeBuffersToCaches();

	    // set where our new read buffer will be
        for (int i = 0; i < m_nNextEmptyBuffer->Get() + nBufsAway - 1 - m_nNextFullBuffer->Get(); ++i)
            IncrementNextFullBuffer();


        // tell the producer how many filled buffers will need to be marked "past" data, i.e., before the read point.
		CCacheMan::GetInstance()->SetPastBuffersToFill( nBufsAway - 1 );
        // adjust the read-point counts.
		m_nFileOffset = nFileOffset;
		m_nBufferOffset = nBufferOffset;
        return true;
    }
    return false;
}

// shift through the buffers to a new read point, maintaining vital counts and synchronizers along the way.
// this isn't thread safe, that responsibility is left to the caller.
// this is only intended for a shift outside the current buffer.  inbuffer shifts are inline.
// params:	nCharsToShift	= number of chars to shift
//			iUnitDirection	= signed unit, indicating direction (i.e., +1 or -1)
//          bBorrowingEmpty = if true, we intend to shift to the location without full semaphore counting, to avoid pausing the producer.
bool CFileCache::ShiftReadPoint(int nFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingEmpty)
{
	CCacheReferee* pRef = CCacheMan::GetInstance()->GetCacheReferee();

    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE,"fc:Shift %s%d\n",(iUnitDirection>0) ? "+" : "-", nCharsToShift);
	// figure out how many chars to skip in the current buffer
    int nToShiftInCurrent;
	if (iUnitDirection > 0)
		nToShiftInCurrent = BUFFER_SIZE - m_nBufferOffset;
	else
		nToShiftInCurrent = m_nBufferOffset;
    // move out of the current buffer
    (iUnitDirection>0) ? IncrementNextFullBuffer() : DecrementNextFullBuffer();

    // release the current buffer back into the appropriate empty/full count.
    // (epg,8/9/2000): if we're moving forward and had previously borrowed an empty, then right the count by skipping a post to empty.
	if (CCacheMan::GetInstance()->IsBorrowingEmpty() && iUnitDirection > 0)
		CCacheMan::GetInstance()->SetBorrowingEmpty(false);
	else
		(iUnitDirection>0)?pRef->PutEmpty():pRef->PutFull();
	// (epg,8/9/2000): if we're moving back and previously borrowed an empty (but not just now), then wait for an extra empty to right the count.
	if (CCacheMan::GetInstance()->IsBorrowingEmpty() && iUnitDirection < 0 && !bBorrowingEmpty)
	{
		CCacheMan::GetInstance()->SetBorrowingEmpty(false);
		(iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty();
	}

    // update position counters to account for portion of the current buffer skipped over
	m_nFileOffset += nToShiftInCurrent * iUnitDirection;
	nCharsToShift -= nToShiftInCurrent;
	m_nBufferOffset = 0;
    // skip over the centerspan of whole buffers
	while (nCharsToShift > BUFFER_SIZE)								
	{
		if ( ((iUnitDirection>0)?pRef->PeekFull():pRef->PeekEmpty()) > 0) {
			if ( ((iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty()) < 0) {
				DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING,"C:trywait failed with %d chars yet to shift, m_nReadBufferIndex=%d\n",nCharsToShift,m_nNextFullBuffer->Get());
			}
        }
		else {
			DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING,"C:none to get with %d chars yet to shift, m_nReadBufferIndex=%d\n",nCharsToShift,m_nNextFullBuffer->Get());
		}
        // consume the character to-skip count
		nCharsToShift -= BUFFER_SIZE;
        // move the buffer index
        (iUnitDirection>0) ? IncrementNextFullBuffer() : DecrementNextFullBuffer();

        // update the file offset count
        m_nFileOffset += BUFFER_SIZE * iUnitDirection;
        // update the empty/full semaphore
		(iUnitDirection>0)?pRef->PutEmpty():pRef->PutFull();
	}
	// skip into the new buffer to the correct place
	// if we're 'borrowing' an empty, then we're using it outside the classic accounting system with the idea of giving it back asap.  therefore
	// we won't actually wait on the semaphore, but rather remember that we owe one to the system.
	if (!bBorrowingEmpty && ((iUnitDirection>0)?pRef->PeekFull():pRef->PeekEmpty()) > 0) {
		if ( ((iUnitDirection>0)?pRef->GetFull():pRef->GetEmpty()) < 0) {
			DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING,"C:last trywait failed\n");
		}
    }
 	else {
		DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING,"C:last trywait failed 2\n");
	}
	m_nFileOffset += nCharsToShift * iUnitDirection;					
	m_nBufferOffset = (iUnitDirection > 0) ? nCharsToShift : (BUFFER_SIZE - nCharsToShift);
    // succeed
	return true;
}

// return true if successfully resets the cache to a new location in the file, throwing out all previously buffered data.
bool CFileCache::FullRebufferForSeek(int nFileOffset)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Rebuf %d\n",nFileOffset);
	// clear skip buffer state
	m_nSkipStart = -1;
	m_nSkipExtent = 0;
    // figure out where we'll start buffering from	
    int nStartOffset;
	nStartOffset = nFileOffset - (BUFFERS_TO_PREBUFFER * BUFFER_SIZE) - 10;
	if (nStartOffset < 0) nStartOffset = 0;
    // threshold the start offset to the nearest buffer boundary, rounding towards the BOF.
    short nSkipBufferCount;
    nSkipBufferCount = nStartOffset / BUFFER_SIZE;
	nStartOffset = nSkipBufferCount * BUFFER_SIZE;
    // set cache manager to fill some buffers as "past" data, i.e. before the readpoint.
    int nPastBufs = (nFileOffset - nStartOffset) / BUFFER_SIZE;
    CCacheMan::GetInstance()->SetPastBuffersToFill(nPastBufs);
	// configure the cache's readpoint counters
    m_nFileOffset = nFileOffset;
	m_nBufferOffset = nFileOffset - (nFileOffset / BUFFER_SIZE) * BUFFER_SIZE;
    // set cache positional counters, buffering will start at first buffer.
    m_nStartChar = nStartOffset;
	m_nChars = 0;
	m_bEOF = false;
	m_nPartialIndex = -1;
	m_nPartialChars = 0;

    CCacheMan::GetInstance()->CullUnneededBuffers();
    CCacheMan::GetInstance()->DistributeBuffersToCaches();
    // init both full and empty pointers to start
    ResetNextFullPointers();
    ResetNextEmptyPointers();
    // move the read point out past the past buffers.
    for (int i = 0; i < nPastBufs; ++i)
        IncrementNextFullBuffer();
	// seek the file to the new producer point	
	WakeDrive();
	DEBUG_WAKE_DRIVE;
    OpenFile();
	if (m_pFile->Seek(IInputStream::SeekStart, nStartOffset) != nStartOffset)
		return false;
	SleepDrive();
	DEBUG_SLEEP_DRIVE;
    // succeed
    return true;
}


void CFileCache::RestoreSkippedEOF()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:RestSkipEOF\n");
    m_bEOF = true;
	CCacheMan::GetInstance()->SetFirstSpinup(false);
	m_nPartialIndex = m_nSkipStart + m_nSkipExtent - 1;
	m_nPartialChars = m_nSkipEOFChars;
	if (m_nPartialIndex >= BUFFER_COUNT)
		m_nPartialIndex -= BUFFER_COUNT;
}

void CFileCache::SkipBuffers(CCacheReferee* pRef)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Skip %d\n",m_nSkipExtent);
	// skip over the region
	m_nChars += m_nSkipExtent * BUFFER_SIZE;
	// perform overflow cleanup
	while (m_nChars > BUFFER_COUNT * BUFFER_SIZE)
	{	
		m_nChars -= BUFFER_SIZE;
		m_nStartChar += BUFFER_SIZE;
	}
	// mark skipped bufs as full
	for (int i = 0; i < m_nSkipExtent; i++ )
	{
		if (pRef->GetEmpty() >= 0)
			pRef->PutFull();
        IncrementNextEmptyBuffer();
	}
	// if restoring eof buffer from a backfill state, set up the cache flags
	if (m_bSkipEOF)
        RestoreSkippedEOF();
	// move the file pointer out to the new read location (past skip bufs), or to eof
	int nCharsToSkip = (m_nSkipExtent - 1) * BUFFER_SIZE;
	if (m_bSkipEOF)
		nCharsToSkip += m_nPartialChars;
	else
		nCharsToSkip += BUFFER_SIZE;
	m_pFile->Seek( IInputStream::SeekCurrent, nCharsToSkip);
	// clear out skip flags
	m_nSkipStart = -1;
	m_nSkipExtent = 0;
	m_bSkipEOF = false;	// consume the status variable's state
}

void CFileCache::NormalizeFileCursor()
{
	int startchar = m_nChars + m_nStartChar;
    if (m_pFile->Position() != startchar)
    {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:NormFileCrsr sk %d to %d\n",m_pFile->Position(),startchar);
        if (m_pFile->Seek(IInputStream::SeekStart, startchar) != startchar) {
            DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "FC:NormFileCrsr sk %d != %d!\n",m_pFile->Position(),startchar);
        }
    }
}

int CFileCache::FilePositionFromBufferListIndex(int nListIndex)
{
    return (m_nStartChar / BUFFER_SIZE + nListIndex);
}

void CFileCache::FillBuffer()
{
	CCacheReferee* pRef = CCacheMan::GetInstance()->GetCacheReferee();
	
    // which buffer to read into
    unsigned char* pBuffer = (*m_itNextEmptyBuffer);
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:F phys %d, fil %d, idx %d\n",CCacheMan::GetInstance()->BufferIndexFromLocation(pBuffer), 
        FilePositionFromBufferListIndex(m_nNextEmptyBuffer->Get()), m_nNextEmptyBuffer->Get());

	// read from the file
	int result = m_pFile->Read((void*)pBuffer, BUFFER_SIZE);
	// if the first read failed, try waking the drive and repeating
	if (result == -1 && errno != EREAD)
	{
		WakeDrive();
		result = m_pFile->Read((void*)pBuffer, BUFFER_SIZE);
	}
	if(result == -1)
    {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "fc:rd err %d\n",m_nChars + m_nStartChar);
	    if ( errno == EREAD ) 
            result = HandleHardError(pBuffer);
	    else
	    {
		    // mark the file as corrupt and fail out
		    DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "fc: unknown err\n");
		    m_bFileCorrupt = true;
		    m_bEOF = true;
            CloseFile();
		    return;
	    }
    }
	if (result != BUFFER_SIZE)
	{
		// a partial read signifies the end of file
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:EOF %d\n",m_nChars + m_nStartChar + result);
		m_bEOF = true;
        if (m_nFileLength > m_nChars + m_nStartChar + result)
            m_nFileLength = m_nChars + m_nStartChar + result;
        CloseFile();
		if (result <= 0)
		{
			// a zero result means the last char of the file was in the previous read, right at the end
			// move pointers back and consider that previous buf as the eof buf.
			m_nPartialChars = 0;
			if (pRef->PeekFull() <= 0)
				// in case the reader is waiting on this fill, notify it that nothing is coming
				pRef->JogFull();
		}
		else
			m_nPartialChars = result;
		m_nPartialIndex = m_nNextEmptyBuffer->Get();
	}
	// update cache count of chars
	m_nChars += result;
	while (m_nChars > BUFFER_COUNT * BUFFER_SIZE)
	{	
		m_nChars -= BUFFER_SIZE;
		m_nStartChar += BUFFER_SIZE;
	}
    if (!m_bEOF)
        IncrementNextEmptyBuffer();
    return;
}

// returns true if a deadspace was added.  updates the byte counters to reflect this.
bool CFileCache::InsertBufferDeadSpace(unsigned char* pBuffer, int &nBytesToRead, int &nBytesAlreadyRead)
{
    if (m_nLastDeadSpace != (int)pBuffer)
    {
        int length = DEADSPACE_CHARS;
        if (DEADSPACE_CHARS > nBytesToRead)
            length = nBytesToRead;
	    memset(pBuffer,0,length);
        nBytesToRead -= length;
        nBytesAlreadyRead += length;
        m_nLastDeadSpace = (int)pBuffer + nBytesAlreadyRead;
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:DeadSpc %d\n",length);
        return length;
    }
    return 0;
}

int CFileCache::HandleHardError(unsigned char* pBuffer)
{
    // (epg,10/18/2001): TODO: if hard error handling is required on hdd, activate it in fs/fat.
	// a hard error has encountered, so there is a region of the disk that is unavailable for reading.
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:HardErr %d\n",m_nChars + m_nStartChar);
	m_nHardErrors = 0;
	bool bHardError=true;
	int nPreReadPos = m_nChars + m_nStartChar;
	int nBytesToRead = BUFFER_SIZE;
	int nBytesAlreadyRead = 0;
	m_nLastDeadSpace = -1;
    int result;
	// keep trying to read after the error until we either find the end or give up from many retries
	while (bHardError && m_nHardErrors < CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
	{
		m_nHardErrors++;
		bHardError = false;
		// find out how much of the read was considered good
        int nNextGoodLBAStart;
        if (!ReReadBeforeHardError(pBuffer, nPreReadPos, nNextGoodLBAStart))
        {
			if (errno == EREAD)
			{
				// another hard error, so loop again
				bHardError = true;
				// leave a zeroed-out gap between non-contiguous regions of the file.
                InsertBufferDeadSpace(pBuffer,nBytesToRead,nBytesAlreadyRead);
				continue;
			}
			else
			{
				// non hard-error failure
				DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "unknown po read error in pre-harderror reread\n");
				result = nBytesAlreadyRead;
			}
		}
		else
		{
			// good read, so track read progress
			nBytesToRead -= result;
			nBytesAlreadyRead += result;
		}

        InsertBufferDeadSpace(pBuffer,nBytesToRead,nBytesAlreadyRead);
		// now move past and start reading data again.
		int nLoc = m_pFile->Seek(IInputStream::SeekStart,nNextGoodLBAStart);
		if (nLoc != nNextGoodLBAStart) {
			DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "sk ret %d != %d\n",nLoc,nNextGoodLBAStart);
		}
		if (nBytesToRead>0)
		{	
			// still more to read
			result = m_pFile->Read((void*) (pBuffer + nBytesAlreadyRead), nBytesToRead);
			if (result == -1)
			{
				if (errno == EREAD)
				{
					// we got another hard error, so update state and fall through to loop again
					nPreReadPos = nNextGoodLBAStart;
					bHardError = true;
                    InsertBufferDeadSpace(pBuffer,nBytesToRead,nBytesAlreadyRead);
				}
				else
				{
					// non hard error fault, just eject
					DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "unknown error in post HE read\n");
					result = nBytesAlreadyRead;
				}
			}
			else
			{
				// the read was good, so we're back on target.
				nBytesAlreadyRead += result;
				nBytesToRead -= result;
				if (nBytesToRead != 0) {
					DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "!nonzero bytes to read after HE handling\n");
				}
				bHardError= false;
				result = nBytesAlreadyRead;		// eof handler will see this value
			}
		}
	}
	if (m_nHardErrors >= CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
	{
		// the disk failed too many times in a row.  give up on the file
		DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "!%d consecutive hard errors encountered, marking file as bad\n",CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED);
		m_nHardErrors = 0;
		m_bFileCorrupt = true;
		result = nBytesAlreadyRead;
	}
    return result;
}

bool CFileCache::ReReadBeforeHardError(unsigned char* pBuffer, int nPreReadPos, int &nNextGoodLBAStart)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:ReRdPreHardErr %d\n",nPreReadPos);
	int nValidChars = pc_get_valid_read_data();
	int nErrorPos = nPreReadPos + nValidChars;				// this is where the error occured
	int nErrorLBAStart = nErrorPos - (nErrorPos % 512);		// this is the start of the bad lba
	int nCurrentPos = m_pFile->Seek(IInputStream::SeekCurrent,0);
	// move the file pointer to before the error
	if (nCurrentPos != nPreReadPos)
	{
		int seekret = m_pFile->Seek(IInputStream::SeekStart, nPreReadPos);
		if (seekret != nPreReadPos)	{
			DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "apparent reseek failure ret %d != prepos %d\n", seekret, nPreReadPos);
		}
	}
	// re read the portion of data that was good before the hard error.
	int nCharsToReread = nValidChars-RE_READ_FUDGE_FACTOR;
	if (nCharsToReread < 0)	nCharsToReread = 0;
	int result = m_pFile->Read((void*) pBuffer, nCharsToReread);
    nNextGoodLBAStart = nErrorLBAStart + LBA_BLOCK_SIZE;
    return (result != -1);
}

bool CFileCache::SetFile(const char* szUrl)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:SetFile\n");
    if (m_bFileOpen && m_pFile)
    {
        m_pFile->Close();
        m_bFileOpen = false;
    }
    SetURL(szUrl);
    return true;
}

// return the count of buffers successfully given up.
// if bFromStart, the buffers are being removed from the start of the cache.
int CFileCache::GiveUpBuffers(int nRequested, bool bFromHead)
{
    int nBufs = m_lstBuffers.Size();
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:GiveBufs %d of %d\n",nRequested,nBufs);
    // if we're already empty, don't bother deconfiguring.
    if (!nBufs)
        return 0;
    // removing a few buffers from the beginning of the cache makes the 
    // cache relatively useless.  removing them all does the same.
    if (nBufs <= nRequested) 
    {
        // give up all caches, and close the file; hang on to the URL for now.
        if (this->IsCurrentCache()) {
            DEBUGP( DBG_FILE_CACHE, DBGLEV_ERROR, "FC:current cache asked to give up all buffers!\n");
        }
        Minimize();
        return nBufs;
    }
    // this will only happen on the current track.
    else if (bFromHead)
    {
        BufferList lstFreed;
        for (int i = 0; i < nRequested; ++i)
        {
            lstFreed.PushBack(m_lstBuffers.PopFront());
            m_nChars -= BUFFER_SIZE;
            m_nStartChar += BUFFER_SIZE;
        }
        // back up the counters to keep them a valid index into lstBuffers.
        m_nNextEmptyBuffer->Set(m_nNextEmptyBuffer->Get() - nRequested);
        m_nNextFullBuffer->Set(m_nNextFullBuffer->Get() - nRequested);
        if (m_bEOF)
            m_nPartialIndex -= nRequested;
        if (m_nSkipStart > -1)
            m_nSkipStart -= nRequested;
        CCacheMan::GetInstance()->AppendFreeBuffers(&lstFreed);
        return nRequested;
    }
    else
    {
        while (m_nNextEmptyBuffer->Get() > m_lstBuffers.Size() - nRequested - 1)
            DecrementNextEmptyBuffer();

        BufferList lstFreed;
        int nNeeded = nRequested;
        if (m_bEOF)
        {
            m_bEOF = false;
            m_nPartialIndex = -1;
            m_nChars -= m_nPartialChars;
            --nNeeded;
            --nBufs;
            lstFreed.PushBack(m_lstBuffers.PopBack());
        }
        m_nChars -= nNeeded * BUFFER_SIZE;
        for (int i = 0; i < nNeeded; ++i)
            lstFreed.PushBack(m_lstBuffers.PopBack());
        CCacheMan::GetInstance()->AppendFreeBuffers(&lstFreed);
        return nRequested;
    }
}

void CFileCache::SetURL(const char* szURL)
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:SetUrl\n");
    delete [] m_szURL;
    m_szURL = new char[strlen(szURL) + 1];
    strcpy (m_szURL,szURL);
}

void CFileCache::OpenFile()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:OpenFile\n");
    if (!m_bFileOpen && m_szURL)
    {
        m_bFileOpen = true;
        m_pFile = CDataSourceManager::GetInstance()->OpenInputStream(m_szURL);
        if (!m_pFile)
        {
            m_bFileCorrupt = true;
            m_bEOF = true;
            m_nFileLength = 0;
            //CPlayManager::GetInstance()->GetContentManager()->DeleteMediaRecord(CPlayManager::GetInstance()->GetContentManager()->GetMediaRecord(m_szURL)->GetID());
        }
    }
    if (!m_szURL) {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "FC:Tried to OpenFile null url\n");
    }
}

void CFileCache::CloseFile()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:CloseFile\n");
    if (m_bFileOpen && m_pFile)
    {
        m_bFileOpen = false;
        m_pFile->Close();
    }
}

void CFileCache::IncrementNextFullBuffer()
{
    if (m_bNextFullOffEnd) 
    {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "FC:Trying to IncNextFull that's already OffEnd\n");
        return;
    }
    ++m_itNextFullBuffer;
    if (m_itNextFullBuffer == m_lstBuffers.GetEnd()) 
        m_bNextFullOffEnd = true;
    m_nNextFullBuffer->Inc();
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:IncNxtFull %d\n",m_nNextFullBuffer->Get());
}

void CFileCache::DecrementNextFullBuffer()
{
    if (m_bNextFullOffEnd)
    {
        m_bNextFullOffEnd = false;
        m_itNextFullBuffer = m_lstBuffers.GetTail();
        m_nNextFullBuffer->Dec();
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:DecNxtFull %d AOE\n",m_nNextFullBuffer->Get());
    }
    else if (m_itNextFullBuffer != m_lstBuffers.GetHead())
    {
        --m_itNextFullBuffer;
        m_nNextFullBuffer->Dec();
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:DecNxtFull %d\n",m_nNextFullBuffer->Get());
    }
}

void CFileCache::IncrementNextEmptyBuffer()
{
    if (m_bNextEmptyOffEnd)
    {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "FC:Trying to IncNextEmpty that's already 0\n");
        return;
    }
    ++m_itNextEmptyBuffer;
    if (m_itNextEmptyBuffer == m_lstBuffers.GetEnd()) 
        m_bNextEmptyOffEnd = true;
    m_nNextEmptyBuffer->Inc();
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:IncNextEmpty %d\n",m_nNextEmptyBuffer->Get());
}

void CFileCache::DecrementNextEmptyBuffer()
{
    if (m_bNextEmptyOffEnd)
    {
        m_bNextEmptyOffEnd = false;
        m_itNextEmptyBuffer = m_lstBuffers.GetTail();
        m_nNextEmptyBuffer->Dec();
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:DecNextEmpty %d AOE\n",m_nNextEmptyBuffer->Get());
    }
    else if (m_itNextEmptyBuffer != m_lstBuffers.GetHead())
    {
        --m_itNextEmptyBuffer;
        m_nNextEmptyBuffer->Dec();
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:DecNextEmpty %d\n",m_nNextEmptyBuffer->Get());
    }
}

void CFileCache::FixNextEmptyBuffer()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:FixNextEmpty %d\n",m_nNextEmptyBuffer->Get());
    m_itNextEmptyBuffer = m_lstBuffers.GetHead();
    int nIndex = 0;
    while (nIndex++ < m_nNextEmptyBuffer->Get())
        ++m_itNextEmptyBuffer;
    if (m_itNextEmptyBuffer != 0)
        m_bNextEmptyOffEnd = false;
    else {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "FC:FixNextEmpty still zero!\n");
    }
}

void CFileCache::PrependBuffersForBOF()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:Prepend4BOF\n");
    int nPrepend = m_nStartChar / BUFFER_SIZE;
    int nFull = CountFullBuffers();
    CCacheMan::GetInstance()->GetFreeBuffers(nPrepend, &m_lstBuffers, true);
    m_nStartChar = 0;
    m_nChars = 0;
    // figure out how many of the past-most buffers we'll get to keep, that the producer can "skip" during filling operations.
	m_nSkipStart = nPrepend;
    // tell the cache how many buffers it can skip, since they're already filled with relevant data.
	m_nSkipExtent = nFull;
	// if we're skipping the eof buffer, we need to track that so the producer can restore that info later when it skips the eof buf.
    // we can't leave the m_bEOF flag on in the cache, or it won't fill right.  OTOH, we can leave the partial_buffer_chars count alone,
    // and it will just be reactivated as-is.
	short nLastSkipBuffer = m_nSkipStart + m_nSkipExtent - 1;
	if (( m_bEOF ) && ( nLastSkipBuffer == m_nPartialIndex))
		m_bSkipEOF = true;
	m_bEOF = false;
	m_nPartialIndex = -1;		// we leave m_nPartialChars, since there's no way to recompute it.
    if ((*CCacheMan::GetInstance()->GetCurrentCache()) == this)
        CCacheMan::GetInstance()->GetCacheReferee()->Reset(0);
}

void CFileCache::RewindCacheToBOF()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:RewToBOF\n");
    m_nFileOffset = m_nStartChar;
    m_nBufferOffset = 0;
    if (!AreBuffersBOFRooted())
        PrependBuffersForBOF();
    // try to make sure we have at least one buf, so we can get the buffer pointers going.
    if (m_lstBuffers.Size() == 0)
    {
        CCacheMan::GetInstance()->GetFreeBuffers(1, &m_lstBuffers);
        ResetNextEmptyPointers();
    }
    ResetNextFullPointers();
    // set up where buffer filling will resume
    if (m_lstBuffers.Size() == 0) {
        DEBUGP( DBG_FILE_CACHE, DBGLEV_WARNING, "fc:unexpected no bufs in rewind\n");
    }
}

void CFileCache::ResetNextFullPointers()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:ResetNxtFull\n");
    m_nNextFullBuffer->Set(0);
    m_itNextFullBuffer = m_lstBuffers.GetHead();
    m_bNextFullOffEnd = false;
}

void CFileCache::ResetNextEmptyPointers()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:ResetNxtEmpty\n");
	m_nNextEmptyBuffer->Set(0);
    m_itNextEmptyBuffer = m_lstBuffers.GetHead();
    m_bNextEmptyOffEnd = false;
}

int CFileCache::FileLength()
{
    if (m_nFileLength == -1 && (m_szURL))
    {
        OpenFile();
        m_nFileLength = m_pFile->Length();
        CloseFile();
        if (m_bEOF)   {
		    m_nFileLength = m_nChars + m_nStartChar + 1;
        }
    }
    return m_nFileLength;
}

int CFileCache::CountUnneededBuffers()
{
    int nSpareChars = m_nFileOffset - m_nBufferOffset - m_nStartChar;
    int nSpareBufs = nSpareChars/BUFFER_SIZE - BUFFERS_EMPTY_AT_SPINDOWN;
    if (nSpareBufs < 0)
        nSpareBufs = 0;
    return nSpareBufs;
}

int CFileCache::CountPreReadPointChars()
{
    return m_nFileOffset - m_nStartChar;
}

bool CFileCache::AreAllBuffersFull()
{
    DEBUGP( DBG_FILE_CACHE, DBGLEV_TRACE, "fc:AreAllFull %d %d\n",CountFullBuffers(),m_lstBuffers.Size());
    return (CountFullBuffers() == m_lstBuffers.Size());
}
