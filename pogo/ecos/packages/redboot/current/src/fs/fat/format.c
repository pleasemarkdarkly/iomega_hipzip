//........................................................................................
//........................................................................................
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 8/25/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/******************************************************************************
* FileName:    FORMAT.C - Format a device
*
* SanDisk Host Developer's Toolkit
* 
* Copyright (c) 1996-1999 SanDisk Corporation
* Copyright (c) 1996 EBS
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*
* Description:
*       Format a device with a complete DOS data structures (MBR, BPB, etc.)
*       Install new MBR, Boot Record, clean FAT and directory entries.
*
*       get_volume_label
*       pc_fat_size
*       do_fdisk
*       pc_do_format
*       pc_format
*
******************************************************************************/

#include <fs/fat/pcdisk.h>


#if (USE_FILE_SYSTEM)


#if (RTFS_WRITE)
SDIMPORT const TEXT sdvolume_label[12];

SDIMPORT const UINT16 part_boot[218/2];

/* Function prototypes */
SDLOCAL UINT16 pc_fat_size( UINT16 nreserved, UINT16 n_fat_copies,
                UINT16 cluster_size, UINT16 root_sectors, 
                ULONG volume_size, INT16 *nibs_per_entry );
SDLOCAL SDBOOL do_fdisk( INT16 driveno, UCHAR *pbuf, 
		ULONG partition_size,
		UINT16 n_cyls, UINT16 n_heads,
                UINT16 sec_ptrack, INT16 nibs_per_entry );


/*****************************************************************************
* Name: do_fat_size
*
* Description:
*       Create Master Boot Record
*
* Entries:
*       UINT16  nreserved
*       UINT16  cluster_size
*       UINT16  n_fat_copies
*       UINT16  root_sectors
*       ULONG   volume_size
*       INT16   *nibs_per_entry
*
* Returns:
*       The size of the FAT
*
******************************************************************************/
SDLOCAL UINT16 pc_fat_size(UINT16 nreserved, UINT16 n_fat_copies, UINT16 cluster_size, UINT16 root_sectors, UINT32 volume_size, INT16 *nibs_per_entry) /*__fn__*/
{
        UINT32 fat_size;
        UINT32 total_clusters;
	UINT16 entries_per_block;

	/*
	Calulate total cluster size. Assuming zero size fat:
	We round up to the nearest cluster boundary
	*/
	total_clusters = volume_size - nreserved - root_sectors;
	total_clusters /= cluster_size;

	/*
	Calculate the number of fat entries per block in the FAT. If
	< 4087 clusters total the fat entries are 12 bits hence 341 
	will fit. else 256 will fit.
        we add in n_fat_copies * 12 here since it takes 12 blocks to represent
	4087 clusters in 3 nibble form. So we add in the worst case FAT size
	here to enhance the accuracy of our guess of the total clusters.
	*/  

        if ( total_clusters <= (UINT32)(4087L + (n_fat_copies * 12)) )
	{
		entries_per_block = 341;
                *nibs_per_entry = DRV_FAT12;
	}

        else /* if ( total_clusters <= (ULONG)(0xFFF8) ) */
	{
		entries_per_block = 256;
                *nibs_per_entry = DRV_FAT16;
	}
#if 0
        else
	{
                entries_per_block = 128;
                *nibs_per_entry = DRV_FAT32;
	}
#endif

	fat_size = (total_clusters + entries_per_block - 1) / entries_per_block;

        return( (UINT16)fat_size );  
}


