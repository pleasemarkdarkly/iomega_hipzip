#include "cd9660_lookup.h"
#include "cd9660_node.h"
#include "cd9660_support.h"
#include "cd9660_internal.h"
#include <stdlib.h>
#include <cyg/infra/diag.h>
#include <cyg/error/codes.h>

#if (NAME_MAX < 255)
#undef NAME_MAX
#define NAME_MAX 255
#endif

static int _lookup(struct nameidata *ndp);

/*
 * Convert a pathname into a pointer to a locked inode.
 *
 * The FOLLOW flag is set when symbolic links are to be followed
 * when they occur at the end of the name translation process.
 * Symbolic links are always followed for all other pathname
 * components other than the last.
 *
 * The segflg defines whether the name is to be copied from user
 * space or kernel space.
 *
 * Overall outline of namei:
 *
 *	copy in name
 *	get starting directory
 *	while (!done && !error) {
 *		call lookup to search path.
 *		if symbolic link, massage name in buffer and continue
 *	}
 */
int
namei(struct nameidata *ndp)
{
    char *cp;		/* pointer into pathname argument */
    cyg_file *dp;	/* the directory we are searching */
    cyg_iovec aiov;		/* uio for reading symbolic links */
    cyg_uio auio;
    int error, linklen;
    struct componentname *cnp = &ndp->ni_cnd;
    
    /*
     * Get a buffer for the name to be translated, and copy the
     * name into the buffer.
     */
    if ((cnp->cn_flags & HASBUF) == 0) {
	cnp->cn_pnbuf = (char *)malloc(PATH_MAX);
	if (cnp->cn_pnbuf == 0) {
	    return (ENOMEM);
	}
    }
    error = copystr(ndp->ni_dirp, cnp->cn_pnbuf,
		    PATH_MAX, &ndp->ni_pathlen);
    
    /*
     * Fail on null pathnames
     */
    if (error == 0 && ndp->ni_pathlen == 0)
	error = ENOENT;

    if (error) {
	free(cnp->cn_pnbuf);
	return (error);
    }

    /*
     *  Strip trailing slashes, as requested 
     */
    if (cnp->cn_flags & STRIPSLASHES) {
	char *end = cnp->cn_pnbuf + ndp->ni_pathlen - 2;
	
	cp = end;
	while (cp >= cnp->cn_pnbuf &&
	       (*cp == '/')) cp--;
	
	/* Still some remaining characters in the buffer */
	if (cp >= cnp->cn_pnbuf) {
	    ndp->ni_pathlen -= (end - cp);
	    *(cp + 1) = '\0';
	}
    }
    
    ndp->ni_loopcnt = 0;
    
    /* TODO Move the following section to before calling this function, so that cdir may be used correctly */
    /*
     * Check if starting from root directory or current directory.
     */
    if (cnp->cn_pnbuf[0] == '/') {
	dp = ndp->ni_rootdir;
    } else {
	dp = ndp->ni_cdir;
    }
    VREF(dp);

    /* Start looking */
    for (;;) {
	cnp->cn_nameptr = cnp->cn_pnbuf;
	ndp->ni_startdir = dp;
	if ((error = _lookup(ndp)) != 0) {
	    free(cnp->cn_pnbuf);
	    return (error);
	}
	/*
	 * Check for symbolic link
	 */
	if ((cnp->cn_flags & ISSYMLINK) == 0) {
	    if ((cnp->cn_flags & (SAVENAME | SAVESTART)) == 0)
		free(cnp->cn_pnbuf);
	    else
		cnp->cn_flags |= HASBUF;
	    return (0);
	}
	if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
	    error = ELOOP;
	    break;
	}
	if (ndp->ni_pathlen > 1)
	    cp = (char *)malloc(PATH_MAX);
	else
	    cp = cnp->cn_pnbuf;
	aiov.iov_base = cp;
	aiov.iov_len = PATH_MAX;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = PATH_MAX;
#if 1
	/* TODO Fix this */
	diag_printf("XXX Want to readlink XXX");
#else
	error = cd9660_readlink(ndp->ni_vp, &auio); //VOP_READLINK(ndp->ni_vp, &auio, cnp->cn_cred);
#endif
	if (error) {
badlink:
	    if (ndp->ni_pathlen > 1)
		free(cp);
	    break;
	}
	linklen = PATH_MAX - auio.uio_resid;
	if (linklen + ndp->ni_pathlen >= PATH_MAX) {
	    error = ENAMETOOLONG;
	    goto badlink;
	}
	if (ndp->ni_pathlen > 1) {
	    bcopy(ndp->ni_next, cp + linklen, ndp->ni_pathlen);
	    free(cnp->cn_pnbuf);
	    cnp->cn_pnbuf = cp;
	} else
	    cnp->cn_pnbuf[linklen] = '\0';
	ndp->ni_pathlen += linklen;
	vput(ndp->ni_vp);
	dp = ndp->ni_dvp;
	/*
	 * Check if root directory should replace current directory.
	 */
	if (cnp->cn_pnbuf[0] == '/') {
	    vrele(dp);
	    dp = ndp->ni_rootdir;
	    VREF(dp);
	}
    }
    free(cnp->cn_pnbuf);
    vrele(ndp->ni_dvp);
    vput(ndp->ni_vp);
    ndp->ni_vp = NULL;
    return (error);
}

