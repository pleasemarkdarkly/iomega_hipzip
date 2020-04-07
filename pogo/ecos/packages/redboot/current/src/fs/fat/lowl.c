//........................................................................................
//........................................................................................
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 8/23/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
* FileName:  LOWL.C - Low level File allocation table management functions. 
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright EBS Inc. 1996
* All rights reserved.
*
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Low level File allocation table management functions. 
*
* The following routines in this file are included:
*
*        pc_alloc_chain      -   Allocate a chain from the FAT.
*        pc_find_free_cluster-   Find the first free cluster in a given range.
*        pc_clalloc          -   Allocate a single cluster in the fragmented region.
*        pc_clgrow           -   Grow a directory chain in the fragmented region.
*        pc_clnext           -   Get the next cluster in a chain.
*        pc_clrelease        -   Return a cluster to the free list.
*        pc_faxx             -   Get a value from the FAT.
*        pc_flushfat         -   Make sure the FAT is up to date on disk.
*        pc_freechain        -   Release a chain to the free list.
*        pc_cl_truncate      -   Truncate a cluster chain.
*        pc_get_chain        -   Return contiguous clusters in a chain.
*        pc_pfaxx            -   Put a value to the FAT.
*        pc_pfswap           -   Swap a block of the FAT into the cache.
*        pc_pfflush          -   Flush the swap cache to disk.
*        pc_gblk0            -   Read block zero and set up internal structures.
*        pc_clzero           -   Write zeroes to a cluster on disk.
*        pc_drno2dr          -   Convert a drive number to a drive structure.
*        pc_dskfree          -   Free resources associated with a drive.
*        pc_ifree            -   Calculate free space from the FAT.
*        pc_sec2cluster      -   Convert a sector number to a cluster value.
*        pc_sec2index        -   Convert a sector number to a cluster offset.
*        pc_cl2sector        -   Convert a cluster value to a sector number.
*        partition_init      -   Interpret a partition table
*
******************************************************************************/

#include <fs/fat/pcdisk.h>


#if (USE_FILE_SYSTEM)


/*****************************************************************************
* Name: PC_CLNEXT - Return the next cluster in a cluster chain
*
* Description:
*       Given a DDRIVE and a cluster number, return the next cluster in the 
*       chain containing clno. Return 0 on end of chain.
*
* Entries:
*       DDRIVE *pdr
*       UINT16  clno
*
* Returns:
*       Return a new cluster number in a chain or 0 on end of chain.
*
******************************************************************************/
UINT32 pc_clnext(DDRIVE *pdr, UINT32 clno) /*__fn__*/
{
        UINT32  nxt;
        UINT32  nxt2;

	/* Get the value at clno. return 0 on any io errors */
        if (! pc_faxx(pdr, clno, &nxt) )
                return ( (UINT32)0L );
	nxt2 = nxt;

        if ( pdr->fasize == DRV_FAT12 )         /* 12-bit FAT ? */
                nxt2 |= 0x0FFFF000L;
        else if ( pdr->fasize == DRV_FAT16 )    /* 16-bit FAT */
                nxt2 |= 0x0FFF0000L;

        if ( (0x0FFFFFF7L < nxt2) && (nxt2 <= 0x0FFFFFFFL) )
                nxt = 0L;               /* end of chain */

        nxt &= 0x0FFFFFFFL;
        return (nxt);
}


UINT32 pc_clprev(DDRIVE *pdr, UINT32 clno) /*__fn__*/
{
        UINT32  nxt;
        UINT32  nxt2;
        UINT32  curClno;

	/* Get the value at clno. return 0 on any io errors */

        if (clno == 2L)
                return ( (UINT32)0L );

        curClno = 2L;
        do
        {
                if (! pc_faxx(pdr, curClno, &nxt) )
                        return ( (UINT32)0L );

                nxt2 = nxt;

                if ( pdr->fasize == DRV_FAT12 )         /* 12-bit FAT ? */
                        nxt2 |= 0x0FFFF000L;
                else if ( pdr->fasize == DRV_FAT16 )    /* 16-bit FAT */
                        nxt2 |= 0x0FFF0000L;

                if ( (0x0FFFFFF7L < nxt2) && (nxt2 <= 0x0FFFFFFFL) )
                        nxt = 0L;               /* end of chain */

                nxt &= 0x0FFFFFFFL;

                if ( nxt == clno )
                        return (curClno);

                curClno++;
        } while (curClno <= pdr->maxfindex);

        return ( (UINT32)0L );
}



/******************************************************************************
* Name: PC_FAXX - Get the value store in the FAT at clno.
*
* Description:
*       Given a DDRIVE and a cluster number. Get the value in the fat
*       at clusterno (the next cluster in a chain.).
*       Handle 32, 16 and 12 bit fats correctly.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 clno
*       UINT16 *pvalue
*
* Returns
*       Returns the the value at clno. In pvalue. 
*       If any error occured while FAT swapping return NO else return YES.
* Note: If fat buffering is disabled, always returns yes
*
******************************************************************************/
SDBOOL pc_faxx(DDRIVE *pdr, UINT32 clusterno, UINT32 *pvalue) /*__fn__*/
{
        UINT16  *pFATbuf;
        UINT32  index;
        UINT32  result;
        UINT32  offset;  
        UINT32  dtemp;
        UINT16  tmpOffset;

        index = clusterno;

        if ( pdr->fasize == DRV_FAT12 ) /* FAT12, 3 nibble ? */
        {
                offset = index + (clusterno >> 1);
                index = (offset >> 1);
        }

        /* Get the FAT buffer containing current cluster information */
        pFATbuf = (UINT16 *)pc_pfswap( pdr, index);
        if ( pFATbuf == SDNULL )
                return (NO);
        

        if ( pdr->fasize == DRV_FAT12 || pdr->fasize == DRV_FAT16) /* FAT12 or FAT16 ? */
        {
                /* Get cluster information */
                tmpOffset = pFATbuf[(index & 0xFF)];
                /* LITTLE to BIG endian conversion if required */
#if (USE_HW_OPTION)
                result = (UINT32)to_WORD((UCHAR *)&tmpOffset);
#else
                result = (UINT32)tmpOffset;
#endif
        }
        else    /* FAT32 */
        {
                /* Get cluster information */
                offset = ((UINT32 *)pFATbuf)[(index & 0x7F)];
                /* LITTLE to BIG endian conversion if required */
#if (USE_HW_OPTION)
                result = to_DWORD((UCHAR *)&offset);
#else
                result = (UINT32)offset;
#endif
        }


        if ( pdr->fasize == DRV_FAT12 ) /* FAT12, 3 nibble ? */
        {
                offset = clusterno & 0x03;

                if ( offset == 0 ) /* (A2 << 8) | A1 A2 */
                        /* if ( offset == 0 )
			|   W0      |   W1      |   W2  |
			A1 A0 B0 A2 B2 B1 C1 C0 D0 C2 D2 D1
                        xx xx xx
                        */
                        result &= 0x0FFF;

                else if ( offset == 3 ) /* (D2 D1) << 4 | D0 */   
                        /* if ( offset == 3 )
			|   W0      |   W1      |   W2  |
			A1 A0 B0 A2 B2 B1 C1 C0 D0 C2 D2 D1
                                                   xx xx xx
                        */
                        result >>= 4;
                else    /* ( offset == 1  or offset == 2 ) */
                {
                        result >>= 8;

                        if (offset == 1)
                        {
                        /* ( offset == 1 )
                                |   W0      |   W1      |   W2  |
                                A1 A0 B0 A2 B2 B1 C1 C0 D0 C2 D2 D1
                                         xx xx xx 
                        */
                                result >>= 4;
                        }
                        else
                        {
                        /* ( offset == 2 )
                                |   W0      |   W1      |   W2  |
                                A1 A0 B0 A2 B2 B1 C1 C0 D0 C2 D2 D1
                                                  xx xx xx 
                        */
                        }
                        /* Next index information */
                        index++;

                        /* Get the FAT buffer containing the cluster information */
                        pFATbuf = (UINT16 *)pc_pfswap( pdr, index );
                        if ( pFATbuf == SDNULL )
                                return (NO);

                        /* Get data */
                        tmpOffset = pFATbuf[(index & 0xFF)];

                        /* LITTLE to BIG endian conversion if required */
#if (USE_HW_OPTION)
                        index = to_WORD((UCHAR *)&tmpOffset);
#else
                        index = tmpOffset;
#endif
                        dtemp = index & 0xFF;

                        index = dtemp << 4;
                        if ( offset == 2 )
                        {
                                dtemp = index & 0xF0;
                                index = dtemp << 4;
                        }

                        result |= index;
                }
        }

        *pvalue = result;

	return (YES);
}


