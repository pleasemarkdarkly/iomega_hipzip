//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 8/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   toddm
// Contributors:  toddm
// Date:        1999-12-07
// Purpose:     FAT wrapper for eCos device layer.
// Description: 
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/io/io.h>

#include <util/debug/debug.h>
#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>
#include <devs/storage/ata/atadrv.h>
#include "iome_fat.h" 

// declare the debug module here
DEBUG_MODULE(FAT);
DEBUG_USE_MODULE(FAT);

/* The following functions are wrapped: */
#if 0
#define CONTROLLER_INIT()       (ide_init()) 
#define INTERFACE_DEVICE_CLOSE(driveno) (ide_drive_close(driveno))
#define INTERFACE_DEVICE_OPEN(driveno)   (ide_drive_open(driveno))
#define DEV_READ_SERIAL(drive_no, idDrvPtr)  ( \
                        ide_read_serial(drive_no, idDrvPtr))
#define DEV_READ(driveno, sector, buffer, count)  (\
                        ide_read(driveno, sector, buffer, count))
#define DEV_WRITE(driveno, sector, buffer, count) (\
                        ide_write(driveno, sector, buffer, count))
#define DEV_SECTORS_ERASE(driveno, sector, count) (\
                        ide_erase(driveno, sector, count))

SDVOID drive_format_information(INT16 driveno, UINT16 *n_heads, UINT16 *sec_ptrack, UINT16 *n_cyls);
SDIMPORT INT16 get_interface_error(INT16 driveno);
#endif

// this assignment is actually fine as long as TOTAL_DRIVES > 0
static cyg_io_handle_t driveno_to_handle[TOTAL_DRIVES] = { 0 };
static cyg_uint16 driveno_interface_error[TOTAL_DRIVES];

bool
iome_fat_ata_init(void)
{
    /* This is done in the device init, no need to repeat it. */
    return true;
}

bool
iome_drive_close(cyg_uint16 driveno)
{
    
    /* This doesn't get called from anywhere, so it is an empty function */
    DBEN(FAT);

#if defined(ENABLE_PM)
    cyg_uint32 len = sizeof(len);
    cyg_io_set_config(driveno_to_handle[driveno], IO_PM_SET_CONFIG_UNREGISTER, &len, &len);
#endif /* !__DHARMA */
    
    DBEX(FAT);
    
    return true;
}

bool
iome_drive_open(cyg_uint16 driveno) 
{
    Cyg_ErrNo err;  
    cyg_uint32 len;

    DBEN(FAT);

    if (driveno_to_handle[driveno] == 0) {
        if( cyg_io_lookup( block_drive_names[driveno], &driveno_to_handle[driveno] ) != ENOERR ) {
            return false;
        }

#if defined(ENABLE_PM)
        /* turn on the disk */
        len = sizeof(len);
        err = cyg_io_set_config(driveno_to_handle[driveno], IO_PM_SET_CONFIG_REGISTER, &len, &len);
        if (err < 0) {
            driveno_to_handle[driveno] = 0;
            driveno_interface_error[driveno] = -err;
            return false;
        }
#else /* ENABLE_PM */
        /* turn on the disk */
        len = sizeof(len);
        err = cyg_io_set_config(driveno_to_handle[driveno], IO_BLK_SET_CONFIG_POWER_UP, &len, &len);
        if( err < 0 && err != -ENOSYS ) {
            driveno_to_handle[driveno] = 0;
            driveno_interface_error[driveno] = -err;
            return false;
        }
#endif /* ENABLE_PM */


#if (DEBUG_LEVEL != 0)
		diag_printf("drive_open: Set special ATA features (AAM)\n");
#endif
   
		err = cyg_io_set_config(driveno_to_handle[driveno],	IO_ATA_SET_CONFIG_FEATURES, 0, &len);

		/* fail silently */
#if (DEBUG_LEVEL != 0)
		if(err != ENOERR)
		{
			diag_printf("drive_open: Set special ATA features failure\n");
		}
#endif

    }

    len = sizeof(len);
    err = cyg_io_get_config(driveno_to_handle[driveno], IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);
    if (err < 0) {
        driveno_interface_error[driveno] = -err;
        return false;
    }
    else {
        return true;
    }
}

