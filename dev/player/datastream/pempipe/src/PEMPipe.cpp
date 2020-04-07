// PEMPipe.cpp: hacky IOutputStream derived class to prebuffer
//              data and feed it to PEM
// danc@iobjects.com

#include <datastream/fatfile/FileOutputStream.h> // TODO fix

#include <codec/mp3/pem/pem.h>
#include "PEMPipe.h"

#include <util/rbuf/rbuf.h>
#include <util/debug/debug.h>

#include <stdio.h>  // memset

#define PEM_NORMAL_FRAME 0
#define PEM_EOS_FRAME    1

DEBUG_MODULE(PP);
DEBUG_USE_MODULE(PP);

REGISTER_FILTER( CPEMPipe, PEMPIPE_KEY );

CPEMPipe* CPEMPipe::s_pCurrentInstance = 0;

CPEMPipe::CPEMPipe() : IThreadedObject(PEM_THREAD_PRIORITY, "PEM Encoder", PEM_THREAD_STACK_SIZE),
                       m_pReadBuf(0),
                       m_pPemWriter(0),
                       m_pPemReader(0)
{
    if( s_pCurrentInstance ) {
        DEBUG( PP, DBGLEV_ERROR, "Multiple PEMPipe instances\n");
    }
    s_pCurrentInstance = this;

}

CPEMPipe::~CPEMPipe()
{
    this->Close();
    s_pCurrentInstance = 0;
}

ERESULT
CPEMPipe::DoWork(bool bFlush) 
{
    ERESULT res;
    int desired = rb_read_avail( m_pReadBuf );
    bool bFlush = false;
    
    if( desired < BYTES_PER_FRAME ) {
        if( rb_read_eof( m_pReadBuf ) == RBUF_EOF ) {
            bFlush = true;
        } else {
            return FILTER_NO_WORK;
        }
    }

    // TODO investigate performance gains by leaving small amounts of data in the rbuf
    //      rather than performing larger numbers of small copies
    int BytesToCopy = desired;
    while( BytesToCopy > 0 ) {
        short* pOutBuf = &( m_pWriteBuffer->m_pSamples[ m_pWriteBuffer->m_iSampleIndex ] );
        int Space = (INPUT_BUFFER_SIZE - m_pWriteBuffer->m_iSampleIndex) << 1; // subtracting shorts, convert to bytes
        unsigned int Copied;

        if( Space >= BytesToCopy ) {
            
            
            rb_read_data( m_pReadBuf, BytesToCopy, &Copied );
            //            rb_copy_read( m_pReadBuf, (void*)pOutBuf, BytesToCopy );
            //            Copied = BytesToCopy;
        } else {
            rb_read_data( m_pReadBuf, Space, &Copied );
            //            rb_copy_read( m_pReadBuf, (void*)pOutBuf, Space );
            //            Copied = Space;
        }
        rb_read_done( m_pReadBuf, Copied );

        m_pWriteBuffer->m_iSampleIndex += (Copied >> 1);  // adding shorts
        m_pWriteBuffer->m_iSampleCount = m_pWriteBuffer->m_iSampleIndex;
        BytesToCopy -= Copied;

        // Flip buffers if full
        if( m_pWriteBuffer->m_iSampleCount == INPUT_BUFFER_SIZE ) {
            m_pWriteBuffer->m_iSampleIndex = 0;
            m_FullBufferQueue.Put( m_pWriteBuffer );
            m_EmptyBufferQueue.Get( &m_pWriteBuffer );
        }
    }
    
    return FILTER_NO_ERROR;
}

ERESULT
CPEMPipe::Ioctl( int Key, void* Value )
{
    return FILTER_FAIL;
}

