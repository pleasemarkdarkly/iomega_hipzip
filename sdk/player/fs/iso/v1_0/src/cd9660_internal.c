#include "cd9660_internal.h"
#include "cd9660_support.h"
#include "cd9660_node.h"
#include "cd9660_fops.h"
#include "cd9660_bio.h"
#include "cd9660_rrip.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cyg/infra/diag.h>
#include <cyg/error/codes.h>

#if (NAME_MAX < 255)
#undef NAME_MAX
#define NAME_MAX 255
#endif

static unsigned int _chars2ui(unsigned char *begin, int len);

int
cd9660_bmap(cyg_file * vp, cyg_int32 bn, cyg_file ** vpp, cyg_int32 * bnp, int * runp)
{
    struct iso_node *ip = VTOI(vp);
    cyg_int32 lblkno = bn;
    int bshift;

    /*
     * Check for underlying vnode requests and ensure that logical
     * to physical mapping is requested.
     */
    if (vpp != NULL)
	*vpp = ip->i_devvp;
    if (bnp == NULL)
	return (0);

    /*
     * Compute the requested block number
     */
    bshift = ip->i_mnt->im_bshift;
    *bnp = (ip->iso_start + lblkno) << (bshift - DEV_BSHIFT);

    /*
     * Determine maximum number of readahead blocks following the
     * requested block.
     */
    if (runp) {
	int nblk;

	nblk = (ip->i_size >> bshift) - (lblkno + 1);
	if (nblk <= 0)
	    *runp = 0;
	/* TODO
	 * else if (nblk >= (MAXBSIZE >> bshift))
	 *  *runp = (MAXBSIZE >> bshift) - 1;
	 */
	else
	    *runp = nblk;
    }

    return (0);
}

static int ipcount = 0;

void inode_malloc_log(struct iso_node *ip)
{
	ipcount++;
	diag_printf("m:in @ %x - t %d\n",ip,ipcount);

}

void inode_free_log(struct iso_node *ip)
{

	ipcount--;
	diag_printf("f:in @ %x - t %d\n",ip,ipcount);

}


int
cd9660_vget_internal(cyg_mtab_entry * mp, ino_t ino, cyg_file * vp, int relocated,
		     struct iso_directory_record * isodir)
{
    struct iso_mnt *imp;
    struct iso_node *ip = NULL;
    struct buf *bp;
    int error;


  retry:
    imp = VFSTOISOFS(mp);


	if(vp->f_data != NULL)
	{
		INODEFLOG(vp->f_data);
		free(vp->f_data);
	}
		

    /* Allocate a new vnode/iso_node. */
    if ((error = getnewvnode(mp, &cd9660_fileops, vp)) != 0) {
		return (error);
    }


    ip = (struct iso_node *)malloc(sizeof(struct iso_node));
	
	INODEMLOG(ip);


	if(ip == NULL)
		return ENOMEM;
	
    bzero((char *)ip, sizeof(struct iso_node));

    vp->f_data = (CYG_ADDRWORD)ip;
    ip->i_vnode = vp;
    ip->i_number = ino;

    if (error) {
	
		vrele(vp);

		if (error == EEXIST)
			goto retry;

		return (error);
    }

    if (isodir == 0) {

		int lbn, off;

		lbn = lblkno(imp, ino);
		if (lbn >= imp->volume_space_size) {
			vput(vp);
			diag_printf("fhtovp: lbn exceed volume space %d\n", lbn);
			return (ESTALE);
		}
		
		off = blkoff(imp, ino);

		if (off + ISO_DIRECTORY_RECORD_SIZE > imp->logical_block_size) {
			vput(vp);
			diag_printf("fhtovp: crosses block boundary %d\n",
				off + ISO_DIRECTORY_RECORD_SIZE);
			return (ESTALE);
		}
		
		error = bread(imp->im_devvp,
				  lbn << (imp->im_bshift - DEV_BSHIFT),
				  imp->logical_block_size, &bp);
		
		if (error) {
			vput(vp);
			brelse(bp);
			diag_printf("fhtovp: bread error %d\n",error);
			return (error);
		}

		isodir = (struct iso_directory_record *)(bp->b_data + off);

		if (off + isonum_711(isodir->length) >
			imp->logical_block_size) {
			vput(vp);
			if (bp != 0)
			brelse(bp);
			diag_printf("fhtovp: directory crosses block boundary %d[off=%d/len=%d]\n",
				off +isonum_711(isodir->length), off,
				isonum_711(isodir->length));
			return (ESTALE);
		}
    } 
	else
		bp = 0;

