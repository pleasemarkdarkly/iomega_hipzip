#ifndef CD9660_INTERNAL_H
#define CD9660_INTERNAL_H

#include <cyg/fileio/fileio.h>
#include "cd9660_iso.h"
#include "cd9660_bio.h"
#include "cd9660_lookup.h"
#include "cd9660_support.h"		/* TODO This is for struct timespec */

#define lblkno(imp, loc)        ((loc) >> (imp)->im_bshift)
#define VFSTOISOFS(mp)	((struct iso_mnt *)((mp)->data))

int cd9660_bmap(cyg_file * vp, cyg_int32 bn, cyg_file ** vpp, cyg_int32 * bnp, int * runp);
int cd9660_vget_internal(cyg_mtab_entry * mp, ino_t ino, cyg_file * vp, int relocated,
			 struct iso_directory_record * isodir);
int cd9660_open_internal(struct nameidata * ndp, int fmode, int cmode);
int cd9660_inactive(cyg_file * vp);
void cd9660_defattr(struct iso_directory_record * isodir, struct iso_node * inop, struct buf * bp);
void cd9660_deftstamp(struct iso_directory_record * isodir, struct iso_node * inop, struct buf * bp);
int cd9660_tstamp_conv7(unsigned char * pi, struct timespec * pu);
int cd9660_tstamp_conv17(unsigned char * pi, struct timespec * pu);
int cd9660_blkatoff(cyg_file * vp, off_t offset, char ** res, struct buf ** bpp);
int cd9660_lookup(cyg_file * dvp, cyg_file * vp, struct componentname * cnp);
int cd9660_pathconf(cyg_file * vp, struct cyg_pathconf_info * info);
int cd9660_stat_internal(cyg_file *vp, struct stat *sb);
    
ino_t isodirino(struct iso_directory_record * isodir, struct iso_mnt * imp);
void isofntrans(cyg_uint8 *infn, int infnlen, cyg_uint8 *outfn, cyg_uint16 *outfnlen,
		int original, int assoc, int joliet_level);
int isochar(const cyg_uint8 *isofn, const cyg_uint8 *isoend, int joliet_level,
	    cyg_uint8 *c);
int isofncmp(const cyg_uint8 *fn, int fnlen, const cyg_uint8 *isofn, int isolen, int joliet_level);

#endif /* CD9660_INTERNAL_H */
