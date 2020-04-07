#include "cd9660_fsops.h"
#include "cd9660_dops.h"
#include "cd9660_rrip.h"
#include "cd9660_internal.h"
#include "cd9660_iso.h"
#include "cd9660_bio.h"
#include "cd9660_node.h"
#include "cd9660_support.h"
#include <stdlib.h>
#include <cyg/io/io.h>
//#include <atadrv/include/atadrv.h>
#include <io/storage/blk_dev.h>

static int _mount(cyg_file * devvp, cyg_mtab_entry * mp);
static int _root(cyg_mtab_entry *mp, cyg_file *vp);
    
/* TODO Optimize SYNCMODE flag */
FSTAB_ENTRY(cd9660_fste, "cd9660", 0,
	    CYG_SYNCMODE_FILE_FILESYSTEM | CYG_SYNCMODE_IO_FILESYSTEM,
	    cd9660_fso_mount,
	    cd9660_fso_unmount,
	    cd9660_fso_open,
	    (cyg_fsop_unlink *)cyg_fileio_erofs,
	    (cyg_fsop_mkdir *)cyg_fileio_erofs,
	    (cyg_fsop_rmdir *)cyg_fileio_erofs,
	    (cyg_fsop_rename *)cyg_fileio_erofs,
	    (cyg_fsop_link *)cyg_fileio_erofs,
	    cd9660_fso_opendir,
	    cd9660_fso_chdir,
	    cd9660_fso_stat,
	    cd9660_fso_getinfo,
	    cd9660_fso_setinfo
    );

int
cd9660_fso_mount(cyg_fstab_entry * fsp, cyg_mtab_entry * mp)
{
    cyg_file * devvp = 0;
    int error = 0;
    cyg_io_handle_t handle = 0;
    cyg_uint32 len;

    /* initialize and invalidate the block cache for this drive */
    binvalidate();
    
    /* Create a cyg_file object representing the device the root is mounted on */
    error = cyg_io_lookup(mp->devname, &handle);
    if (error < 0) {
        return error;
    }
    len = 1;
    error = cyg_io_set_config(handle, IO_PM_SET_CONFIG_REGISTER, 0, &len);
    if (error < 0) {
        return error;
    }

    devvp = (cyg_file *)malloc(sizeof(cyg_file));
    if (devvp == NULL) {
        return (ENOMEM);
    }
    devvp->f_flag = CYG_FALLOC;
    devvp->f_ucount = 0;

    if ((error = getnewvnode(mp, 0, devvp)) != 0) {
        return (error);
    }
    devvp->f_flag |= CYG_FREAD;
    devvp->f_type = CYG_FILE_TYPE_BLK;
    devvp->f_offset = 0;
    devvp->f_data = (CYG_ADDRWORD)handle;
    devvp->f_xops = 0;
    devvp->f_mte = mp;
    devvp->f_syncmode = mp->fs->syncmode;

    /* Mount the device */
    error = _mount(devvp, mp);
    if (error) {
        free(devvp);
        return (error);
    }

    /* Create a cyg_file object representing the root node */
    (cyg_file *)mp->root = (cyg_file *)malloc(sizeof(cyg_file));
    if (mp->root == 0) {
        return (ENOMEM);
    }
    ((cyg_file *)mp->root)->f_flag = CYG_FALLOC;
    ((cyg_file *)mp->root)->f_ucount = 0;

    if ((error = getnewvnode(mp, 0, (cyg_file *)mp->root)) != 0) {
        return (error);
    }
    ((cyg_file *)mp->root)->f_flag |= CYG_FREAD;
    ((cyg_file *)mp->root)->f_type = CYG_FILE_TYPE_REG;

    /* Get the root node */
    error = _root(mp, (cyg_file *)(mp->root));
    if (error) {
        free(devvp);
        vrele((cyg_file *)mp->root);
        free((cyg_file *)mp->root);
	return (error);
    }
	
    return (0);
}

