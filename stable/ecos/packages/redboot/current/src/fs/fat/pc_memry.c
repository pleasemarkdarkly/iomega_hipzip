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
* FileName: PC_MEMRY - System specific memory management routines.
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
* Summary
*    All global memory management occurs in this file including:
*
*        .  Declaration of globals
*        .  Initialization of systemwide lists and tables.
*        .  Variable sized memory allocator functions for FATs.
*        .  Optional de-allocation of systemwide lists and tables.
*
******************************************************************************/

#include <fs/fat/pcdisk.h>

#if (USE_FILE_SYSTEM)

 
SDIMPORT DDRIVE  _mem_drives_structures[NDRIVES];
SDIMPORT BLKBUFF _mem_block_pool[NBLKBUFFS];
SDIMPORT PC_FILE _mem_file_pool[NUSERFILES];
SDIMPORT DROBJ   _mem_drobj_pool[NDROBJS];
SDIMPORT FINODE  _mem_finode_pool[NFINODES];

/* BSS area used by the block buffer pool system */
SDIMPORT UINT32  useindex;
SDIMPORT DDRIVE *scratch_pdrive;



/*****************************************************************************
* Name: PC_NUM_DRIVES -  Return total number of drives in the system
*
* Description
*       This routine returns the number of drives in the system 
*
* Entries:
*       None
*
* Returns
*       The number of devices installed.
*
******************************************************************************/
COUNT pc_num_drives ( SDVOID ) /* __fn__ */
{
        return (NDRIVES);
}

/*****************************************************************************
* Name: PC_NUSERFILES -  Total number of uses allowed in the system
*
* Description
*       This routine returns the number of user in the system   
*
* Returns
*       The number of users
*
******************************************************************************/
COUNT pc_num_users ( SDVOID ) /* __fn__ */
{
        return (NUM_USERS);
}

/*****************************************************************************
* Name: PC_NUSERFILES -  Total number of userfiles alloed in the system
*
* Description:
*       This routine returns the number of user files in the system 
*
* Entries:
*       None
*
* Returns:
*       The number of userfiles
*
******************************************************************************/
COUNT pc_nuserfiles ( SDVOID ) /* __fn__ */
{
        return (NUSERFILES);
}

/*****************************************************************************
* Name: PC_VALIDATE_DRIVENO -  Verify that a drive number is <= NDRIVES
*
* Description:
*       This routine is called when a routine is handed a drive number and
*       needs to know if it is within the number of drives set during
*       the congiguration.
*
* Entries:
*       INT16 driveno
*
* Returns
*    YES if the drive number is valid or NO.
******************************************************************************/
SDBOOL pc_validate_driveno ( INT16 driveno ) /* __fn__ */
{
        if ( (driveno < 0) || (driveno >= NDRIVES) )
                return (NO);

        return (YES);
}

