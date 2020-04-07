#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <devs/audio/dai.h>
#include "dai_hw.h"

#include <util/debug/debug.h>
   
DEBUG_MODULE(AUDIO);
DEBUG_USE_MODULE(AUDIO);

#define CODEC_ENABLE 0x01

DAIBuffer_T * DAIPlaybackBuffer;
unsigned int DAIStarvationCounter;
unsigned int DAIPeak;
#if defined(ENABLE_DAI_RECORD)
DAIBuffer_T * DAIRecordBuffer;
unsigned int DAIOverflowCounter;
#endif

static short PlaybackBuffer[NUM_PLAYBACK_BUFFERS][PLAYBACK_BUFFER_SIZE];
#if defined(ENABLE_DAI_RECORD)
static short RecordBuffer[NUM_RECORD_BUFFERS][RECORD_BUFFER_SIZE];
#endif

static DAIBuffer_T _PlaybackBuffer[NUM_PLAYBACK_BUFFERS];
static DAIBuffer_T * _WriteBuffer;

#if defined(ENABLE_DAI_RECORD)
static DAIBuffer_T _RecordBuffer[NUM_RECORD_BUFFERS];
static DAIBuffer_T * _ReadBuffer;
#endif
 
static bool _VSRMasked = true;
unsigned int _DAISampleFrequency = 44100; /* DAC needs to see this value */

extern void DAIFIQ(void);
extern void DAIFIQMONITOR(void);

#define HAL_INTERRUPT_NUM  CYGNUM_HAL_INTERRUPT_I2SINT

static cyg_handle_t _I2SInterruptHandle;
static cyg_interrupt _I2SInterrupt;

int
_I2SISR(cyg_vector_t Vector, cyg_addrword_t Data)
{
    return (CYG_ISR_HANDLED);
}

short DAIGetPeak()
{
	short ret;
	 /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);

	ret = DAIPeak;
	DAIPeak = 0;

	 /* Exit VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);

	return ret;

}

void
DAIResetPlayback(void)
{
    int Buffer;

    for (Buffer = 0; Buffer < NUM_PLAYBACK_BUFFERS; ++Buffer) {
        _PlaybackBuffer[Buffer].Data = PlaybackBuffer[Buffer];
        _PlaybackBuffer[Buffer].Size = 0;
        _PlaybackBuffer[Buffer].Position = 0;
        _PlaybackBuffer[Buffer].Next = &_PlaybackBuffer[Buffer + 1];
    }
    _PlaybackBuffer[Buffer - 1].Next = &_PlaybackBuffer[0];
    
    DAIPlaybackBuffer = &_PlaybackBuffer[0];
    _WriteBuffer = &_PlaybackBuffer[0];
}

#if defined(ENABLE_DAI_RECORD)
void
DAIResetRecord(void)
{
    int Buffer;
    
    for (Buffer = 0; Buffer < NUM_RECORD_BUFFERS; ++Buffer) {
        _RecordBuffer[Buffer].Data = RecordBuffer[Buffer];
        _RecordBuffer[Buffer].Size = RECORD_BUFFER_SIZE;
        _RecordBuffer[Buffer].Position = 0;
        _RecordBuffer[Buffer].Next = &_RecordBuffer[Buffer + 1];
    }
    _RecordBuffer[Buffer - 1].Next = &_RecordBuffer[0];

    _RecordBuffer[0].Size = 0;
    DAIRecordBuffer = &_RecordBuffer[0];
    _ReadBuffer = &_RecordBuffer[0];
}
#endif  // ENABLE_DAI_RECORD

#if defined(ENABLE_DAI_RECORD)

void
DAISetLoopbackFIQ()
{
    DAIPeak = 0;

	// wait for buffers to play
	DAIWaitForEmpty();

   /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);

	HAL_VSR_SET(CYGNUM_HAL_VECTOR_FIQ, DAIFIQMONITOR, NULL);

   /* Enter VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);


}

#endif

void
DAISetNormalFIQ()
{
	   /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);

     HAL_VSR_SET(CYGNUM_HAL_VECTOR_FIQ, DAIFIQ, NULL);
 
	 /* Enter VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);

}


void
DAIInit(void)
{
    static bool Initialized = false;

    if (!Initialized) {
    
        /* Initialize playback buffers */
        DAIResetPlayback();

#if defined (ENABLE_DAI_RECORD)
        /* Initialize record buffers */
        DAIResetRecord();
