//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 10/20/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/******************************************************************************
 * File Name: APIUTIL.C - Contains support code for API level source code.
 *
 * SanDisk Host Developer's Toolkit
 *
 * Copyright (c) 1996-1999 SanDisk Corporation
 * Copyright EBS Inc. 1996
 * All rights reserved.
 *
 * This code may not be redistributed in source or linkable object
 * form without the consent of its author.
 *
 * The following routines are included:
 *
 *    pc_log_base_2    -   Calculate log2(N).
 *    pc_dskopen       -   Open a disk for business.
 *    pc_dskclose      -   Flush all buffers for a disk and free all core.
 *    pc_diskabort     -   Abort all operations on a disk.
 *    pc_dskinit       -   Mount a disk.
 *    pc_idskclose     -   Unmount a disk.
 *    pc_get_cwd       -   Determine cwd string from current directory inode.
 *    pc_upstat        -   Copy directory entry info to a user's stat buffer  
 *    _synch_file_ptrs -   Make sure file pointers are synchronyzed
 *    pc_find_contig_clusters  - Find at least MIN_CLUSTER clusters.
 *
 ******************************************************************************/

#include "pcdisk.h"
#include "oem.h"


//#define FAST_BOOT
#if (USE_FILE_SYSTEM)

/*
  Each mounted drive needs to store either the whole FAT in core
  or to store portions of it in core with swapping to disk. 
  These declarations preallocate buffers in the BSS for this purpose.
*/


SDLOCAL COUNT pc_log_base_2 ( UCOUNT n );
SDBOOL do_media_check(INT16 driveno);
SDBOOL pc_diskflush(INT16 driveno);



/*****************************************************************************
 * Name: pc_log_base_2
 *
 * Description: - Calculate log2(N).
 *
 * Entries:
 *       UCOUNT   n      number to calculate log2(n)
 *
 * Returns:
 *       log2(n)
 *
 ******************************************************************************/
SDLOCAL COUNT pc_log_base_2 ( UCOUNT n ) /*__fn__*/
{
	COUNT log;

	log = 0;
	if ( n <= 1 )
		return(log);

	while ( n )
	{
		log += 1;
		n >>= 1;
	}

	return ((COUNT)(log-1));
}


/****************************************************************************
 * Name: PC_CHECKDRIVE
 *
 * Description:
 *       Get the drive number from a path specifier and do media check on 
 *       the drive.
 *
 * Entries:
 *       TEXT *name   pathname
 *
 * Returns
 *        driveno     if successful
 *             -1     if failed
 *
 ******************************************************************************/
INT16 check_drive( TEXT *name ) /*__fn__*/
{
	INT16 driveno;

	errno = 0;

	/* Get the drive and make sure it is mounted */
	if ( !pc_parsedrive(&driveno, name) )
	{
		errno = PEDEVICE;
		return (-1);
	}
	if ( do_media_check(driveno) )
		return (driveno);

	return (-1);
}


/****************************************************************************
 * Name: DO_MEDIA_CHECK
 *
 * Description:
 *       Check the mounting drive and its structure.  If the drive is 
 *       not mounted, it may be (re)mounted as a result of check_media_entry
 *       function call.
 * Entries:
 *       INT16   driveno         Drive number
 *
 * Returns
 *        YES     if successful
 *        NO      if failed
 *
 ******************************************************************************/
SDBOOL do_media_check(INT16 driveno) /*__fn__*/
{
	if( driveno < 0 || driveno > TOTAL_DRIVES ) {
		errno = ENODEV;
		return (NO);
	}
    if ( !check_media_entry(driveno) || !pc_drno2dr(driveno) )
    {
        /* TODO errno is set in check_media_status if INTERFACE_DEVICE_OPEN fails
         * so no need to reassign it here.  but check to see where else this function
         * is called from */
        //errno = PEDEVICE;
        return (NO);
    }
    
    return (YES);
}


