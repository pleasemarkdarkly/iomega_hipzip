// SRCFilter.cpp: sample rate conversion filter implementation
// 08/30/2001
// (c) Interactive Objects

#include <util/rbuf/rbuf.h>   // rbufs
#include <string.h>           // memset

#include "SRCFilter.h"

REGISTER_FILTER( CSRCFilter, SRC_FILTER_ID );

CSRCFilter::CSRCFilter() 
{
    m_pReadBuf = NULL;
    m_pWriteBuf = NULL;
    m_InputSampleRate = 0;
    m_FixedOutputSampleRate = 44100;
    m_MinOutputSampleRate   = 44100;
    m_pSrcFunc = NULL;
}

CSRCFilter::~CSRCFilter() 
{

}

ERESULT CSRCFilter::DoWork(bool bFlush) 
{
    // Variables to keep track of how much data we actually
    // read and write
    unsigned int actualr, actualw;

    // Pointers to the input and output buffers.
    short* inptr, *outptr;

    // Fail if we have not assigned a function to do
    // the work.
    if( !m_pSrcFunc ) return FILTER_FAIL;

    // Fail if either buffer is invalid.
    if( !m_pWriteBuf || !m_pReadBuf ) return FILTER_FAIL;

    // Find out how much data in the read buffer is
    // available to be read.
    int amtr = rb_read_avail( m_pReadBuf );
    amtr -= ( amtr % m_GranuleSize );
    
    // Find out how much space is available in the write
    // buffer to hold our output data.
    int Space = rb_write_avail( m_pWriteBuf );

    if( Space == 0 ) {
        return FILTER_NO_WORK;
    }
    
    // If there is less space in the write buffer than data
    // we are going to produce, then lower the amount of
    // data that we are going to process.
    int amtw = (int) (amtr * m_fDataMultiplier) + m_GranuleSize;
    amtw -= (amtw % m_GranuleSize );
    if( Space < amtw ) {
        amtr = (int) (Space / m_fDataMultiplier);
        amtr -= ( amtr % m_GranuleSize );
        amtw = (int) (amtr * m_fDataMultiplier);
        amtw += m_GranuleSize;
        amtw -= (amtw % m_GranuleSize); // round up
        //        amtw = (amtw + m_GranuleSize) & ~( m_GranuleSize ); // round up
    }
    
    // If there is nothing to be processed in the read buffer,
    // or no space to put processed data in the write buffer,
    // then we can't do any work.
    if( (amtr < (2 * m_NumChannels)) ) {
        if( rb_read_eof( m_pReadBuf ) == RBUF_EOF ) {
            rb_write_eof( m_pWriteBuf );
            return FILTER_EOF;
        }
        else {
            return FILTER_NO_WORK;
        }
    }
    
    if( (amtw < (2 * m_NumChannels)) ) {
        return FILTER_NO_WORK;
    }
    
    while( amtr > 0 ) {

        // Find out how much we can actually read and write.
        rb_read_data( m_pReadBuf, amtr, &actualr );
        rb_write_data( m_pWriteBuf, amtw, &actualw );

        if( actualr == 0 ) {
            if( rb_read_eof( m_pReadBuf ) == RBUF_EOF ) {
                rb_write_eof( m_pWriteBuf );
                return FILTER_EOF;
            }
            else {
                return FILTER_NO_WORK;
            }
        }
        if( actualw == 0 ) {
            return FILTER_NO_WORK;
        }
        
        // Get pointers to the first location in the
        // ring buffers where we can start reading and
        // writing.
        inptr = (short*)rb_read_ptr( m_pReadBuf );
        outptr = (short*)rb_write_ptr( m_pWriteBuf );

        // try to scale edge conditions
        if( (actualr * m_fDataMultiplier) > actualw ) {
            actualr = (int) ((actualw / m_fDataMultiplier) + 0.1);
            actualr -= ( actualr % m_GranuleSize );
        }

        // bytes to samples per channels
        actualw = (this->*m_pSrcFunc)( inptr, outptr, actualr >> (1 + (m_NumChannels==2?1:0)) );
        
        // samples to bytes
        actualw <<= 1;
        
        rb_read_done( m_pReadBuf, actualr );
        rb_write_done( m_pWriteBuf, actualw );

        amtr -= actualr;
        amtw -= actualw;

    }
    
    return FILTER_NO_ERROR;
}

