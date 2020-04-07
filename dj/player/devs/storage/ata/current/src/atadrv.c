#include <pkgconf/system.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/io/devtab.h>

#include <devs/storage/ata/_ata_hw.h>
#include <devs/storage/ata/_ata_dev.h>

#include <devs/storage/ata/atadrv.h>
#include "atabus.h"
#include "busctrl.h"

#include <string.h>  // memset, memcpy

#include <util/debug/debug.h>

//DEBUG_MODULE(ATA);
DEBUG_MODULE_S(ATA, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(ATA);

/* ATA devio */
static Cyg_ErrNo _ATAWrite(cyg_io_handle_t Handle, const void * Buffer, cyg_uint32 * Length, cyg_uint32 Block);
static Cyg_ErrNo _ATARead(cyg_io_handle_t Handle, void * Buffer, cyg_uint32 * Length, cyg_uint32 Block);
static cyg_bool _ATASelect(cyg_io_handle_t Handle, cyg_uint32 Which, CYG_ADDRWORD Info);
static Cyg_ErrNo _ATAGetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, void * Buffer, cyg_uint32 * Length);
static Cyg_ErrNo _ATASetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, const void * Buffer, cyg_uint32 * Length);

BLOCK_DEVIO_TABLE(ATADevio,
                  _ATAWrite,
                  _ATARead,
                  _ATASelect,
                  _ATAGetConfig,
                  _ATASetConfig
    );

/* ATAPI devio */
static Cyg_ErrNo _ATAPIWrite(cyg_io_handle_t Handle, const void * Buffer, cyg_uint32 * Length, cyg_uint32 Block);
static Cyg_ErrNo _ATAPIRead(cyg_io_handle_t Handle, void * Buffer, cyg_uint32 * Length, cyg_uint32 Block);
static cyg_bool _ATAPISelect(cyg_io_handle_t Handle, cyg_uint32 Which, CYG_ADDRWORD Info);
static Cyg_ErrNo _ATAPIGetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, void * Buffer, cyg_uint32 * Length);
static Cyg_ErrNo _ATAPISetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, const void * Buffer, cyg_uint32 * Length);

BLOCK_DEVIO_TABLE(ATAPIDevio,
                  _ATAPIWrite,
                  _ATAPIRead,
                  _ATAPISelect,
                  _ATAPIGetConfig,
                  _ATAPISetConfig
    );

static bool _ATAInit(struct cyg_devtab_entry * Entry);
static Cyg_ErrNo _ATALookup(struct cyg_devtab_entry ** Entry,
                            struct cyg_devtab_entry * SubEntry,
                            const char * Name);
static bool _ATAPIInit(struct cyg_devtab_entry * Entry);
static Cyg_ErrNo _ATAPILookup(struct cyg_devtab_entry ** Entry,
                              struct cyg_devtab_entry * SubEntry,
                              const char * Name);

static Cyg_ErrNo _GetMediaStatus(BusSpace_T * Bus, unsigned char Drive);

#if defined(HW_ATA_BUS_0_USE_CD_IRQ) || defined(HW_ATA_BUS_1_USE_CD_IRQ)
static cyg_uint32 ata_media_change_isr( cyg_vector_t, cyg_addrword_t );
static void ata_media_change_dsr( cyg_vector_t, cyg_ucount32, cyg_addrword_t );
static void _RegisterIRQ( BusSpace_T* Bus );

#if defined(HW_ATA_BUS_0_USE_CD_IRQ)
static cyg_interrupt bus0_interrupt_data;
static cyg_handle_t  bus0_interrupt_handle;
#endif // BUS_0_USE_CD_IRQ
#if defined(HW_ATA_BUS_1_USE_CD_IRQ)
static cyg_interrupt bus1_interrupt_data;
static cyg_handle_t  bus1_interrupt_handle;
#endif // BUS_1_USE_CD_IRQ

#endif // BUS_0_USE_CD_IRQ || BUS_1_USE_CD_IRQ

// Map the devtab entries to ATA/ATAPI macros
// The added bonus is for older kernels that dont have BLOCK_DEVTAB_ENTRY, the appropriate
// macro can be substituted here
#define ATA_BLOCK_DEVTAB_ENTRY(v_name,name,arg,priv)  \
		BLOCK_DEVTAB_ENTRY(v_name,name,arg, &ATADevio, _ATAInit, _ATALookup, priv)
#define ATAPI_BLOCK_DEVTAB_ENTRY(v_name,name,arg,priv) \
		BLOCK_DEVTAB_ENTRY(v_name,name,arg, &ATAPIDevio, _ATAPIInit, _ATAPILookup, priv)


//
// For each physical device, generate a devtab entry
// The goal is to use the appropriate macro based on whether the
// device is ATA or atapi
//

#if defined(HW_ATA_ENABLE_BUS_0)
extern BusSpace_T Bus0;
#else
#undef HW_ATA_BUS_0_MASTER
#undef HW_ATA_BUS_0_SLAVE
#endif   // HW_ATA_ENABLE_BUS_0

#if defined(HW_ATA_ENABLE_BUS_1)
extern BusSpace_T Bus1;
#else
#undef HW_ATA_BUS_1_MASTER
#undef HW_ATA_BUS_1_SLAVE
#endif   // HW_ATA_ENABLE_BUS_1


#if defined(HW_ATA_BUS_0_MASTER)
#if defined(HW_ATA_BUS_0_MASTER_ATAPI)
#define BUS0_MASTER_DEVTAB  ATAPI_BLOCK_DEVTAB_ENTRY
#else
#define BUS0_MASTER_DEVTAB  ATA_BLOCK_DEVTAB_ENTRY
#endif   // HW_ATA_BUS_0_MASTER_ATAPI

