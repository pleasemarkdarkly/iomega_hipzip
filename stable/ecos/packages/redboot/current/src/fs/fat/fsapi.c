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
/*****************************************************************************
 * FileName: FSAPI.C - Contains user api level source code.
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright EBS Inc. 1996
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* The following routines are included:
*
*    pc_system_init  - Initialize device, memory
*    pc_system_close - Close PCIC, memory
*    pc_get_error    - Get current API error code.
*    pc_cluster_size - Get the cluster size of a drive
*    pc_fat_size     - Calculate blocks required for a volume's Allocation Table.
*    pc_free         - Calculate and return the free space on a disk.
*    pc_getdfltdrvno - Get the default drive number.
*    pc_gfirst       - Get stats on the first file to match a pattern.
*    pc_gnext        - Get stats on the next file to match a pattern.
*    pc_gdone        - Free resources used by pc_gfirst/pc_gnext.
*    pc_isdir        - Determine if a path is a directory.
*    pc_isvol        - Determine if a path is a volume
*    pc_mkdir        - Create a directory.
*    pc_mv           - Rename a file.
*    pc_pwd          - Get string representation of current working dir.
*    pc_rmdir        - Delete a directory.
*    pc_unlink       - Delete a file.
*    pc_set_cwd      - Set the current working directory.
*    pc_setdfltdrvno - Set the default drive number.
*    pc_set_default_drive - Set the default drive number.
*
******************************************************************************/

#include <fs/fat/pcdisk.h>


#if (USE_FILE_SYSTEM)


/******************************************************************************
* Name: PC_SYSTEM_INIT -  Initialize PCIC, Open a disk for business.
*
* Description:
*    Called by Applications Programs to initialize and open the disk
*
*    THIS ROUTINE MUST BE CALLED BEFORE ANY OTHERS.
*
* Entries:
*       INT16   driveno Drive Number
*
* Returns:
*       YES if sucessful
*       NO if failure
*
******************************************************************************/
SDBOOL pc_system_init(INT16 driveno) /*__fn__*/
{
    SDBOOL  ret_val;

    ret_val = NO;

    /* Initialize pointers to NULL to avoid unexpected suprises */
    if ( mem_drives_structures == SDNULL )
	null_pointers();

    CHECK_MEM(SDBOOL, 0)    /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)   /* Check if a valid user if multitasking */

	/* Try to intialize and mount a device */
	if ( pc_dskopen (driveno) )       
	    ret_val = YES;

    /* Restore the kernel state */
    PC_FS_EXIT()
	return (ret_val);
}


/******************************************************************************
* Name: PC_SYSTEM_CLOSE -  Close PCIC, Close memory, Close disk.
*
* Description:
*    Called by Applications Programs to close PCIC, memory, and disk
*
*    THIS ROUTINE MUST BE CALLED BEFORE EXIT.
*
* Entries:
*       INT16   driveno Drive Number
*
* Returns:
*       None
*
****************************************************************************/
SDVOID pc_system_close ( INT16 driveno ) /*__fn__*/
{
    /* Release all resources used by the disk the disk */
    pc_dskclose(driveno);
    system_controller_close(driveno);
    pc_memory_close();
}


/*****************************************************************************
** Name: PC_GET_ERROR
**
** Description
**       Get error code
**
** Entries:
**       INT16  driveno     drive/socket number
** Returns:
**          0  No Error
**       otherwise, one of the following error code:
**           PEBADF          9    Invalid file descriptor
**           PENOENT         2    File not found or path to file not found
**           PEMFILE         24   No file descriptors available (too many files open)
**           PEEXIST         17   Exclusive access requested but file already exists.
**           PEACCES         13   Attempt to open a read only file or a special (directory)
**           PEINVAL         22   Seek to negative file pointer attempted.
**           PENOSPC         28   Write failed. Presumably because of no space
**           PESHARE         30   Open failed do to sharing
**           PEDEVICE        31   No Valid Disk Present
**
******************************************************************************/
INT16 pc_get_error (INT16 taskno) /*__fn__*/
{
    INT16  ret_val = 0;

    taskno = taskno;

    /* Claim mutual exclusive semaphore that control access to RTFS */
    PC_FS_ENTER();
    ret_val = (errno);
    PC_FS_EXIT();

    return (ret_val);
}


INT16 pc_get_extended_error (INT16 driveno) /*__fn__*/
{
    return(get_extended_error(driveno));
}


#if (RTFS_WRITE)
#if (RTFS_SUBDIRS)
/*****************************************************************************
* Name: PC_MKDIR    -  Create a directory.
*
* Description:
*       Create a sudirectory in the path specified by name. Fails if a
*       file or directory of the same name already exists or if the path
*       is not found.
*
* Entries:
*       TEXT    name
*
* Returns:
*       Returns YES if it was able to create the directory, otherwise
*       it returns NO.
*
*       If pc_mkdir fails, errno will be set to
*       one of the following:
*
*               PENOENT     - path to new directory not found
*               PEEXIST     - File or Dir already exists by this name.
*               PENOSPC     - Directory create failed
*
******************************************************************************/
SDBOOL pc_mkdir ( TEXT *name ) /*__fn__*/
{
    DROBJ   *pobj;
    DROBJ   *parent_obj;
    TEXT    *path;
    SDBOOL  ret_val;
    INT16   driveno;  
    INT16   p_errno;
    TEXT    fileext[4];
    TEXT    filename[10];

    CHECK_MEM(SDBOOL, 0)    /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)   /* Check if a valid user if multitasking */


	saveFileName = SDNULL;
    ret_val = NO;
    parent_obj = SDNULL;
    pobj = SDNULL;

    p_errno = PENOENT;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return(NO);
    }

    PC_DRIVE_ENTER(driveno, NO) /* Register drive in use */

	path = (TEXT *)fspath;
    path[0] = 0;
    /* Get out the filename and d:parent */

    if ( !validate_current_information( path, filename, fileext, name) )
	goto errex;

    /* Find the parent and make sure it is a directory */
    parent_obj = pc_fndnode(path);
    if ( !parent_obj )
	goto errex;


    /* Lock the parent */   
    PC_INODE_ENTER(parent_obj->finode, YES)
	if ( !pc_isadir(parent_obj) || pc_isavol(parent_obj) )
	    goto errex;

    longFileName = saveFileName;

    /* Fail if the directory exists */
    pobj = pc_get_inode(SDNULL,
			parent_obj,
			filename,
			fileext);

    if ( pobj )
    {
	p_errno = PEEXIST;      /* Exclusive fail */
    }
    else
    {

	pobj = pc_mknode( parent_obj,
			  filename,
			  fileext,
                          SDNULL,
                          ADIRENT,
                          0);
	if ( pobj )
	{
	    p_errno = 0;
	    ret_val = YES;
	}
	else
	{
	    p_errno = PENOSPC;
	    goto errex;
	}
    }

  errex:
    if ( pobj )
	pc_freeobj(pobj);

    if ( parent_obj )
    {
	PC_INODE_EXIT(parent_obj->finode)
	    pc_freeobj(parent_obj);
    }
    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	errno = p_errno;
    PC_FS_EXIT()

	return (ret_val);
}
#endif /* (RTFS_SUBDIRS) */



