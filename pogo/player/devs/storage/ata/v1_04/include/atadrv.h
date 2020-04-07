// atadrv.h: proprietary IOCTLs and interface for ata and atapi drives
// danc@iobjects.com
// (c) Interactive Objects

#ifndef __ATADRV_H__
#define __ATADRV_H__

/* IOCTLs */
#define IO_ATA_SET_CONFIG_POWER_DOWN   0x0390
#define IO_ATA_SET_CONFIG_POWER_UP     0x0391
#define IO_ATA_SET_CONFIG_EXEC_COMMAND 0x0392
#define IO_ATA_SET_CONFIG_FEATURES     0x0393
#define IO_ATAPI_GET_POWER_STATUS		0x0394
#define IO_ATAPI_SET_CONFIG_POWER_DOWN   0x03A0
#define IO_ATAPI_SET_CONFIG_POWER_UP     0x03A1
#define IO_ATAPI_SET_CONFIG_EXEC_COMMAND 0x03A2
#define IO_ATAPI_SET_CONFIG_TRAY_OPEN    0x03A3
#define IO_ATAPI_SET_CONFIG_TRAY_CLOSE   0x03A4
#define IO_ATAPI_SET_CONFIG_FEATURES     0x03A5

#define IO_ATA_SET_CONFIG_BUS_LOCK       0x03B0
#define IO_ATA_SET_CONFIG_BUS_UNLOCK     0x03B1

typedef struct
{
    unsigned char Opcode;
    unsigned char Bytes[15];
} SCSICommand_T;

typedef struct
{
    unsigned char ErrorCode;
#define	SSD_ERRCODE	0x7F
#define	SSD_ERRCODE_VALID 0x80
    unsigned char Segment;
    unsigned char Flags;
#define	SSD_KEY		0x0F
#define	SSD_ILI		0x20
#define	SSD_EOM		0x40
#define	SSD_FILEMARK	0x80
    unsigned char Info[4];
    unsigned char ExtraLength;
    unsigned char CmdSpecInfo[4];
    unsigned char ASC;
    unsigned char ASCQ;
    unsigned char FRU;
    unsigned char SenseKeySpecific1;
#define	SSD_SCS_VALID	0x80
    unsigned char SenseKeySpecific2;
    unsigned char SenseKeySpecific3;
    unsigned char SenseKeySpecific4;
    unsigned char ExtraBytes[14];
} SCSISenseData_T;


/* ATAPI command */
typedef struct
{
    unsigned short Flags;
#define ATAPI_DONE 0x0002
#define ATAPI_POLL 0x0004
#define ATAPI_DATA_OUT 0x0008
#define ATAPI_DATA_IN 0x0010
#define ATAPI_AUTO_SENSE 0x0020
    char * Data;
    int DataLength;
    int Residue;
    int Error;
#define ATAPI_NO_ERROR 0
#define ATAPI_TIMEOUT 1
#define ATAPI_RESET 2

#define ATAPI_SENSE 3
#define ATAPI_SHORT_SENSE 4
    int Timeout;
    
    SCSICommand_T * SCSICmd;
    int SCSICmdLength;
    unsigned int ATAPISense;
    SCSISenseData_T SCSISenseData __attribute__((aligned (4)));
    SCSICommand_T _SCSICmd __attribute__((aligned (4)));
} ATAPICommand_T;


/* ATA Command */
typedef struct 
{
    /* Registers */
    unsigned char Command;
    unsigned char Head;
    unsigned short Cylinder;
    unsigned char SectorNumber;
    unsigned char SectorCount;
    unsigned char Features;
    unsigned char DeviceHead;
    
    /* Status register masks */
    unsigned char StatusBefore;	/* Before command */
    unsigned char StatusAfter;	/* After command */
    
    unsigned char Error;	/* Error register after command */
    unsigned short Flags;
#define ATA_CMD_READ     0x0001 /* There is data to read */
#define ATA_CMD_WRITE    0x0002 /* There is data to write (excl. with ATA_CMD_READ) */
#define ATA_CMD_WAIT     0x0008 /* Wait in controller code for command completion */
#define ATA_CMD_POLL     0x0010 /* Poll for command completion (no interrupts) */
#define ATA_CMD_DONE     0x0020 /* Command is done */
#define ATA_CMD_ERROR    0x0040 /* Command is done with error */
#define ATA_CMD_TIMEOUT  0x0080 /* Command timed out */
#define ATA_CMD_READ_REG 0x0200 /* Read registers on completion */
    int Timeout;
    void * Data;
    int ByteCount;
} ATACommand_T;


#endif /* __ATADRV_H__ */
