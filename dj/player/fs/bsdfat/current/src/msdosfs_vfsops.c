/* $FreeBSD: src/sys/fs/msdosfs/msdosfs_vfsops.c,v 1.85 2001/11/28 18:29:16 jhb Exp $ */
/*	$NetBSD: msdosfs_vfsops.c,v 1.51 1997/11/17 15:36:58 ws Exp $	*/

/*-
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/stat.h> 				/* defines ALLPERMS */
#include <stdio.h>
#include <stdlib.h>

#include "ecos_support.h"
#include "bpb.h"
#include "bootsect.h"
#include "direntry.h"
#include "denode.h"
#include "msdosfsmount.h"
#include "fat.h"

#define MSDOSFS_DFLTBSIZE       4096

#if 1 /*def PC98*/
/*
 * XXX - The boot signature formatted by NEC PC-98 DOS looks like a
 *       garbage or a random value :-{
 *       If you want to use that broken-signatured media, define the
 *       following symbol even though PC/AT.
 *       (ex. mount PC-98 DOS formatted FD on PC/AT)
 */
#define	MSDOSFS_NOCHECKSIG
#endif

// Exported routines used by msdosfs_vnops.c
int	msdosfs_mount __P(( cyg_fstab_entry*, cyg_mtab_entry * ));
int	msdosfs_unmount __P((cyg_mtab_entry * /*, int , struct thread * */));

static int	update_mp __P((cyg_mtab_entry *mp, struct msdosfs_args *argp));
static int	mountmsdosfs __P((cyg_file *devvp, cyg_mtab_entry *mp,
                              /*struct thread *td, */struct msdosfs_args *argp));
//static int	msdosfs_fhtovp __P((struct mount *, struct fid *,
//				    struct vnode **));
static int	msdosfs_root __P((cyg_mtab_entry *, struct denode **));
//static int	msdosfs_statfs __P((struct mount *, struct statfs * /*,
//                                                                  struct thread * */));
//static int	msdosfs_sync __P((struct mount *, int, struct ucred * /*,
//                                                                    struct thread * */));
//static int	msdosfs_vptofh __P((struct vnode *, struct fid *));

static int
update_mp(mp, argp)
	/*struct mount*/ cyg_mtab_entry* mp;
	struct msdosfs_args *argp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	int error;

    if( argp ) {
        pmp->pm_gid = argp->gid;
        pmp->pm_uid = argp->uid;
        pmp->pm_mask = argp->mask & ALLPERMS;
        pmp->pm_flags |= argp->flags & MSDOSFSMNT_MNTOPT;
        if (pmp->pm_flags & MSDOSFSMNT_U2WTABLE) {
            bcopy(argp->u2w, pmp->pm_u2w, sizeof(pmp->pm_u2w));
            bcopy(argp->d2u, pmp->pm_d2u, sizeof(pmp->pm_d2u));
            bcopy(argp->u2d, pmp->pm_u2d, sizeof(pmp->pm_u2d));
        }
        if (pmp->pm_flags & MSDOSFSMNT_ULTABLE) {
            bcopy(argp->ul, pmp->pm_ul, sizeof(pmp->pm_ul));
            bcopy(argp->lu, pmp->pm_lu, sizeof(pmp->pm_lu));
        }
    } else {
        pmp->pm_gid  = 0;
        pmp->pm_uid  = 0;
        pmp->pm_mask = 0; // TODO fix these perms
        pmp->pm_flags= 0; // no force flags
    }

    // always get the root directory here
#if 1
    struct denode* rootvp;
    if( (error = msdosfs_root(mp, &rootvp)) != 0 )
        return error;
    mp->root = (cyg_dir)rootvp;
#endif
    
	if (pmp->pm_flags & MSDOSFSMNT_NOWIN95)
		pmp->pm_flags |= MSDOSFSMNT_SHORTNAME;
	else if (!(pmp->pm_flags &
	    (MSDOSFSMNT_SHORTNAME | MSDOSFSMNT_LONGNAME))) {

		/*
		 * Try to divine whether to support Win'95 long filenames
		 */
		if (FAT32(pmp))
			pmp->pm_flags |= MSDOSFSMNT_LONGNAME;
		else {
#if 0
            struct denode* rootvp;
            if( (error = msdosfs_root(mp, &rootvp)) != 0 )
                return error;
            mp->root = (cyg_dir)rootvp;
#endif
			pmp->pm_flags |= findwin95(rootvp) /*VTODE(rootvp))*/
				? MSDOSFSMNT_LONGNAME
					: MSDOSFSMNT_SHORTNAME;
            //			vput(rootvp);
		}
	}
	return 0;
}

