
/*
 * fdisk.c
 * ECOS BSD fdisk port
 * temancl@fullplaymedia.com
 *
 */


/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/* exports, some #defines */
#include <fs/fdisk/fdisk.h>

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <devs/storage/ata/atadrv.h>

/* drive structure stuff */
#include "disklabel.h"
#include "stat.h"


/* usual suspects with ECOS OS magic */
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int iotest;

#define LBUF 100
static char lbuf[LBUF];

#define MBRSIGOFF	510

/* 
 *
 * Ported to 386bsd by Julian Elischer  Thu Oct 15 20:26:46 PDT 1992
 *
 * 14-Dec-89  Robert Baron (rvb) at Carnegie-Mellon University
 *	Copyright (c) 1989	Robert. V. Baron
 *	Created.
 */

#define Decimal(str, ans, tmp) if (decimal(str, &tmp, ans)) ans = tmp
#define Hex(str, ans, tmp) if (hex(str, &tmp, ans)) ans = tmp
#define String(str, ans, len) {char *z = ans; char **dflt = &z; if (string(str, dflt)) strncpy(ans, *dflt, len); }

#define RoundCyl(x) ((((x) + cylsecs - 1) / cylsecs) * cylsecs)

#define MAX_SEC_SIZE 2048	/* maximum section size that is supported */
#define MIN_SEC_SIZE 512	/* the sector size to start sensing at */
int secsize = 0;		/* the sensed sector size */

const char *disk;
const char *disks[] =
{
  "/dev/ad0", "/dev/da0", 0
};

struct disklabel disklabel;		/* disk parameters */

int cyls, sectors, heads, cylsecs, disksecs;

struct mboot
{
	unsigned char padding[2]; /* force the longs to be long aligned */
  	unsigned char *bootinst;  /* boot code */
  	off_t bootinst_size;
	struct	dos_partition parts[4];
};
struct mboot mboot = {{0}, NULL, 0};

#define ACTIVE 0x80
#define BOOT_MAGIC 0xAA55

int dos_cyls;
int dos_heads;
int dos_sectors;
int dos_cylsecs;

#define DOSSECT(s,c) ((s & 0x3f) | ((c >> 2) & 0xc0))
#define DOSCYL(c)	(c & 0xff)
static int partition = -1;


#define MAX_ARGS	10

static int	current_line_number;

static int	geom_processed = 0;
static int	part_processed = 0;
static int	active_processed = 0;


typedef struct cmd {
    char		cmd;
    int			n_args;
    struct arg {
	char	argtype;
	int	arg_val;
    }			args[MAX_ARGS];
} CMD;


static int B_flag  = 0;		/* replace boot code */
static int I_flag  = 0;		/* use entire disk for FreeBSD */
static int a_flag  = 0;		/* set active partition */
static char *b_flag = NULL;	/* path to boot code */
static int i_flag  = 0;		/* replace partition data */
static int u_flag  = 0;		/* update partition data */
static int s_flag  = 0;		/* Print a summary and exit */
static int t_flag  = 1;		/* test only, if f_flag is given */
static char *f_flag = NULL;	/* Read config info from file */
static int v_flag  = 1;		/* Be verbose */

