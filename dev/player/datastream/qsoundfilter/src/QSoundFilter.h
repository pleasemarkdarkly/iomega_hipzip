// QSoundFilter.h: qsound filter class
// danc@iobjects.com 07/26/01
// (c) Interactive Objects

#ifndef __QSOUNDFILTER_H__
#define __QSOUNDFILTER_H__

#include <datastream/filter/Filter.h>
#include <datastream/qsoundfilter/QSoundFilterKeys.h>

#include "q2x.h"

class CQSoundFilter : public IFilter
{
public:
    DEFINE_FILTER( "QSound Labs q2x filter", QSOUND_FILTER_ID );
    
    CQSoundFilter();
    ~CQSoundFilter();

    ERESULT DoWork();
    int Ioctl( int Key, void* Data );

    int GetInputUnitSize()  const { return 4; }
    int GetOutputUnitSize() const { return 4; }

    int SetWriteBuf( rbuf_writer_t* pW );
    int SetReadBuf( rbuf_reader_t* pR );

    rbuf_writer_t* GetWriteBuf() const  { return m_pWriteBuf; }
    rbuf_reader_t* GetReadBuf() const   { return m_pReadBuf;  }
    
private:
    rbuf_reader_t* m_pReadBuf;
    rbuf_writer_t* m_pWriteBuf;

    Q2XState m_State;
};


#endif // __QSOUNDFILTER_H__