/*
 * Search a pathname.
 * This is a very central and rather complicated routine.
 *
 * The pathname is pointed to by ni_ptr and is of length ni_pathlen.
 * The starting directory is taken from ni_startdir. The pathname is
 * descended until done, or a symbolic link is encountered. The variable
 * ni_more is clear if the path is completed; it is set to one if a
 * symbolic link needing interpretation is encountered.
 *
 * The flag argument is LOOKUP, CREATE, RENAME, or DELETE depending on
 * whether the name is to be looked up, created, renamed, or deleted.
 * When CREATE, RENAME, or DELETE is specified, information usable in
 * creating, renaming, or deleting a directory entry may be calculated.
 * If flag has LOCKPARENT or'ed into it, the parent directory is returned
 * locked. If flag has WANTPARENT or'ed into it, the parent directory is
 * returned unlocked. Otherwise the parent directory is not returned. If
 * the target of the pathname exists and LOCKLEAF is or'ed into the flag
 * the target is returned locked, otherwise it is returned unlocked.
 * When creating or renaming and LOCKPARENT is specified, the target may not
 * be ".".  When deleting and LOCKPARENT is specified, the target may be ".".
 * 
 * Overall outline of lookup:
 *
 * dirloop:
 *	identify next component of name at ndp->ni_ptr
 *	handle degenerate case where name is null string
 *	if .. and crossing mount points and on mounted filesys, find parent
 *	call VOP_LOOKUP routine for next component name
 *	    directory vnode returned in ni_dvp, unlocked unless LOCKPARENT set
 *	    component vnode returned in ni_vp (if it exists), locked.
 *	if result vnode is mounted on and crossing mount points,
 *	    find mounted on vnode
 *	if more components of name, do next level at dirloop
 *	return the answer in ni_vp, locked if LOCKLEAF set
 *	    if LOCKPARENT set, return locked parent in ni_dvp
 *	    if WANTPARENT set, return unlocked parent in ni_dvp
 */