/*
 * mp - path - addr in user space of mount point (ie /usr or whatever)
 * data - addr in user space of mount params including the name of the block
 * special file to treat as a filesystem.
 */
int
msdosfs_mount( cyg_fstab_entry* fsp, cyg_mtab_entry* mp )
{
    //	struct vnode *devvp;	  /* vnode for blk device to mount */
    cyg_file* devvp;
	/* msdosfs specific mount control block */
	struct msdosfsmount *pmp = NULL;
    //	size_t size;
	int error=0, flags, devfd;
    //	mode_t accessmode;

    // make sure we are initialized
    msdosfs_init();
    
	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
    pmp = VFSTOMSDOSFS(mp);
	if (pmp && (pmp->mnt_flag & MNT_UPDATE) ) {
		error = 0;
		if (!(pmp->pm_flags & MSDOSFSMNT_RONLY) && (pmp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (pmp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
            //			error = vflush(mp, 0, flags);
		}
		if (!error && (pmp->mnt_flag & MNT_RELOAD))
			/* not yet implemented */
			error = EOPNOTSUPP;
		if (error)
			return (error);
		if ((pmp->pm_flags & MSDOSFSMNT_RONLY) && (pmp->mnt_kern_flag & MNTK_WANTRDWR)) {
			pmp->pm_flags &= ~MSDOSFSMNT_RONLY;
		}
#ifdef	__notyet__	/* doesn't work correctly with current mountd	XXX */
		if (args.fspec == 0) {
			if (args.flags & MSDOSFSMNT_MNTOPT) {
				pmp->pm_flags &= ~MSDOSFSMNT_MNTOPT;
				pmp->pm_flags |= args.flags & MSDOSFSMNT_MNTOPT;
				if (pmp->pm_flags & MSDOSFSMNT_NOWIN95)
					pmp->pm_flags |= MSDOSFSMNT_SHORTNAME;
			}
//#endif
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &args.export));
		}
#endif
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
    //	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec);
    //	error = namei(ndp);
    //	if (error)
    //		return (error);

    // Open the device and assign the handle (cyg_file*)to devvp
    devfd = open( mp->devname, O_RDWR );
    if( devfd == -1 ) {
        // TODO failed to open block device
        return -1;
    }
    devvp = cyg_fp_get( devfd );

    // TODO should check devvp to verify block device type
    //	NDFREE(ndp, NDF_ONLY_PNBUF);

    //	if (!vn_isdisk(devvp, &error)) {
    //		vrele(devvp);
    //		return (error);
    //	}

	if( !pmp || (pmp->mnt_flag & MNT_UPDATE) == 0) {
		error = mountmsdosfs(devvp, mp, 0 /*&args */);  // TODO fixme
#ifdef MSDOSFS_DEBUG		/* only needed for the printf below */
		pmp = VFSTOMSDOSFS(mp);
#endif
	} else {
		if (devvp != pmp->pm_devvp)
			error = EINVAL;	/* XXX needs translation */
        //		else
        //			vrele(devvp);
	}
	if (error) {
        //		vrele(devvp);
		return (error);
	}

	error = update_mp(mp, 0 /*&args */); // TODO fixme
	if (error) {
		msdosfs_unmount(mp /*, MNT_FORCE */);
		return error;
	}
    //	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
    //	    &size);
    //	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
    //	(void) msdosfs_statfs(mp, &mp->mnt_stat, td);
#ifdef MSDOSFS_DEBUG
	printf("msdosfs_mount(): mp %p, pmp %p, inusemap %p\n", mp, pmp, pmp->pm_inusemap);
#endif
	return (0);
}

static int
mountmsdosfs(devvp, mp, /*td, */argp)
	cyg_file *devvp;
	cyg_mtab_entry *mp;
    /*	struct thread *td; */
	struct msdosfs_args *argp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	struct buf *bp;
    //	dev_t dev = devvp->v_rdev;
	union bootsector *bsp;
	struct byte_bpb33 *b33;
	struct byte_bpb50 *b50;
	struct byte_bpb710 *b710;
	u_int8_t SecPerClust;
	u_long clusters;
	int	ronly, error;

    // eCos should disallow multiple mounts for us through the mte setup
	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
    //	error = vfs_mountedon(devvp);
    //	if (error)
    //		return (error);
    //	if (vcount(devvp) > 1 && devvp != rootvp)
    //		return (EBUSY);
    //	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
    //	error = vinvalbuf(devvp, V_SAVE, td->td_proc->p_ucred, td, 0, 0);
    //	VOP_UNLOCK(devvp, 0, td);
    //	if (error)
    //		return (error);

    // note: implicit is that read only mounting of the filesystem is not supported
	ronly = pmp && ((pmp->mnt_flag & MNT_RDONLY) != 0);
    //	vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, td);
    //	error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, td);
    //	VOP_UNLOCK(devvp, 0, td);
    //	if (error)
    //		return (error);
	bp  = NULL; /* both used in error_exit */
	pmp = NULL;

	/*
	 * Read the boot sector of the filesystem, and then check the
	 * boot signature.  If not a dos boot sector then error out.
	 *
	 * NOTE: 2048 is a maximum sector size in current...
	 */
	error = bread(devvp, 0, 2048, &bp);
	if (error)
		goto error_exit;
    bp->b_flags |= B_AGE;
	bsp = (union bootsector *)bp->b_data;
	b33 = (struct byte_bpb33 *)bsp->bs33.bsBPB;
	b50 = (struct byte_bpb50 *)bsp->bs50.bsBPB;
	b710 = (struct byte_bpb710 *)bsp->bs710.bsPBP;

#ifndef MSDOSFS_NOCHECKSIG
	if (bsp->bs50.bsBootSectSig0 != BOOTSIG0
	    || bsp->bs50.bsBootSectSig1 != BOOTSIG1) {
		error = EINVAL;
		goto error_exit;
	}
#endif

	pmp = malloc(sizeof (*pmp)); //, M_MSDOSFSMNT, M_WAITOK | M_ZERO);
    memset( (void*) pmp, 0, sizeof(*pmp) );
    
	pmp->pm_mountp = mp;

	/*
	 * Compute several useful quantities from the bpb in the
	 * bootsector.  Copy in the dos 5 variant of the bpb then fix up
	 * the fields that are different between dos 5 and dos 3.3.
	 */
	SecPerClust = b50->bpbSecPerClust;
	pmp->pm_BytesPerSec = getushort(b50->bpbBytesPerSec);
	pmp->pm_ResSectors = getushort(b50->bpbResSectors);
	pmp->pm_FATs = b50->bpbFATs;
	pmp->pm_RootDirEnts = getushort(b50->bpbRootDirEnts);
	pmp->pm_Sectors = getushort(b50->bpbSectors);
	pmp->pm_FATsecs = getushort(b50->bpbFATsecs);
	pmp->pm_SecPerTrack = getushort(b50->bpbSecPerTrack);
	pmp->pm_Heads = getushort(b50->bpbHeads);
	pmp->pm_Media = b50->bpbMedia;

	/* calculate the ratio of sector size to DEV_BSIZE */
	pmp->pm_BlkPerSec = pmp->pm_BytesPerSec / DEV_BSIZE;

	/* XXX - We should probably check more values here */
	if (!pmp->pm_BytesPerSec || !SecPerClust
		|| !pmp->pm_Heads || pmp->pm_Heads > 255
#ifdef PC98
    	|| !pmp->pm_SecPerTrack || pmp->pm_SecPerTrack > 255) {
#else
		|| !pmp->pm_SecPerTrack || pmp->pm_SecPerTrack > 63) {
#endif
		error = EINVAL;
		goto error_exit;
	}

	if (pmp->pm_Sectors == 0) {
		pmp->pm_HiddenSects = getulong(b50->bpbHiddenSecs);
		pmp->pm_HugeSectors = getulong(b50->bpbHugeSectors);
	} else {
		pmp->pm_HiddenSects = getushort(b33->bpbHiddenSecs);
		pmp->pm_HugeSectors = pmp->pm_Sectors;
	}
	if (pmp->pm_HugeSectors > 0xffffffff / 
	    (pmp->pm_BytesPerSec / sizeof(struct direntry)) + 1) {
		/*
		 * We cannot deal currently with this size of disk
		 * due to fileid limitations (see msdosfs_getattr and
		 * msdosfs_readdir)
		 */
		error = EINVAL;
		printf("mountmsdosfs(): disk too big, sorry\n");
		goto error_exit;
	}

	if (pmp->pm_RootDirEnts == 0) {
		if (bsp->bs710.bsBootSectSig2 != BOOTSIG2
		    || bsp->bs710.bsBootSectSig3 != BOOTSIG3
		    || pmp->pm_Sectors
		    || pmp->pm_FATsecs
		    || getushort(b710->bpbFSVers)) {
			error = EINVAL;
			printf("mountmsdosfs(): bad FAT32 filesystem\n");
			goto error_exit;
		}
		pmp->pm_fatmask = FAT32_MASK;
		pmp->pm_fatmult = 4;
		pmp->pm_fatdiv = 1;
		pmp->pm_FATsecs = getulong(b710->bpbBigFATsecs);
		if (getushort(b710->bpbExtFlags) & FATMIRROR)
			pmp->pm_curfat = getushort(b710->bpbExtFlags) & FATNUM;
		else
			pmp->pm_flags |= MSDOSFS_FATMIRROR;
	} else
		pmp->pm_flags |= MSDOSFS_FATMIRROR;

	/*
	 * Check a few values (could do some more):
	 * - logical sector size: power of 2, >= block size
	 * - sectors per cluster: power of 2, >= 1
	 * - number of sectors:   >= 1, <= size of partition
	 */
	if ( (SecPerClust == 0)
	  || (SecPerClust & (SecPerClust - 1))
	  || (pmp->pm_BytesPerSec < DEV_BSIZE)
	  || (pmp->pm_BytesPerSec & (pmp->pm_BytesPerSec - 1))
	  || (pmp->pm_HugeSectors == 0)
	) {
		error = EINVAL;
		goto error_exit;
	}

	pmp->pm_HugeSectors *= pmp->pm_BlkPerSec;
	pmp->pm_HiddenSects *= pmp->pm_BlkPerSec; /* XXX not used? */
	pmp->pm_FATsecs     *= pmp->pm_BlkPerSec;
	SecPerClust         *= pmp->pm_BlkPerSec;

	pmp->pm_fatblk = pmp->pm_ResSectors * pmp->pm_BlkPerSec;

	if (FAT32(pmp)) {
		pmp->pm_rootdirblk = getulong(b710->bpbRootClust);
		pmp->pm_firstcluster = pmp->pm_fatblk
			+ (pmp->pm_FATs * pmp->pm_FATsecs);
		pmp->pm_fsinfo = getushort(b710->bpbFSInfo) * pmp->pm_BlkPerSec;
	} else {
		pmp->pm_rootdirblk = pmp->pm_fatblk +
			(pmp->pm_FATs * pmp->pm_FATsecs);
		pmp->pm_rootdirsize = (pmp->pm_RootDirEnts * sizeof(struct direntry)
				       + DEV_BSIZE - 1)
			/ DEV_BSIZE; /* in blocks */
		pmp->pm_firstcluster = pmp->pm_rootdirblk + pmp->pm_rootdirsize;
	}

	pmp->pm_maxcluster = (pmp->pm_HugeSectors - pmp->pm_firstcluster) /
	    SecPerClust + 1;
	pmp->pm_fatsize = pmp->pm_FATsecs * DEV_BSIZE; /* XXX not used? */

	if (pmp->pm_fatmask == 0) {
		if (pmp->pm_maxcluster
		    <= ((CLUST_RSRVD - CLUST_FIRST) & FAT12_MASK)) {
			/*
			 * This will usually be a floppy disk. This size makes
			 * sure that one fat entry will not be split across
			 * multiple blocks.
			 */
			pmp->pm_fatmask = FAT12_MASK;
			pmp->pm_fatmult = 3;
			pmp->pm_fatdiv = 2;
		} else {
			pmp->pm_fatmask = FAT16_MASK;
			pmp->pm_fatmult = 2;
			pmp->pm_fatdiv = 1;
		}
	}

	clusters = (pmp->pm_fatsize / pmp->pm_fatmult) * pmp->pm_fatdiv;
	if (pmp->pm_maxcluster >= clusters) {
		printf("Warning: number of clusters (%ld) exceeds FAT "
		    "capacity (%ld)\n", pmp->pm_maxcluster + 1, clusters);
		pmp->pm_maxcluster = clusters - 1;
	}


	if (FAT12(pmp))
		pmp->pm_fatblocksize = 3 * pmp->pm_BytesPerSec;
	else
		pmp->pm_fatblocksize = MSDOSFS_DFLTBSIZE;

	pmp->pm_fatblocksec = pmp->pm_fatblocksize / DEV_BSIZE;
	pmp->pm_bnshift = ffs(DEV_BSIZE) - 1;

	/*
	 * Compute mask and shift value for isolating cluster relative byte
	 * offsets and cluster numbers from a file offset.
	 */
	pmp->pm_bpcluster = SecPerClust * DEV_BSIZE;
	pmp->pm_crbomask = pmp->pm_bpcluster - 1;
	pmp->pm_cnshift = ffs(pmp->pm_bpcluster) - 1;

	/*
	 * Check for valid cluster size
	 * must be a power of 2
	 */
	if (pmp->pm_bpcluster ^ (1 << pmp->pm_cnshift)) {
		error = EINVAL;
		goto error_exit;
	}

	/*
	 * Release the bootsector buffer.
	 */
	brelse(bp);
	bp = NULL;

	/*
	 * Check FSInfo.
	 */
	if (pmp->pm_fsinfo) {
		struct fsinfo *fp;

		if ((error = bread(devvp, pmp->pm_fsinfo, fsi_size(pmp),
                           &bp)) != 0)
			goto error_exit;
		fp = (struct fsinfo *)bp->b_data;
		if (!bcmp(fp->fsisig1, "RRaA", 4)
		    && !bcmp(fp->fsisig2, "rrAa", 4)
		    && !bcmp(fp->fsisig3, "\0\0\125\252", 4)
		    && !bcmp(fp->fsisig4, "\0\0\125\252", 4))
			pmp->pm_nxtfree = getulong(fp->fsinxtfree);
		else
			pmp->pm_fsinfo = 0;
		brelse(bp);
		bp = NULL;
	}

	/*
	 * Check and validate (or perhaps invalidate?) the fsinfo structure?		XXX
	 */

	/*
	 * Allocate memory for the bitmap of allocated clusters, and then
	 * fill it in.
	 */
	pmp->pm_inusemap = (void*)malloc(((pmp->pm_maxcluster + N_INUSEBITS - 1)
				   / N_INUSEBITS)
				  * sizeof(*pmp->pm_inusemap)/*,
                                               M_MSDOSFSFAT, M_WAITOK*/);

	/*
	 * fillinusemap() needs pm_devvp.
	 */
	pmp->pm_devvp = devvp;
    // zero out the underlying device number
    pmp->pm_dev = 0;

	/*
	 * Have the inuse map filled in.
	 */
	if ((error = fillinusemap(pmp)) != 0)
		goto error_exit;

	/*
	 * If they want fat updates to be synchronous then let them suffer
	 * the performance degradation in exchange for the on disk copy of
	 * the fat being correct just about all the time.  I suppose this
	 * would be a good thing to turn on if the kernel is still flakey.
	 */
	if (pmp->mnt_flag & MNT_SYNCHRONOUS)
		pmp->pm_flags |= MSDOSFSMNT_WAITONFAT;

	/*
	 * Finish up.
	 */
	if (ronly)
		pmp->pm_flags |= MSDOSFSMNT_RONLY;
	else
		pmp->pm_fmod = 1;

	mp->data = (unsigned int) pmp;

    //	mp->mnt_stat.f_fsid.val[0] = dev2udev(dev);
    //	mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
	pmp->mnt_flag |= MNT_LOCAL;
    //	devvp->v_rdev->si_mountpoint = mp;

	return 0;

error_exit:
	if (bp)
		brelse(bp);
    //	(void) VOP_CLOSE(devvp, ronly ? FREAD : FREAD | FWRITE, NOCRED, td);
	if (pmp) {
		if (pmp->pm_inusemap)
			free(pmp->pm_inusemap);
		free(pmp);
		mp->data = (unsigned int)0;
	}
	return (error);
}