bool
iome_read_serial(cyg_uint16 driveno, PDRV_GEOMETRY_DESC idDrvPtr)
{
    Cyg_ErrNo err;
    drive_geometry_t dg;
    cyg_uint32 len = sizeof(dg);
    unsigned int i;
  
    /* Wraps cyg_io_get_config(GEOMETRY) */
    err = cyg_io_get_config(driveno_to_handle[driveno], IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len);
    if (err == ENOERR) {
        /* translate from drive_geometry_t to DRV_GEOMETRY_DESC */
        idDrvPtr->dfltCyl = dg.cyl;
        idDrvPtr->dfltHd = dg.hd;
        idDrvPtr->dfltSct = dg.sec;
        idDrvPtr->dfltBytesPerSect = dg.bytes_p_sec;
        idDrvPtr->totalLBA = dg.num_blks;
        for (i = 0; i < 20; ++i) {
            idDrvPtr->serialNum[i] = dg.serial_num[i];
            idDrvPtr->modelNum[i] = dg.model_num[i];
        }
        for (; i < 40; ++i) {
            idDrvPtr->modelNum[i] = dg.model_num[i];
        }
        return true;
    }
    else {
        return false;
    }
}

bool
iome_read(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint32 count)
{
    Cyg_ErrNo err;
    cyg_uint32 num_blks = count * 512;

    err = cyg_io_bread(driveno_to_handle[driveno], buffer, &num_blks, sector);

    if (err < 0) {
        driveno_interface_error[driveno] = -err;
        return false;
    }
    else {
        return true;
    }
}

bool
iome_write(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint32 count)
{
    Cyg_ErrNo err;
    cyg_uint32 num_blks = count * 512;
    
    err = cyg_io_bwrite(driveno_to_handle[driveno], buffer, &num_blks, sector);

    if (err < 0) {
        driveno_interface_error[driveno] = -err;
        return false;
    }
    else {
        return true;
    }
}

bool
iome_erase(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint16 count)
{
#if 1 /* TODO fix this */
    DBEN( FAT );
    return true;
#else
    Cyg_ErrNo err;
    /* Wraps cyg_io_set_config(ERASE) */
    blk_req.lba = sector;
    blk_req.num_blks = count;
    blk_req.buf = 0;
    err = cyg_io_set_config(driveno_to_handle[driveno], IO_ATAPI_SET_CONFIG_ERASE_SECTORS, &blk_req, &blk_req.num_blks);
    if (err == ENOERR) {
        return true;
    }
    else {
        return false;
    }
#endif
}

cyg_uint16
iome_interface_error(cyg_uint16 driveno)
{
    return driveno_interface_error[driveno];
}

bool
iome_check_media_status(cyg_uint16 driveno)
{
    Cyg_ErrNo err;
    cyg_uint32 len;

    len = sizeof(len);
    err = cyg_io_get_config(driveno_to_handle[driveno], IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);

    if (err < 0) {
        driveno_interface_error[driveno] = -err;
        return false;
    }
    else {
        return true;
    }
}

unsigned int
iome_get_lba_last_error(cyg_uint16 driveno)
{
#if 1
    /* TODO Fix this */
    DBEN( FAT );
    return 0;
#else /* __DHARMA */
    unsigned int lba;
    cyg_uint32 len = sizeof(lba);
    Cyg_ErrNo err = cyg_io_get_config(driveno_to_handle[driveno], IO_ATAPI_GET_CONFIG_LBA_LAST_ERROR, &lba, &len);
    DEBUG_HE("HE:lba %x\n",lba);
    return lba;
#endif /* __DHARMA */
}