/*****************************************************************************
* Name: do_fdisk
*
* Description:
*       Create the Master Boot Record
*
* Entries:
*       INT16   driveno
*       UTINY   *pbuf
*       ULONG   partition_size
*       UINT16  n_cyls
*       UINT16  n_heads,
*       UINT16  sec_ptrack
*       INT16   nibs_per_entry
*
* Returns:
*       YES if successful
*       NO if failure
*
******************************************************************************/
SDLOCAL SDBOOL do_fdisk(INT16 driveno, UCHAR *pbuf, ULONG partition_size,
		UINT16 n_cyls, UINT16 n_heads,
                UINT16 sec_ptrack, INT16 nibs_per_entry) /*__fn__*/
{
        PTABLE  *part;
        UINT16  utemp;
        UINT16  utemp2;

        /* Locate a working buffer */
        part = (PTABLE *)&(directory_buffer[0]);

        /* Initialize the buffers */
        pc_memfill( pbuf, 512, (UTINY)0 );
        pc_memfill( part, sizeof(PTABLE), (UTINY)0 );

	/* Copy in the boot code */
        copybuff( (SDVOID *)pbuf,
                (SDVOID *)&part_boot[0],
                sizeof(part_boot) );

	/* Now fill in partition entry 0 */
        /* Set this to 0x80  for bootable */
        part->ents[0].boot = BOOTABLE_SIGNATURE;
        part->ents[0].s_head = 1;        /* Start at head 1 */
        part->ents[0].s_cyl = 1;

        if ( partition_size > (ULONG)0xFFFF )
                part->ents[0].p_typ = 6; /* Huge */
	else
	{
                if ( nibs_per_entry == DRV_FAT16 )
                        part->ents[0].p_typ = 4; /* DOS 16 bit */
		else
                        part->ents[0].p_typ = 1; /* DOS 12 bit */
	}

	/* Ending head is NHEADS-1 since heads go from 0 to n */
        part->ents[0].e_head = (UTINY)(n_heads - 1);

	/*
	Ending cylinder is in the top ten bits, secptrack in
	lower 6 ??.
	-2 cause we leave one free and ncyls is count of cyls,
	we want the lst cyl.
	*/
        utemp = ((n_cyls - 2) & 0xFF) << 8;     /* Low 8 bit to hi bite */
        utemp2 = ((n_cyls - 2) >> 2) & 0xC0;    /* Hi 2 bits to bits 6 + 7 */ 
	utemp |= utemp2;
	utemp |= sec_ptrack;
        part->ents[0].e_cyl = utemp;

	/*
	We always start one track into the drive. So relative sector
	starting sector is sec_ptrack.
	*/
        part->ents[0].r_sec = (ULONG) sec_ptrack;

	/* And partition size */
        part->ents[0].p_size = partition_size;

	/* Now for the signature */
        part->signature = LEGAL_SIGNATURE;

#if (CHAR_16BIT)
        char_pack_ptable((UINT16 *)pbuf, part, 0x1BE);

#if (LITTLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
        for (utemp=0; utemp < 256; utemp++)
                pbuf[utemp] = to_WORD((UCHAR *)&pbuf[utemp]);
 #endif
#endif

#else

	/* Now copy the partition into a block */
	/* The info starts at buf[1be] */
        copybuff( (SDVOID *)(pbuf + 0x1BE),
                        (SDVOID *)part,
                        sizeof(PTABLE) );
#endif

        /* Access to the media directly. Try the write */
        return( DEV_WRITE(driveno, 0L, pbuf, 1) );
}



SDIMPORT const UINT16 bootcode[202/2];
SDIMPORT const FORMAT_DEC_TREE f_d_c[];


