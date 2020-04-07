//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef CYGONCE_IOME_FAT_H
#define CYGONCE_IOME_FAT_H
// ====================================================================
//
//      iome_fat.h
//
//      eCos wrapper for FAT layer.
//
// ====================================================================
// ====================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   toddm
// Contributors:        toddm
// Date:        1999-12-07
// Purpose:     Device I/O wrapper
// Description:
//
//####DESCRIPTIONEND####
//
// ====================================================================

#include <cyg/infra/cyg_type.h>
#include <fs/fat/sdapi.h>

bool iome_fat_ata_init(void);
bool iome_drive_close(cyg_uint16 driveno);
bool iome_drive_open(cyg_uint16 driveno);
bool iome_read_serial(cyg_uint16 driveno, PDRV_GEOMETRY_DESC idDrvPtr);
bool iome_read(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint16 count);
bool iome_write(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint8 *buffer, cyg_uint16 count);
bool iome_erase(cyg_uint16 driveno, cyg_uint32 sector, cyg_uint16 count);
cyg_uint16 iome_interface_error(cyg_uint16 driveno);
bool iome_check_media_status(cyg_uint16 driveno);
unsigned int iome_get_lba_last_error(cyg_uint16 driveno);

/* drive_format_information() is left alone for now, once pc structure is moved into ata driver it can be changed to a cyg_io_get_config() function */

#endif /* CYGONCE_IOME_FAT_H */
