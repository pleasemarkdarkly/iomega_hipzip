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
* FileName:  BLOCK.C - Directory block buffering routines
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
*       Block buffers contain a data block a DDRIVE pointer and a block number.
*       If the DDRIVE pointer is Non Null the data is valid for the block.
*       Blocks have three states.
*               Unused  - May be grabbed to use on a block. (PDRIVE == NULL)
*               Open    - May not be re-used until the routine using it lets go
*               Closed  - Data is valid in the buffer but no one is using it. 
*                         if there are no Unused blocks this one may be used.
*                         But if someone needs the data in here it will be
*                         re-opened and used.           
*
* The following routines in this file are included:
*
*    pc_alloc_blk    -   Internal to this file. 
*    pc_blkpool      -   Internal to this file. 
*    pc_free_all_blk -   Release all buffers associated with a drive
*    pc_free_buf     -   Release a single buffer for possible re-use
*    pc_init_blk     -   Initialize a buffer's drive and block #s. Zero its
*                        data buffer
*    pc_read_blk     -   Read data from disk into a buffer
*    pc_write_blk    -   Write data from the buffer to the disk.
*    pc_scratch_blk  -   Allocate a block to use as a scratch buffer
**
*******************************************************************************/

#include <fs/fat/pcdisk.h>

#if (USE_FILE_SYSTEM)

SDIMPORT UINT32  useindex; /* See flconst.c */
SDIMPORT DDRIVE *scratch_pdrive; /* See flconst.c */


SDLOCAL SDBOOL pc_alloc_blk(BLKBUFF **ppblk, DDRIVE *pdrive, BLOCKT blockno);

/******************************************************************************
* Name:    PC_ALLOC_BLK - Find existing or create an empty block 
*                         in the buffer pool.
*
* Description:
*       Use pdrive and blockno to search for a buffer in the buffer pool.
*       If not found create a new buffer entry by discarding the Least
*       recently used buffer in the buffer pool. The buffer is locked
*       in core. A pointer to the buffer is returned in ppblk. If all
*       buffers are in use it returns NULL in *ppblk.
* Note: This function is used only by functions in this file.
*
* Entries:
*       BLKBUFF **ppblk
*       DDRIVE  *pdrive
*       BLOCKT  blockno
*
* Returns:
*       YES if the buffer was found in the pool or
*       NO if a new buffer was assigned.
*
*****************************************************************************/
SDLOCAL SDBOOL pc_alloc_blk(BLKBUFF **ppblk, DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLKBUFF *pblk;
        BLKBUFF *oldest;
        BLKBUFF *freeblk;
        UINT32  lru;

        lru = (UINT32)~0;       /* Initialize to known value */
        oldest = SDNULL;
        freeblk = SDNULL;

        /* Get or init the block pool */
        pblk = mem_block_pool;

        PC_ENTER_CRITICAL()
        useindex += 1L;

        while ( pblk )
        {
                if ( !pblk->pdrive )
                {
                        /* This buffer's free */
                        freeblk = pblk; /* BUG FIX */
                }
                else
                {
                        if ( (pblk->pdrive == pdrive) &&
                                (pblk->blockno == blockno) )
                        {
                                /* Found it */
                                *ppblk = pblk;

                                /* Update the last recently used stuf */
                                pblk->lru = useindex;
                                pblk->use_count += 1;
                                PC_EXIT_CRITICAL()

                                return (YES);
                        }
                        else
                        {
                                /* No match. see if its a candidate for
                                swapping if we run out of pointers */
                                if ( !pblk->use_count )
                                {
                                        if ( pblk->lru < lru )
                                        {
                                                lru = pblk->lru;
                                                oldest = pblk;
                                        }
                                }
                        }
                }

                pblk = pblk->pnext;
        }

        /* If off the end of the list we have to bump somebody */
        if ( freeblk )
                pblk = freeblk;
        else
                pblk = oldest;


        if ( !pblk )
        {
                PC_EXIT_CRITICAL()
                REPORT_ERROR(PCERR_BLOCKCLAIM);

                /* Panic */
                *ppblk = SDNULL;
                return (NO);
        }

        pblk->lru = useindex;
        pblk->pdrive = pdrive;
        pblk->blockno = blockno;
        pblk->use_count = 1;

        PC_EXIT_CRITICAL()
        *ppblk = pblk;

        /* Return NO since we didn't find it in the buffer pool */
        return (NO);
}


/*****************************************************************************
* Name: PC_FREE_ALL_BLK - Release all buffers associated with a drive
*
* Description:
*       Use pdrive to find all buffers in the buffer pool associated with the
*       drive. Mark them as unused, called by dsk_close.
*       If any are locked, print a debug message in debug mode to warn the
*       programmer.
*
* Entries:
*       DDRIVE  *pdrive
*
* Returns:
*       None
******************************************************************************/
SDVOID pc_free_all_blk(DDRIVE *pdrive) /*__fn__*/
{
        BLKBUFF *pblk;

        /* Get or init the block pool */
        pblk = mem_block_pool;

        PC_ENTER_CRITICAL()
        while ( pblk )
        {
                if ( pblk->pdrive == pdrive )
                {
                        if ( pblk->use_count )
                                REPORT_ERROR(PCERR_BLOCKLOCK);

                        pblk->use_count = 0;
                        pblk->pdrive = SDNULL;
                }
                pblk = pblk->pnext;
        }

        PC_EXIT_CRITICAL()
}


