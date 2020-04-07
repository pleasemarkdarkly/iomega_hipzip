#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <devs/audio/dai.h>

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

static int _GetNextInputBuffer(void);
static int _GetNextOutputBuffer(void);

static short * _InLeft;
static short * _InRight;
static short * _InLeftBegin;
static short * _InLeftEnd;

static short * _OutLeft;
static short * _OutRight;
static short * _OutLeftBegin;
static short * _OutLeftEnd;

static void
_Constructor(void)
{
    DAIInit();
}

int
Start(void)
{
    DACSetVolume(0);
    
    DAIEnable();
    
    return 0;
}

int
Stop(void)
{
    DAIDisable();
    
    return 0;
}

int
StartRecord(void)
{
    DAIResetRecord();
    
    _GetNextInputBuffer();
    
    return 0;
}

int
StopRecord(void)
{
    /* TODO Need to call DAIReset, but that will affect playback */

    return 0;
}

static int
_GetNextInputBuffer(void)
{
    unsigned int NumSamples;
    unsigned int NumSamplesLeft;
    for (;;) {
	NumSamplesLeft = DAIRead(&_InLeft, &NumSamples);
	if (NumSamplesLeft <= 0) {
	    break;
	}
	unsigned int Delay = NumSamplesLeft / DAISamplesPerTick();
	cyg_thread_delay(Delay);
    }
    if (NumSamplesLeft == 0) {
	_InLeftBegin = &_InLeft[0];
	_InLeftEnd = &_InLeft[NumSamples];
	return 0;
    }
    else {
	return NumSamplesLeft;
    }
}

static void
_ReadFullBuffer(void)
{
    DAIReleaseBuffer();
    _GetNextInputBuffer();
}
    
int
Read(short * Left, unsigned long SamplesPerChannel) 
{
    for (unsigned long Sample = 0; Sample < SamplesPerChannel; ++Sample) {
	*Left++ = *_InLeft++;
	if (_InLeft >= _InLeftEnd) {
	    _ReadFullBuffer();
	}
    }
    return 0;
}

int
StartPlayback(void)
{
    DAIResetPlayback();
   
    _GetNextOutputBuffer();

    return 0;
}

static int
_GetNextOutputBuffer(void)
{
    unsigned int NumSamples;
    unsigned int NumSamplesLeft;
    for (;;) {
	NumSamplesLeft = DAIGetNextBuffer(&_OutLeft, &NumSamples);
	if (NumSamplesLeft <= 0) {
	    break;
	}
	unsigned int Delay = NumSamplesLeft / DAISamplesPerTick();
	if (Delay > 10) {
	    DEBUG("%d\n", Delay);
	}
	cyg_thread_delay(Delay);
    }
    if (NumSamplesLeft == 0) {
	_OutLeftBegin = &_OutLeft[0];
	_OutLeftEnd = &_OutLeft[NumSamples];
	return 0;
    }
    else {
	return NumSamplesLeft;
    }
}

static void
_WriteFullBuffer(void)
{
    unsigned int BufferSize = _OutLeft >= _OutLeftEnd ? _OutLeftEnd - _OutLeftBegin : _OutLeft - _OutLeftBegin;

    DAIWrite(BufferSize);
    _GetNextOutputBuffer();
}

int
Write(short * Left, unsigned long SamplesPerChannel)
{
    for (unsigned long Sample = 0; Sample < SamplesPerChannel; ++Sample) {
	*_OutLeft++ = *Left++;
	if (_OutLeft >= _OutLeftEnd) {
	    _WriteFullBuffer();
	}
    }
    return 0;
}

int
StopPlayback()
{
    _OutLeftEnd = _OutLeft;
    _WriteFullBuffer();

    _OutLeftEnd = _OutLeft;
    _WriteFullBuffer();
    _OutLeftEnd = _OutLeft;
    _WriteFullBuffer();
    
    return 0;
}

/*****/

//#define LOOPBACK_BUFFER_SIZE 128*1024
#define LOOPBACK_BUFFER_SIZE 4096
static short RightChannel[LOOPBACK_BUFFER_SIZE];
static short LeftChannel[LOOPBACK_BUFFER_SIZE];

