//........................................................................................
//........................................................................................
//.. Last Modified By: Todd Malsbary    toddm@iobjects.com                                ..    
//.. Modification date: 10/20/2000                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.                                      ..
//..     All rights reserved. This code may not be redistributed in source or linkable  ..
//..      object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                                ..
//........................................................................................
//........................................................................................
/******************************************************************************
 * Filename:    DROBJ.C - Directory object manipulation routines
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
 *       Directory object manipulation routines
 *
 * The following routines in this file are included:
 *
 *    pc_fndnode          -  Find a file or directory on disk and return a DROBJ.
 *    pc_get_inode        -  Find a filename within a subdirectory 
 *    static pc_findin    -  Find a filename in the same directory as argument.
 *
 *    pc_get_mom          -  Find the parent inode of a subdirectory.
 *
 *    pc_mkchild          -  Allocate a DROBJ and fill in based on parent object.
 *    pc_mknode           -  Create an empty subdirectory or file.
 *    pc_insert_inode     - Called only by pc_mknode
 *
 *    pc_rmnode           -  Delete an inode unconditionally.
 *    pc_update_inode     -  Flush an inode to disk
 *
 *    pc_read_obj         -  Assign an initialized BLKBUFF to a DROBJ
 *    pc_write_obj        -  Write a DROBJ's BLKBUFF to disk
 *
 *    pc_get_root         -  Create the special ROOT object for a drive.
 *    pc_firstblock       -  Return the absolute block number of a directory
 *    pc_next_block       -  Calculate the next block owned by an object.
 *    pc_l_next_block     -  Calculate the next block in a chain.
 *
 * We keep a shared image of directory entries around. These routines
 * keep track of them for us.
 *    pc_marki            -  Set dr:sec:index, + stitch FINODE into the inode list
 *    pc_scani            -  Search for an inode in the internal inode list.
 *
 * Heap management routines
 *    pc_allocobj         - 
 *    pc_alloci           - 
 *    pc_free_all_i       - 
 *    pc_freei            -       
 *    pc_freeobj          - 
 *
 * Simple helper routines
 *    pc_dos2inode        -  
 *    pc_ino2dos          -   
 *    pc_init_inode       -   
 *    pc_isadir           - 
 *
 ******************************************************************************/

#include "pcdisk.h"
#include "oem.h"

#if (USE_FILE_SYSTEM)

#ifndef NULL
#define NULL SDNULL
#endif

#if LITTLE_ENDIAN
#define SWAP_NAME_SIZE(pi, longSwitch)
#else
#if USE_HW_OPTION
#define SWAP_NAME_SIZE(pi,longSwitch)
#else
SDVOID name_size_swap(UINT16 *pi, UINT16 longSwitch);
#define SWAP_NAME_SIZE(pi, longSwitch) name_size_swap(pi, longSwitch)
#endif
#endif

#include <util/debug/debug.h>
DEBUG_MODULE(FAT_FS);
DEBUG_USE_MODULE(FAT_FS);



/*****************************************************************************
 * Name: getEntryDataChar16bit -  Get the entry from the disk.
 *
 * Description:
 *       Take an from the buffer and convert it to a local buffer
 *       for processing.  This is ONLY applied to the case of
 *       setting CHAR_16BIT option to 1. 
 *
 * Returns
 *       None.
 *
 ******************************************************************************/
#if (CHAR_16BIT)
SDVOID getEntryDataChar16bit(DOSINODE *pi, UINT16 *buff, UINT16 index);

SDVOID getEntryDataChar16bit(DOSINODE *pi, UINT16 *buff, UINT16 index)
{
    UINT16 fattribute;
#if (LITTLE_ENDIAN)
    fattribute = buff[(index<<4)+5];
    fattribute >>= 8;
#else
    fattribute = buff[(index<<4)+5] & 0x0FF;
#endif

    if (fattribute == CHICAGO_EXT)
        char_unpack_longinode((LONGINODE *)pi, buff,
            (index<<5));  /* unpack data to pi */
    else
        char_unpack_dosinode(pi, buff,
            (index<<5));  /* unpack data to pi */
}
#endif


/*****************************************************************************
 * Name: PC_FNDNODE -  Find a file or directory on disk and return a DROBJ.
 *
 * Description:
 *       Take a full path name and traverse the path until we get to the file
 *       or subdir at the end of the path spec. When found allocate and
 *       initialize (OPEN) a DROBJ.
 *
 * Returns
 *       Returns a pointer to a DROBJ if the file was found, otherwise NULL.
 *
 ******************************************************************************/
DROBJ *pc_fndnode(TEXT *path) /*__fn__*/
{
    DROBJ   *pobj;
    DROBJ   *pmom;
    DROBJ   *pchild;
    DDRIVE  *pdrive;
    TEXT    filename[10];
    TEXT    fileext[4];
    INT16   driveno;

    pmom = SDNULL;

    /* Get past D: plust get drive number if there */
    path = pc_parsedrive( &driveno, path );
    if ( !path )
        return (SDNULL);

    /* Find the drive */
    pdrive = pc_drno2dr(driveno);
    if ( !pdrive )
        return (SDNULL);

    /* Get the top of the current path */
    if ( (*path == (TEXT)char_backslash) || (*path == (TEXT)char_forwardslash) )
    {
        pobj = pc_get_root(pdrive);
        path++;
    }
    else
    {
        pobj = pc_get_cwd(pdrive);
    }

    if ( !pobj )
        return (SDNULL);

    /* Search through the path til exhausted */
    while ( *path )
    {
        path = pc_nibbleparse((TEXT *)filename,
            (TEXT *)fileext,
            path);
        if ( !path )
        {
            pc_freeobj(pobj);
            return (SDNULL);
        }

#if (RTFS_SUBDIRS)
        if (pc_isdot( filename, fileext ))
            ;
#endif
        else
        {
            /*
              Find Filename in pobj. and initialize lpobj with result.
            */
            PC_INODE_ENTER(pobj->finode, NO);
            pchild = pc_get_inode(SDNULL,
                pobj,
                filename,
                fileext);
            if ( !pchild )
            {
                PC_INODE_EXIT(pobj->finode);
                pc_freeobj(pobj);
                return (SDNULL);
            }
#if (RTFS_SUBDIRS)
            /*
              We found it. We have one special case. if "..". we
              need to shift up a level so we are not the child of
              mom but of grand mom.
            */
            if ( pc_isdotdot( filename, fileext ) )
            {
                /* We're done with pobj for now */
                pc_freeobj(pobj);

                /* Find pobj's parent. By looking back from ".." */
                pmom = pc_get_mom(pchild);
                PC_INODE_EXIT(pobj->finode);
                
                if ( !pmom )
                {
                    pc_freeobj(pchild);
                    return (SDNULL);
                }
                else
                {
                    /* We found the parent now free the child */
                    pobj = pmom;
                    pc_freeobj(pchild);
                }
            }
            else
#endif      /* SUBDIRS */
            {
                PC_INODE_EXIT(pobj->finode);
                
                /* We're done with pobj for now */
                pc_freeobj(pobj);

                /* Make sure pobj points at the next inode */
                pobj = pchild;
#if (RTFS_SUBDIRS)
#else
                /* No subdirectory support. Return the
                   one we found.
                */
                return (pobj);
#endif
            }
        }
    }

    return (pobj);
}


/******************************************************************************
 * Name: PC_GET_INODE -  Find a filename within a subdirectory 
 *
 * Description
 *       Search the directory pmom for the pattern or name in filename:ext and
 *       return the an initialized object. If pobj is NULL start the search at
 *       the top of pmom (getfirst) and allocate pobj before returning it. 
 *       Otherwise start the search at pobj (getnext). (see also pc_gfirst,
 *       pc_gnext)
 *
 *       Give a directory mom. And a file name and extension. 
 *       Find find the file or dir and initialize pobj.
 *       If pobj is NULL. We allocate and initialize the object otherwise
 *       we get the next item in the chain of directory entries.
 *
 * Note: Filename and ext must be right filled with spaces to 8 and 3 bytes
 *       respectively. Null termination does not matter.
 *
 * Entries:
 *       DROBJ *pobj
 *       DROBJ *pmom
 *       TEXT *filename
 *       TEXT *fileext
 *
 * Returns:
 *       Returns a drobj pointer or NULL if file not found.
 *
 ******************************************************************************/
DROBJ *pc_get_inode ( DROBJ *pobj, DROBJ *pmom, TEXT *filename, TEXT *fileext ) /*__fn__*/
{
    SDBOOL  starting = NO;
    INT16 ret_blk;

    /* Create the child if just starting */
    if ( !pobj )
    {
        starting = YES;
        pobj = pc_mkchild(pmom);
        if ( !pobj )
            return(SDNULL);
    }
    else /* If doing a gnext don't get stuck in and endless loop */
    {
        if ( ++(pobj->blkinfo.my_index) >= INOPBLOCK )
        {
            ret_blk = pc_next_block(pobj);
            if ( (ret_blk == 0) || (ret_blk == -1) )
            {
                // dc- consistency?
                //if( starting )
                //                    pc_freeobj(pobj);
                
                return (SDNULL);
            }
            else
                pobj->blkinfo.my_index = 0;
        }
    }

    /* Look for the name and assign information to POBJ. */
    if ( pc_findin(pobj, filename, fileext) )
    {
        return (pobj);
    }
    else
    {
        if ( starting )
            pc_freeobj(pobj);
        return (SDNULL);
    }
}

SDLOCAL SDBOOL Get_Next_Entry(DOSINODE **ppi, DROBJ *pobj, SDBOOL *updateRbuf);

/*****************************************************************************
 * Name: Get_Next_Entry -  Find a next filename in the same directory.
 *
 * Description:
 *       Look for the next match of filename or pattern filename:ext in the 
 *       subdirectory containing pobj. If found, update pobj to contain the 
 *       new information. Called by pc_findin().
 *
 * Note: Filename and ext must be right filled with spaces to 8 and 3 bytes
 *       respectively. Null termination does not matter.
 *
 * Returns:
 *       Returns YES if found or NO.
 *
 *       Find filename in the directory containing pobj. If found, load
 *       the inode section of pobj. If the inode is already in the inode
 *       buffers we free the current inode and stitch the existing one in,
 *       bumping its open count.
 *
 ******************************************************************************/
SDLOCAL SDBOOL Get_Next_Entry(DOSINODE **ppi, DROBJ *pobj, SDBOOL *updateRbuf) /*__fn__*/
{
    DOSINODE *pi;
    DIRBLK  *pd;
    BLKBUFF *rbuf;
    INT16   ret_blk;

    pi = *ppi;

    /* For convenience. We want to get at block info here. */
    pd = &pobj->blkinfo;
    rbuf = pobj->pblkbuff;

    pd->my_index++;

    if ( pd->my_index >= INOPBLOCK )
    {

        
        pc_verify_blk(pobj->pblkbuff); // temp

        /* Not in that block. Try again */
        pc_free_buf(rbuf, NO, YES);

        /* Update the objects block pointer */
        ret_blk = pc_next_block(pobj);
        if ( (ret_blk == 0) || (ret_blk == -1) )
            return (NO);

        pd->my_index = 0;

        pobj->pblkbuff = rbuf = pc_read_blk(pobj->pdrive, pobj->blkinfo.my_block);
        *updateRbuf = YES;
    }

    /* Swap data for processing */
    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)), 0);

#if (CHAR_16BIT)
    /* Unpack data to local buffer for processing. */
    getEntryDataChar16bit(pi, (UINT16 *)rbuf->data, pd->my_index);