#endif
	
        /* Initialize starvation counter */
        DAIStarvationCounter = 0;
	
        /* Replace default FIQ handler with DAIFIQ, done seperately without mask protection */
        HAL_VSR_SET(CYGNUM_HAL_VECTOR_FIQ, DAIFIQ, NULL);
 
	
        /* Initialize sampling frequency to 44.1kHz */
        DAISetSampleFrequency(44100);
	
        /* dd: memset the buffers.  good hygiene? */
        memset( &PlaybackBuffer[0][0], 0, (NUM_PLAYBACK_BUFFERS*PLAYBACK_BUFFER_SIZE)<<1 );
#if defined(DAI_RECORD)
        memset( &RecordBuffer[0][0], 0, (NUM_RECORD_BUFFERS*RECORD_BUFFER_SIZE)<<1 );
#endif


        /* Initialize GPIO */
	
        /* Fake register a interrupt handler for the I2S spurious interrupt problem */
        cyg_drv_interrupt_create(HAL_INTERRUPT_NUM,
                                 99,        // priority
                                 0,         // data
                                 (cyg_ISR_t *)_I2SISR,
                                 (cyg_DSR_t *)0,
                                 &_I2SInterruptHandle,
                                 &_I2SInterrupt);
        cyg_drv_interrupt_attach(_I2SInterruptHandle);
        cyg_drv_interrupt_acknowledge(HAL_INTERRUPT_NUM);
	
#if defined(__DJ)
        /* Set codec enable as output (DJ moves ADDAreset to PB0) */
        *(volatile cyg_uint8 *)PBDDR |= CODEC_ENABLE;
#else
        /* Set codec enable as output */
        *(volatile cyg_uint8 *)PEDDR |= CODEC_ENABLE;
#endif
        Initialized = true;

        DAIWrite(0);
    }
}

int
DAISetSampleFrequency(unsigned int Frequency)
{
    int Status = 0;

#if defined(DAI_SUPPORT_HARDWARE_SRC)
    unsigned int FrequencyConfig = DAI64Fs_AUDDIV(2) | DAI64Fs_AUDCLKSRC;	/* Default 44100Hz */
    
    switch (Frequency) {
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)

#ifndef __DJ   // dj only supports 44.1khz and 32khz @ 90mhz due to removal of an oscillator
	    case 48000:
        {
		
            FrequencyConfig = DAI64Fs_AUDDIV(2) | DAI64Fs_AUDCLKSRC;			
            break;
        }
#endif // __DJ

        case 44100:
        {
		
            FrequencyConfig = DAI64Fs_AUDDIV(8); 
            break;
        }

		
		case 32000:
        {
		
            FrequencyConfig = DAI64Fs_AUDDIV(11);
            break;
        }

#ifndef __DJ
        case 22050:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(16);
            break;
        }

        case 11025:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(32);
            break;
        }
#endif // __DJ
#else /* CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000 */
        case 48000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(6);
            break;
        }
	
        case 44100: 
        {
            FrequencyConfig = DAI64Fs_AUDDIV(2) | DAI64Fs_AUDCLKSRC;
            break;
        }

        case 32000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(9);
            break;
        }

        case 24000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(12);
            break;
        }
	
        case 22050:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(4) | DAI64Fs_AUDCLKSRC;
            break;
        }

        case 16000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(18);
            break;
        }

        case 12000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(24);
            break;
        }
        
        case 11025:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(8) | DAI64Fs_AUDCLKSRC;
            break;
        }

        case 8000:
        {
            FrequencyConfig = DAI64Fs_AUDDIV(36);
            break;
        }
#endif /* CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000 */
        default: 
        {
		
            DEBUG(AUDIO,DBGLEV_ERROR,"Unsupported sample frequency %d\n", Frequency);
            Status = -1;
            break;
        }
    }

    if (Status != -1) {
        if( DEBUG_TEST( AUDIO, DBGLEV_WARNING ) && Frequency != 44100 ) {
            DEBUG(AUDIO,DBGLEV_WARNING,"Setting HW sample frequency to %d\n", Frequency);
        }
	
        _DAISampleFrequency = Frequency;
	
        /* Program divider network */
        *(volatile cyg_uint32 *)DAI64Fs = FrequencyConfig |
            DAI64Fs_AUDCLKEN | DAI64Fs_MCLK256EN | DAI64Fs_I2SF64;
    }