/******************************************************************************
* Name: PC_MV -  Rename a file.
*
* Description:
*       Renames the file in path (name) to newname. Fails if name is invalid,
*       newname already exists or path not found. Does not test if name is a
*       simple file. It is possible to rename volumes and directories.
*       (This may change in the multiuser version).
*
* Entries:
*       TEXT    *name      Old name
*       TEXT    *newname   New name
*
* Returns:
*       Returns YES if the file was renamed. Or no if the name not found.
*
******************************************************************************/
SDBOOL pc_mv(TEXT *name, TEXT *newname) /*__fn__*/
{
    DROBJ   *pobj;
    DROBJ   *parent_obj;
    BLOCKT  save_myblock;
    TEXT    *path;
    SDBOOL  ret_val;   
    INT16   driveno;
    INT16   p_errno;
    INT16   save_myindex;
    UINT16  isLongFileName;
    TEXT    fileext[4];
    TEXT    filename[10];
    TEXT    newfileext[4];
    TEXT    newfilename[10];
    TEXT    tmpfilename[10];
    DOSINODE fnode;


    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */


	saveFileName = SDNULL;
    pobj = SDNULL;
    parent_obj = SDNULL;
    ret_val = NO;
    p_errno = PENOENT;
    isLongFileName = NO;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return(NO);
    }

    PC_DRIVE_ENTER(driveno, NO) /* Register access to the drive */

	/* Get the path */
	path = (TEXT *)fspath;
    path[0] = 0;
    if ( !validate_current_information( path, filename, fileext, name ) )
	goto errex;

    parent_obj = pc_fndnode(path);

    if ( !parent_obj )
	goto errex;

    /* Lock the parent exclusive */
    PC_INODE_ENTER(parent_obj->finode, YES)

	path[0] = 0;
    if ( !validate_current_information( path, newfilename, newfileext, newname))
	goto errex;

    if ( saveFileName )
	isLongFileName = YES;

    longFileName = saveFileName;

    /* Find the file and init the structure */
    pobj = pc_get_inode(SDNULL,
			parent_obj,
			newfilename,
			newfileext);
    /* THis file should not exist */
    if ( pobj )
    {
	p_errno = PEEXIST;
	goto errex;
    }

    path[0] = 0;
    if ( !validate_current_information( path, filename, fileext, name ) )
	goto errex;

    longFileName = saveFileName;

    /* Find the file and init the structure */
    pobj = pc_get_inode(SDNULL,
			parent_obj,
			filename,
			fileext);
    if ( !pobj )
	goto errex;


    copybuff(filename, pobj->finode->fname, (8 * sizeof(UTEXT)) );
    copybuff(fileext, pobj->finode->fext, (4 * sizeof(UTEXT)) );


    if ( !isLongFileName )
    {
	/* Verify that the old name is a long file name */
	if (currFileName[0] != 0)
	{
	    save_myblock = pobj->blkinfo.my_block;
	    save_myindex = pobj->blkinfo.my_index;

	    pobj->finode->fname[0] = PCDELETE;
	    if ( !pc_update_inode(pobj, YES, NO) )
	    {
		p_errno = PENOSPC;
		goto errex;
	    }

	    /* Check for change inthe block after the node is updated */
	    if ( save_myblock != pobj->blkinfo.my_block )
	    {
		/* Get the original block back */
		if ( !devio_read(driveno,
				 save_myblock, 
				 (UCHAR *)pobj->pblkbuff->data, 
				 1) )
		{
		    p_errno = PENOSPC;
		    goto errex;
		}
	    }

	    pobj->blkinfo.my_block = save_myblock;
	    pobj->blkinfo.my_index = save_myindex;
	}

	copybuff(pobj->finode->fname, newfilename, (8 * sizeof(UTEXT)) );
	copybuff(pobj->finode->fext, newfileext, (4 * sizeof(UTEXT)) );

	pc_memfill(&pobj->finode->resarea0, (6 * sizeof(UTEXT)), (UTINY)0);
    }
    else
    {
	pobj->finode->fname[0] = PCDELETE;
    }

    /* Convert to native. Overwrite the existing inode.Set archive/date */
    if ( !pc_update_inode(pobj, YES, NO) )
    {
	p_errno = PENOSPC;
	goto errex;
    }

    if ( isLongFileName )
    {
	path[0] = 0;
	if ( !validate_current_information( path, tmpfilename, newfileext, newname))
	    goto errex;

	longFileName = saveFileName;

	copybuff(&fnode, pobj->finode, sizeof(DOSINODE));

	/* It does not belong here.  Make room for the new one */
	pc_freeobj(pobj);

	/* Create for read only if write perm. not allowed */
	pobj = pc_mknode( parent_obj,
                          newfilename,
                          newfileext,
                          &fnode,
                          (UINT16)fnode.fattribute,
                          1);

	if ( !pobj )
	{
	    p_errno = PENOSPC;
	    goto errex;
	}

	copybuff(pobj->finode, &fnode, sizeof(DOSINODE));
	copybuff(pobj->finode->fname, newfilename, (8 * sizeof(UTEXT)) );
	copybuff(pobj->finode->fext,  newfileext, (3 * sizeof(UTEXT)) );

	/* Update the change and flush it out to disk */
	if ( !pc_update_inode(pobj, YES, YES) )
	{
	    copybuff(pobj->finode->fname, filename, (8 * sizeof(UTEXT)) );
	    copybuff(pobj->finode->fext,  fileext, (3 * sizeof(UTEXT)) );

	    p_errno = PENOSPC;
	    goto errex;
	}
    }


    p_errno = 0;
    ret_val = YES;

  errex:
    if ( pobj )
	pc_freeobj(pobj);

    if ( parent_obj )
    {
	PC_INODE_EXIT(parent_obj->finode)
	    pc_freeobj(parent_obj);
    }
    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	errno = p_errno;
    PC_FS_EXIT()

	return (ret_val);
}


