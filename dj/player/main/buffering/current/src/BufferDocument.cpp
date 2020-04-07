#include <main/buffering/BufferDebug.h>
#include <core/playmanager/PlayManager.h>
#include <datastream/input/InputStream.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <fs/fat/sdapi.h>
#include <util/debug/debug.h>
#include <util/utils/utils.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/BufferAccountant.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/BufferReader.h>
#include <playlist/common/Playlist.h>
#include <errno.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_DOC, DBGLEV_DEFAULT | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_DOC);  // debugging prefix : (113) bd

#define __tolower(a) (a>='A'&&a<='Z' ? a + ('a'-'A') : a)
// hard error support constants
#define RE_READ_FUDGE_FACTOR (350)		// don't read right up to the last good char as reported, but rather this far back
#define DEADSPACE_CHARS 32				// insert this many null bytes between sections of good data to demarcate errors to the codec
#define LBA_BLOCK_SIZE (512)
#define DROPPED_BYTES_ALLOWED (256)

CBufferDocument::CBufferDocument() :
                            m_bNextInvalidOffEnd(false),
                            m_bNextValidOffEnd(false),
                            m_pInputStream(0),
                            m_szSourceUrl(0),
                            m_bSourceOpen(false),
                            m_nDroppedStreamBytes(0),
                            m_nReOpens(0)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:Ctor\n"); 
    Initialize();
    m_itNextValidBlock = 0;
    m_itNextInvalidBlock = 0;
    m_itPartialBlock = 0;
}

void CBufferDocument::SetWorker(CBufferWorker* pWorker)
{
    m_pWorker = pWorker;
}

CBufferDocument::~CBufferDocument()
{
}

void CBufferDocument::AttachFile(char* szUrl)
{
    CloseSource();
    SetSourceUrl(szUrl);
    Initialize();
    m_nReOpens = 0;
}

void CBufferDocument::Initialize()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:init\n"); 
	m_bDocEnd = false;
	m_bSourceCorrupt = false;
    m_nHardErrors = 0;
    m_nLastDeadSpace = -1;
	m_nStartByte = 0;
	m_nBytesValid = 0;
	m_nPartialChars = -1;
    m_itPartialBlock = 0;
	m_itOrphanBlockStart = 0;
	m_cOrphanBlocks = 0;
	m_bDocEndOrphaned = false;
    m_cOrphanedEndDocBytes = 0;
	m_nReadByte = 0;	
	m_nBlockReadByte = 0;	
    m_nLength = -1;
    m_nSourceBytes = -1;
    m_nReOpens = 0;
}

// give up all blocks and close the file.
void CBufferDocument::ReleaseBlocks()
{
    CloseSource();
    if (m_lstBlocks.Size() > 0)
    {
        m_pWorker->PutFreeBlocks(&m_lstBlocks);
        m_lstBlocks.Clear();
    }
    Initialize();
}

// release chunks, but also detach from the file ref.
void CBufferDocument::DetachSource()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "bd:detach\n"); 
    CloseSource();
    ReleaseBlocks();
    delete [] m_szSourceUrl;
    m_szSourceUrl = NULL;
}

void CBufferDocument::PrintStateReport(char* caption)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:Doc Report '%s'\n",caption); 
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bNextInvalidOffEnd?"m_bNextInvalidOffEnd=true":"m_bNextInvalidOffEnd=false");
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bNextValidOffEnd?"m_bNextValidOffEnd=true":"m_bNextValidOffEnd=false");
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bDocEndOrphaned?"m_bDocEndOrphaned=true":"m_bDocEndOrphaned=false");
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bDocEnd?"m_bDocEnd=true":"m_bDocEnd=false");
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bSourceCorrupt?"m_bSourceCorrupt=true":"m_bSourceCorrupt=false");
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "%s\n", m_bSourceOpen?"m_bSourceOpen=true":"m_bSourceOpen=false");
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nReadByte %d\n",(int)m_nReadByte);
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nBlockReadByte %d\n",(int)m_nBlockReadByte);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nStartByte %d\n",(int)m_nStartByte);
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nBytesValid %d\n",(int)m_nBytesValid);
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nPartialChars %d\n",(int)m_nPartialChars);
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_cOrphanBlocks %d\n",(int)m_cOrphanBlocks);
	DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_cOrphanedEndDocBytes %d\n",(int)m_cOrphanedEndDocBytes);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nHardErrors %d\n",(int)m_nHardErrors);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nLastDeadSpace %d\n",(int)m_nLastDeadSpace);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_lstBlocks %d\n",(int)&m_lstBlocks);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "block count %d\n",m_lstBlocks.Size()); 
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nLength %d\n",(int)m_nLength);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nSourceBytes %d\n",(int)m_nSourceBytes);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nDroppedStreamBytes %d\n",(int)m_nDroppedStreamBytes);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "m_nReOpens %d\n",(int)m_nReOpens);
    DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:url %s\n",m_szSourceUrl?m_szSourceUrl:"NULL"); 
}

bool CBufferDocument::ForwardFill(int nFileOffset)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:ForFill\n");
	int nDocumentCeiling = LastWindowByte();
	int nBlocksAway = (nFileOffset - nDocumentCeiling) / BUFFER_BLOCK_BYTES;
	int nBlockOffset = nFileOffset % BUFFER_BLOCK_BYTES;
	if ( nBlockOffset > 0) nBlocksAway++;
	if (nBlocksAway > 0 && nBlocksAway <= SEEK_PREBUFFER_BLOCKS)
	{
        DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:FwdFill %d\n",nBlocksAway);
        // cull unneeded blocks and postpend if necessary
        m_pWorker->ReclaimDeadBlocks();
        m_pWorker->DistributeFreeBlocks();

	    // set where our new read buffer will be
        m_itNextValidBlock = m_itNextInvalidBlock + (nBlocksAway -1);
        DBASSERT( DBG_BUF_DOC, ( m_itNextValidBlock != 0 ) , "bd:in forward fill, iterator math pushed nextValidBlock off list\n"); 

        // tell the producer how many filled blocks will need to be marked "past" data, i.e., before the read point.
		m_pWorker->SetOldBlocksToWrite( nBlocksAway - 1 );
        // adjust the read-point counts.
		m_nReadByte = nFileOffset;
		m_nBlockReadByte = nBlockOffset;
        PrintStateReport("ForwardFill");
        return true;
    }
    return false;
}

