/*
* Copyright (c) 1998 Robert Nordier
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided with the
*    distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <fs/utils/format_fat32/format.h>


#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <devs/storage/ata/atadrv.h>

/* drive structure stuff */
#include "disklabel.h"
#include "stat.h"

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



/* DOS partition table -- located in boot block */
#define DOSBBSECTOR	0	/* DOS boot block relative sector number */
#define DOSPARTOFF	446
#define NDOSPART	4
#define	DOSPTYP_386BSD	0xa5	/* 386BSD partition type */
#define	DOSPTYP_LINSWP	0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX	0x83	/* Linux partition */
#define	DOSPTYP_EXT	5	/* DOS extended partition */

#define DOSSECT(s,c) ((s & 0x3f) | ((c >> 2) & 0xc0))
#define DOSCYL(c)	(c & 0xff)

#define DPSECT(s) ((s) & 0x3f)		/* isolate relevant bits of sector */
#define DPCYL(c, s) ((c) + (((s) & 0xc0)<<2)) /* and those that are cylinder */

// magic from BSD sys/param.h
/* Macros for counting and rounding. */
#define howmany(x, y)   (((x)+((y)-1))/(y))
#define rounddown(x, y) (((x)/(y))*(y))
#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))  /* to any y */
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define powerof2(x)     ((((x)-1)&(x))==0)

/* Macros for min/max. */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


#define MAXU16	  0xffff	/* maximum unsigned 16-bit quantity */
#define BPN	  4		/* bits per nibble */
#define NPB	  2		/* nibbles per byte */

#define DOSMAGIC  0xaa55	/* DOS magic number */
#define MINBPS	  128		/* minimum bytes per sector */
#define MAXSPC	  128		/* maximum sectors per cluster */
#define MAXNFT	  16		/* maximum number of FATs */
#define DEFBLK	  4096		/* default block size */
#define DEFBLK16  2048		/* default block size FAT16 */
#define DEFRDE	  512		/* default root directory entries */
#define RESFTE	  2		/* reserved FAT entries */
#define MINCLS12  1		/* minimum FAT12 clusters */
#define MINCLS16  0x1000	/* minimum FAT16 clusters */
#define MINCLS32  2		/* minimum FAT32 clusters */
#define MAXCLS12  0xfed 	/* maximum FAT12 clusters */
#define MAXCLS16  0xfff5	/* maximum FAT16 clusters */
#define MAXCLS32  0xffffff5	/* maximum FAT32 clusters */



#define mincls(fat)  ((fat) == 12 ? MINCLS12 :	\
	(fat) == 16 ? MINCLS16 :	\
MINCLS32)

#define maxcls(fat)  ((fat) == 12 ? MAXCLS12 :	\
	(fat) == 16 ? MAXCLS16 :	\
MAXCLS32)

#define mk1(p, x)				\
(p) = (u_int8_t)(x)

#define mk2(p, x)				\
    (p)[0] = (u_int8_t)(x),			\
(p)[1] = (u_int8_t)((x) >> 010)

#define mk4(p, x)				\
    (p)[0] = (u_int8_t)(x),			\
    (p)[1] = (u_int8_t)((x) >> 010),		\
    (p)[2] = (u_int8_t)((x) >> 020),		\
(p)[3] = (u_int8_t)((x) >> 030)

#define argto1(arg, lo, msg)  argtou(arg, lo, 0xff, msg)
#define argto2(arg, lo, msg)  argtou(arg, lo, 0xffff, msg)
#define argto4(arg, lo, msg)  argtou(arg, lo, 0xffffffff, msg)
#define argtox(arg, lo, msg)  argtou(arg, lo, UINT_MAX, msg)

typedef struct seclook_t {
	unsigned long DiskSize;
	unsigned char SecPerClusVal;
} seclook;

int dos_cyls;
int dos_heads;
int dos_sectors;
int dos_cylsecs;

