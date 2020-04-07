/*************************************************************
Pogo Bringup Test

Author: Teman Clark-Lindh
Date: 10/23/01

Description: 
Extremely simple pass/fail test. Should take < 30 seconds per 
board to test components. Test code taken from existing 
tests and driver initialization routines. Extra hacky at this point. :)


*************************************************************/
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <devs/lcd/lcd.h> 
#include <cyg/hal/mmgpio.h>
#include <devs/storage/ata/atadrv.h>
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // HAL interrupt macros
#include <cyg/hal/hal_if.h>
#include <cyg/hal/drv_api.h>
#include <devs/audio/dai.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>
#include "testgui.h"


/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];
/*
typedef int BOOL;
#define TRUE 1
#define FALSE 0
*/


cyg_io_handle_t blk_devC,blk_devH;

extern int LCDMemoryTest();

// start usb declarations
typedef struct usbs_dharma_pdiusbd12_hardware {
    volatile cyg_uint8 data;
    volatile cyg_uint8 command;
} usbs_dharma_pdiusbd12_hardware;

static usbs_dharma_pdiusbd12_hardware* const usbs_dharma_pdiusbd12_base =
(usbs_dharma_pdiusbd12_hardware* const) 0x40000000;

#define USBS_DATA       (&(usbs_dharma_pdiusbd12_base->data))
#define USBS_COMMAND    (&(usbs_dharma_pdiusbd12_base->command))

#define COMMAND_SELECT_ENDPOINT             0x00
#define COMMAND_READ_LAST_TRANSACTION_STATUS    0x40
#define COMMAND_SET_ENDPOINT_STATUS         0x40
#define COMMAND_GET_ENDPOINT_STATUS         0x80
#define COMMAND_SET_ADDRESS_ENABLE          0xD0
#define COMMAND_SET_ENDPOINT_ENABLE         0xD8
#define COMMAND_READ_BUFFER                 0xF0
#define COMMAND_WRITE_BUFFER                0xF0
#define COMMAND_ACK_SETUP                   0xF1
#define COMMAND_CLEAR_BUFFER                0xF2
#define COMMAND_SET_MODE                    0xF3
#define COMMAND_READ_INTERRUPT              0xF4
#define COMMAND_READ_FRAME_NUMBER           0xF5
#define COMMAND_VALIDATE_BUFFER             0xFA
#define COMMAND_SET_DMA                     0xFB

#define CONFIG1_NO_LAZY_CLOCK               0x02
#define CONFIG1_CLOCK_RUNNING               0x04
#define CONFIG1_INTERRUPT_MODE              0x08
#define CONFIG1_SOFT_CONNECT                0x10
#define CONFIG1_NONISO_MODE                 0x00
#define CONFIG1_ISOOUT_MODE                 0x40
#define CONFIG1_ISOIN_MODE                  0x80
#define CONFIG1_ISOIO_MODE                  0xC0
#define CONFIG2_CLOCK_12M                   0x03
#define CONFIG2_CLOCK_4M                    0x0B
#define CONFIG2_SET_TO_ONE                  0x40
#define CONFIG2_SOF_ONLY                    0x80

#define INTERRUPT_EP0                    0x0001
#define INTERRUPT_EP1                    0x0002
#define INTERRUPT_EP2                    0x0004
#define INTERRUPT_EP3                    0x0008
#define INTERRUPT_EP4                    0x0010
#define INTERRUPT_EP5                    0x0020
#define INTERRUPT_BUS_RESET              0x0040
#define INTERRUPT_SUSPEND                0x0080
#define INTERRUPT_DMA_EOT                0x0100

#define DMA_EP4_INT_ENABLE 0x40
#define DMA_EP5_INT_ENABLE 0x80

#define TRANSACTION_STATUS_DATA_RX_TX_SUCCESS   0x01
#define TRANSACTION_STATUS_ERROR_CODE_MASK      0x1E
#define TRANSACTION_STATUS_SETUP_PACKET         0x20
#define TRANSACTION_STATUS_DATA1_PACKET         0x40
#define TRANSACTION_STATUS_PREVIOUS_NOT_READ    0x80