void CheckClip()
{
  cyg_int8 clip;
  DEBUG("Checking clip\n");
  clip = ADCGetClip();
   
  
  if(clip & CLIP_L) {
    DEBUG("Left Clip bit set\n");
  }

  if(clip & CLIP_R) {
    DEBUG("Right Clip bit set\n");
  }

}

static void
Loopback(void)
{
    StartRecord();
    StartPlayback();
    for (int i = 0; i < 32; ++i) {
	Read(LeftChannel, (sizeof(LeftChannel) / sizeof(short)));
	Write(LeftChannel, (sizeof(LeftChannel) / sizeof(short)));
    }
    StopPlayback();
    StopRecord();
    CheckClip();
}

static void
_RecordThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    _Constructor();

    Start();

    
        
#if defined(DAI_SUPPORT_HARDWARE_SRC)
    unsigned int FrequencyList[] = {48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};
    unsigned int Frequency = 0;

    DEBUG("Testing sampling frequencies\n");
    for (Frequency = 0; Frequency < (sizeof(FrequencyList) / sizeof(FrequencyList[0])); ++Frequency) {
	DEBUG("Setting sampling frequency to %d\n", FrequencyList[Frequency]);
	if (DAISetSampleFrequency(FrequencyList[Frequency]) < 0) {
	    continue;
	}
	Loopback();
    }
    DAISetSampleFrequency(44100);
#endif
    


    DEBUG("Testing digital recording volume\n");
    for (cyg_int8 Volume = ADC_MIN_VOLUME; Volume < ADC_MAX_VOLUME; Volume += 10) {
	DEBUG("Setting volume to %d\n", Volume);
	ADCSetVolume(Volume);
	Loopback();
    }
    ADCSetVolume(0);

    DEBUG("Testing digital recording volume - stereo mode\n");

    ADCEnableStereoVolume();
    DEBUG("Testing Left Volume\n");

    for (cyg_int8 Volume = ADC_MIN_VOLUME; Volume < ADC_MAX_VOLUME; Volume += 10) {
	DEBUG("Setting left volume to %d\n", Volume);
	ADCSetStereoVolume(Volume,0);
	Loopback();
    }

    DEBUG("Testing Right Volume\n");
    for (cyg_int8 Volume = ADC_MIN_VOLUME; Volume < ADC_MAX_VOLUME; Volume += 10) {
	DEBUG("Setting right volume to %d\n", Volume);
	ADCSetStereoVolume(0,Volume);
	Loopback();
    }

    ADCSetStereoVolume(0,0);

      DEBUG("Testing left analog gain\n");
    for (cyg_int8 Volume = ADC_MIN_GAIN; Volume <= ADC_MAX_GAIN; Volume += 2) {
    
      DEBUG("Setting left analog gain to %d\n", Volume);
      ADCSetGain(Volume,0);
      Loopback();
   
  
      
    }
    
  DEBUG("Testing right analog gain\n");
    for (cyg_int8 Volume = ADC_MIN_GAIN; Volume <= ADC_MAX_GAIN; Volume += 2) {
      DEBUG("Setting right analog gain to %d\n", Volume);
      ADCSetGain(0,Volume);
      Loopback();

   
   
      
    }
    
    ADCSetGain(0,0);

    DEBUG("Testing mic boost\n");
    Loopback();
  
    DEBUG("Mic boost on\n");
    ADCEnableMicBoost();
    Loopback();
  
    DEBUG("Mic boost off\n");
    ADCDisableMicBoost();
    Loopback();
  

    DEBUG("Mute left channel\n");
    ADCMute(MUTE_LEFT);
    Loopback();
    DEBUG("Disable mute\n");
    ADCMute(MUTE_DISABLE);
    Loopback();

    Stop();

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" {
void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    cyg_mbox_create(&_PlayMsgBoxH, &_PlayMsgBox);
    
    cyg_thread_create(10, _RecordThread, (cyg_addrword_t) 0, "RecordThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}
};