/*
*This is the table for FAT16 drives. NOTE that this table includes
* entries for disk sizes larger than 512 MB even though typically
* only the entries for disks < 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the DiskSize field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
* must be 2, and BPB_RootEntCnt must be 512. Any of these values
* being different may require the first table entries DiskSize value
* to be changed otherwise the cluster count may be to low for FAT16.
*/

seclook DskTableFAT16 [] = {
{ 8400, 0}, /* disks up to 4.1 MB, the 0 value for SecPerClusVal trips an error */
{ 32680, 2}, /* disks up to 16 MB, 1k cluster */
{ 262144, 4}, /* disks up to 128 MB, 2k cluster */
{ 524288, 8}, /* disks up to 256 MB, 4k cluster */
{ 1048576, 16}, /* disks up to 512 MB, 8k cluster */
/* The entries after this point are not used unless FAT16 is forced */
{ 2097152, 32}, /* disks up to 1 GB, 16k cluster */
{ 4194304, 64}, /* disks up to 2 GB, 32k cluster */
{ 0xFFFFFFFF, 0} /* any disk greater than 2GB, 0 value for SecPerClusVal trips an error */
};
/*
* This is the table for FAT32 drives. NOTE that this table includes
* entries for disk sizes smaller than 512 MB even though typically
* only the entries for disks >= 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the DiskSize field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
* must be 2. Any of these values being different may require the first
* table entries DiskSize value to be changed otherwise the cluster count
* may be to low for FAT32.
*/

seclook DskTableFAT32 [] = {
{ 66600, 0}, /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
{ 532480, 1}, /* disks up to 260 MB, .5k cluster */
{ 16777216, 8}, /* disks up to 8 GB, 4k cluster */
{ 33554432, 16}, /* disks up to 16 GB, 8k cluster */
{ 67108864, 32}, /* disks up to 32 GB, 16k cluster */
{ 0xFFFFFFFF, 64}/* disks greater than 32GB, 32k cluster */
};


struct bs {
    u_int8_t jmp[3];		/* bootstrap entry point */
    u_int8_t oem[8];		/* OEM name and version */
} __attribute__((packed));

struct bsbpb {
    u_int8_t bps[2];		/* bytes per sector */
    u_int8_t spc;		/* sectors per cluster */
    u_int8_t res[2];		/* reserved sectors */
    u_int8_t nft;		/* number of FATs */
    u_int8_t rde[2];		/* root directory entries */
    u_int8_t sec[2];		/* total sectors */
    u_int8_t mid;		/* media descriptor */
    u_int8_t spf[2];		/* sectors per FAT */
    u_int8_t spt[2];		/* sectors per track */
    u_int8_t hds[2];		/* drive heads */
    u_int8_t hid[4];		/* hidden sectors */
    u_int8_t bsec[4];		/* big total sectors */
} __attribute__((packed));

struct bsxbpb {
    u_int8_t bspf[4];		/* big sectors per FAT */
    u_int8_t xflg[2];		/* FAT control flags */
    u_int8_t vers[2];		/* file system version */
    u_int8_t rdcl[4];		/* root directory start cluster */
    u_int8_t infs[2];		/* file system info sector */
    u_int8_t bkbs[2];		/* backup boot sector */
    u_int8_t rsvd[12];		/* reserved */
} __attribute__((packed));

struct bsx {
    u_int8_t drv;		/* drive number */
    u_int8_t rsvd;		/* reserved */
    u_int8_t sig;		/* extended boot signature */
    u_int8_t volid[4];		/* volume ID number */
    u_int8_t label[11]; 	/* volume label */
    u_int8_t type[8];		/* file system type */
} __attribute__((packed));

struct de {
    u_int8_t namext[11];	/* name and extension */
    u_int8_t attr;		/* attributes */
    u_int8_t rsvd[10];		/* reserved */
    u_int8_t time[2];		/* creation time */
    u_int8_t date[2];		/* creation date */
    u_int8_t clus[2];		/* starting cluster */
    u_int8_t size[4];		/* size */
} __attribute__((packed));