#else
    if ( *updateRbuf )
        pi = (DOSINODE *)rbuf->data;
    else
        pi++;

    *ppi = pi;
#endif

    pc_verify_blk(pobj->pblkbuff); // temp

    return (YES);
}

/*****************************************************************************
 * Name: name_size_swap -  Swap a filename in the same directory.
 *
 * Description:
 *
 *       In a long file name case,  the entire entry will be swapped.
 *       In a short name case, only the name and the size are swapped.
 *
 *****************************************************************************/
#if LITTLE_ENDIAN
#else
#if USE_HW_OPTION
#else
SDVOID name_size_swap(UINT16 *pi, UINT16 longSwitch)
{
    UINT16 *ppi;
    INT16   i;               

    /* Get the attribute */
    ppi = pi;
    i = (ppi[5] >> 8);

    /* Since this routine is to swap data both ways,  take care the
       condition when the information is already swapped and we need
       to swap it back.
    */
    if (longSwitch)
        i = (ppi[5] & 0xFF);

    /* Check for long file name. */
    if ( i == CHICAGO_EXT )
    {
        /* Swap the directory or file entry information. This
           entry is 32-byte long.
        */
        for (i = 0; i < 16; i++)
            ppi[i] = to_WORD((UCHAR *)&ppi[i]);
    }
    else
    {
        /* Short name only. Need to swap the name. */
        for (i = 0; i < 6; i++)
            ppi[i] = to_WORD((UCHAR *)&ppi[i]);
#if (CHAR_16BIT)
#else
        /* Swap the size */
        i = ppi[15];
        ppi[15] = ppi[14];
        ppi[14] = i;
#endif
    }
}
#endif
#endif


/*****************************************************************************
 * Name: PC_FINDIN -  Find a filename in the same directory as the argument.
 *
 * Description:
 *       Look for the next match of filename or pattern filename:ext in the 
 *       subdirectory containing pobj. If found update pobj to contain the 
 *       new information  (essentially getnext.) Called by pc_get_inode().
 *
 * Note: Filename and ext must be right filled with spaces to 8 and 3 bytes
 *       respectively. Null termination does not matter.
 *
 * Returns:
 *       Returns YES if found or NO.
 *
 *       Find filename in the directory containing pobj. If found, load
 *       the inode section of pobj. If the inode is already in the inode
 *       buffers we free the current inode and stitch the existing one in,
 *       bumping its open count.
 *
 ******************************************************************************/
SDBOOL pc_findin(DROBJ *pobj, TEXT *filename, TEXT *fileext) /*__fn__*/
{
    BLKBUFF *rbuf;
    DIRBLK  *pd;
    FINODE  *pfi;
    DOSINODE *pi;
    TEXT    *fNameExt;
    SDWCHAR *wfNameExt;
    INT16   ret_blk;
    INT16   i;
    INT16   index;
    INT16   fNameExtLen;
    INT16   wfNameExtLen;
    SDBOOL  retval;
    SDBOOL updateRbuf;
#if (CHAR_16BIT)
    LONGINODE pi_char16;
#endif
    
    fNameExt = (TEXT *)currFileName;
    fNameExt[0] = 0;
    fNameExt[1] = 0;
    wfNameExt = (SDWCHAR *)unicodeFileName;
    fNameExt[0] = 0;
    fNameExt[1] = 0; // for kicks
    retval = NO;
    updateRbuf = NO;
    
    /* For convenience. We want to get at block info here */
    pd = &pobj->blkinfo;
    /* Read the data */
    pobj->pblkbuff = rbuf = pc_read_blk(pobj->pdrive, pobj->blkinfo.my_block);
    
    while ( rbuf )
    {
        pc_verify_blk(rbuf); // temp

        /* Swap the entry information */
        SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)), 0);
        
#if (CHAR_16BIT)
        /* Get local buffer */
        pi = (DOSINODE *)&pi_char16;
        getEntryDataChar16bit(pi, 
            (UINT16 *)rbuf->data,
            pd->my_index);  /* unpack to pi_char16 */
#else
        pi = (DOSINODE *) rbuf->data;
        /* Look at the current inode */
        if( pd->my_index > 15 ) {
            diag_printf("pd->my_index = %d (block %d)\n", pd->my_index, pobj->blkinfo.my_block );
        }
        pi += pd->my_index;
#endif

        /* And look for a match */
        while ( pd->my_index < INOPBLOCK )
        {
            pc_verify_blk(pobj->pblkbuff); // temp
            goto START_ANOTHER_LONG_ENTRY;  // satisfy compiler
START_ANOTHER_LONG_ENTRY:
            /* Check for end of directory if name is NULL */
            if ( !pi->fname[0] )
            {
                pc_free_buf(rbuf, NO, YES);
                return (NO);
            }
        
            *(TEXT *)(fNameExt) = 0;
            *(SDWCHAR *)(wfNameExt) = 0;
            fNameExtLen = 0;
            wfNameExtLen = 0;        
        
            if ( pi->fattribute == CHICAGO_EXT )
            {
            
                /* Calculate the index number for long file name */
                index = (INT16)(((LONGINODE *)pi)->sequence);
                if ( (index != PCDELETE) && (index & LSEQUENCE_END))
                {
                    index = (INT16)((index & 0x3F) - 1);
                }
                else
                    index = -1;

                // safety first, gracefully handle disk corruption
                // dc- (this needs clarification)
                // the 'index' value here is the index within the directory entry chain used for
                // storing the long filename. if the value is > 20 then the filename is > 260 chars, which
                // is not possible, so this is treated as a corrupt chain   
                if(index > 20)
                {
                    diag_printf("corrupt dir entry (block %d, val %d)\n", pobj->blkinfo.my_block, index);
                    //                    index = -1;
                    goto NEXT_DIRENTRY_SEARCH;
                }
            
                /* Get the long file name */
                for (i = index; i >= 0; i--)
                { 
                
                    /* Save the long file name */
                    fNameExtLen += copyLongFileName(i, 5, (UTEXT *)fNameExt,
                        (UTEXT *)((LONGINODE *)pi)->fname, 0);
                    fNameExtLen += copyLongFileName(i, 6, (UTEXT *)fNameExt,
                        (UTEXT *)((LONGINODE *)pi)->fname2, 5);
                    fNameExtLen += copyLongFileName(i, 2, (UTEXT *)fNameExt,
                        (UTEXT *)((LONGINODE *)pi)->fname3, 11);
                                
                
#ifdef _UNICODE_FS_
#error do not build this
                    // store the unicode filename in a separate location
                    wfNameExtLen += wcopyLongFileName(i, 5, (SDWCHAR *)wfNameExt, 
                        (UTEXT *)((LONGINODE *)pi)->fname, 0);
                    wfNameExtLen += wcopyLongFileName(i, 6, (SDWCHAR *)wfNameExt, 
                        (UTEXT *)((LONGINODE *)pi)->fname2, 5);
                    wfNameExtLen += wcopyLongFileName(i, 2, (SDWCHAR *)wfNameExt, 
                        (UTEXT *)((LONGINODE *)pi)->fname3, 11);
#endif // #ifdef _UNICODE_FS_
                
                    /* Swap the entry back */
                
            
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)),
                        ((pi->fattribute == CHICAGO_EXT)? YES: NO));
                
                    updateRbuf = NO;
                    if (!Get_Next_Entry(&pi, pobj, &updateRbuf))
                        return (NO);
                
                    /* Check for new buffer */
                    if ( updateRbuf )
                    {       /* Update buffer pointer */
                        rbuf = pobj->pblkbuff;
                    }

                    pc_verify_blk(NULL); // temp
                }

            
            
                index = (INT16)(((LONGINODE *)pi)->sequence);
                i = (INT16)pi->fattribute;
            
                *(((TEXT *)fNameExt) + fNameExtLen) = 0x00;
                *(((SDWCHAR *)wfNameExt) + wfNameExtLen) = 0x0000;



                /* Check for long file name entry without DOS entry
                   for a legal file name.
                */

                if ( (index != PCDELETE) && (i == CHICAGO_EXT) )
                {
                    /* At this entry the name should be a DOS
                       Alias name but it is NOT. */
                    goto NEXT_DIRENTRY_SEARCH;
                }
            
                if ( (filename[0] != '*') || (fileext[0] != '*') )
                {
                    /* Verify the file name */
                    if ( (pc_patcmp((TEXT *)pi->fname, filename, 8)) &&
                        (pc_patcmp((TEXT *)pi->fext, fileext, 3)) )
                    {
                        if (longFileName != SDNULL)
                        {
                            if (!pc_patcmplong((TEXT *)longFileName, (TEXT *)fNameExt))
                            {
                                convert_alias_name(filename);
                                goto NEXT_DIRENTRY_SEARCH;
                            }
                        }
                    }
                    else
                    {
                        if (pi->fname[0] == PCDELETE || pi->fname[0] == '.')
                            goto NEXT_DIRENTRY_SEARCH;
                    
                        if ( longFileName != SDNULL )
                        {
                            if (pc_patcmplong((TEXT *)longFileName, (TEXT *)fNameExt))
                            {
                                copybuff((SDVOID *)filename, (SDVOID *)pi->fname, (8 * sizeof(UTEXT)));
                                copybuff((SDVOID *)fileext, (SDVOID *)pi->fext, (4 * sizeof(UTEXT)));
                            }
                            else
                            {
                                /* Verify the extension to change the DOS name if matched. */
                                if (pc_patcmp((TEXT *)pi->fext, fileext, 3))
                                {
                                    /* Find out if there is another one. */
                                    if ( pc_patcmpupto8ordot((TEXT *)longFileName, (TEXT *)fNameExt))
                                    {       /* Change to new name */
                                        if (pc_higherAlias (filename, (TEXT *)pi->fname))
                                        {
                                            convert_alias_name(filename);
                                            goto NEXT_DIRENTRY_SEARCH;
                                        }                                    
                                    }
                            
                                }
                            
                            }
                            
                        }
                        
                    }

                }

            }
        
            pc_verify_blk(pobj->pblkbuff); // temp

            /* Check for the file name */
            if ( (pc_patcmp((TEXT *)pi->fname, filename, 8)) &&
                (pc_patcmp((TEXT *)pi->fext, fileext, 3)) )
            {       /* Look for a real DOS entry */
                if ( pi->fattribute != CHICAGO_EXT ) /* Ignore chicago style names */
                {
                    /* We found it */
                    /*
                      See if it already exists in the inode list.
                      If so.. we use the copy from the inode list.
                    */
                
                    pfi = pc_scani(pobj->pdrive,
                        rbuf->blockno,
                        pd->my_index);
                
                    if ( pfi )
                    {
                        pc_freei(pobj->finode);
                        pobj->finode = pfi;
                        retval = YES;
                    }
                    else
                    {       /* No inode in the inode list. Copy the data over
                               and mark where it came from.
                            */
                        pfi = pc_alloci();
                        if ( pfi )
                        {
                            pc_freei(pobj->finode); /* Release the current */
                            pobj->finode = pfi;
                            pc_dos2inode(pobj->finode, pi );
                            pc_marki(pobj->finode,
                                pobj->pdrive,
                                pd->my_block,
                                pd->my_index );
                            retval = YES;
                        }
                    }
                
                    /* Got it,  swap the buffer information back. */
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)),
                        ((pi->fattribute == CHICAGO_EXT)? YES: NO));
                
                    /* Free, no error */
                    pc_free_buf(rbuf, NO, YES);
                    return (retval);
                } /* if (match) */
            }

            pc_verify_blk(NULL); // temp
        
