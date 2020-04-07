//........................................................................................
//........................................................................................
//.. File Name: Consumer.cpp															..
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
#include <main/datastream/fatfile/Consumer.h>
#include <fs/fat/sdtypes.h>
#include <fs/fat/sdapi.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/datastream/fatfile/CacheMan.h>
#include <main/datastream/fatfile/CacheReferee.h>
#include <datastream/fatfile/FileInputStream.h>
//#include "DriveInterface.h"
#include <cyg/kernel/kapi.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CACHE_CONSUMER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_CACHE_CONSUMER);
  	
CConsumer::CConsumer() : m_bReadBufferLocked(false)
{}
CConsumer::~CConsumer() 
{}

// returns true if a buffer was successfully locked for reading.
bool CConsumer::LockBufferForReading()
{
    CCacheMan* pCacheMan = CCacheMan::GetInstance();
    CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CFileCache* pCache = (*pCacheMan->GetCurrentCache());
	
	if (pRef->PeekFull() <= BUFFERS_FULL_AT_SPINUP)
    {
        // (epg,10/19/2001): TODO: decide whether to disable the cautious mode thing. (probably)
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:full low\n");
        pPIS->GetProducerMsgPump()->SetCautiousMode();
	    pPIS->GetProducerMsgPump()->StartProducing();
    }
	if (pRef->GetFull() < 0)
    {
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:couldn't get full in lock!\n");
		return false;
    }
    pRef->Report();
    
    // (epg,10/19/2001): TODO: determine if this is obviated by the above negative GetFull return.
    if (pCacheMan->m_nFakeFullBuffers > 0)
	{
		// we don't really have a buffer, there aren't any to be had, so break out of the function.
		DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:Couldn't access file after seek\n");
		pCacheMan->m_nFakeFullBuffers = 0;
		return -1;
	}
    if (pCache->GetFileOffset() == 0)
    {
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"C:init rd idx to 0\n");
        // init the read index to the first
        pCache->GetNextFullBufferCircIndex()->Set(0);
        pCache->GetReadBufferIterator() = pCache->GetBufferList()->GetHead();
    }
    else
    {
        // increment the read index
        pCache->IncrementNextFullBuffer();
    }

    DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:LK phys %d, fil %d, idx %d\n",pCacheMan->BufferIndexFromLocation((*pCache->GetReadBufferIterator())), 
        pCache->FilePositionFromBufferListIndex(pCache->GetNextFullBufferCircIndex()->Get()), pCache->GetNextFullBufferCircIndex()->Get());

	// reset the buffer level bookmark to the beginning of the new buffer.
    pCache->SetBufferOffset(0);
	m_bReadBufferLocked = true;
    return true;
}

// return true if the cache's file has a nonzero length, or tell the cache the file's eof is reached and return false.
bool ValidateCacheFileLength(CFileCache* pCache)
{
    if (!pCache->FileLength())
    {
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:invalidate file cache\n");
        pCache->SetFileEOF();
        return false;
    }
    return true;
}

// returns the length of the file, or the last valid readable character in the file, in an error situation.
int CConsumer::GetSafeFileLength(CFileCache* pCache)
{
    int nSafeLength = pCache->FileLength();
	if (pCache->IsEOFBuffered())   {
        // if there was an error of some kind, the filesize may not agree with 
        // the eof data, but the eof dictates what we can actually do in the filesystem.
		nSafeLength = pCache->GetCharCount() + pCache->GetStartChar() + 1;
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:safelen %d\n",nSafeLength);
    }
    return nSafeLength;
}

