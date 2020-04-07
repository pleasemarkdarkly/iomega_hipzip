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
* Filename: ioutil.c
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Support routines.
*
******************************************************************************/


/*****************************************************************************/ 
/*********** Portable Peripheral Bus Interface Device driver Utility. ********/
/*****************************************************************************/

#include <fs/fat/sdconfig.h>
#include <fs/fat/sdmmc.h>
#include <fs/fat/iome_fat.h>

#if (USE_FILE_SYSTEM)
SDVOID drive_format_information(INT16 driveno, UINT16 *n_heads, UINT16 *sec_ptrack, UINT16 *n_cyls);
INT16 get_interface_error(INT16 driveno);
#endif  /* (USE_FILE_SYSTEM) */

#if (USE_INTERRUPTS)
INT16 get_irq_number(INT16 controller_no);
INT16 calculate_controllerno(SDVOID);
#endif  /* (USE_INTERRUPTS) */


SDLOCAL INT16 interface_error(INT16 driveno);

/*****************************************************************************/
/**********  Utility functions to support Peripheral Bus Interface ***********/
/*****************************************************************************/


#if (USE_FILE_SYSTEM)
/****************************************************************************** 
* Name: system_controller_init
*
* Description:  Set up routine.
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*        None
*
******************************************************************************/
SDVOID system_controller_init (INT16 driveno) /* __fn__ */
{
        PDEVICE_CONTROLLER      pc;
	INT16 controller_no;

        if (driveno >= TOTAL_DRIVES)
                return;

#if (N_INTERFACES > 1)         /* For multiple controllers */
	controller_no = drno_to_controller_no(driveno);
#else
        controller_no = 0;
        driveno = driveno;
#endif

        pc = &controller_s[controller_no];
        pc->controller_number = -1;
        pc->interrupt_number = -1;
        pc->opencount = 0;
}


/*****************************************************************************
* Name: system_controller_close
*
* Description:  File System clean up routine 
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*       None
******************************************************************************/
SDVOID system_controller_close(INT16 driveno) /* __fn__ */
{
        PDEVICE_CONTROLLER      pc;
	INT16 controller_no;


#if (N_INTERFACES > 1)         /* For multiple controllers */
	controller_no = drno_to_controller_no(driveno);
#else
        driveno = driveno;
        controller_no = 0;
#endif

	OS_CONTROLLER_CLOSE(controller_no);
        pc = &controller_s[controller_no];

        pc->controller_number = -1;
        pc->interrupt_number = -1;
        pc->opencount = 0;
}

#endif  /* (USE_FILE_SYSTEM) */


#if (N_INTERFACES > 1)         /* For multiple controllers */

/*****************************************************************************
* Name: get_controller_number
*
* Description:
*       For a given logical drive number, the controller is calculated
*
* Input:
*
* Returns:
*       Controller Number or
*       Unknown controller  (-1)
*
****************************************************************************/
INT16 get_controller_number(INT16 driveno) /* __fn__ */
{
	INT16 cno;
	INT16 dd, ddd;

        if (driveno >= TOTAL_DRIVES)
		return (-1);

	dd = 0;
        for (cno = 0; cno < N_INTERFACES; cno++)
	{
		ddd = dd + drvs_per_controller[cno];
		/* Checking for range that driveno falls into */
		if (driveno >= dd && driveno < ddd)
			break;

		dd = ddd;
	}

	return(cno);
}



/*****************************************************************************
* Name: drno_to_controller_no 
*
* Description:  Logical drive to controller number.
*
*       NOTE:   This routine is called with the drive already locked so
*               in several cases there is no need for critical section
*               code handling
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*       Controller Number or
*       Unknown controller (-1)
*
******************************************************************************/
INT16 drno_to_controller_no(INT16 driveno) /* __fn__ */
{
	/* Get the controller number */
	return(get_controller_number(driveno));
}


/******************************************************************************
* Name: drno_to_controller
*
* Description:
*       Logical drive to controller structure.
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*       Pointer to the controller structure if successful.
*       NULL if failure.
*
******************************************************************************/
PDEVICE_CONTROLLER drno_to_controller(INT16 driveno) /* __fn__ */
{
	INT16   cno;

        if (driveno >= TOTAL_DRIVES)
		return(0);

	cno = get_controller_number(driveno);
	if (cno < 0)
		return(0);
		
	return(&controller_s[cno]);
}


#if 0
/******************************************************************************
* Name: controller_to_drvno
*
* Description:
*       controller number to logical drive number.
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*       Drive number if successful.
*       -1 if not found.
*
******************************************************************************/
INT16 controller_to_drvno(INT16 controllerno) /* __fn__ */
{
	PDEVICE_CONTROLLER pc;
	INT16 driveno, i;

	pc = &controller_s[controllerno];

	if (pc->drive_active & DRV_ACTIVE)
	{
		/* Get the logical drive number */
		driveno = (INT16)(pc->drive_active & 0x0F);

		for (i = 0; i < controllerno; i++)
			driveno += drvs_per_controller[i]; 
	
		return(driveno);
	}
	else
		return(-1);
}
#endif