/*****************************************************************************
* Name: PC_UNLINK - Delete a file.
*
* Description:
*       Delete the file in name. Fail if not a simple file,if it is open,
*       does not exist or is read only.
*
* Entries:
*       TEXT    name    File to be deleted
*
* Returns
*       Returns YES if it successfully deleted the file.
*
*       If NO is returned fs_user->p_errno will be set to one of these values
*               PENOENT     - File not found or path to file not found
*               PEACCES     - Attempt delete a directory or an open file
*               PENOSPC     - Write failed
*
******************************************************************************/
SDBOOL pc_unlink(TEXT *name) /*__fn__*/
{
    DROBJ   *pobj;
    DROBJ   *parent_obj;
    TEXT    *path;
    SDBOOL  ret_val;
    INT16   driveno;
    INT16   p_errno;
    TEXT    fileext[4];
    TEXT    filename[10];

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

        saveFileName = SDNULL;
    ret_val     = NO;
    parent_obj  = SDNULL;
    pobj        = SDNULL;
    p_errno     = 0;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }

    PC_DRIVE_ENTER(driveno, NO) /* Register access to the drive */

	path = (TEXT *)fspath;
    path[0] = 0;
    /* Get out the filename and d:parent */
    if ( !validate_current_information( path, filename, fileext, name ) )
	goto errex;

    /* Find the parent and make sure it is a directory  */
    parent_obj = pc_fndnode(path);
    if ( !parent_obj )
    {
	p_errno = PEACCES;
	goto errex;
    }

    PC_INODE_ENTER(parent_obj->finode, YES)
	if ( !pc_isadir(parent_obj) || pc_isavol(parent_obj) )
	{
	    p_errno = PEACCES;
	    goto errex;
	}

    longFileName = saveFileName;
         
    /* Find the file */
    pobj = pc_get_inode(SDNULL,
			parent_obj,
			filename,
			fileext);

    if ( pobj )
    {
	/*
	  Be sure it is not the root. Since the root is an abstraction
	  we can not delete it plus Check access permissions.
	*/
	if (  pobj->isroot ||
	      (pobj->finode->opencount > 1) ||
	      (pobj->finode->fattribute & (ARDONLY|AVOLUME|ADIRENT)) )
	{
	    p_errno = PEACCES;
	    ret_val = NO;
	    goto errex;
	}
	else
	{
	    ret_val = pc_rmnode(pobj);
	    if ( !ret_val )
		p_errno = PENOSPC;
	}
    }

  errex:
    if ( pobj )
	pc_freeobj(pobj);

    if ( parent_obj )
    {
	PC_INODE_EXIT(parent_obj->finode)
	    pc_freeobj(parent_obj);
    }
    PC_DRIVE_EXIT(driveno)


	errno = p_errno;

    /* Restore the kernel state */
    PC_FS_EXIT()

	return (ret_val);
}



#if (RTFS_SUBDIRS)
/*****************************************************************************
* Name: PC_RMDIR - Delete a directory.
*
* Description
*       Delete the directory specified in name. Fail if name is not
*       a directory, is read only or contains more than the entries . and ..
*
* Returns
*       Returns YES if the directory was successfully removed.
*
*       If NO is returned fs_user->p_errno will be set to one of these values
*               PENOENT     - Directory not found or path to file not found
*               PEACCES     - Not a directory, not empty or in use
*               PENOSPC     - Write failed
*
******************************************************************************/
SDBOOL pc_rmdir ( TEXT *name ) /*__fn__*/
{
    DROBJ   *parent_obj;
    DROBJ   *pobj;
    DROBJ   *pchild;
    TEXT    *path;
    SDBOOL  ret_val;
    INT16   driveno;
    INT16   p_errno;
    TEXT    fileext[4];
    TEXT    filename[10];
    TEXT    asterixExt[4];
    TEXT    asterixName[10];

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    parent_obj = SDNULL;
    pchild = SDNULL;
    pobj = SDNULL;
    p_errno = 0;
    ret_val = NO;
    p_errno = PENOENT;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }
	
    PC_DRIVE_ENTER(driveno, YES) /* RMDIR is strange so lock the drive */

	path = (TEXT *)fspath;
    path[0] = 0;
    /* Get out the filename and d:parent */
    if ( !validate_current_information( path, filename, fileext, name ) )
	goto errex;

    /* Find the parent and make sure it is a directory \ */
    parent_obj = pc_fndnode(path);
    if ( !parent_obj ||
	 !pc_isadir(parent_obj) ||
	 pc_isavol(parent_obj) )
	goto errex;


    longFileName = saveFileName;

    /* Find the file and init the structure */
    pobj = pc_get_inode(SDNULL,
			parent_obj,
			filename,
			fileext);

    if ( parent_obj )
	pc_freeobj(parent_obj);

    if ( !pobj )
	goto errex;

    if ( !pc_isadir(pobj) || (pobj->finode->opencount > 1) )
    {
	p_errno = PEACCES;
	goto errex;
    }

    /* Search through the directory. look at all files */
    /* Any file that is not '.' or '..' is a problem */
    /* Call pc_get_inode with NULL to give us an obj */
    ret_val = YES;

    pc_memfill(asterixName, (8 * sizeof(UTEXT)), 0x20);
    asterixName[0] = '*';
    asterixName[8] = 0;
    pc_memfill(asterixExt, (3 * sizeof(UTEXT)), 0x20);
    asterixExt[0] = '*';
    asterixExt[3] = 0;

    pchild = pc_get_inode(SDNULL,
			  pobj,
			  (TEXT *)asterixName,
			  (TEXT *)asterixExt);
    if ( pchild )
    {
	do 
	{
	    if ( !(pc_isdot((TEXT *)pchild->finode->fname,
			    (TEXT *)pchild->finode->fext)) )
	    {
		if ( !(pc_isdotdot((TEXT *)pchild->finode->fname,
				   (TEXT *)pchild->finode->fext)) )
		{
		    p_errno = PEACCES;
		    ret_val = NO;
		    goto errex;
		}
	    }
	}
	while (pc_get_inode(pchild,
                            pobj,
                            (TEXT *)asterixName,
                            (TEXT *)asterixExt));
    }
    ret_val = pc_rmnode(pobj);
    if ( !ret_val )
	p_errno = PENOSPC;

  errex:
    if ( pchild )
	pc_freeobj(pchild);

    if ( pobj )
	pc_freeobj(pobj);

    PC_DRIVE_EXIT(driveno)


	errno = p_errno;

    /* Restore the kernel state */
    PC_FS_EXIT()

	return (ret_val);
}
#endif /* Subdirs */

#endif /* (RTFS_WRITE) */