NEXT_DIRENTRY_SEARCH:
            /* Swap the current entry back */
            SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)),
                ((pi->fattribute == CHICAGO_EXT)? YES: NO));
        
            pd->my_index++;
        
            /* Is it the last entry in the buffer? */
            if ( pd->my_index == INOPBLOCK )
                break;
        
            /* Take care of the new entry */
            SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (pd->my_index<<5)), 0);
        
#if (CHAR_16BIT)
            getEntryDataChar16bit(pi, 
                (UINT16 *)rbuf->data,
                pd->my_index);  /* unpack to pi_char16 */
#else
            pi++;
#endif
        }
        
        /* Not in that block. Try again */
        pc_free_buf(rbuf, NO, YES);
        
        /* Update the objects block pointer */
        ret_blk = pc_next_block(pobj);
        if ( (ret_blk == 0) || (ret_blk == -1) )
            break;
        
        /* Get new block of data */
        pd->my_index = 0;    

        pobj->pblkbuff = rbuf = pc_read_blk(pobj->pdrive,
            pobj->blkinfo.my_block);
        
    }
    
    pc_verify_blk(NULL); // temp
    return (NO);
}


#if (RTFS_SUBDIRS)
/*****************************************************************************
 * Name: PC_GET_MOM -  Find the parent inode of a subdirectory.
 *
 * Description:
 *       Given a DROBJ initialized with the contents of a subdirectory's ".."
 *       entry, initialize a DROBJ which is the parent of the current directory.
 *
 * Entries:
 *
 * Returns:
 *    Returns a DROBJ pointer or NULL if could something went wrong.
 *
 *
 * Get mom:
 *       if (!dotodot->cluster)  Mom is root. 
 *               getroot()
 *       else                    cluster points to mom.
 *               find .. in mom
 *       then search through the directory pointed to by moms .. until
 *       you find mom. This will be current block startblock etc for mom.
 *
 ******************************************************************************/
DROBJ *pc_get_mom ( DROBJ *pdotdot ) /*__fn__*/
{
    DROBJ   *pmom;
    DDRIVE  *pdrive;
    BLKBUFF *rbuf;
    DIRBLK  *pd;
    DOSINODE *pi;
    FINODE  *pfi;
    BLOCKT  sectorno;
#if (CHAR_16BIT)
    LONGINODE pi_char16;
#endif


    pdrive = pdotdot->pdrive;

    /* We have to be a subdir */
    if ( !pc_isadir(pdotdot) )
        return (SDNULL);

    /* For all FAT types */
    sectorno = (BLOCKT)pdotdot->finode->fclusterHi;
    sectorno <<= 16;
    sectorno |= (BLOCKT)pdotdot->finode->fcluster;

    /* If ..->cluster is zero then parent is root */
    if ( !sectorno )
        return (pc_get_root(pdrive));

    /* Otherwise: cluster points to the beginning of our parent. we
       also need the position of our parent in it's parent.
    */

    pmom = pc_allocobj();
    if ( !pmom )
        return (SDNULL);

    /* Find .. in our parent's directory */
    sectorno = pc_cl2sector(pdrive, sectorno); 

    /* We found .. in our parents dir. */
    pmom->pdrive = pdrive;
    pmom->blkinfo.my_frstblock =  sectorno; 
    pmom->blkinfo.my_block  =  sectorno;
    pmom->blkinfo.my_index  =  0;
    pmom->isroot = NO;
    pd = &pmom->blkinfo;


    pmom->pblkbuff = rbuf = pc_read_blk(pdrive, pmom->blkinfo.my_block);
    if ( rbuf )
    {
        PC_ENTER_CRITICAL();

        /* Swap the entry information */
        SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (0<<5)), 0);

#if (CHAR_16BIT)
        pi = (DOSINODE *)&pi_char16;
        getEntryDataChar16bit(pi, (UINT16 *)rbuf->data, 0);  /* unpack to pi_char16 */
#else
        pi = (DOSINODE *) rbuf->data;
#endif
        pc_dos2inode(pmom->finode, pi );

        /* Swap the entry information back */
        SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)rbuf->data) + (0<<5)),
            ((pi->fattribute == CHICAGO_EXT)? YES: NO));

        pc_free_buf(rbuf, NO, NO);

        PC_EXIT_CRITICAL();
        
        /* See if the inode is in the buffers */
        pfi = pc_scani(pdrive, sectorno, 0);
        if ( pfi )
        {
            pc_freei(pmom->finode);
            pmom->finode = pfi;
        }
        else
        {
            pc_marki(pmom->finode,
                pmom->pdrive,
                pd->my_block,
                pd->my_index);
        }
        return (pmom);

    }
    else /* Error, something didn't work */
    {
        pc_freeobj(pmom);
        return (SDNULL);
    }   
}
#endif  /* (RTFS_SUBDIRS) */



/*****************************************************************************
 * Name: PC_MKCHILD -  Allocate a DROBJ and fill in based on parent object.
 *
 * Description:
 *       Allocate an object and fill in as much of the the block pointer
 *       section as possible based on the parent.
 *
 * Entries:
 *       DROBJ *pmom
 *
 * Returns:
 *       Returns a partially initialized DROBJ if enough core available and 
 *       pmom was a valid subdirectory.
 *
 ******************************************************************************/
DROBJ *pc_mkchild ( DROBJ *pmom ) /*__fn__*/
{
    DROBJ   *pobj;
    DIRBLK  *pd;

    /* Mom must be a directory */
    if ( !pc_isadir(pmom) )
        return(SDNULL);

    /* init the object - */
    pobj = pc_allocobj();
    if ( !pobj )
        return (SDNULL);

    pd = &pobj->blkinfo;

    pobj->isroot = NO;            /* Child can not be root */
    pobj->pdrive =  pmom->pdrive; /* Child inherets moms drive */

    /* Now initialize the fields storing where the child inode lives */
    pd->my_index = 0;
    pd->my_block = pd->my_frstblock = pc_firstblock(pmom);
    if ( !pd->my_block )
    {
        pc_freeobj(pobj);
        return (SDNULL);
    }

    return (pobj);
}


#if (RTFS_WRITE)
/*****************************************************************************
 * Name: PC_MKNODE -  Create an empty subdirectory or file.
 *
 * Description
 *       Creates a file or subdirectory ("inode") depending on the flag
 *       values in attributes. A pointer to an inode is returned for
 *       further processing.
 *       Make a node from path and attribs create and fill in pobj.
 *
 * Note: The parent directory is locked before this routine is called.
 *       After processing, the DROBJ must be released by calling pc_freeobj.
 *
 * Entries:
 *       DROBJ *pmom
 *       TEXT *filename
 *       TEXT *fileext
 *       UTINY attributes
 *
 * Returns:
 *       Returns a pointer to a DROBJ structure for further use, or NULL
 *       if the inode name already exists or path not found.
 *
 * See po_open(),po_close(), pc_mkdir() et al for examples.
 ******************************************************************************/
DROBJ *pc_mknode ( DROBJ *pmom, TEXT *filename, TEXT *fileext, DOSINODE *pNode, UINT16 attributes, UINT16 dirUpdate) /*__fn__*/
{
    DROBJ   *pobj;
    BLKBUFF *pbuff;
    DDRIVE  *pdrive;
    FINODE  *lfinode, finode;
    DOSINODE *pdinodes;
    UINT32  cluster;
    UINT32  clbase;
    SDBOOL  ret_val;
    DATESTR crdate;
    UINT16  attr;


    pobj = SDNULL;
    ret_val = YES;
    clbase = 0L;

    if ( !pmom )
        return (SDNULL);

    pdrive = pmom->pdrive;
    if ( !pdrive )
        return (SDNULL);

    if( dirUpdate && !pNode ) {
        DEBUG( FAT_FS, DBGLEV_ERROR, "potentially fatal null pointer references\n");
    }
    
    cluster = 0L;

    /* Allocate memory for the node */
    //dc- what on earth are these people thinking
    //    lfinode = (FINODE *)fspath;
    lfinode = &finode;

#if (RTFS_SUBDIRS)
    if ( attributes & ADIRENT )
    {
        attributes &= 0xDF;     /* Remove ARCHIVE attribute */

        /*
          Grab a new cluster and clear it. We optimize here (10/27/90).
          To minimize fragmentation we give clalloc a hint where to 
          start looking for new clusters.
        */
        if ( !pmom->isroot )
        {
            /* For all FAT types including FAT32 */
            clbase = (UINT32)pmom->finode->fclusterHi;
            clbase <<= 16;
            clbase |= (UINT32)pmom->finode->fcluster;
        }

        /* Grab a cluster for a new dir and clear it */
        if ( dirUpdate )
        {
            cluster = ((UINT32)pNode->fclusterHi);
            cluster <<= 16;
            cluster |= ((UINT32)pNode->fcluster);
        }
        else
        {
            PC_FAT_ENTER(pdrive->driveno);
            cluster = pc_clalloc(pdrive, clbase);
            PC_FAT_EXIT(pdrive->driveno) ;
            
            if ( !cluster )
                ret_val = NO;
            else if ( !pc_clzero( pdrive, cluster ) )
            {
                PC_FAT_ENTER(pdrive->driveno);
                pc_clrelease(pdrive, cluster);
                PC_FAT_EXIT(pdrive->driveno) ;
                ret_val = NO;
            }
            // flag this cluster as allocated for directory entries
            //            push_clalloc( cluster );
        }
    }
#endif

    if ( !ret_val )
    {
        return (SDNULL);
    }

    /*
      For a subdirectory. First make it a simple file. We will change
      the attribute after all is clean.
    */
    attr = (UINT16)attributes;

#if (RTFS_SUBDIRS)
    if ( attr & ADIRENT )
        attr = ANORMAL;
#endif

    /* Allocate an empty DROBJ and FINODE to hold the new file. */
    pobj = pc_allocobj();
    if ( !pobj )
    {
        return (SDNULL);
    }

    /* Load the inode copy name,ext,attr,cluster, size, datetime. */
    PC_ENTER_CRITICAL();
    if ( dirUpdate )
    {
        crdate.date = pNode->fdate;
        crdate.time = pNode->ftime;
    }
    else
    {
        PC_GETSYSDATE(&crdate.date, &crdate.time);
    }
        

	// save parent block information before calling inode magic

    pc_init_inode( pobj->finode,
        filename,
        fileext,
        (UTINY)attr,
        cluster,
        0L /*size*/ ,
        &crdate );
    PC_EXIT_CRITICAL();

    if ( dirUpdate )
    {
        /* Setup information to change long file name. */
        pobj->pdrive = pmom->pdrive;
        
        clbase = ((UINT32)pmom->finode->fclusterHi);
        clbase <<= 16;
        clbase |= ((UINT32)pmom->finode->fcluster);
        
        if ( !clbase )
        {
            if (pobj->pdrive->fasize == DRV_FAT32)
                pobj->blkinfo.my_frstblock = pobj->blkinfo.my_block = pmom->pdrive->rootblock;
            else
            {
                
                pobj->blkinfo.my_frstblock = pmom->blkinfo.my_frstblock;
                pobj->blkinfo.my_block = pmom->blkinfo.my_block;
            }
        }
        else
        {
            pobj->blkinfo.my_frstblock = pobj->blkinfo.my_block = pc_cl2sector(pobj->pdrive, clbase);
        }
        
        pobj->blkinfo.my_index = 0;
        
        /* To search for same short name and change it. */
        if ( pc_findin(pobj, filename, fileext) )
            ret_val = NO;
        
        copybuff(pobj->finode->fname, filename, (8 * sizeof(UTEXT)) );
        copybuff(pobj->finode->fext, fileext, (3 * sizeof(UTEXT)) );
    }

    /* Convert pobj to native and stitch it in to mom. */
    if ( !pc_insert_inode(pobj, pmom) )
    {
        if ( !dirUpdate )
        {
            PC_FAT_ENTER(pdrive->driveno);
            pc_clrelease(pdrive, cluster);
            PC_FAT_EXIT(pdrive->driveno);
        }
        pc_freeobj(pobj);
        return (SDNULL);
    }

#if (RTFS_SUBDIRS)
    /*
      Now if we are creating subdirectory we have to make the DOT and
      DOT DOT inodes and then change pobj's attribute to ADIRENT.
      The DOT and DOTDOT are not buffered inodes. We are simply putting
      the to disk.
    */
    if ( attributes & ADIRENT )
    {
        if ( !dirUpdate )
        {
            /* Set up a buffer to do surgery */
            pbuff = pc_init_blk( pdrive, pc_cl2sector(pdrive, cluster));
            if ( !pbuff )
            {
                pc_freeobj(pobj);
                PC_FAT_ENTER(pdrive->driveno); /* claim the fat for alloc */
                pc_clrelease(pdrive, cluster);
                PC_FAT_EXIT(pdrive->driveno); 
                return (SDNULL);
            }

            PC_ENTER_CRITICAL();
            pdinodes = (DOSINODE *)pbuff->data;

            /* Load DOT and DOTDOT in native form */
            /* DOT first. It points to the begining of this sector */
            pc_init_inode( lfinode,
                (TEXT *)string_padded_dot,
                (TEXT *)string_3_spaces,
                ADIRENT,
                cluster,
                0L /*size*/,
                &crdate);
            /* And to the buffer in intel form */
#if (CHAR_16BIT)
            char_pack_dosinode((UINT16 *)(pdinodes),
                lfinode,
                (0<<5));
#else
            pc_ino2dos (pdinodes, lfinode);
#endif

            /* Now DOTDOT points to mom's cluster */				
            pc_init_inode( lfinode,
                (TEXT *)string_padded_dot_dot,
                (TEXT *)string_3_spaces,
                ADIRENT,
                pc_sec2ClusterDir(pdrive, pc_firstblock(pmom)),
                0L /*size*/,
                &crdate);

            /* And to the buffer in intel form */
#if (CHAR_16BIT)
            char_pack_dosinode((UINT16 *)(pdinodes), lfinode, (1<<5));
#else
            pc_ino2dos (++pdinodes, lfinode );
#endif

            /* Write the cluster out */
            if ( !pc_write_blk ( pbuff ) )
            {
                pc_free_buf(pbuff, YES, NO);        /* Error. Chuck the buffer */
                pc_freeobj(pobj);
                PC_FAT_ENTER(pdrive->driveno);   /* claim the fat for alloc */
                pc_clrelease(pdrive, cluster);
                PC_FAT_EXIT(pdrive->driveno);
                PC_EXIT_CRITICAL();
                return (SDNULL);
            }
            else {
                pc_free_buf(pbuff, NO, NO);
                PC_EXIT_CRITICAL();
            }
        }

        /* And write the node out with the original attributes */
        pobj->finode->fattribute = (UTINY)attributes;

        /* Convert to native. Overwrite the existing inode. Set date */
        if ( !pc_update_inode(pobj, NO, YES) )
        {
            pc_freeobj(pobj);
            if ( !dirUpdate )
            {
                PC_FAT_ENTER(pdrive->driveno); /* claim the fat for alloc */
                pc_clrelease(pdrive, cluster);
                PC_FAT_EXIT(pdrive->driveno);
            }
            return (SDNULL);
        }
    }
#endif

    PC_FAT_ENTER(pdrive->driveno);
    if ( !dirUpdate )
    {
        ret_val = pc_flushfat(pdrive->driveno);
    }
    PC_FAT_EXIT(pdrive->driveno);

    if ( ret_val )
    {
        return (pobj);
    }
    else
    {
        pc_freeobj(pobj);
        return (SDNULL);
    }
}


