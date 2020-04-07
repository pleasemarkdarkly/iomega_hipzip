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

CProducer::CProducer() {}
CProducer::~CProducer() {}

// figure out what best to fill next and do so.
bool CProducer::FillBuffer()
{
	CCacheMan* pCacheMan = CCacheMan::GetInstance();
    if (!pCacheMan->IsBufferToFill())
        return false;
    
    CFileCache* pCache = pCacheMan->GetNextCacheToFill();

    // make sure the file is open
    if (!pCache->IsFileOpen())
        pCache->OpenFile();

    if (!pCache->IsCurrentCache() && !pCache->AreBuffersBOFRooted())
    {
        pCache->RewindCacheToBOF();
        pCache->ResetNextEmptyPointers();
    }

    CCacheReferee* pRef = pCacheMan->m_pCacheReferee;
	// are there full buffers we can skip over (from a backward seek and discontinuous filling)
    if (pCache->ReachedBuffersToSkip())
        pCache->SkipBuffers(pRef);
    else
    {
		// grab an empty buffer
		pRef->GetEmpty();

        // if the file pointer isn't in the spot we want to read, move it there
        pCache->NormalizeFileCursor();

        // let the cache perform the core of the fill operation
        pCache->FillBuffer();

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
            pRef->Report();
		}
	}
	return true;
}

