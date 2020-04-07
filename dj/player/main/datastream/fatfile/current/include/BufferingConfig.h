#ifndef __BUFFERING_CONFIGURATION_DEFINES_
#define __BUFFERING_CONFIGURATION_DEFINES_

// how many buffers are in the system total
#define BUFFER_COUNT (4) 
// how many bytes are in one buffer
#define BUFFER_SIZE (63 * 1024)
// the total byte count of all buffers
#define CACHE_SPACE_BYTES (BUFFER_COUNT*BUFFER_SIZE)
// the maximum allowed number of caches, and therefore files cached
#define MAX_CACHE_COUNT (12)
// how many caches representing previous tracks are retained in the playlist sync phase
#define PREV_TRACK_CACHE_COUNT (4)
// how many hard errors to allow consecutively before considering the file as invalid
#define CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED 3
// how many buffers to leave intact before the buffer containing the read point.
#define BUFFERS_EMPTY_AT_SPINDOWN (2)
// when there are this many buffers left full, we need to start filling.
#define BUFFERS_FULL_AT_SPINUP (2)
// how many buffers into the past should we try to go when starting a new buffer point.
#define BUFFERS_TO_PREBUFFER (1)
// how many buffers should be filled to prime the system, before sleeping to allow decoding to get ahead
#define BUFFERS_TO_FILL_AT_LOW_PRIORITY (BUFFER_COUNT / 2)
// the thread priority to use when in aggressive mode
#define HIGH_BUFFERING_THREAD_PRIO 10
// the thread priority to use when not in aggressive mode
#define NORMAL_BUFFERING_THREAD_PRIO 14
// the ID used by the sdk to track input streams available
#define FATFILE_BUFFEREDINPUT_IMP_ID  0x1F

#endif // __BUFFERING_CONFIGURATION_DEFINES_