    ip->i_mnt = imp;
    ip->i_devvp = imp->im_devvp;
    VREF(ip->i_devvp);

    if (relocated) {
	
		/*
		 * On relocated directories we must
		 * read the `.' entry out of a dir.
		 */
		ip->iso_start = ino >> imp->im_bshift;
		if (bp != 0)
			brelse(bp);
		
		if ((error = cd9660_blkatoff(vp, (off_t)0, NULL, &bp)) != 0) {
			vput(vp);
			return (error);
		}
		isodir = (struct iso_directory_record *)bp->b_data;
    }

    ip->iso_extent = isonum_733(isodir->extent);
    ip->i_size = isonum_733(isodir->size);
    ip->iso_start = isonum_711(isodir->ext_attr_length) + ip->iso_extent;
	
    /*
     * Setup time stamp, attribute
     */
    vp->f_type = CYG_FILE_TYPE_NON;
    switch (imp->iso_ftype) {
		default:	/* ISO_FTYPE_9660 */
		{
			struct buf *bp2;
			int off;
			if ((imp->im_flags & ISOFSMNT_EXTATT)
			&& (off = isonum_711(isodir->ext_attr_length)))
			cd9660_blkatoff(vp, (off_t)-(off << imp->im_bshift), NULL,
					&bp2);
			else
			bp2 = NULL;
			cd9660_defattr(isodir, ip, bp2);
			cd9660_deftstamp(isodir, ip, bp2);
			if (bp2)
			brelse(bp2);
			break;
		}
		case ISO_FTYPE_RRIP:
			cd9660_rrip_analyze(isodir, ip, imp);
			break;
    }

    if (bp != 0)
		brelse(bp);

    /*
     * Initialize the associated vnode
     */
    vp->f_type = IFTOVT(ip->inode.iso_mode);
	
    if (ip->iso_extent == imp->root_extent) {
	//vp->f_flag |= VROOT;
    }

    /*
     * XXX need generation number?
     */
	
    return (0);
}

ino_t
isodirino(struct iso_directory_record * isodir, struct iso_mnt * imp)
{
    ino_t ino;

    ino = (isonum_733(isodir->extent) +
	   isonum_711(isodir->ext_attr_length)) << imp->im_bshift;
    return (ino);
}


#if 0
// evil
/*
 * Last reference to an inode, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
int
cd9660_inactive(cyg_file * vp)
{
    struct iso_node *ip = VTOI(vp);
    int error = 0;

    if (vp->f_type != CYG_FILE_TYPE_BLK && ip) {
	// ERROR LEAK FIXME
	// make this leak like a mofo for testing
	// since it performs multiple frees like crazy
	//free(ip);
    }
    return (error);
}

#endif

/*
 * Return buffer with the contents of block "offset" from the beginning of
 * directory "ip".  If "res" is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
int
cd9660_blkatoff(cyg_file *a_vp,
		off_t a_offset,
		char **a_res,
		struct buf **a_bpp)
{
    struct iso_node *ip;
    struct iso_mnt *imp;
    struct buf *bp;
    cyg_int32 lbn;
    int bsize, error;

    ip = VTOI(a_vp);
    imp = ip->i_mnt;
    lbn = lblkno(imp, a_offset);
    bsize = blksize(imp, ip, lbn);

    if ((error = bread(a_vp, lbn, bsize, &bp)) != 0) {
	brelse(bp);
	*a_bpp = NULL;
	return (error);
    }
    if (a_res)
	*a_res = (char *)bp->b_data + blkoff(imp, a_offset);
    *a_bpp = bp;
    return (0);
}

/*
 * File attributes
 */