/******************************************************************************
* Name: PC_GET_CHAIN  -  Return as many contiguous clusters as possible.
*
* Description:
*        Starting at start_cluster return the number of contiguous clusters
*        allocated in the chain containing start_cluster or n_clusters,
*        whichever is less.
*
* Note: The caller locks the fat before calling this routine.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 start_cluster
*       UINT16 *pnext_cluster
*       UCOUNT n_clusters
*
* Returns:
*       Returns the number of contiguous clusters found. Or zero on an error.
*       This function should always return at least one. (start_cluster).
*       Unless an error occurs.
*
*       The word at *pnext_cluster is filled with on of the following:
*        . If we went beyond a contiguous section it contains
*          the first cluster in the next segment of the chain.
*        . If we are still in a section it contains
*          the next cluster in the current segment of the chain.
*        . If we are at the end of the chain it contains the last cluster 
*          in the chain.
*
******************************************************************************/
UINT16 pc_get_chain(DDRIVE *pdr, UINT32 start_cluster,
                        UINT32 *pnext_cluster, UCOUNT n_clusters) /*__fn__*/
{
        UINT32  clno;
        UINT32  value;
        UINT32  tValue;
        UCOUNT  n_contig;


        if ( (start_cluster < 2) || (start_cluster > pdr->maxfindex) )
		return (0);

	clno = start_cluster;
	n_contig = 1;
        *pnext_cluster = 0L;     

	/*
	Get each FAT entry. If its value points to the next contiguous entry
	continue. Otherwise we have reached the end of the contiguous chain.
	At which point we return the number of contig's found and by reference
	the address of the FAT entry beginning the next chain segment.
	*/

        for ( value = 0L; ; )
	{
                if ( !pc_faxx(pdr, clno, &value) )
                        return (0);


                if ( pdr->fasize == DRV_FAT12 )
                        tValue = value | 0x0FFFF000L;
                else if ( pdr->fasize == DRV_FAT16 )
                        tValue = value | 0x0FFF0000L;
                else
                        tValue = value;

		/* check for a bad cluster and skip it if we see it */
                if ( tValue == 0x0FFFFFF7L )
		{
                        clno += 1L;
		}
                else if ( tValue > 0x0FFFFFF7L )
		{
			/*
			Check for end markers set next cluster to the last
			cluster in the chain if we are at the end.
			*/
			value = clno;
			break;
		}
                else if ( value == ++clno )
		{
                        if ( n_contig >= n_clusters )
				break;
			n_contig++;
		}
		else
			break;
	}

        *pnext_cluster = (value & 0x0FFFFFFFL);
	return (n_contig);
}


#if (RTFS_WRITE)

/******************************************************************************
* Name: PC_ALLOC_CHAIN  -  Allocate as many contiguous clusters as possible.
*
* Description
*       Reserve up to n_clusters contiguous clusters from the FAT and
*       return the number of contiguous clusters reserved.
*       If pstart_cluster points to a valid cluster link the new chain
*       to it.
* Note: The FAT should be locked before calling this routine.
*       It was possible on a fairly full disk to have a write fail reporting
*       no space on the drive when there really is enough space. The problem
*       happens when the following condition is true. A file is open for
*       writing and there are no more free clusters available in the file
*       allocation table between the last cluster used by the file and the
*       and the end of the FAT .. and .. the first 1 / 32 of the drive is
*       also already filled up. RTFS favors putting directory information in
*       the first 1/32 of the drive so this region doesn't usually fill up.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 *pstart_cluster
*       UCOUNT n_clusters
*
* Returns
*       Returns the number of contiguous clusters found. Or zero on an error.
*       pstart_cluster contains the address of the start of the chain on
*       return.
*
*****************************************************************************/
UCOUNT pc_alloc_chain(DDRIVE *pdr, UINT32 *pstart_cluster, UCOUNT n_clusters) /*__fn__*/
{
        UINT32  start_cluster;
        UINT32  first_new_cluster;
        UINT32  clno;
        UINT32  value;
        UINT32  last_cluster;
        UINT16  n_contig;

	start_cluster = *pstart_cluster;

        if ( start_cluster && 
                ( (start_cluster < 2) || (start_cluster > pdr->maxfindex) ) )
		return (0);

	/*
	If the user provided a cluster we find the next cluster beyond that
	one. Otherwise we look at the disk structure and find the next 
	free cluster in the free cluster region after the current best guess
	of the region. If that fails we look to the beginning of the region
	and if that fails we look in the non-contiguous region.
	*/

        clno = 0L;

        if ( start_cluster )
	{
		/* search from the start_cluster hint to the end of the fat */
                clno = pc_find_free_cluster( pdr,
				start_cluster,
                                pdr->maxfindex );

		/* If we search again search only to the start_cluster */
		last_cluster = start_cluster;
	}
	else
		/* When we search again search to the end */
		last_cluster = pdr->maxfindex;


	/* Check the most likely place to find contiguous space */
        if ( !clno )
	{
                if ( !start_cluster ||
                        (start_cluster >= pdr->free_contig_pointer) )
		{
			/* search from free_contig_pointer to start_cluster
                        or maxfindex whichever is less.
                        */
                        clno = pc_find_free_cluster( pdr,
					pdr->free_contig_pointer,
                                        last_cluster );
			/* If we search again search only to the free_contig_pointer */
			last_cluster = pdr->free_contig_pointer;
		}
	}

	/* Check the area of the disk beyond where we typically write fragments */
        if ( !clno )
	{
                if ( !start_cluster ||
                        (start_cluster > pdr->free_contig_base) )
		{
                        /*
                        search from free_contig_base to start_cluster or
			free_contig_pointer whichever is less
			*/
                        clno = pc_find_free_cluster( pdr,
					pdr->free_contig_base,
                                        last_cluster );
		}


                /* Check the beginning of the  the disk where we typically write fragments */
                if ( !clno )
                {
                        clno = pc_find_free_cluster( pdr, 2L, pdr->free_contig_base );

                        /*
                        We didn't find any clusters. Scan the whole fat again. This
                        should never work but we did have a bug in this area once
                        before ...
                        */
                        if ( !clno )
                        {
                                clno = pc_find_free_cluster( pdr, 2L, pdr->maxfindex );
                                return (0);
                        }
                }
        }

	first_new_cluster = clno;
        value = 0L;
	n_contig = 1;

	/*
	Look up the FAT. If the next cluster is free we link to it
	and up the contig count.
	*/
	while ( (n_contig < n_clusters) && (clno < pdr->maxfindex) )
	{
                if ( !pc_faxx(pdr, (clno+1), &value) )
                        return (0);

		/* If the next cluster is in-use we're done. */
                if ( value )
			break;

		/* Link the current cluster to the next one */
                if ( !pc_pfaxx(pdr, clno, (clno + 1L)) )
                        return (0L);

                n_contig += 1;  /* Yep.. we got another */
                clno += 1L;      /* Up the FAT table */
	}

	/* Terminate the list we just made */
        if ( !pc_pfaxx(pdr, clno, 0x0FFFFFFFL) )
		return (0);

	/* Update the hint of most likeley place to find a free cluster */
        if ( (clno < pdr->maxfindex) && (clno >= pdr->free_contig_pointer) )
                pdr->free_contig_pointer = clno + 1L;

	/*
	If we were handed a starting cluster we have to stick our new
	chain after it.
	*/
        if ( start_cluster )
	{
                if ( !pc_pfaxx(pdr, start_cluster, first_new_cluster) )
			return (0);
	}

	*pstart_cluster = first_new_cluster;

        if ( pdr->known_free_clusters )
                pdr->known_free_clusters = pdr->known_free_clusters - (UINT32)n_contig;

        return (n_contig);
}