/*****************************************************************************
* Name: PC_FREE_BUF - Unlock a block buffer.
*
* Description:
*       Give back a buffer to the system buffer pool so that it may
*       be re-used. If was_err is YES this means that the data in the 
*       buffer is invalid so discard the buffer from the buffer pool.
*
* NOTE: Free a buffer by unlocking it. If waserr is true, zero out the  
*       drive number so the erroneous data is not cached. 
*
* Entries:
*       BLKBUFF *pblk
*       SDBOOL    waserr  
*
* Returns:
*       None
*
***************************************************************************/
SDVOID pc_free_buf(BLKBUFF *pblk, SDBOOL waserr, SDBOOL do_lock) /*__fn__*/
{
  if( do_lock ) {
    PC_ENTER_CRITICAL()
      }
  
        if ( pblk )
        {
                if ( pblk->use_count )
                        pblk->use_count -= 1;
                /*
                If the buffer is corrupted we null the buffer. This is safe
                even in a multitasking environment because the region of the
                disk containing the block is always locked exclusively when
                buffer writes are taking place
                */
                if ( waserr )
                        pblk->pdrive = SDNULL;
        }
	if( do_lock ) {
	  PC_EXIT_CRITICAL()
	    }
	
}


#if (RTFS_WRITE)
#if (RTFS_SUBDIRS)
/*****************************************************************************
* Name: PC_INIT_BLK - Zero a BLKBUFF and add it to the buffer pool
*
* Description:
*       Allocate and zero a BLKBUFF and add it to the to the buffer pool.
*                   
* Note: After initializing you "own" the buffer. You must release it by
*       calling pc_free_buff() before it may be used for other blocks.
*
* Entries:
*       DDRIVE *pdrive
*       BLOCKT blockno
*
* Returns:
*       Returns a valid pointer or NULL if no core.
*                   
******************************************************************************/
BLKBUFF *pc_init_blk(DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLKBUFF *pblk;

        if ( !pdrive || (blockno >= pdrive->numsecs) )
                return (SDNULL);
        else
        {
                pc_alloc_blk( &pblk, pdrive, blockno );
                if ( pblk )
                {
                        pc_memfill( (UTINY *)pblk->data, 512, 0 );
                }

                return (pblk);
        }
}
#endif  /* (RTFS_SUBDIRS) */


/*****************************************************************************
* Name: PC_WRITE_BLK - Flush a BLKBUFF to disk.
*
* Description:
*       Use pdrive and blockno information in pblk to flush it's data buffer
*       to disk.
*
* Entries:
*       BLKBUFF *pblk
*
* Returns:
*       Returns YES if the write succeeded.
*
******************************************************************************/
SDBOOL pc_write_blk(BLKBUFF *pblk) /*__fn__*/
{
        if ( !pblk || !pblk->pdrive )
        {
                return (NO);
        }

        return (devio_write( pblk->pdrive->driveno,
                        pblk->blockno,
                        (UCHAR *)pblk->data,
                        (COUNT) 1 ));
}

#endif  /* (RTFS_WRITE) */


/****************************************************************************
* Name: PC_READ_BLK - Allocate and read a BLKBUFF, or get it from the buffer pool.
*
* Description:
*       Use pdrive and blockno to determine what block to read. Read the block
*       or get it from the buffer pool and return the buffer.
*
* Note: After reading, you "own" the buffer. You must release it by
*       calling pc_free_buff() before it may be used for other blocks.
*
* Entries:
*       DDRIVE *pdrive
*       BLOCKT blockno
*
* Returns
*       Returns a valid pointer or NULL if block not found and not readable.
*
*****************************************************************************/
BLKBUFF *pc_read_blk(DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLKBUFF *pblk;
        SDBOOL found_buffer;

        if ( !pdrive || (blockno >= pdrive->numsecs) )
                return (SDNULL);

        PC_BP_ENTER(pdrive->driveno)
        found_buffer = pc_alloc_blk(&pblk, pdrive, blockno);

        if ( pblk )
        {
                if ( !found_buffer )
                {
                        /*
                        We allocated a buffer but it doesn't have valid data
                        read it in. If the read fails throw the buffer away.
                        */
                        if ( !devio_read( pdrive->driveno,
                                blockno,
                                (UCHAR *)pblk->data,
                                (COUNT)1) )
                        {
                                /* oops: Error, Discard the buffer */
                                pc_free_buf( pblk, YES, YES );
                                pblk = SDNULL;
                        }
                }
        }
        PC_BP_EXIT(pdrive->driveno)

        return (pblk);
}



/******************************************************************************
* Name: PC_SCRATCH_BLK - Return a block for scratch purposes.
*
* Description:
*       Use the block buffer pool as a heap of 512 byte memory locations
*       When done call SDVOID pc_free_buf(pblk, YES) to clean up
*
* Returns
*       Returns a blkbuf if one is available or NULL
*
******************************************************************************/
BLKBUFF *pc_scratch_blk(SDVOID) /*__fn__*/
{
        BLKBUFF *pblk;

        PC_ENTER_CRITICAL()
        scratch_pdrive++; /* We want a unique number for the alloc routine */
        PC_EXIT_CRITICAL()

        pc_alloc_blk(&pblk, scratch_pdrive, BLOCKEQ0);

        return (pblk);
}

#endif	/* (USE_FILE_SYSTEM) */

