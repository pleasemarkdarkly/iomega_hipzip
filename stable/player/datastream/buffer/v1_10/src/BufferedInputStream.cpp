// BufferedInputStream.cpp: a buffered input stream
// danc@iobjects.com 08/07/01
// (c) Interactive Objects

#include <datastream/buffer/BufferedInputStream.h>
#include <datastream/buffer/BufferThread.h>
#include <datastream/buffer/BufferConfig.h>

#include <util/debug/debug.h>

DEBUG_MODULE( BIS );
DEBUG_USE_MODULE( BIS );


CBufferedInputStream::CBufferedInputStream() 
{
    m_StreamState = NOT_CONFIGURED;
}

CBufferedInputStream::~CBufferedInputStream() 
{
    switch( m_StreamState ) {
        case BUFFERED:     // FALL THROUGH
        case PREBUFFERED:  // FALL THROUGH
        case EMPTY:        // FALL THROUGH
        {
            for( int i = 0; i < m_iBufferCount; i++ ) {
                delete[] m_pBuffers[i].pBuffer;
                cyg_semaphore_destroy( &( m_pBuffers[i].DataAvailable ) );
            }

            delete[] m_pBuffers;
            m_pBufferThread->DecRef();
        }
        case NOT_CONFIGURED:
        {
            if( m_pInputStream ) {
                delete m_pInputStream;
            }
            break;
        }
    };
}


int CBufferedInputStream::Close() 
{
    if( !m_pInputStream ) return 0;
    m_pInputStream->Close();
    return 0;
}

int CBufferedInputStream::Read( void* Buffer, int Count ) 
{
    //    DEBUG( BIS, DBGLEV_ERROR, "Buffer = 0x%08x, Count = %d\n", Buffer, Count );
    if( !m_pInputStream ) {
        return -1;
    }
    if( m_StreamState == NOT_CONFIGURED ) {
        return m_pInputStream->Read( Buffer, Count );
    } else {
        int iBytesLeft = Count;
        while( iBytesLeft ) {
            int iBytesAvail = m_pCurrentReadBuffer->iBufferLength - m_pCurrentReadBuffer->iBufferPos;
            if( iBytesAvail == 0 ) {
                if( m_pCurrentReadBuffer->bEOF ) {
                    return 0;
                }
                if( !m_pCurrentReadBuffer->iBufferLength ) {
                    // hmm, current read buffer is empty
                    // wait on the semaphore until data becomes available
                    DEBUG(BIS, DBGLEV_WARNING, "Going to wait on a buffer semaphore.\n");
                    cyg_semaphore_wait( &( m_pCurrentReadBuffer->DataAvailable ) );
                    continue;
                } else {
                    m_pCurrentReadBuffer->iBufferLength = m_pCurrentReadBuffer->iBufferPos = 0;
                    NextReadBuffer();
                    continue;
                }
            }

            // copy some data
            int iBytesToCopy = (iBytesAvail > iBytesLeft ? iBytesLeft : iBytesAvail);

            // (Count - iBytesLeft) == iBytesCopied
            memcpy( (unsigned char*)Buffer + (Count - iBytesLeft),
                    m_pCurrentReadBuffer->pBuffer + m_pCurrentReadBuffer->iBufferPos,
                    iBytesToCopy );

            // update our state
            iBytesLeft -= iBytesToCopy;
            
            m_pCurrentReadBuffer->iBufferPos += iBytesToCopy;
            m_iBytesAvailable -= iBytesToCopy;

            if( m_iBytesAvailable <= m_iFillLimit ) {
                // fill up some buffers
                // right now this throws it in rampage mondo fill mode, we should probably pace this
                FillBufferBytes( m_iTotalBufferSpace - m_iBytesAvailable );
            }
        }
        return (Count - iBytesLeft);
    }
}

int CBufferedInputStream::Ioctl( int Key, void* Value ) 
{
    if( !m_pInputStream ) {
        return -1;
    }
    return m_pInputStream->Ioctl( Key, Value );
}

int CBufferedInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    if( !m_pInputStream ) {
        return -1;
    }

    if( m_StreamState == NOT_CONFIGURED ) {
        return m_pInputStream->Seek( SeekOrigin, Offset );
    }
    
    int SeekDest;
    
    switch( SeekOrigin ) {
        case SeekCurrent:
        {
            SeekDest = Position();
            break;
        }
        case SeekEnd:
        {
            int len = Length();
            if( len == 0 ) {  // length not available !!
                return -1;
            }
            SeekDest = len;
            break;
        }
        case SeekStart:
        {
            SeekDest = 0;
            break;
        }
    }
    SeekDest += Offset;

    if( (SeekDest < m_iLowerByteLimit) || (SeekDest > m_iUpperByteLimit) ) {
        // TODO rebuffer
    }
    // TODO fix
    return SeekDest;
}

