// dpfs.c: fs api for dataplay drives
// danc@iobjects.com 12/15/2000
// copied the form and the goodness of the ramfs example driver

#include <pkgconf/io_fileio.h>

#include <cyg/kernel/ktypes.h>

#include <unistd.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include <cyg/fileio/fileio.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

#include "dp.h"
#include "dpfs.h"
#include "dpcodes.h"

typedef struct dpfs_node_s {
	DFSHANDLE  handle;
	cyg_uint64 size;
} dpfs_node_t;

static int dpfs_mount    ( cyg_fstab_entry* fst, cyg_mtab_entry* mte );
static int dpfs_umount   ( cyg_mtab_entry *mte );
static int dpfs_open     ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             int mode,  cyg_file *fte );
static int dpfs_unlink   ( cyg_mtab_entry *mte, cyg_dir dir, const char *name );
static int dpfs_mkdir    ( cyg_mtab_entry *mte, cyg_dir dir, const char *name );
static int dpfs_rename   ( cyg_mtab_entry *mte, cyg_dir dir1, const char *name1,
                             cyg_dir dir2, const char *name2 );
static int dpfs_link     ( cyg_mtab_entry *mte, cyg_dir dir1, const char *name1,
                             cyg_dir dir2, const char *name2, int type );
static int dpfs_opendir  ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             cyg_file *fte );
static int dpfs_chdir    ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             cyg_dir *dir_out );
static int dpfs_stat     ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             struct stat *buf);
static int dpfs_getinfo  ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             int key, void *buf, int len );
static int dpfs_setinfo  ( cyg_mtab_entry *mte, cyg_dir dir, const char *name,
                             int key, void *buf, int len );

// File operations
static int dpfs_fo_read      (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio);
static int dpfs_fo_write     (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio);
static int dpfs_fo_lseek     (struct CYG_FILE_TAG *fp, off_t *pos, int whence );
static int dpfs_fo_ioctl     (struct CYG_FILE_TAG *fp, CYG_ADDRWORD com,
                                CYG_ADDRWORD data);
static int dpfs_fo_fsync     (struct CYG_FILE_TAG *fp, int mode );        
static int dpfs_fo_close     (struct CYG_FILE_TAG *fp);
static int dpfs_fo_fstat     (struct CYG_FILE_TAG *fp, struct stat *buf );
static int dpfs_fo_getinfo   (struct CYG_FILE_TAG *fp, int key, void *buf, int len );
static int dpfs_fo_setinfo   (struct CYG_FILE_TAG *fp, int key, void *buf, int len );

// Directory operations
static int dpfs_fo_dirread      (struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio);
static int dpfs_fo_dirlseek     (struct CYG_FILE_TAG *fp, off_t *pos, int whence );


FSTAB_ENTRY( dpfs_fste, "dpfs", 0,
             CYG_SYNCMODE_FILE_FILESYSTEM|CYG_SYNCMODE_IO_FILESYSTEM,
             dpfs_mount,
             dpfs_umount,
             dpfs_open,
             dpfs_unlink,
             dpfs_mkdir,
			 dpfs_unlink,  // rmdir is the same as unlink
             dpfs_rename,
             dpfs_link,
             dpfs_opendir,
             dpfs_chdir,
             dpfs_stat,
             dpfs_getinfo,
             dpfs_setinfo);

static cyg_fileops dpfs_fileops =
{
    dpfs_fo_read,
    dpfs_fo_write,
    dpfs_fo_lseek,
    dpfs_fo_ioctl,
    cyg_fileio_seltrue,
    dpfs_fo_fsync,
    dpfs_fo_close,
    dpfs_fo_fstat,
    dpfs_fo_getinfo,
    dpfs_fo_setinfo
};

static cyg_fileops dpfs_dirops =
{
    dpfs_fo_dirread,
    (cyg_fileop_write *)cyg_fileio_enosys,
    dpfs_fo_dirlseek,
    (cyg_fileop_ioctl *)cyg_fileio_enosys,
    cyg_fileio_seltrue,
    (cyg_fileop_fsync *)cyg_fileio_enosys,
    dpfs_fo_close,
    (cyg_fileop_fstat *)cyg_fileio_enosys,
    (cyg_fileop_getinfo *)cyg_fileio_enosys,
    (cyg_fileop_setinfo *)cyg_fileio_enosys
};

// ****************** functions **********************

