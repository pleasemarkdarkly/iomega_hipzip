/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_fs.c - File system interface implementation for abstraction layer 
 */

/*
 * Platform compatibility support
 */

/*
 * Dependencies.
 */
#include <fs/fat/sdapi.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <unistd.h>
//#include <stdio.h>
#include <string.h>
//#include <fcntl.h>
//#include <errno.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_string.h>

#include <util/debug/debug.h>

//DEBUG_MODULE_S(GNFS, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE);
DEBUG_MODULE_S(GNFS, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(GNFS);

static	gn_error_t gnfs_errno;

static char s_szRootDir[GN_MAX_PATH] = "";
static int s_iRootDrive = -1;

/* Function Prototypes */
static void set_gnfserrno(gn_int32_t error);

/* set the root directory of the cddb hierarchy */
void
gnfs_set_root_directory(gn_cstr_t root_dir)
{
    strncpy(s_szRootDir, root_dir, GN_MAX_PATH - 1);
    if (s_szRootDir[strlen(s_szRootDir) - 1] != '/')
        strcat(s_szRootDir, "/");
    s_iRootDrive = gn_tolower(root_dir[0]) - 'a';
}

/* Convert gnfs_mode_t type to sandisk mode, sd_flags */
/* Changes to return 0 if invalid mode, else 1 */
static int gnfs_mode_to_sd_mode(gnfs_mode_t gn_mode, UINT16* sd_flags, UINT16* sd_mode)
{
    int success = 1;

#if 1
	if(gn_mode & FSMODE_ReadWrite)
	{
		*sd_flags = PO_RDWR | PO_NOSHAREANY;
		*sd_mode = PS_IREAD | PS_IWRITE;
	}
	else if(gn_mode & FSMODE_ReadOnly)
	{
		*sd_flags = PO_RDONLY | PO_NOSHAREWRITE;
		*sd_mode = PS_IREAD;
	}
	else if(gn_mode & FSMODE_WriteOnly)
	{
		*sd_flags = PO_WRONLY | PO_NOSHAREANY;
		*sd_mode = PS_IWRITE;
	}
	else
	{
		/* bad news */
//		diag_printf("gnfs_mode_to_sd_mode: Unsuported mode: %d\n", gn_mode);
		gnfs_errno = FSERR_InvalidArg;
		return 0;
	}

	if(gn_mode & FSMODE_Append)
	{
		*sd_mode |= PO_APPEND;
	}

	if(gn_mode & FSMODE_Create)
	{
		*sd_mode |= PO_CREAT | PO_TRUNC;
	}

	if(gn_mode & FSMODE_Truncate)
	{
		*sd_mode |= PO_TRUNC;
	}

	return 1;

#else
    switch (gn_mode)
    {
        case FSMODE_ReadOnly:
        {
            *sd_flags = PO_RDONLY;
            *sd_mode = PS_IREAD;
            break;
        }
        case FSMODE_WriteOnly:
        {
            *sd_flags = PO_WRONLY;
            *sd_mode = PS_IWRITE;
            break;
        }
        case FSMODE_ReadWrite:
        {
            *sd_flags = PO_RDWR | PO_CREAT;
            *sd_mode = PS_IREAD | PS_IWRITE;
            break;
        }
        case FSMODE_Append:
        {
            *sd_flags = PO_WRONLY | PO_APPEND;
            *sd_mode = PS_IWRITE;
            break;
        }
        case FSMODE_Create:
        {
            *sd_flags = PO_WRONLY | PO_CREAT | PO_TRUNC;
            *sd_mode = PS_IWRITE;
            break;
        }
        case FSMODE_Truncate:
        {
            *sd_flags = PO_WRONLY | PO_CREAT | PO_TRUNC;
            *sd_mode = PS_IWRITE;
            break;
        }
        case FSMODE_Exclusive:
        {
            *sd_flags = PO_RDONLY;
            *sd_mode = PS_IREAD;
            break;
        }
        default:
        {
//           diag_printf("gnfs_mode_to_sd_mode: Unsuported mode: %d\n", gn_mode);
           success = 0;
           gnfs_errno = FSERR_InvalidArg;
           break;
        }
    };

    return success;
#endif
}

/* blank stub */
gn_error_t
gnfs_configure(gnfs_config_t* config)
{
	return(SUCCESS);
}

/* blank stub */
gn_error_t gnfs_initialize()
{
    /* Technically correct, but in reality the disk will be initialized elsewhere. */
/*    pc_system_init(0); */

//	diag_printf("gnfs_initialize:\n");
//	diag_printf("sizeof(gnfs_mode_t): %d\n", sizeof(gnfs_mode_t));
//	diag_printf("File mode mappings:\n");
//	diag_printf("FSMODE_ReadOnly : %3d\n", FSMODE_ReadOnly);
//	diag_printf("FSMODE_WriteOnly: %3d\n", FSMODE_WriteOnly);
//	diag_printf("FSMODE_ReadWrite: %3d\n", FSMODE_ReadWrite);
//	diag_printf("FSMODE_Append   : %3d\n", FSMODE_Append);
//	diag_printf("FSMODE_Create   : %3d\n", FSMODE_Create);
//	diag_printf("FSMODE_Truncate : %3d\n", FSMODE_Truncate);
//	diag_printf("FSMODE_Exclusive: %3d\n", FSMODE_Exclusive);
//	diag_printf("\n");

	return(SUCCESS);
}

/* blank stub to shutdown file system */
gn_error_t gnfs_shutdown(void)
{
    /* Technically correct, but in reality the disk will be closed elsewhere. */
/*    pc_system_close(0); */
	return(SUCCESS);
}

gn_error_t
gnfs_set_fs_logging(gn_bool_t enable, gn_cstr_t logfile)
{
	return SUCCESS;
}

void
gnfs_add_fs_logging_info(gn_cstr_t info)
{
}

/* Functions which actually need to be written */
long _filelength(PCFD fh)
{
    INT32	old_offs;
    INT32	length;
    INT16   err_flag;

    /* save where we were before */
    old_offs = po_lseek(fh, 0, PSEEK_CUR, &err_flag);
   
    /* seek to end and get current size */
    length = po_lseek(fh, 0, PSEEK_END, &err_flag);

    /* now back to where we started */
    po_lseek(fh, old_offs, PSEEK_SET, &err_flag);

    return ((long)length);
}
	
long _tell(PCFD fh)
{
    INT16   err_flag;
	return (po_lseek(fh, 0, PSEEK_CUR, &err_flag));
}

#define	COPY_BUFF_SIZE	4096
int CopyFile(const char * oldname, const char * newname, int failifexist)
{
	int			oldfh = FS_INVALID_HANDLE;
	int			newfh = FS_INVALID_HANDLE;
	gn_uchar_t	buff[COPY_BUFF_SIZE];
	gn_error_t	err = GNERR_NoError;
	gn_size_t	readsize = 0;

	/* if the new file already exists, are we supposed to continue? */
	if (failifexist && (gnfs_exists(newname) == GN_TRUE))
	{
//		diag_printf("CopyFile: %s already exists\n", newname);
		return ABSTERR_FS_FileExists;
	}

	/* (attempt to) create the new file */
	newfh = gnfs_create(newname,FSMODE_WriteOnly,FSATTR_ReadWrite);
	if (newfh == FS_INVALID_HANDLE)
	{
		err = gnfs_get_error();
//		diag_printf("CopyFile: gnfs_create(%s) returned %d\n", newname, err);
		goto cleanup;
	}

	/* open the old file */
	oldfh = gnfs_open(oldname,FSMODE_ReadOnly);
	if (oldfh == FS_INVALID_HANDLE)
	{
		err = gnfs_get_error();
//		diag_printf("CopyFile: gnfs_open(%s) returned %d\n", newname, err);
		goto cleanup;
	}

	while (1)
	{
		/* read, copy, repeat, etc. */
		readsize = gnfs_read(oldfh, buff, COPY_BUFF_SIZE);
		if (readsize < 0)
		{
			err = gnfs_get_error();
//			diag_printf("CopyFile: gnfs_read() returned %d\n", err);
			goto cleanup;
		}

		if (readsize == 0)
			/* end of file - let's bail */
			break;

		if (gnfs_write(newfh, buff, readsize) != readsize)
		{
			/* error writing */
			err = gnfs_get_error();
//			diag_printf("CopyFile: gnfs_write() returned %d\n", err);
			goto cleanup;
		}

		if (readsize < COPY_BUFF_SIZE)
			/* last bit-o-info, let's bail */
			break;
	}

cleanup:
	if (oldfh != FS_INVALID_HANDLE)
		gnfs_close(oldfh);
	if (newfh != FS_INVALID_HANDLE)
	{
		if (err == SUCCESS)
		{
			gnfs_commit(newfh);
			gnfs_close(newfh);
		}
		else
		{
			gnfs_close(newfh);
			gnfs_delete(newname);
		}
	}

	return err;
}
#undef	COPY_BUFF_SIZE

static void set_gnfserrno(gn_int32_t error)
{
    DEBUGP(GNFS, DBGLEV_TRACE, "set_gnfserrno: error: %d\n", error);

	switch(error)
	{
		case PEACCES:
			gnfs_errno = FSERR_InvalidAccess;
			break;

		case PEEXIST:
			gnfs_errno = FSERR_FileExists;
			break;

		case PEINVAL:
			gnfs_errno = FSERR_InvalidArg;
			break;

		case PEMFILE:
			gnfs_errno = FSERR_Toomanyopen;
			break;

		case PENOENT:
			gnfs_errno = FSERR_NotFound;
			break;

		case PEBADF:
			gnfs_errno = FSERR_Invalidhandle;
			break;

		case PENOSPC:
			gnfs_errno = FSERR_NoSpace;
			break;

		default:
			gnfs_errno = FSERR_IckyError;
			break;
	}
}


/* open existing file for reading and/or writing */
gn_handle_t
gnfs_open(gn_cstr_t filename, gnfs_mode_t mode)
{
	gn_handle_t		handle;
    UINT16  flags = 0, pomode = 0;

    char fullfilename[GN_MAX_PATH];
    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_open: file: %s mode: %d\n", fullfilename, mode);

    if(gnfs_mode_to_sd_mode(mode, &flags, &pomode) == 0)
    {
        return FS_INVALID_HANDLE;
    }

    handle = po_open(fullfilename, flags, pomode);

	if (handle == -1)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
	}

//	diag_printf("handle: %d\n", handle);
	return((handle != -1) ? handle : FS_INVALID_HANDLE);
}

