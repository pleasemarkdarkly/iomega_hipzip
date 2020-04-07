//........................................................................................
//........................................................................................
//.. File Name: BufferReader.cpp															..
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

#include <util/debug/debug.h>
#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferReader.h>
#include <fs/fat/sdtypes.h>
#include <fs/fat/sdapi.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/BufferWorker.h>
#include <main/buffering/BufferAccountant.h>
#include <datastream/fatfile/FileInputStream.h>
#include <main/buffering/BufferTypes.h>
#include <core/playmanager/PlayManager.h>
#include <main/buffering/DJConfig.h>
#include <cyg/kernel/kapi.h>
#include <string.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_READER, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_READER);  // debugging prefix : (64) br

CBufferReader::CBufferReader() : m_pfnNotifyPrebuf(NULL), m_bReadBlockLocked(false), m_pWorker(0), m_bQuickLockReady(false)
{
    // set default blocks to default seconds * blocks per second baseline.
    m_nPrebufBlocks = m_nPrebufDefault = PREBUF_DEFAULT_SECONDS * PREBUF_BLOCKRATE_BASELINE;
    m_nPrebufMax = PREBUF_MAX_SECONDS * PREBUF_BLOCKRATE_BASELINE;
    DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:ctor : %d prebuf default, %d max\n",m_nPrebufDefault,m_nPrebufMax); 
}
CBufferReader::~CBufferReader() 
{}

// returns true if a buffer was successfully locked for reading.
bool CBufferReader::LockBlockForReading()
{
    CBufferAccountant* pAcct = m_pWorker->m_pAccountant;
	CBufferingImp* pPIS = CBufferingImp::GetInstance();
	CBufferDocument* pDoc = (*m_pWorker->GetActiveDocument());

    int nValid = pAcct->PeekValid();
    int nReadByte = pDoc->GetReadByte();

    // if we're out of buffers
    bool bPreBuffer = false;

    // if we run out of blocks mid stream, pause and pre buffer again.
    if (nValid == 0 && nReadByte != 0 && !pDoc->SourceAtDocEnd() && nReadByte < pDoc->BufferLength())
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:pausing for re-prebuffer\n"); 
        ++m_nPrebufBlocks;
        if (m_nPrebufBlocks > m_nPrebufMax)
            m_nPrebufBlocks = m_nPrebufMax;
        bPreBuffer = true;
    }
    // at the start of the doc, prebuffer.
    else if (pDoc->IsDocumentActive() && nReadByte == 0)
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:wait for doc start prebuffer\n");
        bPreBuffer = true;
    }
    
	if (nValid <= NET_BUFFER_WAKE_THRESHOLD)
    {
        // only wake up once in the doc-end zone.  since we're only tracking current track fullness in pAcct above,
        // we will tend to wake up every block near the end of the document.  bDocEndWake 
        // will keep us from repeatedly waking by only allowing one wakeup per doc-end.
        static bool bDocEndWake = false;
        if (!pDoc->m_bDocEnd || !bDocEndWake) // m_bDocEnd means EOF
        {
            if (pDoc->m_bDocEnd)
                // we're consuming the single doc-end-wake
                bDocEndWake = true;
            else
                // we're not at doc end, so allot a doc-end-wake to this document later on.
                bDocEndWake = false;
            DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE,"br:low\n");
            pPIS->GetWriterMsgPump()->SetCautiousMode();
	        pPIS->GetWriterMsgPump()->StartProducing();
        }
    }
    
	if (pAcct->GetValid() < 0)
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:couldn't get full in lock!\n");
		return false;
    }
    pAcct->Report();


    // (epg,10/19/2001): TODO: determine if this is obviated by the above negative GetValid return.
    if (m_pWorker->m_nFakeValidBlocks > 0)
	{
		// we don't really have a buffer, there aren't any to be had, so break out of the function.
		DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:Couldn't access file after seek\n");
		m_pWorker->m_nFakeValidBlocks = 0;
		return -1;
	}
    
    // if we're locking down the first block, then reset the index.
    if (nReadByte == 0)
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"BR:init rd idx to 0\n");
        // we're reading from the start of the file.
        // init the read index to the first
        pDoc->ResetNextValidBlock();
    }

    // if the active doc is starting up, prebuffer.
    if (bPreBuffer)
    {
        // make sure we have enough data to proceed.
        int nNeeded = m_nPrebufBlocks;
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:default to %d prebuf\n",m_nPrebufBlocks); 
        int nDocBytesLeft = pDoc->BufferLength() - nReadByte;
        // don't need more than entire file to begin.
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:docBytesLeft %d\n",nDocBytesLeft); 
        if (nDocBytesLeft < m_nPrebufBlocks*BUFFER_BLOCK_BYTES)
        {
            nNeeded = nDocBytesLeft/BUFFER_BLOCK_BYTES;
            DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:file only needs %d more blocks\n",nNeeded);
            if (nNeeded > m_nPrebufBlocks)
                nNeeded = m_nPrebufBlocks;
        }
        // wait until we are prebuffered, or the file dies, or the file ends.
        int nLastValid = 0;
        DEBUGP( DBG_BUF_READER, DBGLEV_INFO, "br:need %d for prebuf\n",nNeeded);
        while (nValid < nNeeded && !pDoc->IsSourceCorrupt() && !pDoc->SourceAtDocEnd())
        {
            cyg_thread_delay(50);
            nValid = pAcct->PeekValid();
            if (nValid != nLastValid)
            {
                nLastValid = nValid;
                DEBUGP( DBG_BUF_READER, DBGLEV_INFO, "br:prebuf %d/%d\n",nValid,nNeeded); 

                int nProgressPercent = (100*nValid)/nNeeded;
                if (nProgressPercent > 100) 
                    nProgressPercent = 100;
                if (m_pfnNotifyPrebuf!=NULL)
                {
                    DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:callback %d percent\n",nProgressPercent); 
                    m_pfnNotifyPrebuf(nProgressPercent);
                }
                else
                    DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:no callback, %d percent\n",nProgressPercent); 
            }
        }
    }
    else
    {
        //DEBUGP( DBG_BUF_READER, DBGLEV_LOW_LEVEL_TRACE, "br:nonvirgin, no prebuff\n"); 
    }

    // (epg,9/23/2002): think I missed this in the re-prebuffering case, just because of the way the code
    // changed over time.  bad case combination in developing the re-prebuffering code.
    if (m_bQuickLockReady)
    {
        m_bQuickLockReady = false;
    }
    else if (nReadByte != 0)
    {
        pDoc->IncrementNextValidBlock();
    }

	// reset the buffer level bookmark to the beginning of the new buffer.
    pDoc->SetBlockReadByte(0);
	m_bReadBlockLocked = true;
    return true;
}