/******************************************************************************
 * Name: PC_I_DSKOPEN -  Open a disk for business.
 *
 * Description:
 *    Called by lower level code in chkmedia to open the disk
 *
 *    THIS ROUTINE MUST BE CALLED BEFORE ANY OTHERS.
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns:
 *    Returns YES if the disk was successfully initialized.
 *
 ******************************************************************************/
SDBOOL pc_i_dskopen ( INT16 driveno ) /*__fn__*/
{
	DDRIVE *pdr;
	DRV_GEOMETRY_DESC *drv_geom;
	SDBOOL  ret_val;

	ret_val = NO;

	/* Initialize the drive */
	ret_val = pc_dskinit(driveno);
	if ( ret_val )
	{
		/* Pointer to the drive data structure */
		pdr = pc_drno2dr(driveno);
		if ( pdr )
		{
			drv_geom = (DRV_GEOMETRY_DESC *)fspath;	/* tm: fspath is scratch space, drive geometry is
                                                     * not used */
			/* Get device information */
			if ( !devio_read_serial(driveno, drv_geom) )
				return (NO);
		}
	}

	return (ret_val);
}

#ifndef FAST_BOOT

SDBOOL pc_get_FAT32freecluster(INT16 driveno) /*__fn__*/
{
    DDRIVE *pdr;

    /* Pointer to the drive data structure */
    pdr = &mem_drives_structures[driveno];


    if ( pdr->fasize == DRV_FAT32 )
    {
        pdr->known_free_clusters = 0L;
        pc_ifree(driveno);
    }
    return (YES);
}

#endif

/*****************************************************************************
 * Name: PC_DSKOPEN -  Flush all buffers for a disk and free all core.
 *
 * Description:
 *       Given a path name containing a valid drive specifier. Flush the
 *       file allocation table and purge any buffers or objects associated
 *       with the drive.
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns
 *    Returns YES if all went well.
 *
 ******************************************************************************/
SDBOOL pc_dskopen ( INT16 driveno ) /*__fn__*/
{
	SDBOOL  ret_val;

	CHECK_MEM(SDBOOL, 0);    /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(SDBOOL, 0);   /* Check if a valid user if multitasking */

    ret_val = NO;

	if ( do_media_check(driveno) )
	{

#ifndef FAST_BOOT
        pc_get_FAT32freecluster(driveno);
#endif

        ret_val = YES;
	}

	/* Restore the kernel state */
	PC_FS_EXIT();
    
    return (ret_val);
}



/******************************************************************************
 * Name: PC_DSKCLOSE -  Flush all buffers for a disk and free all core.
 *
 * Description
 *       Given a path name containing a valid drive specifier. Flush the
 *       file allocation table and purge any buffers or objects associated
 *       with the drive.
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns
 *       YES if all went well.
 *       NO if failure
 *
 ******************************************************************************/
SDBOOL pc_dskclose ( INT16 driveno ) /*__fn__*/
{
	SDBOOL  ret_val;

	CHECK_MEM(SDBOOL, 0);    /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(SDBOOL, 0);   /* Check if a valid user if multitasking */

    ret_val = NO;
	if ( pc_validate_driveno(driveno) )
	{
		/* Grab exclusive access to the drive */
		PC_DRIVE_ENTER(driveno, YES);
        ret_val = pc_idskclose(driveno);
		PC_DRIVE_EXIT(driveno);
    }

	/* Restore the kernel state */
	PC_FS_EXIT();
    
    return (ret_val);
}


/*****************************************************************************
 * Name: PC_DISKABORT -  Abort all operations on a disk and
 *                       Free all resources belonging to a drive
 *                       without flushing anything
 *
 *
 * Description
 *       If an application senses that there are problems with a disk, it
 *       should call pc_diskabort("D:"). This will cause all resources
 *       associated with that drive to be freed, but no disk writes will
 *       be attempted. All file descriptors associated with the drive
 *       become invalid. After correcting the problem call pc_diskopen("D:")
 *       to re-mount the disk and re-open your files.
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns:
 *       None
 *
 ******************************************************************************/
