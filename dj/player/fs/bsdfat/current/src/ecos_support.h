// ecos_support.h: structures and macros needed for FreeBSD fat to compile under ecos
// danc@iobjects.com 01/02/02

// Portions of this file were taken from FreeBSD sources.

#ifndef __BSDFAT_ECOS_SUPPORT_H__
#define __BSDFAT_ECOS_SUPPORT_H__

#include <cyg/fileio/fileio.h>
#include <io/bio/bio.h>

#define MSDOSFS_DEBUG

// Mystery function. i have no idea where this comes from
extern cyg_file* cyg_fp_get(int);

#define DEV_BSIZE 512

#define ffs(x) _msdos_ffs(x)
static inline int _msdos_ffs(int val) 
{
    int res = 1;
    while( !(val & 0x01) ) {
        res++;
        val>>=1;
    }
    return res;
}

struct nameidata {
    /*
     * Arguments to namei/lookup.
     */
    const char *ni_dirp;                /* pathname pointer */
    enum        cyg_uio_seg ni_segflg;  /* location of pathname */
    /*
     * Arguments to lookup.
     */
    cyg_file *ni_startdir;      /* starting directory */
    cyg_file *ni_rootdir;       /* logical root directory */
    cyg_file *ni_cdir;
    /*
     * Results: returned from/manipulated by lookup
     */
    cyg_file *ni_vp;            /* vnode of result */
    cyg_file *ni_dvp;           /* vnode of intermediate directory */
    /*
     * Shared between namei and lookup/commit routines.
     */
    size_t      ni_pathlen;             /* remaining chars in path */
    char        *ni_next;               /* next location in pathname */
    unsigned long       ni_loopcnt;     /* count of symlinks encountered */
    /*
     * Lookup parameters: this structure describes the subset of
     * information from the nameidata structure that is passed
     * through the VOP interface.
     */
    struct componentname {
        /*
         * Arguments to lookup.
         */
        unsigned long   cn_nameiop;     /* namei operation */
        unsigned long   cn_flags;       /* flags to namei */
        /*
         * Shared between lookup and commit routines.
         */
        char    *cn_pnbuf;      /* pathname buffer */
        char    *cn_nameptr;    /* pointer to looked up name */
        long    cn_namelen;     /* length of looked up component */
        unsigned long   cn_hash;/* hash value of looked up name */
        long    cn_consume;     /* chars to consume in lookup() */
    } ni_cnd;
};

#define NDINIT(ndp, op, flags, segflg, namep ) { \
        (ndp)->ni_cnd.cn_nameiop = op; \
        (ndp)->ni_cnd.cn_flags = flags; \
        (ndp)->ni_segflg = segflg; \
        (ndp)->ni_dirp = namep; \
}

/*
 * This structure describes the elements in the cache of recent
 * names looked up by namei. NCHNAMLEN is sized to make structure
 * size a power of two to optimize malloc's. Minimum reasonable
 * size is 15.
 */

#define NCHNAMLEN       31      /* maximum name segment length we bother with */


// namei stuff
#define LOOKUP          0       /* perform name lookup only */
#define CREATE          1       /* setup for file creation */
#define DELETE          2       /* setup for file deletion */
#define RENAME          3       /* setup for file renaming */
#define OPMASK          3       /* mask for operation */

/*
 * namei operational modifier flags, stored in ni_cnd.flags
 */
#define	LOCKLEAF	0x0004	/* lock inode on return */
#define	LOCKPARENT	0x0008	/* want parent vnode returned locked */
#define	WANTPARENT	0x0010	/* want parent vnode returned unlocked */
#define	NOCACHE		0x0020	/* name must not be left in cache */
#define	FOLLOW		0x0040	/* follow symbolic links */
#define	NOOBJ		0x0080	/* don't create object */
#define	NOFOLLOW	0x0000	/* do not follow symbolic links (pseudo) */
#define	MODMASK		0x00fc	/* mask of operational modifiers */

#define NOCROSSMOUNT    0x000100      /* do not cross mount points */
#define RDONLY          0x000200      /* lookup with read-only semantics */
#define HASBUF          0x000400      /* has allocated pathname buffer */
#define SAVENAME        0x000800      /* save pathanme buffer */
#define SAVESTART       0x001000      /* save starting directory */
#define ISDOTDOT        0x002000      /* current component name is .. */
#define MAKEENTRY       0x004000      /* entry is to be added to name cache */
#define ISLASTCN        0x008000      /* this is last component of pathname */
#define ISSYMLINK       0x010000      /* symlink needs interpretation */
#define ISWHITEOUT      0x020000      /* found whiteout */
#define DOWHITEOUT      0x040000      /* do whiteouts */
#define REQUIREDIR      0x080000      /* must be a directory */
#define STRIPSLASHES    0x100000      /* strip trailing slashes */
#define PARAMASK        0x1fff00      /* mask of parameter descriptors */     