// return true if the document's file has a nonzero length, or tell the document the file's eof is reached and return false.
bool ValidateDocumentFileLength(CBufferDocument* pDoc)
{
    DEBUGP( DBG_BUF_READER, DBGLEV_LOW_LEVEL_TRACE, "br:VDFL\n"); 
    if (!pDoc->BufferLength())
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:invalidate file document\n");
        pDoc->SetDocEnd();
        return false;
    }
    return true;
}

// returns the length of the file, or the last valid readable character in the file, in an error situation.
int CBufferReader::GetSafeFileLength(CBufferDocument* pDoc)
{
    DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:GetSafeLen\n"); 
    int nSafeLength = pDoc->BufferLength();
	if (pDoc->SourceAtDocEnd())   {
        // if there was an error of some kind, the filesize may not agree with 
        // the eof data, but the eof dictates what we can actually do in the filesystem.
		nSafeLength = pDoc->GetWindowBytes() + pDoc->FirstWindowByte() + 1;
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:safelen %d\n",nSafeLength);
    }
    return nSafeLength;
}

// returns true if a valid combination of origin + offset is passed in.  sets nFileOffset to a simple offset from beginning of file.
bool CBufferReader::FileOffsetFromOriginOffset(CBufferDocument* pDoc, int nFileSize, eInputSeekPos eOrigin, int nOffset, int &nFileOffset)
{
    switch(eOrigin)
	{
	    case IInputStream::SeekStart:
		    if (nOffset < 0)
		    {
			    DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:Invalid negative offset from bof requested of seek\n");
			    return false;
		    }
		    nFileOffset = nOffset;
		    break;
	    case IInputStream::SeekCurrent:
		    nFileOffset = pDoc->GetReadByte() + nOffset;
		    break;
	    case IInputStream::SeekEnd:
		    if (nOffset > 0)
		    {			
			    DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:Invalid positive offset from eof requested of seek\n");
			    return false;
		    }
		    nFileOffset = nFileSize + nOffset;
		    break;
	}
    return true;
}    

