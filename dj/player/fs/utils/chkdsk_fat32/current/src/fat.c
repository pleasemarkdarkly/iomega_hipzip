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


#include <sys/cdefs.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include "ext.h"
//#include "fsutil.h"


static int checkclnum __P((struct bootblock *, int, cl_t, cl_t *));
static int clustdiffer __P((cl_t, cl_t *, cl_t *, int));
static int tryclear __P((struct bootblock *, struct fatEntry *, cl_t, cl_t *));
static int _readfat __P((int, struct bootblock *, int, u_char *, int, int));

/*
 * Check a cluster number for valid value
 */
static int
checkclnum(boot, fat, cl, next)
	struct bootblock *boot;
	int fat;
	cl_t cl;
	cl_t *next;
{
	if (*next >= (CLUST_RSRVD&boot->ClustMask))
		*next |= ~boot->ClustMask;
	if (*next == CLUST_FREE) {
		boot->NumFree++;
		return FSOK;
	}
	if (*next == CLUST_BAD) {
		boot->NumBad++;
		return FSOK;
	}
	if (*next < CLUST_FIRST
	    || (*next >= boot->NumClusters && *next < CLUST_EOFS)) {
		pwarn("Cluster %u in FAT %d continues with %s cluster number %u\n",
		      cl, fat,
		      *next < CLUST_RSRVD ? "out of range" : "reserved",
		      *next&boot->ClustMask);
		if (ask(0, "Truncate")) {
			*next = CLUST_EOF;
			return FSFATMOD;
		}
		return FSERROR;
	}
	return FSOK;
}

/*
 * Read from the FAT on the disk, but in little pieces
 */
static int
_readfat(fs, boot, no, buffer, fatoffset, buffsize)
	int fs;
	struct bootblock *boot;
	int no;
	u_char *buffer;
	int fatoffset;
	int buffsize;
{
	off_t off;


	off = boot->ResSectors + no * boot->FATsecs;
    //	off *= boot->BytesPerSec;

	if (seclseek(fs, off + fatoffset, SEEK_SET) != (off + fatoffset)) {
		perror("Unable to read FAT");
		goto err;
	}

	if (byteread(fs, buffer, buffsize)
	    != buffsize) {
		perror("Unable to read FAT");
		goto err;
	}

	return 1;

err:
	return 0;
}

/*
 * Read a FAT and decode it into internal format
 */
int
readfat(fs, boot, no, fat)
	int fs;
	struct bootblock *boot;
	int no;
	struct fatEntry *fat;
{
	u_char buffer[512];
	u_char *blockbuff;
	u_int32_t i,blockstart,blocksize,blockend,dataremain, sec_offset;

	cl_t cl;
	int ret = FSOK;

	if(!(boot->ClustMask & CLUST32_MASK))
	{
		perror("FAT32 support only\n");
		return FSFATAL;
	}


	boot->NumFree = boot->NumBad = 0;

	if (!_readfat(fs, boot, no, buffer, 0, 512))
		return FSFATAL;
	
	if (buffer[0] != boot->Media
	    || buffer[1] != 0xff || buffer[2] != 0xff
	    || (boot->ClustMask == CLUST16_MASK && buffer[3] != 0xff)
	    || (boot->ClustMask == CLUST32_MASK
		&& ((buffer[3]&0x0f) != 0x0f
		    || buffer[4] != 0xff || buffer[5] != 0xff
		    || buffer[6] != 0xff || (buffer[7]&0x0f) != 0x0f))) {

		/* Windows 95 OSR2 (and possibly any later) changes
		 * the FAT signature to 0xXXffff7f for FAT16 and to
		 * 0xXXffff0fffffff07 for FAT32 upon boot, to know that the
		 * filesystem is dirty if it doesn't reboot cleanly.
		 * Check this special condition before errorring out.
		 */
		if (buffer[0] == boot->Media && buffer[1] == 0xff
		    && buffer[2] == 0xff
		    && ((boot->ClustMask == CLUST16_MASK && buffer[3] == 0x7f)
			|| (boot->ClustMask == CLUST32_MASK
			    && buffer[3] == 0x0f && buffer[4] == 0xff
			    && buffer[5] == 0xff && buffer[6] == 0xff
			    && buffer[7] == 0x07)))
			ret |= FSDIRTY;
		else {
			/* just some odd byte sequence in FAT */
				
			switch (boot->ClustMask) {
			case CLUST32_MASK:
				pwarn("%s (%02x%02x%02x%02x%02x%02x%02x%02x)\n",
				      "FAT starts with odd byte sequence",
				      buffer[0], buffer[1], buffer[2], buffer[3],
				      buffer[4], buffer[5], buffer[6], buffer[7]);
				break;
			case CLUST16_MASK:
				pwarn("%s (%02x%02x%02x%02x)\n",
				    "FAT starts with odd byte sequence",
				    buffer[0], buffer[1], buffer[2], buffer[3]);
				break;
			default:
				pwarn("%s (%02x%02x%02x)\n",
				    "FAT starts with odd byte sequence",
				    buffer[0], buffer[1], buffer[2]);
				break;
			}

	
			if (ask(1, "Correct"))
				ret |= FSFIXFAT;
		}
	}


