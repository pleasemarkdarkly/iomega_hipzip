//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs    ericg@iobjects.com                                    ..    
//.. Modification date: 8/16/2000                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.                                      ..
//..     All rights reserved. This code may not be redistributed in source or linkable  ..
//..      object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                                ..
//........................................................................................
//........................................................................................
/*****************************************************************************
 * FileName: FLAPI.C - Contains user API file access level source code.
 *
 * SanDisk Host Developer's Toolkit
 *
 * Copyright (c) 1996-1999 SanDisk Corporation
 * Copyright EBS Inc. 1996
 * All rights reserved.
 * This code may not be redistributed in source or linkable object form
 * without the consent of its author.
 *
 *
 * The following routines are included:
 *
 *    po_close        - Close a file and flush the file allocation table.
 *    po_extend_file  - Extend a file by N contiguous clusters.
 *    po_flush        - Flush an open file
 *    po_lseek        - Move the file pointer.
 *    po_open         - Open a file.
 *    po_read         - Read bytes from a file.
 *    po_trunc        - Truncate an open file
 *    po_write        - Write Bytes to a file.
 *
 ******************************************************************************/

#include <util/debug/debug.h>
#include <io/storage/blk_dev.h>
#include "pcdisk.h"
#include "iome_fat.h"

DEBUG_MODULE( PO_READ );
DEBUG_USE_MODULE( PO_READ );

DEBUG_MODULE( PO_WRITE );
DEBUG_USE_MODULE( PO_WRITE );

#if (USE_FILE_SYSTEM)

// Local functions
static ULONG _po_lseek(PC_FILE *pfile, INT32 offset, INT16 origin, INT16 *err_flag);


/******************************************************************************
 * Name: PO_OPEN -  Open a file.
 *
 * Description:
 *       Open the file for access as specified in flag. If creating use
 *       mode to set the access permissions on the file.
 *
 * Entries:
 *       TEXT *name      File Name
 *       UINT16 flag     Flag values are:
 *               PO_BINARY       - Ignored. All file access is binary
 *               PO_TEXT         - Ignored
 *               PO_RDONLY       - Open for read only
 *               PO_RDWR         - Read/write access allowed.
 *               PO_WRONLY       - Open for write only
 *
 *               PO_CREAT        - Create the file if it does not exist. Use
 *                                 mode to specify the permission on the file.
 *               PO_EXCL         - If flag contains (PO_CREAT | PO_EXCL) and
 *                                 the file already exists fail and set
 *                                 fs_user->p_errno to EEXIST
 *               PO_TRUNC        - Truncate the file if it already exists
 *               PO_NOSHAREANY   - Fail if the file is already open. If the
 *                                 open succeeds no other opens will succeed
 *                                 until it is closed.
 *               PO_NOSHAREWRITE - Fail if the file is already open for write.
 *                                 If the open succeeds no other opens for
 *                                 write will succeed until it is closed.
 *       UINT16 mode     Mode values are:
 *               PS_IWRITE       - Write permitted   
 *               PS_IREAD        - Read permitted. (Always true anyway)
 *
 * Returns:
 *       Returns a non-negative integer to be used as a file descriptor
 *       for calling read/write/seek/close otherwise it returns -1 and
 *       fs_user->p_errno is set to one of these values
 *
 *    PENOENT     - File not found or path to file not found
 *    PEMFILE     - No file descriptors available (too many files open)
 *    PEEXIST     - Exclusive access requested but file already exists.
 *    PEACCESS    - Attempt to open a read only file or a special (directory)
 *                  file.
 *    PENOSPC     - Create failed
 *    PESHARE     - Already open in exclusive mode or we want exclusive
 *                  and its already open
 ******************************************************************************/
PCFD po_open(const TEXT *name, UINT16 flag, UINT16 mode) /*__fn__*/
{
    PC_FILE *pfile;
    DROBJ   *parent_obj;
    DROBJ   *pobj;
    TEXT    *path;
    ULONG   ltemp;
    UINT32  cluster;
    UINT16  open_for_write;
    UINT16  sharing_error;
    INT16   p_errno;
    INT16   driveno;
    PCFD    fd;
    TEXT    filename[12];
    TEXT    fileext[4];


    CHECK_MEM(PCFD, -1);     /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(PCFD, -1);    /* Check if a valid user if multitasking */

    sharing_error = NO;
    parent_obj = SDNULL;
    pobj = SDNULL;
    saveFileName = SDNULL;

    ltemp = 0L;
    cluster = 0;
    open_for_write = NO;

#if (RTFS_WRITE)
    /* We'll need to know this in a few places. */
    if ( flag & (PO_WRONLY | PO_RDWR) )
        open_for_write = YES;
#endif
    p_errno = 0;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive((TEXT*)name);
    if ( driveno < 0 )
    {
        PC_FS_EXIT();
        return (-1);
    }

    if ( (fd = pc_allocfile()) < 0 ) /* Grab a file */
    {
        p_errno = PEMFILE;
        goto errex1;
    }

    /* Get the FILE. This will never fail */
    pfile = pc_fd2file(fd, 0);

    /* Paranoia. Set the obj to null until we have one */
    pfile->pobj = SDNULL;
    pfile->setDate = YES;
    pfile->dataBuff = SDNULL;

    PC_DRIVE_ENTER(driveno, NO);     /* Register drive in use */

    /* Get out the filename and d:parent */
    path = (TEXT *)fspath;
    path[0] = 0;

    if ( !validate_current_information(path, filename, fileext, (TEXT*)name) ) {
        p_errno = PENOENT;
        goto errex;
    }

    /* Find the parent */
    parent_obj = pc_fndnode(path);

    if ( !parent_obj ) {
        p_errno = PENOENT;
        goto errex;
    }

    PC_INODE_ENTER(parent_obj->finode, YES);
    if ( !pc_isadir(parent_obj) || pc_isavol(parent_obj) ) {
        p_errno = PENOENT;
        goto errex;
    }

    longFileName = saveFileName;

    pobj =  pc_get_inode(SDNULL,
        parent_obj,
        filename,
        fileext);

    if ( pobj )
    {
        PC_INODE_ENTER(pobj->finode, YES);
        
        /* If we goto exit: we want them linked so we can clean up */
        pfile->pobj = pobj; /* Link the file to the object */
#if (RTFS_SHARE)
        /* check the sharing conditions */
        sharing_error = NO;
        if ( pobj->finode->opencount != 1 )
        {
            /* The file is already open by someone. Lets see
               if we are compatible.
            */
            /* 1. We don't want to share with anyone */
            if ( flag & PO_NOSHAREANY )
                sharing_error = YES;

            /* 2. Someone else doesn't want to share */
            if ( pobj->finode->openflags & OF_EXCLUSIVE )
                sharing_error = YES;

            /* 3. We want exclusive write but already open for write */
            if ( open_for_write && (flag & PO_NOSHAREWRITE) &&
                (pobj->finode->openflags & OF_WRITE) )
                sharing_error = YES;

            /* 4. We want open for write but it's already open for exclusive */
            if ( open_for_write && 
                (pobj->finode->openflags & OF_WRITEEXCLUSIVE) )
                sharing_error = YES;

            /* 5. Open for trunc when already open */
            if ( flag & PO_TRUNC )
                sharing_error = YES;
        }

        if ( sharing_error )
        {
            PC_INODE_EXIT(pobj->finode);
            
            p_errno = PESHARE;
            goto errex;
        }
#endif  /* RTFS_SHARE */

        if ( pc_isadir(pobj) || pc_isavol(pobj) )
        {
            PC_INODE_EXIT(pobj->finode);
            p_errno = PEACCES; /* is a directory */
            goto errex;
        }

#if (RTFS_WRITE)
        if ( (flag & (PO_EXCL|PO_CREAT)) == (PO_EXCL|PO_CREAT) )
        {
            PC_INODE_EXIT(pobj->finode);
            p_errno = PEEXIST; /* Exclusive fail */
            goto errex;
        }

        if ( open_for_write && (pobj->finode->fattribute & ARDONLY) )
        {
            PC_INODE_EXIT(pobj->finode);
            
            p_errno = PEACCES; /* read only file */
            goto errex;
        }

        if ( flag & PO_TRUNC )
        {
            /* For all FAT types including FAT32 */
            cluster = (UINT32)pobj->finode->fclusterHi;
            cluster <<= 16;
            cluster |= (UINT32)pobj->finode->fcluster;
            ltemp   = pobj->finode->fsize;

            /* Clear the file information */
            pobj->finode->fcluster = 0;
            pobj->finode->fclusterHi = 0;
            pobj->finode->fsize = 0L;
            pobj->finode->lastAccess = 0;

            /* Convert to native. Overwrite the existing inode.
               Set archive/date.
            */
            if ( pc_update_inode(pobj, YES, YES) )
            {
                /* And clear up the space */
                PC_FAT_ENTER(pobj->pdrive->driveno);
                
                pc_freechain(pobj->pdrive, cluster);
                pc_flushfat(pobj->pdrive->driveno);

                PC_FAT_EXIT(pobj->pdrive->driveno);
            }
            else
            {
                pobj->finode->fcluster = (UINT16)(cluster & 0x0FFFF);
                pobj->finode->fclusterHi = (UINT16)(cluster >> 16);
                pobj->finode->fsize = ltemp;
                pobj->finode->lastAccess = 0; /* Date */

                PC_INODE_EXIT(pobj->finode);
                p_errno = PEACCES;
                goto errex;
            }
        }
#endif
    }
    else /* File not found */
    {
#if (RTFS_WRITE)
        if ( !(flag & PO_CREAT) )
        {
            p_errno = PENOENT; /* File does not exist */
            goto errex;
        }

        /* Do not allow create if write bits not set */
        if ( !open_for_write )
        {
            p_errno = PEACCES; /* read only file */
            goto errex;
        }

        /* Create for read only if write perm not allowed */
        pobj = pc_mknode( parent_obj,
            filename,
            fileext,
            SDNULL,
            ((mode == PS_IREAD) ? ARDONLY : 0),
            0);

        if ( !pobj )
        {
            p_errno = PENOSPC;
            goto errex;
        }

        pfile->pobj = pobj;     /* Link the file to the object */
#else                                   /* Write not built in. Get out */
        p_errno = PENOENT;      /* File does not exist */
        goto errex;
#endif

        PC_INODE_ENTER(pobj->finode, YES);
    }

    /* Set the file sharing flags in the shared finode structure */
    /* clear flags if we just opened it . */
    if ( pobj->finode->opencount == 1 )
        pobj->finode->openflags = 0;

#if (RTFS_WRITE)
    if ( open_for_write )
    {
        pobj->finode->openflags |= OF_WRITE;
        if ( flag & PO_NOSHAREWRITE )
            pobj->finode->openflags |= OF_WRITEEXCLUSIVE;
    }

    if ( flag & PO_NOSHAREANY )
        pobj->finode->openflags |= OF_EXCLUSIVE;
#endif

    pfile->flag = flag; /* Access flags */
    pfile->fptr = 0L;   /* File pointer */

    /* Set the cluster and block file pointers */
    _synch_file_ptrs(pfile);
    PC_INODE_EXIT(pobj->finode);

    p_errno = 0;
    if ( parent_obj )
    {
        PC_INODE_EXIT(parent_obj->finode);
        pc_freeobj(parent_obj);
    }

    PC_DRIVE_EXIT(driveno);
    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT();
    
    return(fd);
        
    errex:
    pc_freefile(fd);
    if (parent_obj)
    {
        PC_INODE_EXIT(parent_obj->finode);
        pc_freeobj(parent_obj);
    }
    PC_DRIVE_EXIT(driveno);
    errex1:
    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT();
    
    return (-1);
}

