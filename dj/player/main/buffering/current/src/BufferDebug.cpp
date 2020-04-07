
#include <main/buffering/BufferDebug.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_DBG, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_BUF_DBG);  // debugging prefix : (1) bd

#if DEBUG_LEVEL > 0
    extern char __dbg_DBG_BUF_ACCOUNTANT;
    extern char __dbg_DBG_BUF_DOC;
    extern char __dbg_DBG_BUF_FACT;
    extern char __dbg_DBG_BUF_ISTREAM;
    extern char __dbg_DBG_BUF_READER;
    extern char __dbg_DBG_BUF_WRITER;
    extern char __dbg_DBG_BUF_WORKER;
    extern char __dbg_DBG_BUF_DOCORDER_AUTH;
    extern char __dbg_DBG_READER_PUMP;
    extern char __dbg_DBG_WRITER_PUMP;
#endif

void PrintDebugKey()
{
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, "Buffer Key\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " ba   = Accountant\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " bd   = Document\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " bf   = Factory\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " bss  = Stream Switcher\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " brm  = Reader Msgs\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " bwm  = Writer Msgs\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " bw   = Worker\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " doa  = Doc Order Authority\n"); 
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, " Caps = Warn/Err\n"); 
}

static int nEasterDebugCount = 0;

static char scNormalDebugLevel;

void TurnBufferDebugOn()
{
#if DEBUG_LEVEL > 0
    // stash a normal val for later..
    scNormalDebugLevel = __dbg_DBG_BUF_DOC;
    char cAllDebug = DBGLEV_TRACE | DBGLEV_INFO | DBGLEV_WARNING | DBGLEV_ERROR;
    __dbg_DBG_BUF_ACCOUNTANT |= cAllDebug;
    __dbg_DBG_BUF_DOC |= cAllDebug;
    __dbg_DBG_BUF_FACT |= cAllDebug;
    __dbg_DBG_BUF_ISTREAM |= cAllDebug;
    __dbg_DBG_BUF_READER |= cAllDebug;
    __dbg_DBG_BUF_WRITER |= cAllDebug;
    __dbg_DBG_BUF_WORKER |= cAllDebug;
    __dbg_DBG_BUF_DOCORDER_AUTH |= cAllDebug;
    __dbg_DBG_READER_PUMP |= cAllDebug;
    __dbg_DBG_WRITER_PUMP |= cAllDebug;
    DEBUGP( DBG_BUF_DBG, DBGLEV_HIGH_LEVEL_TRACE, "bd:buffer debugging is now LOUD\n"); 
#endif
}

void ReturnBufferDebuggingToNormal()
{
#if DEBUG_LEVEL > 0
    // return them all to whatever BufferDocument was set to prior to Turning on All above.
    __dbg_DBG_BUF_ACCOUNTANT = scNormalDebugLevel;
    __dbg_DBG_BUF_DOC = scNormalDebugLevel;
    __dbg_DBG_BUF_FACT = scNormalDebugLevel;
    __dbg_DBG_BUF_ISTREAM = scNormalDebugLevel;
    __dbg_DBG_BUF_READER = scNormalDebugLevel;
    __dbg_DBG_BUF_WRITER = scNormalDebugLevel;
    __dbg_DBG_BUF_WORKER = scNormalDebugLevel;
    __dbg_DBG_BUF_DOCORDER_AUTH = scNormalDebugLevel;
    __dbg_DBG_READER_PUMP = scNormalDebugLevel;
    __dbg_DBG_WRITER_PUMP = scNormalDebugLevel;
#endif
}

void MaybeTurnOnBufferDebug()
{
    if (++nEasterDebugCount> 6)
        TurnBufferDebugOn();
}

void ResetBufferDebugCount()
{
    nEasterDebugCount = 0;
}

void TurnOnAllBufferDebugging()
{
    TurnBufferDebugOn();
}