int
_lookup(struct nameidata *ndp)
{
    char *cp;		/* pointer into pathname argument */
    cyg_file *dp = 0;	/* the directory we are searching */
    int docache;			/* == 0 do not cache last component */
    int wantparent;			/* 1 => wantparent or lockparent flag */
    int rdonly;			/* lookup read-only flag bit */
    int error = 0;
    int slashes;
    struct componentname *cnp = &ndp->ni_cnd;

    /*
     * Setup: break out flag bits into variables.
     */
    wantparent = cnp->cn_flags & (LOCKPARENT | WANTPARENT);
    docache = (cnp->cn_flags & NOCACHE) ^ NOCACHE;
    if (cnp->cn_nameiop == DELETE ||
	(wantparent && cnp->cn_nameiop != CREATE))
	docache = 0;
    rdonly = cnp->cn_flags & RDONLY;
    ndp->ni_dvp = NULL;
    cnp->cn_flags &= ~ISSYMLINK;
    dp = ndp->ni_startdir;
    ndp->ni_startdir = NULL;

    /*
     * If we have a leading string of slashes, remove them, and just make
     * sure the current node is a directory.
     */
    cp = cnp->cn_nameptr;
    while (*cp == '/') {
	cp++;
    }
    ndp->ni_pathlen -= cp - cnp->cn_nameptr;
    cnp->cn_nameptr = cp;
    
    /*
     * If we've exhausted the path name, then just return the
     * current node.  If the caller requested the parent node (i.e.
     * it's a CREATE, DELETE, or RENAME), and we don't have one
     * (because this is the root directory), then we must fail.
     */
    if (cnp->cn_nameptr[0] == '\0') {
	if (ndp->ni_dvp == NULL && wantparent) {
	    error = EISDIR;
	    goto bad;
	}
	if ((error = dupvnode(ndp->ni_vp, dp)) != 0) {
	    return (error);
	}
	cnp->cn_flags |= ISLASTCN;
	goto terminal;
    }

dirloop:
    /*
     * Search a new directory.
     *
     * The cn_hash value is for use by vfs_cache.
     * The last component of the filename is left accessible via
     * cnp->cn_nameptr for callers that need the name. Callers needing
     * the name set the SAVENAME flag. When done, they assume
     * responsibility for freeing the pathname buffer.
     */
    cnp->cn_consume = 0;
    cnp->cn_hash = 0;
    for (cp = cnp->cn_nameptr; *cp != '\0' && *cp != '/'; cp++)
	cnp->cn_hash += (unsigned char)*cp;
    cnp->cn_namelen = cp - cnp->cn_nameptr;
    if (cnp->cn_namelen > NAME_MAX) {
	error = ENAMETOOLONG;
	goto bad;
    }
#ifdef NAMEI_DIAGNOSTIC
    { char c = *cp;
    *cp = '\0';
    diag_printf("{%s}: ", cnp->cn_nameptr);
    *cp = c; }
#endif
    ndp->ni_pathlen -= cnp->cn_namelen;
    ndp->ni_next = cp;
    /*
     * If this component is followed by a slash, then move the pointer to
     * the next component forward, and remember that this component must be
     * a directory.
     */
    if (*cp == '/') {
	do {
	    cp++;
	} while (*cp == '/');
	slashes = cp - ndp->ni_next;
	ndp->ni_pathlen -= slashes;
	ndp->ni_next = cp;
	cnp->cn_flags |= REQUIREDIR;
    } else {
	slashes = 0;
	cnp->cn_flags &= ~REQUIREDIR;
    }
    /*
     * We do special processing on the last component, whether or not it's
     * a directory.  Cache all intervening lookups, but not the final one.
     */
    if (*cp == '\0') {
	if (docache)
	    cnp->cn_flags |= MAKEENTRY;
	else
	    cnp->cn_flags &= ~MAKEENTRY;
	cnp->cn_flags |= ISLASTCN;
    } else {
	cnp->cn_flags |= MAKEENTRY;
	cnp->cn_flags &= ~ISLASTCN;
    }
    if (cnp->cn_namelen == 2 &&
	cnp->cn_nameptr[1] == '.' && cnp->cn_nameptr[0] == '.')
	cnp->cn_flags |= ISDOTDOT;
    else
	cnp->cn_flags &= ~ISDOTDOT;
    
    /*
     * Handle "..": two special cases.
     * 1. If at root directory (e.g. after chroot)
     *    or at absolute root directory
     *    then ignore it so can't get out.
     * 2. If this vnode is the root of a mounted
     *    filesystem, then replace it with the
     *    vnode which was mounted on so we take the
     *    .. in the other file system.
     */
    if (cnp->cn_flags & ISDOTDOT) {
	if (dp == ndp->ni_rootdir) {
	    ndp->ni_dvp = dp;
	    VREF(dp);
	    if ((error = dupvnode(ndp->ni_vp, dp)) != 0) {
		return (error);
	    }
	    goto nextname;
	}
    }
    
    /*
     * We now have a segment name to search for, and a directory to search.
     */
    ndp->ni_dvp = dp;
    if ((error = cd9660_lookup(dp, ndp->ni_vp, cnp)) != 0) {
#ifdef NAMEI_DIAGNOSTIC
	diag_printf("not found\n");
#endif
	if (error != EJUSTRETURN)
	    goto bad;
	/*
	 * If this was not the last component, or there were trailing
	 * slashes, then the name must exist.
	 */
	if (cnp->cn_flags & REQUIREDIR) {
	    error = ENOENT;
	    goto bad;
	}
	
	/*
	 * We return with ni_vp NULL to indicate that the entry
	 * doesn't currently exist, leaving a pointer to the
	 * (possibly locked) directory inode in ndp->ni_dvp.
	 */
	if (cnp->cn_flags & SAVESTART) {
	    ndp->ni_startdir = ndp->ni_dvp;
	    VREF(ndp->ni_startdir);
	}
	return (0);
    }
#ifdef NAMEI_DIAGNOSTIC
    printf("found\n");
#endif
    
    /*
     * Take into account any additional components consumed by the
     * underlying filesystem.  This will include any trailing slashes after
     * the last component consumed.
     */
    if (cnp->cn_consume > 0) {
	if (cnp->cn_consume >= slashes) {
	    cnp->cn_flags &= ~REQUIREDIR;
	}
	
	ndp->ni_pathlen -= cnp->cn_consume - slashes;
	ndp->ni_next += cnp->cn_consume - slashes;
	cnp->cn_consume = 0;
	if (ndp->ni_next[0] == '\0')
	    cnp->cn_flags |= ISLASTCN;
    }
    
    dp = ndp->ni_vp;
    /*
     * Check for symbolic link.  Back up over any slashes that we skipped,
     * as we will need them again.
     */
    if ((dp->f_type == CYG_FILE_TYPE_LNK) && (cnp->cn_flags & (FOLLOW|REQUIREDIR))) {
	ndp->ni_pathlen += slashes;
	ndp->ni_next -= slashes;
	cnp->cn_flags |= ISSYMLINK;
	return (0);
    }
    
    /*
     * Check for directory, if the component was followed by a series of
     * slashes.
     */
    if ((dp->f_type != CYG_FILE_TYPE_DIR) && (cnp->cn_flags & REQUIREDIR)) {
	error = ENOTDIR;
	goto bad2;
    }
    
  nextname:
    /*
     * Not a symbolic link.  If this was not the last component, then
     * continue at the next component, else return.
     */
    if (!(cnp->cn_flags & ISLASTCN)) {
	cnp->cn_nameptr = ndp->ni_next;
	goto dirloop;
    }
    
  terminal:
	if (ndp->ni_dvp != NULL) {
	    if (cnp->cn_flags & SAVESTART) {
		ndp->ni_startdir = ndp->ni_dvp;
		VREF(ndp->ni_startdir);
	    }
	}
	return (0);

bad2:
	vrele(ndp->ni_dvp);
bad:
	// this was a double delete, thanks leaktracer // vput(dp);
	ndp->ni_vp = NULL;
	return (error);
}
