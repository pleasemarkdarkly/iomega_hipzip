// QSoundFilter.cpp: qsound filter implmementation
// danc@iobjects.com 07/26/01
// (c) Interactive Objects

#include <util/rbuf/rbuf.h>   // rbufs
#include <string.h>           // memset
#include "QSoundFilter.h"

REGISTER_FILTER( CQSoundFilter, QSOUND_FILTER_ID );

CQSoundFilter::CQSoundFilter() 
{
    memset( (void*)&m_State, 0, sizeof( Q2XState ) );

    q2xInitialise( &m_State );
    q2xSetVolume( &m_State, 0x3fff, 0x3fff );

    q2xSetMode( &m_State, 0 );
    q2xSetSampleRate( &m_State, 1 );
    q2xReset( &m_State );
}

CQSoundFilter::~CQSoundFilter() 
{
    // empty
}

#define _min(x,y) (x<y?x:y)

ERESULT CQSoundFilter::DoWork() 
{
    if( !m_pWriteBuf || !m_pReadBuf ) return FILTER_FAIL;
    
    int amt = rb_read_avail( m_pReadBuf );
    int Space = rb_write_avail( m_pWriteBuf );

    if( Space == 0 || amt == 0 ) {
        return FILTER_NO_WORK;
    }
    
    if( Space < amt ) {
        amt = Space;
    }

    unsigned int actualr, actualw, actual;
    short* inptr, *outptr;

    while( amt > 0 ) {
        rb_read_data( m_pReadBuf, amt, &actualr );
        rb_write_data( m_pWriteBuf, amt, &actualw );
    
        inptr = (short*)rb_read_ptr( m_pReadBuf );
        outptr = (short*)rb_write_ptr( m_pWriteBuf );
    
        actual = _min(actualr, actualw);
        
        // do the actual work here
        for( int i = 0, lim=(actual>>2); i < lim; i++ ) {
            long left = (long)*inptr++;
            long right = (long)*inptr++;

            q2xProcess( &m_State, &left, &right );

            *outptr++ = (short) left;
            *outptr++ = (short) right;
        }
        
        rb_read_done( m_pReadBuf, actual );
        rb_write_done( m_pWriteBuf, actual );

        amt -= actual;
    }
    
    return FILTER_NO_ERROR;
    
}

int CQSoundFilter::Ioctl( int Key, void* Data ) 
{
    switch( Key ) {
        case QSOUND_FILTER_IOCTL_SET_VOLUME:
        {
            long* volume = (long*) Data;

            q2xSetVolume( &m_State, *volume, *(volume+1) );
            return FILTER_NO_ERROR;
        }
        case QSOUND_FILTER_IOCTL_SET_SPREAD:
        {
            long spread = *(long*)Data;

            if( spread < 0 ) spread = 0;
            if( spread > 0x7fff ) spread = 0x7fff;

            q2xSetSpread( &m_State, spread );
            
            return FILTER_NO_ERROR;
        }
        case QSOUND_FILTER_IOCTL_SET_MODE:
        {
            long mode = *(long*)Data;
            
            if( mode < 0 ) mode = 0;
            if( mode > 1 ) mode = 1;

            q2xSetMode( &m_State, mode );
            
            return FILTER_NO_ERROR;
        }
        default:
        {
            return FILTER_FAIL;
        }
    };
}

int CQSoundFilter::SetWriteBuf( rbuf_writer_t* pW ) 
{
    m_pWriteBuf = pW;
    return FILTER_NO_ERROR;
}

int CQSoundFilter::SetReadBuf( rbuf_reader_t* pR ) 
{
    m_pReadBuf = pR;
    return FILTER_NO_ERROR;
}
