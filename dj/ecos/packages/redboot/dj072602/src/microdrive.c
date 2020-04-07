#include <redboot.h>
#include <microdrive.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>

#define TIMEOUT_5S 7500		/* For microdrive this is actually 7.5S */

#define ATA_MEMCFG_MASK 0x0000ff00

/* Values to play with */
#define MEMCFG_BUS_WIDTH(n)   (n<<0)
#define MEMCFG_BUS_WIDTH_32   (0<<0)
#define MEMCFG_BUS_WIDTH_16   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (2<<0)
#define MEMCFG_WAIT_STATES(n) (n<<2)
#define MEMCFG_SQAEN          (1<<6)
#define MEMCFG_CLKENB         (1<<7)

#define WAIT_STATES 8

#define ATA_8BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_8)<<8)
#define ATA_16BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_16)<<8)

#define ATA_DEVICE_RESET              0x08
#define ATA_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define ATA_IDENTIFY_DEVICE           0xec
#define ATA_MEDIA_EJECT               0xed
#define ATA_IDENTIFY_PACKET_DEVICE    0xa1
#define ATA_PACKET                    0xa0
#define ATA_WRITE_SECTORS             0x30
#define ATA_READ_SECTORS              0x20
#define ATA_CHECK_POWER_MODE          0xe5

#define ATA_DATA_REG        0xf0000010
#define ATA_ERR_REG         0xf0000030
#define ATA_FEATURES_REG    ATA_ERR_REG
#define ATA_SEC_CNT_REG     0xf0000012
#define ATA_SEC_NUM_REG     0xf0000032
#define ATA_CYL_LOW_REG     0xf0000014
#define ATA_CYL_HI_REG      0xf0000034
#define ATA_DEVICE_HEAD_REG 0xf0000016
#define ATA_STATUS_REG      0xf0000036
#define ATA_COMMAND_REG     ATA_STATUS_REG
#define ATA_ALT_STATUS_REG  0xf000000e
#define ATA_DEVICE_CTL_REG  ATA_ALT_STATUS_REG
#define ATA_DRIVE_ADDR_REG  0xf000002e

/* Status Register */
#define STATUS_BSY  0x80
#define STATUS_DRDY 0x40
#define STATUS_DRQ  0x08
#define STATUS_ERR  0x01
#define STATUS_SERV 0x10

/* Drive Select Register */
#define SELECT_MASTER  0x00
#define SELECT_SLAVE 0x10
#define SELECT_LBA 0x40

/* Control Register */
#define CONTROL_IRQ           0x00
#define CONTROL_POLLED        0x02
#define CONTROL_SRST          0x04

/* Error Register */
#define ERROR_WP   0x40
#define ERROR_UNC  0x40
#define ERROR_MC   0x20
#define ERROR_IDNF 0x10
#define ERROR_MCR  0x08
#define ERROR_ABRT 0x04
#define ERROR_NM   0x02

typedef struct
{
    /* Transfer data */
    unsigned short * Data;
    unsigned int DataLength;	/* NOTE DataLength is number of 16 bit words */
    
    /* Packet command data */
    unsigned char * PacketCommand;
    int DataDirection;
    
    /* Task file registers */
    unsigned char Features;
    unsigned char SectorCount;
    unsigned char SectorNumber;
    unsigned char CylinderLow;
    unsigned char CylinderHigh;
    unsigned char DeviceHead;
    unsigned char DeviceControl;
    unsigned char Command;
} ATATaskFile_T;

/* ATA I/O macros */
#define _ReadReg8(reg,var)             \
        var = *((volatile cyg_uint8 *)reg)

#define _WriteReg8(reg,val)            \
        *((volatile cyg_uint8 *)reg) = (val&0xff)

#define _ReadReg16(reg,var)            \
        var = *((volatile cyg_uint16 *)reg)

#define _WriteReg16(reg,val)           \
        *((volatile cyg_uint16 *)reg) = (val&0xffff)

void _ReadDataBulk(cyg_uint16 * Buffer, unsigned int Length);
void _WriteDataBulk(cyg_uint16 * Buffer, unsigned int Length);

#define _ReadError(Variable) _ReadReg8(ATA_ERR_REG,Variable)
#define _ReadSectorCount(Variable) _ReadReg8(ATA_SEC_CNT_REG,Variable)
#define _ReadSectorNumber(Variable) _ReadReg8(ATA_SEC_NUM_REG,Variable)
#define _ReadCylinderLow(Variable) _ReadReg8(ATA_CYL_LOW_REG,Variable)
#define _ReadCylinderHigh(Variable) _ReadReg8(ATA_CYL_HI_REG,Variable)
#define _ReadDeviceHead(Variable) _ReadReg8(ATA_DEVICE_HEAD_REG,Variable)
#define _ReadStatus(Variable) _ReadReg8(ATA_STATUS_REG,Variable)
#define _ReadAlternateStatus(Variable) _ReadReg8(ATA_ALT_STATUS_REG,Variable)