#if 0
// (epg,11/24/2000): hard error propagation helper, to alleviate paging difficulties from
// increasing the po_read ret_bad_f goto block code size.  Hopefully, calling out
// to another function will make po_read load into memory more efficiently.
PropagateHE(DDRIVE* pdrive,ULONG block_to_read,UCOUNT count,UINT32 n_left)
{
    unsigned int lba_last_error;
    unsigned int valid_read_bytes;
    
    lba_last_error = iome_get_lba_last_error(pdrive->driveno);
    DEBUG_HE("HE:lba last err %d\n",lba_last_error);
    //        DEBUG_HE("HE:count %x n_left %x lba_last_error %x block_to_read %x\n",
    //            count, n_left, lba_last_error, block_to_read);
    
    if ( pdrive->enable_mapping )
    {
        block_to_read += pdrive->volume_start;
        DEBUG_HE("HE:(map)blocks to read bumped %d to %d\n",pdrive->volume_start,block_to_read);
    }
    
    DEBUG_HE("HECalculating vrb=count(%d) - (n_left(%d) - ((lba_last_error(%d) - block_to_read(%d) * 512))\n",count,n_left,lba_last_error,block_to_read);
    valid_read_bytes = count - (n_left - ((lba_last_error - block_to_read) * 512));
    DEBUG_HE("HEvrb calculated as %d, setting\n",valid_read_bytes);
    pc_set_valid_read_data(valid_read_bytes);
    DEBUG_HE("HE:prop done\n");
}
#endif /* !__DHARMA */

/******************************************************************************
 * Name: PO_READ -  Read from a file.
 *
 * Description:
 *       Attempt to read count bytes from the current file pointer of file
 *       at fd and put them in buf. The file pointer is updated.
 *
 * Entries:
 *       PCFD fd
 *       UCHAR *buf
 *       UCOUNT count
 *
 * Returns:
 *       Returns the number of bytes read or -1 on error.
 *       If the return value is ((UINT16) ~0) (UINT16) -1 fs_user->p_errno
 *       will be set with one of the following:
 *
 *               PENBADF     - File descriptor invalid
 *               PENOSPC     - File IO error
 *
 ******************************************************************************/