/******************************************************************************
* Name: PC_FREE - Count the number of free bytes remaining on a disk
*
* Description
*       Given a path containing a valid drive specifier count the number 
*       of free bytes on the drive.
*
* Entries:
*       INT16   driveno         Drive number
*
* Returns:
*       The number of free bytes or zero if the drive is full,not open,
*       or out of range.
*
******************************************************************************/
ULONG pc_free(INT16 driveno) /*__fn__*/
{
    ULONG ret_val;

    CHECK_MEM(LONG, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(LONG, 0)     /* Check if a valid user if multitasking */

	ret_val = 0L;

    /* Get the drive and make sure it is mounted */
    PC_DRIVE_ENTER(driveno, YES) /* Grab exclusive access to the drive */

	if ( check_media_entry(driveno) )
	    ret_val = pc_ifree(driveno);

    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	PC_FS_EXIT()

	return (ret_val);
}

SDLOCAL FSDSTAT statobject;

/*****************************************************************************
* Name: PC_GFIRST - Get first entry in a directory to match a pattern.
*
* Description:
*       Given a pattern which contains both a path specifier and a search
*       pattern fill in the structure at statobj with information about
*       the file and set up internal parts of statobj to supply appropriate
*       information for calls to pc_gnext.
*
*       Examples of patterns are:
*               D:\USR\RELEASE\NETWORK\*.C
*               D:\USR\BIN\UU*.*
*               D:MEMO_?.*
*               D:*.*
*
* Entries:
*       DSTAT *fsstatobj
*       TEXT *name
*
* Returns:
*       YES if a match was found.
*       NO Otherwise.
*
******************************************************************************/
SDBOOL pc_gfirst(DSTAT *fsstatobj, TEXT *name) /*__fn__*/
{
    FSDSTAT *statobj;
    TEXT    *path;
    INT16   driveno;
    INT16   tmpLength;
    TEXT    fileext[4];
    TEXT    filename[10];

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    statobj = (FSDSTAT *)&statobject;
    statobj->fileinfo = fsstatobj;
    statobj->pobj = SDNULL;
    statobj->pmom = SDNULL;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }

    path = (TEXT *)fspath;
    path[0] = 0;
    /* Get out the filename and d:parent */

    if ( !pc_parsepath(path, filename, fileext, name) )
    {
	/* Restore the kernel state */
	PC_FS_EXIT()
	    return (NO);
    }

    if ( longFileName )
    {
	tmpLength = (INT16)(longFileName - path);
	saveFileName = name + tmpLength;
    }
    else
    {
	saveFileName = SDNULL;
    }

    PC_DRIVE_ENTER(driveno, NO) /* Register drive in use */

	/* Save the pattern. we'll need it in pc_gnext */
        copybuff( fsstatobj->pname, filename, (8 * sizeof(UTEXT)) );
    copybuff( fsstatobj->pext, fileext, (4 * sizeof(UTEXT)) );

    /* Copy over the path. we will need it later */
    copybuff( fsstatobj->path, path, (EMAXPATH * sizeof(UTEXT)) );

    /* Find the file and init the structure */
    statobj->pmom = pc_fndnode( path );
    if ( statobj->pmom )
    {
	/* Found it. Check access permissions */
	if ( pc_isadir( statobj->pmom ) ) 
	{
	    PC_INODE_ENTER(statobj->pmom->finode, NO)

		longFileName = saveFileName;

	    /* Now find pattern in the directory */
	    statobj->pobj = pc_get_inode( SDNULL,
					  statobj->pmom,
					  filename,
					  fileext );
	    if ( statobj->pobj )
	    {
		/* And update the stat structure */
		pc_upstat( statobj );

		/* Remember the drive number. used by gnext et al. */
		fsstatobj->driveno = driveno;

		/*
		  9-20-94 release the FINODE and allocate a
		  dummy. This will keep everyone who expects
		  the drobj to own a finode happy but won't
		  leave the finode open which locks out unlink
		  et al.
		*/
		pc_freei( statobj->pobj->finode ); /* Release the current */
		statobj->pobj->finode = pc_alloci();
		/* END 9-20-94 */

		PC_INODE_EXIT(statobj->pmom->finode)
		    PC_DRIVE_EXIT(driveno)

		    /* Restore the kernel state */
		    PC_FS_EXIT()
		    return (YES);
	    }

	    PC_INODE_EXIT(statobj->pmom->finode)
		}
    }

    /* If it gets here, we had a probblem */
    if ( statobj->pmom )
    {
	pc_freeobj( statobj->pmom );
	statobj->pmom = SDNULL;
    }

    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	PC_FS_EXIT()
	return (NO);
}



/******************************************************************************
* Name: PC_GNEXT - Get next entry in a directory that matches a pattern.
*
* Description:
*       Given a pointer to a DSTAT structure that has been set up by a
*       call to pc_gfirst(), search for the next match of the original
*       pattern in the original path. Return yes if found and update
*       statobj for subsequent calls to pc_gnext.
*
* Entries:
*       DSTAT *statobj
*
* Returns
*       Returns YES if a match was found otherwise NO.
*
******************************************************************************/
SDBOOL pc_gnext ( DSTAT *fsstatobj ) /*__fn__*/
{
    DROBJ   *nextobj;
    FSDSTAT *statobj;

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    statobj = (FSDSTAT *)&statobject;
    statobj->fileinfo = fsstatobj;

    /* see if the drive is still mounted. Don't use pmom et al.
       since they may be purged.
    */
    if ( !pc_drno2dr( fsstatobj->driveno ) )
    {
	PC_FS_EXIT()
	    return (NO);
    }

    /* Register drive in use */
    PC_DRIVE_ENTER(statobj->pmom->pdrive->driveno, NO)
	PC_INODE_ENTER(statobj->pmom->finode, NO)

	/* Now find the next instance of pattern in the directory */
	nextobj = pc_get_inode( statobj->pobj,
				statobj->pmom,
				fsstatobj->pname,
				fsstatobj->pext );

    if ( nextobj )
    {
	statobj->pobj = nextobj;

	/* And update the stat structure */
	pc_upstat( statobj );

	/*
	  9-20-94 release the FINODE and allocate a dummy.
	  This will keep everyone who expects the drobj to
	  own a finode happy but won't leave the finode open
	  which locks out unlink et al.
	*/
	pc_freei(statobj->pobj->finode); /* Release the current */
	statobj->pobj->finode = pc_alloci();
	/* END 9-20-94 */


	PC_INODE_EXIT(statobj->pmom->finode)
	    PC_DRIVE_EXIT(statobj->pmom->pdrive->driveno)

	    /* Restore the kernel state */
	    PC_FS_EXIT()
	    return(YES);
    }
    else
    {
	PC_INODE_EXIT(statobj->pmom->finode)
	    PC_DRIVE_EXIT(statobj->pmom->pdrive->driveno)

	    /* Restore the kernel state */
	    PC_FS_EXIT()
	    return (NO);
    }
}


/*****************************************************************************
* Name: PC_GDONE - Free internal resources used by pc_gnext and pc_gfirst.
*
* Description:
*       Given a pointer to a DSTAT structure that has been set up
*       by a call to pc_gfirst() free internal elements used by the statobj.
*
* NOTE: You MUST call this function when done searching through a 
*       directory.
*
* Entries:
*       DSTAT *statobj
*
* Returns:
*    None
*
******************************************************************************/
SDVOID pc_gdone ( DSTAT *fsstatobj ) /*__fn__*/
{
    FSDSTAT *statobj;
    INT16   driveno;

    VOID_CHECK_MEM()      /* Make sure memory is initted */
	PC_FS_ENTER()
	VOID_CHECK_USER()       /* Check if a valid user if multitasking */

	statobj = (FSDSTAT *)&statobject;
    statobj->fileinfo = fsstatobj;

    /*
      see if the drive is still mounted. Don't use pmom et al.
      since they may be purged.
    */
    if ( !pc_drno2dr(fsstatobj->driveno) )
    {
	PC_FS_EXIT()
	    return;
    }
    driveno = statobj->pmom->pdrive->driveno;

    PC_DRIVE_ENTER(driveno, NO)

	if ( statobj->pobj )
	    pc_freeobj( statobj->pobj );

    if ( statobj->pmom )
	pc_freeobj( statobj->pmom );

    saveFileName = SDNULL;
    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	PC_FS_EXIT()
	}