#define _WriteData(Variable) _WriteReg8(ATA_DATA_REG,Variable)
#define _WriteFeatures(Variable) _WriteReg8(ATA_FEATURES_REG,Variable)
#define _WriteSectorCount(Variable) _WriteReg8(ATA_SEC_CNT_REG,Variable)
#define _WriteSectorNumber(Variable) _WriteReg8(ATA_SEC_NUM_REG,Variable)
#define _WriteCylinderLow(Variable) _WriteReg8(ATA_CYL_LOW_REG,Variable)
#define _WriteCylinderHigh(Variable) _WriteReg8(ATA_CYL_HI_REG,Variable)
#define _WriteDeviceHead(Variable) _WriteReg8(ATA_DEVICE_HEAD_REG,Variable)
#define _WriteCommand(Variable) _WriteReg8(ATA_COMMAND_REG,Variable)
#define _WriteDeviceControl(Variable) _WriteReg8(ATA_DEVICE_CTL_REG,Variable)

int ATAHardReset(void);
int ATASoftReset(void);
static int SendCommand(ATATaskFile_T * TaskFile);
static int ProcessInterrupt(ATATaskFile_T * TaskFile);
int PollStatus(unsigned char ClearBits, unsigned char SetBits, unsigned int Timeout);
void ReadDataBulk(cyg_uint16 * Buffer, unsigned int Length);
void WriteDataBulk(cyg_uint16 * Buffer, unsigned int Length);

int
ata_init(void)
{
    static bool Initialized = false;
    int Status = ENOERR;
    
    if (!Initialized) {

	/* Initialize bus */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~ATA_MEMCFG_MASK;
	*(volatile cyg_uint32 *)MEMCFG1 |= ATA_16BIT_MEMCFG;

	/* Power on interface */
	*(volatile cyg_uint8 *)PBDDR |= 0x20;
	*(volatile cyg_uint8 *)PBDR &= ~0x20;

	/* Enable interface */
	*(volatile cyg_uint8 *)PBDDR |= 0x40;
	*(volatile cyg_uint8 *)PBDR &= ~0x40;

#if defined(USE_IRQ)
	/* Set up interrupt handler */
	cyg_semaphore_init(&ATASem, 0);
	cyg_drv_interrupt_create(ATA_IRQ,
				 10,
				 0,
				 (cyg_ISR_t *)_ATAInterruptISR,
				 (cyg_DSR_t *)_ATAInterruptDSR,
				 &_ATAInterruptHandle,
				 &_ATAInterrupt);
	cyg_drv_interrupt_attach(_ATAInterruptHandle);
	cyg_drv_interrupt_acknowledge(ATA_IRQ);
#endif /* USE_IRQ */

	/* Reset the bus */
	Status = ATAHardReset();
	if (Status != ENOERR) {
	    return Status;
	}
	Status = ATASoftReset();
	if (Status != ENOERR) {
	    return Status;
	}

#if defined(USE_IRQ)
	/* Unmask the interrupt */
	cyg_drv_interrupt_unmask(ATA_IRQ);
#endif /* USE_IRQ */

	Initialized = true;
    }

    return Status;
}

int
ata_get_params(ataparams_t * prms)
{
    ATATaskFile_T TaskFile;
    int Status;
    unsigned char TaskFileStatus;
    char tb[DEV_BSIZE];
    
    /* Check prerequisites */
    _ReadStatus(TaskFileStatus);
    if (!(TaskFileStatus & STATUS_DRDY)) {
	return -EAGAIN;
    }
    
    memset(&TaskFile, 0, sizeof(TaskFile));
    TaskFile.Data = (short *)tb;
    TaskFile.DataLength = 256;	/* Number of words */
    TaskFile.DeviceHead = SELECT_MASTER;
    TaskFile.Command = ATA_IDENTIFY_DEVICE;
    Status = SendCommand(&TaskFile);
    if (Status == ENOERR) {
	memcpy(prms, tb, sizeof(ataparams_t));
    }
    return Status;
}