ERESULT
CPEMPipe::Configure( filter_stream_info_t& StreamInfo ) 
{
    // Create rbufs and output stream
    rbuf_t* rb = rb_new( OUTPUT_BUFFER_SIZE );
    m_pPemWriter = rb_new_writer( rb );
    m_pPemReader = rb_new_reader( rb );

    if( !rb || !m_pPemWriter || !m_pPemReader ) {
        // out of memory
        return FILTER_FAIL;
    }

    m_pOutputStream = CFatFileOutputStream::Create();
    if( FAILED( m_pOutputStream->Open( StreamInfo.m_szStreamName ) ) ) {
        // failed to open output stream
        return FILTER_FAIL;
    }

    // Initialize buffer queues
    memset( (void*) m_pBuffers, 0, sizeof( sample_buffer_t ) * BUFFER_COUNT );
    for( int i = 0; i < BUFFER_COUNT; i++ ) {
        m_EmptyBufferQueue.Put( &m_pBuffers[i] );
    }
    m_EmptyBufferQueue.Get( &m_pWriteBuffer );
    
    // Initialize pem
    // Grab a handle to the pem wrapper
    m_pPemWrapper = CPEMWrapper::GetInstance();
    
    // TODO dont assume 128kbps
    m_pPemWrapper->Start( 128, 0, m_pPemWriter );

    this->Start();
    
    return FILTER_READONLY;
}

int
CPEMPipe::GetInputUnitSize() const 
{
    // 576 samples per channel
    return 2304;
}

int
CPEMPipe::GetOutputUnitSize() const
{
    // since we're a read only filter this should be ignored
    return 2304;
}

int
CPEMPipe::SetWriteBuf( rbuf_writer_t* WriteBuf )
{
    // TODO fix error result?
    return FILTER_NO_ERROR;
}

int
CPEMPipe::SetReadBuf( rbuf_reader_t* ReadBuf )
{
    if( !ReadBuf ) {
        return FILTER_FAIL;
    }
    m_pReadBuf = ReadBuf;

    return FILTER_NO_ERROR;
}

rbuf_writer_t*
CPEMPipe::GetWriteBuf() const
{
    return 0;
}

rbuf_reader_t*
CPEMPipe::GetReadBuf() const
{
    return m_pReadBuf;
}

void
CPEMPipe::ThreadBody() 
{
    // Main pump
    // wait for buffer on queue
    //   if the buffer is empty
    //     then exit
    //   encode the buffer
    //   call DoOutput(false)
    m_bStopped = false;
    
    bool bEOF = false;

    DBEN( PP );
    while( !bEOF ) {
        sample_buffer_t* pBuf;

        // wait for a buffer
        m_FullBufferQueue.Get( &pBuf );

        // check for partial or empty frame
        if( pBuf->m_iSampleCount < INPUT_BUFFER_SIZE ) {
            bEOF = true;
        }

        while( pBuf->m_iSampleIndex < pBuf->m_iSampleCount ) {
            ERESULT err;
            short* pData = &( pBuf->m_pSamples[ pBuf->m_iSampleIndex ] );

            int iStereoSamples;
            if( (pBuf->m_iSampleCount - pBuf->m_iSampleIndex) < SAMPLES_PER_FRAME ) {
                iStereoSamples = (pBuf->m_iSampleCount - pBuf->m_iSampleIndex) >> 1;
            } else {
                iStereoSamples = SAMPLES_PER_FRAME >> 1;
            }

            // Encode the frame
            err = m_pPemWrapper->EncodeBuffer( pData, &iStereoSamples, bEOF );

            if( FAILED(err) ) {
                DEBUG(PP, DBGLEV_ERROR, "PEM error code 0x%08x\n", err );
            }

            // Then check to see if we need to write to disk
            this->DoOutput( bEOF );
            
            pBuf->m_iSampleIndex += SAMPLES_PER_FRAME;
        }
        pBuf->m_iSampleIndex = 0;
        pBuf->m_iSampleCount = 0;
        m_EmptyBufferQueue.Put( pBuf );
    }
    DBEX( PP );

    // This lets the IThreadedObject interface know the thread is done
    m_bStopped = true;
}