	// allocate internal fat structure here
	fat->entry = chkdsk_malloc(boot->NumClusters*sizeof(struct fatNextEntry));

	if (fat->entry == NULL) {
		perror("No space for FAT entries");
		chkdsk_free(fat->entry);
		return FSFATAL;
	}

	memset(fat->entry,0,boot->NumClusters*sizeof(struct fatNextEntry));

	fat->chain = chkdsk_malloc(MAX_CLUSTER_CHAINS*sizeof(struct fatChainInfo));

	if (fat->chain == NULL) {
		perror("No space for FAT chain table");
		
		chkdsk_free(fat->entry);
		return FSFATAL;
	}

	memset(fat->chain,0,MAX_CLUSTER_CHAINS*sizeof(struct fatChainInfo));
	

	// fat32 specific code

	// get a 64k buffer
	blockbuff = chkdsk_malloc(64*1024);

	// break table into 64k blocks to convert

	// start at the first valid fat32 entry
	blockstart = 0;
    sec_offset = 0;
    

	// blocksize = ?
	// blockend = ?

	dataremain = (boot->NumClusters*4);

	// cluster position
	cl = CLUST_FIRST;
		
	printf("expected clusters = %d\n",boot->NumClusters);

	while(dataremain != 0)
	{

		if(!chk_callback(pass,0,(boot->NumClusters*4) - dataremain,(boot->NumClusters*4)))
		{
			perror("cancel\n");		
			chkdsk_free(fat->entry);
			chkdsk_free(fat->chain);
			return FSCANCEL;
		}

		if(dataremain % 4 != 0)
		{
			perror("Fat table very wrong\n");		
			chkdsk_free(fat->entry);
			chkdsk_free(fat->chain);
			return FSFATAL;
		}

		if(dataremain > 64*1024)
			blocksize = 64*1024;
		else
			blocksize = dataremain;

		blockend = blockstart + blocksize;

		// read next block of fat entrys to convert

	

		if (!_readfat(fs, boot, no, blockbuff, sec_offset, blocksize))
		{
			perror("error reading data");
			chkdsk_free(blockbuff);
			chkdsk_free(fat->entry);
			chkdsk_free(fat->chain);
			return FSFATAL;
		}	


		// process next block, skip first 2 if this is at the beginning
		for(i = ((blockstart == 0) ? 8 : 0); i < blocksize; i+=4)
		{			

			if(cl >= boot->NumClusters)
			{	
				perror("cluster count overrun\n");
				chkdsk_free(blockbuff);
				chkdsk_free(fat->entry);
				chkdsk_free(fat->chain);
				return FSFATAL;
			}

			// fat32 code
			CLNEXT(fat,cl) = (unsigned int) blockbuff[i] + (blockbuff[i+1] << 8)
						   + (blockbuff[i+2] << 16) + (blockbuff[i+3] << 24);
			
			CLNEXT(fat,cl) &= boot->ClustMask;

			ret |= checkclnum(boot, no, cl, &CLNEXT(fat,cl));
			cl++;
		}

		dataremain -= blocksize;
		blockstart += blocksize;
        sec_offset += (blocksize / boot->BytesPerSec);
	}

	printf("actual clusters = %d\n",cl);

	chkdsk_free(blockbuff);
	return ret;
}

/*
 * Get type of reserved cluster
 */
char *
rsrvdcltype(cl)
	cl_t cl;
{
	if (cl == CLUST_FREE)
		return "free";
	if (cl < CLUST_BAD)
		return "reserved";
	if (cl > CLUST_BAD)
		return "as EOF";
	return "bad";
}