BUS0_MASTER_DEVTAB( Bus_0_Master, HW_ATA_BUS_0_MASTER_NAME ,0,&Bus0.Device[0] );

#endif   // HW_ATA_BUS_0_MASTER

#if defined(HW_ATA_BUS_0_SLAVE)
#if defined(HW_ATA_BUS_0_SLAVE_ATAPI)
#define BUS0_SLAVE_DEVTAB   ATAPI_BLOCK_DEVTAB_ENTRY
#else
#define BUS0_SLAVE_DEVTAB   ATA_BLOCK_DEVTAB_ENTRY
#endif   // HW_ATA_BUS_0_SLAVE_ATAPI

BUS0_SLAVE_DEVTAB( Bus_0_Slave, HW_ATA_BUS_0_SLAVE_NAME ,0,&Bus0.Device[1] );

#endif   // HW_ATA_BUS_0_SLAVE


#if defined(HW_ATA_BUS_1_MASTER)
#if defined(HW_ATA_BUS_1_MASTER_ATAPI)
#define BUS1_MASTER_DEVTAB   ATAPI_BLOCK_DEVTAB_ENTRY
#else
#define BUS1_MASTER_DEVTAB   ATA_BLOCK_DEVTAB_ENTRY
#endif   // HW_ATA_BUS_1_MASTER_ATAPI

BUS1_MASTER_DEVTAB( Bus_1_Master, HW_ATA_BUS_1_MASTER_NAME,0,&Bus1.Device[0] );

#endif   // HW_ATA_BUS_0_MASTER

#if defined(HW_ATA_BUS_1_SLAVE)
#if defined(HW_ATA_BUS_1_SLAVE_ATAPI)
#define BUS1_SLAVE_DEVTAB   ATAPI_BLOCK_DEVTAB_ENTRY
#else
#define BUS1_SLAVE_DEVTAB   ATA_BLOCK_DEVTAB_ENTRY
#endif   // HW_ATA_BUS_1_SLAVE_ATAPI

BUS1_SLAVE_DEVTAB( Bus_1_Slave, HW_ATA_BUS_1_SLAVE_NAME,0,&Bus1.Device[1] );

#endif   // HW_ATA_BUS_1_SLAVE


//////////////////////////////////////////////////////
//
// ATA Functions
//
//////////////////////////////////////////////////////


static Cyg_ErrNo
_ATAWrite(cyg_io_handle_t Handle, const void * Buffer, cyg_uint32 * Length, cyg_uint32 Block)
{
    Cyg_ErrNo Status;
    ATABlockIO_T Cmd;
    ATADeviceInfo_T * DeviceInfo;

    DBTR(ATA);
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;

    Status = _GetMediaStatus(DeviceInfo->Bus, DeviceInfo->Drive);
    if (Status == -EMEDCHG) {
        ATAAttach(DeviceInfo);
    }
    if (Status != ENOERR) {
        return Status;
    }
    
    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags |= (ATA_BIO_LBA | ATA_CMD_READ_REG);
    Cmd.BlkNum = Block;
    Cmd.ByteCount = *Length; // * (DeviceInfo->Partition->SectorSize);
    Cmd.Data = (char *)Buffer;
    
    Status = ATABlockIO(DeviceInfo, &Cmd);
    /* TODO Command queueing not supported, so this block is ignored. */
    if (Status == QUEUED) {
    }
    switch (Cmd.Error) {
        case ATA_BIO_NO_ERROR: Status = ENOERR; break;
        case ATA_BIO_ERROR:    Status = -EIO;   break;
        case ATA_BIO_DF:       Status = -EIO;   break;
        case ATA_BIO_TIMEOUT:  Status = -ETIME; break;
        default:               Status = -EIO;   break;
    }
    return Status;
}

static Cyg_ErrNo
_ATARead(cyg_io_handle_t Handle, void * Buffer, cyg_uint32 * Length, cyg_uint32 Block)
{
    Cyg_ErrNo Status;
    ATABlockIO_T Cmd;
    ATADeviceInfo_T * DeviceInfo;

    DBTR(ATA);

    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;

    Status = _GetMediaStatus(DeviceInfo->Bus, DeviceInfo->Drive);
    if (Status == -EMEDCHG) {
        ATAAttach(DeviceInfo);
    }
    if (Status != ENOERR) {
        return Status;
    }
    
    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags |= (ATA_BIO_LBA | ATA_BIO_READ | ATA_CMD_READ_REG);
    Cmd.BlkNum = Block;
    Cmd.ByteCount = *Length;
    Cmd.Data = (char *)Buffer;
    
    Status = ATABlockIO(DeviceInfo, &Cmd);
    /* TODO Command queueing not supported, so this block is ignored. */
    if (Status == QUEUED) {
    }
    switch (Cmd.Error) {
        case ATA_BIO_NO_ERROR: Status = ENOERR; break;
        case ATA_BIO_ERROR:    Status = -EIO;   break;
        case ATA_BIO_DF:       Status = -EIO;   break;
        case ATA_BIO_TIMEOUT:  Status = -ETIME; break;
        default:               Status = -EIO;   break;
    }
    return Status;
}

static cyg_bool
_ATASelect(cyg_io_handle_t Handle, cyg_uint32 Which, CYG_ADDRWORD Info)
{
    DBTR(ATA);
    
    return true;
}