// forces nFileOffset to within the bounds of the file, as indicated by nFileSize.
void NormalizeFileOffset(int nFileSize, int &nFileOffset)
{
    if (nFileOffset < 0)
	{
		DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:warning: seeking before start of file, forcing to start...\n");
		nFileOffset = 0;
	}
	if (nFileOffset > nFileSize - 1)
	{
		DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:warning: seeking past end of file requested, forcing to end...\n");
		nFileOffset = nFileSize - 1;	
	}
}

// calculate the limits of the current buffer.
void CBufferReader::ReadBlockLimits(CBufferDocument *pDoc, int &nBlockFloor, int &nBlockCeiling)
{
    nBlockFloor = pDoc->GetReadByte() - pDoc->GetBlockReadByte();
    // are we in a partial buffer (eof situation)
    if (pDoc->GetPartialBlockIterator() == pDoc->GetReadBlockIterator() && pDoc->GetPartialBlockIterator() != 0)
		nBlockCeiling = nBlockFloor + pDoc->GetPartialBlockChars() - 1;
	else
		nBlockCeiling = nBlockFloor + BUFFER_BLOCK_BYTES - 1;
}


// return the uppermost character buffered in the current document, in a threadsafe manner.
int ThreadSafeDocumentCeiling (CBufferDocument* pDoc, CBufferAccountant* pAcct, int nBlockCeiling)
{
	cyg_count32 fullbufs = pAcct->PeekValid();
	int nDocumentCeiling = nBlockCeiling + ((fullbufs-1) * BUFFER_BLOCK_BYTES);
	if (pDoc->SourceAtDocEnd())
		nDocumentCeiling += pDoc->GetPartialBlockChars();
	else
		nDocumentCeiling += BUFFER_BLOCK_BYTES;
    if (nDocumentCeiling > (pDoc->FirstWindowByte() + pDoc->GetWindowBytes()))
        nDocumentCeiling = pDoc->FirstWindowByte() + pDoc->GetWindowBytes();
    return nDocumentCeiling;
}        

