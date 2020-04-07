/* Stolen from OpenBSD sources */
#ifndef ISO_H
#define ISO_H

#include <cyg/kernel/kapi.h>
#include <cyg/fileio/fileio.h>
#include "cd9660_support.h"

/*
 * Definitions describing ISO9660 file system structure, as well as
 * the functions necessary to access fields of ISO9660 file system
 * structures.
 */

#define ISODCL(from, to) (to - from + 1)

struct iso_volume_descriptor {
    char type[ISODCL(1,1)]; /* 711 */
    char id[ISODCL(2,6)];
    char version[ISODCL(7,7)];
    char data[ISODCL(8,2048)];
};

/* volume descriptor types */
#define ISO_VD_PRIMARY 1
#define ISO_VD_SUPPLEMENTARY 2
#define ISO_VD_END 255

#define ISO_STANDARD_ID "CD001"
#define ISO_ECMA_ID     "CDW01"

struct iso_primary_descriptor {
    char type			[ISODCL (  1,   1)]; /* 711 */
    char id				[ISODCL (  2,   6)];
    char version			[ISODCL (  7,   7)]; /* 711 */
    char unused1			[ISODCL (  8,   8)];
    char system_id			[ISODCL (  9,  40)]; /* achars */
    char volume_id			[ISODCL ( 41,  72)]; /* dchars */
    char unused2			[ISODCL ( 73,  80)];
    char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
    char unused3			[ISODCL ( 89, 120)];
    char volume_set_size		[ISODCL (121, 124)]; /* 723 */
    char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
    char logical_block_size		[ISODCL (129, 132)]; /* 723 */
    char path_table_size		[ISODCL (133, 140)]; /* 733 */
    char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
    char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
    char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
    char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
    char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
    char volume_set_id		[ISODCL (191, 318)]; /* dchars */
    char publisher_id		[ISODCL (319, 446)]; /* achars */
    char preparer_id		[ISODCL (447, 574)]; /* achars */
    char application_id		[ISODCL (575, 702)]; /* achars */
    char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
    char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
    char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
    char creation_date		[ISODCL (814, 830)]; /* 8.4.26.1 */
    char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
    char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
    char effective_date		[ISODCL (865, 881)]; /* 8.4.26.1 */
    char file_structure_version	[ISODCL (882, 882)]; /* 711 */
    char unused4			[ISODCL (883, 883)];
    char application_data		[ISODCL (884, 1395)];
    char unused5			[ISODCL (1396, 2048)];
};
#define ISO_DEFAULT_BLOCK_SHIFT		11
#define ISO_DEFAULT_BLOCK_SIZE		(1<<ISO_DEFAULT_BLOCK_SHIFT)

/*
 * Used by Microsoft Joliet extension to ISO9660. Almost the same
 * as PVD, but byte position 8 is a flag, and 89-120 is for escape.
 */

struct iso_supplementary_descriptor {
    char type			[ISODCL (  1,   1)]; /* 711 */
    char id				[ISODCL (  2,   6)];
    char version			[ISODCL (  7,   7)]; /* 711 */
    char flags			[ISODCL (  8,   8)];
    char system_id			[ISODCL (  9,  40)]; /* achars */
    char volume_id			[ISODCL ( 41,  72)]; /* dchars */
    char unused2			[ISODCL ( 73,  80)];
    char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
    char escape			[ISODCL ( 89, 120)];
    char volume_set_size		[ISODCL (121, 124)]; /* 723 */
    char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
    char logical_block_size		[ISODCL (129, 132)]; /* 723 */
    char path_table_size		[ISODCL (133, 140)]; /* 733 */
    char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
    char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
    char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
    char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
    char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
    char volume_set_id		[ISODCL (191, 318)]; /* dchars */
    char publisher_id		[ISODCL (319, 446)]; /* achars */
    char preparer_id		[ISODCL (447, 574)]; /* achars */
    char application_id		[ISODCL (575, 702)]; /* achars */
    char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
    char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
    char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
    char creation_date		[ISODCL (814, 830)]; /* 8.4.26.1 */
    char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
    char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
    char effective_date		[ISODCL (865, 881)]; /* 8.4.26.1 */
    char file_structure_version	[ISODCL (882, 882)]; /* 711 */
    char unused4			[ISODCL (883, 883)];
    char application_data		[ISODCL (884, 1395)];
    char unused5			[ISODCL (1396, 2048)];
};

struct iso_directory_record {
    char length			[ISODCL (1, 1)]; /* 711 */
    char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
    cyg_uint8 extent		[ISODCL (3, 10)]; /* 733 */
    cyg_uint8 size			[ISODCL (11, 18)]; /* 733 */
    char date			[ISODCL (19, 25)]; /* 7 by 711 */
    char flags			[ISODCL (26, 26)];
    char file_unit_size		[ISODCL (27, 27)]; /* 711 */
    char interleave			[ISODCL (28, 28)]; /* 711 */
    char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
    char name_len			[ISODCL (33, 33)]; /* 711 */
    char name			[1];			/* XXX */
};
/* can't take sizeof(iso_directory_record), because of possible alignment
   of the last entry (34 instead of 33) */
#define ISO_DIRECTORY_RECORD_SIZE	33

