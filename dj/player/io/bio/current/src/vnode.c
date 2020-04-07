#include <cyg/error/codes.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include <io/bio/vnode.h>
#include <util/debug/debug.h>

DEBUG_USE_MODULE(BIO);

int
IFTOVT(int mode)
{
    switch (mode & 0xff) {
	case __stat_mode_DIR:
	{
	    return (CYG_FILE_TYPE_DIR);
	}
	case __stat_mode_CHR:
	{
	    return (CYG_FILE_TYPE_CHR);
	}
	case __stat_mode_BLK:
	{
	    return (CYG_FILE_TYPE_BLK);
	}
	case __stat_mode_REG: 
	{
	    return (CYG_FILE_TYPE_REG);
	}
	default: 
	{
	    return (CYG_FILE_TYPE_NON);
	}
    }
    return (CYG_FILE_TYPE_NON);
}

int
getnewvnode(cyg_mtab_entry * mte,
	    struct CYG_FILEOPS_TAG * vops,
	    cyg_file * vp)
{
    vp->f_type = CYG_FILE_TYPE_NON;
    vp->f_ops = vops;
    vp->f_offset = 0;
    vp->f_data = 0;
    vp->f_xops = 0;
    vp->f_mte = mte;
    vp->f_syncmode = mte->fs->syncmode;
    
    return (0);
}


void
vrele(cyg_file *vp)
{
    if (vp == NULL)
        DEBUGP(BIO, DBGLEV_ERROR, "XXX vrele: null vp XXX");

#if 0
    vp->f_ucount--;
    if (vp->f_ucount > 0) {
        return;
    }
    // todo: properly handle reference counting/node release
    //  cd9660_inactive is just a sane free
    cd9660_inactive(vp);
#endif
}


int
dupvnode(cyg_file * dst, cyg_file * src, int sz)
{
    if (dst != src) {
        memcpy(dst, src, sizeof(cyg_file));
        if (src->f_data) {
            dst->f_data = malloc( sz );
            //            (struct iso_node *)dst->f_data = (struct iso_node *)malloc(sizeof(struct iso_node));
            INODEMLOG(dst->f_data);
            if (dst->f_data == 0) {
                return (ENOMEM);
            }
            memcpy( (void*)dst->f_data, (const void*)src->f_data, sz );
            //            memcpy((void *)dst->f_data, (const void *)src->f_data, sizeof(struct iso_node));
        }
    }
    return (0);
}

