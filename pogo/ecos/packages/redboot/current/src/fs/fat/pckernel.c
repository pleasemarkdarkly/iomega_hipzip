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
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright EBS Inc. 1996
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
******************************************************************************/
/******************************************************************************
    PC_KERNEL - System specific process management routines.

Summary
    Routines in this file provide system specific process management 
    functionality. 

    Required functions:
        pc_kernel_init          - Initialize process management system
        pc_kernel_shutdown      - Release process management system
        pc_kernel_init_lockobj  - Attach a lock management capability
                                    to a finode (directory entry)
        pc_kernel_free_lockobj  - Release a lock management capability
                                    from a finode 
        Note: pc_kernel_init_lockobj  and  pc_kernel_free_lockobj will be
                called by RTFS. If building a single threaded non multitasking
                version or a single threaded version with a reentrant API you
                may just implement these functions as dummies.
        __fs_user()             - Return the user structure for the current task
        pc_free_all_users()     - Detach all tasks from a drive
        PC_GETSYSDATE()         - Get system date and put in rtfs format


    Functions required in multitasking systems:
        The macros in pckernel.h will presumably be defined to call routines
        in this file to implement the functionality. If the macros can be 
        implemented to call kernel services directly that is fine too.

    The macros to be implemented are:

    PC_FS_ENTER     - This macro is invoked each time rtfs is entered
    PC_FS_EXIT          - This macro is invoked each time rtfs exits
    CHECK_USER          - Verify that the current task is a valid RTFS user
    VOID_CHECK_USER - Verify that the current task is a valid RTFS user
    PC_DRIVE_ENTER      - Claim exclusive or shared access to a drive
    PC_DRIVE_EXIT       - Release exclusive or shared access to a drive
    PC_INODE_ENTER      - Claim exclusive or shared access to a directory entry
    PC_INODE_EXIT       - Release exclusive or shared access to a dir. entry
    PC_FAT_ENTER        - Claim exclusive access to a file allocation table
    PC_FAT_EXIT     - Release exclusive access to a file allocation table
    PC_BP_ENTER     - Claim exclusive access to a drive's buffer pool
    PC_BP_EXIT          - Release exclusive access to a drive's buffer pool
    PC_DRIVE_IO_ENTER   - Claim exclusive access to a device driver
    PC_DRIVE_IO_EXIT    - Release exclusive access to a device driver
    PC_ENTER_CRITICAL   - Claim exclusive access RTFS critical code
    PC_EXIT_CRITICAL    - Release exclusive access RTFS critical code

In this file we implement the required functions for a single tasking system. 


******************************************************************************/

#include <fs/fat/pcdisk.h>

#if (USE_FILE_SYSTEM)

