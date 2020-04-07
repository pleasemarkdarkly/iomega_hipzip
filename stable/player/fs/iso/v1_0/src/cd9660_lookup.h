/* Stolen from OpenBSD sources */
#ifndef CD9660_LOOKUP_H
#define	CD9660_LOOKUP_H

#include <cyg/fileio/fileio.h>

/*
 * Encapsulation of namei parameters.
 */
struct nameidata {
    /*
     * Arguments to namei/lookup.
     */
    const char *ni_dirp;		/* pathname pointer */
    enum	cyg_uio_seg ni_segflg;	/* location of pathname */
    /*
     * Arguments to lookup.
     */
    cyg_file *ni_startdir;	/* starting directory */
    cyg_file *ni_rootdir;	/* logical root directory */
    cyg_file *ni_cdir;
    /*
     * Results: returned from/manipulated by lookup
     */
    cyg_file *ni_vp;		/* vnode of result */
    cyg_file *ni_dvp;		/* vnode of intermediate directory */
    /*
     * Shared between namei and lookup/commit routines.
     */
    size_t	ni_pathlen;		/* remaining chars in path */
    char	*ni_next;		/* next location in pathname */
    unsigned long	ni_loopcnt;	/* count of symlinks encountered */
    /*
     * Lookup parameters: this structure describes the subset of
     * information from the nameidata structure that is passed
     * through the VOP interface.
     */
    struct componentname {
	/*
	 * Arguments to lookup.
	 */
	unsigned long	cn_nameiop;	/* namei operation */
	unsigned long	cn_flags;	/* flags to namei */
	/*
	 * Shared between lookup and commit routines.
	 */
	char	*cn_pnbuf;	/* pathname buffer */
	char	*cn_nameptr;	/* pointer to looked up name */
	long	cn_namelen;	/* length of looked up component */
	unsigned long	cn_hash;/* hash value of looked up name */
	long	cn_consume;	/* chars to consume in lookup() */
    } ni_cnd;
};

/*
 * namei operations
 */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	RENAME		3	/* setup for file renaming */
#define	OPMASK		3	/* mask for operation */
/*
 * namei operational modifier flags, stored in ni_cnd.flags
 */
#define	LOCKLEAF	0x0004	/* lock inode on return */
#define	LOCKPARENT	0x0008	/* want parent vnode returned locked */
#define	WANTPARENT	0x0010	/* want parent vnode returned unlocked */
#define	NOCACHE		0x0020	/* name must not be left in cache */
#define	FOLLOW		0x0040	/* follow symbolic links */
#define	NOFOLLOW	0x0000	/* do not follow symbolic links (pseudo) */
#define	MODMASK		0x00fc	/* mask of operational modifiers */
/*
 * Namei parameter descriptors.
 *
 * SAVENAME may be set by either the callers of namei or by VOP_LOOKUP.
 * If the caller of namei sets the flag (for example execve wants to
 * know the name of the program that is being executed), then it must
 * free the buffer. If VOP_LOOKUP sets the flag, then the buffer must
 * be freed by either the commit routine or the VOP_ABORT routine.
 * SAVESTART is set only by the callers of namei. It implies SAVENAME
 * plus the addition of saving the parent directory that contains the
 * name in ni_startdir. It allows repeated calls to lookup for the
 * name being sought. The caller is responsible for releasing the
 * buffer and for vrele'ing ni_startdir.
 */
#define	NOCROSSMOUNT	0x000100      /* do not cross mount points */
#define	RDONLY		0x000200      /* lookup with read-only semantics */
#define	HASBUF		0x000400      /* has allocated pathname buffer */
#define	SAVENAME	0x000800      /* save pathanme buffer */
#define	SAVESTART	0x001000      /* save starting directory */
#define ISDOTDOT	0x002000      /* current component name is .. */
#define MAKEENTRY	0x004000      /* entry is to be added to name cache */
#define ISLASTCN	0x008000      /* this is last component of pathname */
#define ISSYMLINK	0x010000      /* symlink needs interpretation */
#define	ISWHITEOUT	0x020000      /* found whiteout */
#define	DOWHITEOUT	0x040000      /* do whiteouts */
#define	REQUIREDIR	0x080000      /* must be a directory */
#define STRIPSLASHES    0x100000      /* strip trailing slashes */
#define PARAMASK	0x1fff00      /* mask of parameter descriptors */

/*
 * Initialization of an nameidata structure.
 */
#define NDINIT(ndp, op, flags, segflg, namep, rdir, cdir) { \
	(ndp)->ni_cnd.cn_nameiop = op; \
	(ndp)->ni_cnd.cn_flags = flags; \
	(ndp)->ni_segflg = segflg; \
	(ndp)->ni_dirp = namep; \
        (ndp)->ni_rootdir = rdir; \
        (ndp)->ni_cdir = cdir; \
}

/*
 * This structure describes the elements in the cache of recent
 * names looked up by namei. NCHNAMLEN is sized to make structure
 * size a power of two to optimize malloc's. Minimum reasonable
 * size is 15.
 */

#define	NCHNAMLEN	31	/* maximum name segment length we bother with */

int namei(struct nameidata *ndp);

#endif /* CD9660_LOOKUP_H */