/* create file, leave open for reading and/or writing */
gn_handle_t
gnfs_create(gn_cstr_t filename, gnfs_mode_t mode, gnfs_attr_t attribute)
{
	gn_handle_t		handle;
    UINT16  flags = 0, pomode = 0;

    char fullfilename[GN_MAX_PATH];
    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_create: file: %s mode: %d attribute: %d\n", fullfilename, mode, attribute);

//    diag_printf("gnfs_create: mode: %d  attr: %d  filename: %s\n", mode, attribute, fullfilename);

    if(gnfs_mode_to_sd_mode(mode, &flags, &pomode) == 0)
    {
        return FS_INVALID_HANDLE;
    }

    flags |= PO_CREAT;

	handle = po_open(fullfilename, flags, pomode);
	if (handle == -1)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno);	*/
		return FS_INVALID_HANDLE;
	}

	return handle;
}

/* delete file */
gn_error_t
gnfs_delete(gn_cstr_t filename)
{
    char fullfilename[GN_MAX_PATH];
    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

	if(pc_unlink((TEXT*)fullfilename))
	{
		gnfs_errno = FSERR_NoError;
		return gnfs_errno;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		return(gnfs_errno);
	}
}

/* check to see if file exists */
gn_bool_t
gnfs_exists(gn_cstr_t filename)
{
    DSTAT stat;
    SDBOOL exists;
    char fullfilename[GN_MAX_PATH];

    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

    exists = pc_gfirst(&stat, (TEXT*)fullfilename);

    pc_gdone(&stat);
    return exists ? GN_TRUE : GN_FALSE;
}