void NDFREE __P((struct nameidata *, const uint));

int	namei __P((struct nameidata *ndp));

#define NDF_NO_DVP_RELE		0x00000001
#define NDF_NO_DVP_UNLOCK	0x00000002
#define NDF_NO_DVP_PUT		0x00000003
#define NDF_NO_VP_RELE		0x00000004
#define NDF_NO_VP_UNLOCK	0x00000008
#define NDF_NO_VP_PUT		0x0000000c
#define NDF_NO_STARTDIR_RELE	0x00000010
#define NDF_NO_FREE_PNBUF	0x00000020
#define NDF_ONLY_PNBUF		(~NDF_NO_FREE_PNBUF)

/*
 * User specifiable flags.
 */
#define	MNT_RDONLY	0x00000001	/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x00000002	/* file system written synchronously */
#define	MNT_NOEXEC	0x00000004	/* can't exec from filesystem */
#define	MNT_NOSUID	0x00000008	/* don't honor setuid bits on fs */
#define	MNT_NODEV	0x00000010	/* don't interpret special files */
#define	MNT_UNION	0x00000020	/* union with underlying filesystem */
#define	MNT_ASYNC	0x00000040	/* file system written asynchronously */
#define	MNT_SUIDDIR	0x00100000	/* special handling of SUID on dirs */
#define	MNT_SOFTDEP	0x00200000	/* soft updates being done */
#define	MNT_NOSYMFOLLOW	0x00400000	/* do not follow symlinks */
#define	MNT_JAILDEVFS	0x02000000	/* Jail friendly DEVFS behaviour */
#define	MNT_NOATIME	0x10000000	/* disable update of file access time */
#define	MNT_NOCLUSTERR	0x40000000	/* disable cluster read */
#define	MNT_NOCLUSTERW	0x80000000	/* disable cluster write */
/*
 * External filesystem command modifier flags.
 * Unmount can use the MNT_FORCE flag.
 * XXX These are not STATES and really should be somewhere else.
 */
#define	MNT_UPDATE	0x00010000	/* not a real mount, just an update */
#define	MNT_DELEXPORT	0x00020000	/* delete export host lists */
#define	MNT_RELOAD	0x00040000	/* reload filesystem data */
#define	MNT_FORCE	0x00080000	/* force unmount or readonly change */
#define	MNT_SNAPSHOT	0x01000000	/* snapshot the filesystem */
#define MNT_CMDFLAGS   (MNT_UPDATE	| MNT_DELEXPORT	| MNT_RELOAD	| \
			MNT_FORCE	| MNT_SNAPSHOT)
/*
 * Internal filesystem control flags stored in mnt_kern_flag.
 *
 * MNTK_UNMOUNT locks the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define MNTK_UNMOUNT	0x01000000	/* unmount in progress */
#define	MNTK_MWAIT	0x02000000	/* waiting for unmount to finish */
#define MNTK_WANTRDWR	0x04000000	/* upgrade to read/write requested */
#define	MNTK_SUSPEND	0x08000000	/* request write suspension */
#define	MNTK_SUSPENDED	0x10000000	/* write operations are suspended */

/*
 * Flags set by internal operations,
 * but visible to the user.
 * XXX some of these are not quite right.. (I've never seen the root flag set)
 */
#define	MNT_LOCAL	0x00001000	/* filesystem is stored locally */
#define	MNT_QUOTA	0x00002000	/* quotas are enabled on filesystem */
#define	MNT_ROOTFS	0x00004000	/* identifies the root filesystem */
#define	MNT_USER	0x00008000	/* mounted by a user */
#define	MNT_IGNORE	0x00800000	/* do not show entry in df */

#define MFSNAMELEN	16	/* length of fs type name, including null */
#define	MNAMELEN	80	/* length of buffer for returned name */

/*
 * Flags to various vnode functions.
 */
#define	SKIPSYSTEM	0x0001	/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE	0x0002	/* vflush: force file closure */
#define	WRITECLOSE	0x0004	/* vflush: only close writable files */
#define	DOCLOSE		0x0008	/* vclean: close active files */
#define	V_SAVE		0x0001	/* vinvalbuf: sync file first */
#define	REVOKEALL	0x0001	/* vop_revoke: revoke all aliases */
#define	V_WAIT		0x0001	/* vn_start_write: sleep for suspend */
#define	V_NOWAIT	0x0002	/* vn_start_write: don't sleep for suspend */
#define	V_XSLEEP	0x0004	/* vn_start_write: just return after sleep */


typedef unsigned long device_t;

#endif // __BSDFAT_ECOS_SUPPORT_H__