/*****************************************************************************
 * Name: PC_INSERT_INODE - Insert a new inode into an existing directory inode.
 *
 * Description:
 *       Take mom, a fully defined DROBJ, and pobj, a DROBJ with a finode
 *       containing name, ext, etc, but not yet stitched into the inode buffer
 *       pool, and fill in pobj and its inode, write it to disk and make the
 *       inode visible in the inode buffer pool. (see also pc_mknode() )
 *
 * Note: The parent directory is locked before this routine is called.
 *
 * Entries:
 *       DROBJ *pobj
 *       DROBJ *pmom
 *
 * Returns:
 *       Returns YES if all went well, NO on a write error, disk full error 
 *       or root directory full.
 *
 ******************************************************************************/
SDBOOL pc_insert_inode(  DROBJ *pobj, DROBJ *pmom ) /*__fn__*/
{
    BLKBUFF *pbuff=0;
    DIRBLK  *pd=0;
    DOSINODE *pi=0;
    UINT32  cluster=0;
    BLOCKT  saveMyBlock=0;
    INT16   retBlock=0;
    INT16   saveMyIndex=0;
    INT16   saveTotalEntry=0;
    INT16   totalentry=0;
    INT16   i=0;
    INT16   strLength=0;
    COUNT   remainder=0;
    COUNT   curLength=0;
    COUNT   index=0;
    UINT16  enough_entry=0;
    UINT16  ORed40=0;
    SDBOOL  retval=0;
#if (CHAR_16BIT)
    LONGINODE pi_char16;
#endif
    UINT16  chkSum = 0;

    /* Set up pobj */
#if 0
    pobj->pdrive = pmom->pdrive;
#endif
    pobj->isroot = NO;
    pd = &pobj->blkinfo;

    /* Now get the start of the dir */
    if ( pobj != pmom )
    {
        pobj->pdrive = pmom->pdrive;
        pd->my_block = pd->my_frstblock = pc_firstblock(pmom);
    }

    if ( !pd->my_block )
    {
        return (NO);
    }

    pd->my_index = 0;

    strLength = 0;
    curLength = 0;
    remainder = 0;
    totalentry = 0;
    enough_entry = NO;

    if ( saveFileName )
    {
        strLength = pc_strlen(saveFileName);
        curLength = strLength;
        remainder  = (UCOUNT)(strLength % 13);
    }

DO_INODE_SEARCH:
    /* Read the data */
    pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
        pobj->blkinfo.my_block);
    if ( !pbuff )
        return (NO);

    while ( pbuff )
    {

        pc_verify_blk(pobj->pblkbuff); // temp

        i = pd->my_index = 0;
#if (CHAR_16BIT)
        pi = (DOSINODE *)&pi_char16;
#else
        pi = (DOSINODE *)pbuff->data;
#endif

        /* look for a slot */
        while ( i < INOPBLOCK )
        {
            /* Swap the information for processing */
            SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)), 0);

#if (CHAR_16BIT)
            getEntryDataChar16bit(pi, 
                (UINT16 *)pbuff->data,
                i);  /* unpack to pi_char16 */
#endif

            /* New entry in the directory entries if name is NULL */
            /* Since we have things like cluster boundaries and sector boundaries, we need to scan
             * for space even if we're starting on a free entry, since the scanning code actually extends
             * the cluster chain. in this case, if saveFileName is set, then it represents the long file name,
             * otherwise we are writing out a standard 8.3 file
             */
			if ( (pi->fname[0] == 0 || pi->fname[0] == PCDELETE) && saveFileName == SDNULL )
            {
                /* We can short circuit scan/extend code here */
                goto UPDATE_DOS_OR_ALIAS_NAME;
            }
            else
            {
                if ( saveFileName == SDNULL )
                    goto GET_NEXT_NAME_ENTRY;
                else
                    goto GET_NEXT_LONG_NAME_ENTRY;
            }

GET_NEXT_LONG_NAME_ENTRY:

            /* save index into directory entry */
            if ( totalentry == 0 )
            {
                saveMyBlock = pobj->blkinfo.my_block;
                saveMyIndex = i;
                
                //diag_printf("resetting needed count\n");
                saveTotalEntry = (INT16)((strLength + 12) / 13);
                totalentry = (INT16)(saveTotalEntry + 1);
            }

            // don't assume anything
#if 0   
            /* Is the entry available? */
            if ( pi->fname[0] == 0 )
            {
                //diag_printf("Ack! thpth!\n");
                totalentry = 0xFFFF;
                enough_entry = YES;
                chkSum = pc_chksum(pobj->finode->fname,
                    pobj->finode->fext); 
                ORed40 = YES;
                
                /* Found available entry, write to it */
                goto ENOUGH_LONG_NAME_ENTRY;
            }
#endif

            /* Searching for enough entries to store the name */
            
            //diag_printf("totalentry = %d\n",totalentry);
            
            while ( totalentry )
            {
                /* Entry is not available */
                if ( (pi->fname[0] != 0) &&
                    (pi->fname[0] != PCDELETE) )
                {
                    /* Not enough entries! */
                    //diag_printf("non-contiguous\n");
                    totalentry = 0;
                    enough_entry = NO;
                    break;
                }
                
                /* Swap the entry back to original form. */
                SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)),
                    ((pi->fattribute == CHICAGO_EXT)? YES: NO));
                
                i++;            /* Next entry */
                
                totalentry--;
                /* || (pi->fname[0] == 0) */
                if (totalentry == 0)
                {
                    //diag_printf("Ack! thpth! - finally found enough\n");
                    /* Found enough entries */
                    chkSum = pc_chksum(pobj->finode->fname,
                        pobj->finode->fext); 
                    enough_entry = YES;
                    ORed40 = YES;
                    totalentry = NO;
                    break;
                }
                
                if (i < INOPBLOCK)
                {
                    /* Swap the information for processing */
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)), 0);

                    /* Read the next entry */
#if (CHAR_16BIT)
                    getEntryDataChar16bit(pi,
                        (UINT16 *)pbuff->data,
                        i);  /* unpack to pi_char16 */
#else
                    pi++;
#endif
                }
                else
                {
                    /* Search for next available entries */
                    pc_free_buf(pbuff, NO, YES);
                    
                    /* Update the objects block pointer */
                    retBlock = pc_next_block(pobj);
                    
                    if ( retBlock == 0 ) {
                        return (NO);
                    }

                    cluster = pc_sec2cluster(pobj->pdrive,
                        pobj->blkinfo.my_block);

                    if (pobj->pdrive->fasize == DRV_FAT32)
                    {
                        if ( cluster == 0L )
                            return (NO);
                    }
                    
                    if ( retBlock == -1 )
                    {
                        /* claim the fat for alloc */
                        PC_FAT_ENTER(pobj->pdrive->driveno);
                        cluster = pc_clgrow(pobj->pdrive, cluster);
                        PC_FAT_EXIT(pobj->pdrive->driveno);
                        
                        if (!cluster)
                            return (NO);
                        
                        pobj->pdrive->prevCluster = pobj->pdrive->currCluster;
                        pobj->pdrive->currCluster = cluster;
                        pd->my_frstblock = pd->my_block = pc_cl2sector(pobj->pdrive, cluster);
                        pd->my_index = 0;
                        pobj->pdrive->nextCluster = pc_clnext(pobj->pdrive, pobj->pdrive->currCluster);
                        
                        /* Prepare new directory entries */
                        /* Zero out the cluster  */
                        if ( !pc_clzero( pobj->pdrive, cluster ) )
                            goto clean_and_fail;
                    }
                    
                    /* Get new block of data */
                    pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
                        pobj->blkinfo.my_block);
                    
                    if (!pbuff)
                        return (NO);
                    
                    i = pd->my_index = 0;
                    
                    /* Swap the information for processing */
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)), 0);
                    