/* copy file to new name */
gn_error_t
gnfs_copy_file(gn_cstr_t source_file, gn_cstr_t dest_file)
{
	int	err;

    char fullsourcefilename[GN_MAX_PATH], fulldestfilename[GN_MAX_PATH];
    strcpy(fullsourcefilename, s_szRootDir);
    strcat(fullsourcefilename, source_file);
    strcpy(fulldestfilename, s_szRootDir);
    strcat(fulldestfilename, dest_file);

//	err = CopyFile(fullsourcefilename,fulldestfilename,GN_TRUE);
	err = CopyFile(source_file, dest_file,GN_TRUE);

	if(err == 0)
	{	/* Success */
//		diag_printf("gnfs_copy_file: success: src: %s  dst: %s\n", source_file, dest_file);
		gnfs_errno = FSERR_NoError;
		return err;
	}
	else
	{
//		diag_printf("gnfs_copy_file: failed: src: %s  dst: %s\n", source_file, dest_file);
		gnfs_errno = FSERR_IckyError;
		return err;
	}
}


/* rename file. */
gn_error_t
gnfs_rename_file(gn_cstr_t old_file, gn_cstr_t new_file)
{
    char fullnewfilename[GN_MAX_PATH], fulloldfilename[GN_MAX_PATH];
    strcpy(fullnewfilename, s_szRootDir);
    strcat(fullnewfilename, new_file);
    strcpy(fulloldfilename, s_szRootDir);
    strcat(fulloldfilename, old_file);

	if(pc_mv((TEXT*)fulloldfilename, (TEXT*)fullnewfilename))
	{
		gnfs_errno = FSERR_NoError;
		return gnfs_errno;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		return gnfs_errno;
	}
}
	

