//
// RAWCodec.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __RAWCODEC_H__
#define __RAWCODEC_H__

#include <codec/common/Codec.h>

//! This is a partial derivation of the ICodec interface; the idea being that
//! classes which pump raw audio streams should derive from this, and parse
//! whatever custom headers they may have in the SetSong call

class CRAWCodec : public ICodec
{
public:
    CRAWCodec();
    ~CRAWCodec();

    // DecodeFrame just pumps audio; Seek does seeking based off sample rate
    //  and number of channels
    virtual ERESULT DecodeFrame( unsigned long& TimePos );
    virtual ERESULT Seek( unsigned long& secSeek );

    // The default SetSong() and GetMetadata() routines assume you have a 44.1kHz stereo
    // PCM stream coming in; override these if you have headers to parse
    virtual ERESULT SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                             stream_info_t& streamInfo, IMetadata* pMetadata = 0 );
    virtual ERESULT GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata,
                                 IInputStream* pInputStream );

    // The following routines should be safe to leave alone
    rbuf_writer_t* GetWriteBuf() const 
        { return m_pWriteBuf; }
    void SetWriteBuf( rbuf_writer_t* pW ) 
        { m_pWriteBuf = pW;   }
    IInputStream* GetInputStream() const 
        { return m_pInputStream;  }
    
    virtual int GetOutputUnitSize() const;
    virtual void Stats();
    
protected:
    int m_iGranuleSize;
    unsigned int m_uiSampleRate;
    unsigned int m_uiNumChannels;
    float m_fTimePos;
    IInputStream* m_pInputStream;

private:    
    rbuf_writer_t* m_pWriteBuf;
};



#endif // __RAWCODEC_H__
