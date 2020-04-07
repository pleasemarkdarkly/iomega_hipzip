//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Conti danc@iobjects.com
//.. Modification date: 6/18/2001		   					..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.				..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com						..
//........................................................................................
//........................................................................................
/**************************************************************************** 
* Name: SDCONFIG.H - HDTK tuning constants
*       
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright EBS Inc. 1996
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description: 
*   This file is included in sdapi.h. It contains kernel specific macros 
*   and structures which must be ported to the target kernel.
*
*   It also contains tuning constants.
*
****************************************************************************/

#ifndef __SDCONFIG__
#define __SDCONFIG__

#include <fs/fat/_fat_config.h>

#ifdef __cplusplus
extern "C" {
#endif


#if defined(ENABLE_MMC)
#define __NUMDRIVES ENABLE_MMC_DRIVES
#elif defined(ENABLE_ATA)
#define __NUMDRIVES ENABLE_ATA_DRIVES
#else
#error "Unknown number of physical drives, check your storage configuration"
#endif

  // each mmc device runs on a seperate controller
#define N_CONTROLLERS           (__NUMDRIVES)
#if __NUMDRIVES >= 1
#define DRIVES_PER_CONTROLLER1  1
#else
#define DRIVES_PER_CONTROLLER1  0
#endif
#if __NUMDRIVES >= 2
#define DRIVES_PER_CONTROLLER2  1
#else
#define DRIVES_PER_CONTROLLER2  0
#endif
#define TOTAL_DRIVES            (__NUMDRIVES)

/* ======================== User tunable section ========================= */

/* Number of controllers to support 1, 2, ... */
/* Currently 2 is the maximum for SanDisk ATA support. With a minor
** modification in IOCONST.C, the HDTK can support to n controllers.
**
** For SPI, this is the number of SPI controllers. Normally 1.
** For MMC, this is the number of MMC controller. Normally 1.
*/ 
  //#define N_CONTROLLERS   1


/* Number of devices connecting to the first controller */
/* For SPI, the number of devices varies, the max. value can be 4.
** For MMC, the number of devices varies, the max. value can be 4.
*/
  //#define DRIVES_PER_CONTROLLER1  1


/* Number of devices connecting to the second controller
** For SPI, the number of devices varies, the default is 0.
** For MMC, the number of devices varies, the default is 0.
*/
  //#define DRIVES_PER_CONTROLLER2  0

/* Total number of drives */
  //#define TOTAL_DRIVES       (DRIVES_PER_CONTROLLER1 + DRIVES_PER_CONTROLLER2)




/****************************************************************************/
/*************************** FILE SYTEM SECTION *****************************/
/****************************************************************************/

/*
-------------------------------------------------------------------- 
** The HDTK supports the File System or the Peripheral Bus
** Interface.  It is configurable to use the File System as a 
** way to access data at high level to SanDisk storage devices or 
** the Peripheral Bus Interface to access directly to the storage
** devices without any File System calls.
** The USE_FILE_SYSTEM is the option to allow such feature existing
** in the HDTK. The use of USE_FILE_SYSTEM is explained below.
**      USE_FILE_SYSTEM 1       Enable  the File System
**      USE_FILE_SYSTEM 0       Disable the File System
--------------------------------------------------------------------
*/ 
#define USE_FILE_SYSTEM 1       /* If 1 use the file system */

/*
-------------------------------------------------------------------- 
** If the File System is enable,  the following File System
** options are supported.
**      RTFS_SHARE
**      RTFS_SUBDIRS
**      RTFS_WRITE 
**      NUM_USERS
**      NUSERFILES 
**      NBLKBUFFS
**      FAT_BUFFER_SIZE
--------------------------------------------------------------------
*/
 
#if (USE_FILE_SYSTEM)

/* The following constants when set to 0 selectively exclude
** portions of the File System to save executable image size.
*/

/* RTFS_SHARE option allows files to be shared. It is defined as
** enable = 1 or disable = 0.
**        RTFS_SHARE      1       Enable  file sharing
**        RTFS_SHARE      0       Disable file sharing
** Note that when set to 0 to disable file share modes saves 
** about .5 K of executable image file.
*/
#define RTFS_SHARE      1


/* Sub-directory is a feature supports by the File System. Enable
** or disable this feature is done via RTFS_SUBDIR.
** RTFS_SUBDIR is defined as follows.
**        RTFS_SUBDIR     1       Enable  sub-directory support
**        RTFS_SUBDIR     0       Disable sub-directory support
** Note that set RTFS_SUBDIR to 0 to disable sub-directory support 
** saves about 2.5 K of executable image file.
*/
#define RTFS_SUBDIRS    1


/* The File System supports WRITE PROTECTED feature.  This feature
** is selected or deselected by the RTFS_WRITE option.
**   When RTFS_WRITE is set to 1,  writing to the device is permitted.
**   When RTFS_WRITE is set to zero, the File System will prevent any
**   writing to the device.
** Note that setting RTFS_WRITE to 0 to disable write support saves
** about 5.5 K of executable image file.
*/
#define RTFS_WRITE      1

/* Maximum # of tasks that may use the file system. 
** If this constant is larger than 1,  multitasking service
** must be implemented.  The File System uses MACROs in pckernel.h
** for hooking the multitasking service routines.
*/ 
#define NUM_USERS       NUM_FAT_USERS


/* Number of blocks in the buffer pool. Uses 532 bytes per block. 
** Impacts performance during directory traversals.  The value
** must be at least 1.  The buffer pool is shared by all drives
** in the system.  Note that higher value will improve disk
** traversal performance but also increase RAM footage.
*/
#define NBLKBUFFS       NUM_FAT_BLOCK_BUFFS


/* Maximum number of open files.  This is the number of simultaneous
** files that may be opened at one time.
*/
#define NUSERFILES      NUM_FAT_USER_FILES      /* Maximum Number of open Files */


/* Size of the internal FAT buffer in 512 byte
** chunks (i.e. 12 = 12 * 512 bytes). We statically allocate arrays
** based on this in pc_memry.c. 
** Small values should be OK in the SanDisk environment since 
** IO is fast. Note that reducing the Buffer size will increase 
** the amount of FAT flushing. This value must be at least 1.
*/
#define FAT_BUFFER_SIZE NUM_FAT_TABLE_BUFFS	/* This is the size of the FAT table on a Clik disk */


#endif  /* USE_FILE_SYSTEM */


#define USE_CONTIG_IO   1       /* if 1 use 16-byte contiguous register range */ 

/*
NOTE: In Big Endian(Motorola), 16-bit data bus SHOULD be swapped for ATA environment.
       Card D15-D8=Host D7-D0 and Card D7-D0=Host D15-D8.
*/
#define USE_HW_OPTION   0       /* Must be set to 1 for Motorola ATA environment */


/* In IO Mode, the WORD_ACCESS_ONLY is valid for Data Register accessing only.
** If set, selected WORD Data Register accessing; otherwise, BYTE accessing.
**
** In Memory Mapped Mode, the WORD_ACCESS_ONLY selects Word accessing to
** ALL Task File Registers.
** If set, selected WORD Task File Register accessing; otherwise, BYTE mode.
*/
#define WORD_ACCESS_ONLY        1       /* if 1 Word accessing else Byte */



/* This option supports data handler or event management interrupts.
**      0 to use polling mode.
**      1 to use interrupt mode for both data and event handlers.
*/
#define USE_INTERRUPTS  0       /* If 1 use interrupt mode, else polling */


/* This option selects different addressing scheme.
**      1 use only LBA (Logical Block Addressing),
**      0 CHS (Cylinder, Head, Sector Addressing).
** Since SanDisk products is configured in LBA mode,  the default for
** this option is 1.
*/
#define USE_ONLY_LBA    1

#if (USE_MEMMODE)
#undef USE_CONTIG_IO
/* Regardless the type of the peripheral buses,  when USE_MEMMODE
** is set to one,  USE_CONTIG_IO is always on to map ATA Task File Registers
** to 16-byte contiguous memory address spaces.
*/
#define USE_CONTIG_IO           1
#endif

/* ======================  PROCESSOR CONFIGURATION  ======================== */

/* This feature allows the processor that does not have byte accessing
*  capability, i.e. byte access is the same as word access. Byte access
*  is a 16-bit access.
*  The syntax is defined as follows:
*       CHAR_16BIT      1       Enable byte access as word access.
*       CHAR_16BIT      0       Disable byte access as word access.
*  This is a special case and handled by the File system.
*/
#define CHAR_16BIT      0


/* If set to 1 assume an intel byte ordering scheme. This eliminated 
* byte swapping and improves performance somewhat.
* If set to 0 bytes will be converted to big endian in appropriate
* places.
*/
//#include <sys/types.h>
//#include <sys/endian.h>
#if !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN  1
#endif

/* Always expect a large model pointer */
#if (WORD_ACCESS_ONLY)
  typedef unsigned short * USERADDRESS;
#else
  typedef unsigned char  * USERADDRESS;
#endif

typedef unsigned char   *   FPTR;
typedef unsigned char   *   FPTR08;
typedef unsigned short  *   FPTR16;
typedef unsigned long   *   FPTR32;


/* Error codes if an low level function fails. */
#define BUS_ERC_DIAG       101 /* Drive diagnostic failed in initialize */
#define BUS_ERC_ARGS       102 /* User supplied invalid arguments */
#define BUS_ERC_DRQ        103 /* DRQ should be asserted but it isn't
				  or driver and controller are out of phase*/
#define BUS_ERC_TIMEOUT    104 /* Timeout during some operation */
#define BUS_ERC_STATUS     105 /* Controller reported an error
				  look in the error register */
#define BUS_ERC_ADDR_RANGE 106 /* LBA out of range */
#define BUS_ERC_CNTRL_INIT 107 /* Fail to initialize controller_s structure */
#define BUS_ERC_IDDRV      108 /* Identify drive info error */
#define BUS_ERC_CMD_MULT   109 /* Read/Write Multiple Command attempts
			       * to run before Set Multiple Command
			       * has been executed */
#define BUS_ERC_BASE_ADDR  110 /* Base Address not Available */
#define BUS_ERC_CARD_ATA   111 /* Card is not ATA */



#ifdef __cplusplus
}
#endif

#endif  /* SDCONFIG.H */
