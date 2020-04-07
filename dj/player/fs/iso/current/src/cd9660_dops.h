#ifndef CD9660_DOPS_H
#define CD9660_DOPS_H

#include <cyg/fileio/fileio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern cyg_fileops cd9660_dirops;
    
int cd9660_do_read(cyg_file * fp, cyg_uio * uio);
int cd9660_do_lseek(cyg_file * fp, off_t * pos, int whence);

#ifdef __cplusplus
};
#endif /* __cplusplus */
	
#endif /* CD9660_DOPS_H */