static Cyg_ErrNo
_ATAGetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, void * Buffer, cyg_uint32 * Length)
{
    Cyg_ErrNo Status;
    ATADeviceInfo_T * DeviceInfo;

    DBTR(ATA);

    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;

    switch (Key) {
        case IO_BLK_GET_CONFIG_MEDIA_STATUS:
        {
            Status = _GetMediaStatus(DeviceInfo->Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }
            break;
        }

        case IO_BLK_GET_CONFIG_GEOMETRY:
        {
            drive_geometry_t * Geometry;

            Status = _GetMediaStatus(DeviceInfo->Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }
            if (Status != ENOERR) {
                return Status;
            }
	    
            /* TODO Possible move this info into the parition structure */
            Geometry = (drive_geometry_t *)Buffer;
            Geometry->cyl = DeviceInfo->Cylinders;
            Geometry->hd = DeviceInfo->Heads;
            Geometry->sec = DeviceInfo->SectorsPerTrack;
            Geometry->bytes_p_sec = DeviceInfo->Partition->SectorSize;
            Geometry->num_blks = DeviceInfo->TotalSectors;
            
            Geometry->serial_len = 21;
            memcpy(Geometry->serial_num, DeviceInfo->SerialNumber, 21);
     
            Geometry->model_len  = 41;
            memcpy(Geometry->model_num, DeviceInfo->ModelNumber, 41);     

            Status = ENOERR;
            break;
        }
	
        default:
        {
            Status = -ENOSYS;
            break;
        }
    }
    return Status;
}

static Cyg_ErrNo
_ATASetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, const void * Buffer, cyg_uint32 * Length)
{
    Cyg_ErrNo Status = -ENOSYS;	/* Make the compiler happy */
    ATADeviceInfo_T * DeviceInfo;
    ATACommand_T Cmd;
    BusSpace_T * Bus;
    static int RefCnt = 0;
    
    DBTR(ATA);
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;
    Bus = DeviceInfo->Bus;
    
    switch (Key) {
        case IO_BLK_SET_CONFIG_MEDIA_STATUS_CB:
        {
#ifndef NOKERNEL
            Bus->MediaChangeCB = (void(*)(int,int,Cyg_ErrNo))((unsigned int*)Buffer);
#endif /*NOKERNEL*/
            Status = ENOERR;
            break;
        }
        case IO_ATA_SET_CONFIG_BUS_LOCK:
        {
            BusLock( Bus );
            Status = ENOERR;
            break;
        }
        case IO_ATA_SET_CONFIG_BUS_UNLOCK:
        {
            BusUnlock( Bus );
            Status = ENOERR;
            break;
        }
        case IO_BLK_SET_CONFIG_EXEC_COMMAND: 
        {
            ATACommand_T * Cmd = (ATACommand_T *)Buffer;

            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }
            if (Status != ENOERR) {
                return Status;
            }
	    
            if (*Length == sizeof(ATACommand_T)) {
                /* TODO Check return value and support queuing */
                ATAExecuteCommand(DeviceInfo, Cmd);
                if (Cmd->Flags & ATA_CMD_ERROR) {
                    Status = -EIO;
                }
                else if (Cmd->Flags & ATA_CMD_TIMEOUT) {
                    Status = -ETIME;
                }
                else {
                    Status = ENOERR;
                }
            }
            else {
                Status = -EINVAL;
            }
            break;
        }

        case IO_ATA_SET_CONFIG_FEATURES:
        {
#ifdef PIOMODE
			diag_printf("PIO ATAPI\n");
			// set transfer mode
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x03;   /* set transfer mode */
			Cmd.SectorCount = 0x0B; /* PIO mode 3 */
         
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if(Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
                return Status;
            } else {
                Status = ENOERR;
            }
#endif


			// write caching
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x02;   /* Enable write cache */

            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if(Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
                return Status;
            } else {
                Status = ENOERR;
            }

			// AAM feature set
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x42;   /* Enable AAM feature set */
            Cmd.SectorCount = 0x80; /* quietest possible setting */
            ATAExecuteCommand(DeviceInfo, &Cmd);          
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if(Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
                return Status;
            } else {
                Status = ENOERR;
            }
                
			Status = ENOERR;            

#if 0
			// read lookahead
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0xaa;   /* Enable read lookahead */
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if(Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
                return Status;
            } else {
                Status = ENOERR;
            }
#endif

            break;


        }
        case IO_BLK_SET_CONFIG_RESET:
        {
            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status != ENOMED) {
                Bus->IsReset = 0;
                ATAAttach(DeviceInfo);
                /* TODO Check the status here for real */
            }
            break;
        }

		//
		//	sleep/wakeup notes:
		//	use standby and idle commands to switch
		//  sleep requires a reset with side effects (+time)		


        case IO_BLK_SET_CONFIG_SLEEP:
        {

            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }

            if (Status == ENOERR) {
                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = 0xE0; // sleep
                Cmd.StatusBefore = STATUS_DRDY;
                Cmd.StatusAfter = STATUS_DRDY;
                Cmd.Timeout = ATA_DELAY;
                ATAExecuteCommand(DeviceInfo, &Cmd);
                /* TODO Check the status here for real */
                /* TODO Mask the interrupt */
                Status = ENOERR;
            }

            break;
        }
	
        case IO_BLK_SET_CONFIG_WAKEUP:
        {


            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }

            if (Status == ENOERR) {
                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = 0xE3; // set idle mode
                Cmd.StatusBefore = STATUS_DRDY;
                Cmd.StatusAfter = STATUS_DRDY;
                Cmd.Timeout = ATA_DELAY;
                ATAExecuteCommand(DeviceInfo, &Cmd);
                /* TODO Check the status here for real */
                /* TODO Mask the interrupt */
                Status = ENOERR;
            }

            break;
        }

        case IO_BLK_SET_CONFIG_POWER_DOWN:
        {
            Bus->PowerDown( Bus );
            Status = ENOERR;
            break;
        }

        case IO_BLK_SET_CONFIG_POWER_UP:
        {
            Bus->PowerUp( Bus );
            /* TODO This may be redundant, since it will be taken care of automagically
               by the media detection code now. */
            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == ENOERR) {
                ATAAttach(DeviceInfo);
                /* TODO Check real status */
            }
            break;
        }

        /* TODO HipZip microdrive hack for when no PM present */
        case IO_PM_SET_CONFIG_REGISTER:
        {
            ++RefCnt;
            if (RefCnt == 1) {
                Bus->PowerUp( Bus );
                Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
                if (Status == ENOERR) {
                    /* TODO Check status of this function */
                    ATAAttach(DeviceInfo);
                }
            }
            break;
        }
	    
        case IO_PM_SET_CONFIG_UNREGISTER:
        {
            --RefCnt;
            if (RefCnt < 0) {
                RefCnt = 0;
            }
            if (RefCnt == 0) {
                Bus->PowerDown( Bus );
            }
            break;
        }
	
        default:
        {
            Status = -ENOSYS;
            break;
        }
    }
    return Status;
}