ERESULT CSRCFilter::Configure( filter_stream_info_t& StreamInfo ) 
{
    if( !StreamInfo.m_bIsPcm ) return FILTER_FAIL;
    if( !m_FixedOutputSampleRate && !m_MinOutputSampleRate ) return FILTER_FAIL;
    
    m_InputSampleRate = StreamInfo.un.pcm.SamplingFrequency;
    m_NumChannels     = StreamInfo.un.pcm.Channels;

    long OutputSampleRate;
    
    switch( m_InputSampleRate )
    {
        case 8000:
        {
            m_InputUnitSize = 8000;
            m_GranuleSize   = 2 * m_NumChannels * 2 /* bytes per sample */;
            
            if( m_FixedOutputSampleRate == 16000 || m_MinOutputSampleRate <= 16000 ) {
                OutputSampleRate = 16000;
                m_OutputUnitSize = 16000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_2 : &CSRCFilter::Mono1_2 );
            }
            else if( m_FixedOutputSampleRate == 32000 || m_MinOutputSampleRate <= 32000 ) {
                OutputSampleRate = 32000;
                m_OutputUnitSize = 32000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_4 : &CSRCFilter::Mono1_4 );
            }
            else if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo8_44 : &CSRCFilter::Mono8_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 11025:
        {
            m_InputUnitSize = 44100;
            m_GranuleSize   = 1 * m_NumChannels * 2 /* bytes per sample */;
            
            if( m_FixedOutputSampleRate == 22050 || m_MinOutputSampleRate <= 22050 ) {
                OutputSampleRate = 22050;
                m_OutputUnitSize = m_InputUnitSize * 2;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_2 : &CSRCFilter::Mono1_2 );
            }
            else if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = m_InputUnitSize * 4;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_4 : &CSRCFilter::Mono1_4 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 12000:
        {
            m_InputUnitSize  = 12000;
            m_GranuleSize   = 3 * m_NumChannels * 2 /* bytes per sample */;

            if( m_FixedOutputSampleRate == 24000 || m_MinOutputSampleRate <= 24000 ) {
                OutputSampleRate = 24000;
                m_OutputUnitSize = 24000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_2 : &CSRCFilter::Mono1_2 );
            }
            else if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44100;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo12_44 : &CSRCFilter::Mono12_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 16000:
        {
            m_InputUnitSize = 16000;
            m_GranuleSize   = 4 * m_NumChannels * 2 /* bytes per sample */;
            
            if( m_FixedOutputSampleRate == 32000 || m_MinOutputSampleRate <= 32000 ) {
                OutputSampleRate = 32000;
                m_OutputUnitSize = 32000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_2 : &CSRCFilter::Mono1_2 );
            }
            else if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo16_44 : &CSRCFilter::Mono16_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 22050:
        {
            m_InputUnitSize = 22050;
            m_GranuleSize   = 1 * m_NumChannels * 2 /* bytes per sample */;

            if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44100;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo1_2 : &CSRCFilter::Mono1_2 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 24000:
        {
            m_InputUnitSize = 24000;
            m_GranuleSize   = 6 * m_NumChannels * 2 /* bytes per sample */;

            if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo24_44 : &CSRCFilter::Mono24_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 32000:
        {
            m_InputUnitSize = 32000;
            m_GranuleSize   = 8 * m_NumChannels * 2 /* bytes per sample */;
            
            if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo32_44 : &CSRCFilter::Mono32_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        case 44100:
        {
            // we could do something more friendly here, like act as a pass through, but it seems like it
            // would make more sense to not have the filter running in the first place
            return FILTER_FAIL;
        }
        case 48000:
        {
            // no downsample
            m_InputUnitSize = 48000;
            m_GranuleSize = 12 * m_NumChannels * 2 /* bytes per sample */;

            if( m_FixedOutputSampleRate == 44100 || m_MinOutputSampleRate <= 44100 ) {
                OutputSampleRate = 44100;
                m_OutputUnitSize = 44000;
                m_pSrcFunc = (m_NumChannels == 2 ? &CSRCFilter::Stereo48_44 : &CSRCFilter::Mono48_44 );
            }
            else {
                return FILTER_FAIL;
            }
            break;
        }
        default:
        {
            return FILTER_FAIL;
        }
    };

    // The data multiplier gives us an idea of how much output we get from
    // a given amount of input. so round it up for fractions
    m_fDataMultiplier = ((float)OutputSampleRate) / m_InputSampleRate;

    // let the next filter know what sample rate we are using
    StreamInfo.un.pcm.SamplingFrequency = OutputSampleRate;
    
    return FILTER_NO_ERROR;
}

// Data should be a pointer to 3 longs:
//  InputSampleRate
//  OutputSampleRate
//  NumChannels
int CSRCFilter::Ioctl( int Key, void* Data ) 
{
    switch( Key ){
        case SRC_FILTER_IOCTL_SET_FIXED_OUTPUT:
        {
            m_FixedOutputSampleRate = (unsigned long)Data;
            m_MinOutputSampleRate   = m_FixedOutputSampleRate;
            return FILTER_NO_ERROR;
        }
        case SRC_FILTER_IOCTL_SET_MIN_OUTPUT:
        {
            m_FixedOutputSampleRate = 0;
            m_MinOutputSampleRate   = (unsigned long)Data;
            return FILTER_NO_ERROR;
        }
        default:
        {
            return FILTER_FAIL;
        }
    };
}

int CSRCFilter::SetWriteBuf( rbuf_writer_t* pW ) 
{
    m_pWriteBuf = pW;
    return FILTER_NO_ERROR;
}

int CSRCFilter::SetReadBuf( rbuf_reader_t* pR ) 
{
    m_pReadBuf = pR;
    return FILTER_NO_ERROR;
}

int CSRCFilter::Stereo1_2( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        *dst++ = left;
        *dst++ = right;
    }
    // return samples
    return (dst-start);
}

int CSRCFilter::Mono1_2 ( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = *src;
        *dst++ = *src++;
    }
    // return samples
    return (dst-start);
}