int
ata_write(unsigned int LBA, unsigned short Length, const unsigned char * Data)
{
    ATATaskFile_T TaskFile;
    int Status;
    unsigned char TaskFileStatus;
    unsigned char TaskFileError;

    /* Check prerequisites */
    _ReadStatus(TaskFileStatus);
    if (!(TaskFileStatus & STATUS_DRDY)) {
	return -EAGAIN;
    }
    
    memset(&TaskFile, 0, sizeof(TaskFile));
    TaskFile.Data = (short *)Data;
    TaskFile.DataLength = Length * 256;

    if (Length == 256) {
	TaskFile.SectorCount = 0;
    }
    else {
	TaskFile.SectorCount = Length;
    }
    TaskFile.SectorNumber = LBA & 0xff;
    TaskFile.CylinderLow = (LBA >>= 8) & 0xff;
    TaskFile.CylinderHigh = (LBA >>= 8) & 0xff;
    TaskFile.DeviceHead = SELECT_MASTER | SELECT_LBA | (LBA & 0xf);
    TaskFile.Command = ATA_WRITE_SECTORS;
    Status = SendCommand(&TaskFile);
    if (Status == ENOERR) {
	Status = ENOERR;
    }
    else if (Status == -EIO) {
	_ReadError(TaskFileError);
	if (TaskFileError & ERROR_WP) {
	    printf("%s Media is write protected\n", __FUNCTION__);
	}
	if (TaskFileError & ERROR_MC) {
	    printf("%s Media has changed\n", __FUNCTION__);
	}
	if (TaskFileError & ERROR_IDNF) {
	    printf("%s Address out of range\n", __FUNCTION__);
	}
	if (TaskFileError & ERROR_MCR) {
	    printf("%s Media change request detected\n", __FUNCTION__);
	}
	if (TaskFileError & ERROR_ABRT) {
	    
	    /* Could be either command not supported or address out of range if IDNF is not set */
	    printf("%s Command not supported\n", __FUNCTION__);
	}
	if (TaskFileError & ERROR_NM) {
	    printf("%s No media present\n", __FUNCTION__);
	}
    }
    return Status;
}

/* Mandatory for all devices */
int
ata_read(unsigned int LBA, unsigned short Length, unsigned char * Data)
{
    ATATaskFile_T TaskFile;
    int Status;
    unsigned char TaskFileStatus;
    unsigned char TaskFileError;
    unsigned char TaskFileCylinderHigh, TaskFileCylinderLow;
    
    /* Check prerequisites */
    _ReadStatus(TaskFileStatus);
    if (!(TaskFileStatus & STATUS_DRDY)) {
	printf("ERROR_WAIT_FOR_DEVICE\n");
	return -EAGAIN;
    }
    
    memset(&TaskFile, 0, sizeof(TaskFile));
    TaskFile.Data = (short *)Data;
    TaskFile.DataLength = Length * 256;

    if (Length == 256) {
	TaskFile.SectorCount = 0;
    }
    else {
	TaskFile.SectorCount = Length;
    }
    TaskFile.SectorNumber = LBA & 0xff;
    TaskFile.CylinderLow = (LBA >>= 8) & 0xff;
    TaskFile.CylinderHigh = (LBA >>= 8) & 0xff;
    TaskFile.DeviceHead = SELECT_MASTER | SELECT_LBA | ((LBA >>= 8)& 0x0f);
    TaskFile.Command = ATA_READ_SECTORS;
    Status = SendCommand(&TaskFile);

    _ReadStatus(TaskFileStatus);
    if (!(TaskFileStatus & STATUS_ERR)) {

	/* Command completed succesfully */
	if (!(TaskFileStatus & STATUS_DRDY)) {
	    printf("%s completed succesfully, but DRDY not set\n", __FUNCTION__);
	}
	Status = ENOERR;
    }
    else {

	/* Error occured */
	_ReadError(TaskFileError);
	if (TaskFileError & ERROR_NM) {
	    printf("%s No media present\n", __FUNCTION__);
	}
	else if (TaskFileError & ERROR_ABRT) {

	    /* Check if read was tried on a PACKET device */
	    _ReadCylinderHigh(TaskFileCylinderHigh);
	    _ReadCylinderLow(TaskFileCylinderLow);
	    if (TaskFileCylinderHigh == 0xeb && TaskFileCylinderLow == 0x14) {
		printf("%s Device is packet device\n", __FUNCTION__);
	    }
	    else {
		if (TaskFileError & ERROR_IDNF) {
		    printf("%s Command not supported\n", __FUNCTION__);
		}
		else {
		    printf("%s Address out of range\n", __FUNCTION__);
		}
	    }
	}
	else if (TaskFileError & ERROR_MCR) {
	    printf("%s Media change request\n", __FUNCTION__);
	}
	else if (TaskFileError & ERROR_IDNF) {
	    printf("%s Address out of range\n", __FUNCTION__);
	}
	else if (TaskFileError & ERROR_MC) {
	    printf("%s Media has changed\n", __FUNCTION__);
	}
	else if (TaskFileError & ERROR_UNC) {
	    printf("%s Data uncorrectable\n", __FUNCTION__);
	}
	Status = -EIO;
    }

    return Status;
}

