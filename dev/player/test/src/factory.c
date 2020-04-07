/*************************************************************
Dharma2 Factory Test

Author: Teman Clark-Lindh
Date: 10/23/01

Description: 
Extremely simple pass/fail test. Should take < 30 seconds per 
board to test components. Test code taken from existing 
tests and driver initialization routines.


*************************************************************/
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <devs/lcd/lcd.h> 
#include <cyg/hal/mmgpio.h>
#include <devs/storage/ata/atadrv.h>
#include "atabus.h"
#include "busctrl.h"
#include "ataproto.h"
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // HAL interrupt macros
#include <cyg/hal/hal_if.h>
#include <cyg/hal/drv_api.h>
#include <devs/audio/dai.h>
#include "testgui.h"

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

/* DEFINES */

cyg_io_handle_t blk_devC,blk_devH,blk_devCF;
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

// ide bus declarations
extern BusSpace_T Bus0;
extern BusSpace_T Bus1;

// ethernet declarations

#define CS8900_BASE 0xf0000000
#include "cs8900.h"
#define ETHER_ADDR_LEN 6

static unsigned char enaddr[ETHER_ADDR_LEN];

// 802.11 declarations
#define ORINOCO_ATTRIBUTE_BASE 0x32000000



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
    StartRecord();
    StartPlayback();
    for (i = 0; i < 64; ++i) {
	Read(LeftChannel, (sizeof(LeftChannel) / sizeof(short)));
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

    DAISetSampleFrequency(44100);

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


/* GPIO for power supply */
#define SDA (1<<4)
#define SCL (1<<5)
#define SLA0 (1<<2)

typedef enum 
{
    VL_0,
    VL_33,
    VL_5,
    VL_12
} VoltageLevel_T;

static void
ClockPowerSupplyBit(int Bit)
{
    /* Data is clocked in on positive leading edge of clock,
       so set data line... */
    if (Bit) {
	*(volatile cyg_uint8 *)PDDR |= SDA;
    }
    else {
	*(volatile cyg_uint8 *)PDDR &= ~SDA;
    }

    /* ...and clock it in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;
}
    
static bool
ConfigurePowerSupply(VoltageLevel_T Voltage) 
{
    int ConfigurationData;
    int BitMask;

    /* Configure the GPIO first
      PD4 - SDA
      PD5 - SCL
      PB2 - SLA0 */
    *(volatile cyg_uint8 *)PDDDR &= ~(SDA | SCL);
    *(volatile cyg_uint8 *)PBDDR |= SLA0;
    
    /* Configuration will be for 5V, this gives the following
       bits, MSB (D8) first: 1 0110 1010 */
    if (Voltage != VL_5) {
	diag_printf("XXX Not supported XXX\n");
	return false;
    }
    ConfigurationData = 0x16a;

    /* The interface cannot be clocked faster than 2.5MHz, which
       is a 0.4uS pulse width, so we will be fine */

    /* The datasheet shows a clock before the first data bit is
       clocked in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;

    /* Now clock the data in */
    for (BitMask = 0x100; BitMask; BitMask >>= 1) {
	ClockPowerSupplyBit(ConfigurationData & BitMask);
    }

    /* Now latch the data in */
    *(volatile cyg_uint8 *)PBDR |= SLA0;
    HAL_DELAY_US(5);
    *(volatile cyg_uint8 *)PBDR &= ~SLA0;

    return true;
}

static bool
PowerOnCard(void)
{
    bool Status;
    
    /* NOTE All delays are longer than necessary */
    
    /* Card detect goes high - wait at least 50mS */
    //cyg_thread_delay(10);
    HAL_DELAY_US(10 * 1000);
    
    /* Apply VCC - wait at least 300 mS */
    Status = ConfigurePowerSupply(VL_5);
    if (Status == true) {
	//cyg_thread_delay(60);
	HAL_DELAY_US(60 * 1000);
    
	/* Assert reset for at least 10 uS */
	*(volatile cyg_uint8 *)PBDDR |= (1<<0);
	*(volatile cyg_uint8 *)PBDR |= (1<<0);
	HAL_DELAY_US(20);
	
	/* De-assert reset - wait at least 20 mS */
	*(volatile cyg_uint8 *)PBDR &= ~(1<<0);
	//cyg_thread_delay(10);
	HAL_DELAY_US(10 * 1000);
	
	/* Card access is now allowed */
	return true;
    }
    else {
	/* Could not configure power supply */
	return false;
    }
}

static void
SoftResetCard(volatile cyg_uint16 * COR) 
{
    /* NOTE All delays are longer than necessary */
    
    /* Assert reset for at least 10 uS */
    *COR = 0x80;
    HAL_DELAY_US(20);
    
    /* De-assert reset - wait at least 20 mS */
    *COR = 0x00;
    //cyg_thread_delay(10);
    HAL_DELAY_US(10 * 1000);
    
    /* Card access is now allowed */
}

static char ma_id[4] = { 0x56,0x01,0x02,0x00 };

static bool
PrintCIS(volatile cyg_uint16 * CIS)
{

    int Position = 0;
    cyg_uint8 Tag;
    cyg_uint8 Link;
    int i;
    bool bFound = false;
    
    for (;;) {
	Tag = CIS[Position++] & 0xff;
	diag_printf("TAG %02x ", Tag);
	if (Tag == 0xff) {
	    break;
	}
	Link = CIS[Position++] & 0xff;
	diag_printf("LNK %02x ", Link);
	for (i = 0; i < Link; ++i) {
	  // check manufacturer id
	  if(Tag == 0x20) {
	    bFound = true;
	 

	    // check manufacturer ID, sudden death if it doesn't match
	    if(ma_id[i] != (CIS[Position] & 0xff))
	      return false;
	  }

	    diag_printf("%02x ", CIS[Position++] & 0xff);
	}
	diag_printf("\n");
    }
    diag_printf("\n");

    return bFound;
}

static bool
InitializePCMCIAInterface(void) 
{
    volatile cyg_uint16 * AttributeMemory = (volatile cyg_uint16 *)ORINOCO_ATTRIBUTE_BASE;
    bool Status;
    cyg_uint32 timeout;

     /* Set up GPIO before anything else so that we know what state the
       outputs are in before trying to power on the card */
    SetMMGPO(MMGPO_PCMCIA_IO_SELECT, (MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_REG));

    /* Card insertion */
    diag_printf("nEINT2: %x\n", *(volatile cyg_uint32 *)INTSR1 & INTSR1_EINT2);
    diag_printf("CARD_DETECT (MMI_4): %x\n", GetMMGPI() & MMGPI_PCMCIA_CD);
    
    /* Determine voltage of card */
    diag_printf("VS1 (MMI_0): %x VS2 (MMI_1): %x\n", GetMMGPI() & MMGPI_PCMCIA_VS1,
	  GetMMGPI() & MMGPI_PCMCIA_VS2);

    /* Turn on card */
    diag_printf("Powering on card\n");
    Status = PowerOnCard();
    if (Status == true) {

	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");
	
	// 5 second timeout
	timeout = cyg_current_time() + 5000;
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY)) {
	  if(cyg_current_time() > timeout) {
	    diag_printf("RDY timeout\n");
	    return false;
	  }
	}

	/* Configure bus to 16bit databus, 8 wait states, EXPRDY.
	   Card is found at nCS3 | A25 */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
	*(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
	*(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN; /* This doesn't seem to do anything, but I will play it safe */
	
	/* Soft reset card and wait for ready again */
	diag_printf("Soft resetting card\n");
	SoftResetCard((volatile cyg_uint16 *)0x320003e0); /* 0x3e0 for Orinoco */
	
	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");

    	// 5 second timeout
	timeout = cyg_current_time() + 5000;
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY)) {
	  if(cyg_current_time() > timeout) {
	    diag_printf("RDY timeout\n");
	    return false;
	  }
	}

	
	/* Read CIS , check ID*/
	diag_printf("Reading CIS\n");
	return PrintCIS(AttributeMemory);

    }
    else {
	/* Could not power on card */
	return false;
    }
}

