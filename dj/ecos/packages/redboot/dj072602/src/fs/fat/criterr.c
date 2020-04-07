//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 8/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
 * FileName:   criterr.c    critical error handler
 *
 * SanDisk Host Developer's Toolkit
 *
 * Copyright (c) 1997 - 1998 SanDisk Corporation
 * All rights reserved.
 * This code may not be redistributed in source or linkable object form
 * without the consent of its author.
*
******************************************************************************/
/*****************************************************************************
** Name:   criterr.c    critical error handler
**
**      platform_critical_handler
******************************************************************************/

#include <fs/fat/sdapi.h>
#include <fs/fat/pcdisk.h>
#include <redboot.h>

#if (USE_FILE_SYSTEM)

/******************************************************************************
* Name: platform_critical_handler   Critical error handler function
*
* Processing:
*       Provide a way to communicate with user where a disk access
*       fails and return feedback to the caller for next process. 
*
* Note: Since this handler depends on the platform, there is no way to write
*       this code in a generic fashion (except hardcode).
*       Therefore the implementation of this routine is left to the developer.
*
* Entries:
*       INT16   driveno:        Drive Number
*       INT16   media_status:   Status passing from the file system or interface.
*                               The followings are the error state:
*                               - CRERR_BAD_FORMAT,
*                               - CRERR_NO_CARD,
*                               - CRERR_BAD_CARD,
*                               - CRERR_CHANGED_CARD,
*                               - CRERR_CARD_FAILURE,
*                               - CRERR_ID_ERROR,
*                               - CRERR_ECC_ERROR
*
*       ULONG   sector:         At what sector the error occured.
*
* Returns:
*       Critical Error code.  The followings are critical error codes:
*               - CRITICAL_ERROR_ABORT
*               - CRITICAL_ERROR_RETRY
*               - CRITICAL_ERROR_FORMAT
*               - CRITICAL_ERROR_CLEARECC
*
******************************************************************************/
#if 0
INT16 platform_critical_handler(INT16 driveno, INT16 media_status, ULONG sector)
{
    /* Your code goes here */

    driveno = driveno;
    sector = sector;

    if (media_status == CRERR_BAD_FORMAT)
	return (CRITICAL_ERROR_FORMAT);

    return (CRITICAL_ERROR_ABORT);
    /*        return (CRITICAL_ERROR_RETRY); */
}
#else

/*****************************************************************************/
/*******************************  SAMPLE CODE ********************************/
/*****************************************************************************/

/******************************************************************************
* This is the sample code for DOS with keyboard suported only.
* Please notice that this routine is here to aid in development
* for your platform.
******************************************************************************/

/* Error message */
TEXT *med_st[] = {
    "CRERR_BAD_FORMAT",     /* Bad format */
    "CRERR_NO_CARD",        /* No PC Card */
    "CRERR_BAD_CARD",       /* Bad PC Card */
    "CRERR_CHANGED_CARD",   /* Change PC Card */
    "CRERR_CARD_FAILURE",   /* PC Card Failure */
    "CRERR_ID_ERROR",       /* ID error */
    "CRERR_ECC_ERROR"       /* ECC error */
};

INT16 platform_critical_handler(INT16 driveno, INT16 media_status, ULONG sector)
{
    DDRIVE *pdr;
    SDBOOL needs_flush;
    ULONG ltemp;
	
    /* Get current drive structure */
    pdr = mem_drives_structures+( driveno * sizeof(DDRIVE));

    /* Check to see if FAT is dirty */
    if (pdr->fat_is_dirty || pc_test_all_files(pdr))
	needs_flush = YES;
    else
	needs_flush = NO;
    ltemp = pdr->volume_serialno;

    /* Print some information */
    printf("Media status == %s\n", med_st[media_status - CRERR_BAD_FORMAT]);
    printf("Volume Serial # == %X-%X\n", ltemp >> 16, ltemp & 0xffff);

    if (needs_flush)
	printf("Volume is dirty\n");
    else
	printf("Volume is clean\n");

    switch (media_status) {
	case CRERR_SLEEP: 
	{
	    return CRITICAL_ERROR_IGNORE;
	}

#if defined(__DHARMA)
	case CRERR_DRIVE_RESET:
	{
	    return CRITICAL_ERROR_RETRY;
	}
#endif /* __DHARMA */
	
	case CRERR_NO_CARD:
	{
	    return CRITICAL_ERROR_IGNORE;
	}
	
	case CRERR_CHANGED_CARD:
	{
	    return CRITICAL_ERROR_IGNORE;
	}
	
	case CRERR_BAD_FORMAT: 
	{
	    return CRITICAL_ERROR_ABORT;
	}
	
	default:
	{
	    return CRITICAL_ERROR_ABORT;
	}
    }
}
#endif

#endif  /* (USE_FILE_SYSTEM) */
