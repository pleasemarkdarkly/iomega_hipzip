// ataproto.h: protocol interface for ata
// danc@iobjects.com
// (c) Interactive Objects

#ifndef __ATAPROTO_H__
#define __ATAPROTO_H__

#ifndef DADIO_BOOT
#include <cyg/kernel/kapi.h>
#endif

#include <io/storage/blk_dev.h>

/* ATA commands */
#define ATA_DEVICE_RESET 0x08
#define ATA_READ       0x20
#define ATA_WRITE      0x30
#define ATA_SLEEP      0xe6
#define ATA_IDENTIFY   0xec
#define ATA_READMULTI  0xc4
#define ATA_WRITEMULTI 0xc5
#define ATA_SETMULTI   0xc6
#define ATA_SET_FEATURES 0xef

/* ATAPI commands */
#define ATAPI_SOFT_RESET 0x08
#define ATAPI_PACKET   0xa0
#define ATAPI_IDENTIFY 0xa1

/* SCSI commands */
#define SCSI_READ10 0x28
#define SCSI_WRITE10 0x2a

/* Command status values */
#define COMPLETE 0
#define QUEUED   1

/* Timeout values */
#define MICRODRIVE_TIMEOUT 750 /* 7.5s */
#define ATA_DELAY MICRODRIVE_TIMEOUT
#define ATAPI_DELAY 10 /* 100ms */
#define ATA_MAX_RETRIES 5

/* Register bits */
#define STATUS_BSY  0x80
#define STATUS_DRDY 0x40
#define STATUS_DWF  0x20
#define STATUS_DRQ  0x08
#define STATUS_CORR 0x04
#define STATUS_ERR  0x01

#define CONTROL_IRQ    0x00
#define CONTROL_POLLED 0x02
#define CONTROL_SRST   0x04

#define DEVICE_LBA     0x40

#define ERROR_BBK   0x80	/* Bad block */
#define ERROR_UNC   0x40	/* Uncorrectable data error */
#define ERROR_IDNF  0x10	/* Id not found */
#define ERROR_ABRT  0x04        /* Aborted command */
#define ERROR_TK0NF 0x02	/* Track 0 not found */
#define ERROR_AMNF  0x01	/* Address mark not found */

#define IRSN_CMD    0x01
#define IRSN_IN     0x02
#define IRSN_RELEASE 0x04

#define PHASE_CMDOUT    (STATUS_DRQ | IRSN_CMD)  
#define PHASE_DATAIN    (STATUS_DRQ | IRSN_IN)
#define PHASE_DATAOUT   STATUS_DRQ
#define PHASE_COMPLETED (IRSN_IN | IRSN_CMD)
#define PHASE_ABORTED   0

typedef struct BusSpace_S BusSpace_T;

/* Parition information */
typedef struct
{
    int SectorSize;
} Partition_T;

/* Device specific information */
typedef struct
{
    unsigned char Drive;	/* 0 - Master, 1 - Slave */
    unsigned char Multi;	/* Maximum number of blocks per interrupt for ATA */

    unsigned short Config;
#define ATAPI_CFG_DRQ_MASK 0x0060
#define ATAPI_CFG_IRQ_DRQ  0x0020
    
    struct BusSpace_S * Bus;
    Partition_T * Partition;
    
    unsigned short Cylinders;
    unsigned short Heads;
    unsigned short SectorsPerTrack;
    unsigned int TotalSectors;
    
    char SerialNumber[40];
    char ModelNumber[40];

    int MediaPresent;
    Cyg_ErrNo MediaStatus;
} ATADeviceInfo_T;

/* Transfer on bus. Essentially a subclass for ATAPI and ATA transfers.
   Also allows for queuing of transfers. */
typedef struct Transfer_S
{
    unsigned char Drive;
    unsigned short Flags;
#define XFER_POLL 0x0001
#define XFER_TIMEOUT 0x0002
#define XFER_SENSE 0x0004
#define XFER_ATAPI 0x0008
    void * Cmd;
    void * Data;
    int ByteCount;
    int Skip;
    void (*Start)(BusSpace_T * Bus, struct Transfer_S * Transfer);
    void (*Interrupt)(BusSpace_T * Bus, struct Transfer_S * Transfer);
    void (*KillTransfer)(BusSpace_T * Bus, struct Transfer_S * Transfer);
} Transfer_T;

