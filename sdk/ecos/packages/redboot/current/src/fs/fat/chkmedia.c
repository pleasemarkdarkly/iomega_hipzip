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
* Filename: chkmedia.c - Media check functions
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
*
*       check_media_status
*       check_media
*       check_media_entry
*       check_media_io
*
******************************************************************************/

#include <fs/fat/pcdisk.h>

#if (USE_FILE_SYSTEM)

/******************************************************************************
* Name: check_media_status
*
* Description:
*       Called from the IO layer
*       Check if the drive is mounted or NOT.  If it is then return; otherwise,
*         do a low level open the drive.
*       Return YES if the drive is mounted and it is okay to proceed
*       with IO on a drive. If this call caused the 
*
*  Entries:
*       INT16     driveno        Drive number 0 .. NDRIVES
*
* Returns:
*       0 If drive is mounted and okay to proceed with IO
*       Error code Otherwise
*
******************************************************************************/
INT16 check_media_status ( INT16 driveno ) /*__fn__*/
{
        DDRIVE *pdr;

        /* Pointer to the drive data structure */
        pdr = &mem_drives_structures[driveno];

#if 0
        /* A device is allowed to be removed or inserted anytime.  Check
           it here to make sure the target device is still intact.
	*/
        if ( pdr->drive_installed )
        {
                if ( is_device_changed (driveno) )
                {
                        return (CRERR_NO_CARD);
                }
        }
#endif

        /* Nothing changes, just return */
        if (pdr->dev_flag & DRIVE_INSTALLED) {
	    if (!iome_check_media_status(driveno)) {
		errno = get_interface_error(driveno);
		return (platform_convert_critical_error(errno));
	    }
	    else {
		return 0;
	    }
	}

        if ( !pdr->dev_flag )
        {
                /* Now initialize the drive and try to access the device. */
                if ( !INTERFACE_DEVICE_OPEN(driveno) )
                {
                        errno = get_interface_error(driveno);
                        return (platform_convert_critical_error(errno));
                }
        }

        return (0);
}


/*****************************************************************************
* Name: check_media
*
* Descriptor:
*       Call check_media_status function to check if there is a card
*       change.  If yes, remount the drive.
*
* Entries:
*       INT16   driveno         Drive number 0 .. NDRIVES
*       SDBOOL    mount           If NO return failure if the current mount
*                               was aborted and a new mount was done.
* Returns:
*       YES is it is okay to proceed with IO on a drive
*
******************************************************************************/
SDBOOL check_media ( INT16 driveno, SDBOOL mount_is_not_error ) /*__fn__*/
{
        DDRIVE  *pdr;
        SDBOOL  ret_val;
        INT16   media_status;
        INT16   user_request;

        ret_val = NO;

        /* Get the drive data structure */
        pdr = &mem_drives_structures[driveno];


        media_status = check_media_status( driveno );

        /*
        Allow a card change that's not in the middle of activity
        with another card (i.e., there's no dirty buffers happening etc).
        */
        if ((media_status == CRERR_CHANGED_CARD) &&
	    /* mount_is_not_error && */ NO &&
	    !(pdr->fat_is_dirty || pc_test_all_files(pdr)))
        {
                /* Abort the current mount */
                if ( pdr->dev_flag )
                        pc_dskfree( driveno, YES );

                /* Set media_status to zero to initialize the device again. */
                media_status = 0;
        }   


        if ( !media_status )
        {
                if ( pdr->dev_flag )
                {
                        ret_val = YES;
                }
                else
                {
                        if ( pc_i_dskopen(driveno) )
                        {
			    ret_val = mount_is_not_error;
                        }
                        else
                        {
/*      See PLATFORM_CRITICAL_HANDLER function in CRITERR.C for ideas   */
/*      on how to return the critical error code to the caller for      */
/*      the next process.                                               */

                                ret_val = NO;

                                media_status= platform_convert_critical_error(errno);

                                user_request = CRITICAL_HANDLER( driveno,
                                                                media_status,
                                                                0L );

#if (RTFS_WRITE)
                                if ( user_request == CRITICAL_ERROR_FORMAT )
                                {
                                        if ( pc_format(driveno) )
                                        {
                                                if ( pc_i_dskopen(driveno) )
                                                {
                                                        ret_val = mount_is_not_error;
                                                }
                                                else
                                                        ret_val = NO;
                                        }
                                        else
                                                ret_val = NO;
                                }
                                else 
#endif
                                if ( user_request == CRITICAL_ERROR_ABORT )
                                {
                                        ret_val = NO;
                                }

#if defined(__DHARMA)
				if (user_request == CRITICAL_ERROR_RETRY) {
				    return check_media(driveno, mount_is_not_error);
				}
#endif /* __DHARMA */
                        }
                }
        }
        else
        {
                user_request = CRITICAL_HANDLER( driveno, media_status, 0L );
                if ( user_request == CRITICAL_ERROR_ABORT )
                {
                        /* Abort the current mount */
                        if ( pdr->dev_flag )
                                pc_dskfree ( driveno, YES );
								
                        ret_val = NO;
                }
#if defined(__DHARMA)
		if (user_request == CRITICAL_ERROR_RETRY) {
		    return check_media(driveno, mount_is_not_error);
		}
#endif /* __DHARMA */
        }

        return (ret_val);
}


/*****************************************************************************
* Name: check_media_entry -  Called from the top of the API
*
* Descriptor:
*       This routine is called from the top of the io layer. If the drive is
*       not mounted, it may be (re)mounted as a result of this call and the
*       call will return YES (it is okay to proceed). A similar call,
*       check_media_io(), is called from a lower layer. It too may end up
*       re-mounting the drive but it will return NO since it would be
*       dangerous to proceded from a lower layer to access a newly mounted
*       disk.
*
* Entries:
*       INT16   driveno         Drive number 0 .. NDRIVES
*
* Returns:
*       YES if is mounted and it is okay to proceed with IO
*       NO  if the drive can't be used and the user did not 
*           replace it with a usable disk
*
******************************************************************************/
SDBOOL check_media_entry ( INT16 driveno ) /*__fn__*/
{
        return( check_media(driveno, YES) );
}


/******************************************************************************
* Name: check_media_io
*
* Description:
*       Called from the IO layer
*       Return YES if the drive is mounted and it is okay to proceed
*       with IO on a drive. If this call caused the 
*
*  Entries:
*       INT16     driveno;        Drive number 0 .. NDRIVES
*
* Returns:
*       YES if is mounted and it is okay to proceed with IO
*       NO  if the drive can't be used and the user did not 
*           replace it with a usable disk
*
******************************************************************************/
SDBOOL check_media_io ( INT16 driveno ) /*__fn__*/
{
        return( check_media(driveno, NO) );
}

#endif  /* (USE_FILE_SYSTEM) */