/*****************************************************************************
* Name: pc_get_curr_unicode_fname
*
* Processing:   Loads the current long unicode filename into *filename.  This
*				function needs to be used in conjunction with:
*				pc_gfirst(), pc_gnext(), and pc_gdone().
*				The reason is that when you make a call to pc_gfirst() or
*				pc_gnext(), the file system loads the current unicode filename
*				into a global that this function uses.  Comprede?
*
*				Daniel Bolstad -- danb@iobjects.com

* Entries:
*       SWCHAR   filename         Pointer to 
*
* Returns:
*       None       
*****************************************************************************/
SDVOID pc_get_curr_unicode_fname(SDWCHAR *filename, INT16 buffsize)
{
    SDWCHAR *p_uni = (SDWCHAR*)unicodeFileName;
    SDWCHAR *p_out = filename;

    while(*p_uni && buffsize--)
    {
        *p_out++ = *p_uni++;
    }
    while (buffsize-- > 0)
    {
        *p_out++ = 0;
    }
}


/*****************************************************************************
* Name: PC_SET_DEFAULT_DRIVE - Set the current default drive.
*
* Description
*       Use this function to set the current default drive that will
*       be used when a path specifier does not contain a drive specifier.
*
* Note: The default default is zero (drive A:)
*       Set the currently stored default drive.
*
*    see also pc_setdfltdrvno()
*
* Entries:
*       INT16   driveno         Drive Number
*
* Returns:
*    Return NO if the drive is out of range.
*
******************************************************************************/
SDBOOL pc_set_default_drive( INT16 driveno ) /*__fn__*/
{

    /* Set a selected drive to be the default drive */
    if ( driveno >= 0 )
    {
	return (pc_setdfltdrvno(driveno));
    }
    else
	return (NO);
}


/******************************************************************************
* Name: PC_SETDFLTDRVNO - Set the current default drive.
*
* Description:
*       Use this function to set the current default drive that will be  
*       used when a path specifier does not contain a drive specifier.
* Note: The default default is zero (drive A:)
*       Set the currently stored default drive
*
* see also pc_getdfltdrvno()
*
* Entries:
*       INT16   driveno         Drive Number
*
* Returns:
*       Return NO if the drive is out of range.
*
******************************************************************************/
SDBOOL pc_setdfltdrvno( INT16 driveno ) /*__fn__*/
{
    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	/* Check drive number */
	if ( !pc_validate_driveno(driveno) )
	{
	    /* Restore the kernel state */
	    PC_FS_EXIT()
		return (NO);
	}
	else
	    fs_user->dfltdrv = driveno;

    /* Restore the kernel state */
    PC_FS_EXIT()

	return (YES);
}


/*****************************************************************************
* Name: PC_GETDFLTDRVNO - Return the current default drive.
*
* Description
*       Use this function to get the current default drive when a
*       path specifier does not contain a drive specifier.
*
* see also pc_setdfltdrvno()
*
* Entries:
*       None
*
* Returns
*    Return the current default drive.
*
******************************************************************************/
INT16 pc_getdfltdrvno ( SDVOID ) /*__fn__*/
{
    CHECK_MEM(COUNT, 0)     /* Make sure memory is initted */
	return(fs_user->dfltdrv);
}


SDBOOL pc_inside_subdirectory (DROBJ *pobj) /*__fn__*/
{
    DROBJ *pcwd;
    UINT32 clusterno;
    UINT32 clusternum;

    pcwd = fs_user->lcwd[pobj->pdrive->driveno];
    if (pcwd == SDNULL)
	return (NO);                

    clusterno = pc_sec2cluster(pobj->pdrive, pobj->blkinfo.my_frstblock);

    pcwd = fs_user->lcwd[pobj->pdrive->driveno];
    clusternum = pc_sec2cluster(pcwd->pdrive, pcwd->blkinfo.my_frstblock);

    if (clusterno != clusternum)
	return (YES);

    return (NO);                
}


/*****************************************************************************
* Name: PC_GET_CWD -  Get the current working directory for a drive,
*
* Description:
*       Return the current directory inode for the drive represented
*       by ddrive.
*       Get the current working directory and copy it into pobj.
*         
* Entries:
*       DDRIVE *pdrive
*
* Returns:
*
******************************************************************************/
DROBJ *pc_get_cwd ( DDRIVE *pdrive ) /*__fn__*/
{
    DROBJ *pcwd;
    DROBJ *pobj;

    pobj = SDNULL;

    pcwd = fs_user->lcwd[pdrive->driveno];

    /* If no current working dir set it to the root */
    if ( !pcwd )
    {
	pcwd = pc_get_root(pdrive);
	fs_user->lcwd[pdrive->driveno] = pcwd;
    }

    if ( pcwd )
    {
	pobj = pc_allocobj();
	if ( !pobj )
	{
	    return (SDNULL);
	}
	/* Free the inode that comes with allocobj */
	pc_freei(pobj->finode);

	PC_ENTER_CRITICAL()

	    copybuff(pobj, pcwd, sizeof(DROBJ));

	pobj->finode->opencount += 1;
	PC_EXIT_CRITICAL()

	    return (pobj);
    }

    /* If no cwd is set error */
    return (SDNULL);
}


#if (RTFS_SUBDIRS)
/******************************************************************************
* Name: PC_SET_CWD -  Set the current working directory for a drive.
*
* Description:
*       Find path. If it is a subdirectory make it the current working 
*       directory for the drive.
*
* Entries:
*       TEXT    name
*
* Returns:
*       Returns yes if the current working directory was changed.
*
*       If pc_set_cwd fails, fs_user->p_errno will be set to one of
*       the following:
*               PENOENT     - path to new directory not found
*
******************************************************************************/
SDBOOL pc_set_cwd ( TEXT *name ) /*__fn__*/
{
    DROBJ   *pobj;
    DROBJ   *parent_obj;
    TEXT    *path;
    SDBOOL  ret_val;
    INT16   driveno;
    INT16   tmpLength;
    TEXT    fileext[4];
    TEXT    filename[10];
    INT16   p_errno;

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    parent_obj = SDNULL;
    pobj = SDNULL;
    p_errno = 0;
    ret_val = NO;
    p_errno = PENOENT;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }

    PC_DRIVE_ENTER(driveno, NO)

	path = (TEXT *)fspath;
    path[0] = 0;
    /* Get out the filename and d:parent */
    if ( pc_parsepath( path, filename, fileext, name) )
    {
	if ( longFileName )
	{
	    tmpLength = (INT16)(longFileName - path);
	    saveFileName = name + tmpLength;
	}
	else
	{
	    saveFileName = SDNULL;
	}

	/* Find the parent and make sure it is a directory  */
	parent_obj = pc_fndnode(path);
	if ( !parent_obj ||
	     !pc_isadir(parent_obj) ||
	     pc_isavol(parent_obj) )
	{
	    goto errex;
	}
    }

    /* Get the directory */
    if ( filename[0] == 0 || filename[0] == 0x20 )
    {
	pobj = parent_obj;
    }
    else if ( pc_isdot( filename, fileext ))
    {
	pobj = parent_obj;
    }
    else
    {
	longFileName = saveFileName;

	/* Lock the parent */   
	PC_INODE_ENTER(parent_obj->finode, YES)
	    pobj = pc_get_inode(SDNULL,
				parent_obj,
				filename,
				fileext);
	PC_INODE_EXIT(parent_obj->finode)

	    /* If the request is cd .. then we just found
	       the .. directory entry.
	       we have to call get_mom to access the parent.
	    */
	    if ( pobj &&
		 pc_isdotdot(filename, fileext) )
	    {
		pc_freeobj(parent_obj);
		parent_obj = pobj;

		/* Find parent_obj's parent. By looking back from ".." */
		pobj = pc_get_mom(parent_obj);
	    }  
	pc_freeobj(parent_obj);
    }

    if ( pobj && pc_isadir(pobj) )
    {

	driveno = pobj->pdrive->driveno;
	if ( fs_user->lcwd[driveno] != SDNULL )
	    pc_freeobj(fs_user->lcwd[driveno]);

	fs_user->lcwd[driveno] = pobj;

	ret_val = YES;
	p_errno = 0;
    }
    else
    {
	if ( pobj )
	    pc_freeobj( pobj );
    }



  errex:
    PC_DRIVE_EXIT(driveno)

	errno = p_errno;

    /* Restore the kernel state */
    PC_FS_EXIT()
	return (ret_val);
}
#endif /* (RTFS_SUBDIRS) */