struct part_type
{
 unsigned char type;
 char *name;
}part_types[] =
{
	 {0x00, "unused"}
	,{0x01, "Primary DOS with 12 bit FAT"}
	,{0x02, "XENIX / filesystem"}
	,{0x03, "XENIX /usr filesystem"}
	,{0x04, "Primary DOS with 16 bit FAT (<= 32MB)"}
	,{0x05, "Extended DOS"}
	,{0x06, "Primary 'big' DOS (> 32MB)"}
	,{0x07, "OS/2 HPFS, NTFS, QNX-2 (16 bit) or Advanced UNIX"}
	,{0x08, "AIX filesystem"}
	,{0x09, "AIX boot partition or Coherent"}
	,{0x0A, "OS/2 Boot Manager or OPUS"}
	,{0x0B, "DOS or Windows 95 with 32 bit FAT"}
	,{0x0C, "DOS or Windows 95 with 32 bit FAT, LBA"}
	,{0x0E, "Primary 'big' DOS (> 32MB, LBA)"}
	,{0x0F, "Extended DOS, LBA"}
	,{0x10, "OPUS"}
	,{0x39, "plan9"}
	,{0x40, "VENIX 286"}
	,{0x4D, "QNX 4.2 Primary"}
	,{0x4E, "QNX 4.2 Secondary"}
	,{0x4F, "QNX 4.2 Tertiary"}
	,{0x50, "DM"}
	,{0x51, "DM"}
	,{0x52, "CP/M or Microport SysV/AT"}
	,{0x56, "GB"}
	,{0x61, "Speed"}
	,{0x63, "ISC UNIX, other System V/386, GNU HURD or Mach"}
	,{0x64, "Novell Netware 2.xx"}
	,{0x65, "Novell Netware 3.xx"}
	,{0x75, "PCIX"}
	,{0x80, "Minix 1.1 ... 1.4a"}
	,{0x81, "Minix 1.4b ... 1.5.10"}
	,{0x82, "Linux swap or Solaris x86"}
	,{0x83, "Linux filesystem"}
	,{0x93, "Amoeba filesystem"}
	,{0x94, "Amoeba bad block table"}
	,{0x9F, "BSD/OS"}
	,{0xA0, "Suspend to Disk"}
	,{0xA5, "FreeBSD/NetBSD/386BSD"}
	,{0xA6, "OpenBSD"}
	,{0xA7, "NEXTSTEP"}
	,{0xA9, "NetBSD"}
	,{0xB7, "BSDI BSD/386 filesystem"}
	,{0xB8, "BSDI BSD/386 swap"}
	,{0xDB, "Concurrent CPM or C.DOS or CTOS"}
	,{0xE1, "Speed"}
	,{0xE3, "Speed"}
	,{0xE4, "Speed"}
	,{0xF1, "Speed"}
	,{0xF2, "DOS 3.3+ Secondary"}
	,{0xF4, "Speed"}
	,{0xFF, "BBT (Bad Blocks Table)"}
};

static void print_s0(int which);
static void print_part(int i);
static void init_sector0(unsigned long start);
static void init_boot(void);
static void change_part(int i);
static void print_params();
static void change_active(int which);
static void change_code();
static void get_params_to_use();
static void dos(struct dos_partition *partp);
static int open_disk(int u_flag);
static ssize_t read_disk(off_t sector, void *buf);
static ssize_t write_disk(off_t sector, void *buf);
static int get_params();
static int read_s0();
static int write_s0();
static int ok(char *str);
static int decimal(char *str, int *num, int deflt);
static char *get_type(int type);
static int read_config(char *config_file);
static void reset_boot(void);
static int sanitize_partition(struct dos_partition *);
static void usage(void);
#if 0
static int hex(char *str, int *num, int deflt);
static int string(char *str, char **ans);
#endif

int d; /* drive handle */