ULONG  po_read(PCFD fd, UCHAR *pbuf, ULONG count) /*__fn__*/
{
    PC_FILE *pfile;
    UCHAR   *buf;
    DDRIVE  *pdrive;
    ULONG   block_to_read;
    ULONG   saved_ptr;
    ULONG   saved_ptr_block;     
    UINT32  saved_ptr_cluster;       
    UINT32  next_cluster;
    UINT32  n_bytes;
    UINT32  n_left;
    UINT32  n_clusters;
    ULONG  n_to_read;
    ULONG   ret_val;
    ULONG   utemp;
    UINT32  block_in_cluster;
    UINT32  byte_offset_in_block;
    INT16   p_errno;

    CHECK_MEM(UINT16, ~0);   /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(UINT16, ~0);  /* Check if a valid user if multitasking */
      
    DEBUG( PO_READ, DBGLEV_TRACE, "+%s (%d)\n", __FUNCTION__,__LINE__);
    DEBUG( PO_READ, DBGLEV_TRACE, " %s (%d): fd = %d, pbuf = 0x%08x, count = %d\n",
        __FUNCTION__,__LINE__,fd,(unsigned int)pbuf, count);
    

    errno = 0;
    p_errno = 0;
    buf = pbuf;

    /* Get the FILE. */
    if ( (pfile = pc_fd2file(fd, 0)) == SDNULL )
    {
        p_errno = PEBADF;
        ret_val = (ULONG) ~0;
        goto return_error;
    }

    /* return 0 bytes read if bad arguments */
    if ( !count || !buf )
    {
        ret_val = 0;
        goto return_error;
    }


    PC_ENTER_CRITICAL();
    if ( pfile->fptr >= pfile->pobj->finode->fsize )
    {       /* Dont read if done */
        ret_val = 0;
        PC_EXIT_CRITICAL();
        goto return_error;
    }
    PC_EXIT_CRITICAL();
    
    /* Register Drive in use */
    pdrive = pfile->pobj->pdrive;
    PC_DRIVE_ENTER(pdrive->driveno, NO);
    
    /* Grab exclusive access to the drobj */
    PC_INODE_ENTER(pfile->pobj->finode, YES);
    
    /* Set the cluster and block file pointers if not already set */
    _synch_file_ptrs(pfile);

    saved_ptr = pfile->fptr;
    saved_ptr_block = pfile->fptr_block;        
    saved_ptr_cluster = pfile->fptr_cluster;      


    /* Truncate the read count if we need to */
    if ( (pfile->fptr + count) >= pfile->pobj->finode->fsize )
        count = (ULONG) (pfile->pobj->finode->fsize - pfile->fptr);

    n_left = count;

    /* Remove the following comment to keep the file's date and time
       unchanged if a file is to copy on the same drive.
    */
#if 0
    ret_val = pfile->flag & 0x0FFF;
    if ( (pfile->setDate == YES) && ((ret_val == PO_RDONLY) || ((ret_val & PO_RDWR)== PO_RDWR)))
    {
        pfile->setDate = NO;
        pfile->dataBuff = buf;
    }                
#endif

    while ( n_left )
    {
        block_in_cluster = (UINT32) (pfile->fptr & pdrive->byte_into_cl_mask);
        block_in_cluster >>= 9;
        block_to_read = pfile->fptr_block + block_in_cluster;

        /* How many clusters are left */
        n_to_read = (ULONG) ( (n_left + 511) >> 9 );
        n_clusters = (UINT32) ((n_to_read +
                                   pdrive->secpalloc - 1) >> pdrive->log2_secpalloc);

        /* how many contiguous clusters can we get ? <= n_clusters */
        PC_FAT_ENTER(pdrive->driveno);
        n_clusters = pc_get_chain(pdrive,
            pfile->fptr_cluster,
            &next_cluster,
            n_clusters);
        PC_FAT_EXIT(pdrive->driveno);
        
        if ( !n_clusters )
            goto ret_bad_f;


        /* Are we inside a block */
        if ( (pfile->fptr & 0x1FFL) || (n_left < 512) )
        {
            byte_offset_in_block = (UINT32) (pfile->fptr & 0x1FFL);

            /* READ - Use the block buffer pool to read for us */
            fslocal_buffer = pc_read_blk(pdrive, block_to_read);
            if ( !fslocal_buffer )
                goto ret_bad_f;

            /* Copy source data to the local buffer */
            n_bytes = (ULONG) (512 - byte_offset_in_block);
            if (n_bytes > n_left)
                n_bytes = n_left;

            copybuff(buf,
                (((UCHAR *)fslocal_buffer->data) + byte_offset_in_block),
                (UINT32)n_bytes);

            pc_verify_blk(fslocal_buffer); // temp

#if defined(USE_FS_BUFFER)
            /* Release the buffer. Don't discard it */
            pc_free_buf(fslocal_buffer, NO, YES);
#else /* USE_FS_BUFFER */
            /* Release the buffer. and discard it */
            pc_free_buf(fslocal_buffer, YES, YES);
#endif /* USE_FS_BUFFER */

            buf += n_bytes;
            n_left = (ULONG) (n_left - n_bytes);
            pfile->fptr += n_bytes;


            /* Are we on a cluster boundary  ? */
            if ( !(pfile->fptr & pdrive->byte_into_cl_mask) )
            {
                if ( --n_clusters ) /* If contiguous */
                {
                    pfile->fptr_block += pdrive->secpalloc;
                    pfile->fptr_cluster += 1;
                }
                else
                {
                    pfile->fptr_cluster = next_cluster;
                    pfile->fptr_block = pc_cl2sector(pdrive,
                        next_cluster);
                } /* if (--nclusters) {} else {}; */
            } /* if (!(pfile->fptr & pdrive->byte_into_cl_mask)) */
        } /* if ( (pfile->fptr & 0x1FF) || (n_left < 512) ) */
        else
        {
            /* Read as many blocks as possible */
            /* How many blocks in the current chain */
            n_to_read = (ULONG) (n_clusters << pdrive->log2_secpalloc);


            /* subtract out any leading blocks */
            n_to_read = (ULONG) (n_to_read - block_in_cluster);

            /* How many blocks yet to read */
            utemp = (ULONG) (n_left >> 9);

            /* take the smallest of the two */
            if ( n_to_read > utemp )
                n_to_read = utemp;

            if ( n_to_read )
            {
                /* If we get here we need to read contiguous blocks */
                block_to_read = pfile->fptr_block + block_in_cluster;

                /* READ */
                if ( !devio_read(pdrive->driveno,
                         block_to_read,
                         buf,
                         n_to_read) )
                    goto ret_bad_f;

                n_bytes = (ULONG) (n_to_read << 9);
                buf += n_bytes;
                n_left= (ULONG)(n_left - n_bytes);
                pfile->fptr += n_bytes;

                /* if we advanced to a cluster boundary
                   advance the cluster pointer */
                /* utemp == s how many clusters we read */
                utemp = (ULONG) ((n_to_read + block_in_cluster) >> pdrive->log2_secpalloc);

                if ( utemp == n_clusters )
                {
                    pfile->fptr_cluster = next_cluster;
                }
                else
                {
                    /* advance the pointer as many as we read */
                    pfile->fptr_cluster = (pfile->fptr_cluster + (UINT32)utemp);
                }
                pfile->fptr_block = pc_cl2sector(pdrive,
                    pfile->fptr_cluster);
            }
        }
    } /* while n_left */

    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdrive->driveno);

    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT();

    DEBUG( PO_READ, DBGLEV_TRACE, "-%s (%d)\n", __FUNCTION__,__LINE__);
    return(count);

    ret_bad_f:
    /*
      need to move this goto block into a separate function, or at least the hard error stuff.


    */

    /* Restore pointers and return */
    pfile->fptr = saved_ptr;
    pfile->fptr_block = saved_ptr_block;        
    pfile->fptr_cluster = saved_ptr_cluster;

#ifndef DADIO_BOOT
#if 1
    if (errno == ERDIO) {
    }
#else /* __DHARMA */
    if (errno == EREAD) {
        DEBUG_HE("HE:prop(blocktoread %d,count %d,left %d\n",block_to_read,count,n_left);
        PropagateHE(pdrive,block_to_read,count,n_left);
    }
#endif /* __DHARMA */
    else {
        p_errno = PENOSPC;
    }

#else /* DADIO_BOOT */
    p_errno = PENOSPC;
#endif /* DADIO_BOOT */

    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdrive->driveno);

    ret_val = (ULONG)~0;


    return_error:
    /* Restore the kernel state */
    if (!errno) {
        errno = p_errno;
    }
    PC_FS_EXIT();
    DEBUG( PO_READ, DBGLEV_TRACE, "-%s (%d)\n", __FUNCTION__,__LINE__);

    return (ret_val);
}


#if (RTFS_WRITE)
/******************************************************************************
 * Name: PO_WRITE    -  Write to a file.
 *
 * Description:
 *       Attempt to write count bytes from buf to the current file pointer
 *       of file at fd. The file pointer is updated.
 *
 * Entries:
 *       PCFD fd
 *       UCHAR *buf
 *       UCOUNT count
 *
 * Returns:
 *       Returns the number of bytes written or -1 on error.
 *       If the return value is -1 fs_user->p_errno will be set with
 *       one of the following:
 *
 *               PENBADF     - File descriptor invalid or open read only
 *               PENOSPC     - Write failed. Presumably because of no space
 *
 ******************************************************************************/