// move the consumer read point to the offset from the origin specified.
int CBufferReader::Seek(eInputSeekPos eOrigin, int nOffset)
{
	CBufferingImp*     pPIS  = CBufferingImp::GetInstance();
	CBufferDocument*   pDoc  = (*m_pWorker->GetActiveDocument());
	CBufferAccountant* pAcct = m_pWorker->m_pAccountant;
    
    // make sure we have a read buffer locked
    m_bQuickLockReady = false;
	if (!m_bReadBlockLocked)
        if (!LockBlockForReading())
        {
            DEBUGP( DBG_BUF_READER, DBGLEV_INFO, "br:couldn't lock block for reading!\n"); 
            return -1;
        }
    
    // fail if the document's file reports a zero size.
    if (!ValidateDocumentFileLength(pDoc))
    {
        return -1;
    }
    
    // get the safe file size.
    int nFileSize = GetSafeFileLength(pDoc);
    
    // determine file position relative to file start.
    int nFileOffset;
    if (!FileOffsetFromOriginOffset(pDoc, nFileSize, eOrigin, nOffset, nFileOffset))
    {
        return -1;		
    }
    
    // make sure the file offset is within the file.
    NormalizeFileOffset(nFileSize, nFileOffset);
    
    // calculate limits of current buffer.
	int nBlockFloor, nBlockCeiling;
    ReadBlockLimits(pDoc, nBlockFloor, nBlockCeiling);
    if ((nFileOffset >= nBlockFloor) && (nFileOffset <= nBlockCeiling))
	{
		// we seek a location within the current buffer.  since we own it, just move our bookmarks and move on.
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "br:file offset %d, block floor %d, block ceiling %d\n",
            nFileOffset,nBlockFloor,nBlockCeiling);
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk0(%d)\n",nFileOffset-pDoc->GetReadByte());
        
		int nNewBlockOffset = nFileOffset - nBlockFloor;
		pDoc->SetReadByte(nFileOffset);
		pDoc->SetBlockReadByte(nNewBlockOffset);
		return nFileOffset;
	}
    // check for the case where we're seeking future buffered data.
	else if (nFileOffset >= nBlockFloor)
	{
		// calculate the (threadsafe) upper limit on the chars currently buffered.
        int nDocumentCeiling = ThreadSafeDocumentCeiling(pDoc, pAcct, nBlockCeiling);
        // do we have this location buffered in the future?
        if (nFileOffset <= nDocumentCeiling)
		{
            DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk1(%d)\n",nFileOffset-pDoc->GetReadByte());
			pDoc->SeekInWindow(	nFileOffset, nFileOffset - pDoc->GetReadByte(), 1, false );
			return pDoc->GetReadByte();
		}
	}
    // if the seek point is in the previous buffer, we can assume it is available.  this is guaranteed b/c of general policy to always
    // prebuffer at least one buffer, and to not recycle the buffer until the read point has move further on.  this case is required for
    // wma, since it routinely seeks slightly into the past, potentially into the previous buffer.
    // XXX Change GetReadByte() - GetBlockReadByte() into nBlockFloor.  It seems they are equivalent.
	else if (nFileOffset >= (pDoc->GetReadByte() - pDoc->GetBlockReadByte() - BUFFER_BLOCK_BYTES) && !m_pWorker->m_bReaderUsingPrevBlock)
	{
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk2(-%d)\n",pDoc->GetReadByte() - nFileOffset);
		pDoc->SeekInWindow( nFileOffset, pDoc->GetReadByte() - nFileOffset, -1, false );
		return pDoc->GetReadByte();
	}
    
    // the remaining cases require interrupting the producer thread.
	pPIS->GetWriterMsgPump()->PauseWriterThread();
	if (pDoc->ByteInWindow(nFileOffset))
	{
		// we're seeking a buffered location (probably a 'empty' buffer that we already consumed).
        // We can rearrange the bookmarks and counts and resume the producer.
		if (nFileOffset < pDoc->GetReadByte())
		{
            // move back
			// this is the main flow case, as we took care of most forward seeks (in current blocks) in earlier code.
            DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk3(-%d)\n",pDoc->GetReadByte() - nFileOffset);
			pDoc->SeekInWindow( nFileOffset, pDoc->GetReadByte() - nFileOffset, -1, false ); // shift backwards
            
            // wake up the producer (w/o rebuffering), and we're done here.
			pPIS->GetWriterMsgPump()->ResumeWriterThread();
			return pDoc->GetReadByte();
		}
		else
		{
			// move forward
			// note that this case will usually be handled by earlier code (prior to producer pause), but that
            // since we pause the producer after testing for that situation, it may rear its head only after
            // the pause (eg, the producer may have done buffering between the checks, allowing this code to do the seek).
            DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk4(%d)\n",nFileOffset-pDoc->GetReadByte());
			pDoc->SeekInWindow( nFileOffset, nFileOffset - pDoc->GetReadByte(), 1, false ); // shift forward
            
			pPIS->GetWriterMsgPump()->ResumeWriterThread();
			return pDoc->GetReadByte();
		}
	}
    // #define BACKFILL_ON 0 Removed backfill code, if you want it back, look in vss history
    else if (pDoc->ForwardFill(nFileOffset)) // (nFileOffset > pDoc->m_nStartByte + pDoc->m_nBytesValid)
	{
        // return our read buffer to the "empty" category, returning borrowed if necessary
        DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk6(%d)\n",nFileOffset-pDoc->GetReadByte());
		if (m_pWorker->m_bReaderUsingPrevBlock)
			m_pWorker->m_bReaderUsingPrevBlock = false;
		else
			pAcct->PutInvalid();
        // set all blocks to empty
        pAcct->SetAllBlocksInvalid();
        
        // wake up the producer
		pPIS->ResumeCautiousWriting();
        
        // lock down our new read buffer
		if (pAcct->GetValid() < 0) {
			return -1;
        }
        // (epg,10/19/2001): TODO: is this obviated by the return on negative GetValid?
		if (m_pWorker->m_nFakeValidBlocks > 0)
		{
			// we don't really have a buffer, there aren't any to be had, so break out of the function.
			DEBUGP( DBG_BUF_READER, DBGLEV_WARNING,"BR:Couldn't access file after seek\n");
			m_pWorker->m_nFakeValidBlocks = 0;
			return -1;
		}
		return nFileOffset;
	}

    // we're seeking far into the past or future.  the only thing for it is a full rebuffer.
    DEBUGP( DBG_BUF_READER, DBGLEV_HIGH_LEVEL_TRACE,"br:sk7(to %d)\n",nFileOffset);
	int nPosition = pDoc->InvalidateAndReAnchorWindow( nFileOffset );
    
    pPIS->GetWriterMsgPump()->ResumeWriterThread();
    pPIS->GetWriterMsgPump()->StartProducing();
    return nPosition;
}

