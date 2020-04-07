/*
 * Copyright (C) 1995, 1996, 1997 Wolfgang Solfrank
 * Copyright (c) 1995 Martin Husemann
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
 */

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <devs/storage/ata/atadrv.h>
#include <cyg/fileio/fileio.h>

#include <sys/types.h>
#include <errno.h>
#include <paths.h>
#include <time.h>

#include <sys/cdefs.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "ext.h"
//#include "fsutil.h"

static int allocs = 0;
static unsigned int allocsize = 0;

// track partition offset (actually read partition)
static unsigned int partoffset = 0;

void* chkdsk_malloc(size_t size)
{	

	void *p;
	p = malloc(size);
	allocs++;
	allocsize+=size;
    //	diag_printf("chkdsk: allocated %d bytes at %x, %d allocs, %d allocsize\n",size,p,allocs,allocsize);
	return p;
}

void chkdsk_free(void* p)
{
	free(p);
	allocs--;	
    //	diag_printf("chkdsk: freeing buffer at %x, %d allocs, %d allocsize\n",p,allocs,allocsize);
}


// read, write and lseek replacements to mask sector
// only work if all operations are sector aligned
int byteread(int fd, char* buff, int size)
{
	int ret;
	//
	if(size % 512 == 0)
	{		
		// aligned read
		read(fd,buff,size);
	}
	else
	{

		// FIXME: as long as buffers are sector aligned, this should be ok
		ret = size;
		perror("warning: unaligned read attempted\n");
//		perror("byteread %d %x %d\n",fd,buff,size);
		size += (512 - (size % 512));
		read(fd,buff,size);
		return ret;
	}


	return size;
}

static int last_seek_offset = 0;

int bytewrite(int fd, char* buff, int size)
{

//    diag_printf("[** bytewrite offset %015d size %015d **]\n", last_seek_offset, size);
	
	if(size % 512 == 0)
	{
		// FIXME: unsafe for the moment
		write(fd,buff,size);

	}
	else
	{	
	
		perror("error: unaligned write attempted\n");
		
		return 0;
	}


	return size;

}



// callback is (run, phase, phasecount, phasetotal)

int pass = 0;
bool (*chk_callback)(int,int,int,int); 

int
chkdsk_fat32(const char* fname,bool (*chkstatuscb)(int,int,int,int))
{	

	chk_callback = chkstatuscb;
	
	diag_printf("chkdsk fat32 test\n");
	int ret;
	pass = 0;
	cyg_io_handle_t blk_devH;
    cyg_uint32 len;
    drive_geometry_t dg;
    
    if (cyg_io_lookup(fname, &blk_devH) != ENOERR) 
	{
		diag_printf("Could not get handle to dev %s",fname);
		return FSERROR;
    }

	diag_printf("got handle to %s\n", fname);
	len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) 
	{
		diag_printf("Could not power up device\n");
    }

    while (cyg_io_set_config(blk_devH, 	IO_ATA_SET_CONFIG_FEATURES, 0, &len) != ENOERR) 
	{
		diag_printf("Could set device features\n");
    }


	
	diag_printf("powered up %s\n", fname);

    len = sizeof(dg);
    
	if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) 
	{
		diag_printf("Could not get geometry\n");
		return FSERROR;
    }

    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);


	// setup devfs so open works
	mount("", "/dev", "devfs");




	// maximum work, minimum interaction
	rdonly = 0;
	alwaysyes = 1;
	alwaysno = preen = 0;
	
	ret = FSFATMOD;

	while(ret & (FSFATMOD | FSDIRMOD | FSBOOTMOD))
	{	
		ret = checkfilesys(fname);
		pass++;
	}	

    diag_printf("chkdsk done\n");

    return ret;
}                                     



int alwaysno;	/* assume "no" for all questions */
int alwaysyes;	/* assume "yes" for all questions */
int preen;	/* we are preening */
int rdonly;


// ask placeholder, return yes for all, print ask
int
ask(int def, const char *fmt, ...)
{

	diag_printf("ASK: %s - YES\n");

	return 1;
}



int seclseek(int fd, unsigned int off, int flg)
{
#if 1
    unsigned int realoff = off + partoffset;

    unsigned int res;
    res = lseek( fd, realoff, flg );
    
    return last_seek_offset = off;
#else
	unsigned int realoff;

	// generate real offset to seek to, in bytes
	realoff = off + (partoffset * 512);


//	diag_printf("bytelseek offset %d\n",off);
	if(realoff % 512 == 0)
	{
		// all seeks should be aligned, protect from 0 cases
		int res = lseek(fd, realoff ? realoff / 512 : 0, flg);
        diag_printf(" realoff = %u, res = %u\n");
		return last_seek_offset = off; // return relative offset, not absolute
	}
	else
	{
		perror("unaligned seek attempted\n");
		return 0;
	}
#endif

}

