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
* Filename: ioconst.c - Data Intitialzation
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997-1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Constant declarations 
*
******************************************************************************/

#define INTERFACE_DATA_STRUCT
#include <fs/fat/drive.h>



/********************************* IDE Section *******************************/
/***************************************************************************** 
 * Constant declarations used by the IDE driver.
 * IO addresses and interrupts used for ATA devices.
 *              Primary  Secondary Tertiary Quaternary
 *              0x01f0  0x0170     0x01e8   0x0168  
 *              0x03f6  0x0376     0x03ee   0x036e
 *              14      15         11       10
 *      Note: -1 for interrupts means used polled
 ******************************************************************************/

#if (USE_TRUE_IDE)

#if (USE_MEMMODE)
const ULONG mem_mapped_addresses[N_INTERFACES] = {
#if (N_INTERFACES > 0)
  ATA_PRIMARY_MEM_ADDRESS,
#endif
#if (N_INTERFACES > 1)
  ATA_SECONDARY_MEM_ADDRESS
#endif
};

const FPTR mem_mapped_addresses_pointer[N_INTERFACES] = {
#if (N_INTERFACES > 0)
  (FPTR)ATA_PRIMARY_MEM_ADDRESS,
#endif
#if (N_INTERFACES > 1)
  (FPTR)ATA_SECONDARY_MEM_ADDRESS
#endif
};

#else	/* IO mapped mode */

const UINT16 io_mapped_addresses[N_INTERFACES]= {
#if (N_INTERFACES > 0)
  ATA_PRIMARY_IO_ADDRESS,
#endif
#if (N_INTERFACES > 1)
  ATA_SECONDARY_IO_ADDRESS
#endif
};

#endif	/* USE_MEMMODE */

#if (USE_INTERRUPTS)
const INT16  dev_interrupts[N_INTERFACES] = {
#if (N_INTERFACES > 0)
  ATA_PRIMARY_INTERRUPT,
#endif
#if (N_INTERFACES > 1)
  ATA_SECONDARY_INTERRUPT
#endif
};
#endif	/* USE_INTERRUPTS */

#endif  /* USE_TRUE_IDE */


/******************************* PCMCIA Section ******************************/
/*****************************************************************************/
#if (USE_PCMCIA)

#if (USE_MEMMODE)

const ULONG mem_mapped_addresses[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  MEM_WINDOW_1,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  MEM_WINDOW_2,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  MEM_WINDOW_3,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  MEM_WINDOW_4
#endif
};

const FPTR mem_mapped_addresses_pointer[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  (FPTR)MEM_ADDRESS_1,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  (FPTR)MEM_ADDRESS_2,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  (FPTR)MEM_ADDRESS_3,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  (FPTR)MEM_ADDRESS_4
#endif
};


#if (WORD_ACCESS_ONLY)

const UCHAR S16BIT_WINDOW[N_INTERFACES] = {    /* memory window # for 16 bit accesses */

#if (DRIVES_PER_CONTROLLER1 > 0)
  _16BIT_WINDOW_1,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  _16BIT_WINDOW_2,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  _16BIT_WINDOW_3,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  _16BIT_WINDOW_4
#endif
};

#else	/* 8-bit access */

const UCHAR S8BIT_WINDOW[N_INTERFACES] = {    /* memory window # for 8 bit accesses */

#if (DRIVES_PER_CONTROLLER1 > 0)
  _8BIT_WINDOW_1,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  _8BIT_WINDOW_2,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  _8BIT_WINDOW_3,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  _8BIT_WINDOW_4
#endif
};

#endif	/* WORD_ACCESS_ONLY */


/* Configuration option register values primary & secondary */
/* Memory mode. zero to config option register */
const UTINY  cfg_opt_regs[N_INTERFACES]     = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  0,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  0
#endif
};

#else	/* IO mapped mode */

const UINT16 io_mapped_addresses[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  ATA_PRIMARY_IO_ADDRESS,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  ATA_SECONDARY_IO_ADDRESS,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  ATA_THIRD_IO_ADDRESS,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  ATA_FOURTH_IO_ADDRESS
#endif
};


/* Configuration option register values primary & secondary */
/* Memory mode. zero to config option register */
const UTINY  cfg_opt_regs[N_INTERFACES]     = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  ATA_PRIMARY_CFGOPT,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  ATA_SECONDARY_CFGOPT,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  ATA_PRIMARY_CFGOPT,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  ATA_SECONDARY_CFGOPT
#endif
};

#endif	/* USE_MEMMODE */


#if (USE_INTERRUPTS)
const INT16  dev_interrupts[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  ATA_PRIMARY_INTERRUPT,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  ATA_SECONDARY_INTERRUPT,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  ATA_PRIMARY_INTERRUPT,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  ATA_SECONDARY_INTERRUPT
#endif
};

#endif	/* USE_INTERRUPTS */


/* CIS windows */
const ULONG cis_address[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  MEM_WINDOW_0,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  MEM_WINDOW_0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  MEM_WINDOW_0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  MEM_WINDOW_0
#endif
};

/* CIS pointers */
const FPTR cis_address_pointer[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  (FPTR)MEM_ADDRESS_0,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  (FPTR)MEM_ADDRESS_0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  (FPTR)MEM_ADDRESS_0,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  (FPTR)MEM_ADDRESS_0
#endif
};

#endif  /* USE_PCMCIA */


/******************************** SPI Section ********************************/
/*****************************************************************************/
#if (USE_SPI || USE_SPI_EMULATION)

