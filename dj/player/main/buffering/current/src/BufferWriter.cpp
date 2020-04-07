//........................................................................................
//........................................................................................
//.. File Name: Writer.cpp															..
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

#include <string.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferDocument.h>
#include <datastream/fatfile/FileInputStream.h>
#include <main/buffering/BufferWorker.h>
#include <main/buffering/BufferWriter.h>
#include <main/buffering/BufferAccountant.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_WRITER, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_WRITER);  // debugging prefix : (51) bd

CBufferWriter::CBufferWriter() : m_pWorker(0) {}
CBufferWriter::~CBufferWriter() {}

// figure out what best to fill next and do so.
eBWResult CBufferWriter::WriteBlock()
{
    DEBUGP( DBG_BUF_WRITER, DBGLEV_LOW_LEVEL_TRACE, "bw:W\n"); 
    if (!m_pWorker->IsBlockToFill())
    {
        DEBUGP( DBG_BUF_WRITER, DBGLEV_LOW_LEVEL_TRACE, "bw:no block to fill\n"); 
        // notify the caller that this thread can rest
        return BW_DONE;
    }
    
    CBufferDocument* pDoc = m_pWorker->GetDocumentToWrite();

    // make sure the file is open
    if (!pDoc->IsSourceOpen())
        if (!pDoc->OpenSource())
            // return BW_OK to continue writing.  the failure to open
            // will have marked the document corrupt, and next time we'll move onto
            // another.
            return BW_OK;

    if (!pDoc->IsDocumentActive() && !pDoc->WindowAtDocStart())
    {
        DEBUGP( DBG_BUF_WRITER, DBGLEV_VERBOSE, "bd:jumping in write for inactive nonbof\n"); 
        pDoc->JumpToDocStart();
        pDoc->ResetNextInvalidBlock();
    }

    CBufferAccountant* pAcct = m_pWorker->m_pAccountant;
	// are there full blocks we can skip over (from a backward seek and discontinuous filling)
    eBWResult result = BW_OK;

    if (pDoc->ReachedOrphanWindow())
    {
        DEBUGP( DBG_BUF_WRITER, DBGLEV_VERBOSE, "bd:reclaim orphan window\n"); 
        pDoc->ReclaimOrphanWindow(pAcct);
        // we have reactivated some non-contiguous blocks with the active window.  
        // that's as good as a write, so return BW_OK.
        result = BW_OK;
    }
    else
    {
		// grab an empty buffer
		pAcct->GetInvalid();

        // if the file pointer isn't in the spot we want to read, move it there
        pDoc->ThrottleFileIndex();

        // at this point, the theory goes that we definitely have a block to write into.  when theory fails,
        // invoke last-ditch effort to find more blocks.  the incidence of this condition is very low at this
        // point, so this might be enough to at least make it irrelevantly rare.
        if (pDoc->m_bNextInvalidOffEnd)
        {
            DEBUGP( DBG_BUF_WRITER, DBGLEV_INFO, "BD:error condition detected, turning up debugging loudness for state investigation.\n"); 
            TurnOnAllBufferDebugging();
            CBufferingImp::GetInstance()->m_pWorker->ReportOnAllDocuments("before last-ditch cull");
            DEBUGP( DBG_BUF_WRITER, DBGLEV_INFO, "bd:pragmatic cull\n"); 
            CBufferingImp::GetInstance()->GetWriterMsgPump()->CullAtLowPrio();
            if (pDoc->m_bNextInvalidOffEnd)
            {
                CBufferingImp::GetInstance()->m_pWorker->ReportOnAllDocuments("after last-ditch cull");
                // we're not going to be able to write into this doc, so return the invalid into the mix, and sleep on it.
                // (epg,10/1/2002): what I'm seeing is, this occurs when we're way out in the future and don't really need
                // to be filling anything, so it's best to just not worry about it and go to sleep.  I also fixed a possible
                // misfire in GetDocumentToWrite, where a zero-length file would seem to need filling despite being all done.
                DEBUGP( DBG_BUF_WRITER, DBGLEV_INFO, "bd:pragmatic sleep\n"); 
                pAcct->ReturnInvalid();
                // turn things back down
                ReturnBufferDebuggingToNormal();
                return BW_DONE;
            }
            else
            {
                DEBUGP( DBG_BUF_WRITER, DBGLEV_INFO, "bd:pragmatic cull succeeded\n"); 
                // turn things back down and carry on
                ReturnBufferDebuggingToNormal();
            }
        }

        // do the read, with error and eof handling.
        result = pDoc->WriteBlock();

		// update the current track counts.
		if (m_pWorker->m_nReAnchorPastBlocks > 0)
		{
			// if we're working just before the read point, to have some back-seek data onhand,
			// then the full buffer actually gets posted to 'empty' which really has 'past or empty blocks', relating to the document chars count.
			m_pWorker->m_nReAnchorPastBlocks--;
             
			pAcct->PutInvalid();
		}
		// recheck that the current active caches are still what the were at the top of the function (avoid next-track timing errors)
		else 
		{
			// generally, we fill blocks ahead of the read point, so post to 'full', which implies 'future'
            DEBUGP( DBG_BUF_WRITER, DBGLEV_VERBOSE, "bd:V+\n"); 
			pAcct->PutValid();
            pAcct->Report();
		}
	}
	return result;
}

void CBufferWriter::SetWorker(CBufferWorker* pWkr)
{
    m_pWorker = pWkr;
}