/* SDBOOL pc_kernel_init()            - Initialize process management system
*
*  Initialize the run time system so the kernel support services will be
*  available when requested.
*
*   When this routine is finished executing the following process resources
*   should be functional.
*
*   In any implementation including single tasking single threaded:
*       __fs_user() should be prepared to return a user structure
*       pc_free_all_users() should be functional
*
*   In an implementation where a reentrant API is required but RTFS may run
*   in a single threaded non reentrant manner:
*   
*   The macro PC_FS_ENTER() should be able to claim a MUTEX semaphore and
*   the macro PC_FS_EXIT() should be able to release it. How these macros are
*   implemented is system dependant. It is assumed that they call routines in 
*   this module and that pc_kernel_init() initializes the semaphore they act on
*   but the macros may be implemented to call kernel services directly as well
*   assuming the semaphore already exists.
*   For this mode of operation the routines __fs_user(), pc_free_all_users()
*   and the macros CHECK_USER and VOID_CHECK_USER should also be implemented.
*
*   In an implementation where a reentrant API is required and where RTFS
*   yields the processor during IO calls and is preemptable the following
*   resources should be available.
*
*   Note: Two types of semaphores need to be implemented. One type is a simple
*           MUTEX semaphore where the requesting task blocks until the owning 
*           task releases the semaphore. The second type is one where the access 
*           request may be for either shared or exclusive access. When a task
*           requests shared access it will be granted access regardless of
*           how many tasks have shared access claimed, however if a task has
*           the resource claimed for exclusive access any task requesting 
*           shared or exclusive access will block until the resource is released.
*           Similarly, a task requesting exclusive access will block until all
*           tasks with shared access claims release their claims. We will use the
*           term SHAREEX semaphore as a shorthand for this object.
*   One SHAREEX semaphore per supported drive (see NDRIVES in pckernel.h)  
*   these semaphores are claimed by PC_DRIVE_ENTER and release by 
*   PC_DRIVE_EXIT. The macros take the drive number (0 to NDRIVES-1) as
*   arguments.
*
*   One SHAREEX semaphore per concurrently accessable directory entry
*   (see NFINODES in pckernel.h) these semaphores are claimed by 
*   PC_FINODE_ENTER and released by PC_FINODE_EXIT. The semaphore must
*   be bound to the system specific lock_object field of the finode 
*   structure by pc_kernel_init_lockobj.
* 
*   One MUTEX semaphore per supported drive (see NDRIVES in pckernel.h)  
*   these semaphores are claimed by PC_FAT_ENTER and released by PC_FAT_EXIT
*   The macros take the drive number (0 to NDRIVES-1) as arguments.
*
*   One MUTEX semaphore per supported drevice driver
*   (see NDRIVES in pckernel.h) these semaphores are claimed by
*   PC_DRIVE_IO_ENTER and released by PC_DRIVE_IO_EXIT. The macros take
*   the lock_no field from the device table (see devtable.c). If two 
*   device entry points share the same device driver then they should 
*   share a lock no field otherwise the value should be unique.
*
*   One MUTEX semaphore to control access to critical code sections
*   The macros PC_ENTER_CRITICAL and PC_EXIT_CRITICAL should be implemented
*   such that access to RTFS critical code sections is exclusive to a task who
*   has called PC_ENTER_CRITICAL but not yet called PC_EXIT_CRITICAL. 
*   This can be done with a simple task lock or preferably with a 
*   MUTEX semaphore. 
*
*       Note: pc_kernel_init() is called by the RTFS startup routine 
*       pc_memory_init(). The task thread must be locked when
*       pc_memory_init() is called since PC_ENTER_CRITICAL is assumed to not
*       be  functional until after the function has returned. 
* 
*/


/* Pc_kernel_init - Initialize kernel resources.
 *  For the reference port we set up a user structure. That will be
 *  returned by __fs_user(). 
 *  We use an array _user_heap[NUM_USERS]. NUM_USERS is one for the reference
 *  port but we use the array to show a possible path to providing
 *  support for more then on task.
 */ 
SDIMPORT FILE_SYSTEM_USER _user_heap[NUM_USERS];

#if defined(THREAD_SAFE_FS)
#include "pckernel.h"
cyg_mutex_t fs_mutex;
static cyg_ucount32 valid_data_index;
#endif

SDBOOL pc_kernel_init(SDVOID) /* __fn__ */
{
    PC_SYNC_INIT()
        /* Initialize our user list - managed by code in pc_users.c */
        pc_memfill(_user_heap, sizeof(FILE_SYSTEM_USER)*NUM_USERS, (UTINY) 0);

#if defined(THREAD_SAFE_FS)
		valid_data_index = cyg_thread_new_data_index();
#endif
#if !defined(__DHARMA)
		DEBUG_HE("HE:VDI %d\n",valid_data_index);
#endif /* !__DHARMA */
		
        return(YES);
}

/* Pc_kernel_shutdown - Release kernel resources.
 *
 * This routine is only called if pc_memory_free() is called. Most embedded
 * systems won't need to do this so implementing the function is typically
 * unnecessary. If implementation is needed then the resources alocated
 * by pc_kernel_init() should be put in such a state that a subsequant call
 * to that function will succeed.
 *
 * */

void
pc_set_valid_read_data(unsigned int valid_read_data)
{
#if !defined(__DHARMA)
    DEBUG_HE("HE:setting valid_read_data %x\n", valid_read_data);
#endif /* !__DHARMA */
#if defined(THREAD_SAFE_FS)
    cyg_thread_set_data(valid_data_index, valid_read_data);
#endif
}

