//........................................................................................
//........................................................................................
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 10/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
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


#ifdef __cplusplus
extern "C" {
#endif


/* ======================== User tunable section ========================= */

/* Number of controllers to support 1, 2, ... */
/* Currently 2 is the maximum for SanDisk ATA support. With a minor
** modification in IOCONST.C, the HDTK can support to n controllers.
**
** For SPI, this is the number of SPI controllers. Normally 1.
** For MMC, this is the number of MMC controller. Normally 1.
*/ 
#define N_CONTROLLERS   1


/* Number of devices connecting to the first controller */
/* For SPI, the number of devices varies, the max. value can be 4.
** For MMC, the number of devices varies, the max. value can be 4.
*/
#define DRIVES_PER_CONTROLLER1  1


/* Number of devices connecting to the second controller
** For SPI, the number of devices varies, the default is 0.
** For MMC, the number of devices varies, the default is 0.
*/
#define DRIVES_PER_CONTROLLER2  0

/* Total number of drives */
#define TOTAL_DRIVES       (DRIVES_PER_CONTROLLER1 + DRIVES_PER_CONTROLLER2)




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
#define RTFS_SHARE      0


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
#define NUM_USERS       1


/* Number of blocks in the buffer pool. Uses 532 bytes per block. 
** Impacts performance during directory traversals.  The value
** must be at least 1.  The buffer pool is shared by all drives
** in the system.  Note that higher value will improve disk
** traversal performance but also increase RAM footage.
*/
#define NBLKBUFFS       1


/* Maximum number of open files.  This is the number of simultaneous
** files that may be opened at one time.
*/
#define NUSERFILES      8      /* Maximum Number of open Files */


/* Size of the internal FAT buffer in 512 byte
** chunks (i.e. 12 = 12 * 512 bytes). We statically allocate arrays
** based on this in pc_memry.c. 
** Small values should be OK in the SanDisk environment since 
** IO is fast. Note that reducing the Buffer size will increase 
** the amount of FAT flushing. This value must be at least 1.
*/
#define FAT_BUFFER_SIZE 1


#endif  /* USE_FILE_SYSTEM */


/****************************************************************************/
/*************************** INTERFACE SECTION ******************************/
/****************************************************************************/

/*
--------------------------------------------------------------------
** There are four different Peripheral Bus Interfaces that the
** HDTK supports.  They are:
**      SPI
**      MMC
**      SPI EMULATION
**      MMC EMULATION
** There is only one Peripheral Bus Interface enabled at one time. 
** The use of these options is very simple,  either it is enabled
** by setting to 1 or disabled by setting to zero.
**      USE_SPI         1       Enable  SPI interface
**      USE_SPI         0       Disable SPI interface
**
**      USE_MMC         1       Enable  MMC interface
**      USE_MMC         0       Disable MMC interface
** and so on...
--------------------------------------------------------------------
*/
#define USE_TRUE_IDE    1       /* If 1 use IDE interface */
#define USE_PCMCIA      0       /* If 1 use PCMCIA interface */ 
#define USE_SPI         0       /* If 1 use SPI interface */
#define USE_MMC         0       /* If 1 use MMC interface */

/* if 1 use SPI or MMC Emulation */
#define USE_SPI_EMULATION       0
#define USE_MMC_EMULATION       0


/* 
** When the Peripheral Bus Interface is selected, the following
** options are allowed you to enable different features in the
** Peripheral Bus Interface to take the advantage of SanDisk
** products.
** Note that these options are available depending on the Peripheral
** Bus Interface, and may have different meanings.
**      USE_MEMMODE
**      USE_CONTIG_IO 
**      USE_HW_OPTION   ***for Motorola ATA environment ONLY***
**      WORD_ACCESS_ONLY  
**      USE_INTERRUPTS
**      USE_ONLY_LBA
**      USE_MULTI
**      USE_SET_FEATURES
**      PREERASE_ON_DELETE
**      PREERASE_ON_ALLOC
**      PREERASE_ON_FORMAT
*/ 

/* The USE_MEMMODE provides a way to configure the HDTK in memory
** mapped or IO mapped mode of your platform. The use of USE_MEMMODE
** is shown below:
**      USE_MEMMODE     1       Enable memory mapped mode.
**      USE_MEMMODE     0       Select IO mapped mode.
*/
#define USE_MEMMODE     1       /* if 1 use memory mapped mode Else IO */ 


/* The USE_CONTIG_IO allows the HDTK to map the ATA Task File Registers
** in any selected contiguous 16 byte IO address spaces.
** The definition of USE_CONTIG_IO is described as follows:
**
**      USE_CONTIG_IO   1       Allow IO accessing in any 16-byte
**                               contiguous IO address spaces.
**      USE_CONTIG_IO   0       IO accessing in 2 different IO regions.
**                              For example,
**                                Standard PC-AT IDE disk IO address spaces:
**                                1F0h-1F7h, 3F6-3F7 (Primary   IO) 
**                                170h-177h, 376-377 (Secondary IO)
*/
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


/* This constant when set, enable Read and Write Multiple Commands
**      0       Disable write multiple command.
**      1       Enable write multiple command.
** NOTE: SanDisk supports one sector per block max, so there is no gain.
*/
#define USE_MULTI       0


/* This option allows the use of SanDisk proprietary Set Feature Command.
**      0       Disable SanDisk Set Feature Command.
**      1       Enable Sandisk Set Feature Command.
**  (tradeoff between current drawn and read/write speed)
**  Refer to SanDisk Product Manual for detail of tradeoff.
**  If this option is set to 1, then the IDE_FEATURE_SETPERF_VALUE in
**  ATADRV.H should also be set accordingly for the tradeoff.
*/
#define USE_SET_FEATURES        0


/* If 1, pre-erase sectors when files are deleted. */
#define PREERASE_ON_DELETE      0


/* If 1, pre-erase allocated clusters when files are extended. */
#define PREERASE_ON_ALLOC       0


/* If 1, pre-erase sectors when volume is formatted. */
#define PREERASE_ON_FORMAT      0


/* In memory mapped mode,  a contiguous I/O register region is reffered in
** places where the alternate status register is accessed.
** It's convenient to use the contiguous I/O mode code.  Thus, enabling
** memory mode also turns on this one, but it has no other affects.
** This is the preferred choice and NOT intended to be CHANGED. 
*/
#if (USE_MEMMODE)
#undef USE_CONTIG_IO
/* Regardless the type of the peripheral buses,  when USE_MEMMODE
** is set to one,  USE_CONTIG_IO is always on to map ATA Task File Registers
** to 16-byte contiguous memory address spaces.
*/
#define USE_CONTIG_IO           1
#endif


/* Make sure one of the Peripheral Bus Interfaces is selected. */
#if (! (USE_TRUE_IDE || USE_PCMCIA || USE_SPI || USE_SPI_EMULATION || USE_MMC || USE_MMC_EMULATION) ) 
error Interface not defined.
#endif


#if (!USE_FILE_SYSTEM)
/* Since the low level storage driver does not have any restriction
** on writing,  the RTFS_WRITE option must always set when use without
** the FILE SYSTEM.
** This is NOT intended to be CHANGED. Otherwise, writing to the device
** is prohibited.
*/
#ifndef RTFS_WRITE
#define RTFS_WRITE      1
#endif
#endif


/* This option supports power management.
**      Set to 0 will cause the drive to go to sleep after 5ms of inactivity.
**      None zero value will be multiple of 5ms to stay in idle.
**      e.g. USE_PWR_MGMT 5  (device goes to sleep after 25ms of inactivity)
*/
#define USE_PWR_MGMT 0  /* Default of 5ms to go to sleep */


/*****************************************************************************/
/****************************** IDE SECTION **********************************/
/*****************************************************************************/

#if (USE_TRUE_IDE)

#if (USE_MEMMODE)      /* Memory mapped mode is preferred */
#ifdef _IOME_COMDEX_
#define ATA_PRIMARY_MEM_ADDRESS   0x50000000 /* First memory base address */
#define ATA_SECONDARY_MEM_ADDRESS 0x50000000 /* Second memory base address */
#else
#define ATA_PRIMARY_MEM_ADDRESS   0x50000800 /* First memory base address */
#define ATA_SECONDARY_MEM_ADDRESS 0x50000800 /* Second memory base address */
#endif
#else
/* For native I/O mapped mode */ 
#define ATA_PRIMARY_IO_ADDRESS   0x170  /* First I/O base address */
#define ATA_SECONDARY_IO_ADDRESS 0x1f0  /* Second I/O base address */
#endif
 
#if (USE_INTERRUPTS)
#define ATA_PRIMARY_INTERRUPT   IRQ_15  /* First IRQ channel */
#define ATA_SECONDARY_INTERRUPT IRQ_14  /* Second IRQ channel */
#else
#define ATA_PRIMARY_INTERRUPT   -1      /* No IRQ */
#define ATA_SECONDARY_INTERRUPT -1 
#endif

#endif  /* (USE_TRUE_IDE) */


/*****************************************************************************/
/****************************** PCMCIA section *******************************/
/*****************************************************************************/

#if (USE_PCMCIA)


#if (USE_CONTIG_IO)
/* Contiguous IO region  */
/* Default for pcmcia environment when primary & secondary are in use */
#define ATA_PRIMARY_IO_ADDRESS   0x380
#define ATA_SECONDARY_IO_ADDRESS 0x3A0
#if (N_CONTROLLERS > 1)
#define ATA_THIRD_IO_ADDRESS     0x580
#define ATA_FOURTH_IO_ADDRESS    0x5A0
#endif
#define ATA_PRIMARY_CFGOPT       0x41
#define ATA_SECONDARY_CFGOPT     0x41
#else   /* (! USE_CONTIG_IO) */
/* Flipped primary + secondary for pcmcia environment */ 
#define ATA_PRIMARY_IO_ADDRESS   0x170 
#define ATA_SECONDARY_IO_ADDRESS 0x1F0 
#define ATA_PRIMARY_CFGOPT      0x43
#define ATA_SECONDARY_CFGOPT    0x42
#endif  /* (USE_CONTIG_IO) */


#if (USE_INTERRUPTS)
#if (USE_CONTIG_IO)
#define ATA_PRIMARY_INTERRUPT   IRQ_10 
#define ATA_SECONDARY_INTERRUPT IRQ_10 
#else
#define ATA_PRIMARY_INTERRUPT   IRQ_15 
#define ATA_SECONDARY_INTERRUPT IRQ_14 
#endif
#else
#define ATA_PRIMARY_INTERRUPT   -1
#define ATA_SECONDARY_INTERRUPT -1 
#endif  /* (USE_INTERRUPTS) */

/* IO spaces 0x3E0 and 0x3E1 are Index and Data registers respectively
** for PCIC Controllers, such as Intel 82X65, Vadem VG-X65,
** and Cirrus Logic PD67XX families.
*/
#define PCIC_BASE       0x3E0
#define ADDR_INDEX      0x0   /* PCIC Index register io space.  */
#define ADDR_DATA       0x1   /* PCIC Data  register io space.  */ 

#define PCMCIA_CIS_SPACE 0x0L   /* start address of CIS in CIS window */ 
#define CIS_WINDOW      0       /* memory window # for CIS */

#if (WORD_ACCESS_ONLY)  /* 16-bit access */
#define _16BIT_WINDOW_1   1     /* memory window # for 16 bit accesses
				   in memory mapped mode for socket 0.
				*/
#define _16BIT_WINDOW_2   2     /* memory window # for 16 bit accesses
				   in memory mapped mode for socket 1.
				*/
#if (N_CONTROLLERS > 1)
#define _16BIT_WINDOW_3   3     /* memory window # for 16 bit accesses
				   in memory mapped mode for socket 0.
				*/
#define _16BIT_WINDOW_4   4     /* memory window # for 16 bit accesses
				   in memory mapped mode for socket 1.
				*/
#endif
#else   /* 8-bit access */
#define _8BIT_WINDOW_1    1     /* memory window # for 8 bit accesses
				   in memory mapped mode for socket 0.
				*/
#define _8BIT_WINDOW_2    2     /* memory window # for 8 bit accesses
				   in memory mapped mode for socket 1.
				*/
#if (N_CONTROLLERS > 1)
#define _8BIT_WINDOW_3    3     /* memory window # for 8 bit accesses
				   in memory mapped mode for socket 0.
				*/
#define _8BIT_WINDOW_4    4     /* memory window # for 8 bit accesses
				   in memory mapped mode for socket 1.
				*/
#endif
#endif

#define IO_WINDOW_0     0       /* IO window # for socket 0, 1 */
#define IO_WINDOW_1     1

/* Constants used by pcmctrl.c */
/* MEM_WINDOW_0 - Used to access the
   CIS window of socket 0 or 1.
				   */
#define MEM_WINDOW_0  0xC8000L     /* HOST 24 bit physical address */ 
#define MEM_ADDRESS_0 0xC8000000   /* HOST 24 bit physical address */
/* MEM_WINDOW_1 - Used to access the
				      Common Memory window of socket 0
				      (by default) in Memory Mapped Mode.
				   */
#define MEM_WINDOW_1  0xC9000L     /* HOST 24 bit physical address */ 
#define MEM_ADDRESS_1 0xC9000000   /* HOST 24 bit physical address */
/* MEM_WINDOW_2 - Used to access the
				      Common Memory window of socket 1
				      (by default) in Memory Mapped Mode.
				   */
#define MEM_WINDOW_2  0xCA000L     /* HOST 24 bit physical address */ 
#define MEM_ADDRESS_2 0xCA000000   /* HOST 24 bit physical address */ 

/* 
** Note: The offset at 0x400 in Common Memory window in Memory Mapped Mode
**       is the 512-byte Data Buffer area. This enables the use of block
**       move instruction of word or byte for faster accress.
*/         
				   /* MEM_WINDOW_3, MEM_WINDOW_4,
				      MEM_WINDOW_5 are available for
				      use of PCIC Controller.
				   */
#define MEM_WINDOW_3  0xCB000L     /* HOST 24 bit physical address */
#define MEM_ADDRESS_3 0xCB000000   /* HOST 24 bit physical address */
#define MEM_WINDOW_4  0xCC000L     /* HOST 24 bit physical address */ 
#define MEM_ADDRESS_4 0xCC000000   /* HOST 24 bit physical address */
#define MEM_WINDOW_5  0xCD000L     /* HOST 24 bit physical address */ 
#define MEM_ADDRESS_5 0xCD000000   /* HOST 24 bit physical address */


#endif  /* USE_PCMCIA */


/*****************************************************************************/
/****************************** SPI SECTION **********************************/
/*****************************************************************************/

#if (USE_SPI || USE_SPI_EMULATION)

/*********************************** NATIVE SPI ******************************/
 #if (USE_SPI)

  #if (USE_MEMMODE)
	/* 68328 Processor memory base address */
	#define SPI_BASE_ADDR   0xFFF000
	#define SPI_PRIMARY_MEM_ADDRESS         0x0
	#define SPI_SECONDARY_MEM_ADDRESS       0x0


	/* 68328 SPI Master control & Data registers */
	/* Offset from the Memory Base Address. */
	#define SPIM_OFF_DATAOUT 0x800   /* Master Data Out */
	#define SPIM_OFF_DATAIN 0x800   /* Master Data In */
	#define SPIM_OFF_CNTL   0x802
	#define SPIM_OFF_STAT   0x802

	/* Default value for SPI to control Speed, Bus Width, IRQ, Polarity,..*/
	#define SPI_CNTL_DEFAULT        0x6247


	/* SPI Control information */
	#define SPIM_IRQ_BIT    0x0080
	#define SPIM_IRQ_MASK   0xFF7F
	#define SPIM_XCH_BIT    0x0100  /* Transfer data bit */
	#define SPIM_XCH_MASK   0xFEFF
	#define SPIM_DEN_BIT    0x0200  /* Data Enable Bit */
	#define SPIM_DEN_MASK   0xFDFF


	/* Status information */
	#define SPIM_END_TRANSACTION    0x0080
	#define SPIM_XFER_ERRORS        0x0000  /* There is no error definition */
						/* for the SPI master */


	/* Enable and Disable SPI Bus Master. These are used by the SPI driver. */
	/* Provide for 16-bit and 8-bit SPI bus per READ/WRITE ACCESS */

   #if (WORD_ACCESS_ONLY)
	/* Enable SPI Bus for 16-bit transaction */
	#define SPIM_ENABLE_BUS16       (SPI_CNTL_DEFAULT+8)
   #endif

	/* Enable SPI Bus for 8-bit transaction */
	#define SPIM_ENABLE_BUS8        SPI_CNTL_DEFAULT

	/* Disable SPI Bus */
	#define SPIM_DISABLE_BUS        (SPIM_DEN_MASK & SPIM_IRQ_MASK & SPIM_XCH_MASK)


	/* For SPI device Chip Select (CS should be low for the device to be active)*/
	#define SPIM_CS_MASK    0x08    /* For SPI device CS */
	#define SPIM_CS_REG     0x441


	/* 68328 Port K. Port K for SPI Master Configuration. */
	/* Offset from the Memory Base Address. */
	#define PK_OFF_DIR      0x440
	#define PK_OFF_DATA     0x441
	#define PK_OFF_PULLUP   0x442
	#define PK_OFF_SELECT   0x443


  #else         /* IO MODE */

	/* SPI controller in IO mode */ 
	#define SPI_PRIMARY_IO_ADDRESS   0x170 
	#define SPI_SECONDARY_IO_ADDRESS 0x180

  #endif        /* USE_MEMMODE */
 
 #endif         /* USE_SPI */

/****************************** SPI EMULATION ********************************/
 #if (USE_SPI_EMULATION)
  
  #if (USE_MEMMODE)      
	/* Good for M32R processor serial emulation */ 
	#define SPI_BASE_ADDR   0xFF000000

	#define SPI_PRIMARY_MEM_ADDRESS  0x04400        /* Offset from base address */
	#define SPI_SECONDARY_IO_ADDRESS 0x00000

	#define SPICsio1Control    (SPI_PRIMARY_MEM_ADDRESS+0x00000001)
	#define SPICsio1Status     (SPI_PRIMARY_MEM_ADDRESS+0x00000003)
	#define SPICsio1TxData     (SPI_PRIMARY_MEM_ADDRESS+0x00000007)
	#define SPICsio1RxData     (SPI_PRIMARY_MEM_ADDRESS+0x00000009)
	#define SPICsio1IntMask    (SPI_PRIMARY_MEM_ADDRESS+0x0000000b)
	#define SPICsio1BaudRateCount      (SPI_PRIMARY_MEM_ADDRESS+0x0000000f)


	#define PORTJ_OFFSET    0x07000 /* Chip Select */
	#define CS_SERON        0x00    /* Turn on CS signal. Select the chip */
	#define CS_SEROFF       0x01    /* Turn off CS signal. Deselect the chip */

	#define PPort2          (PORTJ_OFFSET+0x00000003)
	#define PPort3          (PORTJ_OFFSET+0x00000009)

	#define PPortDir2       (PORTJ_OFFSET+0x00000006)
	#define PPortDir3       (PORTJ_OFFSET+0x0000000C)
  
  #else
	/* Good for SPI emulation of parallel controller */ 
	#define SPI_PRIMARY_IO_ADDRESS   0x378 
	#define SPI_SECONDARY_IO_ADDRESS 0x278

	#define PPDATA_REG      0x00
	#define PPCTRL_REG      0x02
	#define PPSTAT_REG      0x01

  #endif

 #endif  /* USE_SPI_EMULATION */

 #if (USE_INTERRUPTS)
	#define SPI_PRIMARY_INTERRUPT   IRQ_07 
	#define SPI_SECONDARY_INTERRUPT IRQ_05 
 #else
	#define SPI_PRIMARY_INTERRUPT   -1
	#define SPI_SECONDARY_INTERRUPT -1 
 #endif


#endif  /* USE_SPI || USE_SPI_EMULATION */


/*****************************************************************************/
/****************************** MMC SECTION **********************************/
/*****************************************************************************/

#if (USE_MMC || USE_MMC_EMULATION)

 #if (USE_MMC)

  #if (USE_MEMMODE)
	/* Processor memory base address */
	#define MMC_BASE_ADDR   0xFFF000

  #else
	/* MMC controller IO addresses */ 
	#define MMC_PRIMARY_IO_ADDRESS   0x170 
	#define MMC_SECONDARY_IO_ADDRESS 0x180
 
#define REV_NEW         0x310
#define REV_BORDER      0x301


#define FIFO_LENGTH     0x08


/* Register definitions */
#define STR_STP_CLK_REG         0x00    /* start stop clock */
#define STATUS_REG              0x02    /* Status */
#define CLK_RATE_REG            0x04    /* MMC Clock Rate */
#define REVISION_REG            0x06    /* Revision information */
#define SPI_REG                 0x08    /* SPI control */
#define CMD_DAT_CONT_REG        0x0A    /* Command data control */
#define RESPONSE_TOUT_REG       0x0C    /* Response time out */
#define READ_TOUT_REG           0x0E    /* Read time out */
#define BLK_LEN_REG             0x10    /* Block length register */
#define NOB_REG                 0x12    /* Number of blocks to transfer */
#define PWR_REG                 0x14    /* Power */
#define TEST_REG                0x16    /* Test */
#define TEST_CLK_COUNT_REG      0x18    /* Test Clock count */
#define INT_MASK_REG            0x1A    /* Interrupt Mask */
#define CMD_REG                 0x1C    /* Command Index */
#define ARGUMENT_HI_REG         0x1E    /* Argument High word */
#define ARGUMENT_LO_REG         0x20    /* Argument Low word */
#define RES_FIFO_REG            0x22    /* Response read address location */
#define FIFO_OE_REG             0x24    /* */
#define FIFO_RD_WR_REG          0x26    /* */
#define BUF_PART_FULL_REG       0x28    /* Buffer partial full */



/* START_STOP CLOCK register definitions */
#define STOP_CLOCK              0x01    /* Disable the clock */
#define START_CLOCK             0x02    /* Enable the clock */


/* STATUS register definitions (READ ONLY) */
#define TIME_OUT_RECEIVE        0x0001  /* TIME_OUT_RCVD - Time out receive data */
#define TIME_OUT_RESPONSE       0x0002  /* TIME_OUT_RES - Time out response */
#define CRC_WR_ERR              0x0004  /* CRC_WR_ERR - CRC write error */
#define CRC_RD_ERR              0x0008  /* CRC_RD_ERR - CRC read error */
#define ERR_CRC_NO_RESP         0x0010  /* ERR_CRC_NO_RESP - No response CRC */
#define RESP_CRC_ERR            0x0020  /* RESP_CRC_ERR - Response CRC error */
#define FIFO_BUFFER_EMPTY       0x0040  /* EFB - Empty fifo1 */
#define FIFO_BUFFER_FULL        0x0080  /* FFB - Full fifo1 */
#define CLOCK_DISABLE           0x0100  /* CLOCK_ENABLE - Clock enable */
#define RESERVE_EFB2            0x0200  /* EFB2 - Empty fifo2 */
#define RESERVE_FFB2            0x0400  /* FFB2 - Full fifo2 */
#define RD_DATA_AVAILABLE       0x0800  /* READ  -  Data available */
#define DONE_WDATA_XFER         0x0800  /* WRITE -  Finish WRITE request */
#define DONE_PROG_RDWR          0x1000  /* For write, End of programming data */
#define END_CMD_RES             0x2000  /* END_CMD_RES - End command response */
#define RESERVE_FIFO1_PAEN      0x4000  /* FIFO1_PAEN or FIFO1_PAFN */
#define RESERVE_FIFO2_PAEN      0x8000  /* FIFO2_PAEN or FIFO2_PAFN */

/* CLOCK RATE definitions */
#define CLK_RATE_FULL           0x00    /* Master clock */
#define CLK_RATE_HALF           0x01    /* 1/2 Master clock */
#define CLK_RATE_FOUR           0x02    /* 1/4 Master clock */
#define CLK_RATE_EIGHT          0x03    /* 1/8 Master clock */
#define CLK_RATE_SIXTEEN        0x04    /* 1/16 Master clock */
#define CLK_RATE_THIRTY2        0x05    /* 1/32 Master clock */
#define CLK_RATE_SIXTY4         0x06    /* 1/64 Master clock */


/* SPI register definitions */
#define SPI_ENABLE              0x01
#define SPI_CRC_ON              0x02
#define SPI_CS_ENABLE           0x04
#define SPI_CS_ADDR0            0x08
#define SPI_CS_ADDR1            0x10
#define SPI_CS_ADDR2            0x18
#define SPI_CS_ADDR3            0x20


/* CMD_DAT_CONT register definitions */
#define RESPONSE_TYPE_R0        0x00
#define RESPONSE_TYPE_R1        0x01
#define RESPONSE_TYPE_R2        0x02
#define RESPONSE_TYPE_R3        0x03
#define DATA_ENABLE             0x04
#define DATA_READ_SET           0x00
#define DATA_WRITE_SET          0x08
#define DATA_STREAM_BLK         0x10
#define BUSY_SET                0x20
#define SEND_80_CLOCKS          0x40



/* Response Time Out register definitions */
#define DEFAULT_RESPONSE_TOUT   0xFF

/* Read Time Out Register definitions */
#define DEFAULT_READ_TOUT       0xFFFF

/* Block Length register definitions */
#define DEFAULT_BLK_LENGTH      0x200   /* 512-byte block length */

/* Number of Block register definition */
#define DEFAULT_NOB             0x01


/* Interrupt Mask Register definitions */
#define DATA_TRANSFER_INT       0x01
#define PROGPRAM_DONE_INT       0x02
#define CMD_RESPONSE_INT        0x04
#define BUFF_READY_INT          0x08


/* Buffer Partial Full register */
#define BUFF_PARTIAL_FULL       0x01



  #endif

 #endif         /* USE_MMC */

 
 #if (USE_MMC_EMULATION)
  
  #if (USE_MEMMODE)

	/* Processor memory base address */
	#define MMC_BASE_ADDR   0xFFF000
  
  #else      
	/* Good for MMC Emulation in parallel controller */ 
	#define MMC_PRIMARY_IO_ADDRESS   0x378 
	#define MMC_SECONDARY_IO_ADDRESS 0x278

	#define PPDATA_REG      00
	#define PPCTRL_REG      02
	#define PPSTAT_REG      01
  #endif
 
 #endif         /* USE_MMC_EMULATION */

 #if (USE_INTERRUPTS)
	#define MMC_PRIMARY_INTERRUPT   IRQ_07 
	#define MMC_SECONDARY_INTERRUPT IRQ_05 
 #else
	#define MMC_PRIMARY_INTERRUPT   -1
	#define MMC_SECONDARY_INTERRUPT -1 
 #endif


#endif  /* (USE_MMC || USE_MMC_EMMULATION) */


/****************************** INTERRUPT VECTOR *****************************/
#define IRQ_15          0x0F    /* IRQ 15 */
#define IRQ_14          0x0E    /* IRQ 14 */
#define IRQ_12          0x0C    /* IRQ 12 */
#define IRQ_11          0x0B    /* IRQ 11 */
#define IRQ_10          0x0A    /* IRQ 10 */
#define IRQ_07          0x07    /* SPI, MMC IRQ7 */
#define IRQ_05          0x05    /* SPI, MMC IRQ5 */


/* Make sure number of controllers is selected */
#if (!N_CONTROLLERS)
error !!!!!  YOUR SETTING IS NOT CORRECTLY CONFIGURED  !!!!!
error -----   Please check your N_CONTROLLERS setting  -----
#endif

#if (!DRIVES_PER_CONTROLLER1 && !DRIVES_PER_CONTROLLER2)
error -----   Please check your DRIVES_PER_CONTROLLER setting  -----
#endif


/*========================= END TUNABLE CONSTANTS ===========================*/


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
#define LITTLE_ENDIAN  1

/* ====================  MEMORY MANAGEMENT METHODOLOGY  ==================== */

#ifdef __TURBOC__
#define SDFAR far                 /* allocated in "far" memory. This is */ 
#else                           /* only relevent in intel small and   */ 

#ifdef _MSC_VER                 /* medium model code. */ 
#define SDFAR far     
#else

#define SDFAR                     /* For other processors */
#endif

#endif

/* Always expect a large model pointer */
#if (WORD_ACCESS_ONLY)
  typedef unsigned short * USERADDRESS;
#else
  typedef unsigned char  * USERADDRESS;
#endif

typedef unsigned char   SDFAR *   FPTR;
typedef unsigned char   SDFAR *   FPTR08;
typedef unsigned short  SDFAR *   FPTR16;
typedef unsigned long   SDFAR *   FPTR32;


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


/* MMC completion code, the return value of most of the MMC driver fucntions */
typedef enum _MMCERROR
{
	MMC_NO_ERROR = 0,               /* 0- Successful completion */
	MMC_CARD_IS_NOT_RESPONDING=201, /* 201- Time out error on card response */
	MMC_CMD_CRC_ERROR,              /* 202- CRC error detected on card response */
	MMC_DATA_CRC_ERROR,             /* 203- CRC error detected on incoming data block */
	MMC_DATA_STATUS_CRC_ERROR,      /* 204- Card is reporting CRC error on outgoing data block. */
	MMC_CARD_IS_BUSY,               /* 205- Card is busy programming */
	MMC_CARD_IS_NOT_READY,          /* 206- Card did not complete its initialization and is not ready. */
	MMC_COMUNC_CRC_ERROR,           /* 207- Card is reporting CRC error */
	MMC_COMUNC_ILLEG_COM,           /* 208- Card is reporting illegal command */
	MMC_ERASE_PARAM,                /* 209- Erase parameters error */
	MMC_WP_VIOLATION,               /* 210- Attempt to write a WP sector */
	MMC_ERROR,                      /* 211- MMC card internal error */
	MMC_WP_ERASE_SKIP,              /* 212- Attempt to erase WP sector */
	MMC_ADDRESS_ERROR,              /* 213- Sector messaligned error */
	MMC_CARD_READ_FAILURE,          /* 214- Card is reporting Read command failed */
	MMC_INTERFACE_ERROR,            /* 215- Error detected by the MMC HW driver */
	MMC_ILLEGAL_MODE,               /* 216- Not support in the current mode */
	MMC_COMMAND_PARAMETER_ERROR,    /* 217- Card is reporting Address-out-of-range error */
	MMC_ERASE_SEQ_ERROR,            /* 218- Error in the sequence of erase command */
	MMC_ERASE_RESET,                /* 219- Erase command canceled before execution */
	MMC_NO_CRC_STATUS,              /* 220- Time out on CRC status for Write */
	MMC_OVERRUN,                    /* 221- Overrun */
	MMC_UNDERRUN,                   /* 222- Underrun */
	MMC_CIDCSD_OVERWRITE,           /* 223- a) The CID register has been already */
					/*        written and can be overwriten. */
					/*     b) The read only section of CSD does not */
					/*        match the card content. */
					/*     c) An attempt to reverse the copy (set */
					/*        as original) or permanent WP bits was made. */
	MMC_CARD_ECC_DISABLED,          /* 224- The command has been executed without */
					/*     using the internal ECC. */
	MMC_READ_FOR_DATA,              /* 225- Corresponds to buffer empty signalling */
					/*     on the bus. */
	MMC_DATA_LENGTH_ERROR,          /* 226- Data Length more then 512 bytes. */
	MMC_TIME_OUT_RCVD,              /* 227- Time out recive data (B0 for controller) */
	MMC_OUT_OF_RANGE,               /* 228- Address out of range error */
	MMC_CARD_ECC_FAILED             /* 229- Internal Card ECC failed */
} MMC_CC;


#ifdef __cplusplus
}
#endif



#define __SDCONFIG__

#endif  /* SDCONFIG.H */
