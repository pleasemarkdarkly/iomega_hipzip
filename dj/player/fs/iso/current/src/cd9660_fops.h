#ifndef CD9660_FOPS_H
#define CD9660_FOPS_H

#include <cyg/fileio/fileio.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern cyg_fileops cd9660_fileops;
    
int cd9660_fo_read(cyg_file * fp, cyg_uio * uio);
int cd9660_fo_lseek(cyg_file * fp, off_t * pos, int whence);
int cd9660_fo_ioctl(cyg_file * fp, CYG_ADDRWORD cmd, CYG_ADDRWORD data);
int cd9660_fo_fsync(cyg_file * fp, int mode);
int cd9660_fo_close(cyg_file * fp);
int cd9660_fo_fstat(cyg_file * fp, struct stat * sbp);
int cd9660_fo_getinfo(cyg_file * fp, int key, void * buf, int len);
int cd9660_fo_setinfo(cyg_file * fp, int key, void * buf, int len);

#ifdef __cplusplus
};
#endif /* __cplusplus */
	
#endif /* CD9660_FOPS_H */