/* Block IO command */
typedef struct 
{
    unsigned short Flags;
#define ATA_BIO_POLL      0x0002
#define ATA_BIO_DONE      0x0004
#define ATA_BIO_SINGLE    0x0008 /* Each block is a seperate command */
#define ATA_BIO_LBA       0x0010
#define ATA_BIO_READ      0x0020
#define ATA_BIO_CORRECTED 0x0040
#define ATA_BIO_RESET     0x0080 /* Reset and retry the operation */
    int Multi;			/* For multi block transfers */
    unsigned int BlkNum;
    unsigned int ByteCount;
    unsigned int BlkDone;	/* Number of blocks transferred */
    unsigned int NumBlks;	/* Current number of blocks transferred */
    unsigned int NumBytes;	/* Current number of bytes transferred */
    char * Data;
    int Error;
#define ATA_BIO_NO_ERROR 0
#define ATA_BIO_ERROR    1	/* Check ErrorReg */
#define ATA_BIO_DF       2
#define ATA_BIO_TIMEOUT  3
    unsigned char ErrorReg;	/* Copy of the error register */
    Partition_T * Partition;
} ATABlockIO_T;

#define SCSI_REQUEST_SENSE 0x03
typedef struct
{
    unsigned char Opcode;
    unsigned char Byte2;
    unsigned char Unused[2];
    unsigned char Length;
    unsigned char Control;
} SCSIRequestSense_T;


// Functions

void ATASoftReset(BusSpace_T * Bus);
int ATAAttach(ATADeviceInfo_T * DeviceInfo);
int ATAExecuteCommand(ATADeviceInfo_T * DeviceInfo, ATACommand_T * Cmd);

void ATACommand(BusSpace_T * Bus, unsigned char Drive, unsigned char Command,
		       unsigned short Cylinder, unsigned char Head, unsigned char SectorNumber,
		       unsigned char SectorCount, unsigned char Features);
int ATAWait(BusSpace_T * Bus, int Mask, int Bits, int Timeout);
#define WaitForDRQ(Bus, Timeout) ATAWait((Bus), STATUS_DRQ, STATUS_DRQ, (Timeout))
#define WaitForUnbusy(Bus, Timeout) ATAWait((Bus), 0, 0, (Timeout))
#define WaitForReady(Bus, Timeout) ATAWait((Bus), STATUS_DRDY, STATUS_DRDY, (Timeout))
int ATAError(BusSpace_T * Bus, ATABlockIO_T * Cmd);

void ATACommandStart(BusSpace_T * Bus, Transfer_T * Transfer);
void ATACommandInterrupt(BusSpace_T * Bus, Transfer_T * Transfer);
void ATACommandDone(BusSpace_T * Bus, Transfer_T * Transfer);

int ATABlockIO(ATADeviceInfo_T * DeviceInfo, ATABlockIO_T * Cmd);
void ATABlockIOStart(BusSpace_T * Bus, Transfer_T * Transfer);
void ATABlockIOInterrupt(BusSpace_T * Bus, Transfer_T * Transfer);
void ATABlockIODone(BusSpace_T * Bus, Transfer_T * Transfer);

int ATAPIAttach(ATADeviceInfo_T * DeviceInfo);
int ATAPIExecuteCommand(ATADeviceInfo_T * DeviceInfo, ATAPICommand_T * Cmd);
void ATAPIReset(BusSpace_T * Bus, Transfer_T * Transfer);

void ATAPICommandStart(BusSpace_T * Bus, Transfer_T * Transfer);
void ATAPICommandInterrupt(BusSpace_T * Bus, Transfer_T * Transfer);
void ATAPICommandDone(BusSpace_T * Bus, Transfer_T * Transfer);

#endif /* __ATAPROTO_H__ */