int fdisk_drive(char* szdrive, int itype)
{
	int i;

	if(szdrive == NULL)
		return -1;

	disk = szdrive;
	
	if(open_disk(O_RDWR) < 0)
		return -1;


	/* (abu)use mboot.bootinst to probe for the sector size */
	if ((mboot.bootinst = malloc(MAX_SEC_SIZE)) == NULL)
		diag_printf( "cannot allocate buffer to determine disk sector size\n");
	read_disk(0, mboot.bootinst);
	free(mboot.bootinst);
	mboot.bootinst = NULL;

	if (s_flag)
	{
		int i;
		struct dos_partition *partp;

		if (read_s0())
			diag_printf( "read_s0\n");
		diag_printf("%s: %d cyl %d hd %d sec\n", disk, dos_cyls, dos_heads,
		    dos_sectors);
		diag_printf("Part  %11s %11s Type Flags\n", "Start", "Size\n");
		for (i = 0; i < NDOSPART; i++) {
			partp = ((struct dos_partition *) &mboot.parts) + i;
			if (partp->dp_start == 0 && partp->dp_size == 0)
				continue;
			diag_printf("%4d: %11lu %11lu 0x%02x 0x%02x\n", i + 1,
			    (u_long) partp->dp_start,
			    (u_long) partp->dp_size, partp->dp_typ,
			    partp->dp_flag);
		}
		
		return 0;
	}

	diag_printf("******* Working on device %s *******\n",disk);

	if (I_flag)
	{
		struct dos_partition *partp;

		read_s0();
		reset_boot();
		partp = (struct dos_partition *) (&mboot.parts[0]);
		partp->dp_typ = DOSPTYP_386BSD;
		partp->dp_flag = ACTIVE;
		partp->dp_start = dos_sectors;
		partp->dp_size = (disksecs / dos_cylsecs) * dos_cylsecs -
		    dos_sectors;

		dos(partp);
		if (v_flag)
			print_s0(-1);
		write_s0();
		return 0;
	}
	if (f_flag)
	{
	    if (read_s0() || i_flag)
	    {
		reset_boot();
	    }

	    if (!read_config(f_flag))
	    {
		return 1;
	    }
	    if (v_flag)
	    {
		print_s0(-1);
	    }
	    if (!t_flag)
	    {
		write_s0();
	    }
	}
	else
	{
	    if(u_flag)
	    {
		get_params_to_use();
	    }
	    else
	    {
		print_params();
	    }

	    if (read_s0())
		init_sector0(dos_sectors);

	    diag_printf("Media sector size is %d\n", secsize);
	    diag_printf("Warning: BIOS sector numbering starts with sector 1\n\n");
	    diag_printf("Information from DOS bootblock is:\n\n");
	    if (partition == -1)
		for (i = 1; i <= NDOSPART; i++)
		    change_part(i);
	    else
		change_part(partition);

	    if (u_flag || a_flag)
		change_active(partition);

	    if (B_flag)
		change_code();

	    if (u_flag || a_flag || B_flag) {
		if (!t_flag)
		{
		    diag_printf("\nWe haven't changed the partition table yet.  \n");
		    diag_printf("This is your last chance.\n");
		}
		print_s0(-1);
		if (!t_flag)
		{
		    if (ok("Should we write new partition table?"))
			write_s0();
		}
		else
		{
		    diag_printf("\n-t flag specified -- partition table not written.\n");
		}
	    }
	}

	return 0;
}

static void
usage()
{
	fprintf(stderr, "%s%s",
		"usage: fdisk [-BIaistu] [-b bootcode] [-1234] [disk]\n",
 		"       fdisk -f configfile [-itv] [disk]\n");
        return 1;
}

static void
print_s0(int which)
{
int	i;

	print_params();
	diag_printf("Information from DOS bootblock is:\n");
	if (which == -1)
		for (i = 1; i <= NDOSPART; i++)
			diag_printf("%d: ", i), print_part(i);
	else
		print_part(which);
}

static struct dos_partition mtpart = { 0 };

static void
print_part(int i)
{
	struct	  dos_partition *partp;
	u_int64_t part_mb;

	partp = ((struct dos_partition *) &mboot.parts) + i - 1;

	if (!memcmp(partp, &mtpart, sizeof (struct dos_partition))) {
		diag_printf("<UNUSED>\n");
		return;
	}
	/*
	 * Be careful not to overflow.
	 */
	part_mb = partp->dp_size;
	part_mb *= secsize;
	part_mb /= (1024 * 1024);
	diag_printf("sysid %d,(%s)\n", partp->dp_typ, get_type(partp->dp_typ));
	diag_printf("    start %lu, size %lu (%qd Meg), flag %x%s\n",
		(u_long)partp->dp_start,
		(u_long)partp->dp_size, 
		part_mb,
		partp->dp_flag,
		partp->dp_flag == ACTIVE ? " (active)" : "");
	diag_printf("\tbeg: cyl %d/ head %d/ sector %d;\n\tend: cyl %d/ head %d/ sector %d\n"
		,DPCYL(partp->dp_scyl, partp->dp_ssect)
		,partp->dp_shd
		,DPSECT(partp->dp_ssect)
		,DPCYL(partp->dp_ecyl, partp->dp_esect)
		,partp->dp_ehd
		,DPSECT(partp->dp_esect));
}


static void
init_boot(void)
{
	const char *fname;
	int fd, n;
	struct stat sb;

	fname = b_flag ? b_flag : "/boot/mbr";
	if ((fd = open(fname, O_RDONLY)) == -1 ||
	    fstat(fd, &sb) == -1)
		diag_printf( "%s", fname);
	if ((mboot.bootinst_size = sb.st_size) % secsize != 0)
		diag_printf( "%s: length must be a multiple of sector size", fname);
	if (mboot.bootinst != NULL)
		free(mboot.bootinst);
	if ((mboot.bootinst = malloc(mboot.bootinst_size = sb.st_size)) == NULL)
		diag_printf( "%s: unable to allocate read buffer", fname);
	if ((n = read(fd, mboot.bootinst, mboot.bootinst_size)) == -1 ||
	    close(fd))
		diag_printf( "%s", fname);
	if (n != mboot.bootinst_size)
		diag_printf( "%s: short read", fname);
}