/*
 * Unmount the filesystem described by mp.
 */
int
msdosfs_unmount(mp /*, mntflags */ /*, td */)
	cyg_mtab_entry *mp;
    /*	int mntflags; */
    /*	struct thread *td; */
{
	struct msdosfsmount *pmp;
	int error=0, flags;
    int mntflags = MNT_FORCE;

    // TODO call msdosfs_uninit() here ?
    
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
    //	error = vflush(mp, 0, flags);
    //	if (error)
    //		return error;
	pmp = VFSTOMSDOSFS(mp);
    //	pmp->pm_devvp->v_rdev->si_mountpoint = NULL;
#ifdef MSDOSFS_DEBUG
#if 0
	{
		struct vnode *vp = pmp->pm_devvp;

		printf("msdosfs_umount(): just before calling VOP_CLOSE()\n");
		printf("flag %08lx, usecount %d, writecount %d, holdcnt %ld\n",
		    vp->v_flag, vp->v_usecount, vp->v_writecount, vp->v_holdcnt);
		printf("id %lu, mount %p, op %p\n",
		    vp->v_id, vp->v_mount, vp->v_op);
		printf("freef %p, freeb %p, mount %p\n",
		    TAILQ_NEXT(vp, v_freelist), vp->v_freelist.tqe_prev,
		    vp->v_mount);
		printf("cleanblkhd %p, dirtyblkhd %p, numoutput %ld, type %d\n",
		    TAILQ_FIRST(&vp->v_cleanblkhd),
		    TAILQ_FIRST(&vp->v_dirtyblkhd),
		    vp->v_numoutput, vp->v_type);
		printf("union %p, tag %d, data[0] %08x, data[1] %08x\n",
		    vp->v_socket, vp->v_tag,
		    ((u_int *)vp->v_data)[0],
		    ((u_int *)vp->v_data)[1]);
	}
    
	error = VOP_CLOSE(pmp->pm_devvp,
		    (pmp->pm_flags&MSDOSFSMNT_RONLY) ? FREAD : FREAD | FWRITE,
		    td);
    vrele(pmp->pm_devvp);
#endif
#endif
	free(pmp->pm_inusemap);
	free(pmp);
	mp->data = (unsigned int)0;
	pmp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

static int
msdosfs_root(mp, vpp)
	cyg_mtab_entry *mp;
	struct denode **vpp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	struct denode *ndep;
	int error;

#ifdef MSDOSFS_DEBUG
	printf("msdosfs_root(); mp %p, pmp %p\n", mp, pmp);
#endif
	error = deget(pmp, MSDOSFSROOT, MSDOSFSROOT_OFS, &ndep);
	if (error)
		return (error);
    *vpp = ndep;
    //	*vpp = DETOV(ndep); // what the fuck is this shit - convert a denode to a vnode and assign it to a denode?
	return (0);
}