/* get file attribute */
gnfs_attr_t
gnfs_get_attr(gn_cstr_t filename)
{
	UINT16	st;
    char fullfilename[GN_MAX_PATH];
    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

	if(pc_get_attributes((TEXT*)fullfilename, &st))
	{
		return st;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno);	*/
		return FSATTR_Invalid;
	}
}
	

/* set file attribute */
gn_error_t
gnfs_set_attr(gn_cstr_t filename, gnfs_attr_t attribute)
{
    UINT16  st;
    char fullfilename[GN_MAX_PATH];
    strcpy(fullfilename, s_szRootDir);
    strcat(fullfilename, filename);

	if(pc_get_attributes((TEXT*)fullfilename, &st))
	{
        attribute = st;
		gnfs_errno = FSERR_NoError;
		return gnfs_errno;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		return(gnfs_errno);
	}
}



/* read bytes from file */
gn_size_t
gnfs_read(gn_handle_t handle, void* buffer, gn_size_t size)
{
    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_read: file: %d size: %d\n", handle, size);

	size = po_read( handle, (UCHAR*)buffer, size);

	if (size == -1)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno); 	*/
		return 0;
	}
	else
	{
		return size;
	}
}

/* write bytes to file */
gn_size_t
gnfs_write(gn_handle_t handle, void* buffer, gn_size_t size)
{
    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_write: file: %d size: %d\n", handle, size);

	if((size = po_write( handle, (UCHAR*)buffer, size)) != -1)
	{
		return size;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno);	*/
		return 0;
	}

}

/* read bytes from file */
gn_size_t
gnfs_read_at(gn_handle_t handle, gn_foffset_t offset, void* buffer, gn_size_t size)
{
	gn_foffset_t	newoffset;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_read_at: file: %d offset: %d size: %d\n", handle, offset, size);

	newoffset = gnfs_seek(handle, offset, FS_SEEK_START);
	
	if(offset != newoffset)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno);	*/
		return 0;
	}

	if((size = po_read( handle, (UCHAR*)buffer, size)) == 0)
	{
		gnfs_errno = FSERR_EOF;
		return 0;
	}
	else if(size == -1)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno);	*/
		return 0;
	}
	else
	{
		return size;
	}
}