SDVOID pc_diskabort ( INT16 driveno ) /*__fn__*/
{
	VOID_CHECK_MEM();        /* Make sure memory is initted */
    PC_FS_ENTER();
    VOID_CHECK_USER();       /* Check if a valid user if multitasking */

    /* Grab exclusive access to the drive */
    PC_DRIVE_ENTER(driveno, YES);
    /* Release the drive unconditionally */
    pc_dskfree(driveno, YES);
	PC_DRIVE_EXIT(driveno);
    
    /* Restore the kernel state */
    PC_FS_EXIT();
    
    return;
}


#if (RTFS_WRITE)
/****************************************************************************
 * Name: PC_DISKFLUSH -  Flush the FAT and all files on a disk
 *                       Free all resources belonging to a drive
 *                       without flushing anything
 *
 * Description
 *       If an application may call this functions to force all files 
 *       to be flushed and the fat to be flushed. After this call returns
 *       the disk image is synchronized with RTFS's internal view of the
 *       voulme.
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns
 *       YES if the disk flushed else no
 *
 ******************************************************************************/
SDBOOL pc_diskflush ( INT16 driveno ) /*__fn__*/
{
	DDRIVE *pdrive;
	SDBOOL ret_val;

	CHECK_MEM(SDBOOL, 0)    /* Make sure memory is initted */
        PC_FS_ENTER()
        CHECK_USER(SDBOOL, 0)   /* Check if a valid user if multitasking */

        ret_val = NO;
	if ( do_media_check(driveno) )
	{
		/* Find the drive */
		pdrive = pc_drno2dr(driveno);
		if ( pdrive )
		{
			/* Grab exclusive access to the drive */
			PC_DRIVE_ENTER(driveno, YES)
                if ( pc_flush_all_files(pdrive) )
                {
                    if ( pc_flushfat(driveno) )
                        ret_val = YES;
                }
			PC_DRIVE_EXIT(driveno)
                }
	}

	/* Restore the kernel state */
	PC_FS_EXIT()

        return (ret_val);
}

#endif  /* (RTFS_WRITE) */

/* 
 * Note: This routine is called with the drive already locked so in
 *       several cases there is no need for critical section code handling
 */
/*****************************************************************************
 * Name: PC_DSKINIT -  Open a disk for business. (internal)
 *
 * Description:
 *       Given a drive number, open the disk by reading all of the block zero
 *       information and the file allocation table. (called by pc_dskopen())
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns:
 *       YES if the disk was successfully initialized.
 *
 ******************************************************************************/
