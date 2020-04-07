#ifndef __BUFFER_DEBUG_H__
#define __BUFFER_DEBUG_H__

#define BUF_DBG_ON DBGLEV_INFO
#define BUF_DBG_OFF DBGLEV_TRACE

// activate to enable buffering debug print info.
//#define DBGLEV_BUF_COMMON (DBGLEV_INFO)
#define DBGLEV_BUF_COMMON 0

#define DBGLEV_LOCATION_TRACE   BUF_DBG_ON
//#define DBGLEV_LOCATION_TRACE   BUF_DBG_OFF

#define DBGLEV_TRACE_IN_OUT     BUF_DBG_ON
//#define DBGLEV_TRACE_IN_OUT     BUF_DBG_OFF

//#define DBGLEV_LOW_LEVEL_TRACE  BUF_DBG_ON
#define DBGLEV_LOW_LEVEL_TRACE  BUF_DBG_OFF

#define DBGLEV_HIGH_LEVEL_TRACE BUF_DBG_ON
//#define DBGLEV_HIGH_LEVEL_TRACE BUF_DBG_OFF

//#define DBGLEV_VERBOSE          BUF_DBG_ON
#define DBGLEV_VERBOSE          BUF_DBG_OFF

void PrintDebugKey();
void TurnOnAllBufferDebugging();
// must be called after turn-on-all-debug, will return to some semblance of normal output levels.
void ReturnBufferDebuggingToNormal();
void MaybeTurnOnBufferDebug();
void ResetBufferDebugCount();

#endif // __BUFFER_DEBUG_H__
