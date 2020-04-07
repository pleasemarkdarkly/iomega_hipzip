//
// VorbisCodec.h
//
// Copyright (c) 2002 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __VORBISCODEC_H__
#define __VORBISCODEC_H__

#include <codec/common/Codec.h>

#define CODEC_VORBIS_KEY 0x0a

class CVorbisCodec : public ICodec
{
public:
    CVorbisCodec();
    ~CVorbisCodec();
    
    ERESULT DecodeFrame( unsigned long& TimePos );
    ERESULT SetSong( IDataSource* pDS, IInputStream* pInputStream, stream_info_t& streamInfo, IMetadata* pMetadata = 0 );
    ERESULT Seek( unsigned long& secSeek );

    ERESULT GetMetadata( IDataSource* pDS, IMetadata* pMetadata, IInputStream* pInputStream );
    
    rbuf_writer_t* GetWriteBuf() const;
    void SetWriteBuf( rbuf_writer_t* pW );
    int GetOutputUnitSize() const;

    IInputStream* GetInputStream() const;
    
    void Stats();

    DEFINE_CODEC( "iObjects Ogg/Vorbis I Codec", CODEC_VORBIS_KEY, true,"ogg" );
private:
    OggVorbis_File m_VorbisFile;
    IInputStream*  m_pInputStream;
    rbuf_writer_t* m_pWriteBuf;
    int            m_eof;
};

//@}

#endif // __VORBISCODEC_H__
