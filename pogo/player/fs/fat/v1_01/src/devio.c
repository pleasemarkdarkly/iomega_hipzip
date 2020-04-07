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
* Filename:    devio.c - device IO functions
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1997 - 1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Interface layer to the low level driver.
*
* The following routines in this file are included:
*
*       devio_open
*       devio_close
*       devio_read
*       devio_write
*       devio_erase
*       devio_read_serial
*
******************************************************************************/

#include "pcdisk.h"
#include "intrface.h"
#include "oem.h"

#if (USE_FILE_SYSTEM)


/* Local function */
SDLOCAL SDBOOL devio_verify_rdwr(INT16 driveno, ULONG *blockno);


/******************************************************************************
** Name:  devio_open 
**
** Description:
**      Call partition init function and find the first primary DOS partition.
**
** Entries:
**      INT16   driveno         Drive Number
**
** Returns:
**      1       Primary DOS partition found
**      0       if failed
**
******************************************************************************/
SDBOOL devio_open ( INT16 driveno ) /* __fn__ */
{
        SDBOOL ret_val;

        PC_DRIVE_IO_ENTER(driveno)

        /* Get all valid partitions */
        ret_val = get_partition_info(driveno);

        PC_DRIVE_IO_EXIT(driveno)


        return (ret_val);
}


/***************************************************************************
** Name:  devio_close - 
**
** Description:
**
**      Unmount the drive by turn off power to drive and restore all resources.
**
** Entries:
**      driveno         Drive Number
**
** Returns:
**      None
**
****************************************************************************/
SDVOID devio_close ( INT16 driveno ) /* __fn__ */
{
        PC_DRIVE_IO_ENTER(driveno)
        INTERFACE_DEVICE_CLOSE(driveno);
        PC_DRIVE_IO_EXIT(driveno)
}


SDLOCAL SDBOOL devio_verify_rdwr( INT16 driveno, ULONG *blockno )
{
        DDRIVE *pdr;


        /* Pointer to the drive data structure */
        pdr = &mem_drives_structures[driveno];


        PC_DRIVE_IO_ENTER(driveno)

        /* Check if disk is accessable and hasn't changed since mounted */
        if ( !check_media_io(driveno) )
        {
                PC_DRIVE_IO_EXIT(driveno)
                return (NO);
        }

        PC_DRIVE_IO_EXIT(driveno)

        if ( pdr->enable_mapping )
        {
                if ( pdr->volume_end < (pdr->volume_start + *blockno) )
                {
                        return (NO);
                }

                *blockno += pdr->volume_start;
        }

        return (YES);
}

/******************************************************************************
** Name: devio_read
**
** Description:
**       
**      Perform low level read sector(s) command               
**
** Entries:
**      INT16    driveno         Drive Number
**      ULONG    blockno         Block number. Starting sector
**      UTINY    *buf            Pointer to the transfer Buffer
**      UCOUNT   n_to_read       Number of blocks to read
**
** Returns:
**
**      YES      if successful
**      NO       if failed
******************************************************************************/
SDBOOL devio_read(INT16 driveno, ULONG lba, UCHAR *buf, ULONG n_to_read) /* __fn__ */
{
        ULONG   blockno;


        /* Make sure the block address is within range */
        blockno = lba;
        if ( !devio_verify_rdwr(driveno, &blockno) )
                return NO;

        PC_DRIVE_IO_ENTER(driveno)


        /* try the read */
        if ( DEV_READ(driveno, blockno, buf, n_to_read) == YES )
        {
                PC_DRIVE_IO_EXIT(driveno)
                return (YES);
        }

        /* IO failed, get the error condition */
        errno = get_interface_error(driveno);


        PC_DRIVE_IO_EXIT(driveno)
        return (NO);
}



#if (RTFS_WRITE)

/*****************************************************************************
** Name: devio_write
**
** Description:
**
**      Perform low level write sector(s) command               
**
** Entries:
**      INT16    driveno         Drive Number
**      ULONG    blockno         Block number. Starting sector
**      UTINY    *buf            Pointer to the transfer Buffer
**      UCOUNT   n_to_write      Number of blocks to write
**
** Returns:
**
**      YES      if successful
**      NO       if failed
******************************************************************************/
SDBOOL devio_write(INT16 driveno, ULONG lba, UCHAR *buf, ULONG n_to_write) /* __fn__ */
{
        ULONG   blockno;

        /* Make sure the block address is within range */
        blockno = lba;
        if ( !devio_verify_rdwr(driveno, &blockno) )
                return NO;


        PC_DRIVE_IO_ENTER(driveno)

        /* try the write */
        if ( DEV_WRITE(driveno, blockno, buf, n_to_write) == YES )
        {
                PC_DRIVE_IO_EXIT(driveno)
                return (YES);
        }

        /* IO failed, get the error condition */
        errno = get_interface_error(driveno);


        PC_DRIVE_IO_EXIT(driveno)
        return (NO);
}


/*****************************************************************************
** Name: devio_erase
**
** Description:
**
**      Perform low level erase sector(s) command               
**
** Entries:
**      INT16    driveno         Drive Number
**      ULONG    blockno         Block number. Starting sector
**      UTINY    *buf            Pointer to the transfer Buffer
**      UCOUNT   n_to_erase      Number of blocks to erase
**
** Returns:
**
**      YES      if successful
**      NO       if failed
******************************************************************************/
SDBOOL devio_erase(INT16 driveno, ULONG lba, UCOUNT n_to_erase) /* __fn__ */
{
        ULONG   blockno;


        /* Make sure the block address is within range */
        blockno = lba;
        if ( !devio_verify_rdwr(driveno, &blockno) )
                return NO;


        PC_DRIVE_IO_ENTER(driveno)
        if ( DEV_SECTORS_ERASE(driveno, blockno, n_to_erase) == YES )
        {
                PC_DRIVE_IO_EXIT(driveno)
                return YES;
        }

        errno = get_interface_error(driveno);

        PC_DRIVE_IO_EXIT(driveno)

        return (NO);
}

#endif  /* RTFS_WRITE */


/*****************************************************************************
** Name: devio_read_serial - Get the serial of the device
**
** Description:
**      Get device geometry
**
** Entries:
**      INT16   driveno         Drive Number
**      PDRV_GEOMETRY_DESC pserialno      Pointer to the device structure
**
** Returns:
**       YES if sucessful
**       NO  if failure
******************************************************************************/
SDBOOL devio_read_serial(INT16 driveno, PDRV_GEOMETRY_DESC dev_geometry) /* __fn__ */
{
        SDBOOL ret_val;


        PC_DRIVE_IO_ENTER(driveno)
        ret_val = DEV_READ_SERIAL(driveno, dev_geometry);
        if ( ret_val == NO )
                errno = get_interface_error(driveno);

        PC_DRIVE_IO_EXIT(driveno)
        return (ret_val);
}

#endif	/* (USE_FILE_SYSTEM) */