int CBufferedInputStream::Length() const 
{
    if( !m_pInputStream ) return 0;
    return m_pInputStream->Length();
}

int CBufferedInputStream::Position() const 
{
    if( !m_pInputStream ) return 0;
    return m_pCurrentReadBuffer->iBufferBase + m_pCurrentReadBuffer->iBufferPos;
}

void CBufferedInputStream::SetSong( IInputStream* pInputStream, IContentRecord* pContentRecord ) 
{
    if( m_StreamState == NOT_CONFIGURED ) {
        m_pContentRecord = pContentRecord;
        m_pInputStream   = pInputStream;
        m_iInputUnitSize = m_pInputStream->GetInputUnitSize();
    }
}

int CBufferedInputStream::Configure( const buffer_config_t* pConfiguration ) 
{
    if( m_StreamState == NOT_CONFIGURED && m_pInputStream ) {
        m_pBufferThread  = pConfiguration->pBufferThread;
        m_pBufferThread->IncRef();
        
        // Calculate how many bytes per second of audio based on the bitrate
        m_iBytesPerSecond = pConfiguration->iContentBitRate / 8;

        // Silly, but make sure this is in units the input stream can handle
        m_iBlockLimit = pConfiguration->iBlockLimit;
        m_iBlockLimit -= ( m_iBlockLimit % m_iInputUnitSize );

        // Figure out how large we need to make the buffers
        // A buffer should hold just enough audio for a drive spinup and single read to complete
        //  - iMediaWakeupTime is in MS, so convert to sec
        
        // this is the amount we need to buffer while spinning up
        int iBufferSize = ((m_iBytesPerSecond * pConfiguration->iMediaWakeupTime) / 1000 );

        // add in the amount we need to have while reading the next frame
        //  note we fudge a bit, since we dont know the actual size of the next frame just
        //  yet, but we will pad the buffer size to accomodate this
        iBufferSize += ( iBufferSize / pConfiguration->iMediaTransferRate );

        // Since the calculation comes up a little bit short, and some systems will spend a lot
        //  of time either decoding or processing other system events, the user can specify how
        //  much additional padding to put in each buffer. This ensures the decode process never
        //  starves. The pad amount is a percentage - that is, if iPadAmount is '25' then the
        //  buffers will be 125% of their calculated size
        iBufferSize +=  ((iBufferSize * pConfiguration->iPadAmount) / 100);

        // Finally, take the buffer size and round it up to the nearest block limit
        iBufferSize += ( m_iBlockLimit - (iBufferSize % m_iBlockLimit) );
        
        // Figure out how many buffers we can have
        if( pConfiguration->iMemoryLimit != 0 ) {
            m_iBufferCount = pConfiguration->iMemoryLimit / iBufferSize;
        }
        else {
            // no limit has been applied, so calculate how many buffers we need to keep 2 minute
            // of audio queued up
            int iBytesPerMinute = m_iBytesPerSecond * 60;
            m_iBufferCount = (2 * iBytesPerMinute) / iBufferSize;
            // throw in an extra for good measure
            m_iBufferCount += 1;
        }

        if( m_iBufferCount == 0 ) {
            // We're kinda screwed here - they gave us a configuration we can't handle
            DEBUG( BIS, DBGLEV_ERROR, "calculated buffer count of 0, check your configuration\n");
            return -1;
        }

        // ok, we know how many buffers, and of what size. so start allocating
        m_pBuffers = new input_buffer_t[ m_iBufferCount ];

        if( m_pBuffers == NULL ) return -1;

        for( int i = 0; i < m_iBufferCount; i++ ) {
            m_pBuffers[i].pBuffer = new char[ iBufferSize ];
            if( !m_pBuffers[i].pBuffer ) {
                // We could make a valiant attempt to cleanup here,
                //  but if new really fails, then things are fairly
                //  screwed
                DEBUG( BIS, DBGLEV_ERROR, "new failed, out of dynamic memory\n" );
                return -1;
            }
            
            cyg_semaphore_init( &( m_pBuffers[i].DataAvailable ), 0 );
            m_pBuffers[i].iBufferSize   = iBufferSize;
            m_pBuffers[i].iBufferBase   = 0;
            m_pBuffers[i].iBufferPos    = 0;
            m_pBuffers[i].iBufferLength = 0;
            m_pBuffers[i].bEOF          = 0;
        }

        // Set up some of our internal state
        m_iLowerByteLimit   = 0;
        m_iUpperByteLimit   = 0;
        m_iFillLimit        = iBufferSize;
        m_iTotalBufferSpace = m_iBufferCount * iBufferSize;
        m_iBytesAvailable   = 0;

        // set up our buf ptrs
        m_pCurrentReadBuffer  = &( m_pBuffers[0] );
        m_pCurrentWriteBuffer = &( m_pBuffers[0] );
        
        m_StreamState = EMPTY;
        return 0;
    } else {
        DEBUG( BIS, DBGLEV_ERROR, "configure called at wrong phase\n" );
        return -1;
    }
}