void
cd9660_defattr(struct iso_directory_record *isodir,
	       struct iso_node *inop,
	       struct buf *bp)
{
    struct buf *bp2 = NULL;
    struct iso_mnt *imp;
    struct iso_extended_attributes *ap = NULL;
    int off;
	
    if (isonum_711(isodir->flags)&2) {
	inop->inode.iso_mode = __stat_mode_DIR;
	/*
	 * If we return 2, fts() will assume there are no subdirectories
	 * (just links for the path and .), so instead we return 1.
	 */
	inop->inode.iso_links = 1;
    } else {
	inop->inode.iso_mode = __stat_mode_REG;
	inop->inode.iso_links = 1;
    }
    if (!bp
	&& ((imp = inop->i_mnt)->im_flags & ISOFSMNT_EXTATT)
	&& (off = isonum_711(isodir->ext_attr_length))) {
	cd9660_blkatoff(ITOV(inop), (off_t)-(off << imp->im_bshift), NULL,
			&bp2);
	bp = bp2;
    }
    if (bp) {
	ap = (struct iso_extended_attributes *)bp->b_data;
		
	if (isonum_711(ap->version) == 1) {
	    if (!(ap->perm[1]&0x10))
		inop->inode.iso_mode |= S_IRUSR;
	    if (!(ap->perm[1]&0x40))
		inop->inode.iso_mode |= S_IXUSR;
	    if (!(ap->perm[0]&0x01))
		inop->inode.iso_mode |= S_IRGRP;
	    if (!(ap->perm[0]&0x04))
		inop->inode.iso_mode |= S_IXGRP;
	    if (!(ap->perm[0]&0x10))
		inop->inode.iso_mode |= S_IROTH;
	    if (!(ap->perm[0]&0x40))
		inop->inode.iso_mode |= S_IXOTH;
	    inop->inode.iso_uid = isonum_723(ap->owner); /* what about 0? */
	    inop->inode.iso_gid = isonum_723(ap->group); /* what about 0? */
	} else
	    ap = NULL;
    }
    if (!ap) {
	inop->inode.iso_mode |=
	    S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
	inop->inode.iso_uid = (uid_t)0;
	inop->inode.iso_gid = (gid_t)0;
    }
    if (bp2)
	brelse(bp2);
}

/*
 * Time stamps
 */
void
cd9660_deftstamp(struct iso_directory_record *isodir,
		 struct iso_node *inop,
		 struct buf *bp)
{
    struct buf *bp2 = NULL;
    struct iso_mnt *imp;
    struct iso_extended_attributes *ap = NULL;
    int off;
	
    if (!bp
	&& ((imp = inop->i_mnt)->im_flags & ISOFSMNT_EXTATT)
	&& (off = isonum_711(isodir->ext_attr_length))) {
	cd9660_blkatoff(ITOV(inop), (off_t)-(off << imp->im_bshift), NULL,
			&bp2);
	bp = bp2;
    }
    if (bp) {
	ap = (struct iso_extended_attributes *)bp->b_data;
	
	if (isonum_711(ap->version) == 1) {
	    if (!cd9660_tstamp_conv17(ap->ftime,&inop->inode.iso_atime))
		cd9660_tstamp_conv17(ap->ctime,&inop->inode.iso_atime);
	    if (!cd9660_tstamp_conv17(ap->ctime,&inop->inode.iso_ctime))
		inop->inode.iso_ctime = inop->inode.iso_atime;
	    if (!cd9660_tstamp_conv17(ap->mtime,&inop->inode.iso_mtime))
		inop->inode.iso_mtime = inop->inode.iso_ctime;
	} else
	    ap = NULL;
    }
    if (!ap) {
	cd9660_tstamp_conv7(isodir->date,&inop->inode.iso_ctime);
	inop->inode.iso_atime = inop->inode.iso_ctime;
	inop->inode.iso_mtime = inop->inode.iso_ctime;
    }
    if (bp2)
	brelse(bp2);
}

int
cd9660_tstamp_conv7(unsigned char *pi,
		    struct timespec *pu)
{
    int crtime, days;
    int y, m, d, hour, minute, second;
    signed char tz;
	