BOOL pcmciatest()
{
   
    bool InitStatus = false;
    volatile cyg_uint16 * AttributeMemory = (volatile cyg_uint16 *)ORINOCO_ATTRIBUTE_BASE;
    int Status;
    cyg_uint32 timeout;

  
    
    /* Initialize the interface */
    InitStatus = InitializePCMCIAInterface();
    if (InitStatus == true) {

	/* Configure the card */
	diag_printf("Configuring the card\n");
	AttributeMemory[0x3e0 >> 1] = 0x41;
	diag_printf("COR %08x = %02x\n", &AttributeMemory[0x3e0 >> 1], AttributeMemory[0x3e0 >> 1]);

	/* Wait for card to be ready */
	diag_printf("Waiting for card to be ready\n");

	// 5 second timeout
	timeout = cyg_current_time() + 5000;
	while (!(GetMMGPI() & MMGPI_PCMCIA_RDY)) {
	  if(cyg_current_time() > timeout) {
	    diag_printf("RDY timeout\n");
	    return FALSE;
	  }
	}
	
	/* Set the interface to I/O mode */
	diag_printf("Setting interface for I/O mode\n");
	SetMMGPO(0, (MMGPO_PCMCIA_REG | MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_IO_SELECT));

	/* Configure bus to 16bit databus, 8 wait states, EXPRDY.
	   Card is found at nCS3 | A25 */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
	/* 1 wait state + EXPRDY + EXCKEN I would think would work, but nope.
	   Max wait states + EXPRDY doesn't work either, but catches some of the bytes. */
	*(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
	*(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN; /* This doesn't seem to do anything, but I will play it safe */
	
	return TRUE;
    }

    return FALSE;
}


static bool 
cs8900_test()
{

    unsigned short chip_type, chip_rev, chip_status;
    unsigned short reg, write_val, read_val;
    bool bmac = false;
    int i;

    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT3);
    
    chip_type = get_reg(PP_ChipID);
    chip_rev = get_reg(PP_ChipRev);


    diag_printf("CS8900 - type: %x, rev: %x\n", chip_type, chip_rev);


    // Verify memory on chip
    // Walk 1s on data bus
    for (write_val = 0x0001; write_val != 0; write_val <<= 1) {
	for (reg = 0x0150; reg <= 0x015d; reg += 2) {
	    put_reg(reg, write_val);
	    read_val = get_reg(reg);
	    if (write_val != read_val) {
		diag_printf(" Error\n");
		return false;
	    }
	}
    }
    // Incrementing value test
    for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
	put_reg(reg, write_val);
    }
    for (reg = 0x0150, write_val = 0x0101; reg <= 0x015d; reg += 2, write_val += 0x0101) {
	read_val = get_reg(reg);
	if (write_val != read_val) {
	    diag_printf(" Error\n");
	    return false;
	}
    }
    diag_printf("CS8900 - Memory OK\n");
    
    put_reg(PP_SelfCtl, PP_SelfCtl_Reset);  // Reset chip
    // TODO Cirrus driver reads uchar from PPtr, PPtr + 1, PPtr, PPtr + 1 here to transition SBHE from 8 to 16 bit
    // So what is SBHE?
    while ((get_reg(PP_SelfStat) & PP_SelfStat_SIBSY) == 0) ; // Wait for EEPROM not busy
    while ((get_reg(PP_SelfStat) & PP_SelfStat_InitD) == 0) ; // Wait for initialization to be done

    chip_status = get_reg(PP_SelfStat);

    diag_printf("CS8900 - status: %x (%sEEPROM present)\n", chip_status,
                chip_status&PP_SelfStat_EEPROM ? "" : "no ");


    diag_printf("CS8900 - ");
    for (i = 0;  i < ETHER_ADDR_LEN;  i += 2) {
        unsigned short esa_reg = get_reg(PP_IA+i);
        enaddr[i] = esa_reg & 0xFF;
        enaddr[i+1] = esa_reg >> 8;
	diag_printf("%02x %02x", enaddr[i], enaddr[i+1]);
	
	// make sure eeprom is initialized with mac correctly
	if(enaddr[i] != 0xFF || enaddr[i+1] != 0xFF)
	  bmac = true;
    }
    diag_printf("\n");
    
    if(bmac)
      return true;
    else
      return false;
}


