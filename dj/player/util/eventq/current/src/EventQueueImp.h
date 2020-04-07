//
// EventQueueImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef EVENTQUEUEIMP_H_
#define EVENTQUEUEIMP_H_

#include <util/datastructures/MessageQueue.h>

class CEventQueueImp 
{
public:

    CEventQueueImp();
    ~CEventQueueImp();

    void PutEvent( unsigned int, void* );
    bool TryPutEvent( unsigned int, void* );
    void GetEvent( unsigned int*, void** );
    bool TimedGetEvent( unsigned int*, void**, int iWaitMS );
    int EmptyCount();
    int FullCount();
    int Size();

    void SetReader( cyg_handle_t ReaderThread ) 
        {
            m_Reader = ReaderThread;
        }
    
private:

    // internal event representation
    typedef struct event_s 
    {
        unsigned int key;
        void* data;
        // MessageQueue likes =
        struct event_s& operator=(struct event_s& item) 
            {
                key = item.key;
                data = item.data;
                return *this;
            }
    } event_t;

    MessageQueue<event_t> m_MessageQueue;

    cyg_handle_t m_Reader; // A handle to the thread that reads messages off of the queue
};

#endif  // EVENTQUEUEIMP_H_