    y = pi[0] + 1900;
    m = pi[1];
    d = pi[2];
    hour = pi[3];
    minute = pi[4];
    second = pi[5];
    tz = (signed char) pi[6];
	
    if (y < 1970) {
	pu->tv_sec  = 0;
	pu->tv_nsec = 0;
	return (0);
    } else {
#ifdef	ORIGINAL
	/* computes day number relative to Sept. 19th,1989 */
	/* don't even *THINK* about changing formula. It works! */
	days = 367*(y-1980)-7*(y+(m+9)/12)/4-3*((y+(m-9)/7)/100+1)/4+275*m/9+d-100;
#else
	/*
	 * Changed :-) to make it relative to Jan. 1st, 1970
	 * and to disambiguate negative division
	 */
	days = 367*(y-1960)-7*(y+(m+9)/12)/4-3*((y+(m+9)/12-1)/100+1)/4+275*m/9+d-239;
#endif
	crtime = ((((days * 24) + hour) * 60 + minute) * 60) + second;
		
	/* timezone offset is unreliable on some disks */
	if (-48 <= tz && tz <= 52)
	    crtime -= tz * 15 * 60;
    }
    pu->tv_sec  = crtime;
    pu->tv_nsec = 0;
    return (1);
}

int
cd9660_tstamp_conv17(unsigned char *pi,
		     struct timespec *pu)
{
    unsigned char buf[7];
	
    /* year:"0001"-"9999" -> -1900  */
    buf[0] = _chars2ui(pi,4) - 1900;
	
    /* month: " 1"-"12"      -> 1 - 12 */
    buf[1] = _chars2ui(pi + 4,2);
	
    /* day:   " 1"-"31"      -> 1 - 31 */
    buf[2] = _chars2ui(pi + 6,2);
	
    /* hour:  " 0"-"23"      -> 0 - 23 */
    buf[3] = _chars2ui(pi + 8,2);
	
    /* minute:" 0"-"59"      -> 0 - 59 */
    buf[4] = _chars2ui(pi + 10,2);
	
    /* second:" 0"-"59"      -> 0 - 59 */
    buf[5] = _chars2ui(pi + 12,2);
	
    /* difference of GMT */
    buf[6] = pi[16];
	
    return (cd9660_tstamp_conv7(buf,pu));
}

/*
 * translate a filename of length > 0
 */
void
isofntrans(cyg_uint8 *infn,
	   int infnlen,
	   cyg_uint8 *outfn,
	   cyg_uint16 *outfnlen,
	   int original,
	   int assoc,
	   int joliet_level)
{
    int fnidx = 0;
    cyg_uint8 c, d = '\0', *infnend = infn + infnlen;
    
    if (assoc) {
	*outfn++ = ASSOCCHAR;
	fnidx++;
    }
    for (; infn != infnend; fnidx++) {
	infn += isochar(infn, infnend, joliet_level, &c);
	
	if (!original && c == ';') {
	    fnidx -= (d == '.');
	    break;
	} else
	    *outfn++ = c;
	d = c;
    }
    *outfnlen = fnidx;
}

/*
 * Get one character out of an iso filename
 * Obey joliet_level
 * Return number of bytes consumed
 */
int
isochar(const cyg_uint8 *isofn,
	const cyg_uint8 *isoend,
	int joliet_level,
	cyg_uint8 *c)
{
    *c = *isofn++;
    if (joliet_level == 0 || isofn == isoend)
	/* (00) and (01) are one byte in Joliet, too */
	return 1;
    
    /* No Unicode support yet :-( */
    switch (*c) {
	default:
	    *c = '?';
	    break;
	case '\0':
	    *c = *isofn;
	    break;
    }
    return 2;
}