// shift through the blocks to a new read point, maintaining vital counts and synchronizers along the way.
// this isn't thread safe, that responsibility is left to the caller.
// this is only intended for a shift outside the current buffer.  inbuffer shifts are inline.
// params:	nCharsToShift	= number of chars to shift
//			iUnitDirection	= signed unit, indicating direction (i.e., +1 or -1)
//          bBorrowingInvalid = if true, we intend to shift to the location without full semaphore counting, to avoid pausing the producer.
bool CBufferDocument::SeekInWindow(int nFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingInvalid)
{
	CBufferAccountant* pAcct = m_pWorker->GetDocAccountant();

    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE,"bd:Shift %s%d\n",(iUnitDirection>0) ? "+" : "-", nCharsToShift);
	// figure out how many chars to skip in the current buffer
    int nToShiftInCurrent;
	if (iUnitDirection > 0)
		nToShiftInCurrent = BUFFER_BLOCK_BYTES - m_nBlockReadByte;
	else
		nToShiftInCurrent = m_nBlockReadByte;
    // move out of the current buffer
    (iUnitDirection>0) ? IncrementNextValidBlock() : DecrementNextValidBlock();
    
    // release the current buffer back into the appropriate empty/full count.
    // (epg,8/9/2000): if we're moving forward and had previously borrowed an empty, then right the count by skipping a post to empty.
	if (m_pWorker->OwedInvalidBlock() && iUnitDirection > 0)
		m_pWorker->BorrowInvalidBlock(false);
	else
    {
		if (iUnitDirection>0)
            pAcct->PutInvalid();
        else
            pAcct->PutValid();
    }
    
    // (epg,8/9/2000): if we're moving back and previously borrowed an empty (but not just now), then wait for an
    // extra empty to right the count.
	if (m_pWorker->OwedInvalidBlock() && iUnitDirection < 0 && !bBorrowingInvalid)
	{
		m_pWorker->BorrowInvalidBlock(false);
		(iUnitDirection>0)?pAcct->GetValid():pAcct->GetInvalid();
	}

    // update position counters to account for portion of the current buffer skipped over
	m_nReadByte      += nToShiftInCurrent * iUnitDirection;
	nCharsToShift    -= nToShiftInCurrent;
	m_nBlockReadByte  = 0;
    
    // skip over the centerspan of whole blocks
	while (nCharsToShift > BUFFER_BLOCK_BYTES)								
	{
		if ( ((iUnitDirection>0)?pAcct->PeekValid():pAcct->PeekInvalid()) > 0) {
			if ( ((iUnitDirection>0)?pAcct->GetValid():pAcct->GetInvalid()) < 0) {
				DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING,"C:trywait failed with %d chars yet to shift\n",nCharsToShift);
			}
        }
		else {
			DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING,"C:none to get with %d chars yet to shift\n",nCharsToShift);
		}
        
        // consume the character to-skip count
		nCharsToShift -= BUFFER_BLOCK_BYTES;
        
        // move the buffer index
        (iUnitDirection>0) ? IncrementNextValidBlock() : DecrementNextValidBlock();

        // update the file offset count
        m_nReadByte += BUFFER_BLOCK_BYTES * iUnitDirection;
        
        // update the empty/full semaphore
		if (iUnitDirection>0)
            pAcct->PutInvalid();
        else
            pAcct->PutValid();
	}
    
	// skip into the new buffer to the correct place
	// if we're 'borrowing' an empty, then we're using it outside the classic accounting system with
    // the idea of giving it back asap.  therefore we won't actually wait on the semaphore, but rather
    // remember that we owe one to the system.
	if (!bBorrowingInvalid && ((iUnitDirection>0)?pAcct->PeekValid():pAcct->PeekInvalid()) > 0) {
		if ( ((iUnitDirection>0)?pAcct->GetValid():pAcct->GetInvalid()) < 0) {
			DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING,"C:last trywait failed\n");
		}
    }
 	else {
		DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING,"C:last trywait failed 2\n");
	}
    
	m_nReadByte      += nCharsToShift * iUnitDirection;					
	m_nBlockReadByte  = (iUnitDirection > 0) ? nCharsToShift : (BUFFER_BLOCK_BYTES - nCharsToShift);
    
    CBufferingImp::GetInstance()->GetWriterMsgPump()->StartProducing();

    // succeed
    //PrintStateReport("Shift");
	return true;
}