struct bpb {
    u_int bps;			/* bytes per sector */
    u_int spc;			/* sectors per cluster */
    u_int res;			/* reserved sectors */
    u_int nft;			/* number of FATs */
    u_int rde;			/* root directory entries */
    u_int sec;			/* total sectors */
    u_int mid;			/* media descriptor */
    u_int spf;			/* sectors per FAT */
    u_int spt;			/* sectors per track */
    u_int hds;			/* drive heads */
    u_int hid;			/* hidden sectors */
    u_int bsec; 		/* big total sectors */
    u_int bspf; 		/* big sectors per FAT */
    u_int rdcl; 		/* root directory start cluster */
    u_int infs; 		/* file system info sector */
    u_int bkbs; 		/* backup boot sector */
} __attribute__((packed));

static struct {
    const char *name;
    struct bpb bpb;
} stdfmt[] = {
    {"160",  {512, 1, 1, 2,  64,  320, 0xfe, 1,  8, 1}},
    {"180",  {512, 1, 1, 2,  64,  360, 0xfc, 2,  9, 1}},
    {"320",  {512, 2, 1, 2, 112,  640, 0xff, 1,  8, 2}},
    {"360",  {512, 2, 1, 2, 112,  720, 0xfd, 2,  9, 2}},
    {"640",  {512, 2, 1, 2, 112, 1280, 0xfb, 2,  8, 2}},    
    {"720",  {512, 2, 1, 2, 112, 1440, 0xf9, 3,  9, 2}},
    {"1200", {512, 1, 1, 2, 224, 2400, 0xf9, 7, 15, 2}},
    {"1232", {1024,1, 1, 2, 192, 1232, 0xfe, 2,  8, 2}},    
    {"1440", {512, 1, 1, 2, 224, 2880, 0xf0, 9, 18, 2}},
    {"2880", {512, 2, 1, 2, 240, 5760, 0xf0, 9, 36, 2}}
};

static u_int8_t bootcode[] = {
    0xfa,			/* cli		    */
		0x31, 0xc0, 		/* xor	   ax,ax    */
		0x8e, 0xd0, 		/* mov	   ss,ax    */
		0xbc, 0x00, 0x7c,		/* mov	   sp,7c00h */
		0xfb,			/* sti		    */
		0x8e, 0xd8, 		/* mov	   ds,ax    */
		0xe8, 0x00, 0x00,		/* call    $ + 3    */
		0x5e,			/* pop	   si	    */
		0x83, 0xc6, 0x19,		/* add	   si,+19h  */
		0xbb, 0x07, 0x00,		/* mov	   bx,0007h */
		0xfc,			/* cld		    */
		0xac,			/* lodsb	    */
		0x84, 0xc0, 		/* test    al,al    */
		0x74, 0x06, 		/* jz	   $ + 8    */
		0xb4, 0x0e, 		/* mov	   ah,0eh   */
		0xcd, 0x10, 		/* int	   10h	    */
		0xeb, 0xf5, 		/* jmp	   $ - 9    */
		0x30, 0xe4, 		/* xor	   ah,ah    */
		0xcd, 0x16, 		/* int	   16h	    */
		0xcd, 0x19, 		/* int	   19h	    */
		0x0d, 0x0a,
		'N', 'o', 'n', '-', 's', 'y', 's', 't',
		'e', 'm', ' ', 'd', 'i', 's', 'k',
		0x0d, 0x0a,
		'P', 'r', 'e', 's', 's', ' ', 'a', 'n',
		'y', ' ', 'k', 'e', 'y', ' ', 't', 'o',
		' ', 'r', 'e', 'b', 'o', 'o', 't',
		0x0d, 0x0a,
		0
};