ULONG po_write(PCFD fd, UCHAR *buf, ULONG count) /*__fn__*/
{
    PC_FILE *pfile;
    /*
      PC_FILE *tmp_pfile;
    */
    DDRIVE  *pdrive;
    ULONG   saved_ptr;
    ULONG   saved_ptr_block;     
    ULONG   ltemp;
    ULONG   alloced_size;
    ULONG   block_to_write;
    UINT32  next_cluster;
    UINT32  saved_ptr_cluster;       
    UINT32  n_bytes;
    UINT32  n_left;
    ULONG   ret_val;
    ULONG   n_to_write;
    UINT32  block_in_cluster;
    UINT32  byte_offset_in_block;
    UINT32  n_blocks_left;
    UINT32  n_clusters;
    INT16   p_errno;
    INT16   err_flag;

    CHECK_MEM(UINT16, ~0);   /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(UINT16, ~0);  /* Check if a valid user if multitasking */
    DEBUG( PO_WRITE, DBGLEV_TRACE, "+%s (%d)\n", __FUNCTION__,__LINE__);
    DEBUG( PO_WRITE, DBGLEV_TRACE, " %s (%d): fd = %d, buf = 0x%08x, count = %d\n",
        __FUNCTION__,__LINE__,fd,buf,count);
    errno = 0;
    p_errno = 0;
    err_flag = 0;

    /* Get the FILE. must be open for write */
    pfile = pc_fd2file(fd, (PO_WRONLY | PO_RDWR));
    if ( !pfile )
    {
        p_errno = PEBADF;
        ret_val = (ULONG) ~0;
        goto return_error;
    }

    /* Return 0 (none written) on bad args */
    if ( !count || !buf )
    {
        ret_val = 0;
        goto return_error;
    }
    
    /* Register drive in use (non excl) */
    pdrive = pfile->pobj->pdrive;
    PC_DRIVE_ENTER(pdrive->driveno, NO);

    /* Only one process may write at a time */
    PC_INODE_ENTER(pfile->pobj->finode, YES);

    /* 
     * Note the file's finode is LOCKED below so the code needn't be 
     * reentrant relative to the finode. 
     *
     * if the  file is zero sized make sure the current cluster pointer
     is invalid.
    */
    if ( !pfile->pobj->finode->fsize )
        pfile->fptr_cluster = 0;

    /* Set the cluster and block file pointers if not already set */
    _synch_file_ptrs(pfile);
    saved_ptr = pfile->fptr;
    saved_ptr_block = pfile->fptr_block;        
    saved_ptr_cluster = pfile->fptr_cluster;        

    /* dc- dont allow files larger than 2gb, since the fat code doesn't perform chain length checking */
    if( pfile->pobj->finode->fsize + count >= ((unsigned)2*1024*1024*1024) ) {
        goto ret_bad_f;
    }
    
    /* Added this code to allow append mode */
    if ( pfile->flag & PO_APPEND )
    {
        if ( pfile->pobj->finode->fsize )
        {
            _po_lseek(pfile, 0L, PSEEK_END, &err_flag);
            if ( err_flag )
                goto ret_bad_f;

            _synch_file_ptrs(pfile);
        }
    }

    /* calculate initial values */
    n_left = count;

    /*
      Round the file size up to its cluster size by adding in
      clustersize-1 and masking off the low bits.
    */
    alloced_size =  (pfile->pobj->finode->fsize +
        pdrive->byte_into_cl_mask) & ~(pdrive->byte_into_cl_mask);


    /* Remove the following comment to keep the file's date and time
       unchanged if a file is to copy on the same drive.
    */
#if 0
    if (pfile->setDate && (pfile->dataBuff == SDNULL))
    {
        for (ltemp=0; ltemp < NUSERFILES; ltemp++)
        {
            tmp_pfile = &mem_file_pool[ltemp];

            if (tmp_pfile->is_free)
                continue;

            ret_val = tmp_pfile->flag & 0x0FFF;
            if ((ret_val == PO_RDONLY) || ((ret_val & PO_RDWR) == PO_RDWR))
            {
                if (buf == tmp_pfile->dataBuff)
                {
                    PC_ENTER_CRITICAL();
                    pfile->pobj->finode->ftime = tmp_pfile->pobj->finode->ftime;
                    pfile->pobj->finode->fdate = tmp_pfile->pobj->finode->fdate;
                    pfile->setDate = NO;
                    tmp_pfile->dataBuff = SDNULL;
                    PC_EXIT_CRITICAL();
                    break;
                }
            }
        }
        if (ltemp == NUSERFILES)
            pfile->dataBuff = buf;
    }
#endif

    while ( n_left )
    {
        block_in_cluster = (UCOUNT) (pfile->fptr & pdrive->byte_into_cl_mask);
        block_in_cluster >>= 9;

        if ( pfile->fptr >= alloced_size )
        {
            /* Extending the file */
            n_blocks_left = (UINT32)((n_left + 511) >> 9);

            /* how many clusters are left-
             *  assume 1 for the current cluster.
             *   subtract out the blocks in the current
             *   round up by adding secpalloc-1 and then
             *   divide by sectors per cluster

             |  n_clusters = 1 + 
             |   (n_blocks_left-
             |       (pdrive->secpalloc-block_in_cluster)
             |       + pdrive->secpalloc-1) >> pdrive->log2_secpalloc;
             ==>
            */
            n_clusters = (1 + ((n_blocks_left +
                                   block_in_cluster -1) >> pdrive->log2_secpalloc));

            /*
              Call pc_alloc_chain to build a chain up to n_cluster
              clusters long. Return the first cluster in
              pfile->fptr_cluster and return the # of clusters in
              the chain. If pfile->fptr_cluster is non zero link
              the current cluster to the new one.
            */
            PC_FAT_ENTER(pdrive->driveno);
            n_clusters = pc_alloc_chain(pdrive,
                &(pfile->fptr_cluster),
                n_clusters);
            PC_FAT_EXIT(pdrive->driveno);

            if ( !n_clusters )
                goto ret_bad_f;

            /* Calculate the last cluster in this chain. */
            next_cluster = pfile->fptr_cluster + (UINT32)(n_clusters - 1);


            /* link the chain to the directory object if just starting */
            /* dc- assume that if one of these is set that the cluster chain is already attached */
            if ( !pfile->pobj->finode->fcluster && !pfile->pobj->finode->fclusterHi ) {
                pfile->pobj->finode->fcluster = (UINT16)(pfile->fptr_cluster & 0x0FFFF);
                pfile->pobj->finode->fclusterHi = (UINT16)(pfile->fptr_cluster >> 16);
            }

            /* calculate the new block pointer */
            pfile->fptr_block = pc_cl2sector(pdrive,
                pfile->fptr_cluster);

            /* calculate amount of space used by the file */
            ltemp = n_clusters << pdrive->log2_secpalloc;
            ltemp <<= 9;
            alloced_size += ltemp;
        }
        else /* Not extending the file. (writing inside the file) */
        {
            n_blocks_left = (UINT32)((n_left + 511) >> 9);

            /* how many clusters are left-
             *  assume 1 for the current cluster.
             *   subtract out the blocks in the current
             *   round up by adding secpalloc-1 and then
             *   divide by sectors per cluster

             |  n_clusters = 1 + 
             |   (n_blocks_left-
             |       (pdrive->secpalloc-block_in_cluster)
             |       + pdrive->secpalloc-1) >> pdrive->log2_secpalloc;
             ==>
            */
            n_clusters = ( 1 + ((n_blocks_left +
                                    block_in_cluster -1) >> pdrive->log2_secpalloc) );

            /* How many contiguous clusters can we get ? <= n_clusters */
            PC_FAT_ENTER(pdrive->driveno);
            
            n_clusters = pc_get_chain(pdrive,
                pfile->fptr_cluster,
                &next_cluster,
                n_clusters);
            PC_FAT_EXIT(pdrive->driveno);
            
            if ( !n_clusters )
                goto ret_bad_f;
        }

        /* Are we inside a block */
        if ( (pfile->fptr & 0x1FFL) || (n_left < 512) )
        {
            block_in_cluster = (UCOUNT) (pfile->fptr & pdrive->byte_into_cl_mask);
            block_in_cluster >>= 9;
            block_to_write = pfile->fptr_block + block_in_cluster;

            byte_offset_in_block = (UINT32) (pfile->fptr & 0x1FFL);

            /* Copy source data to the local buffer */
            n_bytes = (UINT32)(512 - byte_offset_in_block);
            if ( n_bytes > n_left )
                n_bytes = n_left;

            /* READ - Use the block buffer pool to read for us */
            fslocal_buffer = pc_read_blk(pdrive, block_to_write);
            if ( !fslocal_buffer )
                goto ret_bad_f;

            copybuff(((UCHAR *)fslocal_buffer->data + byte_offset_in_block),
                buf,
                (UINT32)n_bytes);


            pc_verify_blk(fslocal_buffer); // temp

            /* Write */
            if ( !pc_write_blk(fslocal_buffer) )
            {
                /* Release the buffer. and discard it */
                pc_free_buf(fslocal_buffer, YES, YES);
                goto ret_bad_f;
            }

            /* Release the buffer. and discard it */
            pc_free_buf(fslocal_buffer, YES, YES);


            buf += n_bytes;
            n_left = (n_left - n_bytes);
            pfile->fptr += n_bytes;

            /* Are we on a cluster boundary  ? */
            if ( !(pfile->fptr & pdrive->byte_into_cl_mask) )
            {
                if ( --n_clusters ) /* If contiguous */
                {
                    pfile->fptr_block += pdrive->secpalloc;
                    pfile->fptr_cluster += 1;
                }
                else
                {
                    /*
                      NOTE: Put the next cluster into the
                      pointer. If alloced a chain this value
                      is the last cluster in the chain and does
                      not concur with the byte file pointer.
                      This is not a problem since the cluster
                      pointer is known to be off at this point
                      any (fptr>=alloced_size).
                    */
                    pfile->fptr_cluster = next_cluster;
                    pfile->fptr_block = pc_cl2sector(pdrive,
                        next_cluster);
                } /* if (--nclusters) {} else {}; */
            } /* if (!(pfile->fptr & byte_into_cl_mask)) */
        } /* if ( (pfile->fptr & 0x1ff) || (n_left < 512) ) */

        if ( n_clusters && (n_left > 511) )
        {
            /* If we get here we need to write contiguous blocks */
            block_in_cluster = (UINT32) (pfile->fptr &
                pdrive->byte_into_cl_mask);
            block_in_cluster >>= 9;
            block_to_write = pfile->fptr_block + block_in_cluster;

            /* how many do we write ? */
            n_blocks_left = (UINT32)(n_left >> 9);
            n_to_write = ((n_clusters << pdrive->log2_secpalloc) -
                block_in_cluster);

            if ( n_to_write > n_blocks_left )
            {
                n_to_write = n_blocks_left;

                /*
                  If we are not writing to the end of the
                  chain we may not advance the cluster pointer
                  to the beginning of the next chain. We add
                  in block_in_cluster so we account for the
                  partial cluster we've already seen.
                */
                next_cluster = pfile->fptr_cluster +
                               (UINT32)((n_to_write+block_in_cluster) >> pdrive->log2_secpalloc);
            }

            if ( !devio_write(pdrive->driveno,
                     block_to_write,
                     buf,
                     n_to_write) )
                goto ret_bad_f;

            n_bytes = (n_to_write << 9);

            buf += n_bytes; 
            n_left = (ULONG) (n_left - n_bytes);
            pfile->fptr += n_bytes;

            /* See note above */
            pfile->fptr_cluster = next_cluster;
            pfile->fptr_block   = pc_cl2sector(pdrive, next_cluster);
        }
    } /* while n_left */

    if ( pfile->fptr > pfile->pobj->finode->fsize )
    {
        pfile->needs_flush = YES;
        pfile->pobj->finode->fsize = pfile->fptr;
    }

    /*
      If the file pointer is beyond the space allocated to the file note it.
      since we may need to adjust this file's cluster and block pointers
      later if someone else extends the file behind our back.
    */
    pfile->at_eof = (pfile->fptr >= alloced_size);

    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdrive->driveno);
    
    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT();
    
    DEBUG( PO_WRITE, DBGLEV_TRACE, "-%s (%d)\n", __FUNCTION__,__LINE__);
    return (count);