#if (CHAR_16BIT)    
                    pi = (DOSINODE *)&pi_char16;
#else   
                    pi = (DOSINODE *)pbuff->data;
#endif
                }
            } // end while( totalentry )

            pc_verify_blk(pobj->pblkbuff); // temp


ENOUGH_LONG_NAME_ENTRY:
            /*
              Found enough entries for the long name, load back
              old entry and start configuring the entries.
            */
            if ( enough_entry )
            {
                //diag_printf("found enough contiguous entries\n");
                /* Restore the starting entry of the name */
                pobj->blkinfo.my_block = saveMyBlock;
                i = pobj->blkinfo.my_index = saveMyIndex;

                if ( totalentry == NO )
                {
                    /* Load the information back */
                    pc_free_buf(pbuff, NO, YES);
                    pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
                        pobj->blkinfo.my_block);
                }

                /* Calculate where each entry of the long name starts */
                if ( (curLength % 13) == 0 )
                    index = (COUNT)(curLength - 13);
                else
                    index = (COUNT)(curLength - remainder);

                /* Load data in */
#if (CHAR_16BIT)
                pi = (DOSINODE *)&pi_char16;
#else
                pi = ((DOSINODE *)pbuff->data) + i;
#endif

                /* Set up and write the long name to disk */
                while ( saveTotalEntry )
                {
#if (!CHAR_16BIT)
                    /* Swap the information for processing */
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)), 0);
#endif

                    /* Update the DOS disk */
                    ((LONGINODE *)pi)->sequence = (UTINY)(ORed40 ? (saveTotalEntry | LSEQUENCE_END) : saveTotalEntry);

                    LongFilenameToEntry (((LONGINODE *)pi)->fname,  &index, curLength, (COUNT)5);
                    LongFilenameToEntry (((LONGINODE *)pi)->fname2, &index, curLength, (COUNT)6);
                    LongFilenameToEntry (((LONGINODE *)pi)->fname3, &index, curLength, (COUNT)2);

                    curLength = (COUNT)((ORed40 && remainder) ? (curLength - remainder) : (curLength - 13));
                    index = (COUNT)(curLength - 13);

                    ORed40 = NO;

                    ((LONGINODE *)pi)->fattribute = (UTINY)CHICAGO_EXT;
                    ((LONGINODE *)pi)->type = (UTINY)0;
                    ((LONGINODE *)pi)->chksum = (UTINY)chkSum;
                    ((LONGINODE *)pi)->reserved = (UINT16)0;

#if (!CHAR_16BIT)
                    /* Swap the entry back to original form. */
                    SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)),
                        ((((LONGINODE *)pi)->fattribute == CHICAGO_EXT)? YES: NO));
#endif

#if (CHAR_16BIT)
                    char_pack_longinode((UINT16 *)(pbuff->data),
                        (LONGINODE *)pi,
                        (i<<5));
#endif

                    i++;            /* Next entry */

                    saveTotalEntry--;

                    // dc- if we aren't going to reference the next cluster, dont try and fetch the next block
                    //     this fixes the case of directory entries that end exactly on a cluster boundary
                       
                    if (i < INOPBLOCK)
                    {
#if (CHAR_16BIT)
#else
                        pi++;
#endif
                    }
                    else
                    {
                        // as per danc, this should be ok, even when saveTotalEntry == 0

                        /* Write the data */
                        retval = pc_write_blk(pobj->pblkbuff);
                        if ( !retval )
                            return (NO);
                        
                        pc_free_buf(pbuff, NO, YES);
                        
                        retBlock = pc_next_block(pobj);
                        if ( (retBlock == 0) || (retBlock == -1) )
                            return (NO);

                        pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
                            pobj->blkinfo.my_block);
                        if ( !pbuff )
                            return (NO);

                        i = pd->my_index = 0;

#if (CHAR_16BIT)
                        pi = (DOSINODE *)&pi_char16;
#else
                        pi = (DOSINODE *)pbuff->data;
#endif
                    } // if (i < INOPBLOCK) else
                } // while ( saveTotalEntry )

UPDATE_DOS_OR_ALIAS_NAME:
                /* Update Alias or DOS Name to Disk */
                pd->my_index = i;

                /* Don't have to swap the entry back because
                   we are going to overwrite this entry.
                */
#if (CHAR_16BIT)
                char_pack_dosinode((UINT16 *)pbuff->data,
                    pobj->finode,
                    (i<<5));
#else
                pc_ino2dos( pi, pobj->finode );
#endif

                /* Write the data */
                retval = pc_write_blk(pobj->pblkbuff);
                if ( !retval )
                    return (NO);

                /* Mark the inode in the inode buffer */
                if ( retval )
                    pc_marki(pobj->finode,
                        pobj->pdrive,
                        pd->my_block,
                        pd->my_index );
                pc_free_buf(pbuff, !retval, YES);
                return (retval);
                
            } // if ( enough_entry )
            else
            {
                //diag_printf("didn't find enough contiguous entries\n");
            }


GET_NEXT_NAME_ENTRY:

            /* Swap the entry back to original form. */
            SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)),
                ((pi->fattribute == CHICAGO_EXT)? YES: NO) );

            i++;            /* Next entry */
#if (CHAR_16BIT)
#else
            pi++;
#endif
        } // end while (i < NOPBLOCK)

        /* Not in that block. Try again */
        pc_free_buf(pbuff, NO, YES);

        /* Update the objects block pointer */
        retBlock = pc_next_block(pobj);
        if ( (retBlock == 0) || (retBlock == -1) )
            break;

        pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
            pobj->blkinfo.my_block);
    }

    /* Hmmm - root full ??. This is a problem for FAT12 and FAT16 */
    if( (pmom->pdrive->fasize == DRV_FAT12) ||
        (pmom->pdrive->fasize == DRV_FAT16) )
    {
        if ( pmom->isroot )
            return (NO);
        else
        {
#if (RTFS_SUBDIRS)
            goto EXTEND_MORE_ENTRIES;
#else
            return (NO);
#endif
        }                        
    }

#if (RTFS_SUBDIRS)
EXTEND_MORE_ENTRIES:
    /*
      Ok:There are no slots in mom. We have to make one.
      And copy our stuff in.
    */
    /* For all FAT types including FAT32 */
    PC_FAT_ENTER(pobj->pdrive->driveno); /* claim the fat for alloc */
    cluster = pc_sec2cluster(pobj->pdrive, pobj->blkinfo.my_block);
    cluster = pc_clgrow(pobj->pdrive, cluster);
    PC_FAT_EXIT(pobj->pdrive->driveno);

    if ( !cluster )
        return (NO);

    /* Don't forget where the new item is */
    pobj->pdrive->prevCluster = pobj->pdrive->currCluster;
    pobj->pdrive->currCluster = cluster;
    pd->my_frstblock = pd->my_block = pc_cl2sector(pobj->pdrive, cluster);
    pd->my_index = 0;
    pobj->pdrive->nextCluster = pc_clnext(pobj->pdrive, pobj->pdrive->currCluster);

    /* Zero out the cluster  */
    if ( !pc_clzero( pobj->pdrive, cluster ) )
        goto clean_and_fail;

    goto DO_INODE_SEARCH;
#endif

clean_and_fail:
    /* The chain was extended but failed later. Clip (cluster) from the chain */
    PC_FAT_ENTER(pobj->pdrive->driveno); /* claim the fat for alloc */
    pc_cl_truncate(pobj->pdrive, (UINT32)(((UINT32)pmom->finode->fclusterHi << 16) | (UINT32)pmom->finode->fcluster), cluster);
    PC_FAT_EXIT(pobj->pdrive->driveno);

    return (NO);
}


/*****************************************************************************
 * Name: PC_RMNODE - Delete an inode unconditionally.
 *
 * Description:
 *       Delete the inode at pobj and flush the file allocation table.
 *       Does not check file permissions or if the file is already open.
 *       The inode is marked deleted on the disk and the cluster
 *       chain associated with the inode is freed. (Un-delete won't work)
 *       Delete a file / dir or volume. Don't check for write access et al 
 *
 * Note: The parent directory is locked before this routine is called 
 *
 * Returns:
 *    Returns YES if it successfully deleted the inode an flushed the fat.
 *
 * (see also pc_unlink and pc_rmdir).
 *
 ******************************************************************************/
SDBOOL pc_rmnode( DROBJ *pobj) /*__fn__*/
{
    UINT32  cluster;
    SDBOOL  ret_val;

    /* Don't delete anything that has multiple links */
    PC_ENTER_CRITICAL()
        if ( pobj->finode->opencount > 1 )
        {
            PC_EXIT_CRITICAL()
                REPORT_ERROR(PCERR_REMINODE);
            return (NO);
        }

    /* Mark it deleted and unlink the cluster chain */
    pobj->finode->fname[0] = PCDELETE;

    cluster = (UINT32)pobj->finode->fclusterHi;
    cluster <<= 16;
    cluster |= (UINT32)pobj->finode->fcluster;

    /*
      We free up store right away. Don't leave cluster pointer
      hanging around to cause problems.
    */
    // dc- if we are deleting it should be safe to remove cluster entries here
    //    pobj->finode->fclusterHi = 0;
    //    pobj->finode->fcluster = 0;

    PC_EXIT_CRITICAL();
    
    /* Convert to native. Overwrite the existing inode.Set archive/date*/
    if ( pc_update_inode(pobj, YES, YES) )
    {
        /* And clear up the space */
        PC_FAT_ENTER(pobj->pdrive->driveno);
        pc_freechain(pobj->pdrive, cluster);
        ret_val = pc_flushfat(pobj->pdrive->driveno);
        PC_FAT_EXIT(pobj->pdrive->driveno) ;
        return (ret_val);
    }

    /* If it gets here we had a problem */
    return (NO);
}

/*****************************************************************************
 * Name: PC_UPDATE_INODE - Flush an inode to disk
 *
 * Description:
 *       Read the disk inode information stored in pobj and write it to
 *       the block and offset on the disk where it belongs. The disk is
 *       first read to get the block and then the inode info is merged in
 *       and the block is written. (see also pc_mknode() )
 *
 *       Take a DROBJ that contains correct my_index & my_block. And an inode.
 *       Load the block. Copy the inode in and write it back out 
 *
 * Entries:
 *       DROBJ *pobj
 *       SDBOOL set_archive
 *       SDBOOL set_date
 *
 * Returns:
 *       Returns YES if all went well, no on a write error.
 *
 ******************************************************************************/