int
cd9660_open_internal(struct nameidata * ndp, int fmode, int cmode)
{
    cyg_file *vp;
    int error;

    /* Check access flags */
    if ((fmode & (O_ACCMODE)) == 0)
	return (EINVAL);
    if ((fmode & (O_ACCMODE)) != O_RDONLY)
	return (EROFS);
    if (fmode & O_TRUNC ||
	fmode & O_CREAT ||
	fmode & O_APPEND)
	return (EROFS);
    
    /* Find the node */
    ndp->ni_cnd.cn_nameiop = LOOKUP;
    ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF;
    if ((error = namei(ndp)) != 0)
	return (error);
    vp = ndp->ni_vp;

    if (vp->f_type == CYG_FILE_TYPE_SOCKET) {
	error = EOPNOTSUPP;
	goto bad;
    }
    if (vp->f_type == CYG_FILE_TYPE_LNK) {
	error = EMLINK;
	goto bad;
    }
    if ((fmode & O_CREAT) == 0) {
	if ((fmode & O_RDONLY) == 0) {
	    /* TODO Implement access call */
	    //if ((error = VOP_ACCESS(vp, VREAD, cred, p)) != 0)
	    //	goto bad;
	}
	if (fmode & O_WRONLY) {
	    if (vp->f_type == CYG_FILE_TYPE_DIR) {
		error = EISDIR;
		goto bad;
	    }
	    /* TODO VOP_ACCESS should return EROFS for cd9660 */
	    //if ((error = VOP_ACCESS(vp, VWRITE, cred, p)) != 0)
	    //	goto bad;
	    error = EROFS;
	    goto bad;
	}
    }

    return (0);
  bad:
    vput(vp);
    return (error);
}

/*
 * Convert a component of a pathname into a pointer to a locked inode.
 * This is a very central and rather complicated routine.
 * If the file system is not maintained in a strict tree hierarchy,
 * this can result in a deadlock situation (see comments in code below).
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it and the target of the pathname
 * exists, lookup returns both the target and its parent directory locked.
 * When creating or renaming and LOCKPARENT is specified, the target may
 * not be ".".  When deleting and LOCKPARENT is specified, the target may
 * be "."., but the caller must check to ensure it does an vrele and iput
 * instead of two iputs.
 *
 * Overall outline of cd9660_lookup:
 *
 *	check accessibility of directory
 *	look for name in cache, if found, then if at end of path
 *	  and deleting or creating, drop it, else return name
 *	search for name in directory, to found or notfound
 * notfound:
 *	if creating, return locked directory, leaving info on available slots
 *	else return error
 * found:
 *	if at end of path and deleting, return information to allow delete
 *	if at end of path and rewriting (RENAME and LOCKPARENT), lock target
 *	  inode and return info to allow rewrite
 *	if not at end, add name to cache; if at end and neither creating
 *	  nor deleting, add name to cache
 *
 * NOTE: (LOOKUP | LOCKPARENT) currently returns the parent inode unlocked.
 */
int
cd9660_lookup(cyg_file *a_dvp,
	      cyg_file *vp,
	      struct componentname *a_cnp)
{
    cyg_file *vdp;	/* vnode for directory being searched */
    struct iso_node *dp;	/* inode for directory being searched */
    struct iso_mnt *imp;	/* file system that directory is in */
    struct buf *bp;			/* a buffer of directory entries */
    struct iso_directory_record *ep = NULL;
    /* the current directory entry */
    int entryoffsetinblock;		/* offset of ep in bp's buffer */
    int saveoffset = -1;		/* offset of last directory entry in dir */
    int numdirpasses;		/* strategy for directory search */
    doff_t endsearch;		/* offset to end directory search */
    cyg_file *pdp;		/* saved dp during symlink work */
    unsigned long bmask;			/* block offset mask */
    int lockparent;			/* 1 => lockparent flag is set */
    int wantparent;			/* 1 => wantparent or lockparent flag */
    int error;
    ino_t ino = 0;
    int reclen;
    unsigned short namelen;
    char altname[NAME_MAX];
    int res;
    int assoc, len;
    char *name;
    struct componentname *cnp = a_cnp;
    int flags = cnp->cn_flags;
    int nameiop = cnp->cn_nameiop;
    
    bp = NULL;
    vdp = a_dvp;
    dp = VTOI(vdp);
    imp = dp->i_mnt;
    lockparent = flags & LOCKPARENT;
    wantparent = flags & (LOCKPARENT|WANTPARENT);
	