int
cd9660_fso_unmount(cyg_mtab_entry * mp)
{
    struct iso_mnt *isomp;
    int error = 0;
    int len;
    
#if 0
    /* TODO Take a look at this, check reference count */
    if ((error = vflush(mp, NULLVP, flags)) != 0)
        return (error);
#endif
    vrele((cyg_file *)mp->root);
    free((cyg_file *)mp->root);
	
    isomp = VFSTOISOFS(mp);

    len = 1;
    error = cyg_io_set_config((cyg_io_handle_t)isomp->im_devvp->f_data, IO_PM_SET_CONFIG_UNREGISTER, 0, &len);
    /* TODO Assume that unregister worked, what could be done to recover? */
    
    free((cyg_file *)isomp->im_devvp);
    free((char *)isomp);
    mp->data = (CYG_ADDRWORD)0;
    
    return (error);
}

int
cd9660_fso_open(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int mode, cyg_file * vp)
{
    int cmode;
    int error;
    struct nameidata nd;
    
    /* Initialize data structure for parsing path */
    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE,
	   name, (cyg_file *)mp->root, (cyg_file *)dir);

    /* Set default creation mask to all permissions */
    cmode = S_IRWXU | S_IRWXG | S_IRWXO;
    
    /* Find the node */
    vp->f_mte = mp;
    nd.ni_vp = vp;
	vp->f_data = 0;

    if ((error = cd9660_open_internal(&nd, mode, cmode)) != 0) {
        return (error);
    }
    
    if (vp->f_type == CYG_FILE_TYPE_DIR)
        return EISDIR;
    
    /* Initialize file object */
    vp->f_flag |= mode & CYG_FILE_MODE_MASK;
    
    return 0;
}

int
cd9660_fso_opendir(cyg_mtab_entry * mp, cyg_dir dir, const char * name, cyg_file * vp)
{
    int mode;
    int cmode;
    int error;
    struct nameidata nd;

    /* Initialize data structure for parsing path */
    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE,
	   name, (cyg_file *)mp->root, (cyg_file *)dir);

    /* Set default file masks */
    cmode = S_IRWXU | S_IRWXG | S_IRWXO;
    mode = O_RDONLY;

    vp->f_mte = mp;
    nd.ni_vp = vp;
	vp->f_data = 0;

    if ((error = cd9660_open_internal(&nd, mode, cmode)) != 0) {
        return (error);
    }

    if (vp->f_type != CYG_FILE_TYPE_DIR)
        return ENOTDIR;
	
    /* Initialize file object */
    vp->f_flag |= mode & CYG_FILE_MODE_MASK;
    vp->f_ops = &cd9660_dirops;
    vp->f_offset = 0;
    vp->f_xops = 0;

    return (0);
}

int
cd9660_fso_chdir(cyg_mtab_entry * mp, cyg_dir dir, const char * name, cyg_dir * dir_out)
{
    /* If dir_out is not NULL then locate the named directory and
     * and put the directory pointer in dir_out */
    if (dir_out != NULL) {
        /* Open name and stuff the cyg_file * into cyg_dir */
	
        /* If name is null, then root is desired */
    }
    /* Else if dir_out is NULL then dir can be freed */
    else {
    }
    
    return 0;
}

int
cd9660_fso_stat(cyg_mtab_entry * mp, cyg_dir dir, const char * name, struct stat * sbp)
{
    int error;
    struct nameidata nd;

    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE,
	   name, (cyg_file *)mp->root, (cyg_file *)dir);
    nd.ni_vp = (cyg_file *)malloc(sizeof(cyg_file));
	
	memset(nd.ni_vp,0,sizeof(cyg_file));

    if (nd.ni_vp == 0) {
        return ENOMEM;
    }
    if ((error = namei(&nd)) != 0)
        return (error);
    error = cd9660_stat_internal(nd.ni_vp, sbp);
    vrele(nd.ni_vp);
    free(nd.ni_vp);
    return (error);
}

