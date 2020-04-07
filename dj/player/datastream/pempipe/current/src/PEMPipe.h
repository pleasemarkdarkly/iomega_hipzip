// PEMPipe.h: IFilter derived class to prebuffer
//            data and feed it to PEM
// danc@iobjects.com

#ifndef __PEMPIPE_H__
#define __PEMPIPE_H__

#include <datastream/pempipe/PEMPipeKeys.h>
#include <datastream/filter/Filter.h>

#include <util/thread/ThreadedObject.h>
#include <util/datastructures/MessageQueue.h>

// Macros to control the buffering behavior
#define BUFFER_COUNT           2
#define SAMPLES_PER_FRAME   2304
#define BYTES_PER_FRAME     4604
#define FRAMES_PER_BUFFER     49
#define INPUT_BUFFER_SIZE  (SAMPLES_PER_FRAME * FRAMES_PER_BUFFER)
#define OUTPUT_BUFFER_SIZE (64*1024)

// Number of bytes to write to disk at a time
#define WRITE_CHUNK_SIZE   (16*1024)

// Properties for our thread
#define PEM_THREAD_STACK_SIZE 8192
#define PEM_THREAD_PRIORITY     10

class CPEMPipe : IFilter , IThreadedObject
{
public:
    DEFINE_FILTER( "iObjects PEMFilter", PEMPIPE_KEY );

    CPEMPipe();
    ~CPEMPipe();

    ERESULT DoWork(bool bFlush);
    ERESULT Ioctl( int Key, void* Value );
    ERESULT Configure( filter_stream_info_t& StreamInfo );
    
    int GetInputUnitSize() const;
    int GetOutputUnitSize() const;
    
    int SetWriteBuf( rbuf_writer_t* WriteBuf );
    int SetReadBuf( rbuf_reader_t* ReadBuf );

    rbuf_writer_t* GetWriteBuf() const;
    rbuf_reader_t* GetReadBuf() const;
    
    // IThreadedObject hooks
    void ThreadBody();
    
private:
    void Close();
    
    // Copy data from the rbuf to the output stream
    ERESULT DoOutput( bool flush = false );

    // Pointer to the singleton instance
    static CPEMPipe* s_pCurrentInstance;

    // Wrapper object for pem codec
    class CPEMWrapper* m_pPemWrapper;

    // Our MP read handle
    rbuf_reader_t* m_pReadBuf;

    // Internal writer and reader for pem
    rbuf_writer_t* m_pPemWriter;
    rbuf_reader_t* m_pPemReader;

    // Pointer to the output stream
    IOutputStream* m_pOutputStream;

    // Buffer queueing stuff
    typedef struct sample_buffer_s
    {
        int m_iSampleCount;  // total samples in buffer
        int m_iSampleIndex;  // current index in buffer
        short m_pSamples[ INPUT_BUFFER_SIZE ];
    } sample_buffer_t;

    sample_buffer_t m_pBuffers[ BUFFER_COUNT ];
    sample_buffer_t* m_pWriteBuffer;

    MessageQueue<sample_buffer_t*> m_EmptyBufferQueue;
    MessageQueue<sample_buffer_t*> m_FullBufferQueue;
};

#endif // __PEMPIPE_H__