static bool
_ATAInit(struct cyg_devtab_entry * Entry)
{
    ATADeviceInfo_T * DeviceInfo;
    BusSpace_T * Bus;
    
    DBTR(ATA);
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Entry)->priv;
    Bus = DeviceInfo->Bus;
  
    if( Bus->Init( Bus ) ) {
#if defined(HW_ATA_BUS_0_USE_CD_IRQ) || defined(HW_ATA_BUS_1_USE_CD_IRQ)
        _RegisterIRQ( Bus );
#endif
    }
    return true;
}

static Cyg_ErrNo
_ATALookup(struct cyg_devtab_entry ** Entry,
           struct cyg_devtab_entry * SubEntry,
           const char * Name)
{
    ATADeviceInfo_T* DeviceInfo;
    BusSpace_T* Bus;
    
    DBTR(ATA);

    DeviceInfo = (ATADeviceInfo_T*)(*Entry)->priv;
    Bus = DeviceInfo->Bus;

    if( !Bus->IsReset ) {
        ATAAttach( DeviceInfo );
    }
    
    return ENOERR;
}


//////////////////////////////////////////////////////
//
// ATAPI Functions
//
//////////////////////////////////////////////////////

/* ATAPI devio functions */
static Cyg_ErrNo
_ATAPIWrite(cyg_io_handle_t Handle, const void * Buffer, cyg_uint32 * Length, cyg_uint32 Block)
{
    ATADeviceInfo_T * DeviceInfo;
    ATAPICommand_T Cmd;
    unsigned short NumBlks;
    int Status;

    DBTR(ATA);
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;
    
    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags = ATAPI_DATA_OUT | ATAPI_AUTO_SENSE;
    Cmd.Data = (char *)Buffer;
    Cmd.DataLength = *Length;
    Cmd.Timeout = 100 * 30; /* 30s */

    Cmd.SCSICmd = &Cmd._SCSICmd;
    Cmd.SCSICmdLength = 12;
    Cmd.SCSICmd->Opcode = SCSI_WRITE10;
    Cmd.SCSICmd->Bytes[1] = (Block >> 24) & 0xff;
    Cmd.SCSICmd->Bytes[2] = (Block >> 16) & 0xff;
    Cmd.SCSICmd->Bytes[3] = (Block >> 8) & 0xff;
    Cmd.SCSICmd->Bytes[4] = (Block) & 0xff;
    NumBlks = (*Length) / (DeviceInfo->Partition->SectorSize);
    Cmd.SCSICmd->Bytes[6] = (NumBlks >> 8) & 0xff;
    Cmd.SCSICmd->Bytes[7] = (NumBlks) & 0xff;

    Status = ATAPIExecuteCommand(DeviceInfo, &Cmd);
    /* TODO Process status */
    if (Cmd.Error == ATAPI_TIMEOUT) {
        Status = -ETIME;
    }
    else if (Cmd.Error != ATAPI_NO_ERROR) {
        Status = -Cmd.SCSISenseData.ASC;
    }
    else {
        Status = ENOERR;
    }
    return Status;
}

static Cyg_ErrNo
_ATAPIRead(cyg_io_handle_t Handle, void * Buffer, cyg_uint32 * Length, cyg_uint32 Block)
{
    ATADeviceInfo_T * DeviceInfo;
    ATAPICommand_T Cmd;
    unsigned short NumBlks;
    int Status;
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;
    
    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags = ATAPI_DATA_IN | ATAPI_AUTO_SENSE;
    Cmd.Data = (char *)Buffer;
    Cmd.DataLength = *Length;
    Cmd.Timeout = 100 * 30; /* 30s */

    Cmd.SCSICmd = &Cmd._SCSICmd;
    Cmd.SCSICmdLength = 12;
    Cmd.SCSICmd->Opcode = SCSI_READ10;
    Cmd.SCSICmd->Bytes[1] = (Block >> 24) & 0xff;
    Cmd.SCSICmd->Bytes[2] = (Block >> 16) & 0xff;
    Cmd.SCSICmd->Bytes[3] = (Block >> 8) & 0xff;
    Cmd.SCSICmd->Bytes[4] = (Block) & 0xff;
    NumBlks = (*Length) / (DeviceInfo->Partition->SectorSize);
    Cmd.SCSICmd->Bytes[6] = (NumBlks >> 8) & 0xff;
    Cmd.SCSICmd->Bytes[7] = (NumBlks) & 0xff;

    Status = ATAPIExecuteCommand(DeviceInfo, &Cmd);
    /* TODO Process status */
    if (Cmd.Error == ATAPI_TIMEOUT) {
        Status = -ETIME;
    }
    else if (Cmd.Error != ATAPI_NO_ERROR) {
        // interpreting the ASC is best done with the SCSI documentation
        // covering the 'request sense' cmd; specifically spc3r00.pdf section
        // 7.20.6 contains a table with ASC/ASCQ pairs and their meaning
        Status = -Cmd.SCSISenseData.ASC;
    }
    else {
        Status = ENOERR;
    }
    return Status;
}