// return true if successfully resets the document to a new location in the file, throwing out all previously buffered data.
bool CBufferDocument::MoveWindow(int nFileOffset)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:Rebuf %d\n",nFileOffset);

	// clear skip buffer state
	m_itOrphanBlockStart = 0;
	m_cOrphanBlocks      = 0;
    
    // figure out where we'll start buffering from	
    int nStartOffset;
	nStartOffset                       = nFileOffset - (SEEK_PREBUFFER_BLOCKS * BUFFER_BLOCK_BYTES) - 10;
	if (nStartOffset < 0) nStartOffset = 0;
    
    // threshold the start offset to the nearest buffer boundary, rounding towards the DocStart.
    int nSkipBlockCount;
    nSkipBlockCount = nStartOffset / BUFFER_BLOCK_BYTES;
	nStartOffset    = nSkipBlockCount * BUFFER_BLOCK_BYTES;
    
    // set document manager to fill some blocks as "past" data, i.e. before the readpoint.
    int nPastBlocks = (nFileOffset - nStartOffset) / BUFFER_BLOCK_BYTES;
    m_pWorker->SetOldBlocksToWrite(nPastBlocks);
    
	// configure the document's readpoint counters
    m_nReadByte      = nFileOffset;
	m_nBlockReadByte = nFileOffset - (nFileOffset / BUFFER_BLOCK_BYTES) * BUFFER_BLOCK_BYTES;
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:read %d, block %d\n",m_nReadByte,m_nBlockReadByte);
    
    // set document positional counters, buffering will start at first buffer.
    m_nStartByte = nStartOffset;
	m_nBytesValid = 0;
	m_bDocEnd = false;
	m_bDocEndOrphaned = false;
    m_itPartialBlock = 0;
	m_nPartialChars = 0;

    m_pWorker->ReclaimDeadBlocks();
    m_pWorker->DistributeFreeBlocks();
    
    // init both full and empty pointers to start
    ResetNextValidBlock();
    ResetNextInvalidBlock();
    
    // move the read point out past the past blocks.
    //DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:move window with %d past blocks, nxtValid at %d\n",nPastBlocks,GetNextValidBlockIndex()); 
    for (int i = 0; i < nPastBlocks; ++i)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:inc valid moving window\n"); 
        IncrementNextValidBlock();
    }
    
    //DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:now next valid is %d\n",GetNextValidBlockIndex()); 

	// seek the file to the new producer point	
	WakeDrive();
	DEBUG_WAKE_DRIVE;
    if (!OpenSource())
        return false;

    if (m_pInputStream->CanSeek())
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:seeking to %d\n",nStartOffset); 
        if (m_pInputStream->Seek(IInputStream::SeekStart, nStartOffset) != nStartOffset)
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "BD:failed to seek seekable stream!\n"); 
		    return false;
        }
        else
        {
#if DEBUG_LEVEL > 0
            // verify:
            int offset = m_pInputStream->Seek(IInputStream::SeekCurrent, 0);
            DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:no, really, the file is at %d\n",offset); 
#endif
        }
    }
    else if (nFileOffset == 0)
    {
        CloseSource();
//        Initialize();
//        CBufferAccountant* pAcct = m_pWorker->m_pAccountant;
//        pAcct->SetAllBlocksInvalid();
        if(!OpenSource())
            return false;
    }
    else
        DBASSERT( DBG_BUF_DOC, ( 1<0 ) , "bd:trying to move window to %d, when CantSeek!\n",nFileOffset); 

	SleepDrive();
	DEBUG_SLEEP_DRIVE;
    // succeed
    //PrintStateReport("ValidRebuf");
    return true;
}


void CBufferDocument::RecountDocEndOrphan()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:RestSkipDocEnd\n");
    m_bDocEnd = true;
	m_itPartialBlock = m_itOrphanBlockStart + (m_cOrphanBlocks - 1);
    DBASSERT( DBG_BUF_DOC, ( false ) , "bd:recounting doc end orphan, iterator math fell off list\n"); 
	m_nPartialChars = m_cOrphanedEndDocBytes;
}

void CBufferDocument::ReclaimOrphanWindow(CBufferAccountant* pRef)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:Skip %d\n",m_cOrphanBlocks);
	// skip over the region
	m_nBytesValid += m_cOrphanBlocks * BUFFER_BLOCK_BYTES;
	// perform overflow cleanup
	while (m_nBytesValid > BUFFER_BLOCKS * BUFFER_BLOCK_BYTES)
	{	
		m_nBytesValid -= BUFFER_BLOCK_BYTES;
		m_nStartByte += BUFFER_BLOCK_BYTES;
	}
	// mark skipped bufs as full
	for (int i = 0; i < m_cOrphanBlocks; i++ )
	{
		if (pRef->GetInvalid() >= 0)
			pRef->PutValid();
        IncrementNextInvalidBlock();
	}
	// if restoring eof buffer from a backfill state, set up the document flags
	if (m_bDocEndOrphaned)
        RecountDocEndOrphan();
	// move the file pointer out to the new read location (past skip bufs), or to eof
	int nCharsToSkip = (m_cOrphanBlocks - 1) * BUFFER_BLOCK_BYTES;
	if (m_bDocEndOrphaned)
		nCharsToSkip += m_nPartialChars;
	else
		nCharsToSkip += BUFFER_BLOCK_BYTES;
	m_pInputStream->Seek( IInputStream::SeekCurrent, nCharsToSkip);
	// clear out skip flags
	m_itOrphanBlockStart = 0;
	m_cOrphanBlocks = 0;
	m_bDocEndOrphaned = false;	// consume the status variable's state
}

void CBufferDocument::ZeroOutWriteBlock (int nStart, int nBytes)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:ZeroBlock %d,%d @ %d\n",nStart,nBytes,(int)(*m_itNextInvalidBlock)); 
    unsigned char* pBlock = (*m_itNextInvalidBlock);
    memset((void*)pBlock,0,nBytes);
}

void CBufferDocument::ThrottleFileIndex()
{
    //DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:throttleOffset:bytesValid %d, startByte %d\n",m_nBytesValid,m_nStartByte); 
	int startchar = m_nBytesValid + m_nStartByte;
    int nStreamPos = m_pInputStream->Position();
    //DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:streampos %d\n",nStreamPos); 
    if (nStreamPos != startchar)
    {
        if (m_pInputStream->CanSeek())
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:ThrottleFileIndex sk %d to %d\n",m_pInputStream->Position(),startchar);
            if (m_pInputStream->Seek(IInputStream::SeekStart, startchar) != startchar) {
                DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "FC:ThrottleFileIndex sk(%d) = %d!\n",m_pInputStream->Position(),startchar);
            }
        }
        else
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "BD:can't throttle, no seeking.\n"); 
            /*
            // allow the stream to drop some bytes
            if ((nStreamPos > m_nBytesValid+m_nStartByte) && (nStreamPos - m_nBytesValid+m_nStartByte < DROPPED_BYTES_ALLOWED))
            {   
                DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "BD:StreamDrifted\n"); 
                // zero out the beginning of the block
                ZeroOutWriteBlock(0, nStreamPos);
                m_nDroppedStreamBytes = nStreamPos - m_nBytesValid;
                m_nBytesValid = nStreamPos;
            }
            */
        }
    }
}