static int
clustdiffer(cl, cp1, cp2, fatnum)
	cl_t cl;
	cl_t *cp1;
	cl_t *cp2;
	int fatnum;
{
	if (*cp1 == CLUST_FREE || *cp1 >= CLUST_RSRVD) {
		if (*cp2 == CLUST_FREE || *cp2 >= CLUST_RSRVD) {
			if ((*cp1 != CLUST_FREE && *cp1 < CLUST_BAD
			     && *cp2 != CLUST_FREE && *cp2 < CLUST_BAD)
			    || (*cp1 > CLUST_BAD && *cp2 > CLUST_BAD)) {
				pwarn("Cluster %u is marked %s with different indicators, ",
				      cl, rsrvdcltype(*cp1));
				if (ask(1, "fix")) {
					*cp2 = *cp1;
					return FSFATMOD;
				}
				return FSFATAL;
			}
			pwarn("Cluster %u is marked %s in FAT 0, %s in FAT %d\n",
			      cl, rsrvdcltype(*cp1), rsrvdcltype(*cp2), fatnum);
			if (ask(0, "use FAT 0's entry")) {
				*cp2 = *cp1;
				return FSFATMOD;
			}
			if (ask(0, "use FAT %d's entry", fatnum)) {
				*cp1 = *cp2;
				return FSFATMOD;
			}
			return FSFATAL;
		}
		pwarn("Cluster %u is marked %s in FAT 0, but continues with cluster %u in FAT %d\n",
		      cl, rsrvdcltype(*cp1), *cp2, fatnum);
		if (ask(0, "Use continuation from FAT %d", fatnum)) {
			*cp1 = *cp2;
			return FSFATMOD;
		}
		if (ask(0, "Use mark from FAT 0")) {
			*cp2 = *cp1;
			return FSFATMOD;
		}
		return FSFATAL;
	}
	if (*cp2 == CLUST_FREE || *cp2 >= CLUST_RSRVD) {
		pwarn("Cluster %u continues with cluster %u in FAT 0, but is marked %s in FAT %d\n",
		      cl, *cp1, rsrvdcltype(*cp2), fatnum);
		if (ask(0, "Use continuation from FAT 0")) {
			*cp2 = *cp1;
			return FSFATMOD;
		}
		if (ask(0, "Use mark from FAT %d", fatnum)) {
			*cp1 = *cp2;
			return FSFATMOD;
		}
		return FSERROR;
	}
	pwarn("Cluster %u continues with cluster %u in FAT 0, but with cluster %u in FAT %d\n",
	      cl, *cp1, *cp2, fatnum);
	if (ask(0, "Use continuation from FAT 0")) {
		*cp2 = *cp1;
		return FSFATMOD;
	}
	if (ask(0, "Use continuation from FAT %d", fatnum)) {
		*cp1 = *cp2;
		return FSFATMOD;
	}
	return FSERROR;
}

/*
 * Compare two FAT copies in memory. Resolve any conflicts and merge them
 * into the first one.
 */
int
comparefat(boot, first, second, fatnum)
	struct bootblock *boot;
	struct fatEntry *first;
	struct fatEntry *second;
	int fatnum;
{

	int ret = FSOK;
#if 0 // broken
	cl_t cl;
	for (cl = CLUST_FIRST; cl < boot->NumClusters; cl++)
		if (first[cl].next != second[cl].next)
			ret |= clustdiffer(cl, &first[cl].next, &second[cl].next, fatnum);
#endif
	return ret;
}


// this had to be hacked up quite a bit to work correctly
void
clearchain(boot, fat, head)
	struct bootblock *boot;
	struct fatEntry *fat;
	cl_t head;
{
	cl_t p, q;
	int headindex;

	diag_printf("&&& clearing chain at %d\n",head);

	if(head == 0)
		return;

	// get the index to the head cluster
	headindex = CLHEADIDX(fat,head);

	// starting with the head, remove the next tables
	for (p = head; p >= CLUST_FIRST && p < boot->NumClusters; p = q) {
		
		// while all members of a chain
        // dc- if this actually fails, i'm scared
        if (CLHEADIDX(fat,p) != headindex)
            break;

		q = CLNEXT(fat,p);

		// this ups the big O
		// is q in previously visited sectors?
		// if not, add it.

		diag_printf("clearing cl %d\n",p);

		CLNEXT(fat,p) = CLUST_FREE;
        CLHEADIDX(fat,p) = 0;
	
	}

	// destroy chain entry
	fat->chain[headindex].head = CLUST_FREE;
	fat->chain[headindex].length = 0;
}

