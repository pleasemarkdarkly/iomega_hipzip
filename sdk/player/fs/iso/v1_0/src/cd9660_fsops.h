#ifndef CD9660_FSOPS_H
#define CD9660_FSOPS_H

#include <cyg/fileio/fileio.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int cd9660_fso_mount(cyg_fstab_entry * fsp, cyg_mtab_entry * mp);
int cd9660_fso_unmount(cyg_mtab_entry * mp);
int cd9660_fso_open(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int mode, cyg_file * fp);
int cd9660_fso_opendir(cyg_mtab_entry * mp, cyg_dir dir, const char * name, cyg_file * fp);
int cd9660_fso_chdir(cyg_mtab_entry * mp, cyg_dir dir, const char * name, cyg_dir * dir_out);
int cd9660_fso_stat(cyg_mtab_entry * mp, cyg_dir dir, const char * name, struct stat * sbp);
int cd9660_fso_getinfo(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int key, void * buf, int len);
int cd9660_fso_setinfo(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int key, void * buf, int len);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CD9660_FSOPS_H */