int CBufferDocument::FilePositionFromBlockListIndex(int nListIndex)
{
    return (m_nStartByte / BUFFER_BLOCK_BYTES + nListIndex);
}

// read from the source, and respond to errors and eof.
// return BW_OK if normal read, BW_EOF if the source ends.
eBWResult CBufferDocument::WriteBlock()
{
    eBWResult bwResult = BW_OK; 

    // which buffer to read into
    DBASSERT( DBG_BUF_DOC, ( !m_bNextInvalidOffEnd ) , "bd:trying to write w/o block\n");

    unsigned char* pBlock = (*m_itNextInvalidBlock);

    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:F %d\n", m_pWorker->BlockIndexFromLocation(pBlock));

	// read from the file
    int result = 0;
    if (!m_nDroppedStreamBytes)
	    result = m_pInputStream->Read((void*)pBlock, BUFFER_BLOCK_BYTES);
    else
    {
        // ignore the first part of the buffer, since the unseekable stream is past it.
        DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:Drift %d\n",m_nDroppedStreamBytes); 
        result = m_pInputStream->Read((void*)(pBlock+m_nDroppedStreamBytes),BUFFER_BLOCK_BYTES-m_nDroppedStreamBytes);
        if (result != -1)
        {
            result += m_nDroppedStreamBytes;
            m_nDroppedStreamBytes = 0;
        }
    }

	// if the first read failed, try waking the drive and repeating
	if (result == -1 && errno != EREAD)
	{
		WakeDrive();
		result = m_pInputStream->Read((void*)pBlock, BUFFER_BLOCK_BYTES);
	}
    
    // if a partial read occurred, try to complete it until the stream reports utter failure.
#define BUFFERING_RETRY_READS 1
#ifdef BUFFERING_RETRY_READS
    if (result != -1 && result != BUFFER_BLOCK_BYTES)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:retry read with %d\n",result); 
        int nBytesLeft = BUFFER_BLOCK_BYTES - result;
        while (result != -1 && nBytesLeft > 0)
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:iter\n"); 
            int nOffset = BUFFER_BLOCK_BYTES - nBytesLeft;
            result = m_pInputStream->Read((void*)((char*)pBlock+nOffset), nBytesLeft);
            if (result > 0 )
            {
                // iterate
                nBytesLeft -= result;
                DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:retry read got %d\n",result);
            }
            else
            {
                result = -1;
                DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:retry failed\n"); 
            }
        }
        result = BUFFER_BLOCK_BYTES - nBytesLeft;
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:end of retry loop, result is officially %d\n",result); 
    }
    
    // try reopening the file, if we fail out in the first block.
    if (result == -1 && m_nBytesValid == 0 && m_nReOpens < BUFFER_DOC_REOPEN_COUNT)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:reopening %d\n",m_nReOpens); 
        ++m_nReOpens;
        CloseSource();
        if (OpenSource())
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:recursing on write\n"); 
            return WriteBlock();
        }
        else
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:failed to reopen, giving up\n"); 
        }
    }
#endif

    // handle error if its a hard-error.
    if(result == -1 && errno == EREAD)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:HE handling invoked\n"); 
        result = HandleHardError(pBlock);

        // (epg,7/30/2002): removed else case, handled same as 0 read, below.
    }
    
    if (result != BUFFER_BLOCK_BYTES)
	{
        // a non-hard error is treated as a zero-read eof read.
        if (result < 0)
            result = 0;

		// a partial read signifies the end of file
        DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:DocEnd %d\n",m_nBytesValid + m_nStartByte + result);
		m_bDocEnd = true;
        if (m_nSourceBytes > m_nBytesValid + m_nStartByte + result)
            m_nSourceBytes = m_nBytesValid + m_nStartByte + result;
        CloseSource();

		if (result <= 0)
		{
            // the last block is empty, but will remain here as a place holder for the eof.
			m_nPartialChars = 0;

            // (epg,7/30/2002): removing code that posted a fake sem count in this case.  despite the zero read,
            // the caller will post the empty block into the reader thread anyway.

		}
		else
			m_nPartialChars = result;

        m_itPartialBlock = m_itNextInvalidBlock;

        // release any blocks that are after the doc end.
        ReleasePostEOFBlocks();
        
        // signal the call stack that redistribution of blocks should occur
        bwResult = BW_EOF;
	}

    // (epg,7/30/2002): removing code that checks for a preset partial block, as we don't try to foresee the eof.

    // update document count of chars
	m_nBytesValid += result;
	while (m_nBytesValid > BUFFER_BLOCKS * BUFFER_BLOCK_BYTES)
	{	
		m_nBytesValid -= BUFFER_BLOCK_BYTES;
		m_nStartByte  += BUFFER_BLOCK_BYTES;
	}
    
    // increment the next empty counter.  at eof, this will put it off the end of the list, but the access function
    // should deal with that case, and it keeps us conceptually consistent.
    IncrementNextInvalidBlock();

    if (m_bNextInvalidOffEnd && !m_bDocEnd)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "bd:hmmm, not docend, but no more blocks to write in..\n"); 
        if (m_nSourceBytes != -1 && m_nStartByte + m_nBytesValid > m_nSourceBytes)
        {
            // source length is wrong, get more blocks to cover
            DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "bd:inflating doc length, file didn't end!\n"); 
            m_nSourceBytes += DOCSTART_PREBUFFER_BLOCKS * BUFFER_BLOCK_BYTES;
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:sb infl %d\n",m_nSourceBytes); 
            CBufferingImp::GetInstance()->GetWriterMsgPump()->CullAtLowPrio();
        }
    }

    return bwResult;
}