ret_bad_f:
    /* Restore pointers and return */
    pfile->fptr = saved_ptr;
    pfile->fptr_block = saved_ptr_block;        
    pfile->fptr_cluster = saved_ptr_cluster;        
    p_errno = PENOSPC;
    PC_INODE_EXIT(pfile->pobj->finode);      /* Release excl use of finode */
    PC_DRIVE_EXIT(pdrive->driveno);          /* Release non-excl use of drive */
    ret_val = (ULONG) ~0;

return_error:
    /* Restore the kernel state */
    if (!errno) {
        errno = p_errno;
    }
    PC_FS_EXIT();
    DEBUG( PO_WRITE, DBGLEV_TRACE, "-%s (%d)\n", __FUNCTION__,__LINE__);

    return (ret_val);
}
#endif  /* (RTFS_WRITE) */


/*****************************************************************************
 * Name: PO_LSEEK    -  Move file pointer
 *
 * Description:
 *       Move the file pointer offset bytes from the origin described by 
 *       origin. The file pointer is set according to the following rules.
 *
 *       Origin              Rule
 *       PSEEK_SET           offset from begining of file
 *       PSEEK_CUR           offset from current file pointer
 *       PSEEK_END           offset from end of file
 *
 *       Attempting to seek beyond end of file puts the file pointer one
 *       byte past eof. 
 *
 * Entries:
 *       PCFD fd
 *       ULONG offset
 *       INT16 origin
 *       INT16 *err_flag
 *
 * Returns:
 *       Returns the new offset or -1 on error.
 *
 *       If the return value is -1 fs_user->p_errno will be set with
 *       one of the following:
 *               PENBADF     - File descriptor invalid
 *               PEINVAL     - Seek to negative file pointer attempted.
 *
 ******************************************************************************/
ULONG po_lseek(PCFD fd, INT32 offset, INT16 origin, INT16 *err_flag) /*__fn__*/
{
    PC_FILE *pfile;
    DDRIVE  *pdrive;
    ULONG   ret_val;
    INT16   p_errno;

    CHECK_MEM(ULONG, -1);    /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(ULONG, -1);   /* Check if a valid user if multitasking */

    errno = 0;
    p_errno = 0;    

    /* Get the FILE */  
    pfile = pc_fd2file(fd, 0);
    if ( !pfile )
    {
        p_errno = PEBADF;
        *err_flag = -1;
        ret_val = (ULONG)-1;
        goto return_error;
    }

    pdrive = pfile->pobj->pdrive;
    PC_DRIVE_ENTER(pdrive->driveno, NO); /* Register drive in use */

    /* Grab exclusive access to the drobj */
    PC_INODE_ENTER(pfile->pobj->finode, YES);

    /*
     * Note the file's finode is LOCKED below so the code needn't be 
     * reentrant relative to the finode. 
     */
    /* Set the cluster and block file pointers if not already set */
    _synch_file_ptrs(pfile);

    /* Call the internal seek routine that we share with po_trunc */
    ret_val = _po_lseek(pfile, offset, origin, err_flag);

    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdrive->driveno);

    return_error:   /* No only errors return through here. Everything does. */

    /* Restore the kernel state */
    if (!errno) {
        errno = p_errno;
    }
    PC_FS_EXIT();
    
    return (ret_val);
}


#if (RTFS_WRITE)
/*****************************************************************************
 * Name: PO_TRUNCATE -  Truncate an open file.
 *
 * Description:
 *       Move the file pointer offset bytes from the beginning of the file
 *       and truncate the file beyond that point by adjusting the file size
 *       and freeing the cluster chain past the file pointer.
 *
 * Entries:
 *       PCFD fd
 *       ULONG offset
 *
 * Returns:
 *       Returns YES if successful otherwise NO
 *
 *       If the return value is NO fs_user->p_errno will be set
 *       with one of the following:
 *               PENBADF     - File descriptor invalid or open read only
 *               PENOSPC     - IO error
 *               PEINVAL     - Invalid offset
 *               PESHARE     - Can not truncate a file open by more than
 *                             one handle.
 *
 ******************************************************************************/