static void
init_sector0(unsigned long start)
{
struct dos_partition *partp = (struct dos_partition *) (&mboot.parts[3]);

	init_boot();

	partp->dp_typ = DOSPTYP_386BSD;
	partp->dp_flag = ACTIVE;
	start = ((start + dos_sectors - 1) / dos_sectors) * dos_sectors;
	if(start == 0)
		start = dos_sectors;
	partp->dp_start = start;
	partp->dp_size = (disksecs / dos_cylsecs) * dos_cylsecs - start;

	dos(partp);
}

static void
change_part(int i)
{
struct dos_partition *partp = ((struct dos_partition *) &mboot.parts) + i - 1;

    diag_printf("The data for partition %d is:\n", i);
    print_part(i);

    if (u_flag && ok("Do you want to change it?")) {
	int tmp;

	if (i_flag) {
		bzero((char *)partp, sizeof (struct dos_partition));
		if (i == 4) {
			init_sector0(1);
			diag_printf("\nThe static data for the DOS partition 4 has been reinitialized to:\n");
			print_part(i);
		}
	}

	do {
		Decimal("sysid (165=FreeBSD)", partp->dp_typ, tmp);
		Decimal("start", partp->dp_start, tmp);
		Decimal("size", partp->dp_size, tmp);
		if (!sanitize_partition(partp)) {
			diag_printf("ERROR: failed to adjust; setting sysid to 0");
			partp->dp_typ = 0;
		}

		if (ok("Explicitly specify beg/end address ?"))
		{
			int	tsec,tcyl,thd;
			tcyl = DPCYL(partp->dp_scyl,partp->dp_ssect);
			thd = partp->dp_shd;
			tsec = DPSECT(partp->dp_ssect);
			Decimal("beginning cylinder", tcyl, tmp);
			Decimal("beginning head", thd, tmp);
			Decimal("beginning sector", tsec, tmp);
			partp->dp_scyl = DOSCYL(tcyl);
			partp->dp_ssect = DOSSECT(tsec,tcyl);
			partp->dp_shd = thd;

			tcyl = DPCYL(partp->dp_ecyl,partp->dp_esect);
			thd = partp->dp_ehd;
			tsec = DPSECT(partp->dp_esect);
			Decimal("ending cylinder", tcyl, tmp);
			Decimal("ending head", thd, tmp);
			Decimal("ending sector", tsec, tmp);
			partp->dp_ecyl = DOSCYL(tcyl);
			partp->dp_esect = DOSSECT(tsec,tcyl);
			partp->dp_ehd = thd;
		} else
			dos(partp);

		print_part(i);
	} while (!ok("Are we happy with this entry?"));
    }
}

static void
print_params()
{
	diag_printf("parameters extracted from in-core disklabel are:\n");
	diag_printf("cylinders=%d heads=%d sectors/track=%d (%d blks/cyl)\n\n"
			,cyls,heads,sectors,cylsecs);
	if((dos_sectors > 63) || (dos_cyls > 1023) || (dos_heads > 255))
		diag_printf("Figures below won't work with BIOS for partitions not in cyl 1\n");
	diag_printf("parameters to be used for BIOS calculations are:\n");
	diag_printf("cylinders=%d heads=%d sectors/track=%d (%d blks/cyl)\n\n"
		,dos_cyls,dos_heads,dos_sectors,dos_cylsecs);
}

