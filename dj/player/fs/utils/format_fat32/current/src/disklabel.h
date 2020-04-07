// disklabel.h
// stripped for my sanity


/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)disklabel.h	8.2 (Berkeley) 7/10/94
 * $FreeBSD: src/sys/sys/disklabel.h,v 1.49.2.7 2001/05/27 05:58:26 jkh Exp $
 */

#ifndef _SYS_DISKLABEL_H_
#define	_SYS_DISKLABEL_H_

#include <sys/types.h>


/*
 * Each disk has a label which includes information about the hardware
 * disk geometry, filesystem partitions, and drive specific information.
 * The label is in block 0 or 1, possibly offset from the beginning
 * to leave room for a bootstrap, etc.
 */
#define	MAXPARTITIONS	8


#define	LABEL_PART	2		/* partition containing label */
#define	RAW_PART	2		/* partition containing whole disk */
#define	SWAP_PART	1		/* partition normally containing swap */

struct disklabel {
	u_int32_t d_magic;		/* the magic number */
	u_int16_t d_type;		/* drive type */
	u_int16_t d_subtype;		/* controller/d_type specific */
	char	  d_typename[16];	/* type name, e.g. "eagle" */

	/* 
	 * d_packname contains the pack identifier and is returned when
	 * the disklabel is read off the disk or in-core copy.
	 * d_boot0 and d_boot1 are the (optional) names of the
	 * primary (block 0) and secondary (block 1-15) bootstraps
	 * as found in /boot.  These are returned when using
	 * getdiskbyname(3) to retrieve the values from /etc/disktab.
	 */
	union {
		char	un_d_packname[16];	/* pack identifier */
		struct {
			char *un_d_boot0;	/* primary bootstrap name */
			char *un_d_boot1;	/* secondary bootstrap name */
		} un_b;
	} d_un;
#define d_packname	d_un.un_d_packname
#define d_boot0		d_un.un_b.un_d_boot0
#define d_boot1		d_un.un_b.un_d_boot1

			/* disk geometry: */
	u_int32_t d_secsize;		/* # of bytes per sector */
	u_int32_t d_nsectors;		/* # of data sectors per track */
	u_int32_t d_ntracks;		/* # of tracks per cylinder */
	u_int32_t d_ncylinders;		/* # of data cylinders per unit */
	u_int32_t d_secpercyl;		/* # of data sectors per cylinder */
	u_int32_t d_secperunit;		/* # of data sectors per unit */

	/*
	 * Spares (bad sector replacements) below are not counted in
	 * d_nsectors or d_secpercyl.  Spare sectors are assumed to
	 * be physical sectors which occupy space at the end of each
	 * track and/or cylinder.
	 */
	u_int16_t d_sparespertrack;	/* # of spare sectors per track */
	u_int16_t d_sparespercyl;	/* # of spare sectors per cylinder */
	/*
	 * Alternate cylinders include maintenance, replacement, configuration
	 * description areas, etc.
	 */
	u_int32_t d_acylinders;		/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the
	 * formatter or controller when formatting.  When interleaving is
	 * in use, logically adjacent sectors are not physically
	 * contiguous, but instead are separated by some number of
	 * sectors.  It is specified as the ratio of physical sectors
	 * traversed per logical sector.  Thus an interleave of 1:1
	 * implies contiguous layout, while 2:1 implies that logical
	 * sector 0 is separated by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N relative to
	 * sector 0 on track N-1 on the same cylinder.  Finally, d_cylskew
	 * is the offset of sector 0 on cylinder N relative to sector 0
	 * on cylinder N-1.
	 */
	u_int16_t d_rpm;		/* rotational speed */
	u_int16_t d_interleave;		/* hardware sector interleave */
	u_int16_t d_trackskew;		/* sector 0 skew, per track */
	u_int16_t d_cylskew;		/* sector 0 skew, per cylinder */
	u_int32_t d_headswitch;		/* head switch time, usec */
	u_int32_t d_trkseek;		/* track-to-track seek, usec */
	u_int32_t d_flags;		/* generic flags */
#define NDDATA 5
	u_int32_t d_drivedata[NDDATA];	/* drive-type specific information */
#define NSPARE 5
	u_int32_t d_spare[NSPARE];	/* reserved for future use */
	u_int32_t d_magic2;		/* the magic number (again) */
	u_int16_t d_checksum;		/* xor of data incl. partitions */

			/* filesystem and partition information: */
	u_int16_t d_npartitions;	/* number of partitions in following */
	u_int32_t d_bbsize;		/* size of boot area at sn0, bytes */
	u_int32_t d_sbsize;		/* max size of fs superblock, bytes */
	struct	partition {		/* the partition table */
		u_int32_t p_size;	/* number of sectors in partition */
		u_int32_t p_offset;	/* starting sector */
		u_int32_t p_fsize;	/* filesystem basic fragment size */
		u_int8_t p_fstype;	/* filesystem type, see below */
		u_int8_t p_frag;	/* filesystem fragments per block */
		union {
			u_int16_t cpg;	/* UFS: FS cylinders per group */
			u_int16_t sgs;	/* LFS: FS segment shift */
		} __partition_u1;
#define	p_cpg	__partition_u1.cpg
#define	p_sgs	__partition_u1.sgs
	} d_partitions[MAXPARTITIONS];	/* actually may be more */
};

static u_int16_t dkcksum(struct disklabel *lp);

static __inline u_int16_t
dkcksum(struct disklabel *lp)
{
	u_int16_t *start, *end;
	u_int16_t sum = 0;

	start = (u_int16_t *)lp;
	end = (u_int16_t *)&lp->d_partitions[lp->d_npartitions];
	while (start < end)
		sum ^= *start++;
	return (sum);
}


/*
 * Structure used internally to retrieve information about a partition
 * on a disk.
 */
struct partinfo {
	struct disklabel *disklab;
	struct partition *part;
};

/* DOS partition table -- located in boot block */


#define DOSBBSECTOR	0	/* DOS boot block relative sector number */
#define DOSPARTOFF	446
#define NDOSPART	4
#define	DOSPTYP_386BSD	0xa5	/* 386BSD partition type */
#define	DOSPTYP_LINSWP	0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX	0x83	/* Linux partition */
#define	DOSPTYP_EXT	5	/* DOS extended partition */

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

#define DPSECT(s) ((s) & 0x3f)		/* isolate relevant bits of sector */
#define DPCYL(c, s) ((c) + (((s) & 0xc0)<<2)) /* and those that are cylinder */


#endif // _SYS_DISKLABEL_H_