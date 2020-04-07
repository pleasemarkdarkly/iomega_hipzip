/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_fs.h - File system interface for abstraction layer 
 */

#ifndef	_GN_FS_H_
#define _GN_FS_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include	GN_FCNTL_H

#if 0
/* REC commented this code out because MSDEV #defines errno when     */
/* linking against the multithreaded runtime library. The code below */
/* should say something like "defined(GREEN_HILL) && defined(errno)" */
/* once we know the proper symbol for the Green Hill compiler.       */
#if	defined(errno)  /* Prevent Green Hill compiler from expanding */
			/* errno in the header name */
#	undef	errno
#endif
#endif

#include	GN_SYS_STAT_H


#ifdef __cplusplus
extern "C"{
#endif 

/*
 * Constants.
 */

/* invalid handle value */
#define		FS_INVALID_HANDLE	-1

/* modes for seeking (same as stdio lseek) */
#include <fs/fat/sdapi.h>
#define		FS_SEEK_START		PSEEK_SET
#define		FS_SEEK_CURRENT		PSEEK_CUR
#define		FS_SEEK_END			PSEEK_END


/* file system open modes - map to fcntl.h */
#ifdef	PLATFORM_WIN32
	#ifndef O_RDONLY
		#define		O_RDONLY	_O_RDONLY
		#define		O_WRONLY	_O_WRONLY
		#define		O_RDWR		_O_RDWR
		#define		O_APPEND	_O_APPEND
		#define		O_CREAT		_O_CREAT
		#define		O_TRUNC		_O_TRUNC
		#define		O_SYNC		_O_SYNC
		#define		O_EXCL		_O_EXCL
	#endif
#endif	/* ifndef PLATFORM_WIN32 */


/* file system attributes - map to stat.h */
#ifdef	PLATFORM_WIN32
	#ifndef S_IREAD
		#define		S_IREAD		_S_IREAD
		#define		S_IWRITE	_S_IWRITE
		#define		S_IFDIR		_S_IFDIR
	#endif
#endif	/* ifndef PLATFORM_WIN32 */


#if	0 //defined O_RDONLY /* For system where regular <fcntl.h> exists */
#define FSMODE_ReadOnly		O_RDONLY
#define FSMODE_WriteOnly	O_WRONLY
#define FSMODE_ReadWrite	O_RDWR
#define FSMODE_Append		O_APPEND
#define FSMODE_Create		O_CREAT
#define FSMODE_Truncate		O_TRUNC
/*	#define  FSMODE_Synch		O_SYNC		TBD: standard value for this ??? */
#define FSMODE_Exclusive	O_EXCL

#else	/* For system where no such symbols defined, such as MULTI OS */
	/* define them as they are usually done... */
//#define FSMODE_ReadOnly		0x0000
//#define FSMODE_WriteOnly	0x0001
//#define FSMODE_ReadWrite	0x0002
//#define FSMODE_Append		0x0004
//#define FSMODE_Create		0x0100
//#define FSMODE_Truncate		0x0200
//#define FSMODE_Exclusive	0x0400
#define FSMODE_ReadOnly         0x0001
#define FSMODE_WriteOnly        0x0002
#define FSMODE_ReadWrite        0x0004
#define FSMODE_Append           0x0008
#define FSMODE_Create           0x0100
#define FSMODE_Truncate         0x0200
#define FSMODE_Exclusive        0x0400
#endif /* #if	defined O_RDONLY  */


#define	FSATTR_Invalid		-1
#if	defined S_IREAD /* For system where regular <fcntl.h> exists */
#define	FSATTR_Read			S_IREAD
#define	FSATTR_Write		S_IWRITE
#define	FSATTR_Dir			S_IFDIR
#define	FSATTR_ReadWrite	(S_IREAD|S_IWRITE)
#else	/* For system where no such symbols defined, such as MULTI OS */
	/* define them as they are usually done... */
#define	FSATTR_Read			00400
#define	FSATTR_Write		00200
#define	FSATTR_Dir			0x4000
#define	FSATTR_ReadWrite	00600
#endif /* #if	defined S_IREAD  */


/*
 * Typedefs.
 */

typedef gn_uint32_t gnfs_mode_t;
typedef gn_int32_t gnfs_attr_t;

/* abstract type for file offsets */
typedef	long	gn_foffset_t;

/* abstract type for file descriptors */
typedef int		gn_file_t;

/* invalid handle value */
#define		FS_INVALID_OFFSET	((gn_foffset_t)-1L)

/* file subsystem configuration structure */
typedef struct fs_config_t
{
	gn_char_t	file_image_path[GN_MAX_PATH];
	gn_bool_t	set_file_image_path;
}
gnfs_config_t;


/*
 * Prototypes.
 */

/* set the root directory of the cddb hierarchy */
void
gnfs_set_root_directory(gn_cstr_t root_dir);

/* configure the file system prior to initialization */
gn_error_t
gnfs_configure(gnfs_config_t* config);

/* initialize file system subsystem */
gn_error_t
gnfs_initialize(void);

/* initialize file system subsystem */
gn_error_t
gnfs_shutdown(void);

/* enable file system logging */
gn_error_t
gnfs_set_fs_logging(gn_bool_t enable, gn_cstr_t logfile);

/* insert message into file system logging */
void
gnfs_add_fs_logging_info(gn_cstr_t info);

/* open existing file for reading and/or writing */
gn_handle_t
gnfs_open(gn_cstr_t filename, gnfs_mode_t mode);

/* create file, leave open for reading and/or writing */
gn_handle_t
gnfs_create(gn_cstr_t filename, gnfs_mode_t mode, gnfs_attr_t attribute);

/* delete file */
gn_error_t
gnfs_delete(gn_cstr_t filename);

/* check to see if file exists */
gn_bool_t
gnfs_exists(gn_cstr_t filename);

/* copy file to new name */
gn_error_t
gnfs_copy_file(gn_cstr_t source_file, gn_cstr_t dest_file);

/* rename file. */
gn_error_t
gnfs_rename_file(gn_cstr_t old_file, gn_cstr_t new_file);

/* get file attribute */
gnfs_attr_t
gnfs_get_attr(gn_cstr_t filename);

/* set file attribute */
gn_error_t
gnfs_set_attr(gn_cstr_t filename, gnfs_attr_t attribute);

/* read bytes from file (returns -1 if error) */
gn_size_t
gnfs_read(gn_handle_t handle, void* buffer, gn_size_t size);

/* write bytes to file (returns -1 if error) */
gn_size_t
gnfs_write(gn_handle_t handle, void* buffer, gn_size_t size);

/* read bytes from file at offset (returns -1 if error) */
gn_size_t
gnfs_read_at(gn_handle_t handle, gn_foffset_t offset, void* buffer, gn_size_t size);

/* write bytes to file at offset (returns -1 if error) */
gn_size_t
gnfs_write_at(gn_handle_t handle, gn_foffset_t offset, void* buffer, gn_size_t size);

/* get current file position */
gn_foffset_t
gnfs_tell(gn_handle_t handle);

/* set current file position */
gn_foffset_t
gnfs_seek(gn_handle_t handle, gn_foffset_t offset, int mode);

/* get end-of-file position */
gn_foffset_t
gnfs_get_eof(gn_handle_t handle);

/* set end-of-file position */
gn_foffset_t
gnfs_set_eof(gn_handle_t handle, gn_foffset_t offset);

/* commit changes to disk */
gn_error_t
gnfs_commit(gn_handle_t handle);

/* close file */
gn_error_t
gnfs_close(gn_handle_t handle);

/* retrieve last error */
gn_error_t
gnfs_get_error(void);

/* Get the number of bytes free in given directory.
 * For a system with no per-directory restrictions, the value returned
 * represents all free space available on the disk.
 * The free_space_low parameter represents the low part of a 64-bit number.
 * The free_space_high parameter represents the high part of a 64-bit number.
 * A NULL value for directory indicates the current (or default) directory.
 */
gn_error_t
gnfs_get_disk_free_space(gn_cstr_t directory, gn_uint64_t* free_space);

#ifdef __cplusplus
}
#endif 

#endif /* _GN_FS_H_ */
