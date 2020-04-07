#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferFactory.h>
#include <main/buffering/BufferWorker.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_FACT, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_FACT);  // debugging prefix : (0) bf

#include <stddef.h>
#define THREAD_ENTRY_FUNC_T void*
#define THREAD int

THREAD CreateThread(int nPrio, THREAD_ENTRY_FUNC_T fnEntry, char** ppStack)
{
    // (epg,4/10/2002): TODO: create os thread
    return 0;
}

CBufferWorker* CBufferFactory::CreateWorker(tBufferConfig desc)
{
    DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:CreateWkr\n"); 
    CBufferWorker* pWorker = new CBufferWorker;
    CBufferWorker& wkr = *pWorker;

    if (desc.bDocEndWindow)
    {
        DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:+docEndWind\n"); 
        wkr.m_bDocEndWindow = true;
        wkr.m_nDocEndWindowBlocks = desc.nDocEndWindowBlocks;
    }
    if (desc.bDocStartWindow)
    {
        DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:+docStartWind\n"); 
        wkr.m_bDocStartWindow = true;
        wkr.m_nDocStartWindowBlocks = desc.nDocStartWindowBlocks;
    }
    if (desc.bInputThreaded)
    {
        DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:+inThread\n"); 
        wkr.m_bInputThreaded = true;
        wkr.m_nInputPriority = desc.nInputPriority;
        /*  // (epg,4/17/2002): TODO: allow flexible thread creation or lack thereof
        wkr.m_pInputThread = CreateThread(
                desc.nInputPriority,
                wkr.InputThreadStart,
                &wkr.m_pInputThreadStack);
        */
    }
    if (desc.bOutputThreaded)
    {
        DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:+outThread\n"); 
        wkr.m_bOutputThreaded = true;
        wkr.m_nOutputPriority = desc.nOutputPriority;
        /*  // (epg,4/17/2002): TODO: allow flexible thread creation or lack thereof
        wkr.m_pOutputThread = CreateThread(
                desc.nOutputPriority,
                wkr.OutputThreadStart,
                &wkr.m_pOutputThreadStack);
        */
    }
    wkr.m_bSeekable = desc.bSeekable;
    wkr.m_cBytes = desc.nBytes;
    wkr.m_cBlocks = desc.nBlocks;
    wkr.m_cMaxSources = desc.nMaxDocs;

    wkr.m_bfBlockRoom = new unsigned char [ wkr.m_cBytes ];
    DEBUGP( DBG_BUF_FACT, DBGLEV_HIGH_LEVEL_TRACE, "bf:%d bytes at %d\n",wkr.m_cBytes,(int)wkr.m_bfBlockRoom); 
    wkr.InitUnusedBlockList();

    wkr.SetConfig(&desc);

    return pWorker;
}

CBufferFactory::CBufferFactory()
{}
CBufferFactory::~CBufferFactory()
{}