static int dpfs_mount( cyg_fstab_entry* fst, cyg_mtab_entry* mte ) {
	dpfs_node_t* root;
	dp_media_info_t media_info;
	int err;

	root = (dpfs_node_t*) malloc( sizeof( dpfs_node_t ) );
	if( root == NULL ) {
		return ENOSPC;
	}

	// load the media
	err = dp_load_media();

	if( err != DP_NO_ERROR ) {
		free( root );
		return ENOSPC;
	}

	// lock the drive
	err = dp_lock_media();

	if( err != DP_NO_ERROR ) {
		free( root );
		return ENOSPC;  // TODO XXX use real error codes
	}

	// get the media info
	err = dp_get_media_info( &media_info );

	if( err != DP_NO_ERROR ) {
		free( root );
		return ENOSPC;  // TODO XXX use real error codes
	}

	root->handle = media_info.handle;
	// copy the handle to the root directory
	mte->root = (cyg_dir) root;

	return ENOERR;
}

static int dpfs_umount( cyg_fstab_entry* fst, cyg_mtab_entry* mte ) {
	dpfs_node_t* root = (dpfs_node_t*) mte->root;
	int err;

	// commit any data just to be sure
	err = dfs_commit();

	// unlock the media
	err = dp_release_media();

	// free our ptr
	free( root );

	mte->root = CYG_DIR_NULL;

	return ENOERR;
}

static int dpfs_open( cyg_mtab_entry* mte, cyg_dir dir,
					 const char* name, int mode, cyg_file* file )
{
	dpfs_node_t* root = (dpfs_node_t*) dir;
	dpfs_node_t* fl = (dpfs_node_t*) malloc( sizeof( dpfs_node_t ) );
	dp_file_info_t f_info;
	DFSHANDLE h;
	int err = ENOENT;

	if( fl == NULL ) {
		return ENOSPC;
	}

	h = dfs_get_handle( root->handle, 0, name );
	if( h == -1 ) { 
		// file not found
		if( mode & O_CREAT ) {
			// the create bit is set, generate a new file
			h = dfs_createfile( dir, 0, name );
			if( h != -1 ) {
				err = ENOERR;
			}
			fl->size = 0;
		}
	} else {
		// file was found, make sure open behaves properly
		if( (mode & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL) ) {
			err = EEXIST;
		} else {
			err = ENOERR;
		}
		// figure out how big this file is
		dfs_get_file_info( h, &f_info );
		fl->size = f_info.file_size;
	}

	if( err != ENOERR ) {
		free( fl );
		return err;   // TODO XXX
	}

	fl->handle = h;

	file->f_flag  |= mode & CYG_FILE_MODE_MASK;
	file->f_type   = CYG_FILE_TYPE_FILE;
	file->f_ops    = &dpfs_fileops;
	file->f_offset = (mode & O_APPEND) ? fl->size : 0;
	file->f_data   = (CYG_ADDRWORD) fl;
	file->f_xops   = 0;

	return ENOERR;
}

static int dpfs_unlink( cyg_mtab_entry* mte, cyg_dir dir, const char* name ) {
	dpfs_node_t* root = (dpfs_node_t*) dir;
	DFSHANDLE h;
	int err;

	h = dfs_get_handle( root->handle, 0, name );
	if( h == -1 ) {
		return EEXIST;  // TODO XXX
	}

	err = dfs_delete( h );

	return err;

}

static int dpfs_mkdir( cyg_mtab_entry* mte, cyg_dir dir, const char* name ) {
	dpfs_node_t* root = (dpfs_node_t*) dir;
	DFSHANDLE h;

	h = dfs_mkdir( root->handle, name );
	if( h == -1 ) {
		return EEXIST; // TODO XXX
	}
	return ENOERR;
}

static int dpfs_rename( cyg_mtab_entry* mte,
					   cyg_dir dir1, const char* name1,
					   cyg_dir dir2, const char* name2 )
{
	dpfs_node_t* parent1 = (dpfs_node_t*) dir1;
	dpfs_node_t* parent2 = (dpfs_node_t*) dir2;

	// this is not implemented yet

	return ENOERR;
}

static int dpfs_link( cyg_mtab_entry* mte,
					 cyg_dir dir1, const char* name1,
					 cyg_dir dir2, const char* name2, int type )
{
	return ENOERR;
}

static int dpfs_opendir( cyg_mtab_entry* mte, cyg_dir dir,
						const char* name, cyg_file* file )
{
	return ENOERR;
}

static int dpfs_chdir( cyg_mtab_entry* mte, cyg_dir dir,
					  const char* name, cyg_dir* dir_out )
{
	return ENOERR;
}

static int dpfs_stat( cyg_mtab_entry* mte, cyg_dir dir,
					  const char* name, struct stat* buf )
{
	return ENOERR;
}