// returns true if a deadspace was added.  updates the byte counters to reflect this.
bool CBufferDocument::InsertBlockDeadSpace(unsigned char* pBlock, int &nBytesToRead, int &nBytesAlreadyRead)
{
    if (m_nLastDeadSpace != (int)pBlock)
    {
        int length = DEADSPACE_CHARS;
        if (DEADSPACE_CHARS > nBytesToRead)
            length = nBytesToRead;
	    memset(pBlock,0,length);
        nBytesToRead -= length;
        nBytesAlreadyRead += length;
        m_nLastDeadSpace = (int)pBlock + nBytesAlreadyRead;
        DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:DeadSpc %d\n",length);
        return length;
    }
    return 0;
}

int CBufferDocument::HandleHardError(unsigned char* pBlock)
{
    // (epg,10/18/2001): TODO: if hard error handling is required on hdd, activate it in fs/fat.
	// a hard error has encountered, so there is a region of the disk that is unavailable for reading.
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:HardErr %d\n",m_nBytesValid + m_nStartByte);
	m_nHardErrors = 0;
	bool bHardError=true;
	int nPreReadPos = m_nBytesValid + m_nStartByte;
	int nBytesToRead = BUFFER_BLOCK_BYTES;
	int nBytesAlreadyRead = 0;
	m_nLastDeadSpace = -1;
    int result = 0;
	// keep trying to read after the error until we either find the end or give up from many retries
	while (bHardError && m_nHardErrors < CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
	{
		m_nHardErrors++;
		bHardError = false;
		// find out how much of the read was considered good
        int nNextGoodLBAStart;
        if (!ReReadBeforeHardError(pBlock, nPreReadPos, nNextGoodLBAStart))
        {
			if (errno == EREAD)
			{
				// another hard error, so loop again
				bHardError = true;
				// leave a zeroed-out gap between non-contiguous regions of the file.
                InsertBlockDeadSpace(pBlock,nBytesToRead,nBytesAlreadyRead);
				continue;
			}
			else
			{
				// non hard-error failure
				DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "unknown po read error in pre-harderror reread\n");
				result = nBytesAlreadyRead;
			}
		}
		else
		{
			// good read, so track read progress
			nBytesToRead -= result;
			nBytesAlreadyRead += result;
		}

        InsertBlockDeadSpace(pBlock,nBytesToRead,nBytesAlreadyRead);
		// now move past and start reading data again.
		int nLoc = m_pInputStream->Seek(IInputStream::SeekStart,nNextGoodLBAStart);
		if (nLoc != nNextGoodLBAStart) {
			DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "sk ret %d != %d\n",nLoc,nNextGoodLBAStart);
		}
		if (nBytesToRead>0)
		{	
			// still more to read
			result = m_pInputStream->Read((void*) (pBlock + nBytesAlreadyRead), nBytesToRead);
			if (result == -1)
			{
				if (errno == EREAD)
				{
					// we got another hard error, so update state and fall through to loop again
					nPreReadPos = nNextGoodLBAStart;
					bHardError = true;
                    InsertBlockDeadSpace(pBlock,nBytesToRead,nBytesAlreadyRead);
				}
				else
				{
					// non hard error fault, just eject
					DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "unknown error in post HE read\n");
					result = nBytesAlreadyRead;
				}
			}
			else
			{
				// the read was good, so we're back on target.
				nBytesAlreadyRead += result;
				nBytesToRead -= result;
				if (nBytesToRead != 0) {
					DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "!nonzero bytes to read after HE handling\n");
				}
				bHardError= false;
				result = nBytesAlreadyRead;		// eof handler will see this value
			}
		}
	}
	if (m_nHardErrors >= CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
	{
		// the disk failed too many times in a row.  give up on the file
		DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "!%d consecutive hard errors encountered, marking file as bad\n",CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED);
		m_nHardErrors = 0;
		m_bSourceCorrupt = true;
		result = nBytesAlreadyRead;
	}
    return result;
}

bool CBufferDocument::ReReadBeforeHardError(unsigned char* pBlock, int nPreReadPos, int &nNextGoodLBAStart)
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:ReRdPreHardErr %d\n",nPreReadPos);
	int nValidChars = pc_get_valid_read_data();
	int nErrorPos = nPreReadPos + nValidChars;				// this is where the error occured
	int nErrorLBAStart = nErrorPos - (nErrorPos % 512);		// this is the start of the bad lba
	int nCurrentPos = m_pInputStream->Seek(IInputStream::SeekCurrent,0);
	// move the file pointer to before the error
	if (nCurrentPos != nPreReadPos)
	{
		int seekret = m_pInputStream->Seek(IInputStream::SeekStart, nPreReadPos);
		if (seekret != nPreReadPos)	{
			DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "apparent reseek failure ret %d != prepos %d\n", seekret, nPreReadPos);
		}
	}
	// re read the portion of data that was good before the hard error.
	int nCharsToReread = nValidChars-RE_READ_FUDGE_FACTOR;
	if (nCharsToReread < 0)	nCharsToReread = 0;
	int result = m_pInputStream->Read((void*) pBlock, nCharsToReread);
    nNextGoodLBAStart = nErrorLBAStart + LBA_BLOCK_SIZE;
    return (result != -1);
}