int checkpart(int file)
{
	unsigned char partbuff[512];
	struct dos_partition partentry;
	lseek(file,0,SEEK_SET);
	read(file,partbuff,512);

	// same test as is used in sandisk layer - this should provide backward compatibility
	if(partbuff[0] != 0xEB && partbuff[0] != 0xE9)
	{
		memcpy(&partentry,partbuff + 0x1BE,sizeof(partentry));

		partoffset = partentry.dp_start;

		diag_printf("Partition table found\n");
		diag_printf("First partition at sector offset %d\n",partoffset);
	}
	else
	{
		// just to be sure
		partoffset = 0;
	}

}

int
checkfilesys(fname)
	const char *fname;
{
	int dosfs;
	struct bootblock boot;
	struct fatEntry fat;
	int finish_dosdirsection=0;
	int mod = 0;
	int ret = 8;

	rdonly = alwaysno;
	if (!preen)
		printf("** %s", fname);

	dosfs = open(fname, O_RDWR);
	printf("opened handle %d\n",dosfs);
	if (dosfs < 0) {
		perror("Can't open");
		return 8;
	}

	// verify partition table, get start offset sector
	checkpart(dosfs);

	if (readboot(dosfs, &boot) & FSFATAL) {
		close(dosfs);
		printf("fatal\n");
		return 8;
	}

	
	if (!preen)  {
		if (boot.ValidFat < 0)
			printf("** Phase 1 - Read and Compare FATs\n");
		else
			printf("** Phase 1 - Read FAT\n");
	}

	mod |= readfat(dosfs, &boot, boot.ValidFat >= 0 ? boot.ValidFat : 0, &fat);
	if (mod & (FSFATAL | FSCANCEL)) {
		close(dosfs);
		return 8;
	}

#if 0 // FIXME: not enough memory for this yet
	if (boot.ValidFat < 0)
		for (i = 1; i < boot.FATs; i++) {
			struct fatEntry *currentFat;

			mod |= readfat(dosfs, &boot, i, &currentFat);

			if (mod & (FSFATAL | FSCANCEL))
				goto out;

			mod |= comparefat(&boot, fat, currentFat, i);
			chkdsk_free(currentFat);
			if (mod & (FSFATAL | FSCANCEL))
				goto out;
		}
#endif

	if (!preen)
		printf("** Phase 2 - Check Cluster Chains\n");

	mod |= checkfat(&boot, &fat);
	if (mod & (FSFATAL | FSCANCEL))
		goto out;
	/* delay writing FATs */

	if (!preen)
		printf("** Phase 3 - Checking Directories\n");

	// no status possible in this phase, list traversal
	if(!chk_callback(pass,2,0,0))
	{
		if (mod |= FSCANCEL)
			goto out;
	}


	mod |= resetDosDirSection(&boot, &fat);
	finish_dosdirsection = 1;
	if (mod & (FSFATAL | FSCANCEL))
		goto out;
	/* delay writing FATs */

	mod |= handleDirTree(dosfs, &boot, &fat);
	if (mod & (FSFATAL | FSCANCEL))
		goto out;

	if (!preen)
		printf("** Phase 4 - Checking for Lost Files\n");

	mod |= checklost(dosfs, &boot, &fat);
	if (mod & (FSFATAL | FSCANCEL))
		goto out;


	// 5th phase, writing to disk. 
	
	/* now write the FATs */
	if (mod & FSFATMOD) {
		if (ask(1, "Update FATs")) {
			mod |= writefat(dosfs, &boot, &fat, mod & FSFIXFAT);
			if (mod & (FSFATAL | FSCANCEL))
				goto out;
		} else
			mod |= FSERROR;
	}

	if (boot.NumBad)
		pwarn("%d files, %d free (%d clusters), %d bad (%d clusters)\n",
		      boot.NumFiles,
		      boot.NumFree * boot.ClusterSize / 1024, boot.NumFree,
		      boot.NumBad * boot.ClusterSize / 1024, boot.NumBad);
	else
		pwarn("%d files, %d free (%d clusters)\n",
		      boot.NumFiles,
		      boot.NumFree * boot.ClusterSize / 1024, boot.NumFree);

	if (mod && (mod & FSERROR) == 0) {
		if (mod & FSDIRTY) {
			if (ask(1, "MARK FILE SYSTEM CLEAN") == 0)
				mod &= ~FSDIRTY;

			if (mod & FSDIRTY) {
				pwarn("MARKING FILE SYSTEM CLEAN\n");
				mod |= writefat(dosfs, &boot, &fat, 1);
			} else {
				pwarn("\n***** FILE SYSTEM IS LEFT MARKED AS DIRTY *****\n");
				mod |= FSERROR; /* file system not clean */
			}
		}
	}

	if (mod & (FSFATAL | FSERROR))
		goto out;

	ret = 0;

    out:
	if (finish_dosdirsection)
		finishDosDirSection();
	chkdsk_free(fat.entry);
	chkdsk_free(fat.chain);
	close(dosfs);

	if (mod & FSCANCEL)
		pwarn("\n***** FILE SYSTEM CHECK CANCELED *****\n");


	if (mod & (FSFATMOD | FSDIRMOD | FSBOOTMOD))
		pwarn("\n***** FILE SYSTEM WAS MODIFIED *****\n");

	if (allocs != 0)
		pwarn("\n***** MEMORY LEAK DETECTED *****\n");


	return mod;
}
