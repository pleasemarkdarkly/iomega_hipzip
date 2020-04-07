#include "cd9660_fops.h"
#include "cd9660_internal.h"
#include "cd9660_node.h"
#include <cyg/error/codes.h>
#include <unistd.h>

cyg_fileops cd9660_fileops = 
{
	cd9660_fo_read,
	(cyg_fileop_write *)cyg_fileio_erofs,
	cd9660_fo_lseek,
	cd9660_fo_ioctl,
	cyg_fileio_seltrue,
	(cyg_fileop_fsync *)cyg_fileio_enoerr,
	cd9660_fo_close,
	cd9660_fo_fstat,
	cd9660_fo_getinfo,
	cd9660_fo_setinfo
};

//#define ALLOW_MULTIBLOCK_READS

int
cd9660_fo_read(cyg_file * vp, cyg_uio * uio)
{
    struct iso_node *ip = VTOI(vp);
    struct iso_mnt *imp;
    struct buf *bp;
    cyg_int32 lbn;
    off_t diff;
    int error = 0;
    long size, n, on;
    
    uio->uio_offset = vp->f_offset;
    
    if (uio->uio_resid == 0)
        return (0);
    if (uio->uio_offset < 0)
        return (EINVAL);
    ip->i_flag |= IN_ACCESS;
    imp = ip->i_mnt;

#ifdef ALLOW_MULTIBLOCK_READS
    // this read technique tries to as many sectors as possible
    lbn = lblkno(imp, uio->uio_offset);
    on  = blkoff(imp, uio->uio_offset);
    
    // total bytes to read
    n   = uio->uio_resid;
    
    // dont overread from file
    diff = (off_t)ip->i_size - uio->uio_offset;
    if( diff <= 0 ) return 0;
    if( diff <  n ) n = diff;
    
    // bytes to read from device on our behalf
    size = (n + on + imp->logical_block_size - 1) & ~(imp->logical_block_size-1);
    
    //    if(size >= (64*1024)) {
    //        size -= imp->logical_block_size;
    //        n -= imp->logical_block_size;
    //    }
    
    // issue read call
    error = bread( vp,lbn,size,&bp );
    
    if( error ) {
        brelse(bp);
        return error;
    }
    error = cd9660_uiomove( bp->b_data + on, (int) n, uio );
    
    // todo verify this
    if (n + on == imp->logical_block_size ||
        uio->uio_offset == (off_t)ip->i_size)
        bp->b_flags |= B_AGE;
    brelse(bp);
#else
    // this read technique only reads single sectors from the cd drive
    do {
        lbn = lblkno(imp, uio->uio_offset);
        on = blkoff(imp, uio->uio_offset);
        n = min((unsigned int)(imp->logical_block_size - on),
            uio->uio_resid);
        diff = (off_t)ip->i_size - uio->uio_offset;
        if (diff <= 0)
            return (0);
        if (diff < n)
            n = diff;
        size = blksize(imp, ip, lbn);
        error = bread(vp, lbn, size, &bp);
        n = min(n, size - bp->b_resid);
        if (error) {
            brelse(bp);
            return (error);
        }
	
        error = cd9660_uiomove(bp->b_data + on, (int)n, uio);
	
        if (n + on == imp->logical_block_size ||
            uio->uio_offset == (off_t)ip->i_size)
            bp->b_flags |= B_AGE;
        brelse(bp);
    } while (error == 0 && uio->uio_resid > 0 && n != 0);
#endif
    vp->f_offset = uio->uio_offset;
    return (error);
}

int
cd9660_fo_lseek(cyg_file * vp, off_t * apos, int whence)
{
    struct iso_node * ip = (struct iso_node *)vp->f_data;
    off_t pos = *apos;

    switch (whence)
    {
	case SEEK_SET:
	    // Pos is already where we want to be.
	    break;
	    
	case SEEK_CUR:
	    // Add pos to current offset.
	    pos += vp->f_offset;
	    break;
	    
	case SEEK_END:
	    // Add pos to file size.
	    pos += ip->i_size;
	    break;
	    
	default:
	    return EINVAL;
    }
    
    // Check that pos is still within current file size, or at the
    // very end.
    if (pos < 0 || pos > ip->i_size)
        return EINVAL;

    // All OK, set fp offset and return new position.
    *apos = vp->f_offset = pos;
    
    return ENOERR;
}

int
cd9660_fo_ioctl(cyg_file * fp, CYG_ADDRWORD cmd, CYG_ADDRWORD data)
{
    switch (cmd) {
	default:
	    return EINVAL;
    }
    return EINVAL;
}

int
cd9660_fo_close(cyg_file * vp)
{
    vrele(vp);
    return 0;
}

int
cd9660_fo_fstat(cyg_file * vp, struct stat * sbp)
{
    return cd9660_stat_internal(vp, sbp);
}

int
cd9660_fo_getinfo(cyg_file * vp, int key, void * buf, int len)
{
    int error;
	
    switch (key) {
	case FS_INFO_CONF:
	    error = cd9660_pathconf(vp, (struct cyg_pathconf_info *)buf);
	    break;
	    
	default:
	    error = EINVAL;
	    break;
    }
    
    return (error);
}

int
cd9660_fo_setinfo(cyg_file * fp, int key, void * buf, int len)
{
    return EINVAL;
}