static cyg_bool
_ATAPISelect(cyg_io_handle_t Handle, cyg_uint32 Which, CYG_ADDRWORD Info) 
{
    DBTR(ATA);
    
    return true;
}

static Cyg_ErrNo
_ATAPIGetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, void * Buffer, cyg_uint32 * Length)
{
    Cyg_ErrNo Status;
    ATADeviceInfo_T * DeviceInfo;

    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;
    
    switch (Key) {

		case IO_ATAPI_GET_POWER_STATUS:
		{

			 ATACommand_T Cmd;

            /* Issue test unit ready command */
                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = 0xE5; // check power
				Cmd.Flags = ATA_CMD_READ_REG;
                Cmd.StatusBefore = 0;
                Cmd.StatusAfter = 0;
                Cmd.Timeout = ATA_DELAY;
                ATAExecuteCommand(DeviceInfo, &Cmd);
				// diag_printf("power = %x, status =%x\n",Cmd.SectorCount,Status);
                /* TODO Check the status here for real */
                /* TODO Mask the interrupt */
                Status = Cmd.SectorCount;
           		
			break;
		}
	

        case IO_BLK_GET_CONFIG_MEDIA_STATUS:
        {
            ATAPICommand_T Cmd;

            /* Issue test unit ready command */
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Flags = ATAPI_AUTO_SENSE;
            Cmd.Timeout = 100 * 30;
            Cmd.SCSICmd = &Cmd._SCSICmd;
            Cmd.SCSICmdLength = 12;
            Status = ATAPIExecuteCommand(DeviceInfo, &Cmd);
            if (Cmd.Error == ATAPI_TIMEOUT) {
                Status = -ETIME;			
            }
            else if (Cmd.Error != ATAPI_NO_ERROR) {
                Status = -Cmd.SCSISenseData.ASC;				
            }
            else {
                Status = ENOERR;				
            }
            break;
        }
	
        default:
        {
            Status = -ENOSYS;
            break;
        }
    }
    return Status;
}