bool
TATACommandDone(BusSpace_T * Bus, Transfer_T * Transfer) 
{
    ATACommand_T * Cmd;
    bool bRet = true;

    Cmd = (ATACommand_T *)Transfer->Cmd;

    if (Bus->Status & STATUS_ERR) {
	Cmd->Flags |= ATA_CMD_ERROR;
	Cmd->Error = Bus->Error;
	diag_printf("Bus error in TATACommandDone\n");
        bRet = false;
    }
    Cmd->Flags |= ATA_CMD_DONE;
    if ((Cmd->Flags & ATA_CMD_READ_REG) != 0) {
	Cmd->DeviceHead = BusRead8(Bus, Bus->DeviceHeadReg);
	Cmd->Cylinder = BusRead8(Bus, Bus->CylinderHighReg) << 8;
	Cmd->Cylinder |= BusRead8(Bus, Bus->CylinderLowReg);
	Cmd->SectorNumber = BusRead8(Bus, Bus->SectorNumberReg);
	Cmd->SectorCount = BusRead8(Bus, Bus->SectorCountReg);
	Cmd->Error = BusRead8(Bus, Bus->ErrorReg);
	Cmd->Features = BusRead8(Bus, Bus->FeaturesReg);
    }
    else
      diag_printf("State Error in TATACommandDone\n");

    return bRet;

}