SDBOOL pc_is(INT16 op, TEXT *path);

#define ISDIR 1
#define ISVOL 2

/*****************************************************************************
* Name: PC_IS - Test if a path name is a directory or a volume name
*
* Description:
*       Test to see if a path specification ends at a subdirectory or a
*       volume name.
*
* Note: "\" is a directory.
*
* Entries:
*       TEXT   *path
*
* Returns:
*       Returns file attribute in pobj->finode->fattribute.
*
******************************************************************************/
SDBOOL pc_is(INT16 op, TEXT *path) /*__fn__*/
{
    DROBJ   *pobj;
    SDBOOL  ret_val;
    INT16   driveno;

    CHECK_MEM(SDBOOL, 0)    /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)   /* Check if a valid user if multitasking */


	ret_val = NO;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(path);
    if ( driveno >= 0 )
    {
	PC_DRIVE_ENTER(driveno, NO)

	    pobj = pc_fndnode(path);
	if ( pobj )
	{
	    PC_INODE_ENTER(pobj->finode, YES)

		if ( op == ISDIR )
		    ret_val = pc_isadir(pobj);
		else if ( op == ISVOL )
		    ret_val = pc_isavol(pobj);

	    PC_INODE_EXIT(pobj->finode)

		pc_freeobj(pobj);
	}

	PC_DRIVE_EXIT(driveno)
	    }

    PC_FS_EXIT()

	return (ret_val);
}


/*****************************************************************************
* Name: PC_ISDIR - Test if a path name is a directory 
*
* Description:
*       Test to see if a path specification ends at a subdirectory or a
*       volume name.
*
* Note: "\" is a directory.
*
* Entries:
*       TEXT   *path
*
* Returns:
*       Returns file attribute in pobj->finode->fattribute & ADIRRENT
*         or pobj->isroot.
*
******************************************************************************/
SDBOOL pc_isdir(TEXT *path) /*__fn__*/
{
    return(pc_is(ISDIR, path));
}


/*****************************************************************************
* Name: PC_ISVOL - Test if a path name is a directory 
*
* Description:
*       Test to see if a path specification ends at a subdirectory or a
*       volume name.
*
* Entries:
*       TEXT   *path
*
* Returns:
*       Returns file attribute in pobj->finode->fattribute & AVOLUME.
*       Returns YES if it is a volume name.
*
******************************************************************************/
SDBOOL pc_isvol(TEXT *path) /*__fn__*/
{
    return (pc_is(ISVOL, path));
}


#if (RTFS_SUBDIRS)
SDLOCAL SDBOOL pc_gm_name(TEXT *path, DROBJ *pmom, DROBJ *pdotdot);
SDLOCAL SDBOOL pc_l_pwd(TEXT *path, DROBJ *pobj);


/*****************************************************************************
* Name: PC_GM_NAME
*
* Description
*        Get the name of the current directory by searching the parent
*        for an inode with cluster matching the cluster value in ".."
*
* Entries:
*        TEXT  *path
*        DROBJ *parent_obj;
*        DROBJ *pdotdot;
*
* Returns
*       Returns the path name in path. The function returns YES on success
*       Return NO if something went wrong.
*
******************************************************************************/
SDLOCAL SDBOOL pc_gm_name(TEXT *path, DROBJ *parent_obj, DROBJ *pdotdot) /*__fn__*/
{
    DROBJ   *pchild;
    UINT32  clusterno;
    UINT32  fcluster;
    SDBOOL  ret_val;
    TEXT    asterixExt[4];
    TEXT    asterixName[10];

    ret_val = NO;

    clusterno = pc_sec2cluster(pdotdot->pdrive,
			       pdotdot->blkinfo.my_frstblock);

    if (pdotdot)
	pc_freeobj(pdotdot);

    pc_memfill(asterixName, (8 * sizeof(UTEXT)), 0x20);
    asterixName[0] = '*';
    asterixName[8] = 0;
    pc_memfill(asterixExt, (3 * sizeof(UTEXT)), 0x20);
    asterixExt[0] = '*';
    asterixExt[3] = 0;

    pchild = pc_get_inode(SDNULL,
			  parent_obj,
			  (TEXT *)asterixName,
			  (TEXT *)asterixExt);

    if ( pchild )
    {
	do 
	{
	    fcluster = (UINT32)pchild->finode->fclusterHi;
	    fcluster <<= 16;
	    fcluster |= (UINT32)pchild->finode->fcluster;
	    if ( fcluster == clusterno )
	    {
		/* Build a file spec (xxx.yyy) from a file name and extension */
		pc_mfile ((TEXT*)path,
			  (TEXT*)pchild->finode->fname,
			  (TEXT*)pchild->finode->fext);
		ret_val = YES;
		break;
	    }
	}
	while (pc_get_inode(pchild,
			    parent_obj,
			    (TEXT *)asterixName,
			    (TEXT *)asterixExt));
    }

    if ( pchild )
    {
	pc_freeobj(pchild);
    }

    return (ret_val);
}



