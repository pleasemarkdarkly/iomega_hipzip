//
// EventQueue.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <util/eventq/EventQueueAPI.h>
#include "EventQueueImp.h"

//
// C interface to event queue
//

void put_event( unsigned int key, void* data )
{
    CEventQueue::GetInstance()->PutEvent( key, data );
}

bool try_put_event( unsigned int key, void* data )
{
    return CEventQueue::GetInstance()->TryPutEvent( key, data );
}

void get_event( unsigned int* key, void** data )
{
    CEventQueue::GetInstance()->GetEvent( key, data );
}

bool timed_get_event( unsigned int* key, void** data, int iWaitMS ) 
{
    return CEventQueue::GetInstance()->TimedGetEvent( key, data, iWaitMS );
}

int event_queue_empty_count() 
{
    return CEventQueue::GetInstance()->EmptyCount();
}

int event_queue_full_count() 
{
    return CEventQueue::GetInstance()->FullCount();
}

int event_queue_size() 
{
    return CEventQueue::GetInstance()->Size();
}

//
// C++ event queue definition
//

// static members
static CEventQueue* s_pSingleton = 0;

CEventQueue* CEventQueue::GetInstance( void ) 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CEventQueue;
    }
    return s_pSingleton;
}

void CEventQueue::Destroy( void ) 
{
    if( s_pSingleton != 0 ) {
        delete s_pSingleton;
        s_pSingleton = 0;
    }
}


// non static members
CEventQueue::CEventQueue()
{
    m_pImp = new CEventQueueImp;
}

CEventQueue::~CEventQueue() 
{
    delete m_pImp;
}

void CEventQueue::PutEvent( unsigned int key, void* data )
{
    m_pImp->PutEvent(key, data);
}

bool CEventQueue::TryPutEvent( unsigned int key, void* data )
{
    return m_pImp->TryPutEvent(key, data);
}

void CEventQueue::GetEvent( unsigned int* key, void** data )
{
    m_pImp->GetEvent(key, data);
}

bool CEventQueue::TimedGetEvent( unsigned int* key, void** data, int iWaitMS )
{
    return m_pImp->TimedGetEvent(key, data, iWaitMS);
}

int CEventQueue::EmptyCount()
{
    return m_pImp->EmptyCount();
}

int CEventQueue::FullCount()
{
    return m_pImp->FullCount();
}

int CEventQueue::Size()
{
    return m_pImp->Size();
}

void CEventQueue::SetReader( cyg_handle_t ReaderThread )
{
    m_pImp->SetReader(ReaderThread);
}