int
ATAHardReset(void)
{
    int Status = ENOERR;
    
    /* Hardware reset protocol */

    /* Assert reset */
    *(volatile cyg_uint8 *)PBDDR |= 0x10;
    *(volatile cyg_uint8 *)PBDR &= ~0x10;

    /* Wait for at least 25 us */
    HAL_DELAY_US(25);
    
    /* Negate reset and wait */
    *(volatile cyg_uint8 *)PBDR |= 0x10;
    
    /* Wait for at least 2 ms */
    HAL_DELAY_US(2000);
    
    /* Check status */
    if (PollStatus(STATUS_BSY, 0, TIMEOUT_5S)) {
	Status = -ETIME;
    }
    else {
	Status = ENOERR;	
    }
    
    return Status;
}

/* Mandatory for both PACKET and non-PACKET devices */
int
ATASoftReset(void)
{
    int Status = ENOERR;
    
    /* Software reset protocol */
    
  sr_set_srst:
#if defined(USE_IRQ)
    _WriteDeviceControl((CONTROL_SRST|CONTROL_IRQ));
#else /* USE_IRQ */
    _WriteDeviceControl((CONTROL_SRST|CONTROL_POLLED));
#endif /* USE_IRQ */
    HAL_DELAY_US(5);
    goto sr_clear_wait;
    
  sr_clear_wait:
#if defined(USE_IRQ)
    _WriteDeviceControl(CONTROL_IRQ);
#else /* USE_IRQ */
    _WriteDeviceControl(CONTROL_POLLED);
#endif /* USE_IRQ */
    HAL_DELAY_US(2000);
    goto sr_check_status;

  sr_check_status:
    if (PollStatus(STATUS_BSY, 0, TIMEOUT_5S)) {
	Status = -ETIME;
    }
    else {
	Status = ENOERR;
    }

    return Status;
}

static int
SendCommand(ATATaskFile_T * TaskFile)
{
    unsigned char DeviceHead;
    
    /* Bus idle protocol */
  bi_check_status:
    if (PollStatus(STATUS_BSY | STATUS_DRQ, 0, TIMEOUT_5S)) {
	return -ETIME;
    }

    _ReadDeviceHead(DeviceHead);
    if ((DeviceHead & 0x10) != (TaskFile->DeviceHead & 0x10)) {
	goto bi_device_select;
    }
    else {
	goto bi_write_parameters;
    }

  bi_device_select:
    _WriteDeviceHead((TaskFile->DeviceHead & 0x10));
    goto bi_check_status;

  bi_write_parameters:
    _WriteDeviceHead(TaskFile->DeviceHead);
    _WriteFeatures(TaskFile->Features);
    _WriteSectorCount(TaskFile->SectorCount);
    _WriteSectorNumber(TaskFile->SectorNumber);
    _WriteCylinderLow(TaskFile->CylinderLow);
    _WriteCylinderHigh(TaskFile->CylinderHigh);
    goto bi_write_command;

  bi_write_command:
    _WriteCommand(TaskFile->Command);
    return ProcessInterrupt(TaskFile);
}