// returns true if a valid combination of origin + offset is passed in.  sets nFileOffset to a simple offset from beginning of file.
bool CConsumer::FileOffsetFromOriginOffset(CFileCache* pCache, int nFileSize, IInputStream::InputSeekPos eOrigin, int nOffset, int &nFileOffset)
{
    switch(eOrigin)
	{
	    case IInputStream::SeekStart:
		    if (nOffset < 0)
		    {
			    DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:Invalid negative offset from bof requested of seek\n");
			    return false;
		    }
		    nFileOffset = nOffset;
		    break;
	    case IInputStream::SeekCurrent:
		    nFileOffset = pCache->GetFileOffset() + nOffset;
		    break;
	    case IInputStream::SeekEnd:
		    if (nOffset > 0)
		    {			
			    DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:Invalid positive offset from eof requested of seek\n");
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
		DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:warning: seeking before start of file, forcing to start...\n");
		nFileOffset = 0;
	}
	if (nFileOffset > nFileSize - 1)
	{
		DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:warning: seeking past end of file requested, forcing to end...\n");
		nFileOffset = nFileSize - 1;	
	}
}

// calculate the limits of the current buffer.
void CConsumer::ReadBufferLimits (CFileCache *pCache, int &nBufferFloor, int &nBufferCeiling)
{
    nBufferFloor = pCache->GetFileOffset() - pCache->GetBufferOffset();
	if (pCache->GetPartialBufferIndex() == pCache->GetNextFullBufferCircIndex()->Get() && pCache->GetPartialBufferIndex() > -1) // are we in a partial buffer (eof situation)
		nBufferCeiling = nBufferFloor + pCache->GetPartialBufferChars() - 1;
	else
		nBufferCeiling = nBufferFloor + BUFFER_SIZE - 1;
}


// return the uppermost character buffered in the current cache, in a threadsafe manner.
int ThreadSafeCacheCeiling (CFileCache* pCache, CCacheReferee* pRef, int nBufferCeiling)
{
	cyg_count32 fullbufs = pRef->PeekFull();
	int nCacheCeiling = nBufferCeiling + ((fullbufs-1) * BUFFER_SIZE);
	if (pCache->IsEOFBuffered())
		nCacheCeiling += pCache->GetPartialBufferChars();
	else
		nCacheCeiling += BUFFER_SIZE;
    if (nCacheCeiling > (pCache->GetStartChar() + pCache->GetCharCount()))
        nCacheCeiling = pCache->GetStartChar() + pCache->GetCharCount();
    return nCacheCeiling;
}        

// move the consumer read point to the offset from the origin specified.
int CConsumer::Seek(IInputStream::InputSeekPos eOrigin, int nOffset)
{
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
	CFileCache* pCache = (*pCacheMan->GetCurrentCache());
	CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
//    pCache->PrintLocationReport("PreSeek");
    // make sure we have a read buffer locked
	if (!m_bReadBufferLocked)
        if (!LockBufferForReading())
            return -1;
    // fail if the cache's file reports a zero size.
    if (!ValidateCacheFileLength(pCache))
        return -1;
    // get the safe file size.
    int nFileSize = GetSafeFileLength(pCache);
    // determine file position relative to file start.
    int nFileOffset;
    if (!FileOffsetFromOriginOffset(pCache, nFileSize, eOrigin, nOffset, nFileOffset))
        return -1;		
    // make sure the file offset is within the file.
    NormalizeFileOffset(nFileSize, nFileOffset);
    // calculate limits of current buffer.
	int nBufferFloor, nBufferCeiling;
    ReadBufferLimits(pCache, nBufferFloor, nBufferCeiling);
    if ((nFileOffset >= nBufferFloor) && (nFileOffset <= nBufferCeiling))
	{
		// we seek a location within the current buffer.  since we own it, just move our bookmarks and move on.
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk0(%d)\n",nFileOffset-pCache->GetFileOffset());
		int nNewBufferOffset = nFileOffset - nBufferFloor;
		pCache->SetFileOffset(nFileOffset);
		pCache->SetBufferOffset (nNewBufferOffset);
		return nFileOffset;
	}
    // check for the case where we're seeking future buffered data.
	else if (nFileOffset >= nBufferFloor)
	{
		// calculate the (threadsafe) upper limit on the chars currently buffered.
        int nCacheCeiling = ThreadSafeCacheCeiling(pCache, pRef, nBufferCeiling);
        // do we have this location buffered in the future?
        if (nFileOffset <= nCacheCeiling)
		{
            DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk1(%d)\n",nFileOffset-pCache->GetFileOffset());
			pCache->ShiftReadPoint(	nFileOffset, nFileOffset - pCache->GetFileOffset(), 1, false );
			return pCache->GetFileOffset();
		}
	}
    // if the seek point is in the previous buffer, we can assume it is available.  this is guaranteed b/c of general policy to always
    // prebuffer at least one buffer, and to not recycle the buffer until the read point has move further on.  this case is required for
    // wma, since it routinely seeks slightly into the past, potentially into the previous buffer.
	else if (nFileOffset >= (pCache->GetFileOffset() - pCache->GetBufferOffset() - BUFFER_SIZE) && !pCacheMan->m_bConsumerBorrowingEmpty)
	{
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk2(-%d)\n",pCache->GetFileOffset() - nFileOffset);
		pCache->ShiftReadPoint( nFileOffset, pCache->GetFileOffset() - nFileOffset, -1, false );
		return pCache->GetFileOffset();
	}
    // the remaining cases require interrupting the producer thread.
	pPIS->GetProducerMsgPump()->PauseProducerThread();
	if (pCache->OffsetIsBuffered(nFileOffset))
	{
		// we're seeking a buffered location (probably a 'empty' buffer that we already consumed).  We can rearrange the bookmarks and counts and resume the producer.
		if (nFileOffset < pCache->GetFileOffset())
		{
            // move back
			// this is the main flow case, as we took care of most forward seeks (in current buffers) in earlier code.
            DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk3(-%d)\n",pCache->GetFileOffset() - nFileOffset);
			pCache->ShiftReadPoint( nFileOffset, pCache->GetFileOffset() - nFileOffset, -1, false );		// shift backwards
            // wake up the producer (w/o rebuffering), and we're done here.
			pPIS->GetProducerMsgPump()->ResumeProducerThread();
			return pCache->GetFileOffset();
		}
		else
		{
			// move forward
			// note that this case will usu be handled by earlier code (prior to producer pause), but that since we pause the producer after testing for that
			// situation, it may rear its head only after the pause (eg, the producer may have done buffering between the checks, allowing this code to do the seek).
            DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk4(%d)\n",nFileOffset-pCache->GetFileOffset());
			pCache->ShiftReadPoint( nFileOffset, nFileOffset - pCache->GetFileOffset(), 1, false );	// shift forward
			pPIS->GetProducerMsgPump()->ResumeProducerThread();
			return pCache->GetFileOffset();
		}
	}
    else if (pCache->BackFillForSeek(nFileOffset))
    {
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk5(-%d)\n",pCache->GetFileOffset() - nFileOffset);
        // return the current read buffer back into the "empty" category.
		pRef->PutEmpty();
        // set all buffers to empty
        pRef->SetAllBuffersEmpty();
        // wake up the producer
		pPIS->ResumeCautiousBuffering();
        // lock down our new read buffer
		if (pRef->GetFull() < 0)
			return -1;
        // (epg,10/19/2001): TODO: is this obviated by the return on negative GetFull?
        if (pCacheMan->m_nFakeFullBuffers > 0)
		{
			// we don't really have a buffer, there aren't any to be had, so break out of the function.
			DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"C:Couldn't access file after seek\n");
			pCacheMan->m_nFakeFullBuffers = 0;
			return -1;
		}
		return nFileOffset;
    }
	else if (pCache->ForwardFillForSeek(nFileOffset)) // (nFileOffset > pCache->m_nStartChar + pCache->m_nChars)
	{
        // return our read buffer to the "empty" category, returning borrowed if necessary
        DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk6(%d)\n",nFileOffset-pCache->GetFileOffset());
		if (pCacheMan->m_bConsumerBorrowingEmpty)
			pCacheMan->m_bConsumerBorrowingEmpty = false;
		else
			pRef->PutEmpty();
        // set all buffers to empty
        pRef->SetAllBuffersEmpty();
        // wake up the producer
		pPIS->ResumeCautiousBuffering();
        // lock down our new read buffer
		if (pRef->GetFull() < 0)
			return -1;
        // (epg,10/19/2001): TODO: is this obviated by the return on negative GetFull?
		if (pCacheMan->m_nFakeFullBuffers > 0)
		{
			// we don't really have a buffer, there aren't any to be had, so break out of the function.
			DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_WARNING,"C:Couldn't access file after seek\n");
			pCacheMan->m_nFakeFullBuffers = 0;
			return -1;
		}
		return nFileOffset;
	}
    // we're seeking far into the past or future.  the only thing for it is a full rebuffer.
    if (eOrigin == IInputStream::SeekEnd)
		pCacheMan->m_bJustSeekedFromEOF = true;
    DEBUGP( DBG_CACHE_CONSUMER, DBGLEV_TRACE,"c:sk7(to %d)\n",nFileOffset);
	return FullRebuffer( nFileOffset );
}

