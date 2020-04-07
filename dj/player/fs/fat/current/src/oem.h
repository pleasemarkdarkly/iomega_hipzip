//........................................................................................
//........................................................................................
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 10/20/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
*Filename: OEM.H - Defines & structures for ms-dos utilities
*                     
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997-1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description: 
*       OEM function implementation.
*
****************************************************************************/

#ifndef __OEM__


#ifdef __cplusplus
extern "C" {
#endif

#include <fs/fat/sdtypes.h>
#include <fs/fat/sdconfig.h>
//#include <cyg/kernel/kapi.h>
//#include <cyg/hal/hal_edb7xxx.h>

/* Function Prototypes implemented for PLATFORM specific */
/* These routines are system specific and implemented by
** the developers.
**
** The macros allow the run-time driver to HOOK the system
** specific code to the driver.
**/

/************************* DEPENDENT CODE *********************************/
/**************************************************************************/
/* These routines are system specific.  Mostly OEM dependent codes */

#if (!USE_MEMMODE)
UCHAR inpbyte(UINT16 addr);
SDVOID outpbyte(UINT16 addr, UTINY dat);

#if (WORD_ACCESS_ONLY)
UINT16 inpword(UINT16 addr);
SDVOID outpword(UINT16 addr, UINT16 dat);
#endif
#endif

/*****************************************************************************
** These routines:
**      platform_critical_handler,
**      platform_report_error
** are in criterr.c and report.c and needed to be ported to
** the specific platform.
*****************************************************************************/

/**************************** Critical Error ********************************/
/* Critical error handler routine is in criterr.c */
/* Interaction between users and the HDTK. */
INT16 platform_critical_handler(INT16 drv, INT16 err, ULONG sector);
#define CRITICAL_HANDLER(drv, err, sector)  (platform_critical_handler(drv, err, sector))


/***************************** Error Report *********************************/
/* platform_report_error routine is in report.c */
/* Report the error to user */
SDVOID platform_report_error(INT16 err);
#define REPORT_ERROR(err)       (platform_report_error(err))



/********************************* Timer ************************************/
/*****************************************************************************
** These routines:
**      platform_delayms
**      platform_get_ticks,
**      platform_ticks_per_second,
**      oem_getsysdate
** are in timer.c and needed to be ported for to the specific platform.
*****************************************************************************/
SDVOID platform_delayms(COUNT tdelay);    /* Time to delay in milisecond */
ULONG platform_get_ticks(SDVOID);         /* Get the current tick */
ULONG platform_ticks_p_second(SDVOID);    /* Ticks per second */

/* These macros are to allow HOOKING system specific
** code to the run-time driver.  The macros are used
** by the FILE SYSTEM and the INTERFACE drivers.
*/
#define OS_WAIT(tdelay)         (platform_delayms(tdelay))
#define OS_GET_TICKS()          (platform_get_ticks())
#define OS_TICKS_P_SECOND()     (platform_ticks_p_second())


#if (USE_FILE_SYSTEM)
#if (RTFS_WRITE)
/* oem_getsysdate() in timer.c */
SDVOID oem_getsysdate(UINT16 *tdate, UINT16 *ttime);
SDVOID oem_getsysdateext(UTINY *ttime_tenths);
SDVOID oem_setsysdate(UINT16 tdate, UINT16 ttime, UTINY ttime_tenths);
#endif
#endif  /* USE_FILE_SYSTEM */



/*****************************************************************************
** These routines:
**      platform_controller_init,
**      platform_controller_close,
**      platform_clear_signal,
**      platform_set_signal,
**      platform_wait_for_action
** are in interupt.c and needed to be ported to the specific platform.
** These routines are for the interrupt implementation.
**
** The OS_XX_XX macros are for the FILE SYSTEM and INTERFACE drivers.
*****************************************************************************/

#define OS_CONTROLLER_INIT(controller_no)
#define OS_CONTROLLER_CLOSE(controller_no)
#define OS_CLEAR_SIGNAL(controller_no)
#define OS_SET_SIGNAL(controller_no)
#define OS_WAIT_FOR_ACTION(controller_no, time_out) YES


/*****************************************************************************
********************** Transfer speed related topics *************************
*****************************************************************************/
  
/* oem_in_words and oem_out_words are in rdwr.c */
/* These routines are generic.  It may not be fast.
** Feel free to modify them to match to your design */

#if (WORD_ACCESS_ONLY)
SDVOID oem_in_words(FPTR16 p, UCOUNT nwords, volatile FPTR16 dreg);
SDVOID oem_out_words(FPTR16 p, UCOUNT nwords, volatile FPTR16 dreg);
#else
SDVOID oem_in_words(FPTR p, UCOUNT nwords, FPTR dreg);
SDVOID oem_out_words(FPTR p, UCOUNT nwords, FPTR dreg);
#endif

/* Static checking for device present */
SDBOOL is_device_changed (INT16 driveno);
INT16  get_extended_error(INT16 driveno);


/************************* END of DEPENDENT CODE ****************************/


#ifdef __cplusplus
}
#endif


#define __OEM__

#endif