/*****************************************************************************
* Name: pc_find_free_cluster
*
* Description:
*       Find the first free cluster in a range.
* Note: The caller locks the fat before calling this routine
*
* Entries:
*       DDRIVE *pdr
*       UINT16 startpt
*       UINT16 endpt
*
* Returns:
*
******************************************************************************/
UINT32 pc_find_free_cluster(DDRIVE *pdr, UINT32 startpt, UINT32 endpt) /*__fn__*/
{
        UINT32  value;
        UINT32  i;

	for (i = startpt; i < endpt; i++)
	{
		if ( !pc_faxx(pdr, i, &value) ) 
                        return (0L);

                /* Free cluster should be zero */
                if ( value == 0L )
                        return (i);
	}

        return (0L);
}


#if (RTFS_SUBDIRS)
/******************************************************************************
* Name: PC_CLALLOC - Reserve and return the next free cluster on a drive
*                                              
* Description:
*       Given a DDRIVE, mark the next available cluster in the file
*       allocation table as used and return the associated cluster number.
*       Clhint provides a means of selecting clusters that are near each
*       other. This should reduce fragmentation.
*                                              
* NOTE: This routine is used to allocate single cluster chunks for
*       maintaining directories. We artificially break the disks into
*       two regions. The first region is where single clusters chunks
*       used in directory files come from. These are allocated by this
*       routine only. Data file clusters are allocated by pc_alloc_chain.
*                                              
*       THE DISK IS NOT REALLY PARTITIONED. If this routine runs out of
*       space in the first region it grabs a cluster from the second 
*       region.
*       The FAT should be locked before calling this routine.
*
* Returns:
*       Return a new cluster number or 0 if the disk is full.
*                                              
******************************************************************************/
UINT32 pc_clalloc(DDRIVE *pdr, UINT32 clhint) /*__fn__*/
{
        UINT32  clno;

        if ( clhint < 2L )
                clhint = 2L;
        if ( clhint >= pdr->free_contig_base )
                clhint = 2L;

        clno = 0L;

	/* Look in the "fragmentable" region first from clhint up */
        clno = pc_find_free_cluster( pdr, clhint, pdr->free_contig_base );

	/* Look in the  "fragmentable" region up to clhint */
        if ( !clno )
        {
                clno = pc_find_free_cluster( pdr, 2L, clhint );

                /* Look in the contiguos region if the "fragmentable" region is full */
                if ( !clno )
                        clno = pc_find_free_cluster( pdr, pdr->free_contig_base, pdr->maxfindex );

                if ( !clno )
                        return (0L);
        }

	/* Mark the cluster in use */
        if ( !pc_pfaxx(pdr, clno, 0x0FFFFFFFL) )
                return (0L);

        if ( pdr->known_free_clusters )
		pdr->known_free_clusters -= 1;

        return (clno);
}


/******************************************************************************
* Name: PC_CLGROW - Extend a cluster chain and return the next free cluster
*
* Description:
*       Given a DDRIVE and a cluster, extend the chain containing the cluster
*       by allocating a new cluster and linking clno to it. If clno is zero
*       assume it is the start of a new file and allocate a new cluster.
*
* Note: The chain is traversed to the end before linking in the new 
*       cluster. The new cluster terminates the chain.
*       The FAT should be locked before calling this routine.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 clno
*
* Returns:
*    Return a new cluster number or 0 if the disk is full.
*
******************************************************************************/
UINT32  pc_clgrow(DDRIVE *pdr, UINT32 clno) /*__fn__*/
{
        UINT32  nxt;
        UINT32  nextcluster;

	/* Make sure we are at the end of chain */
        if ( clno )   
	{
                nextcluster = pc_clnext( pdr, clno );
                while ( nextcluster )
		{
			clno = nextcluster;
                        nextcluster = pc_clnext( pdr, clno );
		}
	}


	/*
	Get a cluster, clno provides a hint for more efficient
	cluster allocation.
	*/
	nxt = pc_clalloc(pdr, clno);
        if ( !nxt )
                return (0L);

	/* Attach it to the current cluster if not at the begining of the chain */
        if ( clno )
        {
                if ( !pc_pfaxx(pdr, clno, nxt) )
                        return (0L);
        }

        return (nxt);
}
#endif  /* (RTFS_SUBDIRS) */


/******************************************************************************
* Name: PC_CLRELEASE - Return a cluster to the pool of free space on a disk
*
* Description:
*       Given a DDRIVE and a cluster, mark the cluster in the file allocation
*       table as free. It will be used again by calls to pc_clalloc().
*
* Entries:
*       DDRIVE *pdr
*       UINT16 clno
*
* Returns:
*       None
*
******************************************************************************/
SDVOID pc_clrelease(DDRIVE *pdr, UINT32 clno) /*__fn__*/
{
        if ( clno < 2L )
		return;

	/* Don't catch any lower level errors here. You'll catch them soon enough */
        if ( pc_pfaxx(pdr, clno, 0x00000000L) ) /* Mark it as free */
	{
		/* If freeing in the "contiguous" region reset the "hint" if
		   we free space earlier than it.
		*/
                if ( (clno >= pdr->free_contig_base) &&
                                (clno <= pdr->free_contig_pointer) )
			pdr->free_contig_pointer = clno;

                if ( pdr->known_free_clusters )
			pdr->known_free_clusters += 1;
	}
}



/*****************************************************************************
* Name: PC_FLUSHFAT -  Write any dirty FAT blocks to disk
*
* Description
*       Given a valid drive number. Write any fat blocks to disk that
*       have been modified. Updates all copies of the fat.
*
* Entries:
*       INT16 driveno
*
* Returns:
*       Returns NO if driveno is not an open drive. Or a write failed.
*
******************************************************************************/
SDBOOL pc_flushfat(INT16 driveno) /*__fn__*/
{
	DDRIVE *pdr;

	pdr = pc_drno2dr(driveno);

        if ( pdr && (!pdr->fat_is_dirty || pc_pfflush(pdr)) )
	{
		pdr->fat_is_dirty = NO;
                return (YES);
	}

        return (NO);
}


/******************************************************************************
* Name: PC_FREECHAIN - Free a cluster chain associated with an inode.
*
* Description:
*       Trace the cluster chain starting at cluster and return all the
*       clusters to the free state for re-use. The FAT is not flushed.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 cluster
*
* Returns:
*       None.
*
******************************************************************************/
SDVOID pc_freechain(DDRIVE *pdr, UINT32 cluster) /*__fn__*/
{
        UINT32  blockno;
        UINT32  nextcluster;

	nextcluster = pc_clnext(pdr, cluster);
        while ( cluster )
	{
                pc_clrelease( pdr, cluster );
                blockno = pc_cl2sector( pdr, cluster );

#if (PREERASE_ON_DELETE)
                devio_erase( pdr->driveno, blockno, pdr->secpalloc );
#endif
		cluster = nextcluster;
                nextcluster = pc_clnext( pdr, nextcluster );
	}
}



