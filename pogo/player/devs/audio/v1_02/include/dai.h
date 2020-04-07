// dai.h: header for DAI routines
// danc@iobjects.com 07/05/01
// (c) Interactive Objects

#ifndef __DAI_H__
#define __DAI_H__

#if defined(ENABLE_DAC_CS4340)
#include "cs4340.h"
#elif defined(ENABLE_DAC_CS4341)
#include "cs4341.h"
#elif defined(ENABLE_DAC_CS4342)
#include "cs4342.h"
#elif defined(ENABLE_DAC_CS4343)
#include "cs4343.h"
#endif

#if defined(ENABLE_ADC_CS5332)
#include "cs5332.h"
#endif

#if defined(ENABLE_SPDIF_8405A)
#include "cs8405a.h"
#endif

#if defined(__EDB7312)
#define DAI_SUPPORT_HARDWARE_SRC
#endif /* __EDB73XX */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#if 0
}  // brace align
#endif
  
extern unsigned int DAIStarvationCounter;



void DAIInit(void);

void DAIResetPlayback(void);

void DAIEnable(void);
void DAIDisable(void);

int DAISetSampleFrequency(unsigned int Frequency);
unsigned int DAISamplesPerTick(void);

short DAIGetPeak();

void DAIWaitForEmpty(void);
unsigned int DAIGetBufferSize(void);
unsigned int DAIGetNextBuffer(short ** Buffer, unsigned int * NumSamples);
void DAIWrite(unsigned int NumSamples);

void DAISetNormalFIQ();

#if defined(ENABLE_DAI_RECORD)

void DAISetLoopbackFIQ();
extern unsigned int DAIOverflowCounter;

void DAIResetRecord(void);

unsigned int DAIRead(short ** Buffer, unsigned int * NumSamples);
void DAIReleaseBuffer(void);
unsigned int DAIFreeRecordBufferCount(void);

#endif /* ENABLE_DAI_RECORD */
    
#ifdef __cplusplus
};
#endif /* __cplusplus */
    
#endif /* __DAI_H__ */