#if (USE_MEMMODE)       /* Memory mapped mode */
 const ULONG mem_mapped_addresses[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   SPI_BASE_ADDR,
  #endif
  #if (N_INTERFACES > 1)
   SPI_BASE_ADDR
  #endif
};
 const FPTR08 mem_mapped_addresses_pointer[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   (FPTR08)SPI_BASE_ADDR,
  #endif
  #if (N_INTERFACES > 1)
   (FPTR08)SPI_BASE_ADDR
  #endif
};

#else                   /* IO mapped mode */
 const UINT16 io_mapped_addresses[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   SPI_PRIMARY_IO_ADDRESS,
  #endif
  #if (N_INTERFACES > 1)
   SPI_SECONDARY_IO_ADDRESS
  #endif
};
#endif

#if (USE_INTERRUPTS)
 const INT16 dev_interrupts[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   SPI_PRIMARY_INTERRUPT,
  #endif
  #if (N_INTERFACES > 1)
   SPI_SECONDARY_INTERRUPT
  #endif
};

#endif	/* USE_INTERRUPTS */

#endif /* (USE_SPI || USE_SPI_EMULATION) */




/******************************** MMC Section ********************************/
/*****************************************************************************/
#if (USE_MMC || USE_MMC_EMULATION)

#if (USE_MEMMODE)       /* Memory mapped mode */
 const ULONG mem_mapped_addresses[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   MMC_BASE_ADDR,
  #endif
  #if (N_INTERFACES > 1)
   MMC_BASE_ADDR
  #endif
};
 const FPTR08 mem_mapped_addresses_pointer[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   (FPTR08)MMC_BASE_ADDR,
  #endif
  #if (N_INTERFACES > 1)
   (FPTR08)MMC_BASE_ADDR
  #endif
};

#else                   /* IO mapped mode */
 const UINT16 io_mapped_addresses[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   MMC_PRIMARY_IO_ADDRESS,
  #endif
  #if (N_INTERFACES > 1)
   MMC_SECONDARY_IO_ADDRESS
  #endif
};
#endif

#if (USE_INTERRUPTS)
 const INT16 dev_interrupts[N_INTERFACES] = {

  #if (N_INTERFACES > 0)
   MMC_PRIMARY_INTERRUPT,
  #endif
  #if (N_INTERFACES > 1)
   MMC_SECONDARY_INTERRUPT
  #endif
};

#endif	/* USE_INTERRUPTS */

#endif /* (USE_MMC || USE_MMC_EMULATION) */



/* To allow more flexible way to configure the sub-system and reduce
** RAM footage.
*/

DEVICE_CONTROLLER controller_s[N_INTERFACES];

/* Allocate space for devices connecting to the first controller */
#if (DRIVES_PER_CONTROLLER1 > 0)
DRIVE_DESC ddrive1[DRIVES_PER_CONTROLLER1];
#endif

/* Allocate space for devices connecting to the second controller */
#if (DRIVES_PER_CONTROLLER2 > 0)
DRIVE_DESC ddrive2[DRIVES_PER_CONTROLLER2];
#endif


/* Device control structure */
#if (USE_PCMCIA)

/* Drives per controller information array */
INT16 drvs_per_controller[N_INTERFACES] = {

#if (DRIVES_PER_CONTROLLER1 > 0)
  1,
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  1,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  1,
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  1
#endif
};

/* Create the drive structure array */
UCHAR *drive_ptr[N_INTERFACES] = {
#if (DRIVES_PER_CONTROLLER1 > 0)
  (UCHAR *)&ddrive1[0],
#endif
#if (DRIVES_PER_CONTROLLER1 > 1)
  (UCHAR *)&ddrive1[1],
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  (UCHAR *)&ddrive2[0],
#endif
#if (DRIVES_PER_CONTROLLER2 > 1)
  (UCHAR *)&ddrive2[1],
#endif
};

#else /* !(USE_PCMCIA) */

/* Drives per controller information array */
INT16 drvs_per_controller[N_INTERFACES] = {
#if (DRIVES_PER_CONTROLLER1 > 0)
  DRIVES_PER_CONTROLLER1,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  DRIVES_PER_CONTROLLER2,
#endif
};

/* Create the drive structure array */
UCHAR *drive_ptr[N_INTERFACES] = {
#if (DRIVES_PER_CONTROLLER1 > 0)
  (UCHAR *)ddrive1,
#endif
#if (DRIVES_PER_CONTROLLER2 > 0)
  (UCHAR *)ddrive2,
#endif
};

#endif	/* USE_PCMCIA */


/*****************************************************************************
* Name: clear_controller_structure
*
* Description:
*       Initialize the controller structure to a known value.
*
* Entries:
*       INT16   controllerno    Controller Number
*
* Returns:
*       None
*
******************************************************************************/
SDVOID clear_controller_structure(SDVOID) /*__fn__*/
{
  INT16 i;
  PDEVICE_CONTROLLER pc;

  /* Initialize internal data structure to a known state */
  for (i = 0; i < N_INTERFACES; i++)
    {
      /* clear internal drive structure */
      pc = &controller_s[i];

      pc->controller_number = -1;
      pc->interrupt_number = -1;
      pc->opencount = 0;
#if (USE_MEMMODE)
      pc->register_file_address = (FPTR)mem_mapped_addresses_pointer[i];
#else
      pc->register_file_address = io_mapped_addresses[i];
#endif
      /* Get the drive structure infomation */
      pc->drive = (DRIVE_DESC *)drive_ptr[i];
    }
}