#if (RTFS_SUBDIRS)
/*****************************************************************************
* Name: PC_CL_TRUNCATE - Truncate a cluster chain.
*                                                                      
* Description:
*       Trace the cluster chain starting at cluster until l_cluster is
*       reached. Then terminate the chain and free from l_cluster on.
*       The FAT is not flushed.
* 
*       If cluster == l_cluster does nothing. This condition should be
*       handled higher up by updating dirents appropriately and then
*       calling pc_freechain.
*                                                                      
* Note: The caller locks the fat before calling this routine.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 cluster
*       UINT16 l_cluster
*
* Returns:
*       The last cluster in the chain after truncation.
******************************************************************************/
UINT32 pc_cl_truncate(DDRIVE *pdr, UINT32 cluster, UINT32 l_cluster) /*__fn__*/
{
        UINT32 nextcluster;

        nextcluster = pc_clnext( pdr, cluster );
        while ( nextcluster )
	{
                if ( nextcluster == l_cluster )
		{
                        if ( !pc_pfaxx(pdr, cluster, 0x0FFFFFFFL) ) /* Terminate the chain */
				break;                  /* Error break to ret 0 */

                        pc_freechain( pdr, l_cluster );
                        return ( cluster );
                }               
		else
		{
			cluster = nextcluster;
                        nextcluster = pc_clnext( pdr , nextcluster );
		}
	}

        return (0L);
}
#endif


/******************************************************************************
* Name: PC_PFAXX - Write a value to the FAT at clno.
*
* Description:
*       Given a DDRIVE, cluster number and value, Write the value in
*       the FAT at clusterno. Handle 16 and 12 bit fats correctly.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 clno
*       UINT16 value
*
* Returns:
*       No if an io error occurred during fat swapping, else YES.
*
******************************************************************************/
SDBOOL pc_pfaxx(DDRIVE *pdr, UINT32 clusterno, UINT32 value) /*__fn__*/
{
        UINT16  *pFATbuf;
        UINT32  index;
        UINT32  offset;
        UINT32  dtemp;
        UINT32  result;
        UINT16  tmpOffset;


        index = clusterno;

        if ( pdr->fasize == DRV_FAT12 ) /* 3 nibble ? */
        {
                offset = index + (clusterno >> 1);
                index = (offset >> 1);
        }

        /* Get FAT buffer information */
        pFATbuf = (UINT16 *)pc_pfswap( pdr, index );
        if ( pFATbuf == SDNULL)
                return (NO);
        

        offset = pdr->fat_swap_structure.current_access;

        /* If we should mark it dirty, do so in the bit map. */
        pdr->fat_swap_structure.pdirty[offset] = YES;
        pdr->fat_is_dirty = YES;

        if ( pdr->fasize == DRV_FAT12 ) /* FAT12, 3 nibble ? */
        {
                /* Get cluster information */
                tmpOffset = pFATbuf[(index & 0xFF)];

                /* Make sure the data is within range */
                dtemp = (value & 0x0FFF);

                /* Data conversion between LITTLE and BIG endian if required */
#if (USE_HW_OPTION)
                result = (UINT32)to_WORD( (UCHAR *)&tmpOffset );
#else
                result = (UINT32)tmpOffset;
#endif


                offset = (clusterno & 03);

                if ( offset == 0 )
                {
                        result &= 0xF000;
                }
                else if ( offset == 3 )
                {
                        result &= 0x000F;
                        dtemp <<= 4;
                }
                else
                {
                        dtemp <<= 8;
                        if ( offset == 1 )
                        {
                                result &= 0x0FFF;
                                dtemp <<= 4;
                        }
                        else
                        {
                                result &= 0x00FF;
                                dtemp &= 0xFF00;
                        }

                        result |= dtemp;

#if (USE_HW_OPTION)
                        fr_WORD( (UCHAR *)&tmpOffset, (UINT16)result );
#else
                        tmpOffset = (UINT16)result;
#endif

                        /* Write information to the FAT */
                        pFATbuf[index&0x0FF] = tmpOffset;

                        index++;
                        pFATbuf = (UINT16 *)pc_pfswap( pdr, index );
                        if ( pFATbuf == SDNULL )
                                return (NO);

                        dtemp = pdr->fat_swap_structure.current_access;
                        index &= 0x0FF;

                        /* If we should mark it dirty, do so in the bit map. */
                        pdr->fat_swap_structure.pdirty[dtemp] = YES;
                        pdr->fat_is_dirty = YES;

                        /* Get cluster information */
                        tmpOffset = pFATbuf[(index)];

#if (USE_HW_OPTION)
                        result = (UINT32)to_WORD( (UCHAR *)&tmpOffset );
#else
                        result = tmpOffset;
#endif

                        /* Make sure the data is within range */
                        dtemp = (value & 0x0FFF);

                        dtemp >>= 4;
                        if ( offset == 0x01 )
                        {
                                result &= 0xFF00;
                        }
                        else
                        {
                                dtemp >>= 4;
                                result &= 0xFFF0;
                        }

                }

                result |= dtemp;
        }
        else if ( pdr->fasize == DRV_FAT16 ) /* FAT16 ? */
        {
                /* Make sure the data is within range */
                result = (value & 0xFFFFL);
        }
        else     /* FAT32 ? */
        {
                result = value & 0x0FFFFFFFL;
        }


        if ( pdr->fasize == DRV_FAT12 || pdr->fasize == DRV_FAT16 )
        {
                index &= 0x0FF;
                /* BIG to LITTLE endian conversion if required */
#if (USE_HW_OPTION)
                fr_WORD( (UCHAR *)&tmpOffset, (UINT16)result );
#else
                tmpOffset = (UINT16)result;
#endif

                /* Write data to the FAT buffer */
                pFATbuf[index] = tmpOffset;
        }
        else    /* FAT32 */
        {
                /* BIG to LITTLE endian conversion if required */
#if (USE_HW_OPTION)
                fr_DWORD( (UCHAR *)&dtemp, result );
#else
                dtemp = result;
#endif
                ((UINT32 *)pFATbuf)[index & 0x7F] = dtemp;
        }

        return (YES);
}

#endif  /* (RTFS_WRITE) */


/*****************************************************************************
* Name: PC_FATRead - Swap a block of FAT into the cache
*
* Description
*       Swap in FAT_BUFFER_SIZE or pdr->secpfat, whichever the smaller, of
*       new blocks of FAT into the cache.
*
* Entries:
*       DDRIVE *pdr             Drive structure
*       UINT16 index            Index to the FAT
*       UINT16 count            Number of FAT sectors to be read
*       UINT16 starting         Index to the FAT buffer
*
* Returns
*       Pointer to FAT data *pdata
*
******************************************************************************/
UCHAR *pc_FATRead( DDRIVE *pdr, UINT32 index, UINT32 starting_buffer_index, UINT16 count) /*__fn__*/
{
	FATSWAP *pfs;
        UCHAR   *pdata;
	BLOCKT  newblock;
        UINT16  block_offset_in_fat, blk_counts, ij;

        pfs = (FATSWAP *)&pdr->fat_swap_structure;

        blk_counts = count;
        if ( blk_counts > (UINT16)pdr->secpfat )
                blk_counts = pdr->secpfat;

        /* Save the buffer index */
        pfs->current_access = starting_buffer_index;

        if ( (pdr->fasize == DRV_FAT12) || (pdr->fasize == DRV_FAT16) )
        {
                /*
                Convert the index (in words) to block values
                divide by 256 since there are 256 FAT entries per block
                */
                block_offset_in_fat = (UINT16)(index >> 8);
        }
        else
        {
                /*
                Convert the index (in words) to block values
                divide by 128 since there are 128 FAT entries per block
                */
                block_offset_in_fat = (UINT16)(index >> 7);
        }                

	/*
        Whether we are reusing a block or still coming up to speed.
        if the block is not currently in our pool we must mark the
        data block used and read it in.
	*/
        pdata = (UCHAR *)pfs->data_array;
        newblock = (BLOCKT)(block_offset_in_fat + pdr->fatblock);
        pdata += (((ULONG)starting_buffer_index) << 9);

        /* Not mapped, we have to read it in.
           If we haven't reached steady state: use core
           until we get there.
        */

        /* READ the FATs. into the buffers */
        if ( !devio_read(pdr->driveno, newblock, (UCHAR *)pdata, blk_counts) )
	{
		/* Reading the FAT failed */
		return(SDNULL);
	}


        /* Fill the data map with FAT block addresses. */
        ij = (UINT16)starting_buffer_index;
        while ((starting_buffer_index + (UINT32)blk_counts) > (UINT32)ij )
        {
                /* Fill the data map */
                pfs->data_map[ij] = block_offset_in_fat;

                /* Mark it as not dirty */
                pfs->pdirty[ij] = 0;
                block_offset_in_fat++;
                ij++;
        }

        /* Make sure the FAT is not dirty. */
        pdr->fat_is_dirty = NO;

        return (pdata);
}