/*****************************************************************************
* Name: pc_do_format
*
* Description:
*       Create the boot sector, FAT, Directory entry and data area
*
* Entries:
*       INT16 driveno   Drive Number
*
* Returns:
*       YES if successful
*       NO if failure
*
******************************************************************************/
SDBOOL pc_do_format(INT16 driveno) /*__fn__*/
{
#if (CHAR_16BIT)
        UINT16  *pbuf;
        UINT32  l;
#else
        UCHAR   *pbuf;
#endif
        UINT32  partition_size;
        UINT32  disk_size;
        INT16   nibs_per_entry;
        UINT16  secpalloc=0;
        UINT16  n_cyls;
        UINT16  n_heads;
        UINT16  sec_ptrack;
        UINT16  secpfat;
        UINT16  root_sectors;
        UINT16  root_entries=0;
        UINT16  i;
        SDBOOL  ret_val;

	/* Get drive geometry */
        drive_format_information( driveno, 
			&n_heads, 
			&sec_ptrack, 
                        &n_cyls );

	/* Get drive geometry */
        if ( !(n_heads && sec_ptrack && n_cyls) )
		return (NO);

	ret_val = NO;           /* Start by assuming failure */

        /*
        Grab some working space.  This will be a temporary working
        buffer to transfer the MBR and Boot record.
        */
#if (CHAR_16BIT)
        pbuf = (UINT16 *)&fat_drives[FAT_BUFFER_SIZE*driveno];
#else
        pbuf = (UCHAR *)&fat_drives[FAT_BUFFER_SIZE*driveno];
#endif
        disk_size = (ULONG)n_cyls;
        disk_size = disk_size * (ULONG)n_heads;
        disk_size = disk_size * (ULONG)sec_ptrack;

	/*
	Subtract out the reserved track in the front of the drive
	and one cylinder at the end of the volume that fdisk uses
	*/
        partition_size = disk_size - (ULONG)((n_heads + 1) * sec_ptrack);

	ret_val = NO;           /* Start by assuming failure */
        for ( i = 0; !ret_val; i++ )
	{
                if ( !f_d_c[i].n_blocks || (disk_size <= f_d_c[i].n_blocks) )
		{
			root_entries = f_d_c[i].ents_p_root;
                        secpalloc = f_d_c[i].sec_p_alloc;
			break;
		}
	}

        root_sectors = (INT16)(root_entries >> 4);     /* root_entries / 16 */

	/* Calculate sectors per pfat was not provided calculate it here */
	secpfat = pc_fat_size( 1 /* reserved */, 
			2 /*numfats*/, 
			secpalloc,
			root_sectors  /* root sectors */,
			partition_size,
			&nibs_per_entry );

	/* DO fdisk */
        if ( !do_fdisk(driveno, (UCHAR *)pbuf,
			partition_size,
			n_cyls,
			n_heads,
			sec_ptrack, 
			nibs_per_entry) )
		goto done;

	/* now format */    
        pc_memfill( (UCHAR *)pbuf, 512, (UCHAR)0 );

#if (PREERASE_ON_FORMAT)
	/* Erase all sectors 1 track at a time starting at the end of the root */
        i = sec_ptrack;
        for ( disk_size = sec_ptrack;
                disk_size < partition_size;
                disk_size += sec_ptrack )
	{
                if ( (disk_size + sec_ptrack) > partition_size )
                        i = (UINT16)(partition_size - disk_size);

                if ( !(DEV_SECTORS_ERASE(driveno, disk_size, i)) )
		{
			goto done;
		}
	}

#else
	/* Now zero the the fats and root directory */
        for ( i = 1;
		i < (UINT16) ((2 /*numfats*/ * secpfat) + 1 /* reserved */ + root_sectors);
                i++ )
	{
                if ( !DEV_WRITE( driveno,
                        (ULONG)(i + sec_ptrack),
                        (UCHAR *)pbuf,
                        1) )
                {
			goto done;
                }
        }
#endif

        /* Configure the Boot record */
        copybuff( (SDVOID *)pbuf, (SDVOID *)&bootcode[0], sizeof(bootcode) );

#if (CHAR_16BIT)
        fr_WORD((UCHAR *)&i, 512);
        w_pack ( pbuf, i, 11 );       /* bytes per sector */

        fr_WORD((UCHAR *)&i, secpalloc);
        w_pack( pbuf, i, 13 );    /* sectors / cluster */

        fr_WORD((UCHAR *)&i, 1);
        w_pack ( pbuf, i, 14 );         /* Number of reserved sectors. (Including block 0) */

        fr_WORD((UCHAR *)&i, 2);
        w_pack( pbuf, i, 16 );     /* number of duplicate fats */

        fr_WORD((UCHAR *)&i, root_entries);
        w_pack ( pbuf, i, 17 );/* number of dirents in root */

        if ( partition_size <= 0xFFFFL )
        {
                fr_WORD((UCHAR *)&i, ((UINT16)partition_size));
                w_pack ( pbuf, i, 19 ); /* Non-Huge partition  */
        }
        else
        {
                fr_DWORD((UCHAR *)&partition_size, partition_size);
                l_pack ( pbuf, partition_size, 32 );         /* Huge partition */
        }

        fr_WORD((UCHAR *)&i, 0xF8);     /* Media descriptor */
        w_pack( pbuf, i, 21 );

        fr_WORD((UCHAR *)&i, secpfat);
        w_pack ( pbuf, i, 22 );   /* Sectors per fat */

        fr_WORD((UCHAR *)&i, sec_ptrack);
        w_pack ( pbuf, i, 24 );/* sectors per trak */

        fr_WORD((UCHAR *)&i, n_heads);
        w_pack ( pbuf, i, 26 );   /* number heads */

        fr_DWORD((UCHAR *)&l, (ULONG)sec_ptrack);
        l_pack ( pbuf, l, 28 ); /* Hidden sectors */

        fr_WORD((UCHAR *)&i, 0x80);
        w_pack( pbuf, i, 36 );  /* physical drive number */

        fr_WORD((UCHAR *)&i, 0x29);
        w_pack( pbuf, i, 38 );  /* extended boot signature */

        /* Use the current tick for binary volume id */
        fr_DWORD((UCHAR *)&l, (ULONG)OS_GET_TICKS());
        l_pack( pbuf, l, 39 );

        b_pack (pbuf, (UTINY *)sdvolume_label, 11, 43);

        if ( nibs_per_entry == DRV_FAT12 )
                b_pack(pbuf, (UTINY *)string_fat_12, 8, 54);
        else if ( nibs_per_entry == DRV_FAT16 )
                b_pack(pbuf, (UTINY *)string_fat_16, 8, 54);
        else    /*  ( nibs_per_entry == DRV_FAT32 ) */
                b_pack(pbuf, (UTINY *)string_fat_32, 8, 82);

        fr_WORD((UCHAR *)&i, LEGAL_SIGNATURE);
        w_pack ( pbuf, i, 510 );   /* Signature */

#if (LITTLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
        for (i=0; i < 256; i++)
                pbuf[i] = to_WORD((UCHAR *)&pbuf[i]);
 #endif
#endif

#else

        fr_WORD ( pbuf+11, 512 );       /* bytes per sector */
        pbuf[13] = (UCHAR)secpalloc;    /* sectors / cluster */
        fr_WORD ( pbuf+14, 1 );         /* Number of reserved sectors. (Including block 0) */

        pbuf[16] = (UCHAR)0x02;         /* number of duplicate fats */

        fr_WORD ( pbuf+17, root_entries ); /* number of dirents in root */

        if ( partition_size <= 0xFFFFL )
        {
                i = (UINT16)partition_size;
                fr_WORD ( pbuf+19, i ); /* Non-Huge partition  */
        }
	else
                fr_DWORD ( pbuf+32, partition_size ); /* Huge partition */

        pbuf[21] = (UCHAR)0xF8;         /* Media descriptor */
        fr_WORD ( pbuf+22, secpfat );   /* Sectors per fat */

        fr_WORD ( pbuf+24, sec_ptrack );/* sectors per trak */
        fr_WORD ( pbuf+26, n_heads );   /* number heads */
        fr_DWORD ( pbuf+28, (ULONG)sec_ptrack ); /* Hidden sectors */

        pbuf[36] = (UCHAR)(0x80);
        pbuf[38] = (UCHAR)0x29;         /* extended boot signature */

        fr_DWORD( pbuf+39, (ULONG)OS_GET_TICKS() ); /* Use the current tick for binary volume id */
        copybuff( (SDVOID *)(pbuf+43), (SDVOID *)sdvolume_label, 11 );

        if ( nibs_per_entry == DRV_FAT12 )
                copybuff( (SDVOID *)(pbuf+54), (SDVOID *)string_fat_12, 8 );
        else if ( nibs_per_entry == DRV_FAT16 )
                copybuff( (SDVOID *)(pbuf+54), (SDVOID *)string_fat_16, 8 );
        else
                copybuff( (SDVOID *)(pbuf+82), (SDVOID *)string_fat_32, 8 );

        fr_WORD ( pbuf+510, LEGAL_SIGNATURE );   /* Signature */
#endif  /* (CHAR_16BIT) */

	/*
          Write the root block at sec_ptrack. This is the Boot Record
          where the File System starts.
	*/
        disk_size = (ULONG)sec_ptrack;
        if ( DEV_WRITE(driveno, disk_size, (UCHAR *)pbuf, 1) )
        {
                /* Take care the FAT */
                pc_memfill( (UCHAR *)pbuf, 512, (UCHAR)0 );

                /* Now put the signature in the FATs */
#if (CHAR_16BIT)
                /* Now put the signature in the fats */
                fr_WORD((UCHAR *)&i, 0xFFF8);
                w_pack( pbuf, i, 0 );

                i = 0x00FF;
                if ( nibs_per_entry == DRV_FAT16)
                {
                        i |= 0xFF00;    /* 0xF8FFFFFF */ 
                }

                if ( nibs_per_entry == DRV_FAT32 )
                {
                        i  = 0x0FFF;    /* 0xF8FFFF0F */

                        fr_DWORD((UCHAR *)&l, 0x0FFFFFFFL); /* cluster 1 */
                        l_pack( pbuf, l, 4 );
                }

                fr_WORD((UCHAR *)&i, i);
                w_pack( pbuf, i, 2 );           /* cluster 0 */

#if (LITTLE_ENDIAN)
#else
 #if (USE_HW_OPTION)
        for (i=0; i < 4; i++)
                pbuf[i] = to_WORD((UCHAR *)&pbuf[i]);
 #endif
#endif

#else

                pbuf[0] = (UCHAR) 0xF8;         /* Hard disk media descriptor */
                pbuf[1] = (UCHAR) 0xFF;
                pbuf[2] = (UCHAR) 0xFF;

                if (nibs_per_entry == DRV_FAT16)
                {
                        pbuf[3] = (UTINY) 0xFF;
                }

                if (nibs_per_entry == DRV_FAT32)
                {
                        pbuf[3] = (UTINY) 0x0F;
                        fr_DWORD ( pbuf+4, 0x0FFFFFFFL);
                }
#endif
                /* Write to the first FAT */
                disk_size += 1L;
                if ( DEV_WRITE(driveno,
                        disk_size,
                        (UCHAR *)pbuf,
                        1) )
                {
                        /* Write to the second FAT */
                        disk_size += (ULONG)secpfat;
                        if ( DEV_WRITE(driveno,
                                disk_size,
                                (UCHAR *)pbuf,
                                1) )
                        {
                                /* This is the only place where the formatting
                                   process has completed successfully.
                                */
                                ret_val = YES;  /* We've done all we need */
                        }
                }
        }

done:

        return (ret_val);
}