static int
ProcessInterrupt(ATATaskFile_T * TaskFile)
{
    unsigned char Status;
    int Length;

    switch (TaskFile->Command) {
	
	/* PIO data-in command protocol */
	case ATA_IDENTIFY_DEVICE:
	case ATA_READ_SECTORS:
	{
#if defined(USE_IRQ)
	  di_intrq_wait:
	    if (!cyg_semaphore_timed_wait(&ATASem, cyg_current_time() + TIMEOUT_5S)) {
		return -ETIME;
	    }
	    goto di_check_status;
#endif /* USE_IRQ */
	    
	  di_check_status:
	    if (PollStatus(STATUS_BSY, 0, TIMEOUT_5S)) {
		return -ETIME;
	    }
	    _ReadStatus(Status);
	    if (Status & STATUS_DRQ) {
		goto di_transfer_data;
	    }

	    /* Error occured */
	    return -EIO;
	    
	  di_transfer_data:
	    Length = TaskFile->DataLength > 256 ? 256 : TaskFile->DataLength;
	    _ReadDataBulk(TaskFile->Data, Length);
	    TaskFile->Data += Length;
	    TaskFile->DataLength -= Length;
	    if (TaskFile->DataLength) {
#if defined(USE_IRQ)
		goto di_intrq_wait;
#endif /* USE_IRQ */

		/* Wait one PIO cycle before entering next state */
		_ReadAlternateStatus(Status);
		goto di_check_status;
	    }
	    
	    /* Optional read for status register */
	    return ENOERR;
	}

	/* PIO data-out command protocol */
	case ATA_WRITE_SECTORS:
	{
#if defined(USE_IRQ)
	  do_intrq_wait:
	    if (!cyg_semaphore_timed_wait(&ATASem, cyg_current_time() + TIMEOUT_5S)) {
		return -ETIME;
	    }
	    goto do_check_status;
#endif /* USE_IRQ */
	    
	  do_check_status:
	    if (PollStatus(STATUS_BSY, 0, TIMEOUT_5S)) {
		return -ETIME;
	    }
	    _ReadStatus(Status);
	    if (Status & STATUS_DRQ) {
		goto do_transfer_data;
	    }
	    if (Status & STATUS_ERR) {
		
		/* Error occured */
		return -EIO;
	    }
	    return ENOERR;

	  do_transfer_data:
	    Length = TaskFile->DataLength > 256 ? 256 : TaskFile->DataLength;
	    _WriteDataBulk(TaskFile->Data, Length);
	    TaskFile->Data += Length;
	    TaskFile->DataLength -= Length;

#if defined(USE_IRQ)
	    goto di_intrq_wait;
#endif /* USE_IRQ */
	    goto do_check_status;
	}   
    }
}

int
PollStatus(unsigned char ClearBits, unsigned char SetBits, unsigned int Timeout)
{
    cyg_uint8 Status;
    int Ticks = 0;

    Timeout *= 10; // delay is in 100uS.  ms * 10 * 100uS = S

    for (;;) {
	_ReadStatus(Status);
	if (((Status & ClearBits) == 0x00) && ((Status & SetBits) == SetBits)) {
	    break;
	}
	if (++Ticks > Timeout) {
	    return -ETIME;
	}
	CYGACC_CALL_IF_DELAY_US(100);
    }
    return(ENOERR);
}

void
_ReadDataBulk(cyg_uint16 * Buffer, unsigned int Length)
{
    int i;

    for (i = 0; i < Length; ++i) {
	Buffer[i] = *((volatile cyg_uint16 *)ATA_DATA_REG);
    }
}

void
_WriteDataBulk(cyg_uint16 * Buffer, unsigned int Length)
{
    int i;

    for (i = 0; i < Length; ++i) {
	*((volatile cyg_uint16 *)ATA_DATA_REG) = Buffer[i];
    }
}

#if 0

// Microdrive channel structure
static channel_softc_t mem_mod_ch;

//
// ATA commands
//

// Get the disk's parameters
int
ata_get_params(ata_drive_datas_t * drvp, cyg_uint8 flags, ataparams_t * prms)
{
    char tb[DEV_BSIZE];
    ata_command_t ata_c;
    
    memset(tb, 0, DEV_BSIZE);
    memset(prms, 0, sizeof(ataparams_t));
    memset(&ata_c, 0, sizeof(ata_command_t));
    
    ata_c.r_command = ATAC_IDENTIFY;
    ata_c.r_st_bmask = ATAS_DRDY;
    ata_c.r_st_pmask = ATAS_DRQ;
    ata_c.timeout = 1000; /* 1s */
    
    ata_c.flags = AT_READ | flags;
    ata_c.data = tb;
    ata_c.bcount = DEV_BSIZE;
    if (ata_exec_command(drvp, &ata_c) != ATA_COMPLETE) {
	printf("ata_get_parms: ata_exec_command failed\n");
	return CMD_AGAIN;
    }
    if (ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
	printf("ata_get_parms: ata_c.flags=0x%x\n", ata_c.flags);
	return CMD_ERR;
    } else {
	/* Read in parameter block. */
	memcpy(prms, tb, sizeof(ataparams_t));
#if 0
	/*
	 * Shuffle string byte order.
	 * ATAPI Mitsumi and NEC drives don't need this.
	 */
	if ((prms->atap_config & WDC_CFG_ATAPI_MASK) ==
	    WDC_CFG_ATAPI &&
	    ((prms->atap_model[0] == 'N' &&
	      prms->atap_model[1] == 'E') ||
	     (prms->atap_model[0] == 'F' &&
	      prms->atap_model[1] == 'X')))
	    return 0;
	for (i = 0; i < sizeof(prms->atap_model); i += 2) {
	    p = (u_short *)(prms->atap_model + i);
	    *p = ntohs(*p);
	}
	for (i = 0; i < sizeof(prms->atap_serial); i += 2) {
	    p = (u_short *)(prms->atap_serial + i);
	    *p = ntohs(*p);
	}
	for (i = 0; i < sizeof(prms->atap_revision); i += 2) {
	    p = (u_short *)(prms->atap_revision + i);
	    *p = ntohs(*p);
	}
#endif
	return CMD_OK;
    }
}