/*****************************************************************************
* Name: PC_PFSWAP - Swap a block of FAT into the cache
*
* Description
*       Provide a caching mechanism for the file allocation table. We have a 
*       pdirty[offset] array of size FAT_BUFFER_SIZE indicating the offset
*       into our data cache for the current blocks of the FAT.
*       If the pdirty[offset] is NOT zero then the corresponding block must
*       be flushed.
*
*       Flush buffers and then swap in new blocks containing index.
*
* Entries:
*       DDRIVE *pdr
*       UINT16 index
*
* Returns
*       Pointer to FAT data *pdata
*
******************************************************************************/
UCHAR *pc_pfswap(DDRIVE *pdr, UINT32 index) /*__fn__*/
{
        FATSWAP *pfs;
        UCHAR   *pdata;
        ULONG   ltemp;
        UINT32  index_from, index_to;


        pfs = (FATSWAP *)&pdr->fat_swap_structure;

        if ( (pdr->fasize == DRV_FAT12) || (pdr->fasize == DRV_FAT16) )
        {
                /*
                Convert the index (in words) to block values
                divide by 256 since there are 256 FAT entries per block
                */
                index_from = (index >> 8);
        }
        else
        {
                /*
                Convert the index (in dwords) to block values
                divide by 128 since there are 128 FAT entries per block
                */
                index_from = (index >> 7);
        }

        if ( index_from >= (UINT16)pdr->secpfat ) /* Check range */
                return (SDNULL);

        if ( (index_from == pfs->data_map[pfs->current_access]) )
        {
                /* Already in our cache. Set up a pointer to the FAT buffer. */
                ltemp = ((ULONG)pfs->current_access << 9);
                pdata = (UCHAR *)pfs->data_array;
                pdata += ltemp;
                return (pdata);
        }


        /* Check if the block is already mapped. */
        for (index_to = 0; index_to < FAT_BUFFER_SIZE; index_to++)
        {
                /* Since the cache contains different FAT location information,
                   we have to search the data_map for the right FAT location.
                */
                if ( index_from == pfs->data_map[index_to])
                {
                        /* Already in our cache. Set up a pointer to it */
                        ltemp = ((ULONG)index_to << 9);
                        pdata = (UCHAR *)pfs->data_array;
                        pdata += ltemp;
                        pfs->current_access = index_to;

                        return (pdata);
                }
        }


#if (RTFS_WRITE)
        /* Flush the buffer by only flushing the blocks we are swapping. */
        if ( !pc_pfflush(pdr) )
                return (SDNULL);
#endif

        /* Initialize information */
        pdata = (UCHAR *)pfs->data_array;
        index_from = 0;
        index_to = FAT_BUFFER_SIZE;

        /* If the FAT_BUFFER_SIZE is more than 1,  a round-robin scheme
           is performed to swap the FAT.
        */
        if ( pfs->n_blocks_total > 1 )
        {
                /* Look for current FAT location in the buffer.
                   The buffer is filled with FAT information either
                   from the top to the current FAT location or the
                   (current location + 1) to the end of the FAT buffer.
                */
                if ( pfs->current_access == 0 )
                        index_from = 1;
                else
                {
                /* (index_from - index_to) is used to fill FAT information
                   from top to current location or to (current location - 1).
                */
                        index_to = pfs->current_access;
                }

                /* Adjust the pointer to the FAT buffer. */
                ltemp = ((ULONG)index_from << 9);
                pdata += ltemp;
        }


        /* Not mapped, we have to read it in. */
        /*
        Select the block to swap out. Use a simple round-robin
        selection algorithm: pfs->current_access is the current 
        target for swapping.
        */
        if ( pc_FATRead( pdr, index, index_from, (UINT16)(index_to - index_from)) == SDNULL )
                return SDNULL;

        return (pdata);
}


#if (RTFS_WRITE)
/*****************************************************************************
* Name: pc_pfflush
*
* Description:
*       Consult the dirty fat block list and write any. write all copies
*       of the fat.
*
* Note: The caller locks the fat before calling this routine
*
* Entries:
*       DDRIVE *pdr
*
* Returns:
*       YES     if Successful
*       NO      if Failed
*
******************************************************************************/
SDBOOL pc_pfflush ( DDRIVE *pdr ) /*__fn__*/
{
        FATSWAP *pfs;
        UCHAR   *pdata;
        BLOCKT   blockno;
        UINT16   j;

        /* Don't spend any time here if the FAT is clean */
        if ( pdr->fat_is_dirty == NO )
                return (YES);


        pfs = (FATSWAP *)&pdr->fat_swap_structure;


        /* Search for FATs that need to be flushed */
        for (j = 0; j < FAT_BUFFER_SIZE; j++)
	{
                /* Check for FATs needed to be update */
                if ( pfs->pdirty[j] )
                {
                        /* Get its address in our data array */
                        blockno = ((ULONG)pfs->data_map[j]);
                        pdata = pfs->data_array;
                        pdata += (((ULONG)j) << 9);

                        /* Convert to logical disk block */
                        blockno += (BLOCKT)(pdr->fatblock);

                        /* Write to first FAT */
                        if ( !devio_write(pdr->driveno,
                                        blockno,
                                        pdata,
                                        1) )
                        {
                                REPORT_ERROR( PCERR_FAT_FLUSH );
                                return (NO);
                        }

                        if ( pdr->numfats > 1 ) /* Check for second FAT */
                        {
                                /* Write to Second FAT */
                                blockno += pdr->secpfat;
                                if ( !devio_write(pdr->driveno,
                                        blockno,
                                        pdata,
                                        1) )
                                {
                                        REPORT_ERROR( PCERR_FAT_FLUSH );
                                        return (NO);
                                }
                        }

                        pfs->pdirty[j] = 0;
		}
	}


        /* All done,  now clear the flag */
        pdr->fat_is_dirty = NO;
	return (YES);
}
#endif /* (RTFS_WRITE) - for fflush */