int
tryclear(boot, fat, head, trunc)
	struct bootblock *boot;
	struct fatEntry *fat;
	cl_t head;
	cl_t *trunc;
{
	if (ask(0, "Try clear chain starting at %d", head)) {
		clearchain(boot, fat, head);
		return FSFATMOD;
	} else if (ask(0, "Truncate")) {
		*trunc = CLUST_EOF;
		return FSFATMOD;
	} else
		return FSERROR;
}


static int iFreeCluster = 1;
/*
 * Check a complete FAT in-memory for crosslinks
 */
int
checkfat(boot, fat)
	struct bootblock *boot;
	struct fatEntry *fat;
{
	cl_t head, p, h, n;
	u_int len;
	u_int chainlen;
	int headindex = 0;
	int ret = 0, i = 0;
	int conf;

	/*
	 * pass 1: figure out the cluster chains.
	 */

	diag_printf("pass 1\n");

	for (head = CLUST_FIRST; head < boot->NumClusters; head++) {


		// way too many callbacks here otherwise
		if(head % 1000 == 0)
			if(!chk_callback(pass,1,head,boot->NumClusters * 2))
			{
				perror("cancel\n");			
				return FSCANCEL;
			}

		/* find next untravelled chain */
		if (CLHEAD(fat,head) != 0		/* cluster already belongs to some chain */
		    || CLNEXT(fat,head) == CLUST_FREE
		    || CLNEXT(fat,head) == CLUST_BAD)
			continue;		/* skip it. */

		/* cluster is the head of a chain */
		headindex = iFreeCluster;
		iFreeCluster++;

		if(iFreeCluster > MAX_CLUSTER_CHAINS)
		{
			perror("error: maximum cluster chains exceeded\n");
			return 0;
		}

		//diag_printf("new chain %d, head cluster %d\n",headindex,head);
		// create a new entry for this chain
		fat->chain[headindex].head = head;

		/* follow the chain and mark all clusters on the way */
		for (len = 0, p = head;
		     p >= CLUST_FIRST && p < boot->NumClusters && len < MAX_CLUST_PER_CHAIN;
		     p = CLNEXT(fat,p)) {

			if(CLFLAG(fat,head) == FAT_USED)
				diag_printf("error? - cluster belongs to other chain\n");

            // dc- original code did not touch USED bit; shouldn't this be set here?
			// CLFLAG(fat,p) |= (~FAT_USED & headindex);
            CLHEADIDX(fat,p) = headindex;

            //CLFLAG(fat,p) = FAT_UNUSED;
			// CLHEAD(fat,p) = head;
			len++;

			// diag_printf("set cluster %d,- index %d, head = %d\n",p,headindex,fat->chain[headindex].head,CLHEAD(fat,p));
		}

        // dc- this is arguably off-by-one compared to the below loop in pass 2, which does not think a circular chain exists
		if(len >= MAX_CLUST_PER_CHAIN)
		{
			diag_printf("circular in chain starting at cluster %d\n",head);
			// tryclear(boot, fat, head, &CLNEXT(fat,p));
			// diag_printf("clear complete\n");
			// continue;

		}

		/* the head record gets the length */
		
		CLLENGTH(fat,head) = CLNEXT(fat,head) == CLUST_FREE ? 0 : len;
		//diag_printf("set head %d - length = %d\n",head,CLLENGTH(fat,head));
	}


	diag_printf("pass 2\n");