    /*
     * We now have a segment name to search for, and a directory to search.
     */
    len = cnp->cn_namelen;
    name = cnp->cn_nameptr;
    /*
     * A leading `=' means, we are looking for an associated file
     */
    assoc = (imp->iso_ftype != ISO_FTYPE_RRIP && *name == ASSOCCHAR);
    if (assoc) {
	len--;
	name++;
    }
    
    /*
     * If there is cached information on a previous search of
     * this directory, pick up where we last left off.
     * We cache only lookups as these are the most common
     * and have the greatest payoff. Caching CREATE has little
     * benefit as it usually must search the entire directory
     * to determine that the entry does not exist. Caching the
     * location of the last DELETE or RENAME has not reduced
     * profiling time and hence has been removed in the interest
     * of simplicity.
     */
    bmask = imp->im_bmask;
    if (nameiop != LOOKUP || dp->i_diroff == 0 ||
	dp->i_diroff > dp->i_size) {
	entryoffsetinblock = 0;
	dp->i_offset = 0;
	numdirpasses = 1;
    } else {
	dp->i_offset = dp->i_diroff;
	if ((entryoffsetinblock = dp->i_offset & bmask) &&
	    (error = cd9660_blkatoff(vdp, (off_t)dp->i_offset, NULL, &bp)))
	    return (error);
	numdirpasses = 2;
    }
    endsearch = dp->i_size;
    
  searchloop:
    while (dp->i_offset < endsearch) {
	/*
	 * If offset is on a block boundary,
	 * read the next directory block.
	 * Release previous if it exists.
	 */
	if ((dp->i_offset & bmask) == 0) {
	    if (bp != NULL)
		brelse(bp);
	    error = cd9660_blkatoff(vdp, (off_t)dp->i_offset,
				    NULL, &bp);
	    if (error)
		return (error);
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
	    dp->i_offset =
		(dp->i_offset & ~bmask) + imp->logical_block_size;
	    continue;
	}
	
	if (reclen < ISO_DIRECTORY_RECORD_SIZE)
	    /* illegal entry, stop */
	    break;
	
	if (entryoffsetinblock + reclen > imp->logical_block_size)
	    /* entries are not allowed to cross boundaries */
	    break;
	
	namelen = isonum_711(ep->name_len);
	
	if (reclen < ISO_DIRECTORY_RECORD_SIZE + namelen)
	    /* illegal entry, stop */
	    break;
	
	/*
	 * Check for a name match.
	 */
	switch (imp->iso_ftype) {
	    default:
		if ((!(isonum_711(ep->flags)&4)) == !assoc) {
		    if ((len == 1
			 && *name == '.')
			|| (flags & ISDOTDOT)) {
			if (namelen == 1
			    && ep->name[0] == ((flags & ISDOTDOT) ? 1 : 0)) {
			    /*
			     * Save directory entry's inode number and
			     * release directory buffer.
			     */
			    dp->i_ino = isodirino(ep, imp);
			    goto found;
			}
			if (namelen != 1
			    || ep->name[0] != 0)
			    goto notfound;
		    } else if (!(res = isofncmp(name, len, 
						ep->name, namelen, imp->joliet_level))) {
			if (isonum_711(ep->flags)&2)
			    ino = isodirino(ep, imp);
			else
			    ino = dbtob(bp->b_blkno)
				+ entryoffsetinblock;
			saveoffset = dp->i_offset;
		    } else if (ino)
			goto foundino;
#ifdef	NOSORTBUG	/* On some CDs directory entries are not sorted correctly */
		    else if (res < 0)
			goto notfound;
		    else if (res > 0 && numdirpasses == 2)
			numdirpasses++;
#endif
		}
		break;
	    case ISO_FTYPE_RRIP:
		if (isonum_711(ep->flags)&2)
		    ino = isodirino(ep, imp);
		else
		    ino = dbtob(bp->b_blkno) + entryoffsetinblock;
		dp->i_ino = ino;
		cd9660_rrip_getname(ep,altname,&namelen,&dp->i_ino,imp);
		if (namelen == cnp->cn_namelen
		    && !bcmp(name,altname,namelen))
		    goto found;
		ino = 0;
		break;
	}
	dp->i_offset += reclen;
	entryoffsetinblock += reclen;
    }
    if (ino) {
      foundino:
	dp->i_ino = ino;
	if (saveoffset != dp->i_offset) {
	    if (lblkno(imp, dp->i_offset) !=
		lblkno(imp, saveoffset)) {
		if (bp != NULL)
		    brelse(bp);
		if ((error = cd9660_blkatoff(vdp,
					     (off_t)saveoffset, NULL, &bp)) != 0)
		    return (error);
	    }
	    entryoffsetinblock = saveoffset & bmask;
	    ep = (struct iso_directory_record *)
		((char *)bp->b_data + entryoffsetinblock);
	    dp->i_offset = saveoffset;
	}
	goto found;
    }
  notfound:
    /*
     * If we started in the middle of the directory and failed
     * to find our target, we must check the beginning as well.
     */
    if (numdirpasses == 2) {
	numdirpasses--;
	dp->i_offset = 0;
	endsearch = dp->i_diroff;
	goto searchloop;
    }
    if (bp != NULL)
	brelse(bp);
    