// magic from the internet
static u_int8_t partcode[] = {
0x00,0x33,0xC0,0x8E,0xD0,0xBC,0x00,0x7C,0x8B,0xF4,0x50,0x07,0x50,0x1F,0xFB,0xFC, 
0xBF,0x00,0x06,0xB9,0x00,0x01,0xF2,0xA5,0xEA,0x1D,0x06,0x00,0x00,0xBE,0xBE,0x07, 
0xB3,0x04,0x80,0x3C,0x80,0x74,0x0E,0x80,0x3C,0x00,0x75,0x1C,0x83,0xC6,0x10,0xFE, 
0xCB,0x75,0xEF,0xCD,0x18,0x8B,0x14,0x8B,0x4C,0x02,0x8B,0xEE,0x83,0xC6,0x10,0xFE, 
0xCB,0x74,0x1A,0x80,0x3C,0x00,0x74,0xF4,0xBE,0x8B,0x06,0xAC,0x3C,0x00,0x74,0x0B, 
0x56,0xBB,0x07,0x00,0xB4,0x0E,0xCD,0x10,0x5E,0xEB,0xF0,0xEB,0xFE,0xBF,0x05,0x00, 
0xBB,0x00,0x7C,0xB8,0x01,0x02,0x57,0xCD,0x13,0x5F,0x73,0x0C,0x33,0xC0,0xCD,0x13, 
0x4F,0x75,0xED,0xBE,0xA3,0x06,0xEB,0xD3,0xBE,0xC2,0x06,0xBF,0xFE,0x7D,0x81,0x3D, 
0x55,0xAA,0x75,0xC7,0x8B,0xF5,0xEA,0x00,0x7C,0x00,0x00,0x49,0x6E,0x76,0x61,0x6C, 
0x69,0x64,0x20,0x70,0x61,0x72,0x74,0x69,0x74,0x69,0x6F,0x6E,0x20,0x74,0x61,0x62, 
0x6C,0x65,0x00,0x45,0x72,0x72,0x6F,0x72,0x20,0x6C,0x6F,0x61,0x64,0x69,0x6E,0x67, 
0x20,0x6F,0x70,0x65,0x72,0x61,0x74,0x69,0x6E,0x67,0x20,0x73,0x79,0x73,0x74,0x65, 
0x6D,0x00,0x4D,0x69,0x73,0x73,0x69,0x6E,0x67,0x20,0x6F,0x70,0x65,0x72,0x61,0x74, 
0x69,0x6E,0x67,0x20,0x73,0x79,0x73,0x74,0x65,0x6D,0x00,0x00,0x80,0x45,0x14,0x15
};


static void check_mounted(const char *, mode_t);
static void getstdfmt(const char *, struct bpb *);
static void getdiskinfo(int, const char *, const char *, int,
						struct bpb *,struct dos_partition *);
static void print_bpb(struct bpb *);
static u_int ckgeom(const char *, u_int, const char *);
static u_int argtou(const char *, u_int, u_int, const char *);
static int oklabel(const char *);
static void mklabel(u_int8_t *, const char *);
static void setstr(u_int8_t *, const char *, size_t);
static void usage(void);
static void dospartfix(struct dos_partition *partp);



/***********************************************\
* Change real numbers into strange dos numbers	*
\***********************************************/
static void
dospartfix(struct dos_partition *partp)
{
	int cy, sec;
	u_int32_t end;

	/* Start c/h/s. */
	partp->dp_shd = partp->dp_start % dos_cylsecs / dos_sectors;
	cy = partp->dp_start / dos_cylsecs;
	sec = partp->dp_start % dos_sectors + 1;
	partp->dp_scyl = DOSCYL(cy);
	partp->dp_ssect = DOSSECT(sec, cy);

	/* End c/h/s. */
	end = partp->dp_start + partp->dp_size - 1;
	partp->dp_ehd = end % dos_cylsecs / dos_sectors;
	cy = end / dos_cylsecs;
	sec = end % dos_sectors + 1;
	partp->dp_ecyl = DOSCYL(cy);
	partp->dp_esect = DOSSECT(sec, cy);
}



