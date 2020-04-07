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
/*****************************************************************************
* Filename: drive.h  -  Controller structure information
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Portable Peripheral Bus Interface Device driver.
*
******************************************************************************/

#ifndef _DRIVE_H_

#include <fs/fat/sdtypes.h>
#include <fs/fat/sdconfig.h>


#ifdef __cplusplus
extern "C" {
#endif



/* NOT intended to be CHANGED by user */
#define USE_PREERASE_SECTORS    (PREERASE_ON_DELETE|PREERASE_ON_ALLOC|PREERASE_ON_FORMAT)


/* Don't allow more then 128 blocks (64K) for fear of a segment wrap. */
#define MAX_BLOCKS  128


/* Current active device flag */
#define DRV_ACTIVE      0x80

/* First drive */
#define FIRST_DRIVE     0



#if (USE_PCMCIA)
#define  N_INTERFACES  TOTAL_DRIVES
#else
#define  N_INTERFACES  N_CONTROLLERS
#endif


/* Internal READ, WRITE, ERASE Command operations */
#define READING 1
#define WRITING 2
#define ERASING 3


/* drive_control structure - We use this abstraction to manage the driver. 
	Structure contents:
		.  drive description. logical drive structure (heads/secptrak/secpcyl)
		.  virtual registers: These aren't (can't be) mapped onto the
		the controller but we use virtual representation on the
		register file to drive the system.
		.  virtual output register file (to drive) - We load the registers
		and then ask lower level code to send the command block to the 
		controller.
		.  virtual input register file (from drive) - We offload the register
		file from the drive into these fields after a command complete.
		Note: We only offload the whole register file in certain cases.
			The status register is always offloaded. 
*/

/* The control structure contains one of these per drive [2]. We set this
	up at init time after issueing SET_PARAMS. The values are used to convert
	block number to track:head:sector
*/
typedef struct drive_desc
{
    ULONG       total_lba;
    UINT16      num_heads;
    UINT16      sec_p_track;
    UINT16      sec_p_cyl;
    UINT16      block_size;
    UINT16      num_cylinders;
    UINT16      max_multiple;
    UINT16      supports_lba;
} DRIVE_DESC, *PDRIVE_DESC;


typedef struct drive_control
{
	/* Geometry of the drives */
	PDRIVE_DESC drive;
	USERADDRESS user_address;/* Pointer address of the user's buffer */

	/* Address of the controller in memory or IO mode */
#if (USE_MEMMODE)
  FPTR    register_file_address;
#else
  UINT16 register_file_address;
#endif
	SDBOOL command_complete;  /* Set when operation is complete */
	UINT16 opencount;
	INT16 interrupt_number; /* -1 if not being used */
	COUNT controller_number;

	UINT16 error_code;      /* Set if an error occured - This is 
				   the error reported to the user.
				*/
	UINT16 timer;           /* Command fails if this much time elapsed */
	UINT16 block_size;      /* Block size for read/write multiple cmds */
	UCOUNT sectors_remaining;/* Sectors remaining for read/write mult */

	INT16 drive_active;     /* Current active drive */

	/* Virtual output register file - we load these and then 
	   send them to the device.
	*/

#if (USE_MMC || USE_MMC_EMULATION || USE_SPI || USE_SPI_EMULATION)
	UINT16 mode;
	UINT16 currentState;
	UINT16 mmcRdyState;
	UINT16 tempData;
	UINT16 LastResponse[8];
	UINT32 mmcStatus;
#endif
    
#if (USE_TRUE_IDE || USE_PCMCIA)
  UCHAR vo_write_precomp; /* Used to turn on read ahead on certain drives */
  UCHAR vo_sector_count;  /* Sector transfer request */
  UCHAR vo_sector_number; /* Sector number request */
  UCHAR vo_cyl_low;       /* Low byte of cylinder */
  UCHAR vo_cyl_high;      /* High byte of cylinder */
  UCHAR vo_drive_head;    /* Drive & Head register    
                             Bit 5 == 1
                             Bit 4 = Drive 0/1 == C/D,
                             Bit 0-3 == head
                          */
  UCHAR vo_feature;       /* Feature register */
  UCHAR vo_command;       /* command register see command patterns above (W) */
  UCHAR vo_dig_out;       /* Digital output register. Bits enable interrupts
                             and software reset See BIT patterns above
                          */

	/* Virtual input register file - We read these from the drive
	   during our command completion code
	*/
  UCHAR vi_error;         /* Error register. See IDE_ERB_XXXX in ATADRV.H */
  UCHAR vi_sector_count;  /* # Sectors left in request */
  UCHAR vi_sector_number; /* Current sector number in request  */
  UCHAR vi_cyl_low;       /* Low byte of cylinder in request  */
  UCHAR vi_cyl_high;      /* High byte of cylinder in request  */
  UCHAR vi_drive_head;    /* Drive & Head register in request  */
				/* Byte 5 == 1
				   Byte 4 = Drive 1/0 == C/D,
				   Bytes 0-3 == head
				*/
  UCHAR vi_status;        /* Status register see IDE_STB_XXXX above. Reading
                             this register sends an interrupt acknowledge
                             to the drive.
				*/
  UCHAR vi_alt_status;    /* Same as status register but does not clear
                             interrupt state. 
				*/
  UCHAR vi_drive_addr;    /* Bit 0 & 1 are a mask of which drive is selected
                             bits 2-5 are the head number in ones complement.
                             ~(BITS2-5) == head number
                          */
#endif
} DEVICE_CONTROLLER, *PDEVICE_CONTROLLER;



/* INTERFACE_DATA_STRUCT is defined in IOCONST.C */
/* This flag is needed for some compilers to avoid seeing the following
*  declarations as redefinition.
*/
#ifndef INTERFACE_DATA_STRUCT

/* Allocate space for controllers in the system */
#if (N_INTERFACES)
/* Device control structure information */
 SDIMPORT DEVICE_CONTROLLER controller_s[N_INTERFACES];
 SDIMPORT INT16 drvs_per_controller[N_INTERFACES];
 SDIMPORT UCHAR *drive_ptr[N_INTERFACES];
#endif


/* Allocate space for devices connecting to the first controller */
#if (DRIVES_PER_CONTROLLER1 > 0)
SDIMPORT DRIVE_DESC ddrive1[DRIVES_PER_CONTROLLER1];
#endif


/* Allocate space for devices connecting to the second controller */
#if (DRIVES_PER_CONTROLLER2 > 0)
SDIMPORT DRIVE_DESC ddrive2[DRIVES_PER_CONTROLLER2];
#endif


#if (USE_MEMMODE)
SDIMPORT const ULONG mem_mapped_addresses[N_INTERFACES]; 
SDIMPORT const FPTR mem_mapped_addresses_pointer[N_INTERFACES];
#else
SDIMPORT const UINT16 io_mapped_addresses[N_INTERFACES];
#endif

#if (USE_INTERRUPTS)
SDIMPORT const INT16 dev_interrupts[N_INTERFACES];
#endif

#endif  /* INTERFACE_DATA_STRUCT */



/* For multiple controllers */
#if (N_INTERFACES > 1)
INT16 drno_to_controller_no(INT16 driveno);
PDEVICE_CONTROLLER drno_to_controller(INT16 driveno);
INT16 drno_to_phys(INT16 driveno);
INT16 get_controller_number(INT16 driveno);
/* INT16 controller_to_drvno(INT16 controllerno); */
#endif

/* Controller structure Initialization */
SDVOID clear_controller_structure(SDVOID);


#if (USE_FILE_SYSTEM)
SDVOID system_controller_init(INT16 driveno);
SDVOID system_controller_close(INT16 driveno);

SDIMPORT UINT16 *lock_temporary_buffer(SDVOID);
SDIMPORT SDVOID release_temporary_buffer(SDVOID);
#endif



#ifdef __cplusplus
}
#endif


#define _DRIVE_H_

#endif
