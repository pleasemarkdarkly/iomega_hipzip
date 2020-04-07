#ifndef __BIO_H__
#define __BIO_H__

#include <cyg/fileio/fileio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * The buffer header describes an I/O operation in the kernel.
 */
struct buf {
	volatile long b_flags;  /* B_* flags. */
	int	b_error;		    /* Errno value. */
	long	b_bufsize;		/* Allocated buffer size. */
	long	b_bcount;		/* Valid bytes in buffer. */
	size_t	b_resid;		/* Remaining I/O. */
    //    int     b_ucount;
	struct {
		char *	b_addr;		/* Memory, superblocks, indirect etc. */
	} b_un;
    
	cyg_int32 b_lblkno;		/* Logical block number. */
	cyg_int32 b_blkno;		/* Underlying physical block number. */

	cyg_file *b_vp;		/* Device vnode. */
};

/* Device driver compatibility definitions. */
#define	b_data	 b_un.b_addr		/* b_un.b_addr is not changeable. */

/*
 * These flags are kept in b_flags.
 */
#define	B_AGE		0x00000001	/* Move to age queue when I/O done. */
#define	B_NEEDCOMMIT	0x00000002	/* Needs committing to stable storage */
#define	B_ASYNC		0x00000004	/* Start I/O, do not wait. */
#define	B_BAD		0x00000008	/* Bad block revectoring in progress. */
#define	B_BUSY		0x00000010	/* I/O in progress. */
#define	B_CACHE		0x00000020	/* Bread found us in the cache. */
#define	B_CALL		0x00000040	/* Call b_iodone from biodone. */
#define	B_DELWRI	0x00000080	/* Delay I/O until buffer reused. */
#define	B_DIRTY		0x00000100	/* Dirty page to be pushed out async. */
#define	B_DONE		0x00000200	/* I/O completed. */
#define	B_EINTR		0x00000400	/* I/O was interrupted */
#define	B_ERROR		0x00000800	/* I/O error occurred. */
#define	B_GATHERED	0x00001000	/* LFS: already in a segment. */
#define	B_INVAL		0x00002000	/* Does not contain valid info. */
#define	B_LOCKED	0x00004000	/* Locked in core (not reusable). */
#define	B_NOCACHE	0x00008000	/* Do not cache block after use. */
#define	B_PAGET		0x00010000	/* Page in/out of page table space. */
#define	B_PGIN		0x00020000	/* Pagein op, so swap() can count it. */
#define	B_PHYS		0x00040000	/* I/O to user memory. */
#define	B_RAW		0x00080000	/* Set by physio for raw transfers. */
#define	B_READ		0x00100000	/* Read buffer. */
#define	B_TAPE		0x00200000	/* Magnetic tape I/O. */
#define	B_UAREA		0x00400000	/* Buffer describes Uarea I/O. */
#define	B_WANTED	0x00800000	/* Process wants this buffer. */
#define	B_WRITE		0x00000000	/* Write buffer (pseudo flag). */
#define	B_WRITEINPROG	0x01000000	/* Write in progress. */
#define	B_XXX		0x02000000	/* Debugging flag. */
#define	B_VFLUSH	0x04000000	/* Buffer is being synced. */

/*
 * Zero out the buffer's data area.
 */
#define	clrbuf(bp) {							\
	bzero((bp)->b_data, (unsigned int)(bp)->b_bcount);      	\
	(bp)->b_resid = 0;						\
}

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)
    
int bread(cyg_file * vp, cyg_int32 blkno, int size, struct buf ** bpp);
int breadn(cyg_file* vp, cyg_int32 blkno, int size, int* rablock, int* rasize, int, struct buf** bpp);    
int bwrite(struct buf* bp);

// no delayed writes
#define bdwrite(bp) bwrite(bp)
// no async writes
#define bawrite(bp) bwrite(bp)

// no async readhead (TODO IMPLEMENT)
#define breadn(vp, blkno, size, rablock, rasize, cnt, bpp) bread(vp,blkno,size,bpp)
void brelse(struct buf * bp);

#ifdef __cplusplus
};
#endif /* __cplusplus */


#endif // __BIO_H__