bool
TATACommandInterrupt(BusSpace_T * Bus, Transfer_T * Transfer)
{
    ATACommand_T * Cmd;
    int ByteCount;
    char * Data;
    int WaitStatus;
    
    Cmd = (ATACommand_T *)Transfer->Cmd;
    ByteCount = Cmd->ByteCount;
    Data = Cmd->Data;

    // poll with ReadStatus Command
    WaitStatus = ATAWait(Bus, Cmd->StatusAfter, Cmd->StatusAfter, Cmd->Timeout);

    if (Cmd->Flags & ATA_CMD_READ) {
	if (ByteCount > 0) {
	    Bus->Read16Multiple(Bus, Bus->DataReg, (unsigned short *)Data, (ByteCount >> 1));
	}
    }
    else if (Cmd->Flags & ATA_CMD_WRITE) {
	if (ByteCount > 0) {
	    Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)Data, (ByteCount >> 1));
	}
    }
    else 
      diag_printf("Unknown State in TATACommandInterrupt\n");

    return TATACommandDone(Bus, Transfer);
}


bool
TATACommandStart(BusSpace_T * Bus, Transfer_T * Transfer)
{
    int Drive;
    ATACommand_T * Cmd;

    Drive = Transfer->Drive;
    Cmd = (ATACommand_T *)Transfer->Cmd;
    
    BusWrite8(Bus, Bus->DeviceHeadReg, (Drive << 4));

    ATACommand(Bus, Drive, Cmd->Command, Cmd->Cylinder, Cmd->Head, Cmd->SectorNumber,
	       Cmd->SectorCount, Cmd->Features);

    return TATACommandInterrupt(Bus, Transfer);
}

bool
TATAExecuteCommand(ATACommand_T * Cmd, BusSpace_T *Bus, int Drive)
{
    Transfer_T Transfer;

    memset(&Transfer, 0, sizeof(Transfer));
    Transfer.Drive = Drive; // 0 master, 1 slave
    Transfer.Cmd = Cmd;
    
    if(TATACommandStart(Bus, &Transfer))
      return true;

    
    return false;

}

