// msdosfs_fops.c: file operations for msdosfs
// danc@fullplaymedia.com

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <bsd/dirent.h>
#include <stdio.h>   // printf

#include "ecos_support.h"
#include "bpb.h"
#include "direntry.h"
#include "denode.h"
#include "msdosfsmount.h"
#include "fat.h"

#include "msdosfs_fops.h"

#define	DOS_FILESIZE_MAX	0xffffffff

// type appropriate min routine that doesn't do multiple evaluation
#define min(a,b,type) ({ type x=(a),y=(b); x<y ? x : y; })

cyg_fileops fsfat_fileops =
{
    msdosfs_fo_read,
    msdosfs_fo_write,
    msdosfs_fo_lseek,
    msdosfs_fo_ioctl,
    cyg_fileio_seltrue,
    msdosfs_fo_fsync,
    msdosfs_fo_close,
    msdosfs_fo_fstat,
    msdosfs_fo_getinfo,
    msdosfs_fo_setinfo
};

cyg_fileops fsfat_dirops =
{
    msdosfs_fo_read,
    (cyg_fileop_write*)cyg_fileio_enosys,
    msdosfs_fo_dirlseek,
    (cyg_fileop_ioctl *)cyg_fileio_enosys,
    cyg_fileio_seltrue,
    (cyg_fileop_fsync *)cyg_fileio_enosys,
    msdosfs_fo_close,
    (cyg_fileop_fstat *)cyg_fileio_enosys,
    (cyg_fileop_getinfo *)cyg_fileio_enosys,
    (cyg_fileop_setinfo *)cyg_fileio_enosys
};

int
msdosfs_fo_read(struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio)
{
	int error = 0;
	int blsize;
	int isadir;
	int orig_resid;
	u_int n;
	u_long diff;
	u_long on;
	daddr_t lbn;
	daddr_t rablock;
	int rasize;
	int seqcount; // TODO figure this out
	struct buf *bp;
	cyg_file *vp = fp;
	struct denode *dep = VTODE(vp);
	struct msdosfsmount *pmp = dep->de_pmp;

	if (uio->uio_offset < 0)
		return (EINVAL);

	if (uio->uio_offset > DOS_FILESIZE_MAX)
                return (0);
	/*
	 * If they didn't ask for any data, then we are done.
	 */
	orig_resid = uio->uio_resid;
	if (orig_resid <= 0)
		return (0);

    //	seqcount = ap->a_ioflag >> 16;

	isadir = dep->de_Attributes & ATTR_DIRECTORY;
	do {
		if (uio->uio_offset >= dep->de_FileSize)
			break;
		lbn = de_cluster(pmp, uio->uio_offset);
		/*
		 * If we are operating on a directory file then be sure to
		 * do i/o with the vnode for the filesystem instead of the
		 * vnode for the directory.
		 */
		if (isadir) {
			/* convert cluster # to block # */
			error = pcbmap(dep, lbn, &lbn, 0, &blsize);
			if (error == EFBIG) {
				error = EINVAL;
				break;
			} else if (error)
				break;
			error = bread(pmp->pm_devvp, lbn, blsize, &bp);
		} else {
			blsize = pmp->pm_bpcluster;
			rablock = lbn + 1;
			if (seqcount > 1 &&
			    de_cn2off(pmp, rablock) < dep->de_FileSize) {
				rasize = pmp->pm_bpcluster;
				error = breadn(vp, lbn, blsize,
				    &rablock, &rasize, 1, &bp); 
			} else {
				error = bread(vp, lbn, blsize, &bp);
			}
		}
		if (error) {
			brelse(bp);
			break;
		}
		on = uio->uio_offset & pmp->pm_crbomask;
		diff = pmp->pm_bpcluster - on;
		n = diff > uio->uio_resid ? uio->uio_resid : diff;
		diff = dep->de_FileSize - uio->uio_offset;
		if (diff < n)
			n = diff;
		diff = blsize - bp->b_resid;
		if (diff < n)
			n = diff;
		error = uiomove(bp->b_data + on, (int) n, uio);
		brelse(bp);
	} while (error == 0 && uio->uio_resid > 0 && n != 0);
    
	if (!isadir && (error == 0 || uio->uio_resid != orig_resid) &&
	    ( VFSTOMSDOSFS(vp->f_mte)->mnt_flag & MNT_NOATIME) == 0)
		dep->de_flag |= DE_ACCESS;
	return (error);
}