/*****************************************************************************
* Name: PC_GBLK0 -  Read block 0 and load values into a a structure
*
* Description:
*       Given a valid drive number, read block zero and convert
*       its contents from intel to native byte order.
*
* Entries:
*       INT16 driveno
*       DDRIVE *pdr
*
* Returns
*       Returns YES if all went well.
*
******************************************************************************/
SDBOOL pc_gblk0(INT16 driveno, DDRIVE *pdr) /*__fn__*/
{
        BLKBUFF *buf;
        UCHAR   *b;
#if !(LITTLE_ENDIAN) && !(USE_HW_OPTION)
        UINT16  *pbuf;
#endif
        ULONG   ltmp;
        UINT16  signature;
#if (CHAR_16BIT)
        UTEXT   bFatType[6];
#endif


	/* Grab a buffer to play with */
	buf = pc_scratch_blk();
        if ( !buf )
                return (NO);

        b = (UCHAR *)buf->data;          /* Now we don't have to use the stack */

	/* get 1 block starting at 0 from driveno */
	/* READ */
        ltmp = 0L;
        if ( !devio_read( driveno, ltmp, b, 1 ) )
	{
                REPORT_ERROR(PCERR_INITREAD);
                pdr->dev_flag = NO;
                return (NO);
	}


#if (CHAR_16BIT)

#if (LITLLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
 #else
        pbuf = (UINT16 *)buf;
        for (ltmp=0L; ltmp < 256L; ltmp++)
                pbuf[ltmp] = to_WORD((UCHAR *)&pbuf[ltmp]);
 #endif        
#endif

	/* Now load the structure from the buffer */
        b_unpack( (UTINY *)bFatType, (UINT16 *)(b), 5, 0x36 );
        if (bFatType[0] != 0x46)        /* Letter F */
                b_unpack( (UTINY *)bFatType, (UINT16 *)(b), 5, 0x52 );
        bFatType[5] = 0;

        /* Check for FAT12 */
        if (pc_patcmp ( (TEXT *)(bFatType), (TEXT *)string_fat_12, 5))
                pdr->fasize = DRV_FAT12;
        /* Check for FAT16 */
        else if (pc_patcmp ( (TEXT *)(bFatType), (TEXT *)string_fat_16, 5))
                pdr->fasize = DRV_FAT16;
        /* Check for FAT16 */
        else if (pc_patcmp ( (TEXT *)(bFatType), (TEXT *)string_fat_32, 5))
                pdr->fasize = DRV_FAT32;

	/* Now load the structure from the buffer */
        w_unpack( (UINT16 *)&pdr->secpalloc, (UINT16 *)b, 0x0D );
        pdr->secpalloc = to_WORD((UCHAR *)&pdr->secpalloc) & 0x00FF;

        w_unpack( (UINT16 *)&pdr->numfats, (UINT16 *)b, 0x10 );
        pdr->numfats = to_WORD((UCHAR *)&pdr->numfats) & 0x00FF;

        w_unpack( (UINT16 *)&pdr->mediadesc, (UINT16 *)b, 0x15 );
        pdr->mediadesc = to_WORD((UCHAR *)&pdr->mediadesc) & 0x00FF;

        w_unpack( (UINT16 *)&signature, (UINT16 *)b, 0x1FE );
        signature = to_WORD((UCHAR *)&signature );

        w_unpack( (UINT16 *)&pdr->bytspsector, (UINT16 *)b, 0x0B );
        pdr->bytspsector = to_WORD((UCHAR *)&pdr->bytspsector);

        w_unpack( (UINT16 *)&pdr->fatblock, (UINT16 *)b, 0x0E );
        pdr->fatblock = to_WORD((UCHAR *)&pdr->fatblock);

        w_unpack( (UINT16 *)&pdr->numroot, (UINT16 *)b, 0x11 );
        pdr->numroot = to_WORD((UCHAR *)&pdr->numroot);

        w_unpack( (UINT16 *)&pdr->numsecs, (UINT16 *)b, 0x13 );
        pdr->numsecs = to_WORD((UCHAR *)&pdr->numsecs);

        pdr->secpfat   = to_WORD(b+0x16);      /*X*/
        pdr->secptrk   = to_WORD(b+0x18);      /*X*/
        pdr->numhead   = to_WORD(b+0x1A);      /*X*/
        pdr->numhide   = to_DWORD(b+0x1C);     /*X*/
        /* Check if running on a DOS (4.0) huge partition */
        if (!pdr->numsecs)
                pdr->numsecs  = to_DWORD(b+0x20);     /*X*/ /* # secs if > 32M (4.0) */

        l_unpack( (ULONG *)&pdr->volume_serialno, (UINT16 *)b, 0x27 );
        pdr->volume_serialno = to_DWORD((UCHAR *)&pdr->volume_serialno);
	for (ltmp = 0; ltmp < 11; ++ltmp) {
	    pdr->volume_label[ltmp] = b[ltmp + 0x2b];
	}
	pdr->volume_label[ltmp] = 0;

#else   /* (not CHAR_16BIT) */

        /* Check for FAT12 */
        if (pc_patcmp ( (TEXT *)(b+0x36), (TEXT *)string_fat_12, 5))
                pdr->fasize = DRV_FAT12;
        /* Check for FAT16 */
        else if (pc_patcmp ( (TEXT *)(b+0x36), (TEXT *)string_fat_16, 5))
                pdr->fasize = DRV_FAT16;
        /* Check for FAT16 */
        else if (pc_patcmp ( (TEXT *)(b+0x52), (TEXT *)string_fat_32, 5))
                pdr->fasize = DRV_FAT32;

	/* Now load the structure from the buffer */
        signature = to_WORD( b+0x1FE );
        pdr->secpalloc = b[0x0D];
        pdr->numfats = b[0x10];
        pdr->mediadesc = b[0x15];
        pdr->bytspsector = to_WORD(b+0x0B); /*X*/
        pdr->fatblock = to_WORD(b+0x0E);    /*X*/
        pdr->numroot   = to_WORD(b+0x11);   /*X*/
        pdr->numsecs   = to_WORD(b+0x13);   /*X*/
        pdr->secpfat   = to_WORD(b+0x16);   /*X*/
        pdr->secptrk   = to_WORD(b+0x18);   /*X*/
        pdr->numhead   = to_WORD(b+0x1A);   /*X*/
        pdr->numhide   = to_DWORD(b+0x1C);  /*X*/
        if (!pdr->numsecs)
                pdr->numsecs  = to_DWORD(b+0x20); /*X*/ /* # secs if > 32M (4.0) */
        pdr->volume_serialno = to_DWORD(b+0x27);  /*X*/
	for (ltmp = 0; ltmp < 11; ++ltmp) {
	    pdr->volume_label[ltmp] = b[ltmp + 0x2b];
	}
	pdr->volume_label[ltmp] = 0;

#endif  /* (CHAR_16BIT) */

        if (pdr->fasize == 8)
        {
                pdr->secpfat = to_WORD(b+0x24);
                pdr->numroot = to_WORD(b+0x2C); /* Start cluster of Root Directory Entry */

                /* get 1 block starting at 0 from driveno */
                /* READ */
                ltmp = 0L;
                if ( !devio_read( driveno, ltmp+1, b, 1 ) )
                {
                        REPORT_ERROR(PCERR_INITREAD);
                        pdr->dev_flag = NO;
                        return (NO);
                }

#if (LITTLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
 #else
        pbuf = (UINT16 *)buf;
        for (ltmp=0L; ltmp < 256L; ltmp++)
                pbuf[ltmp] = to_WORD((UCHAR *)&pbuf[ltmp]);
 #endif        
#endif

                if (to_DWORD(b+0x01E4) == FSINFOSIG)
                {
                        pdr->known_free_clusters = to_DWORD(b+0x01E8);
                        pdr->free_contig_pointer = to_DWORD(b+0x01EC);
                }
        }

        pc_free_buf( buf, YES, YES );

        if (signature !=  LEGAL_SIGNATURE)
        {
                REPORT_ERROR(PCERR_INITMEDI);
                pdr->dev_flag = NO;
                errno = CRERR_BAD_FORMAT;
                return (NO);
        }

	return(YES);
}

#if (RTFS_WRITE)
#if (RTFS_SUBDIRS)
/******************************************************************************
* Name: PC_CLZERO -  Fill a disk cluster with zeroes
*                   
* Description
*       Write zeroes into the cluster at clusterno on the drive pointed to
*       by pdrive. Used to zero out directory and data file clusters to
*       eliminate any residual data.
*
* Entries:
*       DDRIVE  *pdrive
*       UINT16  cluster
*
* Returns:
*       Returns NO on a write erro.
*                   
******************************************************************************/
SDBOOL pc_clzero(DDRIVE *pdrive, UINT32 cluster) /*__fn__*/
{
        BLKBUFF *pbuff;
        BLOCKT  currbl;
        UINT16  i;

        currbl = pc_cl2sector( pdrive, cluster );
        if ( !currbl )
		return (NO);

	/* Claim the drive's buffer pool while we initialize the blocks */
	PC_BP_ENTER(pdrive->driveno)

        /* Init and write a block for each block in cl.
           Note: init clears the core
        */
        for ( i = 0; i < pdrive->secpalloc; i++, currbl++ )
	{
                pbuff = pc_init_blk( pdrive, currbl );
                if ( !pbuff )
		{
			PC_BP_EXIT(pdrive->driveno)
			return (NO);
		}

		if ( !pc_write_blk ( pbuff ) )
		{
			PC_BP_EXIT(pdrive->driveno)
                        pc_free_buf( pbuff, YES, YES );
			return (NO);
		}

                pc_free_buf( pbuff, NO, YES );
	}

	PC_BP_EXIT(pdrive->driveno)

	return (YES);
}
#endif  /* (RTFS_WRITE) */
#endif  /* (RTFS_SUBDIRS) */



