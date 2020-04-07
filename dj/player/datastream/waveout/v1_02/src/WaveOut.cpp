// WaveOut.cpp: CWaveOutStream implementation
// danc@iobjects.com 07/09/01
// (c) Interactive Objects

#include <datastream/waveout/WaveOut.h>
#include <datastream/waveout/WaveOutKeys.h>
#include <devs/audio/dai.h>

#include <string.h> // memcpy

#include <util/debug/debug.h>

DEBUG_MODULE( WO );
DEBUG_USE_MODULE( WO );

REGISTER_OUTPUTSTREAM( CWaveOutStream, WAVEOUT_KEY );

unsigned int CWaveOutStream::refcount = 0;
short* CWaveOutStream::m_pBufferStart, *CWaveOutStream::m_pBufferEnd,* CWaveOutStream::m_pBuffer;


static bool initted = false;

CWaveOutStream::CWaveOutStream() 
{
	if (!initted) {
		DAIInit();
		DAIResetPlayback();
		initted = true;
	}
    m_uiSamplesPerTick = DAISamplesPerTick();
	open = false;
}

CWaveOutStream::~CWaveOutStream()
{
    Flush();
    Close();
}

ERESULT CWaveOutStream::Open( const char* Source ) 
{
	if (!open) {
		DEBUG( WO, DBGLEV_INFO, "Open: %d", refcount );
		if (refcount++ == 0) {
			DAIEnable();
		
			static bool firstTime = true;

			if (firstTime)
			{
				firstTime = false;
				// hack, should be PLAYBACK_BUFFER_SIZE from dai_hw.h in devs/audio/include
				DAIWrite(12*588);
			}


			DEBUGP( WO, DBGLEV_INFO, " Physical open");
		}
		GetNextBuffer(false);
		DEBUGP( WO, DBGLEV_INFO, " %d\n", refcount);\
		open = true;
	}
    return OUTPUTSTREAM_NO_ERROR;
}

ERESULT CWaveOutStream::Close() 
{
	if (open) {
		DEBUG( WO, DBGLEV_INFO, "Close: %d", refcount );
		if (--refcount == 0) {
			DEBUG( WO, DBGLEV_INFO, "Physical close");
			DAIWaitForEmpty();
			DAIResetPlayback();
		}
		// DAIDisable();
		DEBUGP( WO, DBGLEV_INFO, " %d\n", refcount);
		open = false;
	}
    return OUTPUTSTREAM_NO_ERROR;
}

int CWaveOutStream::Write( const void* Buffer, int Count ) 
{
    short* pBuf = (short*) Buffer;

#ifdef SLOW_COPY
    Count >>= 1;  // convert Count to samples

    while( Count-- ) {
        *m_pBuffer++ = *pBuf++;

        if( m_pBuffer == m_pBufferEnd ) {
            GetNextBuffer();
        }
    }
#else
    while( Count > 0 ) {
        int Space = (m_pBufferEnd - m_pBuffer) << 1;
        int Copy = (Space > Count ? Count : Space);

        memcpy( m_pBuffer, pBuf, Copy );
        Count -= Copy;

        // convert to short*
        Copy >>= 1;
        m_pBuffer += Copy;
        pBuf += Copy;

        if( m_pBuffer == m_pBufferEnd ) {
            GetNextBuffer();
        }
    }
#endif
    return 1;
}

int CWaveOutStream::Ioctl( int Key, void* Value ) 
{
    switch( Key )
    {
        case KEY_WAVEOUT_SET_SAMPLERATE:
        {
            // unsupported sample rate
            if( DAISetSampleFrequency( *(unsigned int*)Value ) == -1 ) {
                return OUTPUTSTREAM_ERROR;
            }
            m_uiSamplesPerTick = DAISamplesPerTick();
            return 1;
        }
        default:
        {
            return 0;
        }
    };
}

bool CWaveOutStream::Flush()
{
    if( (m_pBuffer - m_pBufferStart) > 0 ) {
        GetNextBuffer( true );
    }

    return true;	
}

int CWaveOutStream::GetOutputUnitSize() 
{
    return DAIGetBufferSize();
}

//
// Private routines
//
void CWaveOutStream::GetNextBuffer( bool WriteCurrent ) 
{
    unsigned int NumSamples;
    unsigned int SamplesRemaining;
    
    if( WriteCurrent ) {
        NumSamples = m_pBuffer - m_pBufferStart;
        DAIWrite( NumSamples );
    }

    do {
        SamplesRemaining = DAIGetNextBuffer( &m_pBufferStart, &NumSamples );

        // note that small buffer sizes make this painful
        // additionally, there is no assurance that this will succeed, especially
        // if playback hasn't been enabled
        // -dc 11/28/01  set a threshhold; 50 prevents it from blocking if
        //               approximately 500us of audio data is still in the buffer
        if( SamplesRemaining > 50 ) {
            unsigned int delay = SamplesRemaining / m_uiSamplesPerTick;
            cyg_thread_delay( (delay > 0 ? delay : 1) );
        }
    } while( SamplesRemaining > 0 );
    
    // we received a buffer
    m_pBuffer = &m_pBufferStart[0];
    m_pBufferEnd = &m_pBufferStart[ NumSamples ];
}