static Cyg_ErrNo
_ATAPISetConfig(cyg_io_handle_t Handle, cyg_uint32 Key, const void * Buffer, cyg_uint32 * Length)
{
    Cyg_ErrNo Status;
    ATADeviceInfo_T * DeviceInfo;
    ATACommand_T Cmd;
    BusSpace_T * Bus;
    static int RefCnt = 0;
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Handle)->priv;
    Bus = DeviceInfo->Bus;
    
    switch (Key) {
        case IO_BLK_SET_CONFIG_MEDIA_STATUS_CB:
        {
            
#ifndef NOKERNEL
            Bus->MediaChangeCB = (void(*)(int,int,Cyg_ErrNo))((unsigned int*)Buffer);
#endif /* NOKERNEL */
            
            break;
        }
        case IO_ATA_SET_CONFIG_BUS_LOCK:
        {
            BusLock( Bus );
            Status = ENOERR;
            break;
        }
        case IO_ATA_SET_CONFIG_BUS_UNLOCK:
        {
            BusUnlock( Bus );
            Status = ENOERR;
            break;
        }
        case IO_ATAPI_SET_CONFIG_TRAY_CLOSE: 
        {
            
            // close tray
            ATAPICommand_T  ATAPICmd;
            memset(&ATAPICmd, 0, sizeof(ATAPICmd));
            ATAPICmd.Flags = 0;
            
            ATAPICmd.Timeout = 100*50;
            ATAPICmd.SCSICmd = &ATAPICmd._SCSICmd;
            ATAPICmd.SCSICmdLength = 12;
            ATAPICmd.SCSICmd->Opcode = 0x1B; /* STOP START UNIT */
            ATAPICmd.SCSICmd->Bytes[0] = 0x01; // bit 1 = IMMED?
            ATAPICmd.SCSICmd->Bytes[1] = 0x0;
            ATAPICmd.SCSICmd->Bytes[2] = 0x0;
            ATAPICmd.SCSICmd->Bytes[3] = 0x03 /*inject*/ | (0x02 << 4); /* force active mode */
            ATAPICmd.SCSICmd->Bytes[4] = 0x0;
            
            ATAPIExecuteCommand(DeviceInfo, & ATAPICmd);
            
            // lock media tray
            
            memset(&ATAPICmd, 0, sizeof( ATAPICmd));
            ATAPICmd.Flags = 0;
            
            ATAPICmd.Timeout = 100*30;
            ATAPICmd.SCSICmd = &ATAPICmd._SCSICmd;
            ATAPICmd.SCSICmdLength = 12;
            ATAPICmd.SCSICmd->Opcode = 0x1E; /* PREVENT-ALLOW MEDIA REMOVAL */
            ATAPICmd.SCSICmd->Bytes[1] = 0x1; // bit 1 = IMMED?
            ATAPICmd.SCSICmd->Bytes[2] = 0x0;
            ATAPICmd.SCSICmd->Bytes[3] = 0x0;
            ATAPICmd.SCSICmd->Bytes[4] = 0x1; // lock tray
            ATAPICmd.SCSICmd->Bytes[5] = 0x0;
            
            ATAPIExecuteCommand(DeviceInfo, & ATAPICmd);
            
            
            Status = ENOERR;
            
            break;
            
        }
        
        case IO_ATAPI_SET_CONFIG_TRAY_OPEN: 
        {
            // disk is in, eject
            ATAPICommand_T  ATAPICmd;
            // allow media removal
            memset(&ATAPICmd, 0, sizeof( ATAPICmd));
            ATAPICmd.Flags = 0;
            
            ATAPICmd.Timeout = 100*30;
            ATAPICmd.SCSICmd = &ATAPICmd._SCSICmd;
            ATAPICmd.SCSICmdLength = 12;
            ATAPICmd.SCSICmd->Opcode = 0x1E; /* PREVENT-ALLOW MEDIA REMOVAL */
            ATAPICmd.SCSICmd->Bytes[1] = 0x01; // bit 1 = IMMED?
            ATAPICmd.SCSICmd->Bytes[2] = 0x0;
            ATAPICmd.SCSICmd->Bytes[3] = 0x0;
            ATAPICmd.SCSICmd->Bytes[4] = 0x0; // allow eject
            ATAPICmd.SCSICmd->Bytes[5] = 0x0;
            
            ATAPIExecuteCommand(DeviceInfo, & ATAPICmd);
            
            
            // eject disk
            memset(&ATAPICmd, 0, sizeof( ATAPICmd));
            ATAPICmd.Flags = 0;
            
            ATAPICmd.Timeout = 100*50;
            ATAPICmd.SCSICmd = &ATAPICmd._SCSICmd;
            ATAPICmd.SCSICmdLength = 12;
            ATAPICmd.SCSICmd->Opcode = 0x1B; /* STOP START UNIT */
            ATAPICmd.SCSICmd->Bytes[0] = 0x01; // bit 1 = IMMED?
            ATAPICmd.SCSICmd->Bytes[1] = 0x0;
            ATAPICmd.SCSICmd->Bytes[2] = 0x0;
            ATAPICmd.SCSICmd->Bytes[3] = 0x02; // eject
            ATAPICmd.SCSICmd->Bytes[4] = 0x0;
            
            ATAPIExecuteCommand(DeviceInfo, & ATAPICmd);
            
            
            Status = ENOERR;
            
            break;
        }	
        case IO_ATA_SET_CONFIG_EXEC_COMMAND: 
        {
            ATACommand_T * Cmd = (ATACommand_T *)Buffer;
            
            if (*Length == sizeof(ATACommand_T)) {
                /* TODO Check return value and support queuing */
                ATAExecuteCommand(DeviceInfo, Cmd);
                if (Cmd->Flags & ATA_CMD_ERROR) {
                    Status = -EIO;
                }
                else if (Cmd->Flags & ATA_CMD_TIMEOUT) {
                    Status = -ETIME;
                }
                else {
                    Status = ENOERR;
                }
            }
            else {
                Status = -EINVAL;
            }
            break;
        }
        case IO_BLK_SET_CONFIG_LOOKAHEAD_DISABLE:
        {
            /* TODO Disable this and make caller use EXEC_COMMAND */
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x55;	/* Disable look-ahead subcommand */
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if (Cmd.Flags & ATA_CMD_TIMEOUT) {
                Status = -ETIME;
            }
            else if (Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
            }
            else {
                Status = ENOERR;
            }
            break;
        }
        
		case IO_ATAPI_SET_CONFIG_FEATURES:
		{
           
#ifdef PIOMODE
			diag_printf("PIO ATAPI\n");
			// set transfer mode
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x03;   /* set transfer mode */
			Cmd.SectorCount = 0x0B; /* PIO mode 3 */
         
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if(Cmd.Flags & ATA_CMD_ERROR) {
                Status = -EIO;
                return Status;
            } else {
                Status = ENOERR;
            }
#endif
            // Power up in standby
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter  = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x06;   /* power up in standby */
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if( Cmd.Flags & ATA_CMD_ERROR ) {
                Status = -EIO;
                return Status;
            }
            else {
                Status = ENOERR;
            }

            // Disable power management controlled transitions to standby
            memset(&Cmd, 0, sizeof(Cmd));
            Cmd.Command = ATA_SET_FEATURES;
            Cmd.StatusBefore = STATUS_DRDY;
            Cmd.StatusAfter  = STATUS_DRDY;
            Cmd.Timeout = ATA_DELAY;
            Cmd.Features = 0x05;   /* enable/disable advanced power management */
            Cmd.SectorCount = 0xFE; /* disable standby */
            ATAExecuteCommand(DeviceInfo, &Cmd);
            if( Cmd.Flags & ATA_CMD_TIMEOUT ) {
                Status = -ETIME;
                return Status;
            } else if( Cmd.Flags & ATA_CMD_ERROR ) {
                Status = -EIO;
                return Status;
            }
            else {
                Status = ENOERR;
            }

		}


        case IO_ATAPI_SET_CONFIG_EXEC_COMMAND: 
        {
            ATAPICommand_T * Cmd = (ATAPICommand_T *)Buffer;
            
            if (*Length == sizeof(ATAPICommand_T)) {
                Status = ATAPIExecuteCommand(DeviceInfo, Cmd);
                if (Cmd->Error == ATAPI_TIMEOUT) {
                    Status = -ETIME;
                }
                else if (Cmd->Error != ATAPI_NO_ERROR) {
                    Status = -EIO;
                }
                else {
                    Status = ENOERR;
                }
            }
            else {
                Status = -EINVAL;
            }
            /* TODO This is how it used to behave */
            Status = (DeviceInfo->Bus->Error & 0xf0) >> 4;
            DEBUG(ATA,DBGLEV_INFO, "Status: %x\n", Status);
            break;
        }
        
        case IO_BLK_SET_CONFIG_RESET:
        {
            /* TODO Check the status here for real */
            Bus->IsReset = 0;
            DeviceInfo->Bus->Reset( Bus );
            ATAPIAttach(DeviceInfo);
            break;
        }
        
          case IO_BLK_SET_CONFIG_SLEEP:
        {


            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }

            if (Status == ENOERR) {
                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = ATA_SET_FEATURES;
                Cmd.StatusBefore = STATUS_DRDY;
                Cmd.StatusAfter  = STATUS_DRDY;
                Cmd.Timeout = ATA_DELAY;
                Cmd.Features = 0x05;   /* enable/disable advanced power management */
                Cmd.SectorCount = 0x01; /* enable standby */
                ATAExecuteCommand(DeviceInfo, &Cmd);

                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = 0xE0; // standby immediate
                Cmd.StatusBefore = STATUS_DRDY;
                Cmd.StatusAfter = STATUS_DRDY;
                Cmd.Timeout = ATA_DELAY;
                ATAExecuteCommand(DeviceInfo, &Cmd);
                /* TODO Check the status here for real */
                /* TODO Mask the interrupt */
                Status = ENOERR;
            }

            break;
        }
	
        case IO_BLK_SET_CONFIG_WAKEUP:
        {


            Status = _GetMediaStatus(Bus, DeviceInfo->Drive);
            if (Status == -EMEDCHG) {
                ATAAttach(DeviceInfo);
            }

            if (Status == ENOERR) {
                memset(&Cmd, 0, sizeof(Cmd));
                Cmd.Command = 0xE1; // set idle immediate mode
                Cmd.StatusBefore = STATUS_DRDY;
                Cmd.StatusAfter = STATUS_DRDY;
                Cmd.Timeout = ATA_DELAY;
                ATAExecuteCommand(DeviceInfo, &Cmd);
                /* TODO Check the status here for real */
                /* TODO Mask the interrupt */
                Status = ENOERR;
            }

            break;
        }
        
        case IO_BLK_SET_CONFIG_POWER_DOWN:
        {
            Bus->PowerDown( Bus );
            Status = ENOERR;
            break;
        }
        
        case IO_BLK_SET_CONFIG_POWER_UP:
        {
            Bus->PowerUp( Bus );
            ATAPIAttach(DeviceInfo);
            /* TODO Check real status */
            Status = ENOERR;
            break;
        }
        
        /* TODO HipZip microdrive hack for when no PM present */
        case IO_PM_SET_CONFIG_REGISTER:
        {
            ++RefCnt;
            if (RefCnt == 1) {
                Bus->PowerUp( Bus );
                ATAPIAttach(DeviceInfo);
                /* TODO Check real status */
            }
            Status = ENOERR;
            break;
        }
        
        case IO_PM_SET_CONFIG_UNREGISTER:
        {
            --RefCnt;
            if (RefCnt < 0) {
                RefCnt = 0;
            }
            if (RefCnt == 0) {
                Bus->PowerDown( Bus );
            }
            Status = ENOERR;
            break;
        }
        
        default: 
        {
            Status = -ENOSYS;
            break;
        }
    }
    return Status;
}

