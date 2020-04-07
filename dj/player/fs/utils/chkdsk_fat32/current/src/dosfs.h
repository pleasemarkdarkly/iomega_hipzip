/*
 * Copyright (C) 1995, 1996, 1997 Wolfgang Solfrank
 * Copyright (c) 1995 Martin Husemann
 * Some structure declaration borrowed from Paul Popelka
 * (paulp@uts.amdahl.com), see /sys/msdosfs/ for reference.
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
 *	This product includes software developed by Martin Husemann
 *	and Wolfgang Solfrank.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *	$NetBSD: dosfs.h,v 1.4 1997/01/03 14:32:48 ws Exp $
 * $FreeBSD: src/sbin/fsck_msdosfs/dosfs.h,v 1.1.2.1 2001/08/01 05:47:56 obrien Exp $
 */

#ifndef DOSFS_H
#define DOSFS_H

#define DOSBOOTBLOCKSIZE 512

typedef	u_int32_t	cl_t;	/* type holding a cluster number */


/* partition table entry format */
struct dos_partition {
	unsigned char	dp_flag;	/* bootstrap flags */
	unsigned char	dp_shd;		/* starting head */
	unsigned char	dp_ssect;	/* starting sector */
	unsigned char	dp_scyl;	/* starting cylinder */
	unsigned char	dp_typ;		/* partition type */
	unsigned char	dp_ehd;		/* end head */
	unsigned char	dp_esect;	/* end sector */
	unsigned char	dp_ecyl;	/* end cylinder */
	u_int32_t	dp_start;	/* absolute starting sector number */
	u_int32_t	dp_size;	/* partition size in sectors */
};

/*
 * architecture independent description of all the info stored in a
 * FAT boot block.
 */
struct bootblock {
	u_int	BytesPerSec;		/* bytes per sector */
	u_int	SecPerClust;		/* sectors per cluster */
	u_int	ResSectors;		/* number of reserved sectors */
	u_int	FATs;			/* number of FATs */
	u_int	RootDirEnts;		/* number of root directory entries */
	u_int	Media;			/* media descriptor */
	u_int	FATsmall;		/* number of sectors per FAT */
	u_int	SecPerTrack;		/* sectors per track */
	u_int	Heads;			/* number of heads */
	u_int32_t Sectors;		/* total number of sectors */
	u_int32_t HiddenSecs;		/* # of hidden sectors */
	u_int32_t HugeSectors;		/* # of sectors if bpbSectors == 0 */
	u_int	FSInfo;			/* FSInfo sector */
	u_int	Backup;			/* Backup of Bootblocks */
	cl_t	RootCl;			/* Start of Root Directory */
	cl_t	FSFree;			/* Number of free clusters acc. FSInfo */
	cl_t	FSNext;			/* Next free cluster acc. FSInfo */

	/* and some more calculated values */
	u_int	flags;			/* some flags: */
#define	FAT32		1		/* this is a FAT32 filesystem */
					/*
					 * Maybe, we should separate out
					 * various parts of FAT32?	XXX
					 */
	int	ValidFat;		/* valid fat if FAT32 non-mirrored */
	cl_t	ClustMask;		/* mask for entries in FAT */
	cl_t	NumClusters;		/* # of entries in a FAT */
	u_int32_t NumSectors;		/* how many sectors are there */
	u_int32_t FATsecs;		/* how many sectors are in FAT */
	u_int32_t NumFatEntries;	/* how many entries really are there */
	u_int	ClusterOffset;		/* at what sector would sector 0 start */
	u_int	ClusterSize;		/* Cluster size in bytes */

	/* Now some statistics: */
	u_int	NumFiles;		/* # of plain files */
	u_int	NumFree;		/* # of free clusters */
	u_int	NumBad;			/* # of bad clusters */
};

#if 0
// sweet jesus this needs to be smaller
struct fatEntry {
	u_int32_t	next;			/* pointer to next cluster */
	u_int32_t	head;			/* pointer to start of chain */
	u_int16_t length;		/* number of clusters on chain */
							/* 32k * 2^16 == 2GB, the max file size of fat */
	u_int8_t flags;			/* technically only one bit */
} __attribute__((packed));
#endif


// macros for accessing fatEntry easier
#define CLHEAD(p,i) p->chain[p->entry[i].headidx].head
#define CLLENGTH(p,i) p->chain[p->entry[i].headidx].length
#define CLNEXT(p,i) p->entry[i].next
#define CLHEADIDX(p,i) p->entry[i].headidx
#define CLFLAG(p,i) p->entry[i].flag

// for semi-portable passing
struct fatEntry {

	struct fatNextEntry *entry;
	struct fatChainInfo *chain;

};

// clustercount * 6
struct fatNextEntry {

	u_int32_t next;
    u_int16_t headidx:15,
              flag:1;     
    //	u_int16_t flghead; // bits 0-14 lookup, bit 15 is 'used' flag

};

// 32768 * 8
struct fatChainInfo {	
	u_int32_t head;
	u_int32_t length;
} __attribute__((packed));


#define MAX_CLUSTER_CHAINS 32768


#define	CLUST_FREE	0		/* 0 means cluster is free */
#define	CLUST_FIRST	2		/* 2 is the minimum valid cluster number */
#define	CLUST_RSRVD	0x0ffffff6	/* start of reserved clusters */
#define	CLUST_BAD	0x0ffffff7	/* a cluster with a defect */
#define	CLUST_EOFS	0x0ffffff8	/* start of EOF indicators */
#define	CLUST_EOF	0x0fffffff	/* standard value for last cluster */

/*
 * Masks for cluster values
 */
#define	CLUST12_MASK	0xfff
#define	CLUST16_MASK	0xffff
#define	CLUST32_MASK	0xfffffff

#define MAX_CLUST_PER_CHAIN 0x10000

#define	FAT_USED	0x1		/* This fat chain is used in a file */
#define FAT_UNUSED  0x0

#define	DOSLONGNAMELEN	256		/* long name maximal length */
#define LRFIRST		0x40		/* first long name record */
#define	LRNOMASK	0x1f		/* mask to extract long record
					 * sequence number */

/*
 * Architecture independent description of a directory entry
 */
#define SUPPORT_LONGFN
//#undef SUPPORT_LONGFN
struct dosDirEntry {
	struct dosDirEntry
		*parent,		/* previous tree level */
		*next,			/* next brother */
		*child;			/* if this is a directory */        // 12
#ifdef SUPPORT_LONGFN
    char lname[DOSLONGNAMELEN];	/* real name */             // 256 (!!!)
#endif
	cl_t head;			/* cluster no */                    //  4
	u_int32_t size;			/* filesize in bytes */         //  4
	char name[8+1+3+1];		/* alias name first part */     // 13 (!!!)
	unsigned char fsckflags;	/* flags during fsck */     //  1 - in the pad for above member
	unsigned char flags;        /* attributes */            //  1 - in the pad for above member
};
/* Flags in fsckflags: */
#define	DIREMPTY	1
#define	DIREMPWARN	2

/*
 *  TODO-list of unread directories
 */
struct dirTodoNode {
	struct dosDirEntry *dir;
	struct dirTodoNode *next;
};

#endif