int
ata_read(ata_drive_datas_t * drvp, unsigned int lba, unsigned short len, unsigned char * data)
{
    ata_command_t ata_c;

    memset(&ata_c, 0, sizeof(ata_c));

    ata_c.r_count = (len == 256) ? 0 : len;
    
    ata_c.r_sector = lba & 0xff;
    ata_c.r_cyl = (lba >> 8) & 0xffff;
    ata_c.r_head = (lba >> 24) & 0x0f;
    ata_c.r_command = ATAC_READ;

    ata_c.r_st_bmask = ATAS_DRDY;
    ata_c.r_st_pmask = ATAS_DRQ;
    ata_c.timeout = MD_CMD_TIMEOUT;
    
    ata_c.flags = AT_READ;	// AT_POLL
    ata_c.data = data;
    ata_c.bcount = len * DEV_BSIZE;

    if (ata_exec_command(drvp, &ata_c) != ATA_COMPLETE) {
	return CMD_AGAIN;
    }
    if (ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
	return CMD_ERR;
    }
    else {
	return CMD_OK;
    }
}

int
ata_write(ata_drive_datas_t * drvp, unsigned int lba, unsigned short len, const unsigned char * data)
{
    ata_command_t ata_c;

    memset(&ata_c, 0, sizeof(ata_c));

    ata_c.r_count = (len == 256) ? 0 : len;
    
    ata_c.r_sector = lba & 0xff;
    ata_c.r_cyl = (lba >> 8) & 0xffff;
    ata_c.r_head = (lba >> 24) & 0x0f;
    ata_c.r_command = ATAC_WRITE;

    ata_c.r_st_bmask = ATAS_DRDY;
    ata_c.r_st_pmask = ATAS_DRQ;
    ata_c.timeout = MD_CMD_TIMEOUT;
    
    ata_c.flags = AT_WRITE;	// AT_POLL
    ata_c.data = data;
    ata_c.bcount = len * DEV_BSIZE;

    if (ata_exec_command(drvp, &ata_c) != ATA_COMPLETE) {
	return CMD_AGAIN;
    }
    if (ata_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
	printf("flags: %x\n", ata_c.flags);
	return CMD_ERR;
    }
    else {
	return CMD_OK;
    }
}

//
// ATA driver
//

ata_drive_datas_t *
ata_init(void)
{
    ata_drive_datas_t * ret;
    
    printf("*** Initialize Disk Image System\n");

    // Initialize bus
    *(volatile cyg_uint32 *)MEMCFG1 &= ~MEM_MOD_MEMCFG_MASK;
    *(volatile cyg_uint32 *)MEMCFG1 |= MEM_MOD_16BIT_MEMCFG;
    *(volatile cyg_uint8 *)PBDDR |= 0x20;
    *(volatile cyg_uint8 *)PBDR &= ~0x20;
    *(volatile cyg_uint8 *)PBDDR |= 0x40;
    *(volatile cyg_uint8 *)PBDR &= ~0x40;
    
    // Populate channel structure for memory module
    memset(&mem_mod_ch, 0, sizeof(mem_mod_ch));
    mem_mod_ch.cmd_ioh = (void *)0xf0000000;
    mem_mod_ch.cmd_r0 = 0x10;
    mem_mod_ch.cmd_r1 = 0x30;
    mem_mod_ch.cmd_r2 = 0x12;
    mem_mod_ch.cmd_r3 = 0x32;
    mem_mod_ch.cmd_r4 = 0x14;
    mem_mod_ch.cmd_r5 = 0x34;
    mem_mod_ch.cmd_r6 = 0x16;
    mem_mod_ch.cmd_r7 = 0x36;
    mem_mod_ch.ctl_ioh = (void *)0xf000000e;
    mem_mod_ch.ctl_r0 = 0x00;
    
    // Populate drive data structure for microdrive on channel
    mem_mod_ch.ch_drive[0].drive = 0;
    mem_mod_ch.ch_drive[0].drive_flags = DRIVE_ATA;
    mem_mod_ch.ch_drive[0].chnl_softc = (void *)&mem_mod_ch;

    if (ata_hard_reset(&mem_mod_ch) != 0) {
	printf("   initialization failed: timed out on hard reset\n");
	ret = 0;
    }
    else {
	if (ataprobe(&mem_mod_ch) == 0) {
	    printf("   ataprobe failed: timed out\n");
	    ret = 0;
	}
	else {
	    ret = &mem_mod_ch.ch_drive[0];
	}
    }
    return ret;
}