// return the count of blocks successfully given up.
// if bFromStart, the blocks are being removed from the start of the document.
int CBufferDocument::GiveUpBlocks(int nRequested, bool bFromHead)
{
    int nBlocks = m_lstBlocks.Size();
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:GUB %d/%d\n",nRequested,nBlocks);
    // if we're already empty, don't bother deconfiguring.
    if (!nBlocks)
        return 0;
    // removing a few blocks from the beginning of the document makes the 
    // document relatively useless.  removing them all does the same.
    if (nBlocks <= nRequested) 
    {
        // give up all caches, and close the file; hang on to the URL for now.
        if (this->IsDocumentActive()) {
            DEBUGP( DBG_BUF_DOC, DBGLEV_ERROR, "FC:current document asked to give up all blocks!\n");
        }
        ReleaseBlocks();
        return nBlocks;
    }
    // this will only happen on the current track.
    else if (bFromHead)
    {
        // (epg,6/10/2002): add an extra block to the tally
        if (BlockCountFromCharCount(m_nBytesValid) < nRequested)
        {
            ReleaseBlocks();
            return nRequested;
        }
        BlockList lstFreed;
        for (int i = 0; i < nRequested; ++i)
        {
            lstFreed.PushBack(m_lstBlocks.PopFront());
            m_nBytesValid -= BUFFER_BLOCK_BYTES;
            m_nStartByte += BUFFER_BLOCK_BYTES;
        }
        m_pWorker->PutFreeBlocks(&lstFreed);
        return nRequested;
    }
    else
    {
        BlockList lstFreed;
        int nNeeded = nRequested;
        if (m_bDocEnd)
        {
            m_bDocEnd = false;
            m_itPartialBlock = 0;
            m_nBytesValid -= m_nPartialChars;
            --nNeeded;
            --nBlocks;
            if (!m_bNextInvalidOffEnd)
                DecrementNextInvalidBlock();
            else
            {
                // the ensuing PopBack will invalidate the iterator, so set it back to one past the last buffer.
                DecrementNextInvalidBlock();
                m_bNextInvalidOffEnd = true;
            }
            lstFreed.PushBack(m_lstBlocks.PopBack());
        }
        // just clear the orphan block state here.  it may or may not be necessary, but it will be safe.
        m_itOrphanBlockStart = 0;
        m_bDocEndOrphaned = false;
        // need to make sure the write block isn't past the new list tail.  can't use the block indexes, and the iterators
        // held aren't very convenient to this cause... can use m_nBytesValid: it is safe since only the writer can
        // set it.
        int nMaxChars = (m_lstBlocks.Size() - nNeeded) * BUFFER_BLOCK_BYTES;
        if (m_nBytesValid > nMaxChars)
        {
            // if we're pulling off full blocks, then the 'next invalid' will be logically off the list (they're all valid), so record that state.
            m_itNextInvalidBlock = 0;
            m_bNextInvalidOffEnd = true;
            // also update count of valid bytes.
            m_nBytesValid = nMaxChars;
        }
        for (int i = 0; i < nNeeded; ++i)
            lstFreed.PushBack(m_lstBlocks.PopBack());
        m_pWorker->PutFreeBlocks(&lstFreed);
        return nRequested;
    }
}

// reclaim blocks that are after the EOF
void CBufferDocument::ReleasePostEOFBlocks()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:RecyclePostEOF\n"); 
    BlockList lstFreed;
    if (m_bDocEnd && m_itPartialBlock != 0)
    {
        BlockListIterator itWalk = m_lstBlocks.GetTail();
        while (itWalk != m_itPartialBlock)
        {
            --itWalk;
            lstFreed.PushBack(m_lstBlocks.PopBack());
        }
    }
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:putting back %d blocks\n",lstFreed.Size()); 
    m_pWorker->PutFreeBlocks(&lstFreed);
}

void CBufferDocument::SetSourceUrl(char* szUrl)
{
    delete [] m_szSourceUrl;
    m_szSourceUrl = strdup_new(szUrl);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "SetSourceUrl %s (%d)\n",m_szSourceUrl,(int)this); 
}

bool CBufferDocument::OpenSource()
{
    if (!m_bSourceOpen && m_szSourceUrl && !m_bSourceCorrupt)
    {
        if (m_pInputStream)
            delete m_pInputStream;
        m_pInputStream = CDataSourceManager::GetInstance()->OpenInputStream(m_szSourceUrl);
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:Open(b)%d=%s\n",(int)m_pInputStream,m_szSourceUrl); 
        if (!m_pInputStream)
        {
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "fail!\n"); 
            m_bSourceCorrupt = true;
            m_bDocEnd        = true;
            m_nSourceBytes   = 0;
        }
        else
            m_bSourceOpen = true;
    }
#if DEBUG_LEVEL > 0
    // explore reason not to open source, if sufficient debug level.
    else if (!m_szSourceUrl)
        DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "BD:open no url\n"); 
    else if (m_bSourceCorrupt)
        DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "BD:open corrupt\n"); 
    else if (m_bSourceOpen)
        DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:already open\n"); 
#endif
    return m_bSourceOpen;
}

void CBufferDocument::CloseSource()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:CS\n"); 
    if (m_bSourceOpen && m_pInputStream)
    {
        m_bSourceOpen = false;
        //DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "bd:Close(b)%d\n",(int)m_pInputStream,m_szSourceUrl); 
        m_pInputStream->Close();
        // there is no way to reopen a closed stream, so delete immediately.
        delete m_pInputStream;
        m_pInputStream = NULL;
    }
}

void CBufferDocument::IncrementNextValidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:V+\n");
    
    if (m_bNextValidOffEnd) 
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "FC:Trying to IncNextValid that's already OffEnd\n");
        return;
    }
    
    ++m_itNextValidBlock;
    
    if (m_itNextValidBlock == m_lstBlocks.GetEnd())
        m_bNextValidOffEnd = true;
}