/******************************************************************************
* Name: PC_DRNO2DR -  Convert a drive number to a pointer to DDRIVE
*
* Description:
*       Given a drive number look up the DDRIVE structure associated with it.
*
* Entries:
*       INT16 driveno
*
* Returns:
*    Returns NULL if driveno is not an open drive.
*
******************************************************************************/
DDRIVE  *pc_drno2dr(INT16 driveno) /*__fn__*/
{
	DDRIVE  *pdr;
	DDRIVE  *pretval;

	PC_ENTER_CRITICAL()
        pretval = SDNULL;

	/* Check drive number */
        if ( pc_validate_driveno(driveno) )
	{
                pdr = &mem_drives_structures[driveno];
                if ( pdr->dev_flag )
		{
			pretval = pdr;
		}
	}

	PC_EXIT_CRITICAL()

        return (pretval);
}


SDBOOL pc_updatedskfree(INT16 driveno) /*__fn__*/
{
        DDRIVE  *pdr;
        BLKBUFF *buf;
        UCHAR   *b;

	pdr = pc_drno2dr(driveno);
        if ( !pdr )
	{
                return (NO);
	}

        buf = pc_scratch_blk();
        if ( !buf )
        {
                REPORT_ERROR(PCERR_BLOCKCLAIM);
                pdr->enable_mapping = NO;
                return (NO);
        }

        b = (UCHAR *)buf->data;
                        
        /* get 1 block starting at 1 from driveno */
                /* READ */
        if ( !devio_read( driveno, 1L, b, 1 ) )
        {
                REPORT_ERROR(PCERR_INITREAD);
                pdr->enable_mapping = NO;
                pc_free_buf (buf, YES, YES);
                return (NO);
        }

        if (to_DWORD(b+0x01E4) == FSINFOSIG)
        {
                if ( (to_DWORD(b+0x01E8) != (ULONG)pdr->known_free_clusters) ||
                     (to_DWORD(b+0x01EC) != (ULONG)FSFREEPTR)  )
                {
                        fr_DWORD(&b[0x01E8], (ULONG)pdr->known_free_clusters);
                        fr_DWORD(&b[0x01EC], (ULONG)FSFREEPTR);

                        if ( !devio_write( driveno, 1L, b, 1 ) )
                        {
                                REPORT_ERROR(PCERR_INITWRITE);
                                pdr->enable_mapping = NO;
                                pc_free_buf (buf, YES, YES);
                                return (NO);
                        }
                }                                        
        }

        pc_free_buf (buf, YES, YES);
        return (YES);
}


/*****************************************************************************
* Name: PC_DSKFREE -  Deallocate all core associated with a disk structure
*
* Description:
*       Given a valid drive number. If the drive open count goes to zero,
*       free the file allocation table and the block zero information
*       associated with the drive. If unconditional is true, ignore the
*       open count and release the drive. 
*       If open count reaches zero or unconditional, all future accesses to
*       driveno will fail until re-opened.
* Note: Free up all core associated with the drive  called by close. A drive
*       restart would consist of pc_dskfree(driveno, YES), pc_dskopen().
*
* Entries:
*       INT16 driveno
*       SDBOOL  unconditional
*
* Returns
*    Returns NO if driveno is not an open drive.
*
******************************************************************************/
SDBOOL pc_dskfree(INT16 driveno, SDBOOL unconditional) /*__fn__*/
{
        DDRIVE  *pdr;

	pdr = pc_drno2dr(driveno);
        if ( !pdr )
	{
                return (NO);
	}

	/* If unconditional close. Fake it out by setting open count to 1 */
        if ( unconditional )
                pdr->dev_flag = 1;

        if ( (pdr->dev_flag & 0x7FFF) == 1 )
	{
                if (pdr->fasize == DRV_FAT32)
                        pc_updatedskfree(driveno);

		/* Free the current working directory for this drive for all users */
		pc_free_all_users(driveno);

		/* Free all files, finodes & blocks associated with the drive */
                pc_free_all_files( pdr );
                pc_free_all_i( pdr );
                pc_free_all_blk( pdr );
	}

        pdr->dev_flag -= 1;
        pdr->dev_flag &= 0x7FFF;
        pdr->enable_mapping = NO;

	return (YES);
}


/*****************************************************************************
* Name: PC_IFREE - Count the number of free bytes remaining on a disk (internal)
*
* Description:
*       Given a drive number count the number of free bytes on the drive.
*       Called by pc_free.
*
* Entries:
*       INT16 driveno
*
* Returns:
*       The number of free bytes or zero if the drive is full or 
*       it it not open or out of range.
*
* NOTE: To speed up this operation we maintain a variable in the
*       drive structure.  known_free_clusters. If this variable is
*       non zero a multiple of this value will be returned. To force
*       the free count to be recalculated just set field to zero
*       before calling this function.
*
******************************************************************************/
ULONG pc_ifree(INT16 driveno) /*__fn__*/
{
	DDRIVE  *pdr;
        ULONG   ltemp;
        UINT32  nxt;
        UINT32  freecount;

        freecount = 0L;

	pdr = pc_drno2dr(driveno);

        if ( !pdr )
	{
                return (0L);
	}

        if ( pdr->known_free_clusters )
		freecount = pdr->known_free_clusters;
	else
	{
                for (ltemp = 2L; ltemp <= (ULONG)pdr->maxfindex; ltemp++)
		{
                        if ( !pc_faxx(pdr, ltemp, &nxt) )
                                return (0L);

                        if ( nxt == 0L )
				freecount++;
		}
		pdr->known_free_clusters = freecount;
	}

        ltemp = freecount;
        ltemp *=  (ULONG)pdr->bytespcluster;

        if (pdr->fasize == DRV_FAT32)
        {
                pdr->known_free_clusters = freecount;
                pc_updatedskfree(driveno);
        }

	return (ltemp);
}


#if (RTFS_SUBDIRS)
/*****************************************************************************
* Name: PC_SEC2CLUSTER - Convert a block number to its cluster representation.
*
* Description
*       Convert blockno to its cluster representation if it is in
*       cluster space.
*
* Entries:
*       DDRIVE *pdrive
*       BLOCKT blockno
*
* Returns
*       Returns 0 if the block is not in cluster space, else returns the
*       cluster number associated with block.
*
******************************************************************************/
UINT32 pc_sec2cluster(DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLOCKT  ltemp;
        BLOCKT  answer;

        if ( (blockno >= pdrive->numsecs) ||
                (pdrive->firstclblock > blockno) )
                return (0L);
	else
	{
		/*  (2 + (blockno - pdrive->firstclblock)/pdrive->secpalloc) */
		ltemp = blockno - pdrive->firstclblock;
		answer = ltemp >> pdrive->log2_secpalloc;
		answer += 2;

                return ((UINT32)answer);
	}
}