void
CPEMPipe::Close()
{
    // We indicate an eof to the encoder thread by dropping a partial buffer on the queue
    // This will cause it to signal an eof to pem
    m_pWriteBuffer->m_iSampleIndex = 0;
    m_FullBufferQueue.Put( m_pWriteBuffer );

    // Wait for the encoding thread to wrap up
    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }

    // Release resources here
    // Free up the output stream
    m_pOutputStream->Close();
    delete m_pOutputStream;

    // And the rbuf
    rbuf_t* rb = rb_write_rbuf( m_pPemWriter );
    rb_free_reader( m_pPemReader );
    rb_free_writer( m_pPemWriter );
    rb_free( rb );
}

ERESULT
CPEMPipe::DoOutput(bool flush)
{
	unsigned int desired = rb_read_avail( m_pPemReader );
	ERESULT err;
    ERESULT res = FILTER_NO_WORK;

    // TODO this may be wrong
    if( rb_read_eof( m_pPemReader ) ) {
        flush = true;
    }
    
	if (flush)
        desired = (desired >= WRITE_CHUNK_SIZE) ? WRITE_CHUNK_SIZE : desired;
    else
		desired = (desired >= WRITE_CHUNK_SIZE) ? WRITE_CHUNK_SIZE : 0;
	
	if (desired) {
		unsigned int actual;
		err = rb_read_data( m_pPemReader, desired, &actual );
		switch(err) {
            case RBUF_SPLIT:
            {
                m_pOutputStream->Write( rb_read_ptr( m_pPemReader ), actual );
                rb_read_done( m_pPemReader, actual );
                desired -= actual;
                rb_read_data( m_pPemReader, desired, &actual );
                // fall through
            }
            case RBUF_SUCCESS:
            {
                m_pOutputStream->Write( rb_read_ptr( m_pPemReader ), actual );
                rb_read_done( m_pPemReader, actual );

                res = FILTER_NO_ERROR;
                break;
            }
            default:
            {
                DEBUG( PP, DBGLEV_ERROR, "rbuf failure");

                res = FILTER_FAIL;
                break;
            }
		}
	}

    // god awful hack
    if( flush ) {
        desired = rb_read_avail( m_pPemReader );
        if( desired ) {
            this->DoOutput(true);
        }
    }
	return res;
}

#if 0
ERESULT
CPEMPipe::Open( const char* Source ) 
{
    // put all our buffers into the empty queue
    memset( (void*)m_pBuffers, 0, sizeof( sample_buffer_t ) * BUFFER_COUNT );
    
    for( int i = 0; i < BUFFER_COUNT; i++ ) {
        m_EmptyBufferQueue.Put( &m_pBuffers[i] );
    }
    m_EmptyBufferQueue.Get( &m_pWriteBuffer );
    
    // TODO fix, dont assume 128kbps
    pem_init( 128 );
    
    // open output stream for *Source
    // TODO fix, dont be reliant on CFatFileOutputStream
    m_pOutputStream = new CFatFileOutputStream();

    if( m_pOutputStream->Open( Source ) != OUTPUTSTREAM_NO_ERROR ) {
        DEBUG( PP, DBGLEV_ERROR, "Could not open output stream %s\n", Source );
        return OUTPUTSTREAM_ERROR;
    }
    
    // Start our thread
    this->Start();
    
    return OUTPUTSTREAM_NO_ERROR;
}

ERESULT
CPEMPipe::Close()
{
    m_bStopped = true;
    this->Flush();

    // wait for the worker thread to finish
    while( !m_bWorkerDone ) {
        cyg_thread_delay(1);
    }
    
    m_pOutputStream->Close();

    pem_fini();

    // fini pem
    // close output stream
}

