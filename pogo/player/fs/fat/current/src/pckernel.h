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
* Filename: PCKERNEL.H - Kernel specific definitions plus tuning constants
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
*       This file is included in sdapi.h. It contains kernel 
*       specific macros and structures which must be ported 
*       to the target kernel.
*
*       You do not need to port this file unless you are porting 
*       RTFS to a multitasking environment.
*
****************************************************************************/

#ifndef __PCKERNEL__



#ifdef __cplusplus
extern "C" {
#endif

#if defined(THREAD_SAFE_FS)
#include <util/thread/Mutex.h>
#endif
  
#include <fs/fat/sdtypes.h>


/* ==================== MULTI TASKING SUPPORT ===================== */
/*
    Multitasking code is segregated in three modules. pckernel.h
    pckernel.c and miniexec.c. There are macros defined in this 
    file that are invoked throughout the source code. These macros
    should either do nothing (in a single threaded environment) 
    or invoke the appropriate kernel specific routine in kernel.c
*/

/*======================== MULTI SUPPORT MACROS ====================*/
/* 
    These macros are invoked at approprite times by the source code. In a 
    single tasking system the macros do nothing. In a multitasking system
    the macro should invoke that provides the appropriate functionality */

#if !defined(THREAD_SAFE_FS)

#define PC_SYNC_INIT()   /* dc - This is invoked once in the fat layer,
			    designed to initialize synchronization mechanisms
			 */
  
#define PC_FS_ENTER() /* This is invoked at the top of each API call. In 
			 a single task environment it does nothing. In a
			 multitasking system you may choose to have this
			 macro claim a mutual exclusive semaphore that 
			 controls access to RTFS. If you choose this method
			 you will have a re-entrant API but RTFS won't 
			 cooperate with the kernel to allow multitasking 
			 between tasks using RTFS. If you do use this method 
			 you will only have to implement the macros:
			 PC_FS_EXIT(), CHECK_USER()  and VOID_CHECK_USER().
			 If finer grained multitasking is required the
			 process managment is done with other macros
			 so this routine is not needed.
		       */                        
#define PC_FS_EXIT()  /* This macro is invoked each time rtfs exits
			 an API call. If PC_FS_ENTER is implemented then 
			 this macro should undo its action. Typically 
			 releasing a mutex semaphore */

#define CHECK_USER(X, Y)  /*  This macro is invoked each time rtfs enters
			      an API call after PC_FS_ENTER. Its job is to
			      validate that the current task is a valid 
			      file system user (ie: pc_user() will return a
			      user structure for this task. If the task is
			      not a valid user the macro should return Y 
			      casted to type X.
			      For example: An API call that returns a boolean
			      value will have a call in it:
				  CHECK_USER(SDBOOL, NO)
			      Your macro should look something like:
			      #define CHECK_USER(X, Y) \
				  if (!is_valid_user()) return((X) Y);
			      which will expand to 
				  if (!is_valid_user()) return((SDBOOL) NO);
			      Mapping user structures to tasks is system dependent
			      the only requirement is that if a task gets past
			      CHECK_USER() then the system specific function
			      pc_user() will return a valid pointer to a user 
			      structure. If in your application this condition
			      is guaranteed for every task the CHECK_USER() need
			      not be implemented.
			      You may also choose to assign a user structure to
			      the task the first time CHECK_USER() or 
			      VOID_CHECK_USER is invoked for the task. This is a 
			      reasonable approach to assigning user structures 
			      to tasks. */
#define VOID_CHECK_USER() /* This macro is a special case of CHECK_USER()
			     It invoked each time rtfs enters a void API call.
			     It should behave identically to CHECK_USER() 
			     except that it should expand as follows:
			     For example: An API call that is VOID
			     value will have a call in it:
				 VOID_CHECK_USER()
			     Your macro should look something like:
			     #define VOID_CHECK_USER() \
				 if (!is_valid_user()) return; */

#define PC_DRIVE_ENTER(X,Y)  /*This macro is called by the API after the drive
			       to be used for the operation has been determined 
			       the first argument X will be the drive number 
			       (0 to NDRIVES-1). Y will be the boolean values 
			       YES or NO. If Y is YES it means that the macro
			       should return only when exclusive access to the 
			       drive has been established. If Y is NO then the
			       macro should wait if the drive is already in use
			       exclusively and then put it in a state such that
			       exclusive access requests:
				  PC_DRIVE_ENTER(X,YES)
			      will block but non exclusive access requests:
				  PC_DRIVE_ENTER(X,NO) won't block.
			      Note: Exclusive access request must block until
			      all current non exclusive and exclusive access
			      requests have finished (called PC_DRIVE_EXIT()) */
#define PC_DRIVE_EXIT(X)     /*This macro is called by the API after it is
			       finished with the drive before it returns to 
			       the user. The argument X will be the drive number 
			       (0 to NDRIVES-1). It should work in concert with
			       PC_DRIVE_ENTER(X,Y)  to implement the semaphore
			       capability described above. See the manual and
			       release notes for a broader discusion */
#define PC_INODE_ENTER(X,Y)  /*This macro is called by RTFS when it is actively
			       using a directory entry. The first argument X 
			       will be a pointer to an RTFS FINODE structure.
			       0 to NDRIVES-1). Y will be the boolean values 
			       YES or NO. If Y is YES it means that the macro
			       should return only when exclusive access to the 
			       finode has been established. If Y is NO then the
			       macro should wait if the finode is already in use
			       exclusively and then put it in a state such that
			       exclusive access requests:
				  PC_INODE_ENTER(X,YES)
			      will block but non exclusive access requests:
				  PC_INODE_ENTER(X,NO) won't block.
			      Note: Exclusive access request must block until
			      all current non exclusive and exclusive access
			      requests have finished (called PC_INODE_EXIT())
			      The finode structure contains a field of type
			      LOCKOBJ that you may use for managing the use of
			      PC_INODE_ENTER() and PC_INODE_EXIT() the LOCKOBJ
			      type is declared in pckernel and is not touched
			      by other routines in RTFS. */
#define PC_INODE_EXIT(X)     /*This macro is called by RTFS after it is
				finished with a directory entry  
				The argument X will be a pointer to an RTFS 
				FINODE structure. It should work in concert with
				PC_FINODE_ENTER(X)  to implement the semaphore
				capability described above. See the manual and
				release notes for a broader discusion */
#define PC_FAT_ENTER(X)      /*This macro is called by RTFS when it needs access
				  to a drive's file allocation table (FAT). The 
				  argument X is the drive number (0 to NDRIVES-1).
				  It is required that all access to the FATs be 
				  exclusive so ther should be one MUTEX semaphore
				  per drive for the FAT. PC_FAT_ENTER(X) should
				  claim that semaphore */
#define PC_FAT_EXIT(X)       /*This macro is called by RTFS when it is done
				  with a drive's file allocation table (FAT). The 
				  argument X is the drive number (0 to NDRIVES-1).
				  PC_FAT_EXIT(X) should release the semaphore
				  described in PC_FAT_ENTER(X) */
#define PC_BP_ENTER(X)       /*This macro is called by RTFS when it needs access
				  to a drive's buffer pool. The argument X is the
				  drive number (0 to NDRIVES-1). It is required
				  that all access to the buffer pool be 
				  exclusive so ther should be one MUTEX semaphore
				  per drive for the buffer pool. PC_BP_ENTER(X)
				  should claim that semaphore */
#define PC_BP_EXIT(X)        /*This macro is called by RTFS when it is done
				  with a drive's buffer pool. The 
				  argument X is the drive number (0 to NDRIVES-1).
				  PC_BP_EXIT(X) should release the semaphore
				  described in PC_BP_ENTER(X) */
#define PC_DRIVE_IO_ENTER(X) /*This macro is called by RTFS to gain exclusive
			       access to a device driver before it calls it.
			       This method makes it unnecessary to provide 
			       reentrant drivers and complex IO queuing. 
			       The macro should claim exclusive access to a
			       semaphore associated with the driver. The argument
			       X is always the lock_no field from the device
			       table. The value of the lock_no field should be
			       installed at build time and mapped to a semaphore
			       (by PC_DRIVE_IO_ENTER and PC_DRIVE_IO_ENTER) at
			       run time. If two or more entries in the device 
			       table say (C: and D: (2 & 3)) share the same
			       device driver then lock_no should be the same 
			       for the two entries, unless the driver is 
			       re-entrant when called with the two different 
			       drive. In the latter case lock_no may be 
			       distinct.  */
#define PC_DRIVE_IO_EXIT(X)  /*This macro is called by RTFS to release exclusive
				access to a device driver after it calls it. See
				the discussion of X and the semaphore
				in PC_DRIVE_IO_ENTER(X) */

#define PC_ENTER_CRITICAL() /*This macro is called by RTFS when it is entering
			      a critical (non reentrant) section of code.
			      It may either lock out task switching completely
			      or preferably claim a MUTEX semaphore dedicated
			      to managing RTFS critical sections. */

#define PC_EXIT_CRITICAL() /*This macro is called by RTFS when it is leaving
			      a critical (non reentrant) section of code. It
			      should un-do the action of PC_ENTER_CRITICAL() */

/* Lock object: 
    Most of the locking functions above take either simple integer arguments
    or no arguments at all so the mapping of the calls to the lock resources
    should be rather straight foreward. The macros PC_INODE_ENTER and
    PC_INODE_EXIT however take a pointer to a finode structure. To facilitate
    mapping of the finode to a lock resource we include a field in the finode
    named lock_object. It is of the type LOCKOBJ and is for the private use 
    of the lock management code. 

    During the initialization sequence (pc_memory_init()) RTFS will call a
    system specific routine pc_kernel_init_lockobj() (kernel.c) with a pointer 
    to a finode containing your lockobj field. Initialize the field such that
    it may be used by PC_INODE_ENTER and PC_INODE_EXIT.

    In the reference port we create a dummy value
 */


typedef struct lockobj
{
    LONG dummy;
} LOCKOBJ;


/* User object: 
    RTFS requires a user context structure per task. The structure contains
    the current selected drive and the current working directory per 
    drive. A kernel specific function (fs_user()) in pckernel.c and the macros
    CHECK_USER() and VOID_CHECK_USER() in pckernel.h return the user structure 
    and validate that the current task is a valid user respectively. 
    The user object is a kernel specific sub structure of the user structure
    that may be used by the kernel specific code to assist in implementing
    the function and macros. The structure is defined here and is included
    in the user structure using the label user_object. RTFS does not touch
    the user_object field, it is private to this header and to code in the
    pckernel.c source file.
 */


typedef struct userobj
{
    LONG dummy;
} USEROBJ;

#else  // THREAD_SAFE_FS
#include <cyg/infra/diag.h>
extern cmutex_t fs_fat_cmutex;  // defined in fs/fat/pckernel.c
#define CHECK_USER(x,y)
#define VOID_CHECK_USER()
#define PC_DRIVE_ENTER(x,y)
#define PC_DRIVE_EXIT(x)
#define PC_INODE_ENTER(x,y)
#define PC_INODE_EXIT(x)
#define PC_FAT_ENTER(x)
#define PC_FAT_EXIT(x)
#define PC_BP_ENTER(x)
#define PC_BP_EXIT(x)
#define PC_DRIVE_IO_ENTER(x)
#define PC_DRIVE_IO_EXIT(x)
#define PC_ENTER_CRITICAL()
#define PC_EXIT_CRITICAL()
#if 0
#define PC_SYNC_INIT()       diag_printf("mutex_init\n"); cmutex_init( &fs_fat_cmutex );
#define PC_FS_ENTER()        diag_printf("%s %d enter\n",__FUNCTION__,__LINE__); cmutex_lock( &fs_fat_cmutex );
#define PC_FS_EXIT()         diag_printf("%s %d exit\n",__FUNCTION__,__LINE__); cmutex_unlock( &fs_fat_cmutex );
#else
#define PC_SYNC_INIT()       cmutex_init(   &fs_fat_cmutex );
#define PC_FS_ENTER()        cmutex_lock(   &fs_fat_cmutex );
#define PC_FS_EXIT()         cmutex_unlock( &fs_fat_cmutex );
#endif

typedef struct lockobj
{
  LONG dummy;
} LOCKOBJ;

typedef struct userobj 
{
  LONG dummy;
} USEROBJ;



#endif

#ifdef __cplusplus
}
#endif


#define __PCKERNEL__

#endif


