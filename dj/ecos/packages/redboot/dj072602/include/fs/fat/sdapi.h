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
/*****************************************************************************
* Filename: SDAPI.H - Defines & structures for HDTK API
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
*       Provide FILE SYSTEM API as well as INTERFACE API to different
*       SANDISK products such as ATA PC Cards, CF, SPI, MMC, so on....
*
*       The API provides the initial state, transaction state,
*       device status andclosing state for IDE or PCMCIA, SPI and 
*       MMC controllers.
*
*****************************************************************************/

#ifndef __SDAPI_H__

#ifdef __cplusplus
extern "C" {
#endif


/* HDTK Version */
#define HDTK_MAJOR      3
#define HDTK_MINOR      01

#define HDTK_VERSION    ((HDTK_MAJOR << 8) + HDTK_MINOR)


/* Type Definition of HDTK */
#include <fs/fat/sdtypes.h>

/* Include development environment settings */
#include <fs/fat/sdconfig.h>

/* Include kernel specific definitions */
#include <fs/fat/pckernel.h>

/* Error codes */

#define PCERR_FAT_FLUSH         0  /* Cant flush FAT */
#define PCERR_INITMEDI          1  /* Not a DOS disk */
#define PCERR_INITDRNO          2  /* Invalid driveno */
#define PCERR_INITCORE          3  /* Out of core */
#define PCERR_INITDEV           4  /* Can't initialize device */
#define PCERR_INITREAD          5  /* Can't read block 0 */
#define PCERR_INITWRITE         6  /* Can't write block 0 */
#define PCERR_BLOCKCLAIM        7  /* PANIC: Buffer Claim */
#define PCERR_BLOCKLOCK         8  /* Warning: freeing a locked buffer */
#define PCERR_REMINODE          9  /* Trying to remove inode with open > 1 */
#define PCERR_FATREAD           10  /* IO Error While Failed Reading FAT */
#define PCERR_DROBJALLOC        11 /* Memory Failure: Out of DROBJ Structures */
#define PCERR_FINODEALLOC       12 /* Memory Failure: Out of FINODE Structures */

extern int errno;

/*********************** CRITICAL_ERROR_HANDLER ******************************/
/* Arguments to critical_error_handler() */
#define CRERR_BAD_FORMAT        301
#define CRERR_NO_CARD           302
#define CRERR_BAD_CARD          303
#define CRERR_CHANGED_CARD      304
#define CRERR_CARD_FAILURE      305
#define CRERR_ID_ERROR          306
#define CRERR_ECC_ERROR         307
#define CRERR_SLEEP             308
#if defined(__DHARMA)
#define CRERR_DRIVE_RESET       309
#endif /* __DHARMA */

/* Return code from critical_error_handler() */
#define CRITICAL_ERROR_ABORT    1
#define CRITICAL_ERROR_RETRY    2
#define CRITICAL_ERROR_FORMAT   3
#define CRITICAL_ERROR_CLEARECC 4
#define CRITICAL_ERROR_IGNORE   5

/* Drive geometry structure */
typedef struct drv_geometry_desc
{
	UINT16  dfltCyl;        /* Number of cylinders */
	UINT16  dfltHd;         /* Number of Heads */
	UINT16  dfltSct;        /* Sectors per track */
	UINT16  dfltBytesPerSect; /* Bytes per sector */
    	UINT32  totalLBA;       /* Total drive logical blocks */
	UCHAR   serialNum[20];  /* Drive serial number */
	UCHAR   modelNum[40];   /* Drive model number */
} DRV_GEOMETRY_DESC, *PDRV_GEOMETRY_DESC;



/* OEM platform specific */
#include <fs/fat/oem.h>


#ifndef copybuff
SDVOID copybuff(SDVOID *vto, SDVOID *vfrom, INT16 size);
#endif

#ifndef compbuff
INT16 compbuff(SDVOID *vfrom, SDVOID *vto, INT16 size);
#endif

#ifndef pc_memfill
SDVOID pc_memfill(SDVOID *vto, INT16 size, UTINY c);
#endif

#ifndef pc_strcat
SDVOID pc_strcat(TEXT *to, TEXT *from);
#endif


#if (CHAR_16BIT)
SDVOID b_unpack(UTINY *to, UINT16 *from, UINT16 length, UINT16 offset);
#endif

/* Byte ordering conversion */
UINT16 to_WORD(UTINY *buf);
ULONG to_DWORD(UTINY *buf);
SDVOID fr_WORD ( UTINY *to, UINT16 from );
SDVOID fr_DWORD ( UTINY *to, UINT32 from );


UINT16 swap_hi_low_byte(UINT16 inword);


/****************************************************************************/
/************************** MEMORY or I/O ACCESS ****************************/
/****************************************************************************/

#if (USE_MEMMODE)

	/* Read/Write register access */ 
	#define SDREAD_DATA32(X)        *((volatile FPTR32) (X))
	#define SDREAD_DATA16(X)        *((volatile FPTR16) (X))   
	#define SDREAD_DATA08(X)        *((volatile FPTR08) (X))

	#define SDWRITE_DATA32(X, Y)    (*((volatile FPTR32) (X)) = (UINT32)(Y))
	#define SDWRITE_DATA16(X, Y)    (*((volatile FPTR16) (X)) = (UINT16)(Y))
	#define SDWRITE_DATA08(X, Y)    (*((volatile FPTR08) (X)) = (UINT08)(Y))

#else   /* I/O MODE */

	/* 8-bit interface */

	#define SDREAD_DATA08(X) (inpbyte((UINT16) (X) ))
	#define SDWRITE_DATA08(X,Y) outpbyte((UINT16)(X), (UCHAR)(Y))

   #if (WORD_ACCESS_ONLY)  /* 16-bit interface */

	#define SDREAD_DATA16(X) (inpword((UINT16) (X) ))
	#define SDWRITE_DATA16(X,Y) outpword((UINT16)(X), (UINT16)(Y))

   #endif


#endif  /* USE_MEMMODE */



/*****************************************************************************
* Name: pc_get_error
*
* Description
*       Get error code
*
* Entries:
*    if (USE_FILE_SYSTEM)
*       INT16  taskno     task number
*    else #(!USE_FILE_SYSTEM)
*       INT16  driveno    drive number
* Returns:
*    if (USE_FILE_SYSTEM)
*    {
*          0  No Error
*       otherwise, one of the following error code:
*           PEBADF          9    Invalid file descriptor
*           PENOENT         2    File not found or path to file not found
*           PEMFILE         24   No file descriptors available (too many files open)
*           PEEXIST         17   Exclusive access requested but file already exists.
*           PEACCES         13   Attempt to open a read only file or a special (directory)
*           PEINVAL         22   Seek to negative file pointer attempted.
*           PENOSPC         28   Write failed. Presumably because of no space
*           PESHARE         30   Open failed do to sharing
*           PEDEVICE        31   No Valid Disk Present
*    }
*    else #(!USE_FILE_SYSTEM)
*    {
*
*           0  No Error
*       otherwise, one of the following error code in controller_s:
*           BUS_ERC_DIAG     1   Drive diagnostic failed in initialize
*           BUS_ERC_ARGS     2   User supplied invalid arguments
*           BUS_ERC_DRQ      3   DRQ should be asserted but it isn't
*                                  or driver and controller are out of phase
*           BUS_ERC_TIMEOUT  4   Timeout during some operation 
*           BUS_ERC_STATUS   5   Controller reported an error
*                                  look in the error register
*    }
*
******************************************************************************/
INT16 pc_get_error (INT16 driveno_or_taskno);


INT16 pc_get_extended_error (INT16 driveno);


/****************************************************************************/
/**************************** FILE SYSTEM API *******************************/
/****************************************************************************/

#if (USE_FILE_SYSTEM)


#define PCDELETE (UTINY) 0xE5 /* MS-DOS file deleted char */

#define ARDONLY         0x01    /* MS-DOS File attributes */
#define AHIDDEN         0x02
#define ASYSTEM         0x04
#define AVOLUME         0x08 
#define ADIRENT         0x10
#define ARCHIVE         0x20
#define ANORMAL         0x00
#define CHICAGO_EXT     0x0F    /* Chicago extended filename attribute */


/* Maximum path length. */
#define EMAXPATH        128


/* Date stamping buffer */
typedef struct datestr {
	UINT16 date;        
	UINT16 time;
} DATESTR;


/* Structure for use to do recursive search for files in a selected drive */
typedef struct dstat {
	TEXT    fname[10];      /* Null terminated file and extension */
	TEXT    fext[4];
	UINT16  fattribute;     /* File attributes */
	UINT16  ftime;          /* Time & Date last modified. See date */
  UTINY   ftime_tenths;   /* DIR_CrtTimeTenth field of FAT dnode */
	UINT16  fdate;          /* and time handlers for getting info */
	UINT16  fcrttime;
	UINT16  fcrtdate;
	ULONG   fsize;          /* File size */
	INT16   driveno;
	TEXT    pname[10];      /* Pattern. */
	TEXT    pext[4];
	TEXT    path[EMAXPATH];
	TEXT    longFileName[EMAXPATH];
} DSTAT;


/* 
** Structure for use by with file statistic.
** Portions of this structure and the macros were lifted from BSD sources.
** See the RTFS ANSI library for BSD terms & conditions. 
*/
typedef struct stat
{
	ULONG   st_mode;        /* (see S_xxxx below) */
	ULONG   st_size;        /* file size, in bytes */
	LONG    st_blksize;     /* optimal blocksize for I/O (cluster size) */
	LONG    st_blocks;      /* blocks allocated for file */
	DATESTR st_atime;       /* last access (all times are the same) */
	DATESTR st_mtime;       /* last modification */
	DATESTR st_ctime;       /* last file status change */
	INT16   st_dev;         /* (drive number, rtfs) */
	INT16   st_ino;         /* inode number (0) */
	INT16   st_nlink;       /* (always 1) */
	INT16   st_rdev;        /* (drive number, rtfs) */
	UINT16  fattribute;     /* File attributes - DOS attributes
				   (non standard but useful)
				*/
	UINT16  padding;     
} STAT;


/* Values for the st_mode field */
#define S_IFMT   0170000        /* type of file mask */
#define S_IFCHR  0020000        /* character special (unused) */
#define S_IFDIR  0040000        /* directory */
#define S_IFBLK  0060000        /* block special  (unused) */
#define S_IFREG  0100000        /* regular */
#define S_IWRITE 0000400        /* Write permitted  */
#define S_IREAD  0000200        /* Read permitted. (Always true anyway)*/

#define DEFFILEMODE (S_IREAD|S_IWRITE)
#define S_ISDIR(m)  ((m & 0170000) == 0040000) /* directory */
#define S_ISCHR(m)  ((m & 0170000) == 0020000) /* char special */
#define S_ISBLK(m)  ((m & 0170000) == 0060000) /* block special */
#define S_ISREG(m)  ((m & 0170000) == 0100000) /* regular file */
#define S_ISFIFO(m) ((m & 0170000) == 0010000) /* fifo */



/* File creation permissions for open */
/* Note: OCTAL */
#define PS_IWRITE       0000400 /* Write permitted  */
#define PS_IREAD        0000200 /* Read permitted. (Always true anyway) */


/* File access flags */
#define PO_RDONLY       0x0000 /* Open for read only */
#define PO_WRONLY       0x0001 /* Open for write only */
#define PO_RDWR         0x0002 /* Read/write access allowed. */
#define PO_APPEND       0x0008 /* Seek to eof on each write */
#define PO_CREAT        0x0100 /* Create the file if it does not exist. */
#define PO_TRUNC        0x0200 /* Truncate the file if it already exists */
#define PO_EXCL         0x0400 /* Fail if creating and already exists */
#define PO_TEXT         0x4000 /* Ignored */
#define PO_BINARY       0x8000 /* Ignored. All file access is binary */
#define PO_NOSHAREANY   0x0004 /* Wants this open to fail if already open.
				    Other opens will fail while this open
				    is active
			       */
#define PO_NOSHAREWRITE 0x0800 /* Wants this opens to fail if already open
				  for write. Other open for write calls 
				  will fail while this open is active.
			       */

/* Arguments to extend file */
#define PC_FIRST_FIT    1
#define PC_BEST_FIT     2
#define PC_WORST_FIT    3



/* Errno values */
#define PEBADF          9       /* Invalid file descriptor */
#define PENOENT         2       /* File not found or path to file not found */
#define PEMFILE         24      /* No file descriptors available (too many files open) */
#define PEEXIST         17      /* Exclusive access requested but file already exists. */
#define PEACCES         13      /* Attempt to open a read only file or a special (directory) */
#define PEINVAL         22      /* Seek to negative file pointer attempted. */
#define PENOSPC         28      /* Write failed. Presumably because of no space */
#define PESHARE         30      /* Open failed do to sharing */
#define PEDEVICE        31      /* No Valid Disk Present */
#define PEAGAIN         11      /* Try again */


/* Arguments to SEEK */
#define PSEEK_SET       0       /* offset from begining of file */
#define PSEEK_CUR       1       /* offset from current file pointer */
#define PSEEK_END       2       /* offset from end of file */





/*****************************************************************************
* Name: pc_system_init
*
* Description:   Initialize the system memory and configure the system
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES     if sucessful
*       NO      if failure
*****************************************************************************/
SDBOOL pc_system_init(INT16 driveno);


/*****************************************************************************
* Name: pc_sytem_close
*
* Processing:   Restore old system configuration and release memory
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES     if sucessful
*       NO      if failure
*****************************************************************************/
SDVOID pc_system_close(INT16 driveno);



/*****************************************************************************
* Name: pc_cluster_size
*
* Processing:   Get cluster size from a selected drive
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Drive Cluster size
*****************************************************************************/
INT16 pc_cluster_size(INT16 driveno);


/*****************************************************************************
* Name: pc_dskopen
*
* Processing:   Open a selected drive for disk activities
*
* Entries:
*       INT16   driveno         Drive Number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_dskopen(INT16 driveno);


/*****************************************************************************
* Name: pc_dskclose
*
* Processing:   Stop all disk activities from a selected drive
*               and remove the drive from the system.
*
* Entries:
*       INT16   driveno         Drive Number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_dskclose(INT16 driveno);



/*****************************************************************************
* Name: pc_diskabort
*
* Processing:   Terminate operations on a disk
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       
*****************************************************************************/
SDVOID pc_diskabort(INT16 driveno);



/*****************************************************************************
* Name: pc_dskflush
*
* Processing:   Flushing files to disk
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_dskflush(INT16 driveno);


/*****************************************************************************
* Name: pc_format
*
* Processing:   Format a selected disk
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_format(INT16 driveno);



/*****************************************************************************
* Name: pc_free
*
* Processing:   Get available disk space
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
ULONG pc_free(INT16 driveno);



/*****************************************************************************
* Name: pc_fstat
*
* Processing:   Get file statical
*
* Entries:
*       PCFD    fd      File Handle
*       STAT    pstat   Pointer to file statistic structure  
*
* Returns:
*       
*****************************************************************************/
INT16 pc_fstat(PCFD fd, STAT *pstat);



/*****************************************************************************
* Name: pc_getdlftdrvno
*
* Processing:   Get default drive
*
* Entries:
*       None
*
* Returns:
*       Drive Number       
*****************************************************************************/
INT16 pc_getdfltdrvno(SDVOID);



/*****************************************************************************
* Name: pc_get_attributes
*
* Processing:   Get attribute of a file
*
* Entries:
*       TEXT    path            File name with full path
*       UCHAR  *p_return        File Attribute
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_get_attributes(TEXT *path, UINT16 *p_return);


/*****************************************************************************
* Name: pc_gfirst
*
* Processing:   find first file
*
* Entries:
*       DSTAT   statobj
*       TEXT    name            File Name
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_gfirst(DSTAT *statobj, TEXT *name);



/*****************************************************************************
* Name: pc_gnext
*
* Processing:   Find next file
*
* Entries:
*       DSTAT   statobj         
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_gnext(DSTAT *statobj);



/*****************************************************************************
* Name: pc_gdone
*
* Processing:   Free resources
*
* Entries:
*       DSTAT   statobj         Pointer to 
*
* Returns:
*       None       
*****************************************************************************/
SDVOID pc_gdone(DSTAT *statobj);


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
SDVOID pc_get_curr_unicode_fname(SDWCHAR *filename, INT16 buffsize);


/*****************************************************************************
* Name: pc_isdir
*
* Processing:   Check a selected path for directory
*
* Entries:
*       TEXT    path         
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_isdir(TEXT *path);


/*****************************************************************************
* Name: pc_isvol
*
* Processing:   Check a selected path for volume
*
* Entries:
*       TEXT    path
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_isvol(TEXT *path);



/*****************************************************************************
* Name: pc_mfile
*
* Processing:   Build a file from a file name and extension   
*
* Entries:
*       TEXT    *to      
*
* Returns:
*       The File Name
*****************************************************************************/
TEXT *pc_mfile(TEXT *to, TEXT *filename, TEXT *ext);


/*****************************************************************************
* Name: pc_mpath
*
* Processing:   Build a path specific from a file and path name
*
* Entries:
*       TEXT   *to
*
* Returns:
*       The File Name
*****************************************************************************/
TEXT *pc_mpath(TEXT *to, TEXT *path, TEXT *filename);



/*****************************************************************************
* Name: pc_mkdir
*
* Processing:   Create a directory
*
* Entries:
*       TEXT    name    path name
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_mkdir(TEXT *name);



/*****************************************************************************
* Name: pc_mv
*
* Processing:   Rename File or Directory
*
* Entries:
*       TEXT    name            old name
*       TEXT    newname         new name
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_mv(TEXT *name, TEXT *newname);



/*****************************************************************************
* Name: pc_rmdir
*
* Processing:   Delete  adirectory
*
* Entries:
*       TEXT    name    Full source path
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_rmdir(TEXT *name);



/*****************************************************************************
* Name: pc_pwd
*
* Processing:   Get the current working directory
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_pwd(TEXT *drive, TEXT *path);



/*****************************************************************************
* Name: pc_setdfltdrvno
*
* Processing:   Set a selected drive to be a default drive
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_setdfltdrvno(INT16 driveno);

 

/*****************************************************************************
* Name: pc_set_attribute
*
* Processing:   Set file attribute to a selected file
*
* Entries:
*       TEXT    path            Full file name source
*       UCHAR   attributes      File attribute
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_set_attributes(TEXT *path, UINT16 attributes);


/*****************************************************************************
* Name: pc_touch
*
* Processing:   Update date of a selected file
*
* Entries:
*       TEXT    path            Full file name source
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_touch(TEXT *path);


/*****************************************************************************
* Name: pc_set_cwd
*
* Processing:   Set a selected directory to be a current directory
*
* Entries:
*       TEXT    name    Directory Name
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_set_cwd(TEXT *name);



/*****************************************************************************
* Name: pc_set_default_drive
*
* Processing:   Set a selected drive to be a default drive
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_set_default_drive(INT16 driveno);



/*****************************************************************************
* Name: pc_stat
*
* Processing:   File statistic
*
* Entries:
*       TEXT    name            File Name
*       STAT    pstat           File statistic structure
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
INT16 pc_stat(TEXT *name, STAT *pstat);



/*****************************************************************************
* Name: pc_unlink
*
* Processing:   Delete a file
*
* Entries:
*       TEXT    name    File Name
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL pc_unlink(TEXT *name);


/* name is path of volume
 * volume label should be 12 bytes, which includes room for null-termination
 * return value is YES or NO depending on whether the volume_label was succesfully
 *  acquired */
SDBOOL pc_get_volume_label(TEXT * name, TEXT * volume_label);

/****************************************************************************/
/****************************** FILE TRANSACTION ****************************/
/****************************************************************************/


/*****************************************************************************
* Name: po_close
*
* Processing:   Close a file
*
* Entries:
*       PCFD    fd      File Handle
*
* Returns:
*       
*****************************************************************************/
INT16 po_close(PCFD fd);


/*****************************************************************************
* Name: po_flush
*
* Processing:   Flush a file
*
* Entries:
*       PCFD    fd      File Handle
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL po_flush(PCFD fd);


/*****************************************************************************
* Name: po_lseek
*
* Processing:   Move the file pointer to a specific location
*
* Entries:
*       PCFD    fd      File Handle
*       ULONG   offset
*       INT16   origin
*
* Returns:
*       New location of the file handle       
*****************************************************************************/
ULONG po_lseek(PCFD fd, INT32 offset, INT16 origin, INT16 *err_flag);


/*****************************************************************************
* Name: po_open
*
* Processing:   Open  a file
*
* Entries:
*       TEXT    name    File Name
*       UINT16  flag
*       UINT16  mode
*
* Returns:
*       File Handle       
*****************************************************************************/
PCFD po_open(TEXT *name, UINT16 flag, UINT16 mode);



/*****************************************************************************
* Name: po_read
*
* Processing:   Read data from a file
*
* Entries:
*       PCFD    fd      File Handle
*       UCHAR   buf     Buffer Pointer
*       UCOUNT  count   Number of bytes to write
*
* Returns:
*       Number of bytes sucessful to read from file       
*****************************************************************************/
UCOUNT po_read(PCFD fd,  UCHAR *buf, UCOUNT count);


void pc_set_valid_read_data(unsigned int data);
unsigned int pc_get_valid_read_data(void);


/*****************************************************************************
* Name: po_write
*
* Processing:   Write data to a file 
*
* Entries:
*       PCFD    fd      File Handle
*       UCHAR   buf     Buffer Pointer
*       UCOUNT  count   Number of bytes to write
*
* Returns:
*       Number of bytes sucessful writing to file       
*****************************************************************************/
UCOUNT po_write(PCFD fd, UCHAR *buf, UCOUNT count);


/*****************************************************************************
* Name: po_truncate
*
* Processing:   Truncate a file to a specific location
*
* Entries:
*       PCFD    fd      File Handle
*       ULONG   offset  Specific location
*
* Returns:
*       Sucessful if YES
*       Failure if NO
*****************************************************************************/
SDBOOL po_truncate(PCFD fd, LONG offset);


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
UINT16 po_extend_file(PCFD fd, UINT16 n_clusters, INT16 method, SDBOOL preerase_region);


/****************************************************************************/
/************************* END OF FILE SYSTEM API ***************************/
/****************************************************************************/

#endif  /* (USE_FILE_SYSTEM) */



/*****************************************************************************/
/****************************** INTERFACE API ********************************/
/*****************************************************************************/

/**************************** TRUE IDE SECTION ******************************/
/********************* This is for IDE Controller ONLY **********************/

#if (USE_TRUE_IDE)

/*****************************************************************************
* Name: ide_init
*
* Processing:
*       Initialize data structure for IDE controller.
*
* Entries:
*       None
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_init (SDVOID);


/*****************************************************************************
* Name: ide_drive_open
*
* Processing:
*       Set up internal data structure and configure the system.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_drive_open(INT16 driveno);


/*****************************************************************************
* Name: ide_drive_close
*
* Processing:
*       Release internal data structure and restore memory configuration
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_drive_close(INT16 driveno);



/*****************************************************************************
* Name: ide_read
*
* Processing:
*       Get data from the drive at the requested logical block addresses.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_read(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);


/*****************************************************************************
* Name: ide_write
*
* Processing:
*       Send information to the specific locations for the selected drive.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_write(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: ide_erase
*
* Processing:
*       Clear the data for count sectors to make the block addresses ready 
*       for the next write.  This will improve data write access time.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_erase(INT16 driveno, ULONG sector, UCOUNT count);


/*****************************************************************************
* Name: ide_read_serial
*
* Processing:
*       Get the drive geometry information.
*
* Entries:
*       INT16   driveno         Driver number
*       PDRV_GEOMETRY_DESC idDrvPtr     Device Geometry buffer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL ide_read_serial(INT16 drive_no, PDRV_GEOMETRY_DESC idDrvPtr);

#endif  /* (USE_TRUE_IDE) */


/************************** PCMCIA SECTION ********************************/
/******************* This is for PCMCIA Controller ONLY *******************/

#if (USE_PCMCIA)

/*****************************************************************************
* Name: pcm_init
*
* Processing:
*       Initialize data structure for PCMCIA controller.
*
* Entries:
*       None
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_init (SDVOID);


/*****************************************************************************
* Name: pcm_drive_open  
*
* Processing:
*       Set up internal data structure and configure the system.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_drive_open(INT16 driveno);


/*****************************************************************************
* Name: pcm_drive_close
*
* Processing:
*       Release internal data structure and restore memory configuration
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_drive_close(INT16 driveno);



/*****************************************************************************
* Name: pcm_read_serial
*
* Processing:
*       Get the drive geometry information.
*
* Entries:
*       INT16   driveno         Driver number
*       PDRV_GEOMETRY_DESC idDrvPtr     Device Geometry buffer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_read_serial(INT16 drive_no, PDRV_GEOMETRY_DESC idDrvPtr);



/*****************************************************************************
* Name: pcm_read
*
* Processing:
*       Get data from the device at the requested LBA.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_read(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: pcm_write
*
* Processing:
*       Send information to the specific location on the drive.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_write(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: pcm_erase
*
* Processing:
*       Clear the LBA to make it ready for the next write access. This
*       will improve the write data access.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL pcm_erase(INT16 driveno, ULONG sector, UCOUNT count);

/*****************************************************************************
* Name: is_status_changed_bit_set
*
* Processing:
*       Check device status to verify the device condition.
*
* Entries:
*       INT16   socket         Socket number
*
* Returns:
*       YES if the device is changed.
*       NO if the same.
*       
*****************************************************************************/
SDBOOL  is_status_changed_bit_set (INT16 socket);

#endif  /* (USE_PCMCIA) */



/****************************** SPI SECTION ********************************/
/********************** This is for SPI Controller ONLY ********************/
#if (USE_SPI || USE_SPI_EMULATION)

/*****************************************************************************
* Name: spi_init
*
* Processing:
*       Initialize data structure for SPI controller.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_init(SDVOID);


/*****************************************************************************
* Name: spi_drive_open
*
* Processing:
*       Set up internal data structure and configure the system.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_drive_open(INT16 driveno);



/*****************************************************************************
* Name: spi_drive_close
*
* Processing:
*       Release internal data structure and restore memory configuration
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_drive_close(INT16 driveno);


/*****************************************************************************
* Name: spi_read_serial
*
* Processing:
*       Get the drive geometry information.
*
* Entries:
*       INT16   driveno         Driver number
*       PDRV_GEOMETRY_DESC idDrvPtr     Device Geometry buffer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_read_serial(INT16 driveno, PDRV_GEOMETRY_DESC idDrvPtr);



/*****************************************************************************
* Name: spi_read
*
* Processing:
*       Get data from the drive at the requested logical block addresses.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_read(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: spi_write
*
* Processing:
*       Send information to the specific location on the drive.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_write(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: spi_erase
*
* Processing:
*       Clear the LBA to make it ready for the next write access. This
*       will improve the write data access.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL spi_erase(INT16 driveno, ULONG sector, UCOUNT count);

/* Configure the block size */
SDBOOL setBlockSize( INT16 driveno, UINT16 blockSize );

#endif  /* (USE_SPI || USE_SPI_EMULATION) */



/****************************** MMC SECTION ********************************/
/********************** This is for MMC Controller ONLY ********************/
#if (USE_MMC || USE_MMC_EMULATION)

/*****************************************************************************
* Name: mmc_init
*
* Processing:
*       Initialize data structure for MMC controller.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_init(SDVOID);


/*****************************************************************************
* Name: mmc_drive_open
*
* Processing:
*       Set up internal data structure and configure the system.
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_drive_open(INT16 driveno);



/*****************************************************************************
* Name: mmc_drive_close
*
* Processing:
*       Release internal data structure and restore memory configuration
*
* Entries:
*       INT16   driveno         Driver number
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_drive_close(INT16 driveno);



/*****************************************************************************
* Name: mmc_read_serial
*
* Processing:
*       Get the drive geometry information.
*
* Entries:
*       INT16   driveno         Driver number
*       PDRV_GEOMETRY_DESC idDrvPtr Device geometry buffer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_read_serial(INT16 driveno, PDRV_GEOMETRY_DESC idDrvPtr);



/*****************************************************************************
* Name: mmc_read
*
* Processing:
*       Get data from the drive at the requested logical block addresses.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_read(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: mmc_write
*
* Processing:
*       Send information to the specific location on the drive.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UTINY   *buffer         Data buffer
*       UCOUNT  count           Number of sector to transfer
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_write(INT16 driveno, ULONG sector, UTINY *buffer, UCOUNT count);



/*****************************************************************************
* Name: mmc_erase
*
* Processing:
*       Clear the LBA to make it ready for the next write access. This
*       will improve the write data access.
*
* Entries:
*       INT16   driveno         Driver number
*       ULONG   sector          Starting logical block address
*       UCOUNT  count           Number of sectors
*
* Returns:
*       YES if successful
*       NO if failure
*       
*****************************************************************************/
SDBOOL mmc_erase(INT16 driveno, ULONG sector, UCOUNT count);

/* Configure the block size */
SDBOOL setBlockSize( INT16 driveno, UINT16 blockSize );

#endif  /* (USE_MMC || USE_MMC_EMULATION) */


#ifdef __cplusplus
}
#endif

#define __SDAPI_H__

#endif  /* __SDAPI_H__ */