	/*
	 * pass 2: check for crosslinked chains (we couldn't do this in pass 1 because
	 * we didn't know the real start of the chain then - would have treated partial
	 * chains as interlinked with their main chain)
	 */
	for (head = CLUST_FIRST; head < boot->NumClusters; head++) {

		// way too many callbacks here otherwise
		if(head % 1000 == 0)
			if(!chk_callback(pass,1,boot->NumClusters + head,boot->NumClusters * 2))
			{
				perror("cancel\n");			
				return FSCANCEL;
			}

		/* find next untravelled chain */
		if (CLHEAD(fat,head) != head)
			continue;

		/* follow the chain to its end (hopefully) */
		chainlen = 0;
		for (p = head;
		     (n = CLNEXT(fat,p)) >= CLUST_FIRST && n < boot->NumClusters && chainlen < MAX_CLUST_PER_CHAIN;
		     p = n)
		{
			if (CLHEAD(fat,n) != head)
			{
				diag_printf("head mismatch - %d != %d, index = %d\n",CLHEAD(fat,n),head,CLHEADIDX(fat,n));
				break;
			}
			
			chainlen++;

		}


		 if(chainlen >= MAX_CLUST_PER_CHAIN)
		 {
			 diag_printf("chain starting at %d has circular reference\n",head);
			 ret |= tryclear(boot, fat, head, &CLNEXT(fat,p));
			 continue;
		 }	

		if (n >= CLUST_EOFS || n >= CLUST_EOF)
			continue;

		if (n == CLUST_FREE || n >= CLUST_RSRVD) {
			pwarn("Cluster chain starting at %u ends with cluster marked %s\n",
			      head, rsrvdcltype(n));
			ret |= tryclear(boot, fat, head, &CLNEXT(fat,p));
			continue;
		}
		if (n < CLUST_FIRST || n >= boot->NumClusters) {
			pwarn("Cluster chain starting at %u ends with cluster out of range (%u)\n",
			      head, n);
			ret |= tryclear(boot, fat, head, &CLNEXT(fat,p));
			continue;
		}
		pwarn("Cluster chains starting at %u and %u are linked at cluster %u\n",
		      head, CLHEAD(fat,n), n);

		conf = tryclear(boot, fat, head, &CLNEXT(fat,p));

		if (ask(0, "Clear chain starting at %u", h = CLHEAD(fat,n))) {
			if (conf == FSERROR) {
				/*
				 * Transfer the common chain to the one not cleared above.
				 */
				for (p = n;
					 p >= CLUST_FIRST && p < boot->NumClusters;
					 p = CLNEXT(fat,p)) {
					if (h != CLHEAD(fat,p)) {
						/*
						 * Have to reexamine this chain.
						 */
						head--;
						break;
					}

					// diag_printf("lookup\n");
					headindex = 0;

					// lookup head cluster to get index
					for(i = 0; i < MAX_CLUSTER_CHAINS; i++)
					{
						if(fat->chain[i].head == head)
						{
							headindex = i;
						}
					}

					if(headindex == 0)
					{
						perror("fatal error: looking up head index\n");
						return 0;
					}
					CLFLAG(fat,p) = FAT_UNUSED;

                    CLHEADIDX(fat,p) = headindex;
                    //					CLFLAG(fat,p) |= (~FAT_USED & headindex);
					if(CLHEAD(fat,p) == 0)
					{
						diag_printf("headindex improperly represented %d\n",headindex);
					}
					// CLHEAD(fat,p) = head;
				}
			}
			clearchain(boot, fat, h);
			conf |= FSFATMOD;
		}

		ret |= conf;
	}

	return ret;
}

/*
 * Write out FATs encoding them from the internal format
 */