/* write bytes to file */
gn_size_t
gnfs_write_at(gn_handle_t handle, gn_foffset_t offset, void* buffer, gn_size_t size)
{
	gn_foffset_t	newoffset;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_write_at: file: %d offset: %d size: %d\n", handle, offset, size);

	newoffset = gnfs_seek(handle, offset, FS_SEEK_START);
	
	if(offset != newoffset)
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno); */
		return 0;
	}

	if((size = po_write( handle, (UCHAR*)buffer, size)) != -1)
	{
		return size;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
		/* GNERR_LOG_ERR(gnfs_errno); */
		return 0;
	}
}

/* get current file position */
gn_foffset_t
gnfs_tell(gn_handle_t handle)
{
	gn_foffset_t pos;

	if ((pos =_tell(handle)) == -1)
	{
		gnfs_errno = FSERR_Invalidhandle;
		/* GNERR_LOG_ERR(gnfs_errno); */
	}
	return pos;
}

/* set current file position */
gn_foffset_t
gnfs_seek(gn_handle_t handle, gn_foffset_t offset, int mode)
{
    INT16 err_flag;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_seek: file: %d offset: %d mode: %d\n", handle, offset, mode);

	return po_lseek(handle,offset,mode,&err_flag);
}

/* get end-of-file position */
gn_foffset_t
gnfs_get_eof(gn_handle_t handle)
{
	gn_foffset_t pos;

	if ((pos = _filelength(handle)) == -1)
	{
		gnfs_errno = FSERR_Invalidhandle;
		/* GNERR_LOG_ERR(gnfs_errno); */
		return FS_INVALID_OFFSET;
	}

	return pos;
}

/* set end-of-file position */
gn_foffset_t
gnfs_set_eof(gn_handle_t handle, gn_foffset_t offset)
{
	gn_foffset_t	newoffset = FS_INVALID_HANDLE;
    long    length;
    INT16   err_flag;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_set_eof: file: %d offset: %d\n", handle, offset);

    length = po_lseek(handle, 0, PSEEK_END, &err_flag);
    if (length > offset)
    {
	    po_truncate(handle, offset);
		return gnfs_seek(handle, 0L, FS_SEEK_END);
    }
    else if (length == offset)
		return length;
    else
    {
        UCHAR garbage[1024];
        newoffset = length;
        while (newoffset < offset)
        {
            long towrite = offset - newoffset < 1024 ? offset - newoffset : 1024;
            long written = po_write(handle, garbage, towrite);
            if (written < 0)
                return newoffset;
            newoffset += written;
        }
    }

	return newoffset;
}

/* commit changes to disk */
gn_error_t
gnfs_commit(gn_handle_t handle)
{
	int	err = FSERR_NoError;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_commit: file: %d\n", handle);

	if(po_flush(handle))
	{
		gnfs_errno = FSERR_NoError;
	}
	else
	{
        set_gnfserrno(pc_get_error(s_iRootDrive));
	}

	return(err);
}

/* close file */
gn_error_t
gnfs_close(gn_handle_t handle)
{
	gn_error_t	err;

    DEBUGP(GNFS, DBGLEV_TRACE, "gnfs_close: file: %d\n", handle);

	err = po_close(handle);

	if(err == -1) {
        set_gnfserrno(pc_get_error(s_iRootDrive));
		return(gnfs_errno);
	}
	else {
		gnfs_errno = FSERR_NoError;
		return gnfs_errno;
	}
}


/* retrieve last error */
gn_error_t
gnfs_get_error(void)
{
	return gnfs_errno;
}


/* Get the number of bytes free in given directory.
 * For a system with no per-directory restrictions, the value returned
 * represents all free space available on the disk.
 * The free_space_low parameter represents the low part of a 64-bit number.
 * The free_space_high parameter represents the high part of a 64-bit number.
 */
gn_error_t
gnfs_get_disk_free_space(gn_cstr_t directory, gn_uint64_t* free_space)
{
	*free_space = (gn_uint64_t)pc_free64(s_iRootDrive);

    return FSERR_NoError;
}