void CBufferDocument::DecrementNextValidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DecNxtValid\n"); 
    if (m_bNextValidOffEnd)
    {
        m_bNextValidOffEnd = false;
        m_itNextValidBlock = m_lstBlocks.GetTail();
//        DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DecValid %d AOE\n",m_ptNextValidBlock->Get());
    }
    else if (m_itNextValidBlock != m_lstBlocks.GetHead())
    {
        --m_itNextValidBlock;
//        DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DecValid %d\n",m_ptNextValidBlock->Get());
    }
}

void CBufferDocument::IncrementNextInvalidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:Inv+\n"); 
    if (m_bNextInvalidOffEnd)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "FC:IncNextInval already OffEnd\n");
        return;
    }
    ++m_itNextInvalidBlock;
    if (m_itNextInvalidBlock == m_lstBlocks.GetEnd()) 
        m_bNextInvalidOffEnd = true;
//    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:IncInvalid %d\n",m_ptNextInvalidBlock->Get());
}

void CBufferDocument::DecrementNextInvalidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:Inv-\n"); 
    if (m_bNextInvalidOffEnd)
    {
        m_bNextInvalidOffEnd = false;
        m_itNextInvalidBlock = m_lstBlocks.GetTail();
        DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DecInvalid %d AOE\n",m_pWorker->BlockIndexFromLocation(*m_itNextInvalidBlock));
    }
    else if (m_itNextInvalidBlock != m_lstBlocks.GetHead())
    {
        --m_itNextInvalidBlock;
        DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DecInvalid %d \n",m_pWorker->BlockIndexFromLocation(*m_itNextInvalidBlock));
    }
    else
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "FC:Asked to Dec Invalid already at head %d \n",(*m_itNextInvalidBlock));
    }
}

void CBufferDocument::SyncNextInvalidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:SyncInv\n"); 
    m_itNextInvalidBlock = m_lstBlocks.GetHead();
    
    // (epg,9/23/2002): instead of counting out based on an index, need to use m_nValidChars to infer the correct iterator location.
    int nBlocksToSkip = m_nBytesValid / BUFFER_BLOCK_BYTES;
    
    // bump the count up if there's a partial not counted by standard c-division.
    if (nBlocksToSkip * BUFFER_BLOCK_BYTES < m_nBytesValid)
        ++nBlocksToSkip;
    
    while (nBlocksToSkip > 0)
    {
        --nBlocksToSkip;
        ++m_itNextInvalidBlock;
    }
    
    if (m_itNextInvalidBlock != 0)
        m_bNextInvalidOffEnd = false;
    else
        DEBUGP( DBG_BUF_DOC, DBGLEV_WARNING, "bd:after sync next invalid iterator, iter still off list!\n"); 
}

void CBufferDocument::BackfillToDocStart()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:backfill to start?  re anchoring\n"); 

    InvalidateAndReAnchorWindow(0);
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:done reanchor, return\n"); 
    return;

    /*
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:BackfillToDocStart\n"); 
    int nPrepend = m_nStartByte / BUFFER_BLOCK_BYTES;
    int nValid = CountValidBlocks();
    m_pWorker->GetFreeBlocks(nPrepend, &m_lstBlocks, true);
    m_nStartByte = 0;
    m_nReadByte = 0;
    m_nBytesValid = 0;
    // skip blocks if we have any left.
    if (nPrepend < m_lstBlocks.Size())
    {
        // figure out how many of the past-most blocks we'll get to keep, that the producer can "skip" during filling operations.
	    m_iOrphanBlock = nPrepend;
        // tell the document how many blocks it can skip, since they're already filled with relevant data.
	    m_cOrphanBlocks = nValid;
        // don't skip more than we have.
        if (m_iOrphanBlock + m_cOrphanBlocks > m_lstBlocks.Size())
            m_cOrphanBlocks = m_lstBlocks.Size() - m_iOrphanBlock;
	    // if we're skipping the eof buffer, we need to track that so the producer can restore that info later when it skips the eof buf.
        // we can't leave the m_bDocEnd flag on in the document, or it won't fill right.  OTOH, we can leave the partial_buffer_chars count alone,
        // and it will just be reactivated as-is.
	    int nLastSkipBlock = m_iOrphanBlock + m_cOrphanBlocks - 1;
	    if (( m_bDocEnd ) && ( nLastSkipBlock == m_nPartialIndex))
		    m_bDocEndOrphaned = true;
	    m_nPartialIndex = -1;		// we leave m_nPartialChars, since there's no way to recompute it.
    }
    else
    {
        m_iOrphanBlock = -1;
        m_cOrphanBlocks = 0;
        m_nPartialIndex = -1;
    }
    m_bDocEnd = false;
    m_pInputStream->Seek(IInputStream::SeekStart, 0);
    if ((*m_pWorker->GetActiveDocument()) == this)
        m_pWorker->GetDocAccountant()->Reset(0);
    ResetNextInvalidBlock();
    */
}

void CBufferDocument::JumpToDocStart()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_HIGH_LEVEL_TRACE, "bd:JumpToDocStart\n");
    // first back up to the start of current blocks.
    m_nReadByte = m_nStartByte;
    m_nBlockReadByte = 0;
    if (!WindowAtDocStart())
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:window not anchored at bof\n"); 
        InvalidateAndReAnchorWindow(0);
    }
    // try to make sure we have at least one buf, so we can get the buffer pointers going.
    if (m_lstBlocks.Size() == 0)
    {
        m_pWorker->GetFreeBlocks(1, &m_lstBlocks);
        ResetNextInvalidBlock();
    }
    ResetNextValidBlock();

    // (epg,7/30/2002): this occurs in regular operation, this is called again after 
    // blocks have been made available, so it fails partially the first time but never
    // the second.  TODO: remove duality slop
    if (m_lstBlocks.Size() == 0) {
        DEBUGP( DBG_BUF_DOC, DBGLEV_TRACE, "bd:unexpected no bufs in rewind\n");
    }
}

