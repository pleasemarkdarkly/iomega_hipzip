#include <main/buffering/DJConfig.h>

#define NULL 0

tBufferConfig* s_pCDBufferConfig = NULL;
tBufferConfig* InitCDBuffering()
{
    // size of each buffer
    s_pCDBufferConfig->nBlockSize = BUFFER_BLOCK_BYTES;
    // size to buffer in, in bytes
    s_pCDBufferConfig->nBytes = BUFFER_BLOCK_BYTES * BUFFER_BLOCKS;
    // how many chunks to break the space up into
    s_pCDBufferConfig->nBlocks = BUFFER_BLOCKS;
    // when therea are nWakeThreshold valid blocks, wake and start writing blocks
    // (epg,4/30/2002): TODO: for now, only the conservative NET threshold is used, but I will split it out according to current datasource
    // and put that in the worker's state for easy access.
    s_pCDBufferConfig->nCDWakeThreshold = NET_BUFFER_WAKE_THRESHOLD;
    s_pCDBufferConfig->nHDDWakeThreshold = HDD_BUFFER_WAKE_THRESHOLD;
    s_pCDBufferConfig->nDataCDWakeThreshold = DATA_CD_BUFFER_WAKE_THRESHOLD;
    s_pCDBufferConfig->nNetWakeThreshold = NET_BUFFER_WAKE_THRESHOLD;
    // input threading
    s_pCDBufferConfig->bInputThreaded = true;
    s_pCDBufferConfig->nInputPriority = HIGH_DJ_BUFFERING_THREAD_PRIO;
    // output threading
    s_pCDBufferConfig->bOutputThreaded = true;
    s_pCDBufferConfig->nOutputPriority = HIGH_DJ_BUFFERING_THREAD_PRIO;
    // is the source seekable
    s_pCDBufferConfig->bSeekable = true;
    // how many simultaneous documents can be prepared
    s_pCDBufferConfig->nMaxDocs = BUFFER_MAX_DOCS;
    // how many past docs should we keep around
    s_pCDBufferConfig->nMaxPastDocs = 1;

    // additional, special use windows at the endpionts of the sources.
    // manage a special window at the start of document.
    s_pCDBufferConfig->bDocStartWindow = false;
    s_pCDBufferConfig->nDocStartWindowBlocks = NULL;
    // manage a special window at the end of document.
    s_pCDBufferConfig->bDocEndWindow = false;
    s_pCDBufferConfig->nDocEndWindowBlocks = NULL;

    //s_pCDBufferConfig->
    return s_pCDBufferConfig;
}

tBufferConfig* DJInBufferConfig()
{
    if (!s_pCDBufferConfig)
    {
        s_pCDBufferConfig = new tBufferConfig;
        InitCDBuffering();
    }
    return s_pCDBufferConfig;
}