    if (nameiop == CREATE || nameiop == RENAME)
	return (EJUSTRETURN);
    return (ENOENT);
    
  found:
    /*
     * Found component in pathname.
     * If the final component of path name, save information
     * in the cache as to where the entry was found.
     */
    if ((flags & ISLASTCN) && nameiop == LOOKUP)
	dp->i_diroff = dp->i_offset;
    
    /*
     * Step through the translation in the name.  We do not `iput' the
     * directory because we may need it again if a symbolic link
     * is relative to the current directory.  Instead we save it
     * unlocked as "pdp".  We must get the target inode before unlocking
     * the directory to insure that the inode will not be removed
     * before we get it.  We prevent deadlock by always fetching
     * inodes from the root, moving down the directory tree. Thus
     * when following backward pointers ".." we must unlock the
     * parent directory before getting the requested directory.
     * There is a potential race condition here if both the current
     * and parent directories are removed before the `iget' for the
     * inode associated with ".." returns.  We hope that this occurs
     * infrequently since we cannot avoid this race condition without
     * implementing a sophisticated deadlock detection algorithm.
     * Note also that this simple deadlock detection scheme will not
     * work if the file system has any hard links other than ".."
     * that point backwards in the directory structure.
     */
    pdp = vdp;
    /*
     * If ino is different from dp->i_ino,
     * it's a relocated directory.
     */
    if (flags & ISDOTDOT) {
	brelse(bp);
	error = cd9660_vget_internal(vdp->f_mte, dp->i_ino, vp,
				     dp->i_ino != ino, NULL);
	if (error) {
	    return (error);
	}
    } else if (dp->i_number == dp->i_ino) {
	brelse(bp);
	VREF(vdp);	/* we want ourself, ie "." */
	if ((error = dupvnode(vp, vdp)) != 0) {
	    return (error);
	}
    } else {
	error = cd9660_vget_internal(vdp->f_mte, dp->i_ino, vp,
				     dp->i_ino != ino, ep);
	brelse(bp);
	if (error)
	    return (error);
    }
	
    /*
     * Insert name into cache if appropriate.
     */
    return (0);
}

/*
 * translate and compare a filename
 * returns (fn - isofn)
 * Note: Version number plus ';' may be omitted.
 */