static void
change_active(int which)
{
	struct dos_partition *partp = &mboot.parts[0];
	int active, i, new, tmp;

	active = -1;
	for (i = 0; i < NDOSPART; i++) {
		if ((partp[i].dp_flag & ACTIVE) == 0)
			continue;
		diag_printf("Partition %d is marked active\n", i + 1);
		if (active == -1)
			active = i + 1;
	}
	if (a_flag && which != -1)
		active = which;
	else if (active == -1)
		active = 1;

	if (!ok("Do you want to change the active partition?"))
		return;
setactive:
	do {
		new = active;
		Decimal("active partition", new, tmp);
		if (new < 1 || new > 4) {
			diag_printf("Active partition number must be in range 1-4."
					"  Try again.\n");
			goto setactive;
		}
		active = new;
	} while (!ok("Are you happy with this choice"));
	for (i = 0; i < NDOSPART; i++)
		partp[i].dp_flag = 0;
	if (active > 0 && active <= NDOSPART)
		partp[active-1].dp_flag = ACTIVE;
}

static void
change_code()
{
	if (ok("Do you want to change the boot code?"))
		init_boot();
}

void
get_params_to_use()
{
	int	tmp;
	print_params();
	if (ok("Do you want to change our idea of what BIOS thinks ?"))
	{
		do
		{
			Decimal("BIOS's idea of #cylinders", dos_cyls, tmp);
			Decimal("BIOS's idea of #heads", dos_heads, tmp);
			Decimal("BIOS's idea of #sectors", dos_sectors, tmp);
			dos_cylsecs = dos_heads * dos_sectors;
			print_params();
		}
		while(!ok("Are you happy with this choice"));
	}
}


/***********************************************\
* Change real numbers into strange dos numbers	*
\***********************************************/
static void
dos(partp)
	struct dos_partition *partp;
{
	int cy, sec;
	u_int32_t end;

	if (partp->dp_typ == 0 && partp->dp_start == 0 && partp->dp_size == 0) {
		memcpy(partp, &mtpart, sizeof(*partp));
		return;
	}

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

int fd;

	/* Getting device status */

static int
open_disk(int u_flag)
{

	/* use devfs to gain access to the drive */
	mount("", "/dev", "devfs");
	fd = open(disk,u_flag);

	if(fd < 0)
	{
		diag_printf("error opening %s\n",disk);
		return -1;
	}


	if (get_params(0) == -1) {
		diag_printf("can't get disk parameters on %s", disk);
		return -1;
	}
	return fd;
}

static ssize_t
read_disk(off_t sector, void *buf)
{
	lseek(fd,(sector * 512), 0);
	if( secsize == 0 )
		for( secsize = MIN_SEC_SIZE; secsize <= MAX_SEC_SIZE; secsize *= 2 )
			{
			/* try the read */
			int size = read(fd, buf, secsize);
			if( size == secsize )
				/* it worked so return */
				return secsize;
			}
	else
		return read( fd, buf, secsize );

	/* we failed to read at any of the sizes */
	return -1;
}

static ssize_t
write_disk(off_t sector, void *buf)
{
#if 0
	lseek(fd,(sector * 512), 0);
	/* write out in the size that the read_disk found worked */
	return write(fd, buf, secsize);
#endif
	diag_printf("write at sector %d disabled\n",sector);
	return 0;
}

static int
get_params()
{


    cyg_io_handle_t blk_devH;
    cyg_uint32 len;
    drive_geometry_t dg;
    
    if (cyg_io_lookup(disk, &blk_devH) != ENOERR) {
	diag_printf("Could not get handle to dev %s",disk);
	return -1;
    }
    diag_printf("got handle to %s\n", disk);
    len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device\n");
    }
    diag_printf("powered up %s\n", disk);

    len = sizeof(dg);
    if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
	diag_printf("Could not get geometry");
	return -1;
    }

    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);
		
	dos_cyls = cyls = dg.cyl;
	dos_heads = heads = dg.hd;
	dos_sectors = sectors = dg.num_blks;
	dos_cylsecs = cylsecs = heads * sectors;
	disksecs = cyls * heads * sectors;

    return (disksecs);
}


static int
read_s0() 
{
	mboot.bootinst_size = secsize;
	if (mboot.bootinst != NULL)
		free(mboot.bootinst);
	if ((mboot.bootinst = malloc(mboot.bootinst_size)) == NULL) {
		diag_printf("unable to allocate buffer to read fdisk "
		      "partition table");
		return -1;
	}
	if (read_disk(0, mboot.bootinst) == -1) {
		diag_printf("can't read fdisk partition table");
		return -1;
	}
	if (*(uint16_t *)&mboot.bootinst[MBRSIGOFF] != BOOT_MAGIC) {
		diag_printf("invalid fdisk partition table found");
		/* So should we initialize things */
		return -1;
	}
	memcpy(mboot.parts, &mboot.bootinst[DOSPARTOFF], sizeof(mboot.parts));
	return 0;
}