#define EP_STATUS_FULL                0x01
#define EP_STALL                      0x02
#define EP_BUFFER0_FULL               0x20
#define EP_BUFFER1_FULL               0x60

#define EP0_BUFFER_SIZE 16
#define EP1_BUFFER_SIZE 16
#define EP4_BUFFER_SIZE 64
#define EP5_BUFFER_SIZE 64
// end usb declarations


// start audio declarations

BOOL usb = FALSE;
BOOL hd = FALSE;
BOOL bl = FALSE;

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
    unsigned int NumSamplesLeft,Delay;
    for (;;) {
	NumSamplesLeft = DAIRead(&_InLeft, &NumSamples);
	if (NumSamplesLeft <= 0) {
	    break;
	}
	Delay = NumSamplesLeft / DAISamplesPerTick();
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
  unsigned long Sample;
    for (Sample = 0; Sample < SamplesPerChannel; ++Sample) {
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
    unsigned int NumSamplesLeft,Delay;
    for (;;) {
	NumSamplesLeft = DAIGetNextBuffer(&_OutLeft, &NumSamples);
	if (NumSamplesLeft <= 0) {
	    break;
	}
	Delay = NumSamplesLeft / DAISamplesPerTick();
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
  unsigned long Sample;
    for (Sample = 0; Sample < SamplesPerChannel; ++Sample) {
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

static void
Loopback(void)
{
  int i;
  diag_printf("Start Record\n");
    StartRecord();
    StartPlayback();
    for (i = 0; i < 64; ++i) {
 //     diag_printf("Read\n");
	Read(LeftChannel, (sizeof(LeftChannel) / sizeof(short)));
//	diag_printf("Write\n");
	Write(LeftChannel, (sizeof(LeftChannel) / sizeof(short)));
	// Read(RightChannel, (sizeof(RightChannel) / sizeof(short)));
	// Write(RightChannel, (sizeof(RightChannel) / sizeof(short)));
    }
    StopPlayback();
    StopRecord();
}

static void
_RecordThread()
{       
  unsigned int FrequencyList[] = {48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};
  unsigned int Frequency = 0;
  cyg_int8 Volume;

    DEBUG("+%s\n", __FUNCTION__);

    _Constructor();

    Start();

    DAISetSampleFrequency(48000);

    Loopback();

#if 0
        
#if defined(DAI_SUPPORT_HARDWARE_SRC)
 
    DEBUG("Testing sampling frequencies\n");
    for (Frequency = 0; Frequency < (sizeof(FrequencyList) / sizeof(FrequencyList[0])); ++Frequency) {
	DEBUG("Setting sampling frequency to %d\n", FrequencyList[Frequency]);
	if (DAISetSampleFrequency(FrequencyList[Frequency]) < 0) {
	    continue;
	}
	Loopback();
    }


#endif
    

    DEBUG("Testing recording volume\n");
    for (Volume = ADC_MIN_VOLUME; Volume < ADC_MAX_VOLUME; Volume += 10) {
	DEBUG("Setting volume to %d\n", Volume);
	ADCSetVolume(Volume);
	Loopback();
    }
    ADCSetVolume(0);

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
#endif // #if 0

    Stop();

    DEBUG("-%s\n", __FUNCTION__);
} 


void ATATest()
{  

  cyg_uint16 length;
  cyg_uint32 lba;
  unsigned int i;
  cyg_uint32 len;
  Cyg_ErrNo err;
  drive_geometry_t dg;
    
   // intialize HD, get info
  if (cyg_io_lookup("/dev/hda/", &blk_devH) != ENOERR) {
	diag_printf("Could not get handle to dev");
  } 
    diag_printf("got handle to hd\n");
    len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device");
	return FALSE;
    }
    diag_printf("powered up hd\n" );


    len = sizeof(dg);
    if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
 	diag_printf("Could not get geometry");
	return false;
    }

    diag_printf("HD Info\n");
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);


}

// Write a value to a register
static void
usbs_dharma_pdiusbd12_write(volatile cyg_uint8* addr, cyg_uint8 value)
{
    volatile int delay;
    
    *addr = value;

    // Delay ~500ns to comply with the timing specification of the PDIUSBD12
    for (delay = 0; delay < 24; ++delay)
	;
}

// Read a value from a register
static cyg_uint8
usbs_dharma_pdiusbd12_read(volatile cyg_uint8* addr)
{
    volatile int delay;
    cyg_uint8 data;
    
    data = *addr;

    // Delay ~500ns to comply with the timing specification of the PDIUSBD12
    for (delay = 0; delay < 24; ++delay)
	;

    return data;
}

BOOL usbtest()
{

  cyg_uint8 test;
  
  diag_printf("Waiting 100 ticks for USB powerup\n");
  cyg_thread_delay(200);
  
  diag_printf("Testing USB registers\n");

  // Activate the hardware
  usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_MODE);
  usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG1_SOFT_CONNECT | CONFIG1_NONISO_MODE);
  usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG2_SET_TO_ONE | CONFIG2_CLOCK_4M);
  
  usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_DMA);
  usbs_dharma_pdiusbd12_write(USBS_DATA, DMA_EP4_INT_ENABLE | DMA_EP5_INT_ENABLE);
   
  usbs_dharma_pdiusbd12_write(USBS_COMMAND, COMMAND_SET_DMA);
  test = usbs_dharma_pdiusbd12_read(USBS_DATA);
  
 if(test == (DMA_EP4_INT_ENABLE | DMA_EP5_INT_ENABLE)) {
   diag_printf("usb register read/write test - succeeded\n");
 }
 else {
   diag_printf("usb register read/write test - failed %x \n",test);
   return FALSE;
 }

 diag_printf("delay one half second\n");
 cyg_thread_delay(50);

 diag_printf("setting clock to 12Mhz\n");
 usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG2_SET_TO_ONE | CONFIG2_CLOCK_12M);

 
 diag_printf("delay one half second\n");
 cyg_thread_delay(50);

 diag_printf("setting clock to 4Mhz\n");
 usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG2_SET_TO_ONE | CONFIG2_CLOCK_4M);

 diag_printf("delay one half second\n");
 cyg_thread_delay(50);

 diag_printf("setting clock to 12Mhz\n");
 usbs_dharma_pdiusbd12_write(USBS_DATA, CONFIG2_SET_TO_ONE | CONFIG2_CLOCK_12M);

 return TRUE;

}