int
msdosfs_fo_write(struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio)
{
	int n;
	int croffset;
	int resid;
	u_long osize;
	int error = 0;
	u_long count;
	daddr_t bn, lastcn;
	struct buf *bp;
    //	int ioflag = ap->a_ioflag;
    //	struct uio *uio = ap->a_uio;
    //	struct thread *td = uio->uio_td;
    cyg_file* vp = fp;
    //	struct vnode *vp = ap->a_vp;
    //	struct vnode *thisvp;
    cyg_file* thisvp;
	struct denode *dep = VTODE(vp);
	struct msdosfsmount *pmp = dep->de_pmp;
    //	struct ucred *cred = ap->a_cred;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_write(vp %p, uio %p, ioflag %x\n",
	    vp, uio, fp->f_flag);
	printf("msdosfs_write(): diroff %lu, dirclust %lu, startcluster %lu\n",
	    dep->de_diroffset, dep->de_dirclust, dep->de_StartCluster);
#endif

	switch (vp->f_type) {
	case CYG_FILE_TYPE_REG:
		if (vp->f_flag & CYG_FAPPEND)
			uio->uio_offset = dep->de_FileSize;
		thisvp = vp;
		break;
	case CYG_FILE_TYPE_DIR:
		return EISDIR;
	default:
		panic("msdosfs_write(): bad file type");
	}

	if (uio->uio_offset < 0)
		return (EFBIG);

	if (uio->uio_resid == 0)
		return (0);

	/*
	 * If they've exceeded their filesize limit, tell them about it.
	 */
#if 0
	if (td &&
	    (uio->uio_offset + uio->uio_resid >
	    td->td_proc->p_rlimit[RLIMIT_FSIZE].rlim_cur)) {
        //		PROC_LOCK(td->td_proc);
        //		psignal(td->td_proc, SIGXFSZ);
        //		PROC_UNLOCK(td->td_proc);
		return (EFBIG);
	}
#endif
    
	if (uio->uio_offset + uio->uio_resid > DOS_FILESIZE_MAX)
                return (EFBIG);

	/*
	 * If the offset we are starting the write at is beyond the end of
	 * the file, then they've done a seek.  Unix filesystems allow
	 * files with holes in them, DOS doesn't so we must fill the hole
	 * with zeroed blocks.
	 */
#if 0 // TODO FIX
	if (uio->uio_offset > dep->de_FileSize) {
		error = deextend(dep, uio->uio_offset, cred);
		if (error)
			return (error);
	}
#endif
	/*
	 * Remember some values in case the write fails.
	 */
	resid = uio->uio_resid;
	osize = dep->de_FileSize;

	/*
	 * If we write beyond the end of the file, extend it to its ultimate
	 * size ahead of the time to hopefully get a contiguous area.
	 */
	if (uio->uio_offset + resid > osize) {
		count = de_clcount(pmp, uio->uio_offset + resid) -
			de_clcount(pmp, osize);
		error = extendfile(dep, count, NULL, NULL, 0);
		if (error &&  (error != ENOSPC /*|| (ioflag & IO_UNIT)*/))
			goto errexit;
		lastcn = dep->de_fc[FC_LASTFC].fc_frcn;
	} else
		lastcn = de_clcount(pmp, osize) - 1;

	do {
		if (de_cluster(pmp, uio->uio_offset) > lastcn) {
			error = ENOSPC;
			break;
		}

		croffset = uio->uio_offset & pmp->pm_crbomask;
		n = min(uio->uio_resid, pmp->pm_bpcluster - croffset, typeof(uio->uio_resid));
		if (uio->uio_offset + n > dep->de_FileSize) {
			dep->de_FileSize = uio->uio_offset + n;
			/* The object size needs to be set before buffer is allocated */
            // todo: resolve
            //			vnode_pager_setsize(vp, dep->de_FileSize);
		}

		bn = de_cluster(pmp, uio->uio_offset);
		if ((uio->uio_offset & pmp->pm_crbomask) == 0
		    && (de_cluster(pmp, uio->uio_offset + uio->uio_resid) 
		        > de_cluster(pmp, uio->uio_offset)
			|| uio->uio_offset + uio->uio_resid >= dep->de_FileSize)) {
			/*
			 * If either the whole cluster gets written,
			 * or we write the cluster from its start beyond EOF,
			 * then no need to read data from disk.
			 */
			bp = getblk(thisvp, bn, pmp->pm_bpcluster);
			clrbuf(bp);
			/*
			 * Do the bmap now, since pcbmap needs buffers
			 * for the fat table. (see msdosfs_strategy)
			 */
			if (bp->b_blkno == bp->b_lblkno) {
				error = pcbmap(dep, bp->b_lblkno, &bp->b_blkno, 
				     0, 0);
				if (error)
					bp->b_blkno = -1;
			}
			if (bp->b_blkno == -1) {
				brelse(bp);
				if (!error)
					error = EIO;		/* XXX */
				break;
			}
		} else {
			/*
			 * The block we need to write into exists, so read it in.
			 */
			error = bread(thisvp, bn, pmp->pm_bpcluster, &bp);
			if (error) {
				brelse(bp);
				break;
			}
		}

		/*
		 * Should these vnode_pager_* functions be done on dir
		 * files?
		 */

		/*
		 * Copy the data from user space into the buf header.
		 */
		error = uiomove(bp->b_data + croffset, n, uio);
		if (error) {
			brelse(bp);
			break;
		}

		/*
		 * If they want this synchronous then write it and wait for
		 * it.  Otherwise, if on a cluster boundary write it
		 * asynchronously so we can move on to the next block
		 * without delay.  Otherwise do a delayed write because we
		 * may want to write somemore into the block later.
		 */
		if (!(vp->f_flag & CYG_FASYNC))
			(void) bwrite(bp);
		else if (n + croffset == pmp->pm_bpcluster)
			bawrite(bp);
		else
			bdwrite(bp);
		dep->de_flag |= DE_UPDATE;
	} while (error == 0 && uio->uio_resid > 0);

	/*
	 * If the write failed and they want us to, truncate the file back
	 * to the size it was before the write was attempted.
	 */
