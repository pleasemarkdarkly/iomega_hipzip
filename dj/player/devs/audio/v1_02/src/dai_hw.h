// dai_hw.h

#ifndef __DAI_HW_H__
#define __DAI_HW_H__

/* 1 MP3 frame at 22050Hz gives ~52mS of audio data.
 * 1 MP3 frame at 44100Hz gives ~26mS of audio data. */
//
#define MP3_FRAME_SIZE (1152 /* samples */ * 2 /* channels */)
#define CDA_FRAME_SIZE ( 588 /* samples */ * 2 /* channels */)

//#define NUM_PLAYBACK_BUFFERS 24	 /* 2 works for MP3. */
				 /* 8 works Ok, but not perfect for WMA. */
//#define PLAYBACK_BUFFER_SIZE (2 * MP3_FRAME_SIZE)

#define NUM_PLAYBACK_BUFFERS   8
#define PLAYBACK_BUFFER_SIZE (12 * CDA_FRAME_SIZE)

#if defined(ENABLE_DAI_RECORD)
#define NUM_RECORD_BUFFERS ( 16 )
#define RECORD_BUFFER_SIZE ( MP3_FRAME_SIZE )
#endif

#ifndef ASM_HEADERS
typedef struct DAIBuffer_S 
{
    short* Data;
    unsigned int Size;
    unsigned int Position;
    struct DAIBuffer_S * Next;
} DAIBuffer_T;
#endif /* ASM_HEADERS */

#define DAI_BUFFER_DATA          0x00
#define DAI_BUFFER_SIZE          0x04
#define DAI_BUFFER_POSITION      0x08
#define DAI_BUFFER_NEXT          0x0c

#endif /* __DAI_HW__ */