unsigned int
pc_get_valid_read_data(void)
{
#if defined(THREAD_SAFE_FS)
    unsigned int vrd;
    vrd = (unsigned int)cyg_thread_get_data(valid_data_index);
    return vrd;
#else
    return 0;
#endif
}

SDVOID pc_kernel_shutdown(SDVOID) /* __fn__ */
{
    pc_memfill(_user_heap, sizeof(FILE_SYSTEM_USER)*NUM_USERS, (UTINY) 0);
}

/* __fs_user() - Return a pointer to a user structure for the current task.
 *
 * This routine is called by RTFS when it needs to set an errno value or a
 * current working drive or directory or when it needs to query the current
 * working drive and or directory. 
 * 
 * In a multitasking system each task should have its own context stored 
 * in a user structure. It is up to this routine to map the task to the 
 * user structure. 
 * Suggested mapping algorithms might be to place a pointer to the user 
 * structure in the task control block or to embed the user structure in 
 * the task control block or to use a lookup table to map tasks to user
 * structures.
 *
 * The macros CHECK_USER or VOID_CHECK_USER are called when the API enters.
 * If every task is guaranteed a valid user structure then they can be left
 * unimplemented. Otherwise it is appropriate to link the task control block
 * to a user structure the first time CHECK_USER or VOID_CHECK_USER is called
 * for the task.
 *
 */

PFILE_SYSTEM_USER __fs_user(SDVOID) /* __fn__ */
{
    return((PFILE_SYSTEM_USER)&_user_heap[0]);
}


/* pc_free_all_users() - Run down the user list releasing drive resources
 *
 * This routine is called by RTFS when it closes a drive.
 * The routine must release the current directory object for that drive
 * for each user. If a user does not have a CWD for the drive it should 
 * not call pc_freeobj.
 *
 * In the reference port we cycle through our array of user structures 
 * to provide the enumeration. Other implementations are equally valid.
 */

SDVOID  pc_free_all_users(INT16 driveno) /*__fn__*/
{
    PFILE_SYSTEM_USER p; 
    INT16 i;

    p = &_user_heap[0];
    for (i = 0; i < NUM_USERS; i++, p++)
    {
        if (p->lcwd[driveno])
        {       
            pc_freeobj(p->lcwd[driveno]);
            p->lcwd[driveno] = SDNULL;
        }
    }
}

/* pc_kernel_init_lockobj() - Attach a SHAREEX semaphore to a FINODE structure.
 *
 * When the FINODE structures are first allocated by pc_memory_init() this 
 * routine is called once per allocated finode. Its job is to modify the
 * kernel dependent LOCK_OBJECT substructure of the finode structure so that
 * future calls to PC_INODE_ENTER(pfi) and PC_INODE_EXIT(pfi) will operate
 * on the SHAREEX semaphore accessible in the finode structure.
 *
 * The LOCK_OBJECT type is declared in pckernel.h. It is not referenced 
 * directly in any RTFS code so its semantics are private to 
 * pc_kernel_init_lockobj(),  pc_kernel_free_lockobj() PC_INODE_ENTER and
 * PC_INODE_EXIT. The routine should return YES if the resource is
 * available, NO if not.
 *
 * Note: This routine is always called by RTFS's pc_memory_init() routine.
 *      In a non multitasking environment it needn't do anything special
 *      except return YES.
 */

SDBOOL pc_kernel_init_lockobj(FINODE *pfi) /* __fn__ */
{
    pfi->lock_object.dummy = 1;
    return(YES);
}

/* pc_kernel_free_lockobj() - Detach a SHAREEX semaphore from a FINODE structure.
 *
 * This routine is only called if pc_memory_free() is called. Most embedded
 * systems won't need to do this so implementing the function is typically
 * unnecessary. If implementation is needed then the SHAREX resource attached 
 * to the FINODE should be released in such a way that a future call to  
 * pc_kernel_init_lockobj() will be able to reallocate the resource
 */

SDVOID pc_kernel_free_lockobj(FINODE *pfi) /* __fn__ */
{
    pfi->lock_object.dummy = 0;
}

#endif	/* (USE_FILE_SYSTEM) */