/*****************************************************************************
* Name: PC_MEMORY_INIT -  Initialize and allocate File system structures.
*
*    THIS ROUTINE MUST BE CALLED BEFORE ANY FILE SYSTEM ROUTINES !!!!!!
*    IT IS CALLED AUTOMATICALLY BY THE API FUNCTIONS
*
* Description
*       This routine must be called before any file system routines. Its job
*       is to allocate tables needed by the file system. We chose to implement
*       memory management this way to provide maximum flexibility for embedded
*       system developers. In the reference port we use malloc to allocate the
*       various chunks of memory we need, but we could just have easily comiled
*       the tables into the BSS section of the program. 
*
*       Use whatever method makes sense in you system.
*
* Note: The total number of bytes allocated by this routine is:
*        (sizeof(DDRIVE) * NDRIVES) + (sizeof(PC_FILE)*NUSERFILES) +
*        (sizeof(BLKBUFF)*NBLKBUFFS)+ (sizeof(DROBJ)*NDROBJS) +
*        (sizeof(FINODE)*NFINODES)
*
* Entries:
*       None
*
* Returns
*       YES on success or no ON Failure.
*
******************************************************************************/
SDBOOL _pc_memory_init ( SDVOID ) /*__fn__*/
{
        DROBJ   *pobj;
        FINODE  *pfi;
        INT16   i;
        INT16   j;
        UINT16  l;

        /* mem_drives_structures will be set if we've already been here */
        if ( mem_drives_structures )
                return (YES);

#if 1//SANDISK_DOESNT_REALIZE_THAT_CONTROLLERS_ARE_NOT_MEMORY
        /* Initialize controllers */
        if ( !CONTROLLER_INIT() )
                return (NO);
#endif

        /* Zero all pointers before we initialize */
        null_pointers();

        /* Zero block buffer pool data */
        useindex = 0L;
        scratch_pdrive = SDNULL;

        /* Call the kernel specific initialization code */
        if ( !pc_kernel_init() )
        {
                return (NO);
        }

        /*
        =!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=
        We simply assign our pointers to the placeholders in the BSS
        =!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=
        */
        mem_drives_structures = (DDRIVE *)_mem_drives_structures;
        mem_block_pool  = (BLKBUFF *)_mem_block_pool;
        mem_file_pool   = (PC_FILE *)_mem_file_pool;
        mem_drobj_pool  = (DROBJ *)_mem_drobj_pool;
        mem_finode_pool = (FINODE *)_mem_finode_pool;

        /* Initialize the drive structures */
        l = sizeof(DDRIVE);
        l *= NDRIVES;
        pc_memfill( mem_drives_structures, (UCOUNT)l, (UTINY)0 );

        /* Initialize the block buffer array */
        for (i = 0, j = 1; i < (NBLKBUFFS-1); i++, j++)
        {
                pc_memfill( &mem_block_pool[i], sizeof(BLKBUFF), (UTINY)0 );
                mem_block_pool[i].data = &directory_buffer[i];
                mem_block_pool[i].pnext = &mem_block_pool[j];
        }
        pc_memfill( &mem_block_pool[NBLKBUFFS-1], sizeof(BLKBUFF), (UTINY)0 );
        mem_block_pool[NBLKBUFFS-1].pnext = SDNULL;
        mem_block_pool[NBLKBUFFS-1].data = &directory_buffer[(NBLKBUFFS-1)];

        /*
        make a NULL terminated freelist of the DROBJ pool using
        pdrive as the link. This linked freelist structure is used by the
        DROBJ memory allocator routine.
        */
        mem_drobj_freelist = mem_drobj_pool;
        for (i = 0,j = 1; i < NDROBJS-1; i++, j++)  
        {
                pobj = &mem_drobj_freelist[j];
                mem_drobj_freelist[i].pdrive = (DDRIVE *)pobj;
        }
        mem_drobj_freelist[NDROBJS-1].pdrive = (DDRIVE *)SDNULL;

        /*
        Make a NULL terminated FINODE freelist using
        pnext as the link. This linked freelist is used by the FINODE 
        memory allocator routine.
        */
        pfi = mem_finode_freelist = mem_finode_pool;
        for (i = 0; i < NFINODES-1; i++)    
        {
                pfi++;
                mem_finode_freelist->pnext = pfi;
                mem_finode_freelist++;
                mem_finode_freelist->pnext = SDNULL;
        }

        /* Call the kernel specific routine to initialize each
        finode's lock_object field. */
        pfi = mem_finode_freelist = mem_finode_pool;
        for (i = 0; i < NFINODES; i++, pfi++)   
                pc_kernel_init_lockobj(pfi);

        /* Mark all user files free */
        for (i = 0; i < NUSERFILES; i++)
                mem_file_pool[i].is_free = YES;

        mem_finode_freelist = mem_finode_pool;

        return (YES);
}