int
cd9660_fso_getinfo(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int key, void * buf, int len)
{
    int error;
    struct nameidata nd;
    
    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE,
	   name, (cyg_file *)mp->root, (cyg_file *)dir);
    if ((error = namei(&nd)) != 0)
        return (error);
    
    switch(key) {
	case FS_INFO_CONF:
	    error = cd9660_pathconf(nd.ni_vp, (struct cyg_pathconf_info *)buf);
	    break;
	    
	default:
	    error = EINVAL;
	    break;
    }
    
    return (error);
}

int
cd9660_fso_setinfo(cyg_mtab_entry * mp, cyg_dir dir, const char * name, int key, void * buf, int len)
{
    return EINVAL;
}

static int
_mount(cyg_file * devvp,
       cyg_mtab_entry * mp)
{
    struct iso_mnt *isomp = (struct iso_mnt *)0;
    struct buf *bp = NULL;
    struct buf *pribp = NULL, *supbp = NULL;
    int error = EINVAL;
    int iso_bsize;
    int iso_blknum, lba = 0;
    int joliet_level;
    struct iso_volume_descriptor *vdp;
    struct iso_primary_descriptor *pri = NULL;
    struct iso_supplementary_descriptor *sup = NULL;
    struct iso_directory_record *rootp;
    int logical_block_size;
    int flags = 0;
    cyg_uint32 args;
    cyg_uint32 len;
    int n;

    /* Open device */
    /* TODO Is this a hack? */
    len = sizeof(args);
    n = 0;
    do {
        error = cyg_io_get_config((cyg_io_handle_t)devvp->f_data, IO_BLK_GET_CONFIG_MEDIA_STATUS, &args, &len);
        ++n;
    } while ((n < 5) && error);
    if (error) 
        return (error);
	
    /* This is the "logical sector size".  The standard says this
     * should be 2048 or the physical sector size on the device,
     * whichever is greater.  For now, we'll just use a constant.
     */
    iso_bsize = ISO_DEFAULT_BLOCK_SIZE;

    
    if( sscanf( mp->fsname, "%*[a-z0-9]:%d", &lba ) == 0 ) {
        lba = 0;
    }
    
    joliet_level = 0;
    for( iso_blknum = 16 + lba; iso_blknum < (100 + lba); iso_blknum++ ) {
        
        if ((error = bread(devvp, iso_blknum * btodb(iso_bsize),
                 iso_bsize, &bp)) != 0)
            goto out;
		
        vdp = (struct iso_volume_descriptor *)bp->b_data;
        if (bcmp(vdp->id, ISO_STANDARD_ID, sizeof vdp->id) != 0) {
            error = EINVAL;
            goto out;
        }
		
        switch (isonum_711 (vdp->type)){
            case ISO_VD_PRIMARY:
                if (pribp == NULL) {
                    pribp = bp;
                    bp = NULL;
                    pri = (struct iso_primary_descriptor *)vdp;
                }
                break;
            case ISO_VD_SUPPLEMENTARY:
                if (supbp == NULL) {
                    supbp = bp;
                    bp = NULL;
                    sup = (struct iso_supplementary_descriptor *)vdp;
            
                    if (bcmp(sup->escape, "%/@", 3) == 0)
                        joliet_level = 1;
                    if (bcmp(sup->escape, "%/C", 3) == 0)
                        joliet_level = 2;
                    if (bcmp(sup->escape, "%/E", 3) == 0)
                        joliet_level = 3;
				
                    if (isonum_711 (sup->flags) & 1)
                        joliet_level = 0;
                }
                break;
                
            case ISO_VD_END:
                goto vd_end;
  
            default:
                break;
        }
        if (bp) {
            brelse(bp);
            bp = NULL;
        }
    }
  vd_end:
    if (bp) {
        brelse(bp);
        bp = NULL;
    }
  
    if (pri == NULL) {
        error = EINVAL;
        goto out;
    }

    logical_block_size = isonum_723 (pri->logical_block_size);
	
    if (logical_block_size < DEV_BSIZE /* TODO || logical_block_size > MAXBSIZE */
        || (logical_block_size & (logical_block_size - 1)) != 0) {
        error = EINVAL;
        goto out;
    }
	
    rootp = (struct iso_directory_record *)pri->root_directory_record;
	
    isomp = (struct iso_mnt *)malloc(sizeof *isomp);
    bzero((char *)isomp, sizeof *isomp);
    isomp->logical_block_size = logical_block_size;
    isomp->volume_space_size = isonum_733 (pri->volume_space_size);
    bcopy (rootp, isomp->root, sizeof isomp->root);
    isomp->root_extent = isonum_733 (rootp->extent);
    isomp->root_size = isonum_733 (rootp->size);
    isomp->joliet_level = 0;
	
    isomp->im_bmask = logical_block_size - 1;
    isomp->im_bshift = ffs(logical_block_size) - 1;

    pribp->b_flags |= B_AGE;
    brelse(pribp);
    pribp = NULL;
	
    mp->data = (CYG_ADDRWORD)isomp;
    isomp->im_mountp = mp;
    isomp->im_devvp = devvp;

    devvp->f_mte = mp;

    /* Check the Rock Ridge Extention support */
    if ((error = bread(isomp->im_devvp, (isomp->root_extent +
					 isonum_711(rootp->ext_attr_length)) <<
		       (isomp->im_bshift - DEV_BSHIFT),
		       isomp->logical_block_size, &bp)) != 0)
        goto out;
	
    rootp = (struct iso_directory_record *)bp->b_data;
	
    if ((isomp->rr_skip = cd9660_rrip_offset(rootp,isomp)) < 0) {
        flags  |= ISOFSMNT_NORRIP;
    } else {
        flags  &= ~ISOFSMNT_GENS;
    }
	
    /*
     * The contents are valid,
     * but they will get reread as part of another vnode, so...
     */
    bp->b_flags |= B_AGE;
    brelse(bp);
    bp = NULL;

    isomp->im_flags = flags & (ISOFSMNT_NORRIP | ISOFSMNT_GENS |
			       ISOFSMNT_EXTATT | ISOFSMNT_NOJOLIET);
    switch (isomp->im_flags & (ISOFSMNT_NORRIP | ISOFSMNT_GENS)) {
        default:
            isomp->iso_ftype = ISO_FTYPE_DEFAULT;
            break;
        case ISOFSMNT_GENS|ISOFSMNT_NORRIP:
            isomp->iso_ftype = ISO_FTYPE_9660;
            break;
        case 0:
            isomp->iso_ftype = ISO_FTYPE_RRIP;
            break;
    }

    /* Decide whether to use the Joliet descriptor */
  
    if (isomp->iso_ftype != ISO_FTYPE_RRIP && joliet_level) {
        rootp = (struct iso_directory_record *)
                sup->root_directory_record;
        bcopy(rootp, isomp->root, sizeof isomp->root);
        isomp->root_extent = isonum_733(rootp->extent);
        isomp->root_size = isonum_733(rootp->size);
        isomp->joliet_level = joliet_level;
        supbp->b_flags |= B_AGE;
    }
  
    if (supbp) {
        brelse(supbp);
        supbp = NULL;
    }
  
    return (0);
  out:
    if (bp)
        brelse(bp);
    if (supbp)
        brelse(supbp);
    if (isomp) {
        free((char *)isomp);
        mp->data = (CYG_ADDRWORD)0;
    }
    return (error);
}

int
_root(cyg_mtab_entry *mp,
      cyg_file *vp)
{
    struct iso_mnt *imp = VFSTOISOFS(mp);
    struct iso_directory_record *dp =
        (struct iso_directory_record *)imp->root;
    ino_t ino = isodirino(dp, imp);
    
    /*
     * With RRIP we must use the `.' entry of the root directory.
     * Simply tell vget, that it's a relocated directory.
     */
    return (cd9660_vget_internal(mp, ino, vp,
				 imp->iso_ftype == ISO_FTYPE_RRIP, dp));
}
