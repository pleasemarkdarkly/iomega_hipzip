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

#include <redboot.h>
#include <microdrive.h>
#include <fs/fat/iome_fat.h>

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

int errno;		/* This seemed like a convenient place to put this */
//static ata_drive_datas_t * microdrive;

bool
iome_fat_ata_init(void)
{
    if (ata_init() == ENOERR) {
	return true;
    }
    else {
	return false;
    }

#if 0
    microdrive = ata_init();
    if (microdrive) {
	return true;
    }
    else {
	return false;
    }
#endif
}

bool
iome_drive_close(cyg_uint16 driveno)
{
    return true;
}

bool
iome_drive_open(cyg_uint16 driveno) 
{
    return true;
}

bool
iome_read_serial(cyg_uint16 driveno, PDRV_GEOMETRY_DESC idDrvPtr)
{
    ataparams_t prms;
  
    //if (ata_get_params(microdrive, 0, &prms) == CMD_OK) {
    if (ata_get_params(&prms) == ENOERR) {
	/* translate from ataparams to DRV_GEOMETRY_DESC */
	idDrvPtr->dfltCyl = prms.atap_cylinders;
	idDrvPtr->dfltHd = prms.atap_heads;
	idDrvPtr->dfltSct = prms.atap_sectors;
	idDrvPtr->dfltBytesPerSect = DEV_BSIZE;
	idDrvPtr->totalLBA = (prms.atap_capacity[1] << 16) | prms.atap_capacity[0];;
	return true;
    }
    else {
	return false;
    }
}

bool
iome_read(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint16 count)
{
    //if (ata_read(microdrive, sector, count, buffer) != CMD_OK) {
    if (ata_read(sector, count, buffer) != ENOERR) {
	return false;
    }
    return true;
}

bool
iome_write(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint16 count)
{
    //if (ata_write(microdrive, sector, count, buffer) != CMD_OK) {
    if (ata_write(sector, count, buffer) != ENOERR) {
	return false;
    }
    return true;
}

bool
iome_erase(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint16 count)
{
    return true;
}

cyg_uint16
iome_interface_error(cyg_uint16 driveno)
{
    return 0;
}

bool
iome_check_media_status(cyg_uint16 driveno) 
{
    return true;
}

unsigned int
iome_get_lba_last_error(cyg_uint16 driveno)
{
    return 0;
}