/*
* Construct a FAT32 file system.
*/
int format_drive(char* fname,void (*statuscb)(int,int))
{
	
	int i;
	
    char buf[1024];
	char partbuff[512];
    struct stat sb;
    struct timeval tv;
    struct bpb bpb;
    struct tm *tm;
    struct bs *bs;
    struct bsbpb *bsbpb;
    struct bsxbpb *bsxbpb;
    struct bsx *bsx;
    struct de *de;
    u_int8_t *img;
    ssize_t n;
    time_t now;
    u_int fat, bss, rds, cls, dir, lsn, x, x1, x2;
    int ch, fd, fd1;
	struct dos_partition dospart;

	


	/* use devfs to gain access to the drive */
	mount("", "/dev", "devfs");
	fd = open(fname,O_RDWR);
	
	if(fd < 0)
	{
		diag_printf("error opening %s\n",fname);
		return -1;
	}
	
    // zero bpb
	memset(&bpb,0,sizeof(bpb));

	memset(&dospart,0,sizeof(dospart));

	// get drive info
	getdiskinfo(fd, fname, NULL, 0, &bpb, &dospart);
    
	if (!powerof2(bpb.bps))
	{
		diag_printf("error: bytes/sector (%u) is not a power of 2", bpb.bps);
		goto clean_error;
	}

    if (bpb.bps < MINBPS)
	{		
		diag_printf("error: bytes/sector (%u) is too small; minimum is %u",
		bpb.bps, MINBPS);
		goto clean_error;
	}
    

	// use partbuff to create sector 0
	memset(partbuff,0,512);

	// copy execution code to offset 00h
	memcpy(partbuff,partcode,sizeof(partcode));

	// copy in partition
	memcpy(partbuff + 0x1BE, &dospart, sizeof(dospart));

	// write signature	
	partbuff[0x1FE] = 0x55;
	partbuff[0x1FF] = 0xAA;


	lseek(fd,0,0);

	if ((n = write(fd, partbuff, bpb.bps)) == -1)
	{
		diag_printf("error writing partition table: %s\n", fname);
		goto clean_error;
	}
	

	// start normal format routine

	fat = 32;

    if (fat == 32)
		bpb.rde = 0;

    bss = 1; 
    fd1 = -1;
 
    if (!bpb.nft)
		bpb.nft = 2;

    x = bss;
    
	if (fat == 32) {
		if (!bpb.infs) {
			if (x == MAXU16 || x == bpb.bkbs)
			{
				diag_printf("error: no room for info sector\n");
				goto clean_error;
			}
			bpb.infs = x;
		}

		if (bpb.infs != MAXU16 && x <= bpb.infs)
			x = bpb.infs + 1;
		
		if (!bpb.bkbs) {
			
			bpb.bkbs = 6;
			x = bpb.bkbs + 1;
		}
    }

    if (!bpb.res)
		bpb.res = fat == 32 ? MAX(x, MAX(16384 / bpb.bps, 4)) : x;
    else if (bpb.res < x)
	{			
		diag_printf("error: too few reserved sectors\n");
		goto clean_error;
	}
    if (fat != 32 && !bpb.rde)
		bpb.rde = DEFRDE;
    rds = howmany(bpb.rde, bpb.bps / sizeof(struct de));

	// yikes! someone should be punished for this one
#if 0
	// espicially since it is wrong
    if (!bpb.spc)
		for (bpb.spc = howmany(fat == 16 ? DEFBLK16 : DEFBLK, bpb.bps);
			bpb.spc < MAXSPC &&
				bpb.res +
				howmany((RESFTE + maxcls(fat)) * (fat / BPN),
				bpb.bps * NPB) * bpb.nft +
				rds +
				(u_int64_t)(maxcls(fat) + 1) * bpb.spc <= bpb.bsec;
			bpb.spc <<= 1);

#endif

	i = 0;
	// last value will always be true
	while(1)
	{
		if(DskTableFAT32[i].DiskSize > bpb.bsec)
		{
			bpb.spc = DskTableFAT32[i].SecPerClusVal;
			break;
		}

		i++;
	}

	if(bpb.spc == 0)
	{
		diag_printf("invalid sector size\n");
		goto clean_error;
	}


	x1 = bpb.res + rds;
	x = bpb.bspf ? bpb.bspf : 1;
	if (x1 + (u_int64_t)x * bpb.nft > bpb.bsec)
	{
		diag_printf("error: meta data exceeds file system size\n");
		goto clean_error;
	}
	x1 += x * bpb.nft;
	x = (u_int64_t)(bpb.bsec - x1) * bpb.bps * NPB /
		(bpb.spc * bpb.bps * NPB + fat / BPN * bpb.nft);
	x2 = howmany((RESFTE + MIN(x, maxcls(fat))) * (fat / BPN),
		bpb.bps * NPB);
	if (!bpb.bspf) {
		bpb.bspf = x2;
		x1 += (bpb.bspf - 1) * bpb.nft;
	}
	cls = (bpb.bsec - x1) / bpb.spc;
	x = (u_int64_t)bpb.bspf * bpb.bps * NPB / (fat / BPN) - RESFTE;
	if (cls > x)
		cls = x;
	if (bpb.bspf < x2)
		diag_printf("warning: sectors/FAT limits file system to %u clusters",
		cls);
	if (cls < mincls(fat))
	{
		diag_printf("error: %u clusters too few clusters for FAT%u, need %u", cls, 
			fat,mincls(fat));
		goto clean_error;
	}

	if (cls > maxcls(fat)) {
		cls = maxcls(fat);
		bpb.bsec = x1 + (cls + 1) * bpb.spc - 1;
		diag_printf("warning: FAT type limits file system to %u sectors",
			bpb.bsec);
	}
	printf("%s: %u sector%s in %u FAT%u cluster%s "
		"(%u bytes/cluster)\n", fname, cls * bpb.spc,
		cls * bpb.spc == 1 ? "" : "s", cls, fat,
		cls == 1 ? "" : "s", bpb.bps * bpb.spc);
	if (!bpb.mid)
		bpb.mid = 0xf8; // not 0xf0
	if (fat == 32)
		bpb.rdcl = RESFTE;
	if (bpb.hid + bpb.bsec <= MAXU16) {
		bpb.sec = bpb.bsec;
		bpb.bsec = 0;
	}
	if (fat != 32) {
		bpb.spf = bpb.bspf;
		bpb.bspf = 0;
	}
	print_bpb(&bpb);
	
	if (!(img = malloc(bpb.bps)))
	{
		goto clean_error;
	}

	dir = bpb.res + (bpb.spf ? bpb.spf : bpb.bspf) * bpb.nft;

	for (lsn = 0; lsn < dir + (fat == 32 ? bpb.spc : rds); lsn++) 
	{
		if(statuscb != NULL)
			statuscb(lsn + 1,dir + (fat == 32 ? bpb.spc : rds));

		x = lsn;
	
		memset(img, 0, bpb.bps);
		
		if (!lsn ||
			(fat == 32 && bpb.bkbs != MAXU16 && lsn == bpb.bkbs)) 
		{
			x1 = sizeof(struct bs);
			bsbpb = (struct bsbpb *)(img + x1);
			mk2(bsbpb->bps, bpb.bps);
			mk1(bsbpb->spc, bpb.spc);
			mk2(bsbpb->res, bpb.res);
			mk1(bsbpb->nft, bpb.nft);
			mk2(bsbpb->rde, bpb.rde);
			mk2(bsbpb->sec, bpb.sec);
			mk1(bsbpb->mid, bpb.mid);
			mk2(bsbpb->spf, bpb.spf);
			mk2(bsbpb->spt, bpb.spt);
			mk2(bsbpb->hds, bpb.hds);
			mk4(bsbpb->hid, bpb.hid);
			mk4(bsbpb->bsec, bpb.bsec);
			x1 += sizeof(struct bsbpb);
			if (fat == 32) 
			{
				bsxbpb = (struct bsxbpb *)(img + x1);
				mk4(bsxbpb->bspf, bpb.bspf);
				mk2(bsxbpb->xflg, 0);
				mk2(bsxbpb->vers, 0);
 				mk4(bsxbpb->rdcl, bpb.rdcl);
				mk2(bsxbpb->infs, bpb.infs);
				mk2(bsxbpb->bkbs, bpb.bkbs);
				x1 += sizeof(struct bsxbpb);
			} 
			bsx = (struct bsx *)(img + x1);
			mk1(bsx->sig, 0x29);
	
			// FIXME: calculate a better volume ID here and label here
			// (or make them parameters)

			mk4(bsx->volid, 0x060679);
			mklabel(bsx->label, "NO NAME");

			sprintf(buf, "FAT%u", fat);
			setstr(bsx->type, buf, sizeof(bsx->type));
	
			x1 += sizeof(struct bsx);
			bs = (struct bs *)img;
			mk1(bs->jmp[0], 0xeb);
			mk1(bs->jmp[1], x1 - 2);
			mk1(bs->jmp[2], 0x90);
			setstr(bs->oem, "DHARMA10",sizeof(bs->oem));
			memcpy(img + x1, bootcode, sizeof(bootcode));
			mk2(img + bpb.bps - 2, DOSMAGIC);

		} else if (fat == 32 && bpb.infs != MAXU16 &&
			(lsn == bpb.infs ||
			(bpb.bkbs != MAXU16 &&
			lsn == bpb.bkbs + bpb.infs))) 
		{
			mk4(img, 0x41615252);
			mk4(img + bpb.bps - 28, 0x61417272);
			mk4(img + bpb.bps - 24, 0xffffffff);
			mk4(img + bpb.bps - 20, bpb.rdcl);
			mk2(img + bpb.bps - 2, DOSMAGIC);
		} else if (lsn >= bpb.res && lsn < dir &&
			!((lsn - bpb.res) %
			(bpb.spf ? bpb.spf : bpb.bspf))) 
		{
			mk1(img[0], bpb.mid);
			for (x = 1; x < fat * (fat == 32 ? 3 : 2) / 8; x++)
				mk1(img[x], fat == 32 && x % 4 == 3 ? 0x0f : 0xff);
		} 

		// seek manually to each sector

		// TODO: adjust here for partition-relative scheme

		lseek(fd,lsn + dospart.dp_start,0);

		if ((n = write(fd, img, bpb.bps)) == -1)
		{
			diag_printf("error: %s\n", fname);
			goto clean_error;
		}
		
		if (n != bpb.bps)
		{	
			diag_printf("error: %s: can't write sector %u\n", fname, lsn);
			goto clean_error;
		}
	}    
    
clean_error:

	close(fd);
	return 0;
}