int
CPEMPipe::Write( const void* Buffer, int Count ) 
{
    int BytesToCopy = Count;
    // Count is in bytes, convert to shorts
    if( Count & 0x01 ) {
        DEBUG( PP, DBGLEV_ERROR, "Uneven byte count to CPEMPipe::Write() (%d)\n", Count );
        // deal ?
        Count &= ~(0x01);
    }

    while( BytesToCopy > 0 ) {
        // Space is in bytes
        short* pBuf = &( m_pWriteBuffer->m_pSamples[ m_pWriteBuffer->m_iSampleIndex ] );
        int Space = (INPUT_BUFFER_SIZE - m_pWriteBuffer->m_iSampleIndex) << 1;
        int Copied;

        // Copy as much as possible
        if( Space >= BytesToCopy ) {
            memcpy( pBuf, Buffer, BytesToCopy );
            Copied = BytesToCopy;
        }
        else {
            memcpy( pBuf, Buffer, Space );
            Copied = Space;
        }
    
        // Update our index
        m_pWriteBuffer->m_iSampleIndex += (Copied >> 1);
        BytesToCopy -= Copied;
    
        // Write the buffer if it is full
        if( m_pWriteBuffer->m_iSampleIndex == INPUT_BUFFER_SIZE ) {
            m_pWriteBuffer->m_iSampleCount = 0;
            m_FullBufferQueue.Put( m_pWriteBuffer );
            m_EmptyBufferQueue.Get( &m_pWriteBuffer );
        }
    }
    return (Count - BytesToCopy);
}

int
CPEMPipe::Ioctl( int Key, void* Value ) 
{
    // TODO: set bitrate?
    return -1;
}

bool
CPEMPipe::Flush()
{
    // dump out whatever data we have and trip an end-of-stream
    ERESULT err;

    if( m_pWriteBuffer->m_iSampleIndex > 0 ) {
        m_pWriteBuffer->m_iSampleCount = 0;
        m_FullBufferQueue.Put( m_pWriteBuffer );
    }
    
    return true;
}

int
CPEMPipe::GetOutputUnitSize() 
{
    // this is an approximation of the maximum output
    // unit from pem
    return 256;
}

void
CPEMPipe::ThreadBody(void) 
{
    m_bWorkerDone = false;
    
    DBEN(PP);
    
    int iFrameType = PEM_NORMAL_FRAME;

    while( !m_bStopped ) {
        sample_buffer_t* pBuffer;
        
        // block until a buffer is available
        m_FullBufferQueue.Get( &pBuffer );

        // if stop has been signaled, check for a partial frame
        if( m_bStopped ) {
            if( pBuffer->m_iSampleIndex > 0 ) {
                iFrameType = PEM_EOS_FRAME;
            }
        }

        while( pBuffer->m_iSampleCount < pBuffer->m_iSampleIndex ) {
            // process the buffer
            ERESULT err;
            int iStereoSamples = PEM_SAMPLE_COUNT >> 1;

            err = pem_work( &( pBuffer->m_pSamples[ pBuffer->m_iSampleCount ] ), &iStereoSamples, iFrameType );
            
            if( FAILED(err) ) {
                DEBUG( PP, DBGLEV_ERROR, "PEM error code 0x%08x\n", err );
                // TODO: break?
            }

            pBuffer->m_iSampleCount += PEM_SAMPLE_COUNT;
        }
        
        pBuffer->m_iSampleIndex = 0;
        m_EmptyBufferQueue.Put( pBuffer );
    }
    DBEX(PP);

    m_bWorkerDone = true;
}


void
CPEMPipe::PEMHook( unsigned char* x, int n ) 
{
    int BytesToCopy = n;

    while( BytesToCopy > 0 ) {
        int Space = OUTPUT_BUFFER_SIZE - m_iOutputIndex;
        int Copied;
        
        if( Space >= BytesToCopy ) {
            memcpy( &( m_pOutputBuffer[m_iOutputIndex] ), x, BytesToCopy );
            Copied = BytesToCopy;
        }
        else {
            memcpy( &( m_pOutputBuffer[m_iOutputIndex] ), x, Space );
            Copied = Space;
        }

        m_iOutputIndex += Copied;
        BytesToCopy -= Copied;

        if( m_iOutputIndex == OUTPUT_BUFFER_SIZE ) {
            m_pOutputStream->Write( m_pOutputBuffer, OUTPUT_BUFFER_SIZE );
            m_iOutputIndex = 0;
        }
    }
}

// fixed-name function called by pem
ERESULT
pem_do_out( unsigned char* x, int n ) 
{
    if( n ) {
        (CPEMPipe::s_pCurrentInstance)->PEMHook( x, n );
    }
    return PEM_NO_ERROR;
}
#endif