int
ata_hard_reset(channel_softc_t * chp)
{
    int stat = 0;
    
    // Assert reset
    *(volatile cyg_uint8 *)PBDDR |= 0x10;
    *(volatile cyg_uint8 *)PBDR &= ~0x10;
    HAL_DELAY_US(25);
    
    // Negate reset and wait
    *(volatile cyg_uint8 *)PBDR |= 0x10;
    HAL_DELAY_US(2000);
    
    // Wait for busy to clear
    if (atawait(chp, ATAS_BSY, 0, MD_CMD_TIMEOUT) != 0) {
	stat = -1;
    }
    return stat;
}

int
ataprobe(channel_softc_t * chp)
{
    cyg_uint8 ret_value = 0x01;
    
    /* assert SRST, wait for reset to complete */
    bus_space_write_1(chp->ctl_ioh, chp->ata_sdh, ATASD_IBM);
    //HAL_DELAY_US(100);
    bus_space_write_1(chp->ctl_ioh, chp->ata_aux_ctlr, ATACTL_RST | ATACTL_IDS);
    HAL_DELAY_US(10000);
    bus_space_write_1(chp->ctl_ioh, chp->ata_aux_ctlr, ATACTL_IDS);
    HAL_DELAY_US(10000);
    (void) bus_space_read_1(chp->cmd_ioh, chp->ata_error);
    bus_space_write_1(chp->ctl_ioh, chp->ata_aux_ctlr, ATACTL_4BIT);
    HAL_DELAY_US(100);
    
    ret_value = __atawait_reset(chp, ret_value);

    /* if reset failed, there's nothing here */
    if (ret_value == 0)
	return 0;
}

int
ata_exec_command(ata_drive_datas_t * drvp, ata_command_t * ata_c)
{
    channel_softc_t * chp = drvp->chnl_softc;
    int ret;
    
    // wdc_exec_xfer(chp, ata_c)
    //  get xfer, put it into queue for chp
    //  wdcstart(chp)
    ata_c->drive = drvp->drive;
	
    __atacommand_start(chp, ata_c);

    if (ata_c->flags & AT_DONE) {
	ret = ATA_COMPLETE;
    }
    else {
	if (ata_c->flags & AT_WAIT) {
	    while ((ata_c->flags & AT_DONE) == 0) {
		//tsleep(wdc_c, PRIBIO, "wdccmd", 0);
	    }
	    ret = ATA_COMPLETE;
	}
	else {
	    ret = ATA_QUEUED;
	}
    }
    return ret;
}

void
atacommand(channel_softc_t * chp, int drive, cyg_uint8 command, cyg_uint16 cylin,
	   cyg_uint8 head, cyg_uint8 sector, cyg_uint8 count, cyg_uint8 features)
{
    printf("atacommand %d: command=0x%x cylin=%d head=%d sector=%d count=%d features=%d\n", drive, command, cylin, head, sector, count, features);
    
    // Select drive, head, and addressing mode
    //bus_space_write_1(chp->cmd_ioh, chp->ata_sdh, ATASD_IBM | (drive << 4) | head);
    bus_space_write_1(chp->cmd_ioh, chp->ata_sdh, ATASD_LBA | (drive << 4) | head);
    // Load parameters
    bus_space_write_1(chp->cmd_ioh, chp->ata_features, features);
    bus_space_write_1(chp->cmd_ioh, chp->ata_cyl_lo, cylin);
    bus_space_write_1(chp->cmd_ioh, chp->ata_cyl_hi, cylin >> 8);
    bus_space_write_1(chp->cmd_ioh, chp->ata_sector, sector);
    bus_space_write_1(chp->cmd_ioh, chp->ata_seccnt, count);

    // Send command
    bus_space_write_1(chp->cmd_ioh, chp->ata_command, command);
    return;
}

int
__atawait_reset(channel_softc_t *chp, int drv_mask)
{
    int timeout;
    cyg_uint8 st0;

    /* wait for BSY to deassert (31s) */
    for (timeout = 0; timeout < 3100000; timeout++) {
	bus_space_write_1(chp->cmd_ioh, chp->ata_sdh, ATASD_IBM); /* master */
	HAL_DELAY_US(100);
	st0 = bus_space_read_1(chp->cmd_ioh, chp->ata_status);
	
	if ((drv_mask & 0x01) != 0 && (st0 & ATAS_BSY) == 0) {
	    /* No slave, master is ready, it's done */
	    goto end;
	}
	HAL_DELAY_US(100);
    }
    /* Reset timed out. Maybe it's because drv_mask was not rigth */
    if (st0 & ATAS_BSY)
	drv_mask &= ~0x01;
  end:
    
    return drv_mask;
}