/***************************************************************************** 
* Name: drno_to_phys
*
* Description:
*       Logical drive to physical drive for a selected controller. 
*
* Input:
*       INT16   driveno         Drive Number
*
* Returns:
*       Physical Drive Number
*
******************************************************************************/
INT16 drno_to_phys(INT16 driveno) /* __fn__ */
{
	INT16 cno, dd, ddd;

	dd = 0;
        for (cno = 0; cno < N_INTERFACES; cno++)
	{
		ddd = dd + drvs_per_controller[cno];
		/* Checking for range that driveno falls into */
		if (driveno >= dd && driveno < ddd)
			break;

		dd = ddd;
	}
	
	/* Convert drive number to physical drive number from
	   the selected controller.
	*/
	return(driveno - dd);
}
#endif  /* (N_INTERFACES > 1) */        /* For multiple controllers */


#if (USE_FILE_SYSTEM)
SDVOID drive_format_information(INT16 driveno, UINT16 *n_heads,
                              UINT16 *sec_ptrack, UINT16 *n_cyls) /* __fn__ */
{
	PDEVICE_CONTROLLER pc;
	INT16 phys_drive;

#if (N_INTERFACES > 1)
	pc = drno_to_controller(driveno);
	phys_drive = drno_to_phys(driveno);
#else
        pc = &controller_s[0];
        phys_drive = driveno;
#endif

	*n_heads =  (UINT16)pc->drive[phys_drive].num_heads;
	*sec_ptrack = (UINT16)pc->drive[phys_drive].sec_p_track;
	*n_cyls = (UINT16)pc->drive[phys_drive].num_cylinders;
}


#endif  /* (USE_FILE_SYSTEM) */


#if (USE_INTERRUPTS)
/***************************************************************************** 
* Name: get_irq_number
*
* Description:  Get an IRQ number defined by user in SDCONFIG.H
*
* Input:
*       INT16   controller_no   Controller Number
*
* Returns:
*       YES if successful
*       NO if failure
*
******************************************************************************/
INT16 get_irq_number(INT16 controller_no) /* __fn__ */
{
	return(dev_interrupts[controller_no]);
}



INT16 calculate_controllerno (SDVOID) /* __fn__ */
{
	PDEVICE_CONTROLLER pc;
	INT16 cno;

        for (cno = 0; cno < N_INTERFACES; cno++)
	{
		pc = &controller_s[cno];
		if (pc->drive_active & DRV_ACTIVE)
			break;
	}
	return (cno);
}


#endif  /* (USE_INTERRUPTS) */


/*****************************************************************************
* Name: interface_error
*
* Description:
*       Get current error code in DEVICE_CONTROLLER structure.
*
* Input:
*       INT16   driveno     Drive Number
*
* Returns:
*       return  (pc->error_code)
*
******************************************************************************/
SDLOCAL INT16 interface_error(INT16 driveno) /* __fn__ */
{
#if 0
	PDEVICE_CONTROLLER pc;

#if (N_INTERFACES > 1)
	pc = drno_to_controller(driveno);
#else
        driveno = driveno;
        pc = &controller_s[0];
#endif

	return(pc->error_code);
#else
	return iome_interface_error(driveno);
#endif
}

#if (USE_FILE_SYSTEM)
/*****************************************************************************
* Name: get_interface_error
*
* Description:
*       Get error code
*
* Input:
*       INT16   driveno     drive/socket number
*
* Returns:
*          0    No Error
*       Otherwise, one of the following error code in controller_s:
*           BUS_ERC_DIAG     1   Drive diagnostic failed in initialize
*           BUS_ERC_ARGS     2   User supplied invalid arguments
*           BUS_ERC_DRQ      3   DRQ should be asserted but it isn't
*                                  or driver and controller are out of phase
*           BUS_ERC_TIMEOUT  4   Timeout during some operation 
*           BUS_ERC_STATUS   5   Controller reported an error
*                                  look in the error register
*           BUS_ERC_ADDR_RANGE  6 LBA out of range
*           BUS_ERC_CNTRL_INIT  7 Fail to initialize controller_s structure
*           BUS_ERC_IDDRV       8 Identify drive info error
*           BUS_ERC_CMD_MULT    9 Read/Write Multiple Command attempts
*                                   to run before Set Multiple Command
*                                   has been executed
*           BUS_ERC_BASE_ADDR  10 Base Address not Available
*
******************************************************************************/
INT16 get_interface_error(INT16 driveno) /* __fn__ */
{
     return(interface_error(driveno));
}

#else /* (!USE_FILE_SYSTEM) */
INT16 pc_get_error (INT16 driveno) /* __fn__ */
{
     return(interface_error(driveno));
}
#endif  /* (USE_FILE_SYSTEM) */