/**************************************************************************
* Name: PC_MEMORY_CLOSE -  Close out memory used by the file system.
*
* Description:
*       Free all memory used by the file system and make it ready to run
*       again. 
*
*       This routine is optional but might come in handy in applications where
*       you may need to swap the file system's core out so it may be used for
*       something else. 
*
* Note: We don't de-allocate any FAT buffers here. Calling pc_dskclose
*       for each open drive will do that.
*
* Entries:
*       None
*
* Returns:
*    None
*
******************************************************************************/
SDVOID pc_memory_close ( SDVOID ) /*__fn__*/
{
        /* Allow the kernel to recycle any resources initialized by pc_kernel_init() */
        pc_kernel_shutdown();
        null_pointers();
}

/*****************************************************************************
* Name: PC_MEMORY_DROBJ -  Allocate a DROBJ structure
*
* Description:
*       If called with a null pointer, allocates and zeroes the space needed
*       to store a DROBJ structure. If called with a NON-NULL pointer the
*       DROBJ structure is returned to the heap.
*
* Entries:
*       DROBJ *pobj
*
* Returns:
*       If an ALLOC returns a valid pointer or NULL if no more core. If
*       a free the return value is the input.
*
******************************************************************************/
DROBJ *pc_memory_drobj ( DROBJ *pobj ) /*__fn__*/
{
        DROBJ *preturn;

        preturn = SDNULL;
        if ( pobj )
        {
                /* Free it by putting it at the head of the freelist 
                NOTE: pdrive is used to link the freelist */
                PC_ENTER_CRITICAL()
                pobj->pdrive = (DDRIVE *)mem_drobj_freelist;
                mem_drobj_freelist = pobj;
                PC_EXIT_CRITICAL()
        }
        else
        {
                /* Alloc: return the first structure from the freelist */
                PC_ENTER_CRITICAL()
                preturn =  mem_drobj_freelist;
                if ( preturn )
                {
                        mem_drobj_freelist = (DROBJ *)preturn->pdrive;
                        pc_memfill(preturn, sizeof(DROBJ), (UTINY) 0);
                        PC_EXIT_CRITICAL()
                }
                else
                {
                        PC_EXIT_CRITICAL()
                        REPORT_ERROR(PCERR_DROBJALLOC);
                }
        }

        return (preturn);
}


/*****************************************************************************
* Name: PC_MEMORY_FINODE -  Allocate a FINODE structure
*
* Description:
*       If called with a null pointer, allocates and zeroes the space
*       needed to store a FINODE structure. If called with a NON-NULL
*       pointer the FINODE structure is returned to the heap.
*
* Entries:
*       FINODE *pinode
*
* Returns:
*       If an ALLOC returns a valid pointer or NULL if no more core. If
*       a free the return value is the input.
*
******************************************************************************/
FINODE *pc_memory_finode ( FINODE *pinode ) /*__fn__*/
{
        FINODE *preturn;
        LOCKOBJ save_lockobj; /* Temporary lock object for saving the finode's
                              lock_object while we zero the finode. We then copy
                              the data back */ 
        preturn = SDNULL;
        if ( pinode )
        {
                /* Free it by putting it at the head of the freelist */
                PC_ENTER_CRITICAL()
                pinode->pnext = mem_finode_freelist;
                mem_finode_freelist = pinode;
                PC_EXIT_CRITICAL()
                preturn = pinode;
        }
        else
        {
                /* Alloc: return the first structure from the freelist */
                PC_ENTER_CRITICAL()
                preturn =  mem_finode_freelist;
                if ( preturn )
                {
                        mem_finode_freelist = preturn->pnext;

                        /*
                        Zero the structure. lock_object can't be zeroed so
                        save it and restore it after zeroeing.
                        */

                        copybuff( &save_lockobj,
                                 &preturn->lock_object,
                                 sizeof(LOCKOBJ) );

                        pc_memfill( preturn, sizeof(FINODE), (UTINY)0 );

                        copybuff( &preturn->lock_object,
                                 &save_lockobj,
                                 sizeof(LOCKOBJ) );

                        PC_EXIT_CRITICAL()
                }
                else
                {
                        PC_EXIT_CRITICAL()
                        REPORT_ERROR(PCERR_FINODEALLOC);
                }
        }

        return (preturn);
}

#endif	/* (USE_FILE_SYSTEM) */


