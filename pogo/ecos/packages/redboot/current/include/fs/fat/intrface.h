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
#include <fs/fat/iome_fat.h>

#ifdef __cplusplus
extern "C" {
#endif

    
#if (USE_FILE_SYSTEM)

/******************************** IDE SECTION ********************************/
/********************** This is for IDE Controller ONLY **********************/
#if (USE_TRUE_IDE)

/******************************* IDE INTERFACE *******************************/

/* ide.c */
/* Initialize IDE controller */
#if 1
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
#else
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
#endif
#endif  /* USE_TRUE_IDE */



/**************************** PCMCIA SECTION ********************************/
/********************* This is for PCMCIA Controller ONLY *******************/
#if (USE_PCMCIA)


/*************************** PCMCIA INTERFACE *******************************/

/* pcmcia.c */
/* Initialize PCMCIA controller */
#define CONTROLLER_INIT()       (pcm_init())
#define INTERFACE_DEVICE_CLOSE(driveno) (pcm_drive_close(driveno))
#define INTERFACE_DEVICE_OPEN(driveno)   (pcm_drive_open(driveno))
#define DEV_READ_SERIAL(drive_no, idDrvPtr)  ( \
                        pcm_read_serial(drive_no, idDrvPtr))
#define DEV_READ(driveno, sector, buffer, count)  (\
                        pcm_read(driveno, sector, buffer, count))
#define DEV_WRITE(driveno, sector, buffer, count) (\
                        pcm_write(driveno, sector, buffer, count))
#define DEV_SECTORS_ERASE(driveno, sector, count) (\
                        pcm_erase(driveno, sector, count))
#endif  /* USE_PCMCIA */


/******************************** SPI *************************************/
#if (USE_SPI || USE_SPI_EMULATION)


/*************************** SPI INTERFACE ********************************/

/* spi.c */
/* Initialize SPI controller */
#define CONTROLLER_INIT()       (spi_init())
#define INTERFACE_DEVICE_CLOSE(driveno)        (spi_drive_close(driveno))
#define INTERFACE_DEVICE_OPEN(driveno)  (spi_drive_open(driveno))
#define DEV_READ_SERIAL(drive_no, idDrvPtr)    (\
                        spi_read_serial(drive_no, idDrvPtr))
#define DEV_READ(driveno, sector, buffer, count) (\
                        spi_read(driveno, sector, buffer, count))
#define DEV_WRITE(driveno, sector, buffer, count) (\
                        spi_write(driveno, sector, buffer, count))
#define DEV_SECTORS_ERASE(driveno, sector, count) (\
                        spi_erase(driveno, sector, count))
#endif          /* (USE_SPI || USE_SPI_EMULATION) */



/******************************** MMC *************************************/
#if (USE_MMC || USE_MMC_EMULATION)

/* mmc.c */

/* Initialize MMC controller */
#define CONTROLLER_INIT()       (mmc_init())
#define INTERFACE_DEVICE_CLOSE(driveno)        (mmc_drive_close(driveno))
#define INTERFACE_DEVICE_OPEN(driveno)  (mmc_drive_open(driveno))
#define DEV_READ_SERIAL(drive_no, idDrvPtr)    (\
                        mmc_read_serial(drive_no, idDrvPtr))
#define DEV_READ(driveno, sector, buffer, count) (\
                        mmc_read(driveno, sector, buffer, count))
#define DEV_WRITE(driveno, sector, buffer, count) (\
                        mmc_write(driveno, sector, buffer, count))
#define DEV_SECTORS_ERASE(driveno, sector, count) (\
                        mmc_erase(driveno, sector, count))
#endif          /* (USE_MMC || USE_MMC_EMULATION) */


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