BOOL
ATATestDevice(BusSpace_T *Bus,int Drive)
{
    ATACommand_T Cmd;
    unsigned short IdentifyData[256];
    bool Status = false;

    ATADeviceInfo_T *DeviceInfo,di;

    DeviceInfo = &di;

    memset(&Cmd, 0, sizeof(Cmd));
    if(Bus == &Bus0 && Drive == 0)
      {
	Cmd.Command = ATAPI_IDENTIFY;
      }
    else
      {
	Cmd.Command = ATA_IDENTIFY;
      }

    Cmd.StatusBefore = STATUS_DRDY;
    Cmd.StatusAfter = STATUS_DRQ;
    Cmd.Timeout = ATA_DELAY;
    Cmd.Flags |= (ATA_CMD_READ);
    Cmd.Data = IdentifyData;
    Cmd.ByteCount = sizeof(IdentifyData);
    
    Status = TATAExecuteCommand(&Cmd,Bus,Drive);

    if(Status)
      {
	diag_printf("identify drive success\n");

	/* Get geometry information */
 	DeviceInfo->Cylinders = IdentifyData[1];
	DeviceInfo->Heads = IdentifyData[3];
	DeviceInfo->SectorsPerTrack = IdentifyData[6];
	DeviceInfo->TotalSectors = (IdentifyData[61] << 16) | IdentifyData[60];
	diag_printf( "C/H/S: %d/%d/%d Total Sectors %d\n",
	       DeviceInfo->Cylinders, DeviceInfo->Heads, DeviceInfo->SectorsPerTrack,
	       DeviceInfo->TotalSectors );
	
	/* TODO Byte swap these strings */
	memcpy(DeviceInfo->SerialNumber, &IdentifyData[10], 20);
	memcpy(DeviceInfo->ModelNumber, &IdentifyData[27], 40);
	diag_printf("SN: %s MN: %s\n", DeviceInfo->SerialNumber, DeviceInfo->ModelNumber );
	return TRUE;
      }
  
    
    diag_printf("identify drive failed\n" );
    return FALSE;

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
   diag_printf("usb register read/write test - failed\n");
   return FALSE;
 }

 return TRUE;

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

 // intialize CD, get info
  if (cyg_io_lookup("/dev/cda/", &blk_devC) != ENOERR) {
	diag_printf("Could not get handle to dev");
  } 
    diag_printf("got handle to cd\n");
    len = sizeof(len);

    while (cyg_io_set_config(blk_devC, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device");
	return FALSE;
    }
    diag_printf("powered up cd\n" );


    len = sizeof(dg);
    if (cyg_io_get_config(blk_devC, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
 	diag_printf("Could not get geometry");
	return false;
    }

    diag_printf("CD Info\n");
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);

   // intialize HD, get info
  if (cyg_io_lookup("/dev/hdb/", &blk_devCF) != ENOERR) {
	diag_printf("Could not get handle to dev");
  } 
    diag_printf("got handle to cf\n");
    len = sizeof(len);

    while (cyg_io_set_config(blk_devCF, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device");
	return FALSE;
    }
    diag_printf("powered up cf\n" );


    len = sizeof(dg);
    if (cyg_io_get_config(blk_devCF, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
 	diag_printf("Could not get geometry");
	return false;
    }

    diag_printf("CF Info\n");
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);


}

void thread_entry(cyg_uint32 data)
{
  BOOL pass; 
  int i,j;

  cyg_uint32 state;
  volatile cyg_uint16 * AttributeMemory;

  pass = TRUE;
  diag_printf("\nDharma2 Factory Test Sequence\n");
    

  // GUI Init
  InitGUI();

  cyg_thread_delay(200);

#if 0
  // a rather useless INT test here
  // both usb and pcmcia trigger EINT1, so this might represent either
  if(*(volatile cyg_uint32 *)INTSR1 & INTSR1_EINT1)
    diag_printf("EINT1 is high\n");
  else {
    diag_printf("EINT1 is low - failed\n");
    pass = FALSE;
  }
#endif

  if(!usbtest()) {
    GUIPrint("USB Test Failed");
    pass = FALSE;
  }
  else
    GUIPrint("USB Passed");

  cyg_thread_delay(200);
      

  diag_printf("Testing ATA Command Interfaces\n");

//  ATATest();

  

  if(!ATATestDevice(&Bus0,0)) {
      GUIPrint("Primary IDE Test Failed");
      pass = FALSE;
  }
  else
    GUIPrint("Primary IDE Passed");

  cyg_thread_delay(200);


  if(!ATATestDevice(&Bus0,1)) {
    GUIPrint("Secondary IDE Test Failed");
     pass = FALSE;
  }
  else
    GUIPrint("Secondary IDE Passed");

  cyg_thread_delay(200);

  if(!ATATestDevice(&Bus1,0)) {
    GUIPrint("CompactFlash Test Failed");
    pass = FALSE;
  }
  else
    GUIPrint("CompactFlash Passed");

  cyg_thread_delay(200);

  if(!cs8900_test()) {
    GUIPrint("CS8900 Ethernet Test Failed");
    pass = FALSE;
  }
  else
    GUIPrint("Ethernet Passed");


  cyg_thread_delay(200);
#if 0

  // no pc card on production dharma 2

  if(!pcmciatest()) {
    GUIPrint("PCMCIA/802.11 Test Failed");
    pass = FALSE;
  }
  else
    GUIPrint("PCMCIA Passed");

  cyg_thread_delay(200);
#endif

 
  GUIPrint("Press key S101 to test audio");
  ResetKeys();
  
 
  while(!KeyPressed(0))
    {
      cyg_thread_delay(10);
    }

  GUIPrint("Testing Audio Loopback...");
  _RecordThread();
  

  GUIPrint("Press Upper Right Remote Button");

  ResetKeys(); 
  while(!KeyPressed(18))
  {
	  cyg_thread_delay(10);
  }

  GUIPrint("IR Port Passed");
  
  cyg_thread_delay(100);

  GUIPrint("Testing Keypad");
  
  cyg_thread_delay(100);
  
  for(i = 1; i < 16; i++)
    {
      char buff[64];
   //    diag_printf("Press button %d\n",i+1);

      sprintf(buff,"Press key S1%02d",i+1);
      GUIPrint(buff);

      ResetKeys(); 
      
    
      while(!KeyPressed(i)) {
	cyg_thread_delay(10);
      }
      
    
      
    }
     

  GUIPrint("Keypad Passed");
  
  cyg_thread_delay(100);


  if(pass) {
    GUIPrint("!!!Pass!!!");
  }
  else {
    GUIPrint("!!!Fail!!!");
  }



  while(1);
}


void cyg_user_start(void)
{

  // spawn test_thread
  cyg_thread_create(9, thread_entry, 0, "main thread",
		    (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
  cyg_thread_resume(threadh[0]);

}