void CBufferDocument::ResetNextValidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:ResetNxtValid\n");
    m_itNextValidBlock = m_lstBlocks.GetHead();
    m_bNextValidOffEnd = false;
}

void CBufferDocument::ResetNextInvalidBlock()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:reset next invalid idx\n"); 
    m_itNextInvalidBlock = m_lstBlocks.GetHead();
    m_bNextInvalidOffEnd = false;
}

int CBufferDocument::Length()
{
    bool bOpen = m_bSourceOpen;

    if (m_nLength == -1 && m_szSourceUrl)
    {
        if (!bOpen && !OpenSource())
            return 0;
        m_nLength = m_pInputStream->Length();

        if (!bOpen)
            CloseSource();
    }
    else
    {
        //DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:src bytes %d, url %d\n",m_nSourceBytes,(int)m_szSourceUrl); 
    }
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:DocLen %d\n",m_nLength); 
    return m_nLength;
}

int CBufferDocument::BufferLength()
{
    bool bOpen = m_bSourceOpen;

    if (m_nSourceBytes == -1 && m_szSourceUrl)
    {
        if (!bOpen && !OpenSource())
            return 0;
        m_nSourceBytes = m_pInputStream->Length();

        // if this is a net source, then ignore the length info and use a safe large value.
        IDataSource* pDS = CDataSourceManager::GetInstance()->GetDataSourceByURL(m_szSourceUrl);
        if ((pDS->GetClassID() == NET_DATA_SOURCE_CLASS_ID) && !m_nSourceBytes)
        {
            // for net, this just indicates we don't know the length.  prepare for a large file (one requiring all blocks for a long time)
            m_nSourceBytes = ARBITRARILY_LARGE_DOCSIZE;
            DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:net stream, assuming %d bytes\n",m_nSourceBytes); 
        }
        if (!bOpen)
            CloseSource();
        if (m_bDocEnd)   {
		    m_nSourceBytes = m_nBytesValid + m_nStartByte + 1;
        }
    }
    else
    {
        //DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:src bytes %d, url %d\n",m_nSourceBytes,(int)m_szSourceUrl); 
    }
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "BLen %d\n",m_nSourceBytes); 
    return m_nSourceBytes;
}

// count old blocks to reassign for new data
int CBufferDocument::CountDeadBlocks()
{
    // count of chars from window start to current file, presumably meaning window size, or m_nBytesValid... ?
    int nSpareChars = m_nReadByte - m_nBlockReadByte - m_nStartByte;
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:%d spare chars\n",nSpareChars); 
    // that many bufs, fudged down to keep a few behind the read point
    int nSpareBlocks = nSpareChars/BUFFER_BLOCK_BYTES - RECLAIM_BLOCK_AGE;
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:%d spare blocks\n",nSpareBlocks); 
    if (nSpareBlocks < 0)
        nSpareBlocks = 0;
    DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:CountDeadBlocks %d\n",nSpareBlocks); 
    return nSpareBlocks;
}

int CBufferDocument::CountOldChars()
{
    DEBUGP( DBG_BUF_DOC, DBGLEV_LOW_LEVEL_TRACE, "bd:CountOld\n"); 
    return m_nReadByte - m_nStartByte;
}

bool CBufferDocument::AreAllBlocksValid()
{
    return (CountValidBlocks() == m_lstBlocks.Size());
}

// throw out the old blocks and start over at the new location (with a certain amount of prebuffering for backseek)
int CBufferDocument::InvalidateAndReAnchorWindow( unsigned int nFileOffset )
{
    Initialize();

    CBufferingImp* pPIS = CBufferingImp::GetInstance();
    CBufferAccountant* pAcct = m_pWorker->m_pAccountant;

    bool bActive = false;
    if (this == (*m_pWorker->GetActiveDocument()))
    {
        bActive = true;
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:invalidating active doc, so all blocks invalid\n"); 

        // (epg,5/30/2002): return read buffer to empty status.  was previously returning the read block to invalid,
        // and then waiting for a valid at the end of this function.  but this fn gets called in
        // two different situations, one in which the GetValid call could not be fulfilled.  by just
        // officially clearing the read block here, we can rely on the reader thread to lock down a 
        // block later, which is by definition going to occur in a valid thread context.
        pPIS->GetReaderMsgPump()->GetReader()->ClearReadBlockLock();
        if (m_pWorker->OwedInvalidBlock())
		    m_pWorker->BorrowInvalidBlock(false);
	    else
		    pAcct->PutInvalid();
        // reset valid count to zero
        pAcct->SetAllBlocksInvalid();

    } 
    else
    {
        // (epg,5/30/2002): if we are reanchoring an inactive (non-current) file, then it can't have been bof-rooted,
        // and therefore wouldn't have been involved in validity counts.  therefore, do nothing in that
        // case.
    }

    // set up the document for the rebuffer
    if (!MoveWindow(nFileOffset))
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:failed to move window to %d\n",nFileOffset); 
        return -1;
    }
    
    if (bActive)
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:setting quick lock\n"); 
        pPIS->GetReaderMsgPump()->GetReader()->SetQuickLockReady();
        DEBUGP( DBG_BUF_DOC, DBGLEV_VERBOSE, "bd:quick locked\n"); 
    }

	// return success
    return nFileOffset;
}

bool CBufferDocument::SourceOffsetValid()
{
 	int startchar = m_nBytesValid + m_nStartByte;
    int nStreamPos = m_pInputStream->Position();
    return (startchar == nStreamPos);  
}

bool CBufferDocument::CanSourceSeek()
{
    if (m_pInputStream)
    {
        return m_pInputStream->CanSeek();
    }
    else
    {
        DEBUGP( DBG_BUF_DOC, DBGLEV_INFO, "bd:no input stream, so unknown DSid\n"); 
        return false;
    }
}