SDBOOL po_truncate(PCFD fd, LONG ofset) /*__fn__*/
{
    PC_FILE *pfile;
    DDRIVE  *pdrive;
    UINT32  clno;
    UINT32  first_cluster_to_release;
    UINT32  last_cluster_in_chain;
    ULONG   offset;
    SDBOOL  ret_val;
    INT16   p_errno;
    INT16   err_flag;

    CHECK_MEM(SDBOOL, 0);      /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(SDBOOL, 0);     /* Check if a valid user if multitasking */

    errno = 0;
    p_errno = 0;    

    /* Assume success until something fails */
    ret_val = YES;

    /* Get the FILE. must be open for write */
    pfile = pc_fd2file(fd, PO_WRONLY|PO_RDWR);

    /* Make sure we have write privilages */
    if ( !pfile )
    {
        p_errno = PEBADF;
        ret_val = NO;
        goto return_error;
    }

    pdrive = pfile->pobj->pdrive;
    PC_DRIVE_ENTER(pdrive->driveno, NO); /* Register drive in use */

    /* Grab exclusive access to the file */
    PC_INODE_ENTER(pfile->pobj->finode, YES);
    

    /*
     * Note the file's finode is LOCKED below so the code needn't be 
     * reentrant relative to the finode. 
     */

    /* Can only truncate a file that you hold exclusively */
    if ( pfile->pobj->finode->opencount > 1 )
    {
        ret_val = NO;
        p_errno = PESHARE;
        goto errex;
    }

    /* Set the cluster and block file pointers if not already set */
    _synch_file_ptrs(pfile);

    /*
      Call the internal seek routine that we share with po_lseek. Seek to
      offset from the origin of zero.
    */
    offset = _po_lseek(pfile, ofset, PSEEK_SET, &err_flag);
    if ( (err_flag) || (offset >= pfile->pobj->finode->fsize) )
    {
        p_errno = PEINVAL;
        ret_val = NO;
    }
    else
    {
        pfile->needs_flush = YES;

        /* Are we on a cluster boundary ? */
        if ( !(offset & pdrive->byte_into_cl_mask) )
        {
            /*
              Free the current cluster and beyond since we're on
              a cluster boundary.
            */
            first_cluster_to_release = pfile->fptr_cluster; 

            /* Find the previous cluster so we can terminate the chain */
            PC_FAT_ENTER(pdrive->driveno); /* claim the fat for alloc */
            clno = (UINT32)(pfile->pobj->finode->fclusterHi);
            clno <<= 16;
            clno |= (UINT32)pfile->pobj->finode->fcluster;
            last_cluster_in_chain = clno;

            while ( clno != first_cluster_to_release )
            {
                last_cluster_in_chain = clno;
                clno = pc_clnext(pdrive, clno);
                if ( clno == 0L )
                {
                    PC_FAT_EXIT(pdrive->driveno); 
                    ret_val = NO;
                    p_errno = PENOSPC;
                    goto errex;
                }
            }

            PC_FAT_EXIT(pdrive->driveno);
            

            /*
              Set ptr_cluster to last in chain so
              read & write will work right.
            */
            pfile->fptr_cluster = last_cluster_in_chain;
            if ( last_cluster_in_chain )
            {
                pfile->fptr_block = pc_cl2sector(pdrive,
                    last_cluster_in_chain);
            }
            else
                pfile->fptr_block = 0;

            pfile->at_eof = YES;
        }
        else /* Simple case. we aren't on a cluster boundary. Just free*/
        {       /* The chain beyond us and terminate the list */
            PC_FAT_ENTER(pdrive->driveno); /* claim the fat */
            last_cluster_in_chain = pfile->fptr_cluster;
            first_cluster_to_release = pc_clnext(pdrive,
                pfile->fptr_cluster);
            PC_FAT_EXIT(pdrive->driveno); 
            pfile->at_eof = YES;
        }

        /*  Now update the directory entry. */
        pfile->pobj->finode->fsize = offset;
        if ( !offset ) /* If the file goes to zero size unlink the chain */
        {
            pfile->pobj->finode->fclusterHi = 0;
            pfile->pobj->finode->fcluster = 0;
            pfile->pobj->finode->lastAccess = 0;
            pfile->fptr_cluster = 0;
            pfile->fptr_block = 0;
            pfile->fptr = 0;
            pfile->at_eof = NO;

            /*
              We're freeing the whole chain so we don't mark
              last_cluster in chain.
            */
            last_cluster_in_chain = 0;
        }

        /*
          Convert to native. Overwrite the existing inode.
          Set archive/date.
        */
        if ( !pc_update_inode(pfile->pobj, YES, YES) )
        {
            ret_val = NO;
            p_errno = PENOSPC;
            goto errex;
        }

        /* Terminate the chain and free the lost chain part */
        PC_FAT_ENTER(pfile->pobj->pdrive->driveno);
        

        /* Free the rest of the chain */
        if ( first_cluster_to_release )
        {
            /* Release the chain */
            pc_freechain(pfile->pobj->pdrive,
                first_cluster_to_release);
        }

        /* Null terminate the chain */
        if ( last_cluster_in_chain )
        {
            if ( !pc_pfaxx(pdrive, last_cluster_in_chain, 0x0FFFFFFFL) )
            {
                ret_val = NO;
                p_errno = PENOSPC;
            }
        }

        if ( !pc_flushfat(pfile->pobj->pdrive->driveno) )
        {
            ret_val = NO;
            p_errno = PENOSPC;
        }
        PC_FAT_EXIT(pfile->pobj->pdrive->driveno); 
    }

    errex:
    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdrive->driveno);
    

    return_error: /* No only errors return through here. Everything does. */

    /* Restore the kernel state */
    if (!errno) {
        errno = p_errno;
    }
    PC_FS_EXIT();
    

    return (ret_val);
}
#endif



/*****************************************************************************
 * Name:  _PO_LSEEK   -  Move file pointer (internal)
 *
 * Description
 *       Behaves as po_lseek but takes a file instead of a file descriptor.
 *
 *       Attempting to seek beyond end of file puts the file pointer one
 *       byte past eof. 
 *
 *       All setting up such as drive_enter and drobj_enter should have
 *       been done before calling here. 
 *
 * Note: when this routine is caled the file's finode is LOCKED so
 *       the code needn't be reentrant relative to the finode. 
 *
 * Called By:
 *       po_lseek and po_truncate.
 *
 * Entries:
 *       PC_FILE *pfile
 *       ULONG offset
 *       INT16 origin
 *       INT16 *err_flag
 *
 * Returns
 *       Returns the new offset or -1 on error.
 *
 *       If the return value is -1 fs_user->p_errno will be set
 *       with one of the following:
 *               PENBADF     - File descriptor invalid
 *               PEINVAL     - Seek to negative file pointer attempted.
 *
 ******************************************************************************/