int CBufferReader::Read(void* pBuffer,int nBytesRequested)
{
    DEBUGP( DBG_BUF_READER, DBGLEV_LOW_LEVEL_TRACE, "br:Read\n"); 
    CBufferingImp* pPIS = CBufferingImp::GetInstance();
	CBufferDocument* pDoc = (*m_pWorker->GetActiveDocument());
	if (pDoc->IsSourceCorrupt())
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "r-1\n"); 
		return 0;
    }
	CBufferAccountant* pAcct = m_pWorker->m_pAccountant;
    // get a read buffer

    if (!m_bReadBlockLocked)
	    if (!LockBlockForReading())
        {
            DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "r-2\n"); 
            // (epg,6/21/2002): return 0, no bytes avail 
            return 0;
        }
    // fail if the document's file reports a zero size.
    if (!ValidateDocumentFileLength(pDoc))
    {
        DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "r-3\n"); 
        return -1;
    }
    // calculate how man characters are available in this buffer
	int nAvailChars = 0;
	bool bDocEnd = false;

//    DEBUGP( DBG_BUF_READER, DBGLEV_LOW_LEVEL_TRACE, "Rd%d\n",pDoc->GetNextValidBlockIndex()); 
	if (pDoc->GetPartialBlockIterator() == pDoc->GetReadBlockIterator() &&
        pDoc->GetPartialBlockIterator() != pDoc->GetBlockList()->GetEnd())
	{
		bDocEnd = true;
		pPIS->SetNearEndOfTrack();
		nAvailChars = pDoc->GetPartialBlockChars() - pDoc->GetBlockReadByte();
	}
	else
		nAvailChars = BUFFER_BLOCK_BYTES - pDoc->GetBlockReadByte();

    // if we can fully service the read request, do so.
	if (nBytesRequested < nAvailChars)
	{
        // copy the data
        BlockListIterator itBuf = pDoc->GetReadBlockIterator();
		memcpy ((char*)pBuffer, (char*)((*itBuf) + pDoc->GetBlockReadByte()), nBytesRequested);
        // update read-point counts, and return
        pDoc->SetReadByte(nBytesRequested + pDoc->GetReadByte());
		pDoc->SetBlockReadByte(pDoc->GetBlockReadByte() + nBytesRequested);
		return nBytesRequested;
	}
    // the read goes beyond the current buffer, so finish this one and recurse.
	else 
	{
        // copy the end of the buffer
        BlockListIterator itBuf = pDoc->GetReadBlockIterator();
		memcpy ((char*)pBuffer, (char*)((*itBuf) + pDoc->GetBlockReadByte()), nAvailChars);
        // update read-point counts
        pDoc->SetReadByte(pDoc->GetReadByte() + nAvailChars);
		pDoc->SetBlockReadByte(pDoc->GetBlockReadByte() + nAvailChars);
        // calculate how many more chars the caller needs
		unsigned int nRemainingChars = nBytesRequested - nAvailChars;
		if (!bDocEnd) 
		{
            // put the used up read-buffer into the "empty" category.
            if (m_pWorker->m_bReaderUsingPrevBlock)
				m_pWorker->m_bReaderUsingPrevBlock = false;
			else
				pAcct->PutInvalid();
            // remember we don't have a buffer locked down
			m_bReadBlockLocked = false;
            // recurse
            return nAvailChars + Read((unsigned char*)pBuffer+nAvailChars,nRemainingChars);
		}
		else 
        // the file is done, just return how many chars were available.
        {
            DEBUGP( DBG_BUF_READER, DBGLEV_VERBOSE, "r-4\n"); 
			return nAvailChars;
        }
	}
}


void CBufferReader::SetWorker(CBufferWorker* pWkr)
{
    m_pWorker = pWkr;
}


void CBufferReader::SetPrebufDefault(int nSeconds)
{
    if (nSeconds > PREBUF_MAX_SECONDS)
    {
        m_nPrebufDefault = PREBUF_MAX_SECONDS;
    }
    m_nPrebufDefault = nSeconds;
}

int CBufferReader::GetPrebufDefault()
{
    return m_nPrebufDefault;
}

void CBufferReader::SetQuickLockReady()
{
    m_bQuickLockReady = true;
}