struct iso_extended_attributes {
    cyg_uint8 owner			[ISODCL (1, 4)]; /* 723 */
    cyg_uint8 group			[ISODCL (5, 8)]; /* 723 */
    cyg_uint8 perm			[ISODCL (9, 10)]; /* 9.5.3 */
    char ctime			[ISODCL (11, 27)]; /* 8.4.26.1 */
    char mtime			[ISODCL (28, 44)]; /* 8.4.26.1 */
    char xtime			[ISODCL (45, 61)]; /* 8.4.26.1 */
    char ftime			[ISODCL (62, 78)]; /* 8.4.26.1 */
    char recfmt			[ISODCL (79, 79)]; /* 711 */
    char recattr			[ISODCL (80, 80)]; /* 711 */
    cyg_uint8 reclen		[ISODCL (81, 84)]; /* 723 */
    char system_id			[ISODCL (85, 116)]; /* achars */
    char system_use			[ISODCL (117, 180)];
    char version			[ISODCL (181, 181)]; /* 711 */
    char len_esc			[ISODCL (182, 182)]; /* 711 */
    char reserved			[ISODCL (183, 246)];
    cyg_uint8 len_au		[ISODCL (247, 250)]; /* 723 */
};

static __inline int isonum_711(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_712(char *) __attribute__ ((unused));
static __inline int isonum_721(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_722(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_723(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_731(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_732(cyg_uint8 *) __attribute__ ((unused));
static __inline int isonum_733(cyg_uint8 *) __attribute__ ((unused));

/* 7.1.1: unsigned char */
static __inline int
isonum_711(cyg_uint8 *p)
{
    return *p;
}

/* 7.1.2: signed(?) char */
static __inline int
isonum_712(char *p)
{
    return *p;
}

/* 7.2.1: unsigned little-endian 16-bit value. */
static __inline int
isonum_721(cyg_uint8 *p)
{
    return *p|((char)p[1] << 8);
}

/* 7.2.2: unsigned big-endian 16-bit value. */
static __inline int     
isonum_722(unsigned char *p)
{
    return ((char)*p << 8)|p[1];
} 

/* 7.2.3: unsigned both-endian (little, then big) 16-bit value */
static __inline int
isonum_723(cyg_uint8 *p)
{
    return *p|(p[1] << 8);
}

/* 7.3.1: unsigned little-endian 32-bit value. */
static __inline int
isonum_731(cyg_uint8 *p)
{
    return *p|(p[1] << 8)|(p[2] << 16)|(p[3] << 24);
}

/* 7.3.2: unsigned big-endian 32-bit value. */
static __inline int
isonum_732(unsigned char *p)
{
    return (*p << 24)|(p[1] << 16)|(p[2] << 8)|p[3];
}

/* 7.3.3: unsigned both-endian (little, then big) 32-bit value */
static __inline int
isonum_733(cyg_uint8 *p)
{
    return *p|(p[1] << 8)|(p[2] << 16)|(p[3] << 24);
}

/*
 * Associated files have a leading '='.
 */
#define	ASSOCCHAR	'='

/* CD-ROM Format type */
enum ISO_FTYPE  { ISO_FTYPE_DEFAULT, ISO_FTYPE_9660, ISO_FTYPE_RRIP, ISO_FTYPE_ECMA };

#ifndef ISOFSMNT_ROOT
#define ISOFSMNT_ROOT   0
#endif
#define ISOFSMNT_NORRIP 0x00000001      /* disable Rock Ridge Ext.*/
#define ISOFSMNT_GENS   0x00000002      /* enable generation numbers */
#define ISOFSMNT_EXTATT 0x00000004      /* enable extended attributes */
#define ISOFSMNT_NOJOLIET 0x00000008    /* disable Joliet Ext.*/

struct iso_mnt {
    int im_flags;

    cyg_mtab_entry *im_mountp;
    cyg_file *im_devvp;

    int logical_block_size;
    int im_bshift;
    int im_bmask;

    int volume_space_size;

    char root[ISODCL (157, 190)];
    int root_extent;
    int root_size;
    enum ISO_FTYPE  iso_ftype;

    int rr_skip;
    int rr_skip0;

    int joliet_level;
};

/*
 * Theoretically, directories can be more than 2Gb in length,
 * however, in practice this seems unlikely. So, we define
 * the type doff_t as a long to keep down the cost of doing
 * lookup on a 32-bit machine. If you are porting to a 64-bit
 * architecture, you should make doff_t the same as off_t.
 */
#define doff_t	long

typedef	struct	{
    struct timespec	iso_atime;	/* time of last access */
    struct timespec	iso_mtime;	/* time of last modification */
    struct timespec	iso_ctime;	/* time file changed */
    cyg_uint16	        iso_mode;	/* files access mode and type */
    uid_t		iso_uid;	/* owner user id */
    gid_t		iso_gid;	/* owner group id */
    short		iso_links;	/* links of file */
    dev_t		iso_rdev;	/* Major/Minor number for special */
} ISO_RRIP_INODE;

struct iso_node {
    cyg_file   *i_vnode;	/* vnode associated with this inode */
    cyg_file   *i_devvp;	/* vnode for block I/O */
    cyg_uint32  i_flag;	        /* see below */
    ino_t	i_number;	/* the identity of the inode
				 * we use the actual starting block of the file */
    struct	iso_mnt *i_mnt;	/* filesystem associated with this inode */
    doff_t	i_endoff;	/* end of useful stuff in directory */
    doff_t	i_diroff;	/* offset in dir, where we found last entry */
    doff_t	i_offset;	/* offset of free space in directory */
    ino_t	i_ino;		/* inode number of found directory */

    long iso_extent;	        /* extent of file */
    long i_size;
    long iso_start;		/* actual start of data of file (may be different
				 * from iso_extent, if file has extended attributes) */
    ISO_RRIP_INODE  inode;
};

/* flags */
#define	IN_ACCESS	0x0020		/* inode access time to be updated */

#endif /* ISO_H */