static int dpfs_getinfo( cyg_mtab_entry* mte, cyg_dir dir,
						const char* name, int key, void* buf,
						int len )
{
	return EINVAL;
}

static int dpfs_setinfo( cyg_mtab_entry* mte, cyg_dir dir,
						const char* name, int key, void* buf,
						int len )
{
	return EINVAL;
}

static int dpfs_fo_read( struct CYG_FILE_TAG* fp,
						struct CYG_UIO_TAG* uio )
{
	dpfs_node_t* node = (dpfs_node_t*) fp->f_data;
	int i;
	off_t pos = fp->f_offset;
	ssize_t resid = uio->uio_resid;

	// yay io vectors
	for( i = 0; i < uio->uio_iovcnt; i++ ) {
		cyg_iovec* iov = &uio->uio_iov[i];
		char* buf = (char*) iov->iov_base;
		off_t len = iov->iov_len;
		int err;

		// make sure we dont ask for too much
		if( len > node->size - pos ) {
			len = node->size - pos;
		}

		err = dfs_read( node->handle, buf, pos, len, 0 );
		if( err != DP_NO_ERROR ) {
			return err;  // TODO XXX
		}
		resid -= len;
		pos += len;
	}

	uio->uio_resid = resid;
	fp->f_offset = pos;

	return ENOERR;
}

static int dpfs_fo_write( struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio )
{
	dpfs_node_t* node = (dpfs_node_t*) fp->f_data;
	off_t pos = fp->f_offset;
	ssize_t resid = uio->uio_resid;
	int i;

	// append mode only
	if( !( fp->f_flag & CYG_FAPPEND ) ) {
		return EINVAL;
	}

	for( i = 0; i < uio->uio_iovcnt; i++ ) {
		cyg_iovec *iov = &uio->uio_iov[i];
		char* buf = (char*) iov->iov_base;
		off_t len = iov->iov_len;
		int err;

		err = dfs_write( node->handle, buf, len );
		if( err != DP_NO_ERROR ) {
			return err; // TODO XXX
		}
		resid -= len;
		pos += len;
	}
	uio->uio_resid = resid;
	fp->f_offset = pos;

	return ENOERR;
}

static int dpfs_fo_lseek( struct CYG_FILE_TAG *fp, off_t *apos, int whence )
{
	dpfs_node_t* node = (dpfs_node_t*) fp->f_data;
	off_t pos = *apos;    // is off_t really signed?

	switch( whence )
	{
	case SEEK_SET:
		{
			break;
		}
	case SEEK_CUR:
		{
			pos += fp->f_offset;
			break;
		}
	case SEEK_END:
		{
			pos += node->size;
			break;
		}
	default:
		{
			return EINVAL;
		}
	}

	if( pos < 0 || pos > node->size ) {
		return EINVAL;
	}

	*apos = fp->f_offset = pos;

	return ENOERR;
}

static int dpfs_fo_ioctl(struct CYG_FILE_TAG *fp, CYG_ADDRWORD com,
                                CYG_ADDRWORD data )
{
	// handle ioctls
	// at some point this would be one place that we could define ioctls
	// to eject media possibly. we need to find a way to make a single interface
	// to the end user, this may be one way
	return EINVAL;
}

static int dpfs_fo_fsync( struct CYG_FILE_TAG *fp, int mode )
{
	int err;
	
	err = dfs_commit();

	if( err != DP_NO_ERROR ) {
		return err;    // TODO XXX
	}
	return ENOERR;
}

static int dpfs_fo_close( struct CYG_FILE_TAG *fp )
{
	// close only makes sense in the context of a file opened for
	// append mode, in which case it should be a commit
	int err = EINVAL;

	if( node->f_flag & CYG_FAPPEND ) {
		err = dfs_commit();
	}
	iff( err != DP_NO_ERROR ) {
		return err;
	}
	return ENOERR;
}

static int dpfs_fo_fstat( struct CYG_FILE_TAG *fp, struct stat *buf )
{
	return EINVAL;
}

static int dpfs_fo_getinfo( struct CYG_FILE_TAG *fp, int key, void *buf, int len )
{
	return EINVAL;
}

static int dpfs_fo_setinfo( struct CYG_FILE_TAG *fp, int key, void *buf, int len )
{
	return EINVAL;
}


// Directory operations
static int dpfs_fo_dirread( struct CYG_FILE_TAG *fp, struct CYG_UIO_TAG *uio )
{
	return EINVAL;
}

static int dpfs_fo_dirlseek( struct CYG_FILE_TAG *fp, off_t *pos, int whence )
{
	return EINVAL;
}