void thread_entry(cyg_uint32 data)
{

	unsigned char temp[4096];

	BOOL pass,bBlack; 
	int i,j;

	cyg_uint32 state;

	pass = TRUE;
	bBlack = FALSE;
	diag_printf("\nPogo Quicktest Sequence\n");

#if 0
	LCDEnable();
	cyg_thread_delay(200);

	while(1)
	{

		// backlight toggle

		LCDReverse(0);
		cyg_thread_delay(100);
		LCDReverse(1);
		cyg_thread_delay(100);
	}


	while(1)
	{
		for(i = 0; i < 128; i++)
		{
			for(j = 0; j < 64; j++)
			{
				LCDPutPixel(1,i,j);
			}	
		}
		cyg_thread_delay(200);
		
		for(i = 0; i < 128; i++)
		{
			for(j = 0; j < 64; j++)
			{
				LCDPutPixel(0,i,j);
			}	
		}
		cyg_thread_delay(200);
	}

#endif


  // GUI Init
  InitGUI();
	GUIPrint("Pogo Bringup Test");
  diag_printf("\nGUI Init Complete\n");
  //  cyg_thread_delay(200);


  LCDMemoryTest();

   diag_printf("Testing USB\n");
  if(!usbtest()) {
    GUIPrint("USB Test Failed");
    pass = FALSE;
  }
  else
    GUIPrint("USB Passed");

  cyg_thread_delay(200);
      
  diag_printf("Testing ATA Command Interfaces\n");

  ATATest();

  GUIPrint("Testing Audio Loopback...");
  _RecordThread();
  
  while(1)
  {
	  cyg_thread_delay(100);
  }


}


void cyg_user_start(void)
{

 
  // spawn test_thread
  cyg_thread_create(9, thread_entry, 0, "main thread",
		    (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
  cyg_thread_resume(threadh[0]);

}