#if 0
static int
msdosfs_statfs(mp, sbp /*, td */)
	cyg_mtab_entry *mp;
	struct statfs *sbp;
    /*	struct thread *td; */
{
	struct msdosfsmount *pmp;

	pmp = VFSTOMSDOSFS(mp);
	sbp->f_bsize = pmp->pm_bpcluster;
	sbp->f_iosize = pmp->pm_bpcluster;
	sbp->f_blocks = pmp->pm_maxcluster + 1;
	sbp->f_bfree = pmp->pm_freeclustercount;
	sbp->f_bavail = pmp->pm_freeclustercount;
	sbp->f_files = pmp->pm_RootDirEnts;			/* XXX */
	sbp->f_ffree = 0;	/* what to put in here? */
#if 0
	if (sbp != &mp->mnt_stat) {
		sbp->f_type = mp->mnt_vfc->vfc_typenum;
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
#endif
	strncpy(sbp->f_fstypename, mp->mnt_vfc->vfc_name, MFSNAMELEN);
    
	return (0);
}
#endif
#if 0
static int
msdosfs_sync(mp, waitfor, cred /*, td */)
	cyg_mtab_entry *mp;
	int waitfor;
	struct ucred *cred;
    /*	struct thread *td; */
{
	struct vnode *vp, *nvp;
	struct denode *dep;
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	int error, allerror = 0;

	/*
	 * If we ever switch to not updating all of the fats all the time,
	 * this would be the place to update them from the first one.
	 */
	if (pmp->pm_fmod != 0) {
		if (pmp->pm_flags & MSDOSFSMNT_RONLY)
			panic("msdosfs_sync: rofs mod");
		else {
			/* update fats here */
		}
	}
	/*
	 * Write back each (modified) denode.
	 */
    //	mtx_lock(&mntvnode_mtx);
loop:
	for (vp = TAILQ_FIRST(&mp->mnt_nvnodelist); vp != NULL; vp = nvp) {
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;
		nvp = TAILQ_NEXT(vp, v_nmntvnodes);

        //		mtx_unlock(&mntvnode_mtx);
        //		mtx_lock(&vp->v_interlock);
		dep = VTODE(vp);
		if (vp->v_type == VNON ||
		    ((dep->de_flag &
		    (DE_ACCESS | DE_CREATE | DE_UPDATE | DE_MODIFIED)) == 0 &&
		    (TAILQ_EMPTY(&vp->v_dirtyblkhd) || waitfor == MNT_LAZY))) {
            //			mtx_unlock(&vp->v_interlock);
            //			mtx_lock(&mntvnode_mtx);
			continue;
		}
        //		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK, td);
        //		if (error) {
        //			mtx_lock(&mntvnode_mtx);
        //			if (error == ENOENT)
        //				goto loop;
        //			continue;
        //		}
		error = VOP_FSYNC(vp, cred, waitfor, td);
		if (error)
			allerror = error;
        //		VOP_UNLOCK(vp, 0, td);
        //		vrele(vp);
        //		mtx_lock(&mntvnode_mtx);
	}
    //	mtx_unlock(&mntvnode_mtx);

	/*
	 * Flush filesystem control info.
	 */
	if (waitfor != MNT_LAZY) {
		vn_lock(pmp->pm_devvp, LK_EXCLUSIVE | LK_RETRY, td);
		error = VOP_FSYNC(pmp->pm_devvp, cred, waitfor, td);
		if (error)
			allerror = error;
		VOP_UNLOCK(pmp->pm_devvp, 0, td);
	}
	return (allerror);
}

static int
msdosfs_fhtovp(mp, fhp, vpp)
	cyg_mtab_entry *mp;
	struct fid *fhp;
	struct vnode **vpp;
{
	struct msdosfsmount *pmp = VFSTOMSDOSFS(mp);
	struct defid *defhp = (struct defid *) fhp;
	struct denode *dep;
	int error;

	error = deget(pmp, defhp->defid_dirclust, defhp->defid_dirofs, &dep);
	if (error) {
		*vpp = NULLVP;
		return (error);
	}
	*vpp = DETOV(dep);
	return (0);
}

static int
msdosfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	struct denode *dep;
	struct defid *defhp;

	dep = VTODE(vp);
	defhp = (struct defid *)fhp;
	defhp->defid_len = sizeof(struct defid);
	defhp->defid_dirclust = dep->de_dirclust;
	defhp->defid_dirofs = dep->de_diroffset;
	/* defhp->defid_gen = dep->de_gen; */
	return (0);
}

static struct vfsops msdosfs_vfsops = {
	msdosfs_mount,
	vfs_stdstart,
	msdosfs_unmount,
	msdosfs_root,
	vfs_stdquotactl,
	msdosfs_statfs,
	msdosfs_sync,
	vfs_stdvget,
	msdosfs_fhtovp,
	vfs_stdcheckexp,
	msdosfs_vptofh,
	msdosfs_init,
	msdosfs_uninit,
	vfs_stdextattrctl,
};

VFS_SET(msdosfs_vfsops, msdosfs, 0);
#endif
