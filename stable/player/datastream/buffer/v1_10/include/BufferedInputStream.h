// BufferedInputStream.h: plain buffering for an input stream
// danc@iobjects.com 08/03/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>    // cyg_sem_t
#include <datastream/input/InputStream.h>

#define BUFFERED_INPUT_ID 0x81

// fdecl
class IContentRecord;
class CBufferThread;
typedef struct buffer_config_s buffer_config_t;

typedef struct input_buffer_s
{
    // "const" values
    int iBufferSize;           // size of the buffer
    char* pBuffer;             // pointer to our actual data

    // status values
    int iBufferBase;           // offset in the stream this buffer corresponds to
    int iBufferPos;            // current (read) position in the buffer
    int iBufferLength;         // length of the current valid data
    cyg_sem_t DataAvailable;   // posted when the buffering thread adds data
    unsigned int bEOF:1;       // end of file flag
        
    
} input_buffer_t;

class CBufferedInputStream : public IInputStream 
{
public:
    DEFINE_INPUTSTREAM( "iObjects buffered input", BUFFERED_INPUT_ID );

    CBufferedInputStream();
    ~CBufferedInputStream();

    // This type of stream cannot be directly opened
    int Open( const char* Source ) 
        { return -1; }
    int Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );
    int GetInputUnitSize()
        { return 4; }

    // we should be careful about promising this if the underlying
    // stream isn't seekable
    bool CanSeek() const { return true; }
    int Seek( InputSeekPos SeekOrigin, int Offset );

    int Length() const;
    int Position() const;

    // Custom routines

    // We can prebuffer streams, which basically means we have x
    // initial bytes queued up, but we're still at 0 in the virtual
    // stream. additionally, we can be unconfigured, or buffered
    enum StreamState
    {
        NOT_CONFIGURED=0,
        EMPTY,
        PREBUFFERED,
        BUFFERED,
    };

    void SetSong( IInputStream* pInputStream, IContentRecord* pContentRecord );
    int Configure( const buffer_config_t* pConfiguration );
    int PreBuffer( int Seconds );
    
    StreamState GetState() const 
        { return m_StreamState; }
    
    bool IsTrack( IContentRecord* pCR ) 
        {
            return pCR == m_pContentRecord;
        }
    
private:
    int FillBufferBytes( int iBytes );
    void NextReadBuffer();
    void NextWriteBuffer();
    
    IInputStream* m_pInputStream;
    IContentRecord* m_pContentRecord;
    
    CBufferThread* m_pBufferThread;
    
    int m_iBytesPerSecond;         // how many bytes for a second of audio?
    int m_iBlockLimit;             // how many bytes can we read in a single op?
    int m_iInputUnitSize;          // cache the size of an input unit

    // Our buffer states
    input_buffer_t* m_pBuffers;
    input_buffer_t* m_pCurrentReadBuffer;
    input_buffer_t* m_pCurrentWriteBuffer;
    int m_iBufferCount;

    // High and low byte offsets within the file
    int m_iLowerByteLimit;
    int m_iUpperByteLimit;

    // How much data left before we fill?
    int m_iFillLimit;

    // How much total buffer space do we have?
    int m_iTotalBufferSpace;
    // How many bytes are currently available?
    int m_iBytesAvailable;
    
    // State of the object
    StreamState m_StreamState;
};
