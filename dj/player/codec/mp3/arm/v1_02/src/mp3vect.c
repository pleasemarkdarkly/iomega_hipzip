#include <codec/common/codec_vects.h>
#include "mpgdata.h"
#include "mpgaudio.h"


#define BASE   MP3_VECTOR_BASE

void MP3Setup( void ) 
{
}

void (*InitMP3Audio)( tMPEGInstance *inst ) = (void*) BASE + 0x00;
tMPEGStatus (*MP3SearchForSyncword)( tMPEGInstance *inst, tMPEGBitstream *bs, int length) = (void*) BASE + 0x04;
tMPEGStatus (*MP3DecodeInfo)( tMPEGInstance *inst, tMPEGBitstream *bs, tMPEGHeader *pmpeg_hdr ) = (void*) BASE + 0x08;
tMPEGStatus (*MP3DecodeData)( tMPEGInstance *inst, short *left, short *right, tMPEGBitstream *bs ) = (void*) BASE + 0x0C;


