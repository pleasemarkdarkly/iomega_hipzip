#ifndef __MSDOSFS_FOPS_H__
#define __MSDOSFS_FOPS_H__


/* File operations */
int msdosfs_fo_read    (struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio);
int msdosfs_fo_write   (struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio);
int msdosfs_fo_lseek   (struct CYG_FILE_TAG* fp, off_t* pos, int whence );
int msdosfs_fo_ioctl   (struct CYG_FILE_TAG* fp, CYG_ADDRWORD com, CYG_ADDRWORD data);
int msdosfs_fo_fsync   (struct CYG_FILE_TAG* fp, int mode);
int msdosfs_fo_close   (struct CYG_FILE_TAG* fp);
int msdosfs_fo_fstat   (struct CYG_FILE_TAG* fp, struct stat* buf );
int msdosfs_fo_getinfo (struct CYG_FILE_TAG* fp, int key, void* buf, int len );
int msdosfs_fo_setinfo (struct CYG_FILE_TAG* fp, int key, void* buf, int len );

int msdosfs_fo_dirread (struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio);
int msdosfs_fo_dirlseek(struct CYG_FILE_TAG* fp, off_t* pos, int whence );

extern cyg_fileops fsfat_fileops;
extern cyg_fileops fsfat_dirops;

#endif // __MSDOSFS_FOPS_H__