static int
write_s0()
{
#ifdef NOT_NOW
	int	flag;
#endif
	int	sector;

	if (iotest) {
		print_s0(-1);
		return 0;
	}
	
	memcpy(&mboot.bootinst[DOSPARTOFF], mboot.parts, sizeof(mboot.parts));
	
	/*
	 * write enable label sector before write (if necessary),
	 * disable after writing.
	 * needed if the disklabel protected area also protects
	 * sector 0. (e.g. empty disk)
	 */

	for(sector = 0; sector < mboot.bootinst_size / secsize; sector++) 
		if (write_disk(sector,
			       &mboot.bootinst[sector * secsize]) == -1) {
			diag_printf("can't write fdisk partition table");
			return -1;
		}

	return(0);
}


static int
ok(str)
char *str;
{
	/* no input method right now */
	diag_printf("not ok\n");
	return 0;
}

static int
decimal(char *str, int *num, int deflt)
{

	diag_printf("no decimal\n");
	return 0;
}


static int
hex(char *str, int *num, int deflt)
{
	diag_printf("no hex\n");
	return 0;
}

static int
string(char *str, char **ans)
{
	diag_printf("no string\n");
	return 0;
}

static char *
get_type(int type)
{
	int	numentries = (sizeof(part_types)/sizeof(struct part_type));
	int	counter = 0;
	struct	part_type *ptr = part_types;


	while(counter < numentries)
	{
		if(ptr->type == type)
		{
			return(ptr->name);
		}
		ptr++;
		counter++;
	}
	return("unknown");
}


static void
parse_config_line(line, command)
    char	*line;
    CMD		*command;
{
    char	*cp, *end;

    cp = line;
    while (1)	/* dirty trick used to insure one exit point for this
		   function */
    {
	memset(command, 0, sizeof(*command));

	while (isspace(*cp)) ++cp;
	if (*cp == '\0' || *cp == '#')
	{
	    break;
	}
	command->cmd = *cp++;

	/*
	 * Parse args
	 */
	while (1)
	{
	    while (isspace(*cp)) ++cp;
	    if (*cp == '#')
	    {
		break;		/* found comment */
	    }
	    if (isalpha(*cp))
	    {
		command->args[command->n_args].argtype = *cp++;
	    }
	    if (!isdigit(*cp))
	    {
		break;		/* assume end of line */
	    }
	    end = NULL;
	    command->args[command->n_args].arg_val = strtol(cp, &end, 0);
	    if (cp == end)
	    {
		break;		/* couldn't parse number */
	    }
	    cp = end;
	    command->n_args++;
	}
	break;
    }
}


static int
process_geometry(command)
    CMD		*command;
{
    int		status = 1, i;

    while (1)
    {
	geom_processed = 1;
	if (part_processed)
	{
	    diag_printf(
	"ERROR line %d: the geometry specification line must occur before\n\
    all partition specifications",
		    current_line_number);
	    status = 0;
	    break;
	}
	if (command->n_args != 3)
	{
	    diag_printf("ERROR line %d: incorrect number of geometry args",
		    current_line_number);
	    status = 0;
	    break;
	}
	dos_cyls = -1;
	dos_heads = -1;
	dos_sectors = -1;
	for (i = 0; i < 3; ++i)
	{
	    switch (command->args[i].argtype)
	    {
	    case 'c':
		dos_cyls = command->args[i].arg_val;
		break;
	    case 'h':
		dos_heads = command->args[i].arg_val;
		break;
	    case 's':
		dos_sectors = command->args[i].arg_val;
		break;
	    default:
		diag_printf(
		"ERROR line %d: unknown geometry arg type: '%c' (0x%02x)",
			current_line_number, command->args[i].argtype,
			command->args[i].argtype);
		status = 0;
		break;
	    }
	}
	if (status == 0)
	{
	    break;
	}

	dos_cylsecs = dos_heads * dos_sectors;

	/*
	 * Do sanity checks on parameter values
	 */
	if (dos_cyls < 0)
	{
	    diag_printf("ERROR line %d: number of cylinders not specified",
		    current_line_number);
	    status = 0;
	}
	if (dos_cyls == 0 || dos_cyls > 1024)
	{
	    diag_printf(
	"WARNING line %d: number of cylinders (%d) may be out-of-range\n\
    (must be within 1-1024 for normal BIOS operation, unless the entire disk\n\
    is dedicated to FreeBSD)",
		    current_line_number, dos_cyls);
	}

	if (dos_heads < 0)
	{
	    diag_printf("ERROR line %d: number of heads not specified",
		    current_line_number);
	    status = 0;
	}
	else if (dos_heads < 1 || dos_heads > 256)
	{
	    diag_printf("ERROR line %d: number of heads must be within (1-256)",
		    current_line_number);
	    status = 0;
	}

	if (dos_sectors < 0)
	{
	    diag_printf("ERROR line %d: number of sectors not specified",
		    current_line_number);
	    status = 0;
	}
	else if (dos_sectors < 1 || dos_sectors > 63)
	{
	    diag_printf("ERROR line %d: number of sectors must be within (1-63)",
		    current_line_number);
	    status = 0;
	}

	break;
    }
    return (status);
}