int CBufferedInputStream::PreBuffer( int Seconds ) 
{
    if( m_StreamState == EMPTY ) {
        return FillBufferBytes( Seconds * m_iBytesPerSecond );
    } else {
        return -1;
    }
}


//
// Private
//

int CBufferedInputStream::FillBufferBytes( int iBytes ) 
{
    if( m_StreamState == NOT_CONFIGURED ) return -1;

    if( iBytes < 0 ) return -1;

    int iStartPos = m_pInputStream->Position();

    int iAlignAmount = iStartPos % m_iInputUnitSize;

    // quick hack to get us aligned. we end up burning a buf doing this
    if( iAlignAmount ) {
        int iSpace = m_pCurrentWriteBuffer->iBufferSize - m_pCurrentWriteBuffer->iBufferLength;
        if( iSpace == 0 ) {
            NextWriteBuffer();
            iSpace = m_pCurrentWriteBuffer->iBufferSize - m_pCurrentWriteBuffer->iBufferLength;
            if( iSpace == 0 ) {
                return 0;
            }
        }
        if( iSpace < iAlignAmount ) {
            DEBUG( BIS, DBGLEV_ERROR, "Insufficient buffer space but copying anyways\n");
        }
        m_pBufferThread->Buffer( m_pInputStream,
                                 m_pCurrentWriteBuffer,
                                 iAlignAmount );
        m_iBytesAvailable += iAlignAmount;
        m_iUpperByteLimit += iAlignAmount;

        // burn a buffer here
        NextWriteBuffer();
    }
    
    
    int iBytesLeft = iBytes - iAlignAmount;

    
    while( iBytesLeft ) {
        // See how much space is available in the current buffer
        int iBufferSpace = m_pCurrentWriteBuffer->iBufferSize - m_pCurrentWriteBuffer->iBufferLength;

        if( iBufferSpace == 0 ) {
            // the current write buffer is full
            // this is a good indication that we are out of buffer space
            DEBUG( BIS, DBGLEV_WARNING, "iBytes = %d, but only %d bytes copied\n", iBytes, iBytes-iBytesLeft+iAlignAmount );
            break;
        }

        int iBytesToBuffer = ( iBytesLeft < iBufferSpace ? iBytesLeft : iBufferSpace );

        // Don't buffer more then m_iBlockLimit at a time
        int i;
        for( i = iBytesToBuffer; i > m_iBlockLimit; i -= m_iBlockLimit ) {
            m_pBufferThread->Buffer( m_pInputStream,
                                     m_pCurrentWriteBuffer,
                                     m_iBlockLimit );
        }

        // If i didn't cleanly divide into m_iBlockLimit, then read the residual here
        if( i ) {
            m_pBufferThread->Buffer( m_pInputStream,
                                     m_pCurrentWriteBuffer,
                                     i );
        }
        
        // if we filled that buffer, then step to the next one
        if( iBytesLeft >= iBufferSpace) {
            NextWriteBuffer();
        }

        // subtract the amount of data we buffered up
        iBytesLeft -= iBytesToBuffer;
    }
    
    // Increment our total number of available bytes. This data is not yet available naturally
    m_iBytesAvailable += (iBytes - iBytesLeft);

    // Increment our upper byte limit
    m_iUpperByteLimit += (iBytes - iBytesLeft);

    // And return the number of bytes actually read
    return (iBytes - iBytesLeft + iAlignAmount);
}

// Utility functions to increment buffer ptrs

void CBufferedInputStream::NextReadBuffer() 
{
    m_pCurrentReadBuffer++;
    if( m_pCurrentReadBuffer == &( m_pBuffers[m_iBufferCount] ) ) {
        m_pCurrentReadBuffer = &( m_pBuffers[0] );
    }
}
void CBufferedInputStream::NextWriteBuffer() 
{
    m_pCurrentWriteBuffer++;
    if( m_pCurrentWriteBuffer == &( m_pBuffers[m_iBufferCount] ) ) {
        m_pCurrentWriteBuffer = &( m_pBuffers[0] );
    }
}
