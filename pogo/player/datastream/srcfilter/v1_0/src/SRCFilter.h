// SRCFilter.h: sample rate conversion filter class
// 08/30/2001
// (c) Interactive Objects

#ifndef __SRCFILTER_H__
#define __SRCFILTER_H__

#include <datastream/filter/Filter.h>
#include <datastream/srcfilter/SRCFilterKeys.h>

class CSRCFilter : public IFilter
{
public:
   DEFINE_FILTER( "Sample Rate Conversion Filter", SRC_FILTER_ID );
    
   CSRCFilter();
   ~CSRCFilter();

   ERESULT DoWork(bool bFlush);
   ERESULT Configure( filter_stream_info_t& StreamInfo );
   int Ioctl( int Key, void* Data );
   
   int GetInputUnitSize()  const { return m_InputUnitSize; }
   int GetOutputUnitSize() const { return m_OutputUnitSize;  }

   int SetWriteBuf( rbuf_writer_t* pW );
   int SetReadBuf( rbuf_reader_t* pR );

   rbuf_writer_t* GetWriteBuf() const  { return m_pWriteBuf; }
   rbuf_reader_t* GetReadBuf() const   { return m_pReadBuf;  }
    
private:
   rbuf_reader_t* m_pReadBuf;
   rbuf_writer_t* m_pWriteBuf;
   long m_InputSampleRate, m_FixedOutputSampleRate, m_MinOutputSampleRate, m_NumChannels, m_InputUnitSize, m_OutputUnitSize;
   long m_GranuleSize;
   float m_fDataMultiplier;

   typedef int (CSRCFilter::*SRCFunc)( const short* src, short* dest, int samples_per_chan );

   // These convert 8kHz->16kHz, 16kHz->32kHz, 11kHz->22kHz, 22kHz->44kHz, 12kHz->24kHz
   int Stereo1_2(const short*,short*,int);
   int Mono1_2(const short*,short*,int);

   // These convert 8kHz->32kHz, 11kHz->44kHz
   int Stereo1_4(const short*,short*,int);
   int Mono1_4(const short*,short*,int);

   // These convert 8kHz->44kHz
   int Stereo8_44(const short*,short*,int);
   int Mono8_44(const short*,short*,int);
   
   // These convert 12kHz->44kHz
   int Stereo12_44(const short*,short*,int);
   int Mono12_44(const short*,short*,int);
   
   // These convert 16kHz->44kHz
   int Stereo16_44(const short*,short*,int);
   int Mono16_44(const short*,short*,int);

   // These convert 24kHz->44kHz
   int Stereo24_44(const short*,short*,int);
   int Mono24_44(const short*,short*,int);

   // These convert 32kHz->44kHz
   int Stereo32_44(const short*,short*,int);
   int Mono32_44(const short*,short*,int);

   // These convert 48kHz->44kHz
   int Stereo48_44(const short*,short*,int);
   int Mono48_44(const short*,short*,int);
   
   SRCFunc m_pSrcFunc; 
};


#endif // __SRCFILTER_H__
