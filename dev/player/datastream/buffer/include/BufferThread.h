// BufferThread.h: object to do threaded input buffering
// danc@iobjects.com 08/04/01
// (c) Interactive Objects

#ifndef __BUFFERTHREAD_H__
#define __BUFFERTHREAD_H__

#include <cyg/kernel/kapi.h>   // cyg_mbox_t, cyg_thread, etc
#include <util/datastructures/MessageQueue.h>

// fdecl
class IInputStream;
typedef struct input_buffer_s input_buffer_t;
typedef struct buffer_request_s buffer_request_t;

class CBufferThread 
{
public:
    CBufferThread( int iPriority = 10, int iStackSize = 8192 );
    ~CBufferThread();

    void IncRef() 
        {
            m_iRefCount++;
        }
    void DecRef() 
        {
            m_iRefCount--;
            if( m_iRefCount == 0 ) {
                delete this;
            }
        }
    int Buffer( IInputStream* pInputStream, input_buffer_t* pBuffer, int iCount, int iSeekTo = -1 );
private:

    static void BufferThreadEntry( cyg_addrword_t data );
    void BufferThread();

    int m_iRefCount;

    // Messagebox data
    MessageQueue< buffer_request_t* > m_Queue;
    
    cyg_handle_t m_ThreadHandle;
    cyg_thread m_ThreadData;
    char* m_ThreadStack;

    bool m_bStopped;
};


#endif // __BUFFERTHREAD_H__
