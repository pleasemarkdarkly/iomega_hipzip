//
// PCMCodec.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __PCMCODEC_H__
#define __PCMCODEC_H__

#include <codec/pcm/RawCodec.h>

// TODO: move this to codec/pcm/PCMCodecKeys.h
#define CODEC_PCM_KEY 0x01

//! Dadio(tm) has a precompiled PCM codec capable of playing back 44.1kHz
//! stereo 16bit raw PCM streams such as those found on audio CDs. This
//! codec does not look for any header on incoming bitstreams and assumes
//! data being given to it is in this format.
class CPCMCodec : public CRAWCodec
{
public:
    DEFINE_CODEC( "iObjects PCM Codec", CODEC_PCM_KEY, false, "raw", "pcm", "cda" );
};

//@}

#endif // __PCMCODEC_H__
