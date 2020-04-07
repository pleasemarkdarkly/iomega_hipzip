// codec_workspace.h: definitions for codec workspace size requirements
//                and sram usage locations.
// ericg@iobjects.com 11/16/01
// (c) Interactive Objects

#ifndef __CODEC_WORKSPACE_H__
#define __CODEC_WORKSPACE_H__

// location where codecs should be placed for high priority decoding.
#define FAST_DECODE_ADDR (0x60000000 + (2048))

// maximum size of codec instances
#define CODEC_WORKSPACE_SIZE (0xBE00 - (2048))

#endif // __CODEC_WORKSPACE_H__