// dc - sandisk foolios boned this one up, most of this routine is a critical section,
//      which locks the mutex, but unfortunately for those of us who use PC_*_CRITICAL,
//      sandisk calls a subroutine which also tries to lock the mutex.
SDBOOL pc_update_inode(DROBJ *pobj, SDBOOL set_archive, SDBOOL set_date) /*__fn__*/
{
    BLKBUFF *pbuff;
    DIRBLK  *pd;
    DOSINODE *pi;
    DATESTR crdate;
    UTINY date_tenths;
    SDBOOL  retval=0;
    INT16   i;
    UINT16  fortyFound;
    UINT16  needToUpdate;
#if (CHAR_16BIT)
    LONGINODE pi_char16;
#endif

    pi = SDNULL;
    needToUpdate = YES;
    pd = &pobj->blkinfo;
    i  = (INT16)pd->my_index;

    if ( (i >= INOPBLOCK) ) /* Index into block */
        return (NO);

    PC_ENTER_CRITICAL();
    /* Set the archive bit and the date */
    if ( set_archive )
    {
        pobj->finode->fattribute |= ARCHIVE;
    }
    PC_EXIT_CRITICAL();

    if ( set_date )
    {
        PC_ENTER_CRITICAL();
        PC_GETSYSDATE(&crdate.date, &crdate.time);
        pobj->finode->fcrttime = crdate.time;
        pobj->finode->fcrtdate = crdate.date;
        PC_GETSYSDATEEXT(&date_tenths);
        pobj->finode->ftime_tenths = date_tenths;
        PC_EXIT_CRITICAL();
    }
    /* Read the data */
    pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
        pobj->blkinfo.my_block);

    if ( pbuff )
    {
        PC_ENTER_CRITICAL();
        
        /* Update the DOS disk */
#if (CHAR_16BIT)
        char_pack_dosinode((UINT16 *)pbuff->data,
            pobj->finode,
            (i<<5));
#else
        pi = (DOSINODE *)pbuff->data;
        pc_ino2dos( pi+i, pobj->finode );
#endif

        /* Check for file deletion or update. */
        if ( pobj->finode->fname[0] != PCDELETE )
        {
            retval = pc_write_blk(pobj->pblkbuff);

            /*
              Free the buff. If retval == NO(fail), pass a yes to
              freebuf so it will discard the buffer.
            */
            pc_free_buf(pbuff, !retval, NO);

            PC_EXIT_CRITICAL();
            return (retval);
        }
        else
        {       /* File deletion. For long file name,  we will go back
                   to search and delete all entries associated to the
                   DOS file name.
                */
            if ( i == 0 )           /* Check for first entry */
            {
                retval = pc_write_blk(pobj->pblkbuff);

                pc_free_buf(pbuff, !retval, NO);
                needToUpdate = NO;

                if (pobj->blkinfo.my_block == pobj->pdrive->rootblock)
                {
                    PC_EXIT_CRITICAL();
                    return (retval);
                }

                if (!pc_prev_block(pobj))
                {
                    PC_EXIT_CRITICAL();
                    return (NO);
                }

                i = pd->my_index = INOPBLOCK;

                /* Get new block of data */
                pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
                    pobj->blkinfo.my_block);

                if (!pbuff) { // dc - are you returning? yes, you are. punks.
                    PC_EXIT_CRITICAL() ;
                    return (NO);
                }
                
#if (CHAR_16BIT)
                pi = (DOSINODE *)&pi_char16;
#else
                pi = (DOSINODE *) pbuff->data;
#endif
            }


            i--;
            pd->my_index = i;

            /* Swap data for processing */
            SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (pd->my_index<<5)), 0);

#if (CHAR_16BIT)
            pi = (DOSINODE *)&pi_char16;
            getEntryDataChar16bit(pi, 
                (UINT16 *)pbuff->data,
                i);  /* unpack to pi_char16 */
#else
            pi = (DOSINODE *)pbuff->data;
            pi += i;
#endif

            fortyFound = NO;
            while ((pi->fattribute == CHICAGO_EXT) &&
                (!fortyFound) )
            {
                if (((LONGINODE *)pi)->sequence == PCDELETE)
                    break;

                if (((LONGINODE *)pi)->sequence & LSEQUENCE_END)
                    fortyFound = YES;

                ((LONGINODE *)pi)->sequence = PCDELETE;

                /* Swap data back to original form */
                SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (i<<5)),
                    ((pi->fattribute == CHICAGO_EXT)? YES: NO));

#if (CHAR_16BIT)
                char_pack_longinode((UINT16 *)(pbuff->data),
                    (LONGINODE *)pi,
                    (i<<5));
#endif
                if (fortyFound)
                    break;

                /* Check for first entry since we go backward */
                if ( i == 0 )
                {       /* Write information to disk */
                    retval = pc_write_blk(pobj->pblkbuff);
                    pc_free_buf(pbuff, !retval, NO);

                    needToUpdate = NO;

                    if (pobj->blkinfo.my_block == pobj->pdrive->rootblock)
                    {
                        PC_EXIT_CRITICAL() ;
                        return (retval);
                    }

                    if ( !pc_prev_block(pobj) )
                    {
                        PC_EXIT_CRITICAL() ;
                        return (NO);
                    }

                    i  = pd->my_index = INOPBLOCK;

                    // dc- this is completely gay. basically pc_read_blk()
                    //     relocks the mutex (via pc_alloc_blk), so the
                    //     eCos single locking mutexes assert on this
                    //                    PC_EXIT_CRITICAL();
                    
                    /* Load in new block */
                    pobj->pblkbuff = pbuff = pc_read_blk(pobj->pdrive,
                        pobj->blkinfo.my_block);
                    
                    //                    PC_ENTER_CRITICAL();
                    if ( !pbuff ) {
                        PC_EXIT_CRITICAL();
                        return (NO);
                    }
                    


#if (CHAR_16BIT)
                    pi = (DOSINODE *)&pi_char16;
#else           
                    pi = (DOSINODE *)pbuff->data;
                    pi += i;
#endif
                }

                i--;
                pd->my_index = i;

                /* Swap data for processing */
                SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (pd->my_index<<5)), 0);

#if (CHAR_16BIT)
                getEntryDataChar16bit(pi, 
                    (UINT16 *)pbuff->data,
                    i);  /* unpack to pi_char16 */
#else
                pi--;
#endif
            }

            if (!fortyFound && needToUpdate)
                /* Swap data back to orginal form */
                SWAP_NAME_SIZE((UINT16 *)(((UCHAR *)pbuff->data) + (pd->my_index<<5)), 0);

            if (fortyFound || needToUpdate)
            {
                retval = pc_write_blk(pobj->pblkbuff);
            }

            /*
              Free the buff. If retval == NO(fail), pass a yes to
              freebuf so it will discard the buffer.
            */
            pc_free_buf(pbuff, !retval, NO);

            PC_EXIT_CRITICAL();
            return (retval);
        }
    }

    return (NO);
}
#endif  /* (RTFS_WRITE) */


/*****************************************************************************
 * Name: PC_GET_ROOT -  Create the special ROOT object for a drive.
 *
 * Description:
 *       Use the information in pdrive to create a special object for
 *       accessing files in the root directory.
 *       Initialize the special "root" object
 *
 * Note: We do not read any thing in here we just set up the block pointers.
 *
 * Returns:
 *       Returns a pointer to a DROBJ, or NULL if no core available.
 *
 ******************************************************************************/
DROBJ *pc_get_root( DDRIVE *pdrive) /*__fn__*/
{
    DIRBLK  *pd;
    DROBJ   *pobj;
    FINODE  *pfi;

    pobj = pc_allocobj();
    if ( !pobj )
        return (SDNULL);

    pfi = pc_scani(pdrive, (BLOCKT)0L, 0);

    if ( pfi )
    {
        pc_freei(pobj->finode);
        pobj->finode = pfi;
    }
    else
    {
        /* No inode in the inode list. Copy the data over
         * and mark where it came from.
         */
        pc_marki(pobj->finode, pdrive, (BLOCKT)0L, 0);
    }


    /* Add a TEST FOR DRIVE INIT Here later */
    pobj->pdrive = pdrive;

    /* Set up the tree stuf so we know it is the root */
    pd = &pobj->blkinfo;
    pd->my_frstblock = pdrive->rootblock;
    pd->my_block = pdrive->rootblock;
    pd->my_index = 0;
    pobj->isroot = YES;

    return (pobj);
}


/*****************************************************************************
 * Name: PC_FIRSTBLOCK -  Return the absolute block number of a directory's 
 *                        contents.
 *
 * Description:
 *       Returns the block number of the first inode in the subdirectory.
 *       If pobj is the root directory the first block of the root will be
 *       returned.
 *
 * Entries:
 *       DROBJ   *pobj
 *
 * Returns:
 *       Returns 0 if the obj does not point to a directory, otherwise the
 *       first block in the directory is returned.
 *       Get the first block of a root or subdir.
 *
 *****************************************************************************/
BLOCKT pc_firstblock( DROBJ *pobj ) /*__fn__*/
{
    UINT32  clno;

    if ( !pc_isadir(pobj) )
        return (BLOCKEQ0);

    /* Root dir ? */
#if (RTFS_SUBDIRS)
    if ( !pobj->isroot )
    {
        clno = (UINT32)pobj->finode->fclusterHi;
        clno <<= 16;
        clno |= (UINT32)pobj->finode->fcluster;
        return ( pc_cl2sector(pobj->pdrive, clno) );
    }
    else
#endif
        return (pobj->blkinfo.my_frstblock);
}


/*****************************************************************************
 * PC_NEXT_BLOCK - Calculate the next block owned by an object.
 *
 * Description:
 *    Find the next block owned by an object in either the root or a cluster
 *    chain and update the blockinfo section of the object.
 *
 * Entries:
 *       DROBJ *pobj
 *
 * Returns:
 *
 *       Return -1  end of chain (needed to extend directory entries.
 *               0  out of bound of directory sectors or total sectors.
 *               1  success
 *
 ******************************************************************************/
INT16 pc_next_block( DROBJ *pobj ) /*__fn__*/
{
    BLOCKT  nxt;

    pc_verify_blk(NULL); // temp

    nxt = pc_l_next_block(pobj);

    if ( nxt == (BLOCKT)0x7FFFFFFF)
        return (-1);

    if ( nxt )
    {
        pobj->blkinfo.my_block = nxt;
        return (1);
    }

    pc_verify_blk(NULL); // temp

    return (0);
}


SDBOOL pc_prev_block( DROBJ *pobj ) /*__fn__*/
{
    BLOCKT  prev;

    pc_verify_blk(NULL); // temp
    prev = pc_l_prev_block(pobj);

    if ( prev )
    {
        return (YES);
    }

    return (NO);
}


/*****************************************************************************
 * Name:    PC_L_NEXT_BLOCK - Calculate the next block in a chain.
 *
 * Description:
 *    Find the next block in either the root or a cluster chain.
 *
 * Entries:
 *       DROBJ *pobj
 *
 * Returns:
 *       Returns 0 on end of root dir or the next block in a chain.
 *
 ******************************************************************************/