/*
* Get a standard format.
*/
static void
getstdfmt(const char *fmt, struct bpb *bpb)
{
    u_int x, i;
	
    x = sizeof(stdfmt) / sizeof(stdfmt[0]);
    for (i = 0; i < x && strcmp(fmt, stdfmt[i].name); i++);
    if (i == x)
		diag_printf("error: %s: unknown standard format", fmt);
    *bpb = stdfmt[i].bpb;
}

/*
* Get disk slice, partition, and geometry information.
*/




static void
getdiskinfo(int fd, const char *fname, const char *dtype, int oflag,
			struct bpb *bpb,struct dos_partition *partp)
{
		
	cyg_io_handle_t blk_devH;
    cyg_uint32 len;
    drive_geometry_t dg;
    
    if (cyg_io_lookup(fname, &blk_devH) != ENOERR) 
	{
		diag_printf("Could not get handle to dev %s",fname);
		return;
    }

	diag_printf("got handle to %s\n", fname);
	len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) 
	{
		diag_printf("Could not power up device\n");
    }
	
	diag_printf("powered up %s\n", fname);

    len = sizeof(dg);
    
	if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) 
	{
		diag_printf("Could not get geometry\n");
		return;
    }

    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);
		
	// format the whole drive
	bpb->hid = 0;

	// fix this for (sector count - parition table)

	bpb->bps = ckgeom(fname, dg.bytes_p_sec, "bytes/sector");
	bpb->spt = ckgeom(fname, dg.sec, "sectors/track");
	bpb->hds = ckgeom(fname, dg.hd, "drive heads");
  
	bpb->bsec = dg.num_blks - bpb->spt;

	// generate partition entry for this crap

	// fill partition 0 structure at 1BEh for this drive
	partp->dp_typ = 0x0C;
	partp->dp_start = bpb->spt; // leave a track free
	partp->dp_size = bpb->bsec; // take the rest of the drive
  
	// for some stolen fdisk code
	dos_cyls = dg.cyl;
	dos_heads = dg.hd;
	dos_sectors = dg.sec;
	dos_cylsecs = dos_heads * dos_sectors;

	dospartfix(partp);

	return;		
}

