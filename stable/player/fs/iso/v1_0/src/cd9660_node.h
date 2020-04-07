#ifndef CD9660_NODE_H
#define CD9660_NODE_H

#include <cyg/fileio/fileio.h>
#include "cd9660_iso.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
/* macros to track inode allocation */
// inode_malloc_log(ip)
#define INODEMLOG(ip) /**/
// inode_free_log(ip)
#define INODEFLOG(ip) /**/

/* macro/inline free for leak tracing */
#define vrele(vp) { if( ((vp)->f_type != CYG_FILE_TYPE_BLK) && VTOI(vp)) { INODEFLOG(VTOI(vp)); free(VTOI(vp)); VTOI(vp) = 0;} }

#define vput vrele
#define VREF(dp) /**/
#define VTOI(vp) ((struct iso_node *)(vp)->f_data)
#define ITOV(ip) ((ip)->i_vnode)
#define VFSTOISOFS(mp)  ((struct iso_mnt *)((mp)->data))
  
#define blksize(imp, ip, lbn)   ((imp)->logical_block_size)
#define blkoff(imp, loc)        ((loc) & (imp)->im_bmask)
    
int IFTOVT(int mode);
int getnewvnode(cyg_mtab_entry * mte, struct CYG_FILEOPS_TAG * vops, cyg_file *vp);
int dupvnode(cyg_file * dst, cyg_file * src);    
// void vrele(cyg_file *vp);
    
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CD9660_VNODE_H */