int
writefat(fs, boot, fat, correct_fat)
	int fs;
	struct bootblock *boot;
	struct fatEntry *fat;
	int correct_fat;
{
	u_char *buffer, *p;
	cl_t cl;
	int i;
	int count;


	u_int32_t fatsz;
	u_int32_t towrite;
	off_t off;
	int ret = FSOK;

	
	fatsz = boot->FATsecs * boot->BytesPerSec;
	
	buffer = chkdsk_malloc(64*1024);
	
	if (buffer == NULL) {
		perror("No space for write buffer\n");
		return FSFATAL;
	}


	for (i = 0; i < boot->FATs; i++) 
	{

		// can't cancel a write
		chk_callback(pass,4,i,boot->FATs);

			
		memset(buffer, 0, 64*1024);


		boot->NumFree = 0;
		
		p = buffer;

		switch (boot->ClustMask) {
		case CLUST32_MASK:
			count = 8;
			break;
		case CLUST16_MASK:
			count = 4;
			break;
		default:
			count = 3;
			break;
		}

		if (correct_fat) {
			*p++ = (u_char)boot->Media;
			*p++ = 0xff;
			*p++ = 0xff;
			switch (boot->ClustMask) {
			case CLUST16_MASK:
				*p++ = 0xff;
				break;
			case CLUST32_MASK:
				*p++ = 0x0f;
				*p++ = 0xff;
				*p++ = 0xff;
				*p++ = 0xff;
				*p++ = 0x0f;
				break;
			}
		} else {
			/* use same FAT signature as the old FAT has */
			// read old header in
			if (!_readfat(fs, boot, boot->ValidFat >= 0 ? boot->ValidFat :0,
						 p, 0, count)) 
			{
				chkdsk_free(buffer);
				return FSFATAL;
			}

			
		}


		off = boot->ResSectors + i * boot->FATsecs;
        /*		off *= boot->BytesPerSec; */

		towrite = fatsz;
		cl = CLUST_FIRST;
	
		// FIXME: more code that won't work outside fat32
		while(towrite > 0)
		{
			
			for(;cl < boot->NumClusters; cl++)
			{

				if(count == 64*1024)
					break;

				if(count > 64*1024)
				{
					perror("oops, went too far\n");
					break;
				}

				
				if (CLNEXT(fat,cl) == CLUST_FREE)
					boot->NumFree++;

				buffer[count] = (u_char)CLNEXT(fat,cl);
				buffer[count+1] = (u_char)(CLNEXT(fat,cl) >> 8);
				buffer[count+2] = (u_char)(CLNEXT(fat,cl) >> 16);
				
				// strip highest 4 bits from high byte
				buffer[count+3] = (u_char)((CLNEXT(fat,cl) >> 24) & 0x0f);

				count += 4;

				
			}

			// zero out remainder of fat region
			if(cl == boot->NumClusters)
			{
				if(towrite > 64*1024)
				{

					for(;count < 64*1024; count++)
					{
						buffer[count] = 0;
					}
				}
				else
				{
					for(;count < towrite; count++)
					{
						buffer[count] = 0;
					}
				}
			}
					



			if (seclseek(fs, off, SEEK_SET) != off
				|| bytewrite(fs, buffer, count) != count) {
				perror("Unable to write FAT\n");
				ret = FSFATAL; /* Return immediately?		XXX */
			}

			towrite -= count;
			off += (count/ boot->BytesPerSec);

			count = 0;
		}
	}


	chkdsk_free(buffer);
	return ret;
}

/*
 * Check a complete in-memory FAT for lost cluster chains
 */
int
checklost(dosfs, boot, fat)
	int dosfs;
	struct bootblock *boot;
	struct fatEntry *fat;
{
	cl_t head;
	int mod = FSOK;
	int ret;
	
	for (head = CLUST_FIRST; head < boot->NumClusters; head++) {

		// way too many callbacks here otherwise
		if(head % 1000 == 0)
			if(!chk_callback(pass,3,head,boot->NumClusters))
			{
				perror("cancel\n");
				return FSCANCEL;
			}

		/* find next untravelled chain */
		if (CLHEAD(fat,head) != head
		    || CLNEXT(fat,head) == CLUST_FREE
		    || (CLNEXT(fat,head) >= CLUST_RSRVD
			&& CLNEXT(fat,head) < CLUST_EOFS)
		    || CLFLAG(fat,head) == FAT_USED)
			continue;

        // the logic here is that we are on the head of a chain, which has valid clusters, but was not marked as used
        pwarn("Lost cluster chain at cluster %u (CLHEAD()=%u,CLNEXT()=%u,CLFLAG()=%u)\n%d Cluster(s) lost\n",
            head, CLHEAD(fat,head), CLNEXT(fat, head), CLFLAG(fat,head), CLLENGTH(fat,head));

#if 0
	
		mod |= ret = reconnect(dosfs, boot, fat, head);
		if (mod & FSFATAL)
			break;
		// ret == FSERROR &&
#endif
		if (ask(0, "Clear")) {
			clearchain(boot, fat, head);
			mod |= FSFATMOD;
		}
	}
	finishlf();

	if (boot->FSInfo) {
		ret = 0;
		if (boot->FSFree != boot->NumFree) {
			pwarn("Free space in FSInfo block (%d) not correct (%d)\n",
			      boot->FSFree, boot->NumFree);
			if (ask(1, "fix")) {
				boot->FSFree = boot->NumFree;
				ret = 1;
			}
		}
		if (boot->NumFree && CLNEXT(fat,boot->FSNext) != CLUST_FREE) {
			pwarn("Next free cluster in FSInfo block (%u) not free\n",
			      boot->FSNext);
			if (ask(1, "fix"))
				for (head = CLUST_FIRST; head < boot->NumClusters; head++)
					if (CLNEXT(fat,head) == CLUST_FREE) {
						boot->FSNext = head;
						ret = 1;
						break;
					}
		}
		if (ret)
			mod |= writefsinfo(dosfs, boot);
	}

	return mod;
}
