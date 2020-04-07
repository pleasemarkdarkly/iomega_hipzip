//........................................................................................
//........................................................................................
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 10/20/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*****************************************************************************
* FileName: PCDISK.H - Defines & structures for File System.
*                     
* SanDisk Host Developer's Toolkit
*
* Copyright (c) 1996-1999 SanDisk Corporation
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description: 
*       File System structures and function prototypes  
*
****************************************************************************/

#ifndef __PCDISK__


#ifdef __cplusplus
extern "C" {
#endif



/* Include HDTK API interface */
#include <fs/fat/sdapi.h>
    
#if (USE_FILE_SYSTEM)

/* Number of drives to support A:,B:,C: .... */
#define NDRIVES   TOTAL_DRIVES


/* Directory Object Needs. Conservative guess is One CWD per user 
** per drive + One per file + One per User for directory traversal.
** Note that this is calculated... do not change.
*/
/* NOT intended to be CHANGED by user */
#define NFINODES        (NUM_USERS*NDRIVES + NUM_USERS + NUSERFILES) 
#define NDROBJS         (NUM_USERS*NDRIVES + NUM_USERS + NUSERFILES)


#define DRIVE_INSTALLED 0x8000


/* Formatted Drive FAT type */
#define DRV_FAT12       0x0003
#define DRV_FAT16       0x0004
#define DRV_FAT32       0x0008


/* Type of operating system */
#define DOS_TYPE        0
#define WIN95_TYPE      1
#define WIN98_TYPE      2

#define BOOTABLE_SIGNATURE      0x80
#define LEGAL_SIGNATURE         0xAA55

/* Structure used to track cached fat blocks */
typedef struct fatswap
{
        UCHAR   *data_array;    /* Block buffer area */
	UINT32  current_access; /* Next to swap. For implementing round robin */
				/* These two counters track cache usage as we fill it.
				   Eventually the FAT fills and we go into swapping mode 
				   at steady state.
				*/
	UINT32  n_blocks_total; /* How many blocks are available in the cache */
	UINT16  data_map[FAT_BUFFER_SIZE];
	UINT16  pdirty[FAT_BUFFER_SIZE];/* blocks needing flushing */
} FATSWAP;


/* Structure to contain block 0 image from the disk.
   This structure is initialized when  a drive is mounted.  It is 
   shared by all tasks.  It contains the location of FAT, the location
   of the root sector, the cluster size, the disk size and other drive
   geometry information.
*/
typedef struct ddrive {
	ULONG   volume_start;          /* Starting LBA of the volume */
	ULONG   volume_end;            /* Ending LBA of the volume */
	ULONG   volume_serialno;       /* Volume serial number block 0 */
        TEXT    volume_label[12];      /* Volume label */
	ULONG   byte_into_cl_mask;     /* And this with file pointer to get the
					  byte offset into the cluster */
	BLOCKT  rootblock;             /* First block of root dir */
	UINT32  prevCluster;           /* Previous cluster number */
	UINT32  currCluster;           /* Current cluster number */
	UINT32  nextCluster;           /* Next cluster number */
	UINT32  prevDirCluster;        /* Previous Dir cluster number */
	UINT32  currDirCluster;        /* Current Dir cluster number */
	UINT32  nextDirCluster;        /* Next Dir cluster number */
	BLOCKT  firstclblock;          /* First block of cluster area */
	BLOCKT  numsecs;               /* Total # sectors on the disk */
	BLOCKT  numhide;               /* # hidden sectors */
	UINT32  free_contig_base;      /* Guess of where file data would most */
	UINT32  free_contig_pointer;   /* Efficiently stored */  
	UINT32  known_free_clusters;   /* If non-zero pc_free may use this value */
	UINT32  maxfindex;             /* Last element in the fat */
	INT16   fasize;                /* Nibbles per fat entry. (2 or 4) */
	INT16   driveno;               /* Driveno. Set when open succeeds */
	UINT16  bytespcluster;         /* Bytes per cluster */
	UINT16  secproot;              /* blocks in root dir */
	UINT16  bytspsector;           /* Must be 512 for this implementation */
	UINT16  log2_secpalloc;        /* Log of sectors per cluster */
	UINT16  fatblock;              /* Reserved sectors before the FAT */
	UINT16  numroot;               /* Maximum # of root dir entries */
	UINT16  secpfat;               /* Size of each fat */
	UINT16  secptrk;               /* sectors per track */
	UINT16  numhead;               /* number of heads */
	UINT16  dev_flag;
	UINT16  enable_mapping;
	UINT16  fat_is_dirty;
	FATSWAP fat_swap_structure;    /* Fat swap structure if swapping */
	UINT16  secpalloc;             /* Sectors per cluster */
	UINT16  numfats;               /* Number of FATS on the disk */
	UINT16  mediadesc;             /* Media descriptor byte */
	UINT16  os_type;
} DDRIVE;


/* Dos Directory Entry Memory Image of Disk Entry.
   This structure is a template if directory blocks is scanned and
   a destination if directory entries are created.
*/
#define INOPBLOCK 16            /* 16 of these fit in a block */
typedef struct dosinode {
	UTEXT   fname[8];
	UTEXT   fext[3];
	UTINY   fattribute;     /* File attributes */
#if 0
	UINT16  resarea[3];
#else
    UTINY resarea0;
    UTINY ftime_tenths;
    UINT16 fcrttime;
    UINT16 fcrtdate;
#endif
	UINT16  lastAccess;     /* Last access date. For Windows */
	UINT16  fclusterHi;     /* High word of starting cluster. FAT32 */
	UINT16  ftime;          /* time & date creation or last modified */
	UINT16  fdate;
	UINT16  fcluster;       /* Starting cluster for data file */
	ULONG   fsize;          /* File size in bytes */
} DOSINODE;


#define LSEQUENCE_END           0x40    /* Long file name entry termination */
#define LFNLEN_PER_ENTRY        0x0D    /* 13 characters per long name entry */

/* Long File Name structure */
typedef struct longinode {
	UTINY   sequence;       /* Sequence byte: 1,2,3..., last entry is 0x40 */
	UTEXT   fname[10];      /* Unicode characters of name */
	UTINY   fattribute;     /* File attributes */
	UTINY   type;           /* Long entry type: 0 */
	UTINY   chksum;         /* Checksum for matching short name alias */
	UTEXT   fname2[12];     /* More unicode characters of name */
	UINT16  reserved;       /* Reserved */
	UTEXT   fname3[4];      /* More unicode characters of name */
} LONGINODE, *PLONGINODE;


/* 
   Internal representation of DOS entry.
   The first 8 fields !MUST! be identical to the DOSINODE structure.
   FINODE structure is shared by all tasks.  All directory and file
   access routines eventually access the FINODE structure.  The
   directory entry that a FINODE represents is uniquely determined
   by a combination of drive structure pointer, block number and
   block offset.  
*/
#define OF_WRITE            0x01/* File is open for write by someone */
#define OF_WRITEEXCLUSIVE   0x02/* File is open for write by someone 
				   they wish exclusive write access.
				*/ 
#define OF_EXCLUSIVE        0x04/* File is open with exclusive access not
				   sharing write or read.
				*/
typedef struct finode {
	UTEXT   fname[8];
	UTEXT   fext[3];
	UTINY   fattribute;     /* File attributes */
#if 0
	UINT16  resarea[3];
#else
    UTINY resarea0;
    UTINY ftime_tenths;
    UINT16 fcrttime;
    UINT16 fcrtdate;
#endif
	UINT16  lastAccess;     /* Last access date. For Windows */
	UINT16  fclusterHi;     /* High word of starting cluster. FAT32 */
	UINT16  ftime;          /* time & date lastmodified */
	UINT16  fdate;
	UINT16  fcluster;       /* Cluster for data file */
	ULONG   fsize;          /* File size */

	BLOCKT  my_block;
	ULONG   alloced_size;   /* Size rounded up to the hearest cluster 
				   (only maintained for files)
				*/
	LOCKOBJ lock_object;    /* for locking the finode */
	DDRIVE  *my_drive;
	struct  finode *pnext;  /* Next node */      
	struct  finode *pprev;  /* Previous node */

	COUNT   opencount;      /* If the finode is an open file the following
				   flags control the sharing. They are 
				   maintained by po__open.
				*/
	INT16   my_index;       /* index into directory block  */
	INT16   openflags;      /* For Files. Track how files have it open */
	INT16   padding;
} FINODE;


/* contain location information for a directory */
typedef struct dirblk {
	BLOCKT  my_frstblock; /* First block in this directory */
	BLOCKT  my_block;     /* Current block number */
	UINT16  my_index;     /* dir. entry number in my cluster */
} DIRBLK;



/* Structure used to reserve arrays of blocks */
typedef struct block_alloc {
	UINT16   core[256];
} BLOCK_ALLOC;


/* Block buffer.  This structure controls directory block buffering.
   It contains a 512-byte block buffer and control field. All 
   directory accesses are done through block buffers.  Block buffer
   are shared by all tasks in the system.
*/
typedef struct blkbuff {
	BLOCK_ALLOC *data;      /* Pointer to the buffer area */
	struct  blkbuff *pnext; /* Pointer to next block buffer */
	DDRIVE  *pdrive;        /* Pointer to Drive structure */
	BLOCKT  blockno;
	UINT32  lru;            /* Least recent used flag */
	INT16   use_count;
	INT16   padding;
} BLKBUFF;



/* Object used to find a directory entry on a disk and its parent's.
   This structure s used by tasks to manage access to directories.
   The DROBJ structure is private to the task that allocated it and
   is not shared in any way. The DROBJ structure is a fully self
   sufficient data structure for scanning operation.
*/
typedef struct drobj {
	DDRIVE  *pdrive;
	FINODE  *finode;
	BLKBUFF *pblkbuff;
	SDBOOL  isroot; /* True if this is the root */
	DIRBLK  blkinfo;
} DROBJ;


/* Structure for use to do recursive search for files in a selected drive */
typedef struct fsdstat {
	DSTAT   *fileinfo;
	DROBJ   *pobj;          /* Info for getting at the inode */
	DROBJ   *pmom;          /* Info for getting at parent inode */
} FSDSTAT;



/* Internal file representation */
typedef struct pc_file {
	DROBJ   *pobj;        /* Info for getting at the inode */
	ULONG   fptr;         /* Current file pointer */
	ULONG   fptr_block;   /* Block address at boundary of fprt_cluster */
	UINT32  fptr_cluster; /* Current cluster boundary for fptr */
	SDBOOL  needs_flush;  /* If YES this FILE must be flushed */
	SDBOOL  is_free;      /* If YES this FILE may be used (see pc_memry.c) */
	SDBOOL  at_eof;       /* True if fptr was > alloced size last time we
				 set it. If so synch_file pointers will fix
				 it if the file has expanded. */
	SDBOOL  setDate;      /* Update date. */
	UTINY  *dataBuff;
	UINT16  flag;         /* Acces flags from po_open(). */
} PC_FILE;


/* INTERNAL !! */
/* BPB for a FAT16 partition */
typedef struct _A_BF_BPB16 {
	/* This portion is the BPB (BIOS Parameter Block) */
	UINT16  bsBytesPerSector;       /* bytes per sector */
	UTINY   bsSectorsPerCluster;    /* sectors per cluster */
	UINT16  bsReservedSectors;      /* number of reserved sectors */
	UTINY   bsNumberOfFATs;         /* number of file allocation tables */
	UINT16  bsRootEntries;          /* number of root-directory entries */
	UINT16  bsTotalSectors;         /* total number of sectors */
	UTINY   bsMediaDescriptor;      /* media descriptor */
	UINT16  bsSectorsPerFAT;        /* number of sectors per FAT */
	UINT16  bsSectorsPerTrack;      /* number of sectors per track */
	UINT16  bsHeads;                /* number of read/write heads */
	UINT32  bsHiddenSectors;        /* number of hidden sectors */
	UINT32  bsBigTotalSectors;      /* number of sectors if bsSectors==0 */
	/* End of BPB  */
} A_BF_BPB16, *PA_BF_BPB16;


/***************************** FAT32 Structure *******************************/

/* BPB for a FAT32 partition */
typedef struct _A_BF_BPB32 {
	/* Start common for all FAT type */
	UINT16  bsBytesPerSector;
	UTINY   bsSectorsPerCluster;
	UINT16  bsReservedSectors;
	UTINY   bsNumberOfFATs;
	UINT16  bsRootEntries;
	UINT16  bsTotalSectors;
	UTINY   bsMediaDescriptor;
	UINT16  bsSectorsPerFAT;
	UINT16  bsSectorsPerTrack;
	UINT16  bsHeads;
	UINT32  bsHiddenSectors;
	UINT32  bsBigTotalSectors;
	/* End common for all FAT type */

	/* Additional Information for FAT 32 */
	UINT32  bsBigSectorsPerFat;
	UINT16  bsExtFlags;
	UINT16  bsFS_Version;
	UINT32  bsRootDirStrtClus;
	UINT16  bsFSInfoSec;
	UINT16  bsBkUpBootSec;
	UINT16  bsReserved[6];
} A_BF_BPB32, *PA_BF_BPB32;


typedef struct _BOOTSECT {
	UCHAR   bsJump[3];      /* jmp instruction */
	TEXT    bsOemName[8];   /* OEM name and version */

	/* This portion is the BPB */
	union {
		A_BF_BPB32      bpb32;  /* For FAT32 */
		A_BF_BPB16      bpb16;  /* For FAT12 or FAT16 */
	} bpb;

	UTINY   bsDriveNumber;  /* 80h if first hard drive */
	UTINY   bsReserved;     
	UTINY   bsBootSignature;/* 29h if extended boot-signature record */
	UINT32  bsVolumeID;     /* volume ID number */
	TEXT    bsVolumeLabel[11]; /* volume label */
	TEXT    bsFileSysType[8]; /* file-system type (FAT12, FAT16, FAT32)  */
} BOOTSECTOR, *PBOOTSECTOR;

#define         FSINFOSIG       0x61417272L     /* aArr */
#define         FSFREEPTR       0x00000002L

typedef struct  _BIGFATBOOTFSINFO {
	UINT32  bfFSInf_Sig;            /* signature given by FSINFOSIG */
	UINT32  bfFSInf_free_clus_cnt;  /* */
	UINT32  bfFSInf_next_free_clus; /* */
	UINT32  bfFSInf_resvd[3];       /* */
} BIGFATBOOTFSINFO, *PBIGFATBOOTFSINFO;


/* User structure: Each task that accesses the file system may have one of 
* these structures. The pointer fs_user() points to the current user. In 
* a real time exec. you might put this in the process control block or
* have an array of them an change the fs_user pointer at task switch time
* Note: Having one of these structures per task is NOT absolutely necessary.
*       If you do not these properties are simply system globals. 
* Code in pc_memory.c and pc_user.c provide a plausible way of managing the
* user structure.
*/


/* Define a macro to map fs_user to call __fs_user() */
#define fs_user __fs_user()

typedef struct file_system_user
{
	INT16   p_errno;        /* error number */
	INT16   dfltdrv;        /* Default drive to use if no drive specified */
	DROBJ   *lcwd[NDRIVES]; /* current working dirs */
	USEROBJ user_object;    /* for private use by the kernel functions */
} FILE_SYSTEM_USER, *PFILE_SYSTEM_USER;

/* Table for calculating sectors per cluster */
typedef struct format_dec_tree
{
	UINT16  sec_p_alloc;    /* Sectors per allocation */
	UINT16  ents_p_root;    /* Number of entries per root */
	ULONG   n_blocks;       /* Number of sectors per partition */
} FORMAT_DEC_TREE;


/* Partition table descriptions. */
/* One disk partition table */
typedef struct ptable_entry {
	UTINY   boot;   /* Boot signature */
	UTINY   s_head; /* Starting head */
	UINT16  s_cyl;  /* Starting cylinder */
	UTINY   p_typ;  /* Partition type */
	UTINY   e_head; /* Ending head */
	UINT16  e_cyl;  /* Ending cylinder */
	ULONG   r_sec;  /* Relative sector of start of part */
	ULONG   p_size; /* Size of partition */
} PTABLE_ENTRY;


typedef struct ptable {
	PTABLE_ENTRY ents[4];
	UINT16  signature;      /* should be 0xAA55 */
} PTABLE;


#ifndef FILE_SYSTEM_DATA_STRUCT    /* define in FLCONST.C */

SDIMPORT FINODE  *inoroot;
SDIMPORT DDRIVE  *mem_drives_structures;
SDIMPORT BLKBUFF *mem_block_pool;
SDIMPORT PC_FILE *mem_file_pool;
SDIMPORT DROBJ   *mem_drobj_pool;
SDIMPORT FINODE  *mem_finode_pool;
SDIMPORT FINODE  *mem_finode_freelist;
SDIMPORT DROBJ   *mem_drobj_freelist;


/* FAT buffers. For File System use internally */
SDIMPORT BLOCK_ALLOC fat_drives[FAT_BUFFER_SIZE*NDRIVES];

/* Directory buffers.  Internally to the File System */
SDIMPORT BLOCK_ALLOC directory_buffer[NBLKBUFFS];

/* Full string path for File System use internally */
SDIMPORT UINT16 fspath[EMAXPATH];
SDIMPORT UINT16 currFileName[EMAXPATH];
SDIMPORT TEXT *longFileName;
SDIMPORT TEXT *saveFileName;
SDIMPORT SDWCHAR unicodeFileName[EMAXPATH];


SDIMPORT BLKBUFF *fslocal_buffer;


/* String constants */
SDIMPORT const UTEXT string_star[4];
SDIMPORT const UTEXT string_3_spaces[4];
SDIMPORT const UTEXT string_backslash[4];
SDIMPORT const UTEXT string_fat_12[10];
SDIMPORT const UTEXT string_fat_16[10];
SDIMPORT const UTEXT string_fat_32[10];
SDIMPORT const UTEXT string_padded_dot[10];
SDIMPORT const UTEXT string_padded_dot_dot[10];
SDIMPORT const UTEXT string_null[1];
SDIMPORT const UTEXT char_backslash;

SDIMPORT const UTEXT char_forwardslash;


#endif /* FILE_SYSTEM_DATA_STRUCT */


/* Make sure memory is initted prolog for api functions */
#define CHECK_MEM(TYPE, RET)  if (!_pc_memory_init()) return((TYPE) RET);
#define VOID_CHECK_MEM()  if (!_pc_memory_init()) return;
#define IS_AVOLORDIR(X) ((X->isroot) || (X->finode->fattribute & AVOLUME|ADIRENT))


/* File System function prototypes.  Import from the low level driver. */
SDIMPORT SDVOID system_controller_init(INT16 driveno);
SDIMPORT SDVOID system_controller_close(INT16 driveno);


/* File FLAPI.C */
INT16   check_drive(TEXT *name);
SDBOOL  pc_i_dskopen(INT16 driveno);
ULONG   _po_lseek(PC_FILE *pfile, INT32 offset, INT16 origin, INT16 *err_flag);
SDBOOL  _po_flush(PC_FILE *pfile);

UINT32  pc_find_contig_clusters(DDRIVE *pdr, UINT32 startpt, UINT32 *pchain, UINT16 min_clusters, INT16 method);

/* File FILESRVC.C */
PC_FILE *pc_fd2file(PCFD fd, INT16 flags);
PCFD    pc_allocfile(SDVOID);
SDVOID  pc_freefile(PCFD fd);
SDBOOL  pc_flush_all_files(DDRIVE *pdrive);
INT16   pc_test_all_files(DDRIVE *pdrive);
SDVOID  pc_free_all_files(DDRIVE *pdrive);

/* File APIUTIL.C */
SDBOOL  pc_dskinit(INT16 driveno);
SDBOOL  pc_idskclose(INT16 driveno);
SDBOOL  pc_inside_subdirectory (DROBJ *pobj);
DROBJ   *pc_get_cwd(DDRIVE *pdrive);
SDVOID  _synch_file_ptrs(PC_FILE *pfile);
SDVOID  pc_upstat(FSDSTAT *statobj);
SDVOID  pc_finode_stat(FINODE *pi, STAT *pstat);
SDBOOL  get_disk_volume(INT16 driveno, ULONG *pserialno);

/* File BLOCK.C */
/*BLKBUFF *pc_blkpool(DDRIVE *pdrive);*/
SDVOID  pc_free_all_blk(DDRIVE *pdrive);
SDVOID  pc_free_buf(BLKBUFF *pblk, SDBOOL waserr, SDBOOL do_lock);
BLKBUFF *pc_init_blk(DDRIVE *pdrive, BLOCKT blockno);
BLKBUFF *pc_read_blk(DDRIVE *pdrive, BLOCKT blockno);
SDBOOL  pc_write_blk(BLKBUFF *pblk);
BLKBUFF *pc_scratch_blk(SDVOID);

/* File DROBJ.C */
DROBJ   *pc_fndnode(TEXT *path);
DROBJ   *pc_get_inode(DROBJ *pobj, DROBJ *pmom, TEXT *filename, TEXT *fileext);
SDBOOL  pc_findin(DROBJ *pobj, TEXT *filename, TEXT *fileext);
DROBJ   *pc_get_mom(DROBJ *pdotdot);
DROBJ   *pc_mkchild(DROBJ *pmom);
DROBJ   *pc_mknode ( DROBJ *pmom, TEXT *filename, TEXT *fileext, DOSINODE *pnode, UINT16 attributes, UINT16 dirUpdate);
SDBOOL  pc_insert_inode( DROBJ *pobj, DROBJ *pmom);
SDBOOL  pc_rmnode(DROBJ *pobj);
SDBOOL  pc_update_inode(DROBJ *pobj, SDBOOL set_archive, SDBOOL set_date);
DROBJ   *pc_get_root(DDRIVE *pdrive);
BLOCKT  pc_firstblock(DROBJ *pobj);
INT16   pc_next_block(DROBJ *pobj);
SDBOOL  pc_prev_block(DROBJ *pobj);
BLOCKT  pc_l_next_block(DROBJ *pobj);
BLOCKT  pc_l_prev_block(DROBJ *pobj);
SDVOID  pc_marki( FINODE *pfi, DDRIVE *pdrive, BLOCKT sectorno, INT16 index);
FINODE  *pc_scani(DDRIVE *pdrive, BLOCKT sectorno, INT16 index);
DROBJ   *pc_allocobj(SDVOID);
FINODE  *pc_alloci(SDVOID);
SDVOID  pc_free_all_i(DDRIVE *pdrive);
SDVOID  pc_freei(FINODE *pfi);
SDVOID  pc_freeobj(DROBJ *pobj);
SDVOID  pc_dos2inode (FINODE *pdir, DOSINODE *pbuff);
SDVOID  pc_init_inode(FINODE *pdir, TEXT *filename, TEXT *fileext, UTINY attr, UINT32 cluster, ULONG size, DATESTR *crdate);
SDVOID  pc_ino2dos (DOSINODE *pbuff, FINODE *pdir);
SDBOOL  pc_isavol( DROBJ *pobj);
SDBOOL  pc_isadir( DROBJ *pobj);
INT16   pc_strlen (TEXT *name);
SDVOID  LongFilenameToEntry (UTINY *leName, COUNT *ind, COUNT length, COUNT totalChar);

/* File LOWL.C */
UCOUNT  pc_alloc_chain(DDRIVE *pdr, UINT32 *pstart_cluster, UCOUNT n_clusters);
UINT32  pc_find_free_cluster(DDRIVE *pdr, UINT32 startpt, UINT32 endpt);
UINT32  pc_clalloc(DDRIVE *pdr, UINT32 clhint);
UINT32  pc_clgrow(DDRIVE *pdr, UINT32  clno);
UINT32  pc_clnext(DDRIVE *pdr, UINT32  clno);
UINT32  pc_clprev(DDRIVE *pdr, UINT32  clno);
SDVOID  pc_clrelease(DDRIVE *pdr, UINT32  clno);
SDBOOL  pc_faxx(DDRIVE *pdr, UINT32 clno, UINT32 *pvalue);
SDBOOL  pc_flushfat(INT16 driveno);
SDVOID  pc_freechain(DDRIVE *pdr, UINT32 cluster);
UINT32  pc_cl_truncate(DDRIVE *pdr, UINT32 cluster, UINT32 l_cluster);
UINT16  pc_get_chain(DDRIVE *pdr, UINT32 start_cluster, UINT32 *pnext_cluster, UCOUNT n_clusters);
SDBOOL  pc_pfaxx(DDRIVE *pdr, UINT32 clno, UINT32 value);
UCHAR   *pc_pfswap(DDRIVE *pdr, UINT32 index);
UCHAR   *pc_FATRead(DDRIVE *pdr, UINT32 index, UINT32 starting, UINT16 count);
SDBOOL  pc_pfflush(DDRIVE *pdr);
SDBOOL  pc_gblk0(INT16 driveno, DDRIVE *pdr);
SDBOOL  pc_clzero(DDRIVE *pdrive, UINT32 cluster);
DDRIVE  *pc_drno2dr(INT16 driveno);
SDBOOL  pc_get_FAT32freecluster(INT16 driveno);
SDBOOL  pc_updatedskfree(INT16 driveno);
SDBOOL  pc_dskfree(INT16 driveno, SDBOOL  unconditional);
ULONG   pc_ifree(INT16 driveno);
UINT32  pc_sec2cluster(DDRIVE *pdrive, BLOCKT blockno);
UINT32  pc_sec2ClusterDir(DDRIVE *pdrive, BLOCKT blockno);
UINT16  pc_sec2index(DDRIVE *pdrive, BLOCKT blockno);
BLOCKT  pc_cl2sector(DDRIVE *pdrive, UINT32 cluster);
INT16   get_partition_info(INT16 driveno);


/* File PC_MEMRY.C */
SDBOOL  _pc_memory_init(SDVOID);
SDVOID  pc_memory_close(SDVOID);
DROBJ   *pc_memory_drobj(DROBJ *pobj);
FINODE  *pc_memory_finode(FINODE *pinode);

/* File FLUTIL.C */
SDBOOL  pc_allspace(TEXT *p, INT16 i);
SDBOOL  pc_isdot(TEXT *fname, TEXT *fext);
SDBOOL  pc_isdotdot(TEXT *fname, TEXT *fext);
TEXT    *pc_parsedrive(INT16 *driveno, TEXT  *path);
SDBOOL  pc_fileparse(TEXT *filename, TEXT *fileext, TEXT *p);
TEXT    *pc_nibbleparse(TEXT *filename, TEXT *fileext, TEXT *path);
SDBOOL  pc_parsepath(TEXT *topath, TEXT *filename, TEXT *fileext, TEXT *path);
SDBOOL  pc_higherAlias (TEXT *filename1, TEXT *filename2);
SDBOOL  pc_patcmpupto8ordot ( TEXT *pPtr1, TEXT *pPtr2 );
SDBOOL  pc_patcmplong(TEXT *p, TEXT *pattern);
SDBOOL  pc_patcmp(TEXT *p, TEXT *pattern, INT16 size);
SDVOID  pc_str2upper(TEXT *to, TEXT *from);
SDVOID  pc_strn2upper(TEXT *to, TEXT *from, INT16 n);
TEXT    pc_byte2upper(TEXT c);
SDBOOL  validate_filename(TEXT *name, INT16 len);
SDBOOL  is_current_highest_alias_name(TEXT *filename, TEXT *aliasname);
SDVOID  convert_alias_name(TEXT *filename);
UINT16  pc_chksum(UTINY *name, UTINY *ext);
INT16   copyLongFileName (INT16 offset, INT16 index, UTEXT *fNameExt, UTEXT *curNameExt, INT16 displ);
INT16   wcopyLongFileName (INT16 offset, INT16 index, SDWCHAR *fNameExt, UTEXT *curNameExt, INT16 displ);

/* File pckernel.c */
SDBOOL  pc_kernel_init(SDVOID);
SDVOID  pc_kernel_shutdown(SDVOID);
PFILE_SYSTEM_USER __fs_user(SDVOID);
SDVOID  pc_free_all_users(INT16 driveno);
SDBOOL  pc_kernel_init_lockobj(FINODE *pfi);
SDVOID  pc_kernel_free_lockobj(FINODE *pfi);

/* devio.c */
SDBOOL  devio_open(INT16 driveno);
SDVOID  devio_close(INT16 driveno);
SDBOOL  devio_read(INT16 driveno, ULONG blockno, UCHAR *buf, UINT16 n_to_read);
SDBOOL  devio_write(INT16 driveno, ULONG blockno, UCHAR *buf, UCOUNT n_to_write);
SDBOOL  devio_erase(INT16 driveno, ULONG blockno, UCOUNT n_to_erase);
SDBOOL  devio_read_serial(INT16 driveno, PDRV_GEOMETRY_DESC pserialno);

/* chkmedia.c */
SDBOOL  check_media(INT16 driveno, SDBOOL mount_is_not_error);
SDBOOL  check_media_entry(INT16 driveno);
INT16   check_media_status(INT16 driveno);
SDBOOL  check_media_io(INT16 driveno);

/* powmgmt.c */
INT16   pcntrl_get_power_count(SDVOID);

/* format.c */
SDBOOL  pc_do_format(INT16 driveno);
TEXT    *get_volume_label(SDVOID);

/* flconst.c */
SDVOID  null_pointers(SDVOID);

/* flutil.c */
#if (CHAR_16BIT)
SDVOID  b_pack(UINT16 *to, UTINY *from, UINT16 length, UINT16 offset);
SDVOID  w_pack(UINT16 *to, UINT16 from, UINT16 offset);
SDVOID  w_unpack(UINT16 *to, UINT16 *from, UINT16 offset);
SDVOID  l_pack(UINT16 *to, ULONG from, UINT16 offset);
SDVOID  l_unpack(ULONG *to, UINT16 *from, UINT16 offset);

SDVOID  char_pack_dosinode(UINT16 *pbuff, FINODE *pdir, UINT16 offset);
SDVOID  char_unpack_dosinode(DOSINODE *pdir, UINT16 *pbuff, UINT16 offset);
SDVOID  char_pack_longinode(UINT16 *pbuff, LONGINODE *pdir, UINT16 offset);
SDVOID  char_unpack_longinode(LONGINODE *pdir, UINT16 *pbuff, UINT16 offset);
SDVOID  char_pack_ptable_entry(UINT16 *dst, PTABLE_ENTRY *scr, UINT16 offset);
SDVOID  char_unpack_ptable_entry(PTABLE_ENTRY *dst, UINT16 *src, UINT16 offset);
SDVOID  char_pack_ptable(UINT16 *dst, PTABLE *src, UINT16 offset);
SDVOID  char_unpack_ptable(PTABLE *dst, UINT16 *src, UINT16 offset);
#endif

/* errcode.c */
INT16   platform_convert_critical_error(INT16 errno);


SDBOOL validate_current_information( TEXT *topath, TEXT *filename, TEXT *fileext, TEXT *name );

COUNT   pc_num_drives(SDVOID);
COUNT   pc_num_users(SDVOID);
COUNT   pc_nuserfiles(SDVOID);
SDBOOL  pc_validate_driveno(INT16 driveno);


/****************************** Time Stamp **********************************/
#define PC_GETSYSDATE(d, t)     (oem_getsysdate(d, t))
#define PC_GETSYSDATEEXT(t)     (oem_getsysdateext(t))
#define PC_SETSYSDATE(d, t)     (oem_setsysdate(d, t))
 
/* Peripheral Bus interface for IDE, PCMCIA, SPI, MMC, ...*/
#include "intrface.h"

#endif  /* (USE_FILE_SYSTEM) */


#ifdef __cplusplus
}
#endif



#define __PCDISK__

#endif