/* TODO These may need to be device specific versus protocol specific */
static bool
_ATAPIInit(struct cyg_devtab_entry * Entry)
{
    ATADeviceInfo_T * DeviceInfo;
    
    DBTR(ATA);
    
    DeviceInfo = (ATADeviceInfo_T *)((cyg_devtab_entry_t *)Entry)->priv;
    
    if( DeviceInfo->Bus->Init( DeviceInfo->Bus ) ) {
#if defined(HW_ATA_BUS_0_USE_CD_IRQ) || defined(HW_ATA_BUS_1_USE_CD_IRQ)
        //_RegisterIRQ( DeviceInfo->Bus );
#endif
    }

    return true;
}

static Cyg_ErrNo
_ATAPILookup(struct cyg_devtab_entry ** Entry,
             struct cyg_devtab_entry * SubEntry,
             const char * Name)
{
    return ENOERR;
}

static Cyg_ErrNo
_GetMediaStatusIRQ(BusSpace_T * Bus, unsigned char Drive)
{
    Cyg_ErrNo Status = ENOERR;
    
    /* Prevent the media change DSR from messing with MediaStatus */
    cyg_drv_dsr_lock();
    
    if (Bus->Device[Drive].MediaStatus == -EMEDCHG) {
        Bus->Device[Drive].MediaStatus = ENOERR;
        Status = -EMEDCHG;
    }
    else {
        Status = Bus->Device[Drive].MediaStatus;
    }
    
    cyg_drv_dsr_unlock();

    return Status;
}

static Cyg_ErrNo
_GetMediaStatusPoll(BusSpace_T * Bus, unsigned char Drive, int MediaPresent)
{
    Cyg_ErrNo Status = ENOERR;    

    if (MediaPresent) {
        if (Bus->Device[Drive].MediaPresent == 0) {
            Bus->IsReset = 0;
            Status = -EMEDCHG;
        }
        else {
            Status = ENOERR;
        }
    }
    else {
        Status = -ENOMED;
    }
    Bus->Device[Drive].MediaPresent = MediaPresent;
    
    return Status;
}
    
static Cyg_ErrNo
_GetMediaStatus(BusSpace_T * Bus, unsigned char Drive) 
{
    Cyg_ErrNo Status = ENOERR;
    int MediaPresent = 1; /* Default to media present */

#if defined(HW_ATA_ENABLE_BUS_0)
    if (Bus == &Bus0) {
#if defined(HW_ATA_BUS_0_USE_CD_IRQ)
        Status = _GetMediaStatusIRQ(Bus, Drive);
#else
#if defined(HW_ATA_BUS_0_CD_PORT) /* This option implies that the drive is 0 */
        MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_0_CD_PORT) & HW_ATA_BUS_0_CD_PIN;
        MediaPresent = MediaPresent ? 0 : 1;
#endif   // HW_ATA_BUS_0_CD_PORT
        Status = _GetMediaStatusPoll(Bus, Drive, MediaPresent);
#endif
    }
#endif