UINT32 pc_sec2ClusterDir(DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLOCKT  ltemp;
        BLOCKT  clusterno;               

        if ( blockno >= pdrive->numsecs )
                return (0L);

        if (pdrive->fasize == DRV_FAT12 || pdrive->fasize == DRV_FAT16)
        {
                if (pdrive->firstclblock > blockno)
                        return (0L);
        }

        /* Check Root Directory Entries for FAT32 */
        ltemp = blockno - (BLOCKT)pdrive->firstclblock;
        clusterno = ltemp >> pdrive->log2_secpalloc;
        clusterno += 2;

        if (pdrive->fasize == DRV_FAT32)
        {
                if (clusterno == pdrive->currDirCluster)
                        return (0L);

                if (clusterno > pdrive->currDirCluster)
                {
                        while (pdrive->nextDirCluster)
                        {
                                pdrive->prevDirCluster = pdrive->currDirCluster;
                                pdrive->currDirCluster = pdrive->nextDirCluster;
                                pdrive->nextDirCluster = pc_clnext(pdrive, pdrive->currDirCluster);
                                if (clusterno == pdrive->currDirCluster)
                                        return (0L);
                        }
                        goto DATA_AREA_END;
                }

                if (clusterno < pdrive->currDirCluster)
                {
                        while (pdrive->prevDirCluster)
                        {
                                pdrive->nextDirCluster = pdrive->currDirCluster;
                                pdrive->currDirCluster = pdrive->prevDirCluster;
                                pdrive->prevDirCluster = pc_clprev(pdrive, pdrive->currDirCluster);
                                if (clusterno == pdrive->currDirCluster)
                                        return (0L);
                        }
                        goto DATA_AREA_END;
                }
        }

DATA_AREA_END:

        return ((UINT32)clusterno);
}



/******************************************************************************
* Name: PC_SEC2INDEX - Calculate the offset into a cluster for a block.
*
* Description:
*       Given a block number offset from the beginning of the drive,
*       calculate which block number within a cluster it will be. If
*       the block number coincides with a cluster boundary, the return
*       value will be zero. If it coincides with a cluster (boundary + 1)
*       block, the value will be 1, etc.
*
* Note: Convert sector to index into a cluster . No error detection.
*
* Entries:
*
* Returns:
*       0,1,2 upto blockspcluster -1.
*
******************************************************************************/
UINT16 pc_sec2index(DDRIVE *pdrive, BLOCKT blockno) /*__fn__*/
{
        BLOCKT  answer;

	answer = blockno - pdrive->firstclblock;
	answer = answer % pdrive->secpalloc;

	return ( (UINT16) answer);
}

#endif  /* (RTFS_SUBDIRS) */


/******************************************************************************
* Name: PC_CL2SECTOR - Convert a cluster number to block number representation.
*
* Description
*    Convert cluster number to a block number.
*
* Entries:
*       DDRIVE *pdrive
*       UINT16 cluster
*
* Returns
*       Returns 0 if the cluster is out of range. else returns the
*       block number of the beginning of the cluster.
*
******************************************************************************/
BLOCKT pc_cl2sector(DDRIVE *pdrive, UINT32 cluster) /*__fn__*/
{
        BLOCKT  blockno;
        UINT32  cltmp;

        if ( cluster < 2 )
		return (BLOCKEQ0);
	else 
	{
                cltmp = cluster - 2L;

                cltmp = cltmp << pdrive->log2_secpalloc;
                blockno = pdrive->firstclblock + cltmp;
	}

        if ( blockno >= pdrive->numsecs )
		return (BLOCKEQ0);

        return (blockno);
}



/******************************************************************************
* Name: GET_PARTITION_INFO -  Partition table interpretor
*
* Description:
*       Given a physical drive number and the addresses of two tables, 
*       partition start and partition end, this routine interprets the
*       partion tables on the physical drive and fills in the start and end
*       blocks of each partition. Extended partitions are supported. If there
*       are more than max partitions, only max will be returned.
*
* Note: The physical drive must be in a raw state so no partition mapping
*       takes place.
*
* Entries:
*       INT16 driveno
*
* Returns
*       The number of partitions found on the drive.
*
******************************************************************************/
INT16 get_partition_info(INT16 driveno) /*__fn__*/
{
	DDRIVE  *pdr;
        PTABLE  *ppart;
        BLKBUFF *bbuf;
        UCHAR   *buf;
#if !(LITTLE_ENDIAN) && !(USE_HW_OPTION)
        UINT16  *pbuf;
#endif
        ULONG   ltemp;
        INT16   nparts;
        UINT16  i;


	nparts = 0;

        /* Only support one partition per drive */
        pdr = &mem_drives_structures[driveno];
        pdr->enable_mapping = NO;


        /* Grab some working space. This is temporary buffer */
        bbuf = pc_scratch_blk();
        if ( !bbuf )
                return (0);

        buf = (UCHAR *)bbuf->data;
	/* Read sector 0, where the Master Boot Record is */
        if ( !devio_read(driveno, 0L, (UCHAR *)buf, 1) )
	{
                /* Unable to get the Master Boot Record */
                pc_free_buf(bbuf, YES, YES);
                return (nparts);
	}


	//#if (LITLLE_ENDIAN)
#if (LITTLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
 #else
        pbuf = (UINT16 *)buf;
        for (ltemp=0L; ltemp < 256L; ltemp++)
                pbuf[ltemp] = to_WORD((UCHAR *)&pbuf[ltemp]);
 #endif        
#endif


#if (CHAR_16BIT)
        ppart = (PTABLE *)&fat_drives[(FAT_BUFFER_SIZE*driveno)];

        /* Copy the table to a word aligned buffer */
        char_unpack_ptable(ppart, (UINT16 *)buf, 0x1BE);
#else   

        /* The partition of the MBR info starts at buf[1BE] */
        ppart = (PTABLE *)(buf + 0x1BE);

#endif  /* (CHAR_16BIT) */

#if LITTLE_ENDIAN
#else
        ppart->signature = to_WORD((UCHAR *)&ppart->signature);
#endif

        if ( ppart->signature != LEGAL_SIGNATURE)
	{
		/* signature does not match, ABORT .... */
                pc_free_buf(bbuf, YES, YES);
                errno = CRERR_BAD_FORMAT;
                return (nparts);
	}

	/* Read through the partition table. Find the primary DOS partition */
	for (i = 0; i < 4; i++)
	{
	    /* TODO stepping through this in the debugger showed that r_sec
	     * wasn't getting loaded correctly into the register, the comparison
	     * was failing, and thus disks with partitions would always get
	     * fucked up */
	    //if (ppart->ents[i].r_sec < 0x100) {
                ppart->ents[i].p_typ &= 0xFF; 
		if ( (ppart->ents[i].p_typ == 0x01) ||
			(ppart->ents[i].p_typ == 0x04) ||
                        (ppart->ents[i].p_typ == 0x06) ||
                        (ppart->ents[i].p_typ == 0x0B) || /* Win98 */
                        (ppart->ents[i].p_typ == 0x0C) || /* Win95 */
                        (ppart->ents[i].p_typ == 0x0D) ||
                        (ppart->ents[i].p_typ == 0x0E) )   

		{
			/* Get the relative start and size */
                        pdr->volume_start = to_DWORD ((UCHAR *)&ppart->ents[i].r_sec);
                        ltemp = to_DWORD ((UCHAR *)&ppart->ents[i].p_size);
                        pdr->volume_end = pdr->volume_start + ltemp - 1L;
                        pdr->enable_mapping = YES;
			nparts = 1;
			break;
		}
		else if (ppart->ents[i].p_typ == 0x07) { /* NTFS */
		    errno = 0;	/* TODO this is somewhat of a hack
				 * we say no error so that CRERR_BAD_FORMAT is returned in
				 * platform_convert_critical_error, and the bad format gets
				 * propagated appropriately */
		    break;
		}
		//}
        }
	if (i == 4 && nparts == 0) {
	    /* disk formatted without partition table */
	    pdr->enable_mapping = NO;
	    /* these parameters are only used if mapping is enabled */
	    pdr->volume_start = 0;
	    pdr->volume_end = 0;
	    /* must return nparts > 0 for this function to succeed */
	    nparts = 1;
	}

        pdr->os_type = (UTINY)DOS_TYPE;
        if (ppart->ents[i].p_typ == 0x0B)
                pdr->os_type = (UTINY)WIN98_TYPE;
        else if (ppart->ents[i].p_typ == 0x0C)
                pdr->os_type = (UTINY)WIN95_TYPE;

        pc_free_buf(bbuf, YES, YES);
        return (nparts);
}

#endif  /* (USE_FILE_SYSTEM) */


