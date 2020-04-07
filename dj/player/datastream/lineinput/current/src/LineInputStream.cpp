// LineInputStream.cpp: Line input stream
// danc@iobjects.com 08/29/01
// (c) Interactive Objects

#include <datastream/lineinput/LineInputStream.h>
#include <datasource/lineindatasource/LineInDataSource.h>

#include <devs/audio/dai.h>

#include <util/debug/debug.h>

#include <string.h>  // memcpy

DEBUG_MODULE_S(LINEINPUTSTREAM, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(LINEINPUTSTREAM);

CLineInputStream::CLineInputStream()
{
    DAIInit();
    DAIResetRecord();
}

CLineInputStream::~CLineInputStream()
{
    Close();
}

ERESULT
CLineInputStream::Open( const char* Source )
{
    // never fails
    DAISetSampleFrequency( 44100 );
    DAIEnable();
    m_pCurrentBuffer = m_pBufferEnd = NULL;
    m_iPosition = 0;
    GetNextBuffer( false );
    return INPUTSTREAM_NO_ERROR;
}

ERESULT
CLineInputStream::Close()
{
    DAIDisable();
    DAIResetRecord();
    return INPUTSTREAM_NO_ERROR;
}

int
CLineInputStream::Read( void* Buffer, int Count )
{
    int iBytesRead = 0;

    while( Count > 0 ) {
        int iBytesAvailable = (m_pBufferEnd - m_pCurrentBuffer) << 1;
        
        if( iBytesAvailable == 0 ) {
            GetNextBuffer();
            continue;
        }

        int iBytesToCopy = (Count < iBytesAvailable ? Count : iBytesAvailable);

        memcpy( (unsigned char*)Buffer + iBytesRead, m_pCurrentBuffer, iBytesToCopy );

        m_pCurrentBuffer += (iBytesToCopy >> 1);

        iBytesRead += iBytesToCopy;
        Count -= iBytesToCopy;
    }

    m_iPosition += iBytesRead;
    return iBytesRead;
}

int
CLineInputStream::Ioctl( int Key, void* Value )
{
    return 0;
}

int
CLineInputStream::GetInputUnitSize() 
{
    return (int) DAIGetBufferSize();
}


void
CLineInputStream::GetNextBuffer( bool bRelease ) 
{
    if( bRelease ) {
        DAIReleaseBuffer();
    }
    unsigned int num_samples = 0;
    unsigned int samples_left, samples_per_tick;

    samples_per_tick = DAISamplesPerTick();
    while( (samples_left = DAIRead( &m_pCurrentBuffer, &num_samples )) ) {
        if( samples_left > samples_per_tick ) {
            // block for input
            cyg_thread_delay( samples_left / samples_per_tick );
        }
    }

    m_pBufferEnd = m_pCurrentBuffer + num_samples;
}