static ULONG _po_lseek(PC_FILE *pfile, INT32 offset, INT16 origin, INT16 *err_flag) /*__fn__*/
{
    DDRIVE  *pdrive;
    LONG    file_pointer;
    LONG    ret_val;
    ULONG   alloced_size;
    ULONG   ltemp;
    ULONG   ltemp2;
    UINT32  first_cluster;
    SDBOOL  past_file;
    COUNT   log2_bytespcluster;
    UCOUNT  n_clusters_to_seek;
    UCOUNT  n_clusters;
    INT16   p_errno;

    p_errno = NO;    

    pdrive = pfile->pobj->pdrive;

    /* If file is zero size'd. were there */
    if ( !(pfile->pobj->finode->fsize) )
    {
        ret_val = 0L;
        return (ret_val);
    }

    if ( origin == PSEEK_SET )      /*  offset from begining of file */
        file_pointer = offset;
    else if ( origin == PSEEK_CUR ) /* offset from current file pointer */
    {
        file_pointer = (LONG) pfile->fptr;
        file_pointer += offset;
    }
    else if ( origin == PSEEK_END ) /*  offset from end of file */
    {
        file_pointer = (LONG) pfile->pobj->finode->fsize;
        file_pointer += offset;
    }
    else /* Illegal origin */
    {
        file_pointer = -1L;
    }

    if ( file_pointer < 0L )
    {
        goto errex;
    }

    if ( file_pointer >= (LONG)pfile->pobj->finode->fsize )
    {
        file_pointer = (LONG)pfile->pobj->finode->fsize;
        past_file = YES;
    }
    else
        past_file = NO;

    log2_bytespcluster = (COUNT) (pdrive->log2_secpalloc + 9);

    /* How many clusters do we need to seek */
    /* use the current cluster as the starting point if we can */
    if ( file_pointer >= (LONG)pfile->fptr )
    {
        first_cluster = pfile->fptr_cluster;

        ltemp =  file_pointer;
        if ( past_file )
        {
            /* If seeking past eof reduce the seek count
               by one since we can't really seek past EOF */
            ltemp -= 1;
        }
        ltemp >>= log2_bytespcluster;
        ltemp2 = pfile->fptr  >> log2_bytespcluster;

        /*
          If this algorithm was run twice n_clusters_to_seek would
          end up -1. Which would still work but spin 64K times.
          Thanks top Morgan Woodson of EMU systems for the patch.
        */
        if ( ltemp >= ltemp2 )
            n_clusters_to_seek = (UCOUNT) (ltemp - ltemp2);
        else
            n_clusters_to_seek = (UCOUNT) 0;
    }
    else
    {
        /* seek from the beginning */
        first_cluster = (UINT32)pfile->pobj->finode->fclusterHi;
        first_cluster <<= 16;
        first_cluster |= (UINT32)pfile->pobj->finode->fcluster;
        ltemp = file_pointer >> log2_bytespcluster;
        n_clusters_to_seek = (UCOUNT) ltemp;
    }

    while ( n_clusters_to_seek )
    {
        PC_FAT_ENTER(pdrive->driveno) ;
        n_clusters = pc_get_chain(pdrive,
            first_cluster,
            &first_cluster,
            n_clusters_to_seek);
        PC_FAT_EXIT(pdrive->driveno);
            

        if ( !n_clusters )
        {
            goto errex;
        }

        n_clusters_to_seek = (UCOUNT) (n_clusters_to_seek - n_clusters);
    }

    pfile->fptr_cluster = first_cluster;
    pfile->fptr_block = pc_cl2sector(pdrive, first_cluster);
    pfile->fptr= file_pointer;

    /*
      If seeking to the end of file see if we are beyond the allocated size of
      the file. If we are we set the at_eof flag so we know to try to move the
      cluster pointer in case another file instance extends the file.
    */
    pfile->at_eof = NO;
    if ( past_file )
    {
        /* Round the file size up to its cluster size by adding
           in clustersize-1 and masking off the low bits.
        */
        alloced_size =  (pfile->pobj->finode->fsize +
            pdrive->byte_into_cl_mask) & ~(pdrive->byte_into_cl_mask);

        /* If the file pointer is beyond the space allocated to the
           file note it since we may need to adjust this file's cluster
           and block pointers later if someone else extends the file
           behind our back.
        */
        if ( pfile->fptr >= alloced_size )
            pfile->at_eof = YES;
    }

    ret_val = pfile->fptr;
    *err_flag = 0;

    /* No only errors return through here. Everything does. */
    errno = p_errno;

    return (ret_val);
    errex:

    p_errno = PEINVAL;
    *err_flag = 1;
    ret_val = -1L;
    /* No only errors return through here. Everything does. */
    errno = p_errno;

    return (ret_val);
}


#if (RTFS_WRITE)
/*****************************************************************************
 * Name: _po_flush
 *
 * Description:
 *       Note:   when this routine is caled the file's finode is LOCKED
 *       so the code needn't be reentrant relative to the finode. 
 *
 *       Internal version of po_flush() called by po_flush and po_close.
 *
 * Entries:
 *       PC_FILE *pfile
 *
 * Returns:
 *       YES if successful
 *       NO if failure
 *
 ******************************************************************************/
// referenced by filesrcv.c
SDBOOL _po_flush(PC_FILE *pfile) /*__fn__*/
{
    SDBOOL ret_val;

    ret_val = YES;

    /* Convert to native. Overwrite the existing inode.Set archive/date */
    if ( pfile->needs_flush )
    {
        if ( pc_update_inode(pfile->pobj, YES, pfile->setDate) )
            /*                if ( pc_update_inode(pfile->pobj, YES, YES) ) */
        {
            pfile->needs_flush = NO;

            /* Flush the file allocation table */
            PC_FAT_ENTER(pfile->pobj->pdrive->driveno)
                if ( !pc_flushfat(pfile->pobj->pdrive->driveno) )
                    ret_val = NO;
            PC_FAT_EXIT(pfile->pobj->pdrive->driveno)
                }
    }

    return (ret_val);
}

#endif

#if (RTFS_WRITE)
/******************************************************************************
 * Name: PO_EXTEND_FILE  - Extend a file by N contiguous clusters.
 *                  
 * Description:
 *       Given a file descriptor, n_clusters clusters and method, extend
 *       the file and update the file size.
 *                   
 *       Method may be one of the following:
 *            PC_FIRST_FIT  - The first free chain >= n_clusters is alloced
 *            PC_BEST_FIT - The smallest chain    >= n_clusters is alloced
 *            PC_WORST_FIT  - The largest chain   >= n_clusters is alloced
 *                   
 * Note: PC_FIRST_FIT is significantly faster than the others
 *                                      
 * See: pc_find_contig_clusters()
 *
 * Entries:
 *       PCFD    fd
 *       UCOUNT  n_clusters
 *       COUNT   method
 *       SDBOOL    preerase_region
 *
 * Returns
 *       0xFFFF if an error occured.
 *       Returns n_clusters if the file was extended. Otherwise it returns
 *       the largest free chain available. If it n_clusters is not returned
 *       the files was not extended.
 *                   
 *       If the return value is not n_clusters, fs_user->p_errno will
 *       be set with one of the following:
 *               PENBADF     - File descriptor invalid or open read only
 *               PENOSPC     - IO failure
 *                   
 ******************************************************************************/