/*****************************************************************************
* Name: PC_L_PWD
*
* Description
*        Recursively get the name of the current directory by searching
*        the parent for an inode with cluster matching the cluster value
*        in "..".  Then concat the filename to the path.
*
* Entries:
*        TEXT  *path
*        DROBJ *pobj;
*
* Returns
*       Returns the path name in path. The function returns YES on success
*       Return NO if something went wrong.
*
******************************************************************************/
SDLOCAL SDBOOL pc_l_pwd(TEXT *path, DROBJ *pobj) /*__fn__*/
{
    DROBJ   *pchild;
    TEXT    lname[12];

    if ( pobj->isroot )
    {
	*path++ = (TEXT)char_backslash;
	*path = 0;
	pc_freeobj(pobj);
	return (YES);
    }

    /* Find '..' so we can find the parent */
    pchild = pc_get_inode(SDNULL,
			  pobj,
			  (TEXT*)string_padded_dot_dot,
			  (TEXT*)string_3_spaces);
    if ( !pchild )
    {
	pc_freeobj(pobj);
	return (NO);
    }

    pc_freeobj(pobj);

    /* Get the parent directory */
    pobj = pc_get_mom(pchild);
    if ( !pobj )
    {
	pc_freeobj(pchild);
	return (NO);
    }

    /*
      Get the name of the current directory by searching the parent
      for an inode with cluster matching the cluster value in ".."
    */
    if ( !pc_gm_name(lname, pobj, pchild) )
    {
	pc_freeobj(pobj);
	return (NO);
    }

    /*
      Now recurse. when we return we tack our name on the end.
      This way the path string is built in the right order.
    */
    if ( pc_l_pwd(path, pobj) )
    {
	pc_strcat((TEXT *)path, (TEXT *)lname);
	pc_strcat((TEXT *)path, (TEXT *)string_backslash);
	return (YES);
    }

    return (NO);
}
#endif /* (RTFS_SUBDIRS) */


/*****************************************************************************
* Name: PC_PWD  -  Return a string containing the current working directory
*                  for a drive.
*
* Description
*       Fill in a string with the full path name of the current working
*       directory.
*       If *drive is null or an invalid drive specifier the default drive
*       is used.
*
* Entries:
*
* Returns
*       Returns the path name in path. The function returns YES on success
*       Return NO if something went wrong.
*
******************************************************************************/
SDBOOL pc_pwd ( TEXT *drive, TEXT *path ) /*__fn__*/
{
#if (RTFS_SUBDIRS)
    DDRIVE  *pdrive;
    DROBJ   *pobj;
    INT16   driveno;
    SDBOOL  ret_val;

    CHECK_MEM(SDBOOL, 0)    /* Make sure memory is initted */
	PC_FS_ENTER()           /* Must be last line in declarations */
	CHECK_USER(SDBOOL, 0)   /* Check if a valid user if multitasking */


	saveFileName = SDNULL;
    ret_val = NO;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(drive);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()            /* Restore the kernel state */
	    return(NO);
    }

    PC_DRIVE_ENTER(driveno, YES)    /* Lock the drive since we work
				       upwards in the directory tree */
	/* Find the drive */
	pdrive = pc_drno2dr(driveno);
    if ( pdrive )
    {
	pobj = pc_get_cwd(pdrive);
	if ( pobj )
	    ret_val = pc_l_pwd(path, pobj);
    }

    PC_DRIVE_EXIT(driveno)

	/* Restore the kernel state */
	PC_FS_EXIT()

	return (ret_val);
#else
    path[0] = char_backslash;
    path[1] = 0;

    return (YES);
#endif
}


/******************************************************************************
* Name: PC_FSTAT  -  Obtain statistics on an open file
*
* Description:
*    Fills in the stat buffer for information about an open file.
*
* See pc_stat for a description of the stat buffer.
*
* Entries:
*       PCFD fd
*       STAT *pstat
*
* Returns:
*       Returns 0 if all went well otherwise it returns -1 and
*       fs_user->p_errno is set to one of these values
*               PENBADF - Invalid file descriptor
*
******************************************************************************/
INT16 pc_fstat(PCFD fd, STAT *pstat) /*__fn__*/
{
    PC_FILE *pfile;
    INT16   ret_val;
    INT16   p_errno;

    CHECK_MEM(INT16, -1)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(INT16, -1)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    p_errno = 0;    

    /* Get the FILE. Write or read access okay */
    pfile = pc_fd2file(fd, 0);
    if ( !pfile )
    {
	p_errno = PEBADF;
	ret_val = -1;
    }
    else
    {
	/* cal pc_finode_stat() to update the stat structure */
	pc_finode_stat(pfile->pobj->finode, pstat);
	ret_val = 0;
    }

    /* Restore the kernel state */
    errno = p_errno;
    PC_FS_EXIT()
	return (ret_val);
}


/****************************************************************************
* Name: PC_STAT  -  Obtain statistics on a path.
*
* Description
*       This routine searches for the file or directory provided in the first 
*       argument. If found it fills in the stat structure as described here.
*
* Entries:
*       TEXT *name      
*       STAT *pstat
*               st_dev  -   The entry's drive number
*               st_mode;        
*                       S_IFMT  type of file mask 
*                       S_IFCHR character special (unused) 
*                       S_IFDIR directory 
*                       S_IFBLK block special   (unused) 
*                       S_IFREG regular         (a "file")  
*                       S_IWRITE    Write permitted
*                       S_IREAD Read permitted.
*               st_rdev -   The entry's drive number
*               st_size -   file size
*               st_atime    -   creation date in DATESTR format
*               st_mtime    -   creation date in DATESTR format
*               st_ctime    -   creation date in DATESTR format
*               t_blksize   -   optimal blocksize for I/O (cluster size)
*               t_blocks    -   blocks allocated for file 
*               fattributes -   The DOS attributes. This is non-standard but
*                               supplied if you want to look at them
*
* Returns:
*       Returns 0 if all went well otherwise it returns -1 and
*       fs_user->p_errno is set to one of these values
*               PENOENT
*
******************************************************************************/
INT16 pc_stat(TEXT *name, STAT *pstat) /*__fn__*/
{
    DROBJ   *pobj;
    INT16   driveno;
    INT16   ret_val;
    INT16   p_errno;

    CHECK_MEM(INT16, -1)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(INT16, -1)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    ret_val = -1; /* Assume an error to start */
    errno = 0;
    p_errno = PENOENT;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(name);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return -1;
    }

    PC_DRIVE_ENTER(driveno, NO) /* Register access to the drive */

	pobj = pc_fndnode(name);
    if ( pobj )
    {       
	PC_INODE_ENTER(pobj->finode, NO)

	    /* cal pc_finode_stat() to update the stat structure */
	    pc_finode_stat(pobj->finode, pstat);
	PC_INODE_EXIT(pobj->finode)
	    p_errno = 0;
	ret_val = 0;
    }

    if ( pobj )
	pc_freeobj(pobj);

    PC_DRIVE_EXIT(driveno)

	errno = p_errno;

    /* Restore the kernel state */
    PC_FS_EXIT()

	return (ret_val);
}


