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
/******************************************************************************
* Filename: INTRFACE.H - Interface layer for IDE, PCMCIA, SPI, MMC
*                     
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997-1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description: 
*       Provide the initial state, device status and closing state
*       for IDE, PCMCIA, SPI, or MMC controller.
*
******************************************************************************/

#ifndef __INTRFACE_H__

#include <fs/fat/sdtypes.h>
#include <fs/fat/sdconfig.h>
#include "iome_fat.h"

#ifdef __cplusplus
extern "C" {
#endif

    
#if (USE_FILE_SYSTEM)

/******************************** IDE SECTION ********************************/
/********************** This is for IDE Controller ONLY **********************/

/******************************* IDE INTERFACE *******************************/

/* ide.c */
/* Initialize IDE controller */
#define CONTROLLER_INIT()       (iome_fat_ata_init())
#define INTERFACE_DEVICE_CLOSE(driveno) (iome_drive_close(driveno))
#define INTERFACE_DEVICE_OPEN(driveno)   (iome_drive_open(driveno))
#define DEV_READ_SERIAL(drive_no, idDrvPtr)  ( \
                        iome_read_serial(drive_no, idDrvPtr))
#define DEV_READ(driveno, sector, buffer, count)  (\
                        iome_read(driveno, sector, buffer, count))
#define DEV_WRITE(driveno, sector, buffer, count) (\
                        iome_write(driveno, sector, buffer, count))
#define DEV_SECTORS_ERASE(driveno, sector, count) (\
                        iome_erase(driveno, sector, count))



/* Drive Formating and mapping information */ 
SDVOID drive_format_information(INT16 driveno, UINT16 *n_heads, UINT16 *sec_ptrack, UINT16 *n_cyls);

/* Interface error routine */
SDIMPORT INT16 get_interface_error(INT16 driveno);

#endif  /* (USE_FILE_SYSTEM) */


#ifdef __cplusplus
}
#endif


#define __INTRFACE_H__

#endif