#if defined(HW_ATA_ENABLE_BUS_1)
    if (Bus == &Bus1) {
#if defined(HW_ATA_BUS_1_USE_CD_IRQ)
        Status = _GetMediaStatusIRQ(Bus, Drive);
#else
#if defined(HW_ATA_BUS_1_CD_PORT) /* This option implies that the drive is 0 */
        MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_1_CD_PORT) & HW_ATA_BUS_1_CD_PIN;
        MediaPresent = MediaPresent ? 0 : 1;
#endif   // HW_ATA_BUS_1_CD_PORT
        Status = _GetMediaStatusPoll(Bus, Drive, MediaPresent);
#endif
    }
#endif
    return Status;
}

#if defined(HW_ATA_BUS_0_USE_CD_IRQ) || defined(HW_ATA_BUS_1_USE_CD_IRQ)

static void
_RegisterIRQ( BusSpace_T* Bus )
{
#if defined(HW_ATA_ENABLE_BUS_0)
    if( Bus == &Bus0 ) {
#if defined(HW_ATA_BUS_0_USE_CD_IRQ)
        cyg_drv_interrupt_create( HW_ATA_BUS_0_CD_IRQ,
                                  40,
                                  (cyg_addrword_t) Bus,
                                  (cyg_ISR_t*) ata_media_change_isr,
                                  (cyg_DSR_t*) ata_media_change_dsr,
                                  &bus0_interrupt_handle,
                                  &bus0_interrupt_data );
        cyg_interrupt_attach( bus0_interrupt_handle );
        cyg_interrupt_acknowledge( HW_ATA_BUS_0_CD_IRQ );
        cyg_interrupt_unmask( HW_ATA_BUS_0_CD_IRQ );
#endif   // HW_ATA_BUS_0_USE_CD_IRQ
    }
#endif // HW_ATA_ENABLE_BUS_0

#if defined(HW_ATA_ENABLE_BUS_1)
    if( Bus == &Bus1 ) {
#if defined(HW_ATA_BUS_1_USE_CD_IRQ)
        cyg_drv_interrupt_create( HW_ATA_BUS_1_CD_IRQ,
                                  30,
                                  (cyg_addrword_t) Bus,
                                  (cyg_ISR_t*) ata_media_change_isr,
                                  (cyg_DSR_t*) ata_media_change_dsr,
                                  &bus1_interrupt_handle,
                                  &bus1_interrupt_data );
        cyg_interrupt_attach( bus1_interrupt_handle );
        cyg_interrupt_acknowledge( HW_ATA_BUS_1_CD_IRQ );
#if !defined(CYGPKG_DEVS_PCMCIA_DHARMA)
        cyg_interrupt_unmask( HW_ATA_BUS_1_CD_IRQ );
#endif
#endif   // HW_ATA_BUS_1_USE_CD_IRQ
        
    }
#endif   // HW_ATA_ENABLE_BUS_1
}

static cyg_uint32
ata_media_change_isr(cyg_vector_t Vector, cyg_addrword_t Data) 
{
    int MediaPresent = 1;
    BusSpace_T * Bus = (BusSpace_T *)Data;

    /* First off, there's no good way to do this that's not going to miss an insertion
       or removal sometime.  There seems to be a difference between when the interrupt
       gets fired and when the card detect pin shows the true state of the card.  The
       deciding factor in getting this working would be to know how long it is possible
       for the card detect pin to have the wrong state after the interrupt fires.  Then
       we could check the state after that interval. */
    
#if defined(HW_ATA_ENABLE_BUS_0)
    if( Bus == &Bus0 ) {
#if defined(HW_ATA_BUS_0_CD_PORT) /* This option implies that the drive is 0 */
        MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_0_CD_PORT) & HW_ATA_BUS_0_CD_PIN;
#endif   // HW_ATA_BUS_0_CD_PORT
    }
#endif   // HW_ATA_ENABLE_BUS_0
    
#if defined(HW_ATA_ENABLE_BUS_1)
    if( Bus == &Bus1 ) {
#if defined(HW_ATA_BUS_1_CD_PORT) /* This option implies that the drive is 0 */
        MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_1_CD_PORT) & HW_ATA_BUS_1_CD_PIN;
#endif   // HW_ATA_BUS_1_CD_PORT
    }
#endif   // HW_ATA_ENABLE_BUS_1

    MediaPresent = MediaPresent ? 0 : 1;
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    if (MediaPresent != Bus->Device[0].MediaPresent) {
#endif
        Bus->Device[0].MediaPresent = MediaPresent;
        return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    }
    else {
        return 0;
    }
#endif
}

static void
ata_media_change_dsr(cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data)
{
    BusSpace_T * Bus = (BusSpace_T *)Data;
    int BusIndex;
    
    /* We know coming in here that the media state changed, so media present implies
       media changed. */

#if defined(HW_ATA_ENABLE_BUS_0)
    BusIndex = (Bus == &Bus0 ? 0 : 1);
#else
    BusIndex = 1;
#endif
    
    /* Master */
    if (Bus->Device[0].MediaPresent) {
        Bus->Device[0].MediaStatus = -EMEDCHG;
    }
    else {
        Bus->Device[0].MediaStatus = -ENOMED;
    }
    if (Bus->MediaChangeCB) {
        Bus->MediaChangeCB(BusIndex, 0, Bus->Device[0].MediaStatus);
    }

#if 0 /* Not supported */
    /* Slave */
    if (Bus->Device[1].MediaPresent) {
        Bus->Device[1].MediaStatus = -EMEDCHG;
    }
    else {
        Bus->Device[1].MediaStatus = -ENOMED;
    }
    if (Bus->MediaChangeCB) {
        Bus->MediaChangeCB((Bus == &Bus0 ? 0 : 1), 1, Bus->Device[1].MediaStatus);
    }
#endif
}

#endif // BUS_0_USE_CD_IRQ || BUS_1_USE_CD_IRQ