/*****************************************************************************
    pc_get_attributes - Get File Attributes  

 Description
    Given a file or directory name return the directory entry attributes 
    associated with the entry.

    The following values are returned:

    BIT Nemonic
    0       ARDONLY
    1       AHIDDEN
    2       ASYSTEM
    3       AVOLUME 
    4       ADIRENT
    5       ARCHIVE
    6-7 Reserved

 Returns
    Returns YES if successful otherwise it returns NO and fs_user->p_errno
    is set to one of these values

    PENOENT

****************************************************************************/
SDBOOL pc_get_attributes(TEXT *path, UINT16 *p_return) /*__fn__*/
{
    DROBJ   *pobj;
    TEXT    *ppath;
    SDBOOL  ret_val;
    INT16   p_errno;
    INT16   driveno;
    INT16   index;

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    ret_val = NO;
    p_errno = PENOENT;

    /* Get the drive and make sure it is mounted */
    driveno = check_drive(path);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }   

    PC_DRIVE_ENTER(driveno, NO)     /* Register access to the drive */

	ppath = (TEXT *)fspath;
    ppath[0]=0;

    for (index=0; path[index] != 0x00; index++)
	ppath[index] = path[index];
    ppath[index] = 0;

    pobj = pc_fndnode(ppath);
    if ( pobj )
    {
	PC_INODE_ENTER(pobj->finode, NO)
	    *p_return = (UINT16)pobj->finode->fattribute;
	PC_INODE_EXIT(pobj->finode)

	    pc_freeobj(pobj);
	ret_val = YES;
	p_errno = 0;
    }
    PC_DRIVE_EXIT(driveno)

	errno = p_errno;
    PC_FS_EXIT()

	return (ret_val);
}


#if (RTFS_WRITE)
/*****************************************************************************
* Name: pc_set_attributes - Set File Attributes  

 Description
    Given a file or directory name return set directory entry attributes 
    associated with the entry.

    The following values may be set:

    BIT Nemonic
    0       ARDONLY
    1       AHIDDEN
    2       ASYSTEM
    5       ARCHIVE

    Note: bits 3 & 4 (AVOLUME,ADIRENT) may not be changed.


 Returns
    Returns YES if successful otherwise it returns NO and fs_user->p_errno
    is set to one of these values

    PENOENT - Entry not found
    PENOSPC - Write failed

****************************************************************************/
SDBOOL pc_set_attributes(TEXT *path, UINT16 attributes) /*__fn__*/
{
    DROBJ   *pobj;
    TEXT    *ppath;
    SDBOOL  ret_val;
    INT16   p_errno;
    INT16   driveno;
    INT16   index;

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    p_errno = PENOENT;
    ret_val = NO;

    driveno = check_drive(path);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }   

    PC_DRIVE_ENTER(driveno, NO) /* Register access to the drive */

	ppath = (TEXT *)fspath;
    ppath[0]=0;

    for (index = 0; path[index] != 0x00; index++)
	ppath[index] = path[index];
    ppath[index] = 0;
	
    pobj = pc_fndnode(ppath);
    if ( pobj )
    {
	PC_INODE_ENTER(pobj->finode, YES)

	    /* Change the attributes if legal */
	    index = (INT16)pobj->finode->fattribute;
	if ( (attributes & (AVOLUME|ADIRENT)) == 
	     (UINT16)(index & (AVOLUME|ADIRENT)) )
	{
	    pobj->finode->fattribute = (UTINY)attributes;

	    /* Overwrite the existing inode. Don't set archive/date*/
	    ret_val = pc_update_inode(pobj, NO, NO);
	    if ( ret_val )
		p_errno = 0;
	    else
		p_errno = PENOSPC;
	}

	PC_INODE_EXIT(pobj->finode)
	    pc_freeobj(pobj);
    }

    PC_DRIVE_EXIT(driveno)
	errno = p_errno;
    PC_FS_EXIT()
	return (ret_val);
}

/*****************************************************************************
* Name: pc_touch - Update last access time

 Description
    Given a file or directory name return set directory entry attributes 
    to curren time.

 Returns
    Returns YES if successful otherwise it returns NO and fs_user->p_errno
    is set to one of these values

    PENOENT - Entry not found
    PENOSPC - Write failed

****************************************************************************/
SDBOOL pc_touch(TEXT *path) /*__fn__*/
{
    DROBJ   *pobj;
    TEXT    *ppath;
    SDBOOL  ret_val;
    INT16   p_errno;
    INT16   driveno;
    INT16   index;

    CHECK_MEM(SDBOOL, 0)      /* Make sure memory is initted */
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)     /* Check if a valid user if multitasking */

	saveFileName = SDNULL;
    p_errno = PENOENT;
    ret_val = NO;

    driveno = check_drive(path);
    if ( driveno < 0 )
    {
	PC_FS_EXIT()
	    return (NO);
    }   

    PC_DRIVE_ENTER(driveno, NO) /* Register access to the drive */

	ppath = (TEXT *)fspath;
    ppath[0]=0;

    for (index = 0; path[index] != 0x00; index++)
	ppath[index] = path[index];
    ppath[index] = 0;
	
    pobj = pc_fndnode(ppath);
    if ( pobj )
    {
	PC_INODE_ENTER(pobj->finode, YES)

	    /* Overwrite the existing inode. Don't set archive */
	    ret_val = pc_update_inode(pobj, NO, YES);
	if ( ret_val )
	    p_errno = 0;
	else
	    p_errno = PENOSPC;

	PC_INODE_EXIT(pobj->finode)
	    pc_freeobj(pobj);
    }

    PC_DRIVE_EXIT(driveno)
	errno = p_errno;
    PC_FS_EXIT()
	return (ret_val);
}
#endif



/*****************************************************************************
* Name: PC_CLUSTER_SIZE  - Return the number of bytes per cluster for a drive
*
* Description:
*        This function will return the cluster size mounted device
*        named in the argument. 
*
* Entries:
*       INT16   driveno         Drive Number
*
* Returns:
*       The cluster size or zero if the device is not mounted.
*
*****************************************************************************/
COUNT pc_cluster_size(INT16 driveno) /*__fn__*/
{
    DDRIVE *pdrive;

    CHECK_MEM(COUNT, 0)     /* Make sure memory is initted */
	CHECK_USER(COUNT, 0)    /* Check if a valid user if multitasking */


	/* get drive no */
	pdrive = pc_drno2dr(driveno);
    if ( pdrive )
	return (pdrive->bytespcluster);

    return (0);
}

SDBOOL pc_get_volume_label(TEXT * name, TEXT * volume_label)
{
    DDRIVE * pdr;
    INT16 driveno;
    int i;
    
    CHECK_MEM(SDBOOL, 0)
	PC_FS_ENTER()
	CHECK_USER(SDBOOL, 0)

	driveno = check_drive(name);
    if (driveno < 0) {
	PC_FS_EXIT()
	    return NO;
    }

    PC_DRIVE_ENTER(driveno, NO)
	pdr = &mem_drives_structures[driveno];
    for (i = 0; i < 12; ++i) {
	volume_label[i] = pdr->volume_label[i];
    }
    PC_DRIVE_EXIT(driveno)
	
	PC_FS_EXIT()
	return YES;
}
    

#endif    /* (USE_FILE_SYSTEM) */


