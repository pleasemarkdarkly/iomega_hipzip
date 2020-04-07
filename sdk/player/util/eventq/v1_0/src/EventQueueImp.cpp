//
// EventQueueImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "EventQueueImp.h"

#define EVENT_TABLE_SIZE 60     //CYGNUM_KERNEL_SYNCH_MBOX_QUEUE_SIZE

//
// Verify that the kernel has support for timed operations. If not, we want to generate a warning message
// on timed get operations.
//

#if (CYGMFN_KERNEL_SYNCH_CONDVAR_TIMED_WAIT==0)
#include <cyg/infra/diag.h>
#define TIMED_OP_ASSERT() diag_printf("%s %d: timed operation called (but your kernel does not support this)",__FUNCTION__,__LINE__)
#else
#define TIMED_OP_ASSERT() //
#endif

//
// C++ event queue definition
//

// non static members
CEventQueueImp::CEventQueueImp() : m_MessageQueue( EVENT_TABLE_SIZE )
{
}

CEventQueueImp::~CEventQueueImp() 
{
}

void CEventQueueImp::PutEvent( unsigned int key, void* data )
{
    // pop an element off the empty queue
    event_t Event;
    Event.key = key;
    Event.data = data;

    // If the queue gets near full, make every thread wait except the
    // one reading so as to make some more room and prevent deadlock
    // when the queue fills up.
    bool bReaderThread = (m_Reader == cyg_thread_self());
    while (!bReaderThread && (EmptyCount() <= 6)) {
        cyg_thread_delay(50);
    }
    
    // push it into the full queue
    m_MessageQueue.Put( Event );
}

bool CEventQueueImp::TryPutEvent( unsigned int key, void* data )
{
    event_t Event;

    Event.key = key;
    Event.data = data;
    return m_MessageQueue.TryPut( Event );
}

void CEventQueueImp::GetEvent( unsigned int* key, void** data )
{
    event_t Event;
    m_MessageQueue.Get( &Event );
    
    *key = Event.key;
    *data = Event.data;
}

bool CEventQueueImp::TimedGetEvent( unsigned int* key, void** data, int iWaitMS )
{
    TIMED_OP_ASSERT();

    event_t Event;
    
    if( m_MessageQueue.TimedGet( iWaitMS, &Event ) ) {
        *key = Event.key;
        *data = Event.data;
        return true;
    }
    return false;
}

int CEventQueueImp::EmptyCount()
{
    return m_MessageQueue.EmptyCount();
}

int CEventQueueImp::FullCount()
{
    return m_MessageQueue.FullCount();
}

int CEventQueueImp::Size()
{
    return m_MessageQueue.Size();
}