BLOCKT pc_l_next_block( DROBJ *pobj ) /*__fn__*/
{
    UINT32  cluster;
    BLOCKT  curblock;
    DDRIVE  *pdrive;

    pdrive = pobj->pdrive;
    curblock = pobj->blkinfo.my_block;

    /* If the block is in the root area */
    if ((pdrive->fasize == DRV_FAT12) || (pdrive->fasize == DRV_FAT16))
    {
        if (curblock < pdrive->firstclblock )
        {
            if ( curblock < pdrive->rootblock )
                return (BLOCKEQ0);
            else if ( ++curblock < pdrive->firstclblock )
                return (curblock);
            else
                return (BLOCKEQ0);
        }
#if (RTFS_SUBDIRS)
        else
        {
            goto DO_SUBDIR_NEXT;
        }
#else
        return (BLOCKEQ0);
#endif
    }
    else
DO_SUBDIR_NEXT:
    {
        /* In cluster space */
        if ( curblock >= pdrive->numsecs )
            return (BLOCKEQ0);

        /* Get the next block */
        curblock += 1;

        /*
          If the next block is not on a cluster edge then it must be
          in the same cluster as the current. - otherwise we have to
          get the first block from the next cluster in the chain.
        */
        if ( pc_sec2index(pdrive, curblock) )
            return (curblock);
        else
        {
            curblock -= 1;
            /* Get the old cluster number - No error test needed */
            cluster = pc_sec2cluster(pdrive, curblock);

            /* Consult the fat for the next cluster */
            PC_FAT_ENTER(pdrive->driveno) /* claim the fat for alloc */

                if ( cluster == pdrive->currCluster )
                {
                    if ( !pdrive->nextCluster || pdrive->nextCluster == 0xFFFFFFF)
                    {
                        PC_FAT_EXIT(pdrive->driveno) 
                            return ((BLOCKT)0x7FFFFFFF);    /* End of chain */
                    }
                    else
                    {
                        pdrive->prevCluster = cluster;
                        pdrive->currCluster = pdrive->nextCluster;
                        curblock = pc_cl2sector(pdrive, pdrive->currCluster);
                        pdrive->nextCluster = pc_clnext(pdrive, pdrive->currCluster);
                        PC_FAT_EXIT(pdrive->driveno) 
                            if (curblock)
                                pobj->blkinfo.my_frstblock = curblock;
                            else
                                curblock = (BLOCKT)0x7FFFFFFFL;
                        return (curblock);
                    }
                }
                else
                {
                    pdrive->prevCluster = cluster;
                    pdrive->currCluster = pc_clnext(pdrive, cluster);
                    curblock = pc_cl2sector(pdrive, pdrive->currCluster);
                    pdrive->nextCluster = pc_clnext(pdrive, pdrive->currCluster);
                    PC_FAT_EXIT(pdrive->driveno) 
                        if (curblock)
                            pobj->blkinfo.my_frstblock = curblock;
                        else
                            curblock = (BLOCKT)0x7FFFFFFFL;
                    return (curblock);
                }
        }
    }
}

BLOCKT pc_l_prev_block( DROBJ *pobj ) /*__fn__*/
{
    UINT32  cluster;
    UINT16  sectorToIndex;
    BLOCKT  curblock;
    DDRIVE  *pdrive;

    pdrive = pobj->pdrive;
    curblock = pobj->blkinfo.my_block;

    /* If the block is in the root area */
    if ((pdrive->fasize == DRV_FAT12) || (pdrive->fasize == DRV_FAT16))
    {
        if (curblock < pdrive->firstclblock )
        {
            if ( curblock <= pdrive->rootblock )
                return (BLOCKEQ0);
            else if ( --curblock >= pdrive->rootblock )
            {
                pobj->blkinfo.my_block = curblock;
                return (curblock);
            }
            else
                return (BLOCKEQ0);
        }
#if (RTFS_SUBDIRS)
        else
        {
            goto DO_SUBDIR_PREV;
        }
#else
        return (BLOCKEQ0);
#endif
    }
    else
DO_SUBDIR_PREV:
    {
        /* In cluster space */
        if ( curblock <= pdrive->firstclblock )
            return (BLOCKEQ0);

        /* Get the previous block */
        curblock -= 1;

        /*
          If the previous block is not on a cluster edge then it must be
          in the same cluster as the current. - otherwise we have to
          get the first block from the next cluster in the chain.
        */

        sectorToIndex = pc_sec2index(pdrive, curblock);

        if ( sectorToIndex < (UINT16)(pdrive->secpalloc - 1))
        {
            pobj->blkinfo.my_block = curblock;
            return (curblock);
        }
        else
        {
            curblock += 1;

            /* Get the old cluster number - No error test needed */
            cluster = pc_sec2cluster(pdrive, curblock);

            /* Consult the fat for the next cluster */
            PC_FAT_ENTER(pdrive->driveno) /* claim the fat for alloc */
                if ( cluster == pdrive->currCluster )
                {
                    if ( !pdrive->prevCluster )
                    {
                        PC_FAT_EXIT(pdrive->driveno) 
                            return (BLOCKEQ0);    /* End of chain */
                    }
                    else
                    {
                        pdrive->nextCluster = cluster;
                        cluster = pdrive->prevCluster;
                        curblock = pc_cl2sector(pdrive, cluster);
                        pobj->blkinfo.my_frstblock =  curblock;
                        pobj->blkinfo.my_block =
                            curblock + (BLOCKT)(pobj->pdrive->secpalloc - 1);

                        pdrive->prevCluster = pc_clprev(pdrive, cluster);
                        PC_FAT_EXIT(pdrive->driveno)
                            return (curblock);
                    }
                }
                else
                {
                    pdrive->nextCluster = cluster;
                    pdrive->currCluster = pc_clprev(pdrive, cluster);
                    curblock = pc_cl2sector(pdrive, pdrive->currCluster);
                    pobj->blkinfo.my_frstblock =  curblock;
                    pobj->blkinfo.my_block =
                        curblock + (BLOCKT)(pobj->pdrive->secpalloc - 1);
                    pdrive->prevCluster = pc_clprev(pdrive, pdrive->currCluster);
                    PC_FAT_EXIT(pdrive->driveno) 
                        return (curblock);
                }
        }
    }
}


/*****************************************************************************
 * Name: PC_MARKI -  Set dr:sec:index info and stitch a FINODE into the inode list
 *                Take an unlinked inode and link it in to the inode chain.
 *                Initialize the open count and sector locater info.
 *
 * Description
 *       Each inode is uniquely determined by DRIVE, BLOCK and Index into that 
 *       block. This routine takes an inode structure assumed to contain the
 *       equivalent of a DOS directory entry. And stitches it into the current
 *       active inode list. Drive block and index are stored for later calls
 *       to pc_scani and the inode's opencount is set to one.
 *
 * Entries:
 *       FINODE *pfi
 *       DDRIVE *pdrive
 *       BLOCKT sectorno
 *       INT16 index
 *
 * Returns:
 *       None
 *
 *****************************************************************************/
SDVOID pc_marki( FINODE *pfi, DDRIVE *pdrive, BLOCKT sectorno, INT16 index) /*__fn__*/
{
    PC_ENTER_CRITICAL();

    if( pfi->pprev || pfi->pnext ) {
        DEBUGP( FAT_FS, DBGLEV_WARNING, "finode at sector %d index %d may be multiply linked\n" );
    }
    
    pfi->my_drive = pdrive;
    pfi->my_block = sectorno;
    pfi->my_index = index;
    pfi->opencount = 1;

    /* Stitch the inode at the front of the list */
    if ( inoroot )
        inoroot->pprev = pfi;

    pfi->pprev = SDNULL;
    pfi->pnext = inoroot;

    inoroot = pfi;
    PC_EXIT_CRITICAL();
}


/*****************************************************************************
 * Name: PC_SCANI -  Search for an inode in the internal inode list.
 *
 * Description:
 *       Each inode is uniquely determined by DRIVE, BLOCK and Index into that 
 *       block. This routine searches the current active inode list to see
 *       if the inode is in use. If so the opencount is changed and a pointer
 *       is returned. This guarantees that two processes will work on the same 
 *       information when manipulating the same file or directory.
 *
 * Returns:
 *        A pointer to the FINODE for pdrive:sector:index or NULL if not found
 *
 * See if the inode for drive,sector , index is in the list. If so..
 *    bump its open count and return it. Else return NULL
 *
 ******************************************************************************/
FINODE *pc_scani( DDRIVE *pdrive, BLOCKT sectorno, INT16 index ) /*__fn__*/
{
    FINODE *pfi;

    PC_ENTER_CRITICAL();
    pfi = inoroot;

    while ( pfi )
    {
        if ( (pfi->my_drive == pdrive) &&
            (pfi->my_block == sectorno) &&
            (pfi->my_index == index) )
        {
            pfi->opencount += 1;
            PC_EXIT_CRITICAL();
            return (pfi);
        }
        pfi = pfi->pnext;
    }
    PC_EXIT_CRITICAL();
    
    return (SDNULL);
}


/*****************************************************************************
 * Name: PC_ALLOCOBJ -  Allocate a DROBJ structure
 *
 * Description
 *       Allocates and zeroes the space needed to store a DROBJ structure.
 *       Also allocates and zeroes a FINODE structure and links the two
 *       via the finode field in the DROBJ structure.
 *
 * Returns
 *    Returns a valid pointer or NULL if no more core.
 *
 ******************************************************************************/
DROBJ *pc_allocobj( SDVOID ) /*__fn__*/
{
    DROBJ *pobj;

    /* Alloc a DROBJ */
    pobj = pc_memory_drobj(SDNULL);
    if ( pobj )
    {
        pobj->finode = pc_alloci();
        if ( !pobj->finode )
        {
            /* Free the DROBJ */
            pc_memory_drobj(pobj);
            pobj = SDNULL;
        }
    }

    return (pobj);
}           


/*****************************************************************************
 * Name: PC_ALLOCI -  Allocate a FINODE structure
 *
 * Description:
 *       Allocates and zeroes a FINODE structure.
 *
 * Returns:
 *       Returns a valid pointer or NULL if no more core.
 *
 *****************************************************************************/
FINODE *pc_alloci( SDVOID ) /*__fn__*/
{
    FINODE *p;

    p = pc_memory_finode(SDNULL);

    return (p);
}


/*****************************************************************************
 * Name: PC_FREE_ALL_I -  Release all inode buffers associated with a drive.
 *
 * Description
 *       Called by pc_dskclose().
 *       For each internally buffered finode (dirent) check if it exists on
 *       pdrive. If so delete it. In debug mode print a message since all
 *       finodes should be freed before pc_dskclose is called.
 *
 * Returns
 *       None
 *
 ******************************************************************************/
SDVOID pc_free_all_i( DDRIVE *pdrive ) /*__fn__*/
{
    FINODE *pfi;

    PC_ENTER_CRITICAL();
    pfi = inoroot;
    PC_EXIT_CRITICAL();
    
    while ( pfi )
    {
        if ( pfi->my_drive == pdrive )
        {
            /* Set the opencount to 1 so freei releases the inode */
            pfi->opencount = 1;
            pc_freei(pfi);
            
            /* Since we changed the list go back to the top */
            PC_ENTER_CRITICAL();
            pfi = inoroot;
            PC_EXIT_CRITICAL();
        }
        else
        {
            PC_ENTER_CRITICAL();
            pfi = pfi->pnext;
            PC_EXIT_CRITICAL();
        }
    }
}


/*****************************************************************************
 * Name: PC_FREEI -  Release an inode from service
 *
 * Description
 *       If the FINODE structure is only being used by one file or DROBJ,
 *       unlink it from the internal active inode list and return it to
 *       the heap; otherwise reduce its open count.
 *
 * Entries:
 *       FINODE  *pfi
 *
 * Returns
 *       None
 *
 *****************************************************************************/
