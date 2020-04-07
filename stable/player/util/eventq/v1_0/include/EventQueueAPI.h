//
// EventQueueAPI.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

// Both C and C++ interfaces for the event queue system

#ifndef EVENTQUEUEAPI_H_
#define EVENTQUEUEAPI_H_

#include <cyg/kernel/kapi.h>

//********* Begin C style declarations

#ifdef __cplusplus
extern "C" {
#endif

    void put_event( unsigned int, void* );
    bool try_put_event( unsigned int, void* );
    void get_event( unsigned int*, void** );
    bool timed_get_event( unsigned int*, void**, int iWaitMS );
    int event_queue_empty_count();
    int event_queue_full_count();
    int event_queue_size();


#ifdef __cplusplus
};
#endif
//********* End C style declarations


//********* Begin C++ class interface
#ifdef __cplusplus

class CEventQueueImp;

class CEventQueue 
{
public:
    static CEventQueue* GetInstance( void );
    static void Destroy( void );

    void PutEvent( unsigned int, void* );
    bool TryPutEvent( unsigned int, void* );
    void GetEvent( unsigned int*, void** );
    bool TimedGetEvent( unsigned int*, void**, int iWaitMS );
    int EmptyCount();
    int FullCount();
    int Size();

    // Tell event queue which thread reads events from it.  This info is
    // necessary to avoid deadlock when the queue starts to get full.
    void SetReader( cyg_handle_t ReaderThread );
private:

    CEventQueue();
    ~CEventQueue();

    CEventQueueImp* m_pImp;
};

#endif // __cplusplus


#endif  // EVENTQUEUEAPI_H_
