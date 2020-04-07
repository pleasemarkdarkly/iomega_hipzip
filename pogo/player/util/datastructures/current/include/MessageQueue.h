// MessageQueue.h: flexible size message queue
// danc@iobjects.com 08/28/01
// (c) Interactive Objects

#ifndef __MESSAGEQUEUE_H__
#define __MESSAGEQUEUE_H__

#include <cyg/kernel/kapi.h>  // cyg_sem_t ..
#ifdef __cplusplus

template <class DataType>
class MessageQueue
{
  public:
    MessageQueue( int iNumElems = 20 ) :
        m_iWriteIndex(0),
        m_iReadIndex(0),
        m_iNumElements(iNumElems)
        {
            m_dtItemArray = new DataType[ iNumElems ];
            cyg_semaphore_init( &m_EmptyCount, iNumElems );
            cyg_semaphore_init( &m_FullCount, 0 );
        }

    ~MessageQueue() 
        {
            cyg_semaphore_destroy( &m_EmptyCount );
            cyg_semaphore_destroy( &m_FullCount );
            delete [] m_dtItemArray;
        }
    void Put( DataType data ) 
        {
            cyg_semaphore_wait( &m_EmptyCount );
            m_dtItemArray[ m_iWriteIndex++ ] = data;
            if( m_iWriteIndex == m_iNumElements ) m_iWriteIndex = 0;
            cyg_semaphore_post( &m_FullCount );
        }
    bool TryPut( DataType data ) 
        {
            if( cyg_semaphore_trywait( &m_EmptyCount ) ) {
                m_dtItemArray[ m_iWriteIndex++ ] = data;
                if( m_iWriteIndex == m_iNumElements ) m_iWriteIndex = 0;
                cyg_semaphore_post( &m_FullCount );
                return true;
            }
            return false;
        }
    void Get( DataType* val ) 
        {
            cyg_semaphore_wait( &m_FullCount );
            *val = m_dtItemArray[ m_iReadIndex++ ];
            if( m_iReadIndex == m_iNumElements ) m_iReadIndex = 0;
            cyg_semaphore_post( &m_EmptyCount );
        }
    bool TryGet( DataType* val )
        {
            if( cyg_semaphore_trywait( &m_FullCount ) ) {
                *val = m_dtItemArray[ m_iReadIndex++ ];
                if( m_iReadIndex == m_iNumElements ) m_iReadIndex = 0;
                cyg_semaphore_post( &m_EmptyCount );
                return true;
            }
            return false;
        }
    bool TimedGet( int ticks, DataType* val )
        {
            if( cyg_semaphore_timed_wait( &m_FullCount, cyg_current_time() + ticks ) ) {
                *val = m_dtItemArray[ m_iReadIndex++ ];
                if( m_iReadIndex == m_iNumElements ) m_iReadIndex = 0;
                cyg_semaphore_post( &m_EmptyCount );
                return true;
            }
            return false;
        }
    int EmptyCount()
        {
            int i;
            cyg_semaphore_peek( &m_EmptyCount, &i );
            return i;
        }
    int FullCount()
        {
            int i;
            cyg_semaphore_peek( &m_FullCount, &i );
            return i;
        }
    int Size()
        {
            return m_iNumElements;
        }

  private:
    cyg_sem_t m_EmptyCount;
    cyg_sem_t m_FullCount;
    DataType* m_dtItemArray;
    int m_iWriteIndex;
    int m_iReadIndex;
    int m_iNumElements;
};

#endif // __cplusplus
#endif // __MESSAGEQUEUE_H__
