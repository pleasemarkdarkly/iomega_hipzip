//........................................................................................
//........................................................................................
//.. File Name: Producer.cpp															..
//.. Date: 01/25/2001																	..
//.. Author(s): Eric Gibbs																..
//.. Description of content:															..
//.. Usage:																				..
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 03/27/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <string.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/datastream/fatfile/FileCache.h>
#include <datastream/fatfile/FileInputStream.h>
#include <main/datastream/fatfile/CacheMan.h>
#include <main/datastream/fatfile/Producer.h>
#include <main/datastream/fatfile/CacheReferee.h>
#include <util/debug/debug.h>
#include <errno.h>
#include <fs/fat/sdapi.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CACHE_PRODUCER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_CACHE_PRODUCER );


//#include <dadio/io/atapi.h>
//#include "DriveInterface.h"

#define LBA_BLOCK_SIZE (512)

CProducer::CProducer() {}
CProducer::~CProducer() {}

// figure out what best to fill next and do so.
int CProducer::FillBuffer()
{
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
	CBufferedFatFileInputStreamImp* pPIS = CBufferedFatFileInputStreamImp::GetInstance();
	CFileCache* pCache = pPIS->GetCache();
	// (epg,10/19/2001): if nothing to fill, return
	CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
	cyg_count32 nFull;
	nFull = pRef->PeekFull();
	int nIndexToFill = pCache->m_nNextEmptyBuffer->Get();
	unsigned char* pBuffer = pCache->m_aBuffers[nIndexToFill];
	// are there full buffers we can skip over (from a backward seek and discontinuous filling)
	if ( pCache->m_nSkipStart == pCache->m_nNextEmptyBuffer->Get() )
	{
		// skip over the region
		pCache->m_nChars += pCache->m_nSkipExtent * BUFFER_SIZE;
		// perform overflow cleanup
		while (pCache->m_nChars > pCache->m_nBuffers * BUFFER_SIZE)
		{	
			pCache->m_nChars -= BUFFER_SIZE;
			pCache->m_nStartChar += BUFFER_SIZE;
		}
		// if we're counting (b/c on current active track), mark skipped bufs as full
		for (int i = 0; i < pCache->m_nSkipExtent; i++ )
		{
			if (pRef->GetEmpty() >= 0)
				pRef->PutFull();
			pCache->m_nNextEmptyBuffer->Inc();
		}
		// if restoring eof buffer from a backfill state, set up the cache flags
		if (pCache->m_bSkipEOF)
		{
			pCache->m_bEOF = true;
			pCacheMan->m_bFirstSpinup = false;
			pCache->m_nPartialIndex = pCache->m_nSkipStart + pCache->m_nSkipExtent - 1;
			pCache->m_nPartialChars = pCache->m_nSkipEOFChars;
			if (pCache->m_nPartialIndex >= pCache->m_nBuffers)
				pCache->m_nPartialIndex -= pCache->m_nBuffers;
		}
		// move the file pointer out to the new read location (past skip bufs), or to eof
		int nCharsToSkip = (pCache->m_nSkipExtent - 1) * BUFFER_SIZE;
		if (pCache->m_bSkipEOF)
			nCharsToSkip += pCache->m_nPartialChars;
		else
			nCharsToSkip += BUFFER_SIZE;
		pCache->m_pFile->Seek( IInputStream::SeekCurrent, nCharsToSkip);
		// clear out skip flags
		pCache->m_nSkipStart = -1;
		pCache->m_nSkipExtent = 0;
		pCache->m_bSkipEOF = false;	// consume the status variable's state
	}
	else	// normal fill case, nothing to skip
	{
		// grab an empty buffer count
		pRef->GetEmpty();
		CFatFileInputStream* pFile = pCache->GetFile();
		// if the file pointer isn't in the spot we want to read, move it there
		int startchar = pCache->m_nChars + pCache->m_nStartChar;
		if (pFile->Position() != startchar)
			pFile->Seek(IInputStream::SeekStart, startchar);
		// read from the file
		int result = pFile->Read((void*)pBuffer, BUFFER_SIZE);
		// if the first read failed, try waking the drive and repeating
		if (result == -1 && errno != EREAD)
		{
			WakeDrive();
			result = pFile->Read((void*)pBuffer, BUFFER_SIZE);
		}
#define RE_READ_FUDGE_FACTOR (350)		// don't read right up to the last good char as reported, but rather this far back
#define DEADSPACE_CHARS 32				// insert this many null bytes between sections of good data to demarcate errors to the codec
		if(result == -1)
		{
			unsigned int nValidChars;
            // (epg,10/18/2001): TODO: if hard error handling is required on hdd, activate it in fs/fat.
			if ( errno == EREAD ) 
			{
				// a hard error has encountered, so there is a region of the disk that is unavailable for reading.
				pCache->m_nHardErrors = 0;
				bool bHardError=true;
				int nPreReadPos = pCache->m_nChars + pCache->m_nStartChar;
				int nBytesToRead = BUFFER_SIZE;
				int nBytesAlreadyRead = 0;
				static int nLastDeadSpace = -1;
				// keep trying to read after the error until we either find the end or give up from many retries
				while (bHardError && pCache->m_nHardErrors < CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
				{
					pCache->m_nHardErrors++;
					bHardError = false;
					// find out how much of the read was considered good
					nValidChars = pc_get_valid_read_data();
					int nErrorPos = nPreReadPos + nValidChars;				// this is where the error occured
					int nErrorLBAStart = nErrorPos - (nErrorPos % 512);		// this is the start of the bad lba
					int nCurrentPos = pFile->Seek(IInputStream::SeekCurrent,0);
					// move the file pointer to before the error
					if (nCurrentPos != nPreReadPos)
					{
						int seekret = pCache->GetFile()->Seek(IInputStream::SeekStart, nPreReadPos);
						if (seekret != nPreReadPos)	{
							DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "apparent reseek failure ret %d != prepos %d\n", seekret, nPreReadPos);
						}
					}
					// re read the portion of data that was good before the hard error.
					int nCharsToReread = nValidChars-RE_READ_FUDGE_FACTOR;
					if (nCharsToReread < 0)
						nCharsToReread = 0;
					result = pFile->Read((void*) pBuffer, nCharsToReread);
					if (result == -1)
					{
						if (errno == EREAD)
						{
							// another hard error, so loop again
							bHardError = true;
							// punctuate the error region with a 'deadspace' of zero chars
							// so the codec can identify data boundaries and doesn't just
							// keep trying to decode the stream across the discontinuity.  reduces
							// codec garbling considerably to have this padding.
							if (nLastDeadSpace != (int)pBuffer)
							{
								memset(pBuffer,0,DEADSPACE_CHARS);
								pBuffer += DEADSPACE_CHARS;
								nLastDeadSpace = (int)pBuffer;
								nBytesToRead -= (DEADSPACE_CHARS);
								nBytesAlreadyRead += DEADSPACE_CHARS;
							}
							continue;
						}
						else
						{
							// non hard-error failure
							DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "unknown po read error in pre-harderror reread\n");
							result = nBytesAlreadyRead;
						}
					}
					else
					{
						// good read, so track read progress
						nBytesToRead -= result;
						nBytesAlreadyRead += result;
					}

					if (nLastDeadSpace != (int)pBuffer)
					{
						// punctuate the error region with a 'deadspace' of zero chars
						memset(pBuffer,0,DEADSPACE_CHARS);
						pBuffer += DEADSPACE_CHARS;
						nLastDeadSpace = (int)pBuffer;
						nBytesToRead -= (DEADSPACE_CHARS);
						nBytesAlreadyRead += DEADSPACE_CHARS;
					}
					// now move past and start reading data again.
					int nNextGoodLBAStart = nErrorLBAStart + LBA_BLOCK_SIZE;
					int nLoc = pFile->Seek(IInputStream::SeekStart,nNextGoodLBAStart);
					if (nLoc != nNextGoodLBAStart) {
						DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "sk ret %d != %d\n",nLoc,nNextGoodLBAStart);
					}
					if (nBytesToRead>0)
					{	
						// still more to read
						result = pFile->Read((void*) (pBuffer + nBytesAlreadyRead), nBytesToRead);
						if (result == -1)
						{
							if (errno == EREAD)
							{
								// we got another hard error, so update state and fall through to loop again
								nPreReadPos = nNextGoodLBAStart;
								bHardError = true;
								if (nLastDeadSpace != (int)pBuffer)
								{
									memset(pBuffer,0,DEADSPACE_CHARS);
									pBuffer += DEADSPACE_CHARS;
									nLastDeadSpace = (int)pBuffer;
									nBytesToRead -= (DEADSPACE_CHARS);
									nBytesAlreadyRead += DEADSPACE_CHARS;
								}
							}
							else
							{
								// non hard error fault, just eject
								DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "unknown error in post HE read\n");
								result = nBytesAlreadyRead;
							}
						}
						else
						{
							// the read was good, so we're back on target.  leave the hole in the buffer and go on.
							nBytesAlreadyRead += result;
							nBytesToRead -= result;
							if (nBytesToRead != 0) {
								DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "!nonzero bytes to read after HE handling\n");
							}
							bHardError= false;
							result = nBytesAlreadyRead;		// eof handler will see this value
						}
					}
				}
				if (pCache->m_nHardErrors >= CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED)
				{
					// the disk failed too many times in a row.  give up on the file
					DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "!%d consecutive hard errors encountered, marking file as bad\n",CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED);
					pCache->m_nHardErrors = 0;
					pCache->m_bFileCorrupt = true;
					result = nBytesAlreadyRead;
				}
			}
			else
			{
				// if there was an unknown error, just give up and return that we couldn't get any chars (resulting in a next track scenario)
				DEBUGP( DBG_CACHE_PRODUCER, DBGLEV_INFO, "P:*Unknown po_read error encountered, abandoning file\n");
				pCache->m_bFileCorrupt = true;
				pCache->m_bEOF = true;
				return 0;
			}
		}
		if (result != BUFFER_SIZE)
		{
			// a partial read signifies the end of file
			pCache->m_bEOF = true;
			if (result <= 0)
			{
				// a zero result means the last char of the file was in the previous read, right at the end
				// move pointers back and consider that previous buf as the eof buf.
				pCache->m_nNextEmptyBuffer->Dec();			
				pCache->m_nPartialChars = BUFFER_SIZE;
				pCacheMan->m_nRebufferPastBuffersToFill++;	
				if (pRef->PeekFull() <= 0)
					// in case the reader is waiting on this fill, notify it that nothing is coming
					pRef->JogFull();
			}
			else
				pCache->m_nPartialChars = result;
			pCache->m_nPartialIndex = pCache->m_nNextEmptyBuffer->Get();
		}
		// update cache count of chars
		pCache->m_nChars += result;
		while (pCache->m_nChars > pCache->m_nBuffers * BUFFER_SIZE)
		{	
			pCache->m_nChars -= BUFFER_SIZE;
			pCache->m_nStartChar += BUFFER_SIZE;
		}
		pCache->m_nNextEmptyBuffer->Inc();
		// update the current track counts.
		if (pCacheMan->m_nRebufferPastBuffersToFill > 0)
		{
			// if we're working just before the read point, to have some back-seek data onhand,
			// then the full buffer actually gets posted to 'empty' which really has 'past or empty buffers', relating to the cache chars count.
			pCacheMan->m_nRebufferPastBuffersToFill--;
			pRef->PutEmpty();
		}
		// recheck that the current active caches are still what the were at the top of the function (avoid next-track timing errors)
		else 
		{
			// generally, we fill buffers ahead of the read point, so post to 'full', which implies 'future'
			pRef->PutFull();
		}
	}
	return 0;
}