static int
process_partition(command)
    CMD		*command;
{
    int				status = 0, partition;
    u_int32_t			prev_head_boundary, prev_cyl_boundary;
    u_int32_t			adj_size, max_end;
    struct dos_partition	*partp;

    while (1)
    {
	part_processed = 1;
	if (command->n_args != 4)
	{
	    diag_printf("ERROR line %d: incorrect number of partition args",
		    current_line_number);
	    break;
	}
	partition = command->args[0].arg_val;
	if (partition < 1 || partition > 4)
	{
	    diag_printf("ERROR line %d: invalid partition number %d",
		    current_line_number, partition);
	    break;
	}
	partp = ((struct dos_partition *) &mboot.parts) + partition - 1;
	bzero((char *)partp, sizeof (struct dos_partition));
	partp->dp_typ = command->args[1].arg_val;
	partp->dp_start = command->args[2].arg_val;
	partp->dp_size = command->args[3].arg_val;
	max_end = partp->dp_start + partp->dp_size;

	if (partp->dp_typ == 0)
	{
	    /*
	     * Get out, the partition is marked as unused.
	     */
	    /*
	     * Insure that it's unused.
	     */
	    bzero((char *)partp, sizeof (struct dos_partition));
	    status = 1;
	    break;
	}

	/*
	 * Adjust start upwards, if necessary, to fall on an head boundary.
	 */
	if (partp->dp_start % dos_sectors != 0)
	{
	    prev_head_boundary = partp->dp_start / dos_sectors * dos_sectors;
	    if (max_end < dos_sectors ||
		prev_head_boundary > max_end - dos_sectors)
	    {
		/*
		 * Can't go past end of partition
		 */
		diag_printf(
	"ERROR line %d: unable to adjust start of partition %d to fall on\n\
    a head boundary",
			current_line_number, partition);
		break;
	    }
	    diag_printf(
	"WARNING: adjusting start offset of partition %d\n\
    from %u to %u, to fall on a head boundary",
		    partition, (u_int)partp->dp_start,
		    (u_int)(prev_head_boundary + dos_sectors));
	    partp->dp_start = prev_head_boundary + dos_sectors;
	}

	/*
	 * Adjust size downwards, if necessary, to fall on a cylinder
	 * boundary.
	 */
	prev_cyl_boundary =
	    ((partp->dp_start + partp->dp_size) / dos_cylsecs) * dos_cylsecs;
	if (prev_cyl_boundary > partp->dp_start)
	    adj_size = prev_cyl_boundary - partp->dp_start;
	else
	{
	    diag_printf(
	"ERROR: could not adjust partition to start on a head boundary\n\
    and end on a cylinder boundary.");
	    return (0);
	}
	if (adj_size != partp->dp_size)
	{
	    diag_printf(
	"WARNING: adjusting size of partition %d from %u to %u\n\
    to end on a cylinder boundary",
		    partition, (u_int)partp->dp_size, (u_int)adj_size);
	    partp->dp_size = adj_size;
	}
	if (partp->dp_size == 0)
	{
	    diag_printf("ERROR line %d: size of partition %d is zero",
		    current_line_number, partition);
	    break;
	}

	dos(partp);
	status = 1;
	break;
    }
    return (status);
}