SDBOOL pc_dskinit ( INT16 driveno ) /*__fn__*/
{
	DDRIVE  *pdr;
	UINT32  *pbuf;

	/* Check drive number */
	if ( !pc_validate_driveno(driveno) )
	{
		REPORT_ERROR(PCERR_INITDRNO);
		return (NO);
	}

	/* The memory should be initialized at this point. Make sure it does */
	if ( !mem_drives_structures )
	{
		/* Failed: pc_memory_init() must not have been called */
		REPORT_ERROR(PCERR_INITCORE);
		return (NO);
	}

	PC_ENTER_CRITICAL();
    pdr = &mem_drives_structures[driveno];
	pdr->dev_flag += 1;
	PC_EXIT_CRITICAL();
    

    /* Don't do anything on reopens */
    if ( pdr->dev_flag & DRIVE_INSTALLED )
    {
        return (YES);
    }
    else
    {
        /* Zero the structure so all of our initial values are right */
        PC_ENTER_CRITICAL();
        pc_memfill ( pdr, sizeof(DDRIVE), (UTINY)0 );
        pdr->dev_flag = 1;
        PC_EXIT_CRITICAL();
    }
    
	/* Clear the current working directory */
	fs_user->lcwd[driveno] = SDNULL;

	/* Get DOS partition table. Get Master Boot Record */
	if ( !devio_open(driveno) )
	{
        /* Found no partition table */
		REPORT_ERROR(PCERR_INITDEV);
		pdr->dev_flag = NO;
		return (NO);
	}

	/* Read block 0. This is the Boot Record. */
	if ( !pc_gblk0( driveno, pdr ) )
	{
		return(NO);
	}

    pdr->prevCluster = 0L;
    pdr->currCluster = 2L;
    pdr->prevDirCluster = 0L;  /* Use for FAT32 directory search */
    pdr->currDirCluster = 0L;  /* Use for FAT32 directory search */
    pdr->nextDirCluster = 0L;  /* Use for FAT32 directory search */

    if (pdr->fasize == DRV_FAT12 || pdr->fasize == DRV_FAT16 )
	{
		/* The first block of the root is just past the fat copies */
		pdr->rootblock = (BLOCKT)pdr->fatblock + (BLOCKT)(pdr->secpfat * pdr->numfats);
		pdr->secproot =  (pdr->numroot + INOPBLOCK - 1) / INOPBLOCK;

		/* The first block of the cluster area is just past the root */
		/* Round up if we have to */
		pdr->firstclblock = ( pdr->rootblock +
			((pdr->numroot + INOPBLOCK - 1) / INOPBLOCK) );
	}

	pdr->bytespcluster = (UCOUNT)(pdr->bytspsector * pdr->secpalloc);

	/* Bits to mask in to calculate byte offset in cluster from
	   file pointer. AND file pointer with this to get byte offset
	   in cluster a shift right 9 to get block offset in cluster.
	*/
	pdr->byte_into_cl_mask = (ULONG)pdr->bytespcluster;
	pdr->byte_into_cl_mask -= 1L;

	/* save away log of sectors per alloc */
	pdr->log2_secpalloc = pc_log_base_2 ( (UCOUNT)pdr->secpalloc );



	
	/*
      Keep track of how much free space is on the drive. (This will
      speed up pc_free()) when calculating free space.
      Always clear known_free_clusters, regardless of fat16 vs 32
	*/

	pdr->known_free_clusters = 0;


	/*
      Calculate the largest index in the file allocation table.
      Total # block in the cluster area)/Blockpercluster =='s total
      Number of clusters. Entries 0 & 1 are reserved so the highest
      valid fat index is 1 + total # clusters.
	*/
	pdr->maxfindex = (ULONG)(1 + (pdr->numsecs - pdr->firstclblock) / pdr->secpalloc);

    if ( pdr->fasize == DRV_FAT12 || pdr->fasize == DRV_FAT16 )
	{
		/*
          If calculated size > fff0 set it to one less.
          FFF0 to FFFF are reserved values.
		*/
		if ( pdr->maxfindex >= 0x0FFF0 ) {
			pdr->maxfindex = 0x0FFEF;
        }


		/*
          Create a hint for where we should write file data. We do this
          because directories are allocated in one cluster chunks while
          file may allocate larger chunks. We try to put directory 
          data at the beginning of the disk in a seperate region so we
          don't break the contiguous space further out.
		*/

		/* starting of free cluster allocated to file is alway cluster #2 */
        pdr->free_contig_base = 2L;

		/*
          Set the pointer to where to look for free clusters to the
          contiguous area. On the first call to write this will hunt
          for the real free blocks.
		*/
        pdr->free_contig_pointer = pdr->free_contig_base;
	}

	/* Initialize the FAT management code */
    pdr->fat_swap_structure.data_array = (UCHAR *)&fat_drives[((UINT16)driveno * FAT_BUFFER_SIZE)];
   
	/* Remember how many blocks we allocated */
	pdr->fat_swap_structure.n_blocks_total = FAT_BUFFER_SIZE;

	/* Set driveno now because the drive structure is valid */
	pdr->driveno = driveno; 

	/* Swap in available FAT. (i.e. read the first FAT_BUFFER_SIZE of the FAT) */
	/* Read in all FAT's sectors or until FAT_BUFFER_SIZE */
	pbuf = (UINT32 *)pc_FATRead(pdr, 0L, 0L, FAT_BUFFER_SIZE);
	if ( !pbuf )
	{
		pdr->dev_flag = NO;
		return (NO);
	}

    if ( pdr->fasize == DRV_FAT12 || pdr->fasize == DRV_FAT16 )
	{
        pdr->nextCluster = pc_clnext(pdr, 2L);
    }                

    if ( pdr->fasize == DRV_FAT32 )
    {
		pdr->secproot = 0;
        pdr->nextCluster = *(((UINT32 *)pbuf)+2);
        pdr->currDirCluster = 2L;
        pdr->nextDirCluster = *(((UINT32 *)pbuf)+2);
#if 0
        /* pdr->numroot contains Root Dir Start Cluster */
        pdr->rootblock = pc_cl2sector( pdr, (BLOCKT)pdr->numroot );
#endif
        pdr->rootblock = (BLOCKT)pdr->fatblock + (BLOCKT)(pdr->secpfat * pdr->numfats);
		pdr->firstclblock = pdr->rootblock;
        pdr->maxfindex = (ULONG)(1 + (pdr->numsecs - pdr->firstclblock) / pdr->secpalloc);

		/*
          If calculated size > 0x0FFFFFF0 set it to one less.
          0x0FFFFFF0 to 0x0FFFFFFF are reserved values.
		*/
        if ( pdr->maxfindex >= 0x0FFFFFF0L ) {
            pdr->maxfindex = 0x0FFFFFEFL;
        }

	}
	
	/* Now, the device is configured, set it to successful state. */
	pdr->dev_flag |= DRIVE_INSTALLED;



	return (YES);
}