// return 0 ok, -1 on timeout
int
atawait(channel_softc_t * chp, int mask, int bits, int timeout)
{
    cyg_uint8 status;
    int ticks = 0;

    timeout *= 10; // delay is in 100uS.  ms * 10 * 100uS = S
    
    for (;;) {
	chp->ch_status = status = bus_space_read_1(chp->cmd_ioh, chp->ata_status);
	if ((status & ATAS_BSY) == 0 && (status & mask) == bits) {
	    break;
	}
	if (++ticks > timeout) {
	    return -1;
	}
	CYGACC_CALL_IF_DELAY_US(100);
    }
    if (status & ATAS_ERR) {
	chp->ch_error = bus_space_read_1(chp->cmd_ioh, chp->ata_error);
    }
    return 0;
}

void
__atacommand_start(channel_softc_t * chp, ata_command_t * ata_c)
{
    int drive = ata_c->drive;
    
    bus_space_write_1(chp->cmd_ioh, chp->ata_sdh, ATASD_IBM | (drive << 4));
    if (atawait(chp, ata_c->r_st_bmask | ATAS_DRQ, ata_c->r_st_bmask, ata_c->timeout) != 0) {
	ata_c->flags |= AT_TIMEOU;
	__atacommand_done(chp, ata_c);
    }
    atacommand(chp, drive, ata_c->r_command, ata_c->r_cyl, ata_c->r_head,
	       ata_c->r_sector, ata_c->r_count, ata_c->r_features);
#if 0
    if ((ata_c->flags & AT_POLL) == 0) {
	chp->ch_flags |= ATAF_IRQ_WAIT;
	callout_reset(&chp->ch_callout, ata_c->timeout / 1000 * hz,
		      atatimeout, chp);
	return;
    }
#endif
    // Polled command
    // Wait at least 400ns for status bit to be valid.
    HAL_DELAY_US(1);
    __atacommand_intr(chp, ata_c, 0);
}

// Return 1 if interrupt handled, 0 if not
int
__atacommand_intr(channel_softc_t * chp, ata_command_t * ata_c, int irq)
{
    int ata_bcount = ata_c->bcount;
    int bcount;
    char * data = ata_c->data;
    
    while (ata_bcount) {
	if (atawait(chp, ata_c->r_st_pmask, ata_c->r_st_pmask,
		    (irq == 0) ? ata_c->timeout : 0)) {
	    ata_c->flags |= AT_TIMEOU;
	    __atacommand_done(chp, ata_c);
	    return 1;
	}
	
	bcount = ata_bcount > 512 ? 512 : ata_bcount;
	if (ata_c->flags & AT_READ) {
	    if (bcount > 0) {
		bus_space_read_multi_2(chp->cmd_ioh, chp->ata_data, (cyg_uint16 *)data, bcount >> 1);
	    }
	}
	else if (ata_c->flags & AT_WRITE) {
	    if (bcount > 0) {
		bus_space_write_multi_2(chp->cmd_ioh, chp->ata_data, (cyg_uint16 *)data, bcount >> 1);
	    }
	}
	data += bcount;
	ata_bcount -= bcount;
    }
    __atacommand_done(chp, ata_c);
    return 1;
}

void
__atacommand_done(channel_softc_t * chp, ata_command_t * ata_c)
{
    if (chp->ch_status & ATAS_DWF) {
	ata_c->flags |= AT_DF;
    }
    if (chp->ch_status & ATAS_ERR) {
	ata_c->flags |= AT_ERROR;
	ata_c->r_error = chp->ch_error;
    }
    ata_c->flags |= AT_DONE;
    return;
}

//
// Bus access functions
//

cyg_uint8
bus_space_read_1(volatile cyg_uint8 * ioh, int off)
{
    return *(ioh + off);
}

void
bus_space_write_1(volatile cyg_uint8 * ioh, int off, cyg_uint8 val)
{
    *(ioh + off) = val;
}

void
bus_space_read_multi_2(volatile cyg_uint16 * ioh, int off, cyg_uint16 * data, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
	data[i] = *(ioh + (off >> 1));
    }
}

void
bus_space_write_multi_2(volatile cyg_uint16 * ioh, int off, cyg_uint16 * data, int count)
{
    int i;
    int j;
    printf("ioh: %x off %x data %x count %x\n", ioh, off, data, count);
    for (i = 0; i < count * 2; ++i) {
	//*(ioh + (off > 1)) = data[i];
	*(cyg_uint16 *)0xf0000010 = data[i];
    }
}

#endif
