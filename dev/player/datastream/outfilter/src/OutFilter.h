// OutFilter.h: filter step that drives an output stream
// danc@iobjects.com 07/10/01
// (c) Interactive Objects

#ifndef __OUTFILTER_H__
#define __OUTFILTER_H__

#include <datastream/filter/Filter.h>
#include <datastream/outfilter/OutFilterKeys.h>

// fdecl
class IOutputStream;


class COutFilter : public IFilter
{
public:

    DEFINE_FILTER( "iObjects OutputFilter", OUTFILTER_KEY );
    
    COutFilter();
    ~COutFilter();

    ERESULT DoWork(bool bFlush);
    ERESULT Configure( filter_stream_info_t& StreamInfo );
    ERESULT Ioctl( int Key, void* Value );

    int GetInputUnitSize() const;
    int GetOutputUnitSize() const;

    int SetWriteBuf( rbuf_writer_t* WriteBuf );
    int SetReadBuf( rbuf_reader_t* ReadBuf );

    rbuf_writer_t* GetWriteBuf() const { return m_pWriteBuf; }
    rbuf_reader_t* GetReadBuf()  const { return m_pReadBuf;  }

private:
    rbuf_reader_t* m_pReadBuf;
    rbuf_writer_t* m_pWriteBuf;
    IOutputStream* m_pOutput;
    int m_iUnitSize;
};





#endif // __OUTFILTER_H__