/*****************************************************************************
* Name: pc_format   -  Format a device.
*
* Description:
*       Place a standard partition and volume on a selected device.
*       This routine ascertains the drive parameters from the device.
*       It then parttions an formats the device based on those values.
*
* Entries:
*       INT16 driveno   Drive Number
*
* Returns:
*       YES if it was able to format the card, otherwise
*       it returns NO.
*
* Note: If pc_format fails, fs_user->p_errno will be set to one of
*       the following:
*
*        PEDEVICE    - Invalid device
*
******************************************************************************/
SDBOOL pc_format(INT16 driveno) /*__fn__*/
{
	DDRIVE *pdr;

        CHECK_MEM(SDBOOL, 0)

        if ( driveno >= NDRIVES )
	{
                errno = PEDEVICE;
                return (NO);
	}

        pdr = &mem_drives_structures[driveno];

	/* Abort the current mount */
        if ( pdr->dev_flag )
	{
		pc_dskfree(driveno, YES);
                pdr->enable_mapping = NO;
	}

	/* Check if the device is installed and do a low level open */
        if ( check_media_status(driveno) != 0 )
	{
                return (NO);
	}


	/*
        Format it, then shut the card down so we will mount it
        next time we're in.
	*/
        if ( !pc_do_format(driveno) )
	{
                /* Format failed */
                return (NO);
	}

        /* Format succeeded. Clear the flags for next installation */
        pdr->dev_flag = NO;
        pdr->enable_mapping = NO;

        return (YES);
}

#endif  /* (RTFS_WRITE) */

#endif  /* (USE_FILE_SYSTEM) */