/*****************************************************************************
 * Name: PC_IDSKCLOSE -  Flush all buffers for a disk and free all core. (internal)
 *
 * Description
 *       Given a valid drive number. Flush the file allocation table and
 *       purge any buffers or objects associated with the drive.
 *       (called by pc_dskclose)
 *
 * Note: This routine is called with the drive already locked so in
 *       several cases there is no need for critical section code handling
 *
 * Entries:
 *       INT16   driveno Drive Number
 *
 * Returns
 *       Returns YES if all went well.
 *
 ******************************************************************************/
SDBOOL pc_idskclose ( INT16 driveno ) /*__fn__*/
{
	SDBOOL ret_val;

	/* Check drive number */
	if ( pc_drno2dr(driveno) )
	{
#if (RTFS_WRITE)
		PC_FAT_ENTER(driveno)
            ret_val = pc_flushfat(driveno);
		PC_FAT_EXIT(driveno)
#endif
            if ( ret_val )        
            {
                /* Release the drive if dev_flag == 0 */
                pc_dskfree ( driveno, NO );
                return (YES);
            }
	}

	return (NO);
}


/******************************************************************************
 * Name: PC_UPSTAT - Copy private information to public fields for a DSTAT struc.
 *
 * Description:
 *       Given a pointer to a DSTAT structure that contains a pointer to an 
 *       initialized DROBJ structure, load the public elements of DSTAT with
 *       name filesize, date of modification et al.
 *       (Called by pc_gfirst & pc_gnext)
 *
 * Note: Copy internal stuf so the outside world can see it.
 *
 * Entries:
 *       DSTAT *statobj
 *
 * Returns:
 *       None
 *
 ******************************************************************************/
