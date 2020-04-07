/*
 * mpgaudio.h:
 * Copyright (C) ARM Limited 1999. All Rights Reserved.
 */

#ifndef	_MPGAUDIO_
#define	_MPGAUDIO_

#ifdef __cplusplus
extern "C" {
#endif

#define	MP3_MAX_PCM_LENGTH	1152
#define MP3_MAX_BITS_REQUIRED   11520
#define MP3_NINFOBITS		32

/*
 * Define the sample rate type.
 */

typedef enum tagSampleRate
{
	SR_11_025kHz,
	SR_12kHz,
	SR_8kHz,
	SR_ReservedMPEG2_5,
	SR_22_05kHz,
	SR_24kHz,
	SR_16kHz,
	SR_ReservedLSF,
	SR_44_1kHz,
	SR_48kHz,
	SR_32kHz,
	SR_Reserved
} tSampleRate ;

typedef	struct	tagMPEGBitstream   tMPEGBitstream ;
typedef	tMPEGBitstream            *ptMPEGBitstream ;

struct tagMPEGBitstream
{
    unsigned int   *bufptr;
	unsigned int    bitidx;
} ;



typedef	struct	tagMPEGHeader   tMPEGHeader ;
typedef	tMPEGHeader            *ptMPEGHeader ;

struct tagMPEGHeader
{
	tSampleRate		sample_rate ;
	unsigned int    samplesperchannel;
	unsigned int    numchans;
	unsigned int	packed_info;
	unsigned int	bits_required;
	unsigned int	free_format;
} ;

typedef enum tagMPEGStatus
{
	eNoErr,
	eNoSyncword,
	eCRCError,
	eBrokenFrame,
	eEndOfBitstream,
	eDataOverflow,
	eCantAllocateBuffer,
	eUnsupportedLayer,
    eFrameDiscarded,
	eReservedSamplingFrequency,
	eForbiddenBitRate,
    eWrongMPEGBuild
} tMPEGStatus ;

//extern void MP3Setup(void);

/*
 *       InitMP3Audio
 *       ============
 *
 * Description
 * -----------
 * Initializes the MPEG audio decoder.
 * Call this function before using any other decoder functions.
 *
 * Syntax
 * ------
 *   tMPEGStatus InitMP3Audio( tMPEGInstance *inst )
 *
 * where:
 *
 *   inst        pointer to instance data for the decoder.
 */

extern void InitMP3Audio( tMPEGInstance *inst );



/*
 *       MP3SearchForSyncword
 *       ====================
 *
 * Description
 * -----------
 * This function searches for the syncword that marks the beginning of an
 * MPEG audio frame.
 *
 * Syntax
 * ------
 *  tMPEGStatus MP3SearchForSyncword( tMPEGInstance *inst, tMPEGBitstream *bs, int length);
 *
 * where:
 *
 *   inst        pointer to instance data for the decoder.
 *   bs       pointer to the structure that points to the incoming bitstream.
 *   length   number of new valid bits
 *
 * Return Value
 * ------------
 *
 *   tMPEGStatus  function's return status:
 *
 *   eNoErr                      Frame decoded ok
 *   eNoSyncword                 No syncronization word
 */

extern tMPEGStatus MP3SearchForSyncword( tMPEGInstance *inst, tMPEGBitstream *bs, int length);


/*
 *       MP3DecodeInfo
 *       =============
 *
 * Description
 * -----------
 * This function decodes a frame of samples from the MP3 stream.
 *
 * Syntax
 * ------
 *   tMPEGStatus MP3DecodeInfo( tMPEGInstance *inst, tMPEGBitstream *bs, tMPEGHeader *pmpeg_hdr ) ;
 *
 * where: 
 *
 *   inst        pointer to instance data for the decoder.
 *   bs          pointer to the structure that points to the incoming bitstream.
 *   pmpeg_hdr   pointer to a structure to hold MP3 header information.
 *
 * Return Value
 * ------------
 *
 *   tMPEGStatus  function's return status:
 *
 *   eNoErr                      Frame decoded ok
 *   eNoSyncword                 No syncronization word
 *   eCRCError                   CRC error failed
 *   eBrokenFrame                Header or side information is inconsistent 
 *   eEndOfBitstream             (temporary for demo API)
 *   eUnsupportedLayer           Unsupported or illegal ('00') MPEG audio layer
 *   eFrameDiscarded             Insufficient main data to decode the frame
 *   eReservedSamplingFrequency  Undefined sampling frequency ('11')
 *   eForbiddenBitRate           Illegal bit rate ('1111')
 *
 */ 

extern tMPEGStatus MP3DecodeInfo( tMPEGInstance *inst, tMPEGBitstream *bs, tMPEGHeader *pmpeg_hdr ) ;




/*
 *       MP3DecodeData
 *       =============
 *
 * Description
 * -----------
 * This function decodes a frame of samples from the MP3 stream.
 *
 * Syntax
 * ------
 *   tMPEGStatus MP3DecodeData( tMPEGInstance *inst, short *left, short *right, tMPEGBitstream *bs ) ;
 *
 * where: 
 *
 *   inst     pointer to instance data for the decoder.
 *   left     pointer to output buffer for left-channel PCM.
 *   right    pointer to output buffer for right-channel PCM.
 *   bs       pointer to the structure that points to the incoming bitstream.
 *
 * Return Value
 * ------------
 *
 *   tMPEGStatus  function's return status:
 *
 *   eNoErr                      Frame decoded ok
 *   eCRCError                   CRC error failed
 *   eBrokenFrame                Side information is inconsistent 
 *   eFrameDiscarded             Insufficient main data to decode the frame
 *
 */ 

extern tMPEGStatus MP3DecodeData( tMPEGInstance *inst, short *left, short *right, tMPEGBitstream *bs ) ;

#ifdef __cplusplus
};
#endif
#endif	/* _MPGAUDIO_ */
