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
/******************************************************************************
* FileName:  timer.c - Time and Date.
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
******************************************************************************/
/* timer.c - Timer service routines for run-time drivers.
*
*  These routines provide executive services for the run-time drivers
*  running on a PC with no executive kernel. Equivalent services should
*  be provided by the kernel environment, replacing these.
*
* The following functions must be provided
* 
* SDVOID  oem_getsysdate()          - Date and time information
* ULONG platform_get_ticks()      - Return the current system tick count
* ULONG platform_ticks_per_second()    - Return the tick period
* 
*/          

#ifdef X86      /* INTEL & compatible processors.  Compilation option. */
#include <conio.h>
#include <dos.h>
#include <bios.h>
#include <time.h>
#include "oem.h"

#define TICKS_PER_SECOND        18

#else

#include <fs/fat/oem.h>
//#include <cyg/kernel/kapi.h>
#define TICKS_PER_SECOND        100

#endif          /* Processors */
 

/* ======================== Timer management functions ===================== */

#if (USE_FILE_SYSTEM)

#if (RTFS_WRITE)
/******************************************************************************
* Name:  OEM_GETSYSDATE - Get the current system time and date (USER SUPPLIED)
*
* Description:
*       When the system needs to date stamp a file it will call this routine
*       to get the current time and date. YOU must modify the shipped routine
*       to support your hardware's time and date routines. If you don't modify
*       this routine the file date on all files will be the same.
*
* NOTE: Be sure to retain the property of returning its argument. The 
*       package depends on it.
*
* Entries:
*       UINT16  *date   ( relative to 1980 )
*       UINT16  *time	( Note: seconds are 2 second/per. ie 3 == 6 seconds )
*
* Returns:
*       The argument it was passed. A information containing
*       date and time info.
*
* Example:
*       #include "pcdisk.h"
*
*       Load the inode copy name, ext, attr, cluster, size, datetime
*       OEM_GETSYSDATE(*date, *time);
*
******************************************************************************/
static UINT16 oem_ttime = 0;
static UINT16 oem_tdate = (UINT16)(18 << 9);      /* Default to 1998 */
static UTINY  oem_ttime_tenths = 0;

SDVOID oem_getsysdate(
        UINT16  *tdate,  /* relative to 1980 */ 
        UINT16  *ttime) /*__fn__*/ /* Note: seconds are 2 second/per. ie 3 == 6 seconds */
{
#ifdef X86      /* For INTEL & INTEL COMPATIBLE CPU */
        struct tm *timeptr;
        time_t timer;
	UINT16 ttemp;

        /* Get date and time from the host system. This is for DOS only */
        time(&timer);
        timeptr = localtime(&timer);

        ttemp = (UINT16) (timeptr->tm_sec >> 1);
        *ttime = (UINT16) ( (UINT16)(timeptr->tm_min << 5) | ttemp );
        ttemp = (UINT16) (timeptr->tm_hour << 11);
        *ttime |= ttemp;
        /*
        Date comes back relative to 1900 (eg 93). The pc wants it
        relative to 1980. so subtract 80.
        */
        ttemp = (UINT16) (timeptr->tm_mon + 1);
        *tdate = (UINT16) ( (ttemp << 5) | (UINT16)timeptr->tm_mday );
        ttemp = (UINT16) (timeptr->tm_year - 80);
        *tdate |= (ttemp << 9);

#else           /* For other CPU such as MOTOROLA, POWER PC,.... */
	*ttime = oem_ttime;
	*tdate = oem_tdate;
#endif
}

SDVOID oem_getsysdateext(UTINY * ttime_tenths)
{
    *ttime_tenths = oem_ttime_tenths;
}

SDVOID oem_setsysdate(
    UINT16 tdate,
    UINT16 ttime,
    UTINY  ttime_tenths)
{
    oem_ttime = ttime;
    oem_ttime_tenths = ttime_tenths;
    oem_tdate = tdate;
}

#endif  /* USE_RTFS */

#endif  /* USE_FILE_SYSTEM */


/* ======================= Timer management functions ===================== */

/****************************************************************************
** Name:  PLATFORM_TICK_P_SECOND - Get current tick per second (USER SUPPLIED)
**
** Summary:
**
**    #include "oem.h"
**
**    platform_tick_p_second(SDVOID)
**
** Description:
**       Provide a way to get the number of ticks per second.
**       The default is 18 ticks per second for INTEL platform
**       and 60 ticks per second for other platforms.
**
**
**    NOTE: Be sure to retain the property of returning its argument. The 
**            package depends on it.
**
** Entries:
**    None
**
** Returns
**    The ticks per second.
**
**
*****************************************************************************/
ULONG platform_ticks_p_second(SDVOID) /*__fn__*/
{
        return(TICKS_PER_SECOND);
}


/****************************************************************************
** Name:  PLATFORM_DELAYMS - delay in certain time (USER SUPPLIED)
**
** Summary:
**
**    #include "oem.h"
**
**    platform_delayms(SDVOID)
**
** Description:
**    Wait for a certain time requested by the user
**
** Entries:
**    sleep     The granularity in miliseconds
**
** Returns
**    None.
**
** Example:
**    #include "oem.h"
**
** Note:
** The calculation of the ticks at a given delay time in millisecond is:
**      onetick in milli second = 1000 milli second / TICKS_PER_SECOND
**      no. ticks = (delay time in millisecond) / (onetick in milli second)
**
*****************************************************************************/
SDVOID platform_delayms(COUNT delay_millis) /*__fn__*/
{
#if 0
  ULONG tticks;

  tticks = platform_get_ticks();
  tticks += (ULONG)(delay_millis / (1000 / TICKS_PER_SECOND));

  /* Wait for awhile */
  while (platform_get_ticks() <= tticks)
    ;
#else
  //HAL_US_DELAY(delay_millis * 1000);
#endif
}


/****************************************************************************
** Name:  PLATFORM_GET_TICKS - Get current timer tick (USER SUPPLIED)
**
** Summary:
**
**    #include "oem.h"
**
**    platform_get_ticks(SDVOID)
**
** Description:
**    Provide a way to get the current timer tick.
**
** Entries:
**    None
**
** Returns
**    The granularity timer ticks.
**
** Example:
**    #include "oem.h"
**
**
*****************************************************************************/
ULONG platform_get_ticks(SDVOID) /*__fn__*/
{
#ifdef X86      /* For INTEL & COMPATIBLE CPUs */
  ULONG myTicks;

  _bios_timeofday(_TIME_GETCLOCK, (long *)&myTicks);
  return(myTicks);
#else           /* For other processors such as MOTOROLA, POWER PC,... */
  //  return (system_tick_dev->read_word());
  return 0;
#endif
}