errexit:
	if (error) {
        //		if (ioflag & IO_UNIT) {
        detrunc(dep, osize, /*ioflag & IO_SYNC*/ !(vp->f_flag & CYG_FASYNC), NULL);
			uio->uio_offset -= resid - uio->uio_resid;
			uio->uio_resid = resid;
            //		} else {
            //			detrunc(dep, dep->de_FileSize, ioflag & IO_SYNC, NOCRED, NULL);
            //			if (uio->uio_resid != resid)
            //				error = 0;
            //		}
	} else if (!(vp->f_flag & CYG_FSYNC))
		error = deupdat(dep, 1);
	return (error);
}

// TODO implement
int
msdosfs_fo_lseek   (struct CYG_FILE_TAG* fp, off_t* pos, int whence )
{
    return -ENOSYS;
}

int
msdosfs_fo_ioctl   (struct CYG_FILE_TAG* fp, CYG_ADDRWORD com, CYG_ADDRWORD data)
{
    return ENOSYS;
}


// TODO fix
int
msdosfs_fo_fsync(struct CYG_FILE_TAG* fp, int mode)
{
#if 0
	cyg_file *vp = fp;
	int s;
	struct buf *bp, *nbp;

	/*
	 * Flush all dirty buffers associated with a vnode.
	 */
loop:
	s = splbio();
	for (bp = TAILQ_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = TAILQ_NEXT(bp, b_vnbufs);
        //		if (BUF_LOCK(bp, LK_EXCLUSIVE | LK_NOWAIT))
        //			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("msdosfs_fsync: not dirty");
		bremfree(bp);
		splx(s);
		(void) bwrite(bp);
		goto loop;
	}
	while (vp->v_numoutput) {
		vp->v_flag |= VBWAIT;
		(void) tsleep((caddr_t)&vp->v_numoutput, PRIBIO + 1, "msdosfsn", 0);
	}
#ifdef DIAGNOSTIC
	if (!TAILQ_EMPTY(&vp->v_dirtyblkhd)) {
		vprint("msdosfs_fsync: dirty", vp);
		goto loop;
	}
#endif
	splx(s);
	return (deupdat(VTODE(vp), ap->a_waitfor == MNT_WAIT));
#endif
    return -ENOSYS;
}


// TODO fix
int
msdosfs_fo_close(struct CYG_FILE_TAG* fp)
{
#if 0
	struct vnode *vp = ap->a_vp;
	struct denode *dep = VTODE(vp);
	struct timespec ts;

	mtx_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		getnanotime(&ts);
		DETIMES(dep, &ts, &ts, &ts);
	}
	mtx_unlock(&vp->v_interlock);
#endif
	return 0;
}

// TODO implement fstat
int
msdosfs_fo_fstat   (struct CYG_FILE_TAG* fp, struct stat* buf )
{
    return ENOSYS;
}

// TODO implement file attributes through getinfo/setinfo interface ?
int
msdosfs_fo_getinfo (struct CYG_FILE_TAG* fp, int key, void* buf, int len )
{
    return ENOSYS;

}

int
msdosfs_fo_setinfo (struct CYG_FILE_TAG* fp, int key, void* buf, int len )
{
    return ENOSYS;

}

int
msdosfs_fo_dirread (struct CYG_FILE_TAG* fp, struct CYG_UIO_TAG* uio)
{
    return ENOSYS;

}

int
msdosfs_fo_dirlseek(struct CYG_FILE_TAG* fp, off_t* pos, int whence )
{
    return ENOSYS;

}

