#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <devs/audio/dai.h>

#if 1
#include "short_audio_left.h"
#include "short_audio_right.h"
#define AUDIO_LENGTH (13 * 4608)
#else
#include "long_audio_left.h"
#include "long_audio_right.h"
#define AUDIO_LENGTH (147 * 4608)
#endif

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)
//#define DEBUG(s...) /**/

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static cyg_handle_t _PlayMsgBoxH;
static cyg_mbox _PlayMsgBox;

/* FUNCTIONS */

static int _GetNextBuffer(void);

static void
_Constructor(void)
{
    DAIInit();
}

int
StartPlayback(void)
{
    DAIResetPlayback();
    DAIEnable();

    _GetNextBuffer();
    
    return 0;
}

int
Initialize(void)
{
    DACSetVolume(0);

    return 0;
}

#if 0
static short * _Left;
static short * _Right;
static short * _LeftBegin;
static short * _LeftEnd;
#else
static short* _Buffer;
static short* _BufferBegin;
static short* _BufferEnd;
#endif

static int
_GetNextBuffer(void)
{
    unsigned int NumSamples;
    unsigned int NumSamplesLeft;
    for (;;) {
        //	NumSamplesLeft = DAIGetNextBuffer(&_Left, &_Right, &NumSamples);
        NumSamplesLeft = DAIGetNextBuffer( &_Buffer, &NumSamples );
	if (NumSamplesLeft <= 0) {
	    break;
	}
	unsigned int Delay = NumSamplesLeft / DAISamplesPerTick();
	cyg_thread_delay(Delay);
    }
    if (NumSamplesLeft == 0) {
	_BufferBegin = &_Buffer[0];
	_BufferEnd = &_Buffer[NumSamples];
	return 0;
    }
    else {
	return NumSamplesLeft;
    }
}

static void
_WriteFullBuffer(void)
{
    //    unsigned int BufferSize = _Left >= _LeftEnd ? _LeftEnd - _LeftBegin : _Left - _LeftBegin;
    unsigned int BufferSize = (_Buffer >= _BufferEnd) ? _BufferEnd - _BufferBegin : _Buffer - _BufferBegin;

    DAIWrite(BufferSize);
    _GetNextBuffer();
}

int
Write(short * Left, short * Right, unsigned long SamplesPerChannel)
    //Write(short * Buffer, unsigned long SamplesPerChannel)
{
    for (unsigned long Sample = 0; Sample < SamplesPerChannel; ++Sample) {
        *_Buffer++ = *Left++;
        *_Buffer++ = *Right++;
	if (_Buffer >= _BufferEnd) {
	    _WriteFullBuffer();
	}
    }
    return 0;
}

int
EndPlayback()
{
    _BufferEnd = _Buffer;
    _WriteFullBuffer();

    _BufferEnd = _Buffer;
    _WriteFullBuffer();
    _BufferEnd = _Buffer;
    _WriteFullBuffer();

    //    DAIDisable();
    return 0;
}

static void
_PlayThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    _Constructor();
    Initialize();

#if defined(DAI_SUPPORT_HARDWARE_SRC)
    unsigned int FrequencyList[] = {48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};
    unsigned int Frequency = 0;

    DEBUG("Testing sampling frequencies\n");
    for (Frequency = 0; Frequency < (sizeof(FrequencyList) / sizeof(FrequencyList[0])); ++Frequency) {
	DEBUG("Setting sampling frequency to %d\n", FrequencyList[Frequency]);
	DAISetSampleFrequency(FrequencyList[Frequency]);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }
    DAISetSampleFrequency(44100);
#endif
    
    DEBUG("Testing main volume\n");
    for (cyg_int8 Volume = DAC_MIN_VOLUME; Volume < DAC_MAX_VOLUME; Volume += 10) {
	DEBUG("Setting volume to %d\n", Volume);
	DACSetVolume(Volume);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }
    DACSetVolume(0);

#if defined(DAC_SUPPORT_HEADPHONE_VOLUME)
    DEBUG("Testing headphone volume\n");
    for (cyg_int8 Volume = DAC_MIN_HEADPHONE_VOLUME; Volume < DAC_MAX_HEADPHONE_VOLUME; Volume += 5) {
	DEBUG("Setting headphone volume to %d\n", Volume);
	DACSetHeadphoneVolume(Volume);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }
    DACSetHeadphoneVolume(0);
#endif /* DAC_SUPPORT_HEADPHONE_VOLUME */

#if defined(DAC_SUPPORT_TONE_CONTROL)
    DEBUG("Testing treble boost\n");
    for (cyg_int8 Volume = DAC_MIN_TREBLE_BOOST; Volume < DAC_MAX_TREBLE_BOOST; Volume += 2) {
	DEBUG("Setting treble boost to %d\n", Volume);
	DACSetTone(Volume, 0);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }

    DEBUG("Testing treble corner frequency\n");
    for (cyg_int8 Corner = 2; Corner >= 0; --Corner) {
	DEBUG("Setting corner to %d\n", Corner);
	DACSetToneCornerFrequency((TrebleBoostCornerFrequency_T)Corner, BBCF_200Hz);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }
    DACSetTone(0, 0);    
    
    DEBUG("Testing bass boost\n");
    for (cyg_int8 Volume = DAC_MIN_BASS_BOOST; Volume < DAC_MAX_BASS_BOOST; Volume += 2) {
	DEBUG("Setting bass boost to %d\n", Volume);
	DACSetTone(0, Volume);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }

    DEBUG("Testing bass corner frequency\n");
    for (cyg_int8 Corner = 2; Corner >= 0; --Corner) {
	DEBUG("Setting corner to %d\n", Corner);
	DACSetToneCornerFrequency(TBCF_7kHz, (BassBoostCornerFrequency_T)Corner);
	StartPlayback();
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	Write((short *)left_channel, (short *)right_channel, left_channel_length);
	EndPlayback();
    }
    DACSetTone(0, 0);  
#endif /* DAC_SUPPORT_TONE_CONTROL */
    
    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" {
void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    cyg_mbox_create(&_PlayMsgBoxH, &_PlayMsgBox);
    
    cyg_thread_create(10, _PlayThread, (cyg_addrword_t) 0, "PlayThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}
};
