// OutFilter.cpp: filter step that drives an output stream
// danc@iobjects.com 07/10/01
// (c) Interactive Objects

#include <datastream/output/OutputStream.h>

#include "OutFilter.h"

#include <util/rbuf/rbuf.h>

#ifndef NULL
#define NULL 0
#endif

REGISTER_FILTER( COutFilter, OUTFILTER_KEY );

COutFilter::COutFilter() 
{
    m_pOutput = NULL;
    m_pReadBuf = NULL;
    m_pWriteBuf = NULL;
    m_iUnitSize = -1;
}

COutFilter::~COutFilter()
{
    if( m_pOutput ) {
        m_pOutput->Close();
        delete m_pOutput;
    }
}
#if 1
// dc- this is the bulk processing implementation, which will attempt to write as large a chunk
//     as it possibly can
ERESULT COutFilter::DoWork(bool bFlush)
{
    ERESULT res = FILTER_NO_ERROR;
    unsigned int avail = rb_read_avail( m_pReadBuf );

    // If there's less than a unit's worth of data available then bail,
    // unless we're flushing.
    if( (avail == 0) ||
        ((signed)avail < m_iUnitSize && !bFlush) )
    {
        if( rb_read_eof( m_pReadBuf ) == RBUF_EOF ) {
            return FILTER_EOF;
        }
        return FILTER_NO_WORK;
    }

    if (!bFlush)
        // round the available bytes to a UnitSize based number
        avail -= (avail % m_iUnitSize);
   
    unsigned int actual;
    ERESULT r = rb_read_data( m_pReadBuf, avail, &actual );

    m_pOutput->Write( rb_read_ptr( m_pReadBuf ), actual );

    rb_read_done( m_pReadBuf, actual );
    
    if( r == RBUF_SPLIT ) {
        unsigned int amt = avail - actual;
        
        rb_read_data( m_pReadBuf, amt, &actual );
        m_pOutput->Write( rb_read_ptr( m_pReadBuf ), actual );

        rb_read_done( m_pReadBuf, actual );
    }
    else if( r == RBUF_EOF ) {
        return FILTER_EOF;
    }
    return res;
}

#else
// dc- this is the original implementation. it is more true to the intent of the system, in that
//     it performs on granule of work, but it is significantly slower
ERESULT COutFilter::DoWork(bool bFlush) 
{
    ERESULT res = FILTER_NO_ERROR;
    unsigned int avail = rb_read_avail( m_pReadBuf );
    if( (signed)avail < m_iUnitSize ) {
        if( rb_read_eof( m_pReadBuf ) == RBUF_EOF ) {
            return FILTER_EOF;
        }
        return FILTER_NO_WORK;
    }

    unsigned int actual;
    ERESULT r = rb_read_data( m_pReadBuf, m_iUnitSize, &actual );

    m_pOutput->Write( rb_read_ptr( m_pReadBuf ), actual );

    rb_read_done( m_pReadBuf, actual );
    
    if( r == RBUF_SPLIT ) {
        unsigned int amt = m_iUnitSize - actual;
        
        rb_read_data( m_pReadBuf, amt, &actual );
        m_pOutput->Write( rb_read_ptr( m_pReadBuf ), actual );

        rb_read_done( m_pReadBuf, actual );
    }
    else if( r == RBUF_EOF ) {
        return FILTER_EOF;
    }
    return res;
}
#endif

ERESULT COutFilter::Configure( filter_stream_info_t& StreamInfo ) 
{
    if( m_pOutput == NULL ) return FILTER_FAIL;

    // TODO check if the string is empty first? nah
    m_pOutput->Open( StreamInfo.m_szStreamName );
    
    return FILTER_READONLY;
}

ERESULT COutFilter::Ioctl( int Key, void* Value ) 
{
    switch( Key )
    {
        case KEY_OUTFILTER_SET_OUTPUTSTREAM:
        {
            m_pOutput = (IOutputStream*) Value;
            if( m_pOutput ) {
                // cache this
                m_iUnitSize = m_pOutput->GetOutputUnitSize();
            }
            
            return FILTER_NO_ERROR;
        }
        default: 
        {
            return FILTER_FAIL;
        }
    };
}

int COutFilter::GetInputUnitSize() const
{
    return m_iUnitSize;
}

int COutFilter::GetOutputUnitSize() const
{
    return m_iUnitSize;
}

int COutFilter::SetWriteBuf( rbuf_writer_t* WriteBuf ) 
{
    m_pWriteBuf = WriteBuf;
    return 0;
}

int COutFilter::SetReadBuf( rbuf_reader_t* ReadBuf ) 
{
    m_pReadBuf = ReadBuf;
    return 0;
}