int
isofncmp(const cyg_uint8 *fn, int fnlen, const cyg_uint8 *isofn, int isolen, int joliet_level)
{
    int i, j;
    cyg_uint8 c;
    const cyg_uint8 *fnend = fn + fnlen, *isoend = isofn + isolen;
	
    for (; fn != fnend; fn++) {
	if (isofn == isoend)
	    return *fn;
	isofn += isochar(isofn, isoend, joliet_level, &c);
	if (c == ';') {
	    if (*fn++ != ';')
		return fn[-1];
	    for (i = 0; fn != fnend; i = i * 10 + *fn++ - '0') {
		if (*fn < '0' || *fn > '9') {
		    return -1;
		}
	    }
	    for (j = 0; isofn != isoend; j = j * 10 + c - '0')
		isofn += isochar(isofn, isoend,
				 joliet_level, &c);
	    return i - j;
	}
	if (((cyg_uint8) c) != *fn) {
	    if (c >= 'A' && c <= 'Z') {
		if (c + ('a' - 'A') != *fn) {
		    if (*fn >= 'a' && *fn <= 'z')
			return *fn - ('a' - 'A') - c;
		    else
			return *fn - c;
		}
	    } else
		return *fn - c;
	}
    }
    if (isofn != isoend) {
	isofn += isochar(isofn, isoend, joliet_level, &c);
	switch (c) {
	    default:
		return -c;
	    case '.':
		if (isofn != isoend) {
		    isochar(isofn, isoend, joliet_level, &c);
		    if (c == ';')
			return 0;
		}
		return -1;
	    case ';':
		return 0;
	}
    }
    return 0;
}

int
cd9660_stat_internal(cyg_file *vp,
		     struct stat *sbp)
{
    unsigned short mode;
    struct iso_node *ip = VTOI(vp);

    sbp->st_dev = 0;
    sbp->st_ino = ip->i_number;
    sbp->st_mode = ip->inode.iso_mode & ALLPERMS;
    sbp->st_nlink = ip->inode.iso_links;
    sbp->st_uid	= ip->inode.iso_uid;
    sbp->st_gid	= ip->inode.iso_gid;
    /* TODO Fix assignments regarding types */ 
    //sbp->st_atime	= ip->inode.iso_atime;
    //sbp->st_mtime	= ip->inode.iso_mtime;
    //sbp->st_ctime	= ip->inode.iso_ctime;

    sbp->st_size	= (cyg_uint64) ip->i_size;
    if (ip->i_size == 0 && vp->f_type  == CYG_FILE_TYPE_LNK) {
#if 1
	diag_printf("XXX Following symlink not supported yet XXX");
#else
	struct vop_readlink_args rdlnk;
	cyg_iovec aiov;
	cyg_uio auio;
	char *cp;
	
	cp = (char *)malloc(MAXPATHLEN);
	aiov.iov_base = cp;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = MAXPATHLEN;
	rdlnk.a_uio = &auio;
	rdlnk.a_vp = a_vp;
	if (cd9660_readlink(&rdlnk) == 0)
	    vap->va_size = MAXPATHLEN - auio.uio_resid;
	free(cp);
#endif
    }
    /*
     * Copy from vattr table
     */
    switch (vp->f_type) {
	case CYG_FILE_TYPE_FILE:
	    mode |= __stat_mode_REG;
	    break;
	case CYG_FILE_TYPE_DIR:
	    mode |= __stat_mode_DIR;
	    break;
	case CYG_FILE_TYPE_BLK:
	    mode |= __stat_mode_BLK;
	    break;
	default:
	    /* TODO Handle other CYG_FILE types */
	    return (EBADF);
    }
    return (0);
}

int
cd9660_pathconf(cyg_file * vp, struct cyg_pathconf_info * info)
{
    switch (info->name) {
	case _PC_LINK_MAX:
	    info->value = 1;
	    return (0);
	case _PC_NAME_MAX:
	    if (VTOI(vp)->i_mnt->iso_ftype == ISO_FTYPE_RRIP)
		info->value = NAME_MAX;
	    else
		info->value = 37;
	    return (0);
	case _PC_PATH_MAX:
	    info->value = PATH_MAX;
	    return (0);
	case _PC_CHOWN_RESTRICTED:
	    info->value = 1;
	    return (0);
	case _PC_NO_TRUNC:
	    info->value = 1;
	    return (0);
	default:
	    return (EINVAL);
    }
    return (EINVAL);
}

static unsigned int
_chars2ui(unsigned char *begin, int len)
{
    unsigned int rc;
    
    for (rc = 0; --len >= 0;) {
	rc *= 10;
	rc += *begin++ - '0';
    }
    return (rc);
}