SDVOID pc_upstat ( FSDSTAT *statobj ) /*__fn__*/
{
    DROBJ   *pobj;
    FINODE  *pi;

    pobj = statobj->pobj;
    pi = pobj->finode;
       
    copybuff( statobj->fileinfo->fname, pi->fname, (8 * sizeof(UTEXT)) );
    copybuff( statobj->fileinfo->fext, pi->fext, (3 * sizeof(UTEXT)) );

    statobj->fileinfo->fname[8] = 0;
    statobj->fileinfo->fext[3] = 0;

    statobj->fileinfo->fattribute = pi->fattribute;
    statobj->fileinfo->ftime = pi->ftime;
    statobj->fileinfo->ftime_tenths = pi->ftime_tenths;
    statobj->fileinfo->fdate = pi->fdate;
    statobj->fileinfo->fcrttime = pi->fcrttime;
    statobj->fileinfo->fcrtdate = pi->fcrtdate;
    statobj->fileinfo->fsize = pi->fsize;

    statobj->fileinfo->longFileName[0] = 0;
    if (currFileName[0] != 0)
        copybuff((SDVOID *)statobj->fileinfo->longFileName, (SDVOID *)currFileName, (EMAXPATH * sizeof(UTEXT)));
}



/******************************************************************************
 * Name: _synch_file_ptrs
 *
 * Description:
 *       Synchronize file pointers. Read write Seek and close all call here.
 *       This fixes the following BUGS:
 *       1. If a file is created and left open and then opened again with a new 
 *          file handle before any writing takes place. Neither file will get
 *          its fptr_cluster set correctly initially. The first one to write 
 *          would get set up correctly but the other wouldn't. Thus if fptr_cluster
 *          is zero we see if we can set it.
 *       2. If one file seeked to the end of the file or has written to the end of 
 *          the file its file pointer will point beyond the last cluster in the 
 *          chain, the next call to write will notice the fptr is beyond the 
 *          file size and extend the file by allocating a new cluster to the
 *          chain. During this time the cluster/block and byte offsets are
 *          out of synch. If another instance extends the file during this time 
 *          the next call to write will miss this condition since fptr is not 
 *          >= fsize any more. To fix this we note in the file when this 
 *          condition is true AND, afterwards each time we work with the file 
 *          we see if the file has grown and adjust the cluster pointer and block
 *          pointer if needed.
 * 
 * Note: The finode owned by the file is always locked when this routine is 
 *       called so the routine does not need to be reentrant with respect to
 *       the finode. Note too that pfile is not a shared structure so the
 *       routine doesn't have to be reentrant with respect to it either.
 ******************************************************************************/
SDVOID  _synch_file_ptrs ( PC_FILE *pfile ) /*__fn__*/
{
	UINT32  clno;

	if ( !pfile->fptr_cluster )
	{
		/* For all FAT types, including FAT32 */
		pfile->fptr_cluster = (UINT32)pfile->pobj->finode->fclusterHi;
		pfile->fptr_cluster <<= 16;
		pfile->fptr_cluster |= (UINT32)pfile->pobj->finode->fcluster;

		/* Current cluster - note on a new file this will be zero */
		if ( pfile->fptr_cluster )
			pfile->fptr_block = pc_cl2sector(pfile->pobj->pdrive,
                pfile->fptr_cluster);
		else
			pfile->fptr_block = 0;
	}

	if ( pfile->at_eof )
	{
		if ( pfile->fptr_cluster )
		{
			clno = pc_clnext(pfile->pobj->pdrive, pfile->fptr_cluster);
			if ( clno )
			{
				/* Check for FAT8, FAT16, and FAT32 */
                if ( pfile->pobj->pdrive->fasize == DRV_FAT12 ||
                    pfile->pobj->pdrive->fasize == DRV_FAT16 )
				{
					pfile->fptr_cluster = (clno & 0x0FFFF);
				}
				else
				{
					pfile->fptr_cluster = clno & 0x0FFFFFFF;
				}

				pfile->fptr_block = pc_cl2sector(pfile->pobj->pdrive,
                    clno);
				pfile->at_eof = NO;
			}
		}
	}
}


