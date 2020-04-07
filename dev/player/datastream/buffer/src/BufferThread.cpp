// BufferThread.cpp: object to do threaded input buffering
// danc@iobjects.com 08/04/01
// (c) Interactive Objects

#include <datastream/buffer/BufferedInputStream.h>
#include <datastream/buffer/BufferThread.h>
#include <datastream/input/InputStream.h>

#include <cyg/kernel/kapi.h>
#include <util/debug/debug.h>

DEBUG_MODULE( BT );
DEBUG_USE_MODULE( BT );

//
// buffer_request_t
//

typedef struct buffer_request_s 
{
    IInputStream* pInputStream;
    input_buffer_t* pBuffer;
    int iBytesToRead;
    int iSeekTo;                // -1 if no seek needed
} buffer_request_t;

//
// support routines
//

static buffer_request_t* NewRequest() 
{
    return new buffer_request_t;
}

static void FreeRequest( buffer_request_t* p ) 
{
    delete p;
}



CBufferThread::CBufferThread( int iPriority, int iStackSize ) : m_Queue(80)
{
    m_iRefCount = 0;
    m_bStopped = false;
    m_ThreadStack = new char[iStackSize];
    if( !m_ThreadStack ) {
        // do something witty here
    }
    
    cyg_thread_create( iPriority,
                       CBufferThread::BufferThreadEntry,
                       (cyg_addrword_t) this,
                       "Buffer Thread",
                       m_ThreadStack,
                       iStackSize,
                       &m_ThreadHandle,
                       &m_ThreadData );

    cyg_thread_resume( m_ThreadHandle );
}

CBufferThread::~CBufferThread() 
{
    // stop the thread. free up resources
    m_Queue.Put( NULL );

    // soft synchronize
    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }
    // junk the thread now
    while( !cyg_thread_delete( m_ThreadHandle ) ) {
        cyg_thread_delay( 1 );
    }
    // and free the stack
    delete [] m_ThreadStack;
}

int CBufferThread::Buffer( IInputStream* pInputStream, input_buffer_t* pBuffer, int iCount, int iSeekTo ) 
{
    // assemble the arguments into a structure. place on the message queue
    buffer_request_t* pReq = NewRequest();

    if( !pReq ) return -1;

    pReq->pInputStream = pInputStream;
    pReq->pBuffer = pBuffer;
    pReq->iBytesToRead = iCount;
    pReq->iSeekTo = iSeekTo;

    m_Queue.Put( pReq );

    return 0;
}

void CBufferThread::BufferThreadEntry( cyg_addrword_t data ) 
{
    reinterpret_cast<CBufferThread*>(data)->BufferThread();
}

//
// Handle buffering requests
//

void CBufferThread::BufferThread() 
{
    while( true ) {
        buffer_request_t* pReq;
        
        m_Queue.Get( &pReq );
        
        if( pReq ) {
            // perform the requested read
            // post to the given semaphore
            if( pReq->iSeekTo >= 0 ) {
                if( pReq->pInputStream->CanSeek() &&
                    pReq->iSeekTo != pReq->pInputStream->Position() ) {
                    pReq->pInputStream->Seek( IInputStream::SeekStart, pReq->iSeekTo );
                }
                else {
                    // bad request, ignore
                    FreeRequest( pReq );
                    continue;
                }
            }

            // we should be at the appropriate position now
            // verify we have sufficient space for the read
            input_buffer_t* pBuf = pReq->pBuffer;
            int Space = pBuf->iBufferSize - pBuf->iBufferLength;

            if( Space < pReq->iBytesToRead ) {
                // TODO make a valiant attempt to read in data anyways?
                // TODO fix
                pReq->iBytesToRead = Space;
            }

            // this should be a light call
            pBuf->iBufferBase = pReq->pInputStream->Position();
            
            int Count = pReq->pInputStream->Read( pBuf->pBuffer + pBuf->iBufferLength,
                                                  pReq->iBytesToRead );

            if( Count < 0 ) {
                DEBUG( BT, DBGLEV_ERROR, "Physical read fail\n");
                // TODO handle error propogation somehow
            } else {
                // update the buffer count of valid data,
                // and notify the requester of the update
                pBuf->iBufferPos = 0;
                pBuf->iBufferLength += Count;
                if( Count < pReq->iBytesToRead ) {
                    // assume EOF
                    pBuf->bEOF = 1;
                }
                
                int val;
                cyg_semaphore_peek( &( pBuf->DataAvailable ), &val );
                if( !val ) {
                    cyg_semaphore_post( &( pBuf->DataAvailable ) );
                }
            }
                
            // we're now done with pReq
            FreeRequest( pReq );
        }
        else {
            // stop loop
            break;
        }
    }

    m_bStopped = true;
}