int CSRCFilter::Stereo1_4( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;
    }
    return (dst-start);
}

int CSRCFilter::Mono1_4( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = *src;
        *dst++ = *src;
        *dst++ = *src;
        *dst++ = *src++;
    }
    return (dst-start);
}

int CSRCFilter::Stereo8_44( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;

        // we alternate between creating 5 and 6 samples
        if( i & 0x01 ) {
            *dst++ = left;
            *dst++ = right;
        }
    }
    return (dst-start);
}
int CSRCFilter::Mono8_44( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = *src;
        *dst++ = *src;
        *dst++ = *src;
        *dst++ = *src;
        // copy 6 samples every other time
        if( i & 0x01 ) {
            *dst++ = *src;
        }
        *dst++ = *src++;
    }
    return (dst-start);
}
int CSRCFilter::Stereo12_44( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        *dst++ = left;
        *dst++ = right;
        *dst++ = left;
        *dst++ = right;
        // 2 out of every three times we copy a 4th sample
        n++;
        if( n < 3 ) {
            *dst++ = left;
            *dst++ = right;
        }
        else {
            n = 0;
        }
    }
    return (dst-start);
}
int CSRCFilter::Mono12_44( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = *src;
        *dst++ = *src;
        n++;
        if( n < 3 ) {
            *dst++ = *src;
        } else {
            n = 0;
        }
        *dst++ = *src++;
    }
    return (dst-start);
}

int CSRCFilter::Stereo16_44( const short* src, short* dst, int samp_per_chan )
{
    short left, right,*start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        *dst++ = left;
        *dst++ = right;
        n++;
        // 3 out of 4 times we copy a 3rd sample
        if( n < 4 ) {
            *dst++ = left;
            *dst++ = right;
        } else {
            n = 0;
        }
    }
    return (dst-start);
}
int CSRCFilter::Mono16_44( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = *src;
        n++;
        if( n < 4 ) {
            *dst++ = *src;
        }
        else {
            n = 0;
        }
        *dst++ = *src++;
    }
    return (dst-start);
}
#include <cyg/infra/diag.h>
int CSRCFilter::Stereo24_44( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right,*start = dst;
    int n = 0;
    if( (samp_per_chan % 6) != 0 ) {
        diag_printf(" bogus samp_per_chan = %d\n", samp_per_chan );
    }
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        n++;
        // 5 out of 6 times we copy a second sample
        if( n < 6 ) {
            *dst++ = left;
            *dst++ = right;
        }
        else {
            n = 0;
        }
    }
    return (dst-start);
}
int CSRCFilter::Mono24_44( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        n++;
        if( n < 6 ) {
            *dst++ = *src;
        }
        else {
            n = 0;
        }
        *dst++ = *src++;
    }
    return (dst-start);
}

int CSRCFilter::Stereo32_44( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        *dst++ = left  = *src++;
        *dst++ = right = *src++;
        // duplicate 3 of every 8 samples
        n++;
        if( (n == 2) || (n == 4) || (n == 7) ) {
            *dst++ = left;
            *dst++ = right;
        }
        else if( n == 8 ) {
            n = 0;
        }
    }
    return (dst-start);
}
int CSRCFilter::Mono32_44( const short* src, short* dst, int samp_per_chan ) 
{
    short* start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        n++;
        if( n == 2 || n == 4 || n == 7 ) {
            *dst++ = *src;
        }
        else if( n == 8 ) {
            n = 0;
        }
        *dst++ = *src++;
    }
    return (dst-start);
}

int CSRCFilter::Stereo48_44( const short* src, short* dst, int samp_per_chan ) 
{
    short left, right, *start = dst;
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        left  = *src++;
        right = *src++;
        // drop 1 of every 12 samples
        n++;
        if( n < 12 ) {
            *dst++ = left;
            *dst++ = right;
        }
        else {
            n = 0;
        }
    }
    return (dst-start);
}

int CSRCFilter::Mono48_44( const short* src, short* dst, int samp_per_chan ) 
{
    short samp, *start = dst;
    
    int n = 0;
    for( int i = samp_per_chan; i ; i-- ) {
        samp = *src++;
        n++;
        
        if( n < 12 ) {
            *dst++ = samp;
        }
        else {
            n = 0;
        }
    }
    return (dst-start);
}
