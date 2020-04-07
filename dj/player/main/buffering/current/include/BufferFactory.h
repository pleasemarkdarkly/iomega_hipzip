//
// BlockFactory.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//
#ifndef __CLASS_BUFFER_FACTORY_H__
#define __CLASS_BUFFER_FACTORY_H__

class CBufferWorker;

struct tBufferConfig 
{
    // bytes in a block
    int nBlockSize;
    // size to buffer in, in bytes
    int nBytes;
    // how many chunks to break the space up into
    int nBlocks;
    // input threading
    bool bInputThreaded;
    int nInputPriority;
    // output threading
    bool bOutputThreaded;
    int nOutputPriority;
    // is the source seekable
    bool bSeekable;
    // how many simultaneous documents can be prepared
    int nMaxDocs;
    // how many past docs should we keep around
    int nMaxPastDocs;

    // additional, special use windows at the endpionts of the sources.
    // manage a special window at the start of document.
    bool bDocStartWindow;
    int nDocStartWindowBlocks;
    // manage a special window at the end of document.
    bool bDocEndWindow;
    int nDocEndWindowBlocks;

    int nCDWakeThreshold;
    int nHDDWakeThreshold;
    int nDataCDWakeThreshold;
    int nNetWakeThreshold;
};

class  CBufferFactory {
public:
    static CBufferWorker* CreateWorker(tBufferConfig tBufDesc);
private:
    CBufferFactory();
    ~CBufferFactory();
};

#endif // __CLASS_BUFFER_FACTORY_H__