#else /* DAI_SUPPORT_HARDWARE_SRC */
    if (Frequency != 44100) {
        Status = -1;
    } 
#endif /* DAI_SUPPORT_HARDWARE_SRC */

    return Status;
}

void
DAIEnable(void)
{   
    /* If DAI is already enabled then return */
    if (*(volatile cyg_uint32 *)I2S_CTL & I2S_CTL_EN) {
        return;
    }

    /* Enable codec interface */
#if defined(__IOMEGA_32)
    *(volatile cyg_uint8 *)PEDR &= ~CODEC_ENABLE;
#elif defined(__DJ) /* PB0 is reset */
    *(volatile cyg_uint8 *)PBDR |= CODEC_ENABLE;
#else
    *(volatile cyg_uint8 *)PEDR |= CODEC_ENABLE;
#endif /* __IOMEGA_32 */

    /* Select the I2S interface */
#if defined(__EDB7312)
    *(volatile cyg_uint32 *)SYSCON3 |= SYSCON3_I2SSEL;
#else /* __EDB7312 */
    *(volatile cyg_uint32 *)SYSCON3 |= SYSCON3_I2SSEL | SYSCON3_128FS;
#endif /* __EDB7312 */
    
    /* Program the I2S control register to use the external clock (MCLK) and
     * interrupt when the right channel transmit FIFO is half empty */
    *(volatile cyg_uint32 *)I2S_CTL = I2S_CTL_FLAG | I2S_CTL_ECS | I2S_CTL_RCTM;

    /* Clear the overflow and underflow status bits */
    *(volatile cyg_uint32 *)I2S_STAT = 0xffffffff;

    /* Enable the I2S interface */
    *(volatile cyg_uint32 *)I2S_CTL |= I2S_CTL_EN;

    /* Enable the FIFOs for the left and right channels */
    while (!(*((volatile cyg_uint32 *)I2S_STAT) & I2S_STAT_FIFO))
        ;
    *((volatile cyg_uint32 *)I2S_FIFO_CTL) = I2S_FIFO_CTL_RIGHT_ENABLE;
    while (!(*((volatile cyg_uint32 *)I2S_STAT) & I2S_STAT_FIFO))
        ;
    *((volatile cyg_uint32 *)I2S_FIFO_CTL) = I2S_FIFO_CTL_LEFT_ENABLE;
    while (!(*((volatile cyg_uint32 *)I2S_STAT) & I2S_STAT_FIFO))
        ;
    
    /* Enable the DAC */
    DACEnable();
    
#if defined(ENABLE_DAI_RECORD)
    /* Enable the ADC */
    ADCEnable();
#endif
    
#if defined(ENABLE_SPDIF_CS8405A)
    SPDIFEnable();
#endif
}

void
DAIDisable(void)
{
    /* If DAI is already disabled then return */
    if (!(*(volatile cyg_uint32 *)I2S_CTL & I2S_CTL_EN)) {
        return;
    }

    /* Disable the DAC */
    DACDisable();

#if defined(ENABLE_DAI_RECORD)
    /* Disable the ADC */
    ADCDisable();
#endif
    
    /* Disable I2S FIFOs */
    while ((*(volatile cyg_uint32 *)I2S_STAT & I2S_STAT_FIFO) == 0)
        ;
    *(volatile cyg_uint32 *)I2S_STAT = I2S_STAT_FIFO;
    *(volatile cyg_uint32 *)I2S_FIFO_CTL = I2S_FIFO_CTL_RIGHT_DISABLE;
    while ((*(volatile cyg_uint32 *)I2S_STAT & I2S_STAT_FIFO) == 0)
        ;
    *(volatile cyg_uint32 *)I2S_STAT = I2S_STAT_FIFO;
    *(volatile cyg_uint32 *)I2S_FIFO_CTL = I2S_FIFO_CTL_LEFT_DISABLE;
    while ((*(volatile cyg_uint32 *)I2S_STAT & I2S_STAT_FIFO) == 0)
        ;
    *(volatile cyg_uint32 *)I2S_STAT = I2S_STAT_FIFO;
    
    /* Disable I2S interface */
    *(volatile cyg_uint32 *)I2S_CTL &= ~I2S_CTL_EN;

    /* Mask the I2S interrupt */
    if (!_VSRMasked) {
        cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);
        _VSRMasked = true;
    }

    /* Disable codec interface */
