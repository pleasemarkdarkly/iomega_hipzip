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
/******************************************************************************
* FileName: report.c - Error report routine. User interaction.
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1998 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
******************************************************************************/
/*
** report.c - Error report routine for the run-time drivers.
*/

#include <fs/fat/oem.h>
#include <redboot.h>

#if (USE_FILE_SYSTEM)

/******************************************************************************
* Name:   platform_report_error() - Report internal error:  (USER SUPPLIED !)
*
* Description:
*       When the system detects an error needs it calls this routine with 
*       PCERR_????? as an argument. You may use this information to
*       implement your own user interaction with these errors.
*
* Entries:
*       INT16   error_number    Error code
*              PCERR_FAT_FLUSH      (* Cant flush FAT *)
*              PCERR_INITMEDI       (* Not a DOS disk:pc_dskinit *)
*              PCERR_INITDRNO       (* Invalid driveno to pc_dskinit *)
*              PCERR_INITCORE       (* Out of core:pc_dskinit *)
*              PCERR_INITDEV        (* Can't initialize device:pc_dskinit *)
*              PCERR_INITREAD       (* Can't read block 0:pc_dskinit  *)
*              PCERR_INITWRITE      (* Can't write block 0:pc_dskfree  *)
*              PCERR_BLOCKCLAIM     (* PANIC: Buffer Claim  *)
*              PCERR_BLOCKLOCK      (* Warning: freeing a locked buffer  *)
*              PCERR_REMINODE       (* Trying to remove inode with open > 1 *)  
*              PCERR_FATREAD        (* IO Error While Failed Reading FAT" *)
*              PCERR_DROBJALLOC     (* Memory Failure: Out of DROBJ Structures *)
*              PCERR_FINODEALLOC    (* Memory Failure: Out of FINODE Structures" *)
*
* Returns
*       None

******************************************************************************/
#if 0
SDVOID platform_report_error(INT16 error_number) /*__fn__*/
{
        /* Your routine goes here */
        error_number = error_number;
}

#else

/*************************************************************************/
/**************************** SAMPLE CODE ********************************/
/*************************************************************************/

/* The following is just an example */

TEXT *pc_error_strings[] = {
    "Cant flush FAT",
    "Not a DOS disk:pc_dskinit",
    "Invalid driveno to pc_dskinit",
    "Out of core:pc_dskinit",
    "Can't initialize device:pc_dskinit",
    "Can't read block 0:pc_dskinit",
    "Can't write block 0:pc_dskfree",
    "PANIC: Buffer Claim",
    "Warning: freeing a locked buffer",
    "Trying to remove inode with open > 1",
    "IO Error While Failed Reading FAT",
    "Memory Failure: Out of DROBJ Structures",
    "Memory Failure: Out of FINODE Structures"
};

#define PCERR_FAT_FLUSH 0       /* Cant flush FAT */
#define PCERR_INITMEDI  1       /* Not a DOS disk:pc_dskinit */
#define PCERR_INITDRNO  2       /* Invalid driveno to pc_dskinit */ 
#define PCERR_INITCORE  3       /* Out of core:pc_dskinit */
#define PCERR_INITDEV   4       /* Can't initialize device:pc_dskinit */
#define PCERR_INITREAD  5       /* Can't read block 0:pc_dskinit  */
#define PCERR_INITWRITE 6       /* Can't write block 0:pc_dskfree  */
#define PCERR_BLOCKCLAIM 7      /* PANIC: Buffer Claim  */
#define PCERR_BLOCKLOCK  8      /* Warning: freeing a locked buffer  */
#define PCERR_REMINODE   9      /* Trying to remove inode with open > 1 */  
#define PCERR_FATREAD    10     /* IO Error While Failed Reading FAT" */
#define PCERR_DROBJALLOC 11     /* Memory Failure: Out of DROBJ Structures"  */
#define PCERR_FINODEALLOC 12    /* Memory Failure: Out of FINODE Structures" */ 


SDVOID platform_report_error(INT16 error_number) /*__fn__*/
{
    /*    printf("Error %d: %s\n", error_number, pc_error_strings[error_number]); */
    printf("Internal FS Error %d: %s\n", error_number, pc_error_strings[error_number]);
}
#endif

#endif /* USE_FILE_SYSTEM */
