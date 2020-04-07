//
// WAVCodec.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef __WAVCODEC_H__
#define __WAVCODEC_H__

#include <codec/pcm/RAWCodec.h>
#include <codec/wav/WAVCodecKeys.h>

class CWAVCodec : CRAWCodec
{
public:

    ERESULT SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                     stream_info_t& streamInfo, IMetadata* pMetadata = 0 );
    ERESULT GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata,
                         IInputStream* pInputStream );
    
    DEFINE_CODEC( "iObjects WAV Codec", CODEC_WAV_KEY, true, "wav" );
};



#endif // __WAVCODEC_H__