UCOUNT po_extend_file(PCFD fd, UCOUNT n_clusters, COUNT method, SDBOOL preerase_region) /* __fn__ */
{
    PC_FILE *pfile;
    DDRIVE  *pdr;
    ULONG   ltemp;
    UINT32  clno;
    UINT32  largest_chain;
    UINT32  first_cluster;
    UINT32  last_cluster_in_chain;
    UINT32  n_alloced;
    UCOUNT  ret_val;
    UCOUNT  i;

    CHECK_MEM(UCOUNT, ~0);   /* Make sure memory is initted */
    PC_FS_ENTER();           /* Must be last line in declarations */
    CHECK_USER(UCOUNT, ~0);  /* Check if a valid user if multitasking */

    if ( !n_clusters )
    {
        errno = 0;
        ret_val = 0;
        goto return_error;
    }

    /* Assume error to start */
    errno = PENOSPC;
    ret_val = 0xFFFF;

    /* Get the FILE. Second argument is ignored */
    pfile = pc_fd2file(fd, PO_WRONLY|PO_RDWR);

    /* Make sure we have write privilages. Make sure we got a  count */
    if ( !pfile || !n_clusters )
    {
        errno = PEBADF;
        goto return_error;
    }


    /* From here on we exit through alloc_done so we will unlock these resources */
    pdr = pfile->pobj->pdrive;

    PC_DRIVE_ENTER(pdr->driveno, NO);
    PC_INODE_ENTER(pfile->pobj->finode, YES); /* Exclusive */
    PC_FAT_ENTER(pdr->driveno);
    /* Make sure our file pointer is ok */
    _synch_file_ptrs(pfile);

    /* Find the end of the file's chain */
    last_cluster_in_chain = 0;  
    clno = pfile->fptr_cluster;
    while ( clno )
    {
        last_cluster_in_chain = clno;
        clno = pc_clnext(pdr, clno);
    }

    /*
      Now allocate clusters. To find the free space we look in three
      regions until we find space:
      1. we look from the last cluster in the file to the end of the fat
      (skip 1 if there is no chain)
      2. we look from the beginning of the data area to the end of the fat
      3. we look from the beginning of the fat area to the end of the fat
    */

    n_alloced   = 0;
    largest_chain = 0;
    clno = last_cluster_in_chain;
    if ( !clno )
        clno = pdr->free_contig_base;

    while ( clno )
    {
        n_alloced =  pc_find_contig_clusters(pdr,
            clno,
            &first_cluster,
            n_clusters,
            method);
        if ( n_alloced == 0x0FFFFFFF )
            goto alloc_done;
        else if ( n_alloced >= (UINT32)n_clusters )
            break; /* We got our chain */
        else
        {
            /* We didn't get enough space. keep track of
               the biggest chain.
               Don't need to store first_cluster since we
               won't alocate chains smaller than what we need.
            */
            if ( largest_chain < n_alloced )  
                largest_chain = n_alloced;
        }

        /*
          If we were searching between from the end of the  file
          and end of fat look from the beginning of the file data area
        */
        if ( clno == last_cluster_in_chain )
            clno = pdr->free_contig_base;
        /*
          If we were searching between the beginning of the file
          data area and end of fat  look from the fat.
        */
        else if ( clno == first_cluster )
        {
            clno = 2;
            break;
        }
        else /* We've looked everywhere. No luck */
            break;
    }

    if ( n_alloced < (UINT32)n_clusters )
    {
        /*
          We didn't get what we asked for so we return the
          biggest free contiguous chain.
        */
        if ( largest_chain <= first_cluster )
            ret_val = (UINT16)largest_chain;
        else
            ret_val = (UINT16)(largest_chain - first_cluster);

        goto alloc_done;
    }
    /* else */

    /* We found a large enough contiguous group of clusters */
    /* Turn them into a chain */
    clno = first_cluster;
    for (i = 0; i < (n_clusters-1); i++, clno++)
    {
        /* Link the current cluster to the next one */
        if ( !pc_pfaxx(pdr, clno, (clno+1L)) )
            goto alloc_done;
    }

    /* Terminate the list */
    if ( !pc_pfaxx(pdr, clno, 0x0FFFFFFFL) )
        goto alloc_done;

    if ( last_cluster_in_chain )
    {
        /* The file already has clusters in it. Append our new chain */
        if ( !pc_pfaxx(pdr, last_cluster_in_chain, first_cluster) )
            goto alloc_done;
    }
    else
    {
        /* Put our chain into the directory entry */
        /* Check for FAT32 */
        pfile->pobj->finode->fcluster = (UINT16)(first_cluster & 0x0FFFF);
        pfile->pobj->finode->fclusterHi = (UINT16)(first_cluster >> 16);
        pfile->pobj->finode->lastAccess = 0;

        /* Use synch_pointers to set our file pointers up */
        pfile->fptr_cluster = 0; /* This is already true but... */
        pfile->fptr_block = 0;
        pfile->fptr = 0;
    }

    /* Now recalculate the file size */
    ltemp = n_clusters;
    ltemp <<= (pdr->log2_secpalloc + 9);
    pfile->pobj->finode->fsize += ltemp;

    /* call synch to take care of both the eof condition and the case
       where we just alloced the beginning of the chain */
    _synch_file_ptrs(pfile);

    /* Flush the fat */
    if ( !pc_flushfat(pdr->driveno) )
        goto alloc_done;

    /* Write the directory entry. Set archive & date */
    if ( !pc_update_inode(pfile->pobj, YES, YES) )
        goto alloc_done;

    /* It worked !  Set the return to the number of clusters requested */
    /* Now erase the sectors */
    /*#if (PREERASE_ON_ALLOC)*/
    if ( preerase_region )
    {
        clno = first_cluster;
        for (i = 0; i < n_clusters; i++, clno++)
        {
            ltemp = pc_cl2sector(pdr, clno);
            devio_erase(pdr->driveno, ltemp, pdr->secpalloc);
        }
    }
    /*#endif*/


    ret_val = (UINT16)n_clusters;
    errno = 0;

    /*
      All code exits through here. ret_val determines if the function was
      successful. If 0xffff it's an error. If n_clusters it's a success and
      the file is expanded. Otherwise the return value.
    */
    alloc_done:
    PC_FAT_EXIT(pdr->driveno) ;
    PC_INODE_EXIT(pfile->pobj->finode);
    PC_DRIVE_EXIT(pdr->driveno);
    

    return_error:

    /* Restore the kernel state */
    PC_FS_EXIT();
    

    return (ret_val);
}
#endif  /* (RTFS_WRITE) */


#if (RTFS_WRITE)
/******************************************************************************
 * Name:   PO_FLUSH  -  Flush a file.
 *                  
 * Description:
 *       Flush the file updating the disk.
 *
 * Entries:
 *       PCFD    fd
 *
 * Returns:
 *       Returns YES if all went well otherwise it returns NO.
 *       fs_user->p_errno is set to one of these values
 *               PENBADF - Invalid file descriptor
 *               PENOSPC - IO error occured
 *
 ******************************************************************************/
SDBOOL po_flush(PCFD fd) /*__fn__*/
{
    PC_FILE *pfile;
    SDBOOL  ret_val;
    INT16   p_errno;

    CHECK_MEM(SDBOOL, 0);      /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(SDBOOL, 0);     /* Check if a valid user if multitasking */

    /* Start by assuming failure */
    ret_val = NO;
    p_errno = PEBADF;   

    /* Get the FILE. must be open for write */
    pfile = pc_fd2file(fd, (PO_WRONLY|PO_RDWR));
    if ( pfile )
    {
        /* Register drive in use */
        PC_DRIVE_ENTER(pfile->pobj->pdrive->driveno, NO);
        

        /* Claim exclusive access on flush */
        PC_INODE_ENTER(pfile->pobj->finode, YES);
        

        ret_val = _po_flush(pfile);
        if ( !ret_val )
            p_errno = PENOSPC;  

        PC_INODE_EXIT(pfile->pobj->finode);
        

        PC_DRIVE_EXIT(pfile->pobj->pdrive->driveno);
    }

    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT();
    

    return (ret_val);
}
#endif  /* (RTFS_WRITE) */


/******************************************************************************
 * Name: PO_CLOSE  -  Close a file.
 *
 * Description:
 *       Close the file updating the disk and freeing all core
 *       associated with FD.
 *
 * Entries:
 *       PCFD fd
 *
 * Returns
 *       Returns 0 if all went well otherwise it returns -1.
 *       fs_user->p_errno is set to one of these values:
 *               PENBADF - Invalid file descriptor
 *               PENOSPC - IO error occured
 *
 ******************************************************************************/
INT16 po_close(PCFD fd) /*__fn__*/
{
    PC_FILE *pfile;
    INT16   ret_val;
    UINT16  p_errno;

    CHECK_MEM(INT16, -1);      /* Make sure memory is initted */
    PC_FS_ENTER();
    CHECK_USER(INT16, -1);     /* Check if a valid user if multitasking */

    ret_val = NO;

    if ( (pfile = pc_fd2file(fd, 0)) == SDNULL )
    {
        p_errno = PEBADF;
        ret_val = -1;
    }
    else
    {
        PC_DRIVE_ENTER(pfile->pobj->pdrive->driveno, NO);     /* Register drive in use */
        ret_val = NO;
        p_errno = NO;
#if (RTFS_WRITE)
        if ( pfile->flag & ( PO_RDWR | PO_WRONLY ) )
        {
            PC_INODE_ENTER(pfile->pobj->finode, YES);
            /* Update the file and FAT */
            if ( !_po_flush(pfile) )
            {
                p_errno = PENOSPC;  
                ret_val = -1;
            }
            PC_INODE_EXIT(pfile->pobj->finode);
        }
#endif
        /* Release the FD and its core */
        pc_freefile(fd);
        PC_DRIVE_EXIT(pfile->pobj->pdrive->driveno);
    }

    errno = p_errno;

    /* Restore the kernel state */
    PC_FS_EXIT();
    

    return(ret_val);
}

#endif    /* (USE_FILE_SYSTEM) */