#if defined(__IOMEGA_32)
    *(volatile cyg_uint8 *)PEDR |= CODEC_ENABLE;
#elif defined(__DJ)
    *(volatile cyg_uint8 *)PBDR &= ~CODEC_ENABLE;
#else
    *(volatile cyg_uint8 *)PEDR &= ~CODEC_ENABLE;
#endif /* __IOMEGA_32 */
}

unsigned int
DAISamplesPerTick(void)
{
    return (_DAISampleFrequency / 100);
}

unsigned int
DAIGetBufferSize( void ) 
{
    return sizeof(short) * PLAYBACK_BUFFER_SIZE;
}

unsigned int
DAIGetNextBuffer(short ** Buffer, unsigned int* NumSamples)
{
    unsigned int NumSamplesLeft = 0;

    /* Enter VSR critical section */
    if (!_VSRMasked) {
        cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);
    }

    /* Return number of samples left to play in buffer */
    NumSamplesLeft = _WriteBuffer->Size - _WriteBuffer->Position;

    /* If buffer is empty then */
    if (NumSamplesLeft == 0 || _WriteBuffer->Position > _WriteBuffer->Size) {
        NumSamplesLeft = 0;
        /* Set return values */
        *Buffer = _WriteBuffer->Data;
        *NumSamples = PLAYBACK_BUFFER_SIZE;
    }
	
    /* Leave VSR critical section */
    if (!_VSRMasked) {
        cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);
    }
    
    return NumSamplesLeft;
}

void
DAIWaitForEmpty(void) 
{
    DAIBuffer_T* pBuf = _PlaybackBuffer;
    unsigned int samples, samples_per_tick;

    samples_per_tick = DAISamplesPerTick();
    
    while( (pBuf->Size > pBuf->Position) ||
           (pBuf->Next->Size > pBuf->Next->Position ) )
    {
        if( pBuf->Size == pBuf->Position ) {
            pBuf = pBuf->Next;
        }
        samples = pBuf->Size - pBuf->Position;
        if( samples > samples_per_tick ) {
            cyg_thread_delay( (samples/samples_per_tick) );
        }
        else {
            cyg_thread_delay( 1 );
        }
    }
}

void
DAIWrite(unsigned int NumSamples)
{
    /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);
    
    /* Update write buffer size and position.  This will
     * enable the playback. */
    _WriteBuffer->Size = NumSamples;
    _WriteBuffer->Position = 0;
    
    /* Leave VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);
    
    /* Goto next write buffer */
    _WriteBuffer = _WriteBuffer->Next;
}

#if defined(ENABLE_DAI_RECORD)
unsigned int
DAIRead(short ** Buffer, unsigned int * NumSamples)
{
    unsigned int NumSamplesLeft = 0;
    
    /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);

    if (_ReadBuffer->Size == 0) {
        /* This will start recording */
        _ReadBuffer->Size = RECORD_BUFFER_SIZE;
    }

    /* Return number of samples left to play in buffer */
    NumSamplesLeft = _ReadBuffer->Size - _ReadBuffer->Position;

    /* If buffer is full then */
    if (NumSamplesLeft == 0) {

        /* Set return values */
        *Buffer = _ReadBuffer->Data;
        *NumSamples = RECORD_BUFFER_SIZE;
    }
    
    /* Leave VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);

    return NumSamplesLeft;
}

void
DAIReleaseBuffer(void)
{
    /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);
    
    /* Update read buffer size and position.  This will _not_ disable recording.
       DAIDisable must be called to stop recording once it is started. */
    _ReadBuffer->Size = RECORD_BUFFER_SIZE;
    _ReadBuffer->Position = 0;
    
    /* Leave VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);
    
    /* Goto next write buffer */
    _ReadBuffer = _ReadBuffer->Next;
}

unsigned int
DAIFreeRecordBufferCount(void)
{
    DAIBuffer_T *buf;
    unsigned int count = 0;
    /* Enter VSR critical section */
    cyg_drv_interrupt_mask(HAL_INTERRUPT_NUM);

    for(buf = _ReadBuffer->Next; buf != _ReadBuffer; buf = buf->Next) {
	    if (buf->Position == 0)
		    count++;
    }


    /* Leave VSR critical section */
    cyg_drv_interrupt_unmask(HAL_INTERRUPT_NUM);

    return count;
}

#endif /* ENABLE_DAI_RECORD */
