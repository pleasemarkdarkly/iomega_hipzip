#include "cd9660_dops.h"
#include "cd9660_fops.h"
#include "cd9660_internal.h"
#include "cd9660_rrip.h"
#include "cd9660_bio.h"
#include "cd9660_node.h"
#include "cd9660_support.h"
#include <cyg/error/codes.h>
#include <stdlib.h>
#include <unistd.h>
// dc- hack to get around bsd/dirent.h malformation
#define _KERNEL
#include <bsd/dirent.h>
#undef _KERNEL

/* Structure for reading directories */
struct isoreaddir {
    struct dirent saveent;
    struct dirent assocent;
    struct dirent current;
    off_t saveoff;
    off_t assocoff;
    off_t curroff;
    cyg_uio *uio;
    off_t uio_off;
    int eofflag;
};

/* Internal helper functions */
static int _uiodir(struct isoreaddir *, struct dirent *, off_t);
static int _shipdir(struct isoreaddir *);

cyg_fileops cd9660_dirops =
{
    cd9660_do_read,
    (cyg_fileop_write *)cyg_fileio_erofs,
    cd9660_do_lseek,
    cd9660_fo_ioctl,
    cyg_fileio_seltrue,
    (cyg_fileop_fsync *)cyg_fileio_enosys,
    cd9660_fo_close,
    cd9660_fo_fstat,
    cd9660_fo_getinfo,
    cd9660_fo_setinfo
};

int
cd9660_do_read(cyg_file * vdp, cyg_uio * uio)
{
    struct isoreaddir *idp;
    struct iso_node *dp;
    struct iso_mnt *imp;
    struct buf *bp = NULL;
    struct iso_directory_record *ep;
    int entryoffsetinblock;
    doff_t endsearch;
    unsigned long bmask;
    int error = 0;
    int reclen;
    unsigned short namelen;

    dp = VTOI(vdp);
    imp = dp->i_mnt;
    bmask = imp->im_bmask;

    idp = (struct isoreaddir *)malloc(sizeof(*idp));
#if 1
    idp->saveent.d_namlen = idp->assocent.d_namlen = 0;
#endif
    /*
     * XXX
     * Is it worth trying to figure out the type?
     */
#if 1
    idp->saveent.d_type = idp->assocent.d_type = idp->current.d_type =
	DT_UNKNOWN;
#endif
    idp->uio = uio;
    idp->eofflag = 1;
    uio->uio_offset = vdp->f_offset;
    idp->curroff = uio->uio_offset;
    
    if ((entryoffsetinblock = idp->curroff & bmask) &&
	(error = cd9660_blkatoff(vdp, (off_t)idp->curroff, NULL, &bp))) {
	free(idp);
	return (error);
    }
    endsearch = dp->i_size;
    
    while (idp->curroff < endsearch) {
	/*
	 * If offset is on a block boundary,
	 * read the next directory block.
	 * Release previous if it exists.
	 */
	if ((idp->curroff & bmask) == 0) {
	    if (bp != NULL)
		brelse(bp);
	    error = cd9660_blkatoff(vdp, (off_t)idp->curroff,
				    NULL, &bp);
	    if (error)
		break;
	    entryoffsetinblock = 0;
	}
	/*
	 * Get pointer to next entry.
	 */
	ep = (struct iso_directory_record *)
	    ((char *)bp->b_data + entryoffsetinblock);
	
	reclen = isonum_711(ep->length);
	if (reclen == 0) {
	    /* skip to next block, if any */
	    idp->curroff =
		(idp->curroff & ~bmask) + imp->logical_block_size;
	    continue;
	}
	
	if (reclen < ISO_DIRECTORY_RECORD_SIZE) {
	    error = EINVAL;
	    /* illegal entry, stop */
	    break;
	}
	
	if (entryoffsetinblock + reclen > imp->logical_block_size) {
	    error = EINVAL;
	    /* illegal directory, so stop looking */
	    break;
	}
	
	idp->current.d_namlen = isonum_711(ep->name_len);
	
	if (reclen < ISO_DIRECTORY_RECORD_SIZE + idp->current.d_namlen) {
	    error = EINVAL;
	    /* illegal entry, stop */
	    break;
	}
	
	if (isonum_711(ep->flags)&2)
	    idp->current.d_fileno = isodirino(ep, imp);
	else
	    idp->current.d_fileno = dbtob(bp->b_blkno) +
		entryoffsetinblock;
	
	idp->curroff += reclen;
	
	switch (imp->iso_ftype) {
	    case ISO_FTYPE_RRIP:
		cd9660_rrip_getname(ep,idp->current.d_name, &namelen,
				    &idp->current.d_fileno,imp);
		idp->current.d_namlen = (u_char)namelen;
		if (idp->current.d_namlen)
		    error = _uiodir(idp,&idp->current,idp->curroff);
		break;
	    default:	/* ISO_FTYPE_DEFAULT || ISO_FTYPE_9660 */
		strcpy(idp->current.d_name,"..");
		if (idp->current.d_namlen == 1 && ep->name[0] == 0) {
		    idp->current.d_namlen = 1;
		    error = _uiodir(idp,&idp->current,idp->curroff);
		} else if (idp->current.d_namlen == 1 && 
			   ep->name[0] == 1) {
		    idp->current.d_namlen = 2;
		    error = _uiodir(idp,&idp->current,idp->curroff);
		} else {
		    isofntrans(ep->name,idp->current.d_namlen,
			       idp->current.d_name, &namelen,
			       imp->iso_ftype == ISO_FTYPE_9660,
			       isonum_711(ep->flags) & 4,
			       imp->joliet_level);
		    idp->current.d_namlen = (u_char)namelen;
		    if (imp->iso_ftype == ISO_FTYPE_DEFAULT)
			error = _shipdir(idp);
		    else
			error = _uiodir(idp,&idp->current,idp->curroff);
		}
	}
	if (error)
	    break;

	entryoffsetinblock += reclen;
	break;
    }
	
    if (!error && imp->iso_ftype == ISO_FTYPE_DEFAULT) {
	idp->current.d_namlen = 0;
	error = _shipdir(idp);
    }
    if (error < 0)
	error = 0;

    if (bp)
	brelse (bp);

    uio->uio_offset = idp->uio_off;
	
    vdp->f_offset = uio->uio_offset;

    free(idp);

    return (error);
}