#if (RTFS_WRITE)
/******************************************************************************
 * Name: PC_FIND_CONTIG_CLUSTERS  - Find at least MIN_CLUSTER clusters.
 *
 *
 * Description:
 *        Using the provided method, search the FAT from start_pt to the
 *        end for a free contiguous chain of at least MIN_CLUSTERS. If less
 *        than MIN_CLUSTERS are found the largest free chain in the region is
 *        returned.
 *
 *        There are three possible methods:
 *            PC_FIRST_FIT  - The first free chain >= MIN_CLUSTERS is returned
 *            PC_BEST_FIT - The smallest chain    >= MIN_CLUSTERS is returned
 *            PC_WORST_FIT  - The largest chain   >= MIN_CLUSTERS is returned
 *
 *        Choose the method that will work best for you.
 *
 * Note: PC_FIRST_FIT is significantly faster faster than the others
 *       The chain is not created. The caller must convert the 
 *       clusters to an allocated chain.
 *       The FAT is locked before this call is made.
 *
 * Returns
 *       Returns the number of contiguous clusters found up to MIN_CLUSTERS.
 *       *pchain contains the cluster number at the beginning of the chain.
 *       On error return 0xFFFFFFFF.
 *
 * Example:
 *       Get the largest free chain on the disk:
 *       large = pc_find_contig_clusters(pdr, 2, &chain, 0xffff, PC_FIRST_FIT);
 *
 ******************************************************************************/
UINT32 pc_find_contig_clusters(DDRIVE *pdr, UINT32 startpt, UINT32 *pchain, UINT16 min_clusters, INT16 method) /* __fn__ */
{
	UINT32  ii;
	UINT32  chain_start;
	UINT32  largest_size;
	UINT32  largest_chain;
	UINT32  endpt;
	UINT32  value;
	UINT32  best_size;
	UINT32  chain_size;
	UINT32  best_chain;


	best_chain = 0L;
	best_size = 0L;
	chain_start = 0L;
	chain_size = 0L;
	largest_size  = 0L;
	largest_chain = 0L;
	endpt = pdr->maxfindex;


	for (ii = startpt; ii <= endpt; ii++)
	{
		if ( !pc_faxx(pdr, ii, &value) ) 
			return (0xFFFFFFFF); /* IO error .. oops */

		if ( value == 0L )
		{   
			/* Cluster is free. Run some tests on it. */
			if ( chain_start )
			{
				/* We're in a contiguous region already. Bump the count */
				chain_size++;
			}
			else
			{
				/* Just starting a contiguous region */
				chain_size = 1L;
				chain_start = ii;
			}

			/* If using first fit see if we crossed the threshold */
			if ( method == PC_FIRST_FIT )
			{
				if ( chain_size >= min_clusters )
				{
					best_chain = chain_start;
					best_size = chain_size;
					break;
				}
			}
		} /* if value == 0 */

		/* Did we just finish scanning a contiguous chain ?? */
		if ( chain_size && ((value != 0L) || (ii == endpt)) )
		{
			/* Remember the largest chain */
			if ( chain_size > largest_size )
			{
				largest_size  = chain_size;
				largest_chain = chain_start;
			}

			if ( method == PC_BEST_FIT )
			{
				if ( chain_size == min_clusters )
				{
					/* The chain is exactly the size we need take it. */
					best_chain = chain_start;
					best_size = chain_size;
					break;
				}

				if ( chain_size > min_clusters )
				{
					if ( !best_chain ||
						(chain_size < best_size) )
					{
						/* Chain is closest to what we need so far note it. */
						best_size = chain_size;
						best_chain = chain_start;
					}
				}
			} /* if BEST_FIT */
			else if ( method == PC_WORST_FIT )
			{
				if ( chain_size >= min_clusters )
				{
					if ( !best_chain ||
						chain_size > best_size )
					{
						best_size = chain_size;
						best_chain = chain_start;
					}
				}
			} /* if WORST_FIT */
			/* 
             *           else if (method == PC_BEST_FIT)
             *               ;
             */
			chain_size = 0L;
			chain_start = 0L;
		} /* if (chain_size && ((value != 0L) || (i == endpt)) ) */
	} /* for (ii = startpt; ii <= endpt; ii++) */

	/* If we have a best chain return it here. Else return the largest chain */
	if ( best_chain )
	{
		*pchain = best_chain;
		return (best_size);
	}
	else
	{
		*pchain = largest_chain;
		return (largest_size);
	}
}

