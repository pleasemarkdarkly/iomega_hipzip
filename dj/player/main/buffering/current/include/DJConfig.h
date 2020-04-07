#ifndef __DJ_BUFFERING_CONFIGURATION_DEFINES_
#define __DJ_BUFFERING_CONFIGURATION_DEFINES_

#include <main/buffering/BufferFactory.h>

// how many hard errors to allow consecutively before considering the file as invalid
#define CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED 3
// how many blocks to leave intact before the buffer containing the read point.
#define RECLAIM_BLOCK_AGE (2)

#define BUFFER_BLOCK_BYTES (2352*15) // 34.45k, a multiple of a CDDA sector, which is 2352 bytes.
#define BUFFER_MEGS 6
#define BUFFER_BLOCKS_PER_MEG 30
#define BUFFER_BLOCKS (BUFFER_MEGS*BUFFER_BLOCKS_PER_MEG) // 30 is about 1MB
#define BUFFER_MAX_DOCS 8
// when there are this many blocks left full, we need to start filling.
#define NET_BUFFER_WAKE_THRESHOLD (2* BUFFER_BLOCKS_PER_MEG)
#define HDD_BUFFER_WAKE_THRESHOLD (2* BUFFER_BLOCKS_PER_MEG)
#define DATA_CD_BUFFER_WAKE_THRESHOLD (2* BUFFER_BLOCKS_PER_MEG)
#define CD_BUFFER_WAKE_THRESHOLD (2* BUFFER_BLOCKS_PER_MEG)
#define ARBITRARILY_LARGE_DOCSIZE (1024*1024*1024)
#define BUFFER_DOC_REOPEN_COUNT (2)
// how many blocks should be ready before a file can play back?
#define DOCSTART_PREBUFFER_BLOCKS (2)
// how many blocks into the past should we try to go when starting a new buffer point.
#define SEEK_PREBUFFER_BLOCKS (1)
// how many blocks should be filled to prime the system, before sleeping to allow decoding to get ahead
#define BLOCKS_TO_FILL_AT_LOW_PRIORITY (BUFFER_BLOCKS / 2)
// the thread priority to use when in aggressive mode
#define HIGH_DJ_BUFFERING_THREAD_PRIO 10
// the thread priority to use when not in aggressive mode
#define NORMAL_DJ_BUFFERING_THREAD_PRIO 13
// the ID used by the sdk to track input streams available
#define FATFILE_BUFFEREDINPUT_IMP_ID  0x1F

#define BUFFER_RE_CULL_THRESHOLD (BUFFER_BLOCKS_PER_MEG * 2)

// instead of differentiating between bitrates, we assume a 32kB/s rate (256kbps), since that's a handy 1s~=1block number.
#define PREBUF_BLOCKRATE_BASELINE (1) // blocks per second
#define PREBUF_DEFAULT_SECONDS (4)
#define PREBUF_MAX_SECONDS (30)


extern tBufferConfig* DJInBufferConfig();

#endif // __DJ_BUFFERING_CONFIGURATION_DEFINES_