static int
process_active(command)
    CMD		*command;
{
    int				status = 0, partition, i;
    struct dos_partition	*partp;

    while (1)
    {
	active_processed = 1;
	if (command->n_args != 1)
	{
	    diag_printf("ERROR line %d: incorrect number of active args",
		    current_line_number);
	    status = 0;
	    break;
	}
	partition = command->args[0].arg_val;
	if (partition < 1 || partition > 4)
	{
	    diag_printf("ERROR line %d: invalid partition number %d",
		    current_line_number, partition);
	    break;
	}
	/*
	 * Reset active partition
	 */
	partp = ((struct dos_partition *) &mboot.parts);
	for (i = 0; i < NDOSPART; i++)
	    partp[i].dp_flag = 0;
	partp[partition-1].dp_flag = ACTIVE;

	status = 1;
	break;
    }
    return (status);
}


static int
process_line(line)
    char	*line;
{
    CMD		command;
    int		status = 1;

    while (1)
    {
	parse_config_line(line, &command);
	switch (command.cmd)
	{
	case 0:
	    /*
	     * Comment or blank line
	     */
	    break;
	case 'g':
	    /*
	     * Set geometry
	     */
	    status = process_geometry(&command);
	    break;
	case 'p':
	    status = process_partition(&command);
	    break;
	case 'a':
	    status = process_active(&command);
	    break;
	default:
	    status = 0;
	    break;
	}
	break;
    }
    return (status);
}


static int
read_config(config_file)
    char *config_file;
{
	diag_printf("no config\n");
    return 0;
}


static void
reset_boot(void)
{
    int				i;
    struct dos_partition	*partp;

    init_boot();
    for (i = 0; i < 4; ++i)
    {
	partp = ((struct dos_partition *) &mboot.parts) + i;
	bzero((char *)partp, sizeof (struct dos_partition));
    }
}

static int
sanitize_partition(partp)
    struct dos_partition	*partp;
{
    u_int32_t			prev_head_boundary, prev_cyl_boundary;
    u_int32_t			max_end, size, start;

    start = partp->dp_start;
    size = partp->dp_size;
    max_end = start + size;
    /* Only allow a zero size if the partition is being marked unused. */
    if (size == 0) {
	if (start == 0 && partp->dp_typ == 0)
	    return (1);
	diag_printf("ERROR: size of partition is zero");
	return (0);
    }
    /* Return if no adjustment is necessary. */
    if (start % dos_sectors == 0 && (start + size) % dos_sectors == 0)
	return (1);

    if (start % dos_sectors != 0)
	diag_printf("WARNING: partition does not start on a head boundary");
    if ((start  +size) % dos_sectors != 0)
	diag_printf("WARNING: partition does not end on a cylinder boundary");
    diag_printf("WARNING: this may confuse the BIOS or some operating systems");
    if (!ok("Correct this automatically?"))
	return (1);

    /*
     * Adjust start upwards, if necessary, to fall on an head boundary.
     */
    if (start % dos_sectors != 0) {
	prev_head_boundary = start / dos_sectors * dos_sectors;
	if (max_end < dos_sectors ||
	    prev_head_boundary >= max_end - dos_sectors) {
	    /*
	     * Can't go past end of partition
	     */
	    diag_printf(
    "ERROR: unable to adjust start of partition to fall on a head boundary");
	    return (0);
        }
	start = prev_head_boundary + dos_sectors;
    }

    /*
     * Adjust size downwards, if necessary, to fall on a cylinder
     * boundary.
     */
    prev_cyl_boundary = ((start + size) / dos_cylsecs) * dos_cylsecs;
    if (prev_cyl_boundary > start)
	size = prev_cyl_boundary - start;
    else {
	diag_printf("ERROR: could not adjust partition to start on a head boundary\n\
    and end on a cylinder boundary.");
	return (0);
    }

    /* Finally, commit any changes to partp and return. */
    if (start != partp->dp_start) {
	diag_printf("WARNING: adjusting start offset of partition to %u",
	    (u_int)start);
	partp->dp_start = start;
    }
    if (size != partp->dp_size) {
	diag_printf("WARNING: adjusting size of partition to %u", (u_int)size);
	partp->dp_size = size;
    }

    return (1);
}