#endif  /* (RTFS_WRITE) */


/******************************************************************************
 * Name: PC_FINODE_STAT - Convert finode information to stat info for
 *                        stat and fstat
 *
 * Description:
 *       Given a pointer to a FINODE and a STAT structure
 *       load STAT with filesize, date of modification et al. Interpret
 *       the fattributes field of the finode to fill in the st_mode
 *       field of the stat structure.
 *
 * Entries:
 *       FINODE *pi
 *       STAT *pstat
 *
 * Returns:
 *       None
 *
 ******************************************************************************/
SDVOID pc_finode_stat ( FINODE *pi, STAT *pstat ) /*__fn__*/
{
	pstat->st_dev  = pi->my_drive->driveno; /* (drive number, rtfs) */
	pstat->st_ino  = 0;                     /* inode number (0) */
	pstat->st_mode = 0;                     /* (see S_xxxx below) */

	/* Store away the DOS file attributes in case someone needs them */
	pstat->fattribute = pi->fattribute;
	pstat->st_mode |= S_IREAD;
	if ( !(pstat->fattribute & ARDONLY) )
		pstat->st_mode |= S_IWRITE;

	if ( pstat->fattribute & ADIRENT )
		pstat->st_mode |= S_IFDIR;

	if ( !(pstat->fattribute & (AVOLUME|ADIRENT)) )
		pstat->st_mode |= S_IFREG;

	pstat->st_nlink = 1;             /* (always 1) */
	pstat->st_rdev  = pstat->st_dev; /* (drive number, rtfs) */
	pstat->st_size  = pi->fsize;     /* file size, in bytes */

	pstat->st_atime.date  = pi->fdate;      /* last access  */
	pstat->st_atime.time  = pi->ftime;
	pstat->st_mtime  = pstat->st_atime;     /* last modification */
    pstat->st_ctime.date  = pi->fcrtdate;   /* last status change */
    pstat->st_ctime.time  = pi->fcrttime;

	/* Optimal buffering size. is a cluster */
	pstat->st_blksize = (ULONG) pi->my_drive->bytespcluster;

	/* Blocks is file size / 512. with round up */
	pstat->st_blocks  =  (ULONG) ((pi->fsize + 511) >> 9);
}



/******************************************************************************
 * Name: validate_current_information - Check for a valid path and 
 *                       convert information to name and extension
 *
 * Description:
 *
 * Entries:
 *
 * Returns:
 *       YES if successfully
 *       NO if failure
 *
 ******************************************************************************/
SDBOOL validate_current_information( TEXT *topath, TEXT *filename, TEXT *fileext, TEXT *name ) /*__fn__*/
{
    INT16 tmpLength;

	/* Get out the filename and d:parent */
	if ( !pc_parsepath(topath, filename, fileext, name) )
		return NO;

	if ( !validate_filename(filename, 8) )
		return NO;

	if ( !validate_filename(fileext, 3) )
		return NO;

	if ( longFileName )
	{
        tmpLength = (INT16)(longFileName - topath);
        saveFileName = name + tmpLength;
	}
    else
    {
#if 0
        saveFileName = filename;
#endif
        saveFileName = SDNULL;
    }

	return YES;
}

UINT16 *lock_temporary_buffer(SDVOID);
SDVOID release_temporary_buffer(SDVOID);

UINT16 *lock_temporary_buffer(SDVOID)
{
	fslocal_buffer = pc_scratch_blk();
	return ((UINT16 *)fslocal_buffer->data);
}

SDVOID release_temporary_buffer(SDVOID)
{
    // dc - tell pc_free_buf to do its critical section code
	pc_free_buf(fslocal_buffer, YES, YES);
}

#endif  /* (USE_FILE_SYSTEM) */

