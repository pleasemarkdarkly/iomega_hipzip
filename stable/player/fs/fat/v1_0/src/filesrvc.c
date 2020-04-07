/******************************************************************************
* File Name: FILESRVC.C - Contains code for file structure source code.
*
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* All rights reserved.
*
* This code may not be redistributed in source or linkable object
* form without the consent of its author.
*
* The following routines are included:
*
*    pc_fd2file       -   Map a file descriptor to a file structure.
*    pc_allocfile     -   Allocate a file structure.
*    pc_freefile      -   Release a file structure.
*    pc_free_all_file -   Release all file structures for a drive.
*
******************************************************************************/

#include "pcdisk.h"

#if (USE_FILE_SYSTEM)


#define ENUM_FLUSH 1
#define ENUM_TEST  2 
#define ENUM_FREE  3


/*
    Each mounted drive needs core to store either the whole FAT in core
    or to store portions of it in core with swapping to disk. 
    These declarations preallocate buffers in the BSS for this purpose.
*/

SDLOCAL INT16 pc_enum_file(DDRIVE *pdrive, INT16 chore);


/****************************************************************************
    Miscelaneous File and file descriptor management functions

    These functions are private functions used by the po_ file io routines.

    pc_fd2file -
        Map a file descriptor to a file structure. Return null if the file is
        not open. If an error has occured on the file return NULL unless 
        allow_err is true.

    pc_allocfile -
        Allocate a file structure an return its handle. Return -1 if no more 
        handles are available.

    pc_freefile -
        Free all core associated with a file descriptor and make the descriptor
        available for future calls to allocfile.

    pc_free_all_file -
*****************************************************************************/

/******************************************************************************
* Name: pc_fd2file
*
* Description:
*       Map a file descriptor to a file structure. Return null if the file is
*       not open or the flags don't match (test for write access if needed). 
*
* Entries:
*       PCFD fd
*       INT16 flags
*
* Returns:
*
******************************************************************************/
PC_FILE *pc_fd2file ( PCFD fd, INT16 flags ) /*__fn__*/
{
        PC_FILE *pfile;
        PC_FILE *pret_val;

        PC_ENTER_CRITICAL()
        pret_val = SDNULL; 

        if ( (0 <= fd) && (fd <= pc_nuserfiles()) )
        {
                pfile = &mem_file_pool[fd];
                if ( pfile && !pfile->is_free )
                {
                        /* If flags == 0. Any access allowed. Otherwise at
                           least one bit in the file open flags must match
                           the flags sent in.
                        */
                        if ( !flags || (pfile->flag & flags) )
                        {
                                pret_val = pfile;
                        }
                }
        }

        PC_EXIT_CRITICAL()
        return (pret_val);
}


/*****************************************************************************
* Name: pc_allocfile
*
* Description:
*       Assign zeroed out file structure to an FD and return the handle.
*
* Entries:
*       None
*
* Returns:
*       -1 on error.
*
******************************************************************************/
PCFD pc_allocfile ( SDVOID ) /*__fn__*/
{
        PC_FILE *pfile;
        PCFD i;

        PC_ENTER_CRITICAL()
        pfile = mem_file_pool;

        for (i = 0; i < pc_nuserfiles(); i++, pfile++)
        {   
                if ( pfile->is_free )
                {
                        pc_memfill(pfile, sizeof(PC_FILE), (UTINY)0);
                        PC_EXIT_CRITICAL()
                        return (i);
                }
        }
        PC_EXIT_CRITICAL()

        return (-1);
}


/*****************************************************************************
* Name: pc_freefile
*
* Description:
*       Free core associated with a file descriptor. Release
*       the FD for later use.
*
* Entries:
*       PCFD fd
*
* Returns:
*       None
*
******************************************************************************/
SDVOID pc_freefile ( PCFD fd ) /*__fn__*/
{
        PC_FILE *pfile;

        if ( (pfile = pc_fd2file(fd, 0)) == SDNULL )
                return;

        if ( pfile->pobj )
                pc_freeobj(pfile->pobj);

        pfile->is_free = YES;
}


/*****************************************************************************
* Name: pc_enum_file
*
* Description:
*       Release all file descriptors associated with a drive and
*       free up all core associated with the files
*
* Note: called by dsk_close
*
* Entries:
*       DDRIVE *pdrive
*       INT16 chore
*
* Returns:
*
******************************************************************************/
SDLOCAL INT16 pc_enum_file ( DDRIVE *pdrive, INT16 chore ) /*__fn__*/
{
        PC_FILE *pfile;
        PCFD    i, nuserfiles;
        INT16   dirty_count;

        nuserfiles = (PCFD)pc_nuserfiles();
        dirty_count = 0;

        for (i = 0; i < nuserfiles; i++)
        {
                pfile = pc_fd2file(i, 0);
                if ( pfile )
                {
                        if ( (pfile->pobj) && (pfile->pobj->pdrive == pdrive) )
                        {
                                /*
                                Print a debug message since in normal operation
                                all files should be close closed before closing
                                the drive.
                                */
#if (RTFS_WRITE)
                                if ( chore == ENUM_FLUSH )
                                {
                                        if ( !_po_flush(pfile) )
                                                return (-1);
                                }
#endif
                                if ( chore == ENUM_TEST )
                                {
                                        if ( pfile->needs_flush )
                                                dirty_count += 1;
                                }

                                if ( chore == ENUM_FREE )
                                {
                                        pc_freefile(i);
                                }
                        }
                }
        }

        return (dirty_count);
}


/*****************************************************************************
* Name: pc_free_all_files
*
* Description:
*       Release all file descriptors associated with a drive and
*       free up all core associated with the files called by dsk_close 
*
* Entries:
*       DDRIVE *pdrive
*
* Returns:
*       None
*
******************************************************************************/
SDVOID pc_free_all_files ( DDRIVE *pdrive ) /*__fn__*/
{
        pc_enum_file( pdrive, ENUM_FREE );
}



#if (RTFS_WRITE)
/*****************************************************************************
* Name: pc_flush_all_files
*
* Description:
*       Flush all files on a drive
*
* Entries:
*       DDRIVE *pdrive
*
* Returns:
*       YES if successful
*       NO if failure
*
******************************************************************************/
SDBOOL pc_flush_all_files ( DDRIVE *pdrive ) /*__fn__*/
{
        if ( pc_enum_file(pdrive, ENUM_FLUSH) == 0 )
                return (YES);

        return (NO);
}
#endif  /* (RTFS_WRITE) */


/******************************************************************************
* Name: pc_test_all_files
*
* Description:
*       Test the dirty flag for all files.
*
* Entries:
*       DDRIVE *pdrive
*
* Returns:
*
******************************************************************************/
INT16 pc_test_all_files ( DDRIVE *pdrive ) /*__fn__*/
{
        return( pc_enum_file(pdrive, ENUM_TEST) );
}

#endif  /* (USE_FILE_SYSTEM) */


