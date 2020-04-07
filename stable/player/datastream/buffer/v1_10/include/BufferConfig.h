// BufferConfig.h: buffer_config_t structure
// danc@iobjects.com 08/28/01
// (c) Interactive Objects

#ifndef __BUFFERCONFIG_H__
#define __BUFFERCONFIG_H__

typedef struct buffer_config_s
{
    CBufferThread* pBufferThread;   // the buffer thread to use for read requests
    int iMediaWakeupTime;           // how long (in ms) does the media take to wakeup?
    int iMediaTransferRate;         // how fast (in kbytes/sec) can the media provide data?
    int iContentBitRate;            // bit rate of the content

    // Limit the characteristics of this buffer module
    int iMemoryLimit;               // maximal memory usage for this instance (0 for no limit)
    int iBlockLimit;                // how large of blocks may we read (0 for no limit)

    int iPadAmount;                 // percentage of calculated buffer size to pad
} buffer_config_t;

#endif // __BUFFERCONFIG_H__