SDVOID pc_freei( FINODE *pfi ) /*__fn__*/
{
    if ( !pfi )
    {
        return;
    }

    PC_ENTER_CRITICAL();
    if ( pfi->opencount )
    {
        if ( --pfi->opencount ) /* Decrement opencount and return if non zero */
        {
            PC_EXIT_CRITICAL();
            return;
        }
        else
        {
            if ( pfi->pprev ) /* Pont the guy behind us at the guy in front*/
            {
                pfi->pprev->pnext = pfi->pnext;
            }
            else if( inoroot == pfi )
            {
                /* No prev, we were at the front so
                   make the next guy the front.
                */
                inoroot = pfi->pnext;
            } else {
                /* no previous pointer, but we're not root, so the chain is corrupted */
                DBASSERT(FAT_FS, (pfi->pprev || inoroot == pfi), "Inode chain corrupted\n");
            }
            
            if ( pfi->pnext ) /* Make the next guy point behind */
            {
                pfi->pnext->pprev = pfi->pprev;
            }
        }
    }
    PC_EXIT_CRITICAL();
    
    /* release the core */
    pc_memory_finode(pfi);
}


/*****************************************************************************
 * Name: PC_FREEOBJ -  Free a DROBJ structure
 *
 * Description:
 *       Return a drobj structure to the heap. Calls pc_freei to reduce the
 *       open count of the finode structure it points to and return it to the
 *       heap if appropriate.
 *
 * Entries:
 *       DROBJ *pobj
 *
 * Returns
 *       None
 *
 *****************************************************************************/
SDVOID pc_freeobj( DROBJ *pobj ) /*__fn__*/
{
    if ( pobj )
    {
        pc_freei(pobj->finode);

        /* Release the core */
        pc_memory_drobj(pobj);
    }
}


/*****************************************************************************
 * Name: PC_DOS2INODE - Convert a dos disk entry to an in memory inode.
 *
 * Description:
 *       Take the data from pbuff which is a raw disk directory entry and copy
 *       it to the inode at pdir. The data goes from INTEL byte ordering to 
 *       native during the transfer.
 *
 * Entries:
 *       FINODE *pdir
 *       DOSINODE *pbuff
 *
 * Returns
 *       None
 *
 ******************************************************************************/
SDVOID pc_dos2inode (FINODE *pdir, DOSINODE *pbuff) /*__fn__*/
{
    /*
      UINT16 i = 0;
      ULONG ii = 0L;
      #if (CHAR_16BIT)
      b_unpack((UTINY *)pdir->fname, (UINT16 *)pbuff, 8, 0);
      b_unpack((UTINY *)pdir->fext, (UINT16 *)pbuff, 3, 8);
      w_unpack((UINT16 *)&pdir->fattribute, (UINT16 *)pbuff, 8+3);
      for (ii=0; ii < 3; ii++)
      {
      w_unpack((UTINY *)&i, (UINT16 *)pbuff, 8+3+1+(ii<<1));
      pdir->resarea[ii] = to_WORD((UCHAR *)&i);
      }
      w_unpack((UINT16 *)&i, (UINT16 *)pbuff, 8+3+1+3);
      pdir->lastAccess = to_WORD((UCHAR *)&i);
      w_unpack((UINT16 *)&i, (UINT16 *)pbuff, 8+3+1+3+1);
      pdir->fclusterHi = to_WORD((UCHAR *)&i);
      w_unpack((UINT16 *)&i, (UINT16 *)pbuff, 8+3+1+3+1+1);
      pdir->ftime = to_WORD((UCHAR *)&i);
      w_unpack((UINT16 *)&i, (UINT16 *)pbuff, 8+3+1+3+1+1+1);
      pdir->fdate = to_WORD((UCHAR *)&i);
      w_unpack((UINT16 *)&i, (UINT16 *)pbuff, 8+3+1+3+1+1+1+1);
      pdir->fcluster = to_WORD((UCHAR *)&i);
      l_unpack((UINT16 *)&ii, (UINT16 *)pbuff, 8+3+1+3+1+1+1+1+1);
      pdir->fsize = to_DWORD((UCHAR *)&ii);
      #else
    */

#if (LITTLE_ENDIAN)
    copybuff((SDVOID *)pdir, (SDVOID *)pbuff, sizeof(DOSINODE));
#else
#if (USE_HW_OPTION)
#if (CHAR_16BIT)
    copybuff((SDVOID *)pdir, (SDVOID *)pbuff, sizeof(DOSINODE));
#else
    copybuff((SDVOID *)pdir, (SDVOID *)pbuff, 12);
    for (ii=0; ii < 3; ii++)
    {
        i = to_WORD((UCHAR *)&pbuff->resarea[ii]);
        pdir->resarea[ii] = to_WORD((UCHAR *)&i);
    }
    i = to_WORD((UCHAR *)&pbuff->lastAccess);
    pdir->lastAccess = i;
    i = to_WORD((UCHAR *)&pbuff->fclusterHi);
    pdir->fclusterHi = i;
    i = to_WORD((UCHAR *)&pbuff->ftime);
    pdir->ftime = i;
    i = to_WORD((UCHAR *)&pbuff->fdate);
    pdir->fdate = i;
    i = to_WORD((UCHAR *)&pbuff->fcluster);
    pdir->fcluster = i;
    ii = to_DWORD((UCHAR *)&pbuff->fsize);
    pdir->fsize = ii;
#endif /* (CHAR_16BIT) */
#else
    copybuff((SDVOID *)pdir, (SDVOID *)pbuff, sizeof(DOSINODE));

#endif /* (USE_HW_OPTION) */

#endif /* (LITTLE_ENDIAN) */
}


#if (RTFS_WRITE)
/*****************************************************************************
 * Name: PC_INIT_INODE -  Load an in memory inode up with user supplied values.
 *
 * Description
 *       Take an uninitialized inode (pdir) and fill in some fields. No other
 *       processing is done. This routine simply copies the arguments into the
 *       FINODE structure. 
 *
 * Note: filename & fileext do not need null termination.
 *
 * Entries:
 *       FINODE *pdir
 *       const char *filename
 *       const char *fileext
 *       UTINY attr
 *       UCOUNT cluster
 *       ULONG size
 *       DATESTR *crdate
 *
 * Returns
 *       None
 ******************************************************************************/
SDVOID pc_init_inode( FINODE *pdir,
    TEXT *filename,
    TEXT *fileext,
    UTINY attr,
    UINT32 cluster,
    ULONG size,
    DATESTR *crdate ) /*__fn__*/
{

    /* Copy the file names and pad with ' ''s */
    copybuff(pdir->fname, filename, (8 * sizeof(UTEXT)) );
    copybuff(pdir->fext, fileext, (4 * sizeof(UTEXT)));

    pdir->fattribute = attr;

    // dc- avoid nasty structure hackery
    //    pc_memfill(&pdir->resarea0, (10 * sizeof(UTEXT)) , 0);
    pdir->resarea0     = 0;
    pdir->ftime_tenths = 0;
    pdir->fcrttime     = 0;
    pdir->fcrtdate     = 0;

    pdir->ftime = crdate->time;
    pdir->fdate = crdate->date;

    /* If FAT32, the lower word is save in another location */
    pdir->fclusterHi = (UINT16)((cluster >> 16) & 0x0FFFF);
    pdir->fcluster = (UINT16)(cluster & 0x0FFFF);
    pdir->lastAccess = 0;
    pdir->fsize = size;
}
#endif

#if (RTFS_WRITE)

/*****************************************************************************
 * Name: PC_INO2DOS - Convert an in memory inode to a dos disk entry.
 *
 * Description
 *       Take in memory native format inode information and copy it
 *       to a buffer. Translate the inode to INTEL byte ordering
 *       during the transfer.
 *
 * Entries:
 *       FINODE *pdir
 *       DOSINODE *pbuff
 *
 * Returns
 *    None
 *
 *****************************************************************************/
SDVOID pc_ino2dos (DOSINODE *pbuff, FINODE *pdir) /*__fn__*/
{
#if (CHAR_16BIT)
#error wrong
    UINT16 i;
    UINT16 j;
    ULONG  k;

    b_pack((UINT16 *)pbuff,
        (UTINY *)(&pdir->fattribute), 1, 0+8+3);

    /* Reserved area */
    for (i = 0 ; i < 5; i++)
    {
        fr_WORD((UCHAR *)&j, pdir->resarea[i]);
        w_pack((UINT16 *)pbuff, j, (0+8+3+1+(i<<1)));
    }

    fr_WORD((UCHAR *) &j, pdir->ftime);       /*X*/
    w_pack((UINT16 *)pbuff, j, (0+8+3+1+10));
    fr_WORD((UCHAR *) &j, pdir->fdate);       /*X*/
    w_pack((UINT16 *)pbuff, j, (0+8+3+1+10+2));
    fr_WORD((UCHAR *) &j, pdir->fcluster);       /*X*/
    w_pack((UINT16 *)pbuff, j, (0+8+3+1+10+2+2));
    fr_DWORD((UCHAR *) &k, pdir->fsize);       /*X*/
    l_pack((UINT16 *)pbuff, k, (0+8+3+1+10+2+2+2));
#else

#if LITTLE_ENDIAN
    /* Copy Name, Extension and Attribute */
    copybuff((SDVOID *)&pbuff->fname[0], (SDVOID *)&pdir->fname[0], 12);  /*X*/
    pbuff->resarea0 = pdir->resarea0;
    pbuff->ftime_tenths = pdir->ftime_tenths;
    fr_WORD((UCHAR*) &pbuff->fcrttime, pdir->fcrttime );
    fr_WORD((UCHAR*) &pbuff->fcrtdate, pdir->fcrtdate );
    fr_WORD((UCHAR *) &pbuff->fclusterHi, pdir->fclusterHi);       /*X*/
    fr_WORD((UCHAR *) &pbuff->ftime, pdir->ftime);       /*X*/
    fr_WORD((UCHAR *) &pbuff->fdate, pdir->fdate);       /*X*/
    fr_WORD((UCHAR *) &pbuff->fcluster, pdir->fcluster); /*X*/
    fr_DWORD((UCHAR *) &pbuff->fsize, pdir->fsize);      /*X*/

#else
#error wrong
    /* Copy the entry to the buffer */
    copybuff((SDVOID *)(pbuff), (SDVOID *)&pdir->fname[0], 32);  /*X*/

    /* Swap the information back to original form */
    SWAP_NAME_SIZE((UINT16 *)pbuff, 0);
#endif
#endif  /* (CHAR_16BIT) */
}

#endif  /* (RTFS_WRITE) */


/*****************************************************************************
 * Name: PC_ISAVOL -  Test a DROBJ to see if it is a volume
 *
 * Description:
 *       Looks at the appropriate elements in pobj and determines if
 *       it is a root or subdirectory.
 *
 * Entries:
 *       DROBJ *pobj
 *
 * Returns:
 *       Returns NO if the obj does not point to a directory.
 *
 *****************************************************************************/
SDBOOL pc_isavol( DROBJ *pobj ) /*__fn__*/
{
    return (pobj->finode->fattribute & AVOLUME);
}


/*****************************************************************************
 * Name:    PC_ISADIR -  Test a DROBJ to see if it is a root or subdirectory
 *
 * Description
 *       Looks at the appropriate elements in pobj and determines if it is
 *       a root or subdirectory.
 *
 * Entries:
 *       DROBJ *pobj
 *
 * Returns:
 *       Returns NO if the obj does not point to a directory.
 *
 *****************************************************************************/
SDBOOL pc_isadir( DROBJ *pobj) /*__fn__*/
{
    if( !pobj || !pobj->finode ) {
        DEBUGP(FAT_FS, DBGLEV_WARNING, "bad values to pc_isadir (%p %p) bt 0x%x\n", pobj, pobj->finode, __builtin_return_address(0));
        return NO;
    }
        
    return ( (pobj->isroot) || (pobj->finode->fattribute & ADIRENT)  );
}


#endif  /* (USE_FILE_SYSTEM) */