/*
* Print out BPB values.
			*/
static void
print_bpb(struct bpb *bpb)
{
   diag_printf("bps=%u spc=%u res=%u nft=%u", bpb->bps, bpb->spc, bpb->res,
		bpb->nft);
    if (bpb->rde)
		printf(" rde=%u", bpb->rde);
    if (bpb->sec)
		printf(" sec=%u", bpb->sec);
   diag_printf(" mid=%x", bpb->mid);
    if (bpb->spf)
		printf(" spf=%u", bpb->spf);
   diag_printf(" spt=%u hds=%u hid=%u", bpb->spt, bpb->hds, bpb->hid);
    if (bpb->bsec)
		printf(" bsec=%u", bpb->bsec);
    if (!bpb->spf) {
		printf(" bspf=%u rdcl=%u", bpb->bspf, bpb->rdcl);
		printf(" infs=");
		printf(bpb->infs == MAXU16 ? "%#x" : "%u", bpb->infs);
		printf(" bkbs=");
		printf(bpb->bkbs == MAXU16 ? "%#x" : "%u", bpb->bkbs);
    }
   diag_printf("\n");
}

/*
* Check a disk geometry value.
*/
static u_int
ckgeom(const char *fname, u_int val, const char *msg)
{
    if (!val)
		diag_printf("error: %s: no default %s", fname, msg);
    if (val > MAXU16)
		diag_printf("error: %s: illegal %s", fname, msg);
    return val;
}

/*
* Convert and check a numeric option argument.
*/
static u_int
argtou(const char *arg, u_int lo, u_int hi, const char *msg)
{
    char *s;
    u_long x;
	
    errno = 0;
    x = strtoul(arg, &s, 0);
    if (errno || !*arg || *s || x < lo || x > hi)
		diag_printf("error: %s: bad %s", arg, msg);
    return x;
}

/*
* Check a volume label.
*/
static int
oklabel(const char *src)
{
    int c, i;
	
    for (i = 0; i <= 11; i++) {
		c = (u_char)*src++;
		if (c < ' ' + !i || strchr("\"*+,./:;<=>?[\\]|", c))
			break;
    }
    return i && !c;
}

/*
* Make a volume label.
*/
static void
mklabel(u_int8_t *dest, const char *src)
{
    int c, i;
	
    for (i = 0; i < 11; i++) {
		c = *src ? toupper(*src++) : ' ';
		*dest++ = !i && c == '\xe5' ? 5 : c;
    }
}

/*
* Copy string, padding with spaces.
*/
static void
setstr(u_int8_t *dest, const char *src, size_t len)
{
    while (len--)
		*dest++ = *src ? *src++ : ' ';
}

