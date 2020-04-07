// format.h
// fat32 format utility
// temancl@fullplaymedia.com 04/16/02

#ifndef _CHKDISK_H
#define _CHKDISK_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Return values of various functions
 */
#define	FSOK		0		/* Check was OK */
#define	FSBOOTMOD	1		/* Boot block was modified */
#define	FSDIRMOD	2		/* Some directory was modified */
#define	FSFATMOD	4		/* The FAT was modified */
#define	FSERROR		8		/* Some unrecovered error remains */
#define	FSFATAL		16		/* Some unrecoverable error occured */
#define FSDIRTY		32		/* File system is dirty */
#define FSFIXFAT	64		/* Fix file system FAT */
#define FSCANCEL	128		/* cancel operation */
    
int chkdsk_fat32(const char* fname,bool (*chkstatuscb)(int,int,int,int));

#ifdef __cplusplus
}
#endif

#endif // _CHKDISK_H