int
cd9660_do_lseek(cyg_file * fp, off_t * pos, int whence)
{
    // Only allow SEEK_SET to zero
    
    if (whence != SEEK_SET || *pos != 0)
	return EINVAL;
    
    *pos = fp->f_offset = 0;
    
    return ENOERR;
}

int
_uiodir(struct isoreaddir *idp,
	   struct dirent *dp,
	   off_t off)
{
    int error;

    dp->d_name[dp->d_namlen] = 0;
    dp->d_reclen = DIRENT_SIZE(dp);
	
    if (idp->uio->uio_resid < dp->d_reclen) {
	idp->eofflag = 0;
	return (-1);
    }

    if ((error = cd9660_uiomove((caddr_t)dp, dp->d_reclen, idp->uio)) != 0)
	return (error);
    idp->uio_off = off;
    return (0);
}

int
_shipdir(struct isoreaddir *idp)
{
    struct dirent *dp;
    int cl, sl, assoc;
    int error;
    char *cname, *sname;

    cl = idp->current.d_namlen;
    cname = idp->current.d_name;

    if ((assoc = cl > 1 && *cname == ASSOCCHAR)) {
	cl--;
	cname++;
    }

    dp = &idp->saveent;
    sname = dp->d_name;
    if (!(sl = dp->d_namlen)) {
	dp = &idp->assocent;
	sname = dp->d_name + 1;
	sl = dp->d_namlen - 1;
    }
    if (sl > 0) {
	if (sl != cl
	    || bcmp(sname,cname,sl)) {
	    if (idp->assocent.d_namlen) {
		error = _uiodir(idp, &idp->assocent,
				   idp->assocoff);
		if (error)
		    return (error);
		idp->assocent.d_namlen = 0;
	    }
	    if (idp->saveent.d_namlen) {
		error = _uiodir(idp, &idp->saveent,
				   idp->saveoff);
		if (error)
		    return (error);
		idp->saveent.d_namlen = 0;
	    }
	}
    }
    idp->current.d_reclen = DIRENT_SIZE(&idp->current);
    if (assoc) {
	idp->assocoff = idp->curroff;
	bcopy(&idp->current,&idp->assocent,idp->current.d_reclen);
    } else {
	idp->saveoff = idp->curroff;
	bcopy(&idp->current,&idp->saveent,idp->current.d_reclen);
    }
    return (0);
}
