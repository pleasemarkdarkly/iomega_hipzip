// codec_vects.h: definitions for codec base locations in rom and
//                offsets for various codec vector entries
// danc@iobjects.com 1/21/01
// (c) Interactive Objects

#ifndef __CODEC_VECTS_H__
#define __CODEC_VECTS_H__


// this is the base address of where the vector table is located. it should
// correspond to where the codec image starts in flash
#ifdef __POGO
#warning RAM codecs enabled!
#define CODEC_VECTOR_BASE  ((void*)0xed0000)
#else
#define CODEC_VECTOR_BASE  ((void*)0xe0180000)
#endif

// the following values shouldn't need any adjustment

#define MP3_VECTOR_BASE   ( CODEC_VECTOR_BASE + 0x00 )
#define AAC_VECTOR_BASE   ( CODEC_VECTOR_BASE + 0x20 )
#define WMA_VECTOR_BASE   ( CODEC_VECTOR_BASE + 0x40 )

// acelp is no longer directly accessed

// this corresponds to the location where codecs can set up their callback vector table
// eventually i will figure out a reasonable way to reserve this memory
#define CALLBACK_VECTOR_BASE   ((void*)0x60000000)

// the following values shouldn't need any adjustment
#define WMA_CALLBACK_BASE   ( CALLBACK_VECTOR_BASE + 0x00 )

// this should stay the same
#define ACELP_RW_DATA_DEST   ((void*)0x60000040)

// these need to get tweaked for every build of the libs. fucking acelp.
#define ACELP_RW_DATA_BASE   ((void*)0xe01b6328)
#define ACELP_RW_DATA_SIZE   (0x300)


// This is the total amount of SRAM we are using
#define CODEC_SRAM_USAGE     (ACELP_RW_DATA_SIZE + (ACELP_RW_DATA_DEST - CALLBACK_VECTOR_BASE) )

#endif // __CODEC_VECTS_H__