int CConsumer::Read(void* pBuffer,int nBytesRequested)
{
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CFileCache* pCache = (*pCacheMan->GetCurrentCache());
	if (pCache->FileIsCorrupt())
		return 0;
	CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
    // get a read buffer
    if (!m_bReadBufferLocked)
	    if (!LockBufferForReading())
            return -1;
    // fail if the cache's file reports a zero size.
    if (!ValidateCacheFileLength(pCache))
        return -1;
    // calculate how man characters are available in this buffer
	int nAvailChars = 0;
	bool bEOF = false;
	if (pCache->GetPartialBufferIndex() == pCache->GetNextFullBufferCircIndex()->Get())
	{
		bEOF = true;
		pPIS->SetNearEndOfTrack();
		nAvailChars = pCache->GetPartialBufferChars() - pCache->GetBufferOffset();
	}
	else
		nAvailChars = BUFFER_SIZE - pCache->GetBufferOffset();
	// if we can fully service the read request, do so.
	if ( nBytesRequested < nAvailChars)
	{
        // copy the data
        BufferListIterator itBuf = pCache->GetReadBufferIterator();
		memcpy ((char*)pBuffer, (char*)((*itBuf) + pCache->GetBufferOffset()), nBytesRequested);
        // update read-point counts, and return
        pCache->SetFileOffset(nBytesRequested + pCache->GetFileOffset());
		pCache->SetBufferOffset(pCache->GetBufferOffset() + nBytesRequested);
		return nBytesRequested;
	}
    // the read goes beyond the current buffer, so finish this one and recurse.
	else 
	{
        // copy the end of the buffer
        BufferListIterator itBuf = pCache->GetReadBufferIterator();
		memcpy ((char*)pBuffer, (char*)((*itBuf) + pCache->GetBufferOffset()), nAvailChars);
        // update read-point counts
        pCache->SetFileOffset(pCache->GetFileOffset() + nAvailChars);
		pCache->SetBufferOffset(pCache->GetBufferOffset() + nAvailChars);
        // calculate how many more chars the caller needs
		unsigned int nRemainingChars = nBytesRequested - nAvailChars;
		if (!bEOF) 
		{
            // put the used up read-buffer into the "empty" category.
            if (pCacheMan->m_bConsumerBorrowingEmpty)
				pCacheMan->m_bConsumerBorrowingEmpty = false;
			else
				pRef->PutEmpty();
            // remember we don't have a buffer locked down
			m_bReadBufferLocked = false;
            // recurse
            return nAvailChars + Read((unsigned char*)pBuffer+nAvailChars,nRemainingChars);
		}
		else 
            // the file is done, just return how many chars were available.
			return nAvailChars;
	}
}

// throw out the old buffers and start over at the new location (with a certain amount of prebuffering for backseek)
int CConsumer::FullRebuffer( unsigned int nFileOffset )
{
    // get contributor ptrs
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CFileCache* pCache = (*pCacheMan->GetCurrentCache());
	CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
    // return read buffer to empty status
	if (pCacheMan->IsBorrowingEmpty())
		pCacheMan->SetBorrowingEmpty(false);
	else
		pRef->PutEmpty();
    // reset all buffers to empty status	
    pRef->SetAllBuffersEmpty();
    // set up the cache for the rebuffer
    if (!pCache->FullRebufferForSeek(nFileOffset))
        return -1;
    // get the producer going
    pPIS->ResumeCautiousBuffering();
    // lock down our new read buffer
	if (pRef->GetFull() < 0)
		return -1;
	// return success
    return nFileOffset;
}
