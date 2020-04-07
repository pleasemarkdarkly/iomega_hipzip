//==========================================================================
//
//      disk.c
//
//      RedBoot - On disk image store support
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000, 2001 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm
// Contributors: gthomas
// Date:         2001-03-15
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>
#include <dis.h>
#include <microdrive.h>
#include <fs/fat/sdapi.h>

#define IMAGE_STORE_NAME "/boot"
#ifdef CYGIMP_REDBOOT_CONFIG_STORE_DISK
#define CONFIG_DATA_NAME "boot.conf"
#endif

// Exported CLI functions
RedBoot_cmd("dis", 
            "Manage Disk images", 
            "{cmds}",
            do_dis
    );

// Internal commands
local_cmd_entry("init",
                "Initialize Disk Image System [DIS]",
                "[-f]",
                dis_init,
                DIS_cmds
    );
local_cmd_entry("list",
                "Display contents of Disk Image System [DIS]",
                "[-c]",
                dis_list,
                DIS_cmds
    );
local_cmd_entry("free",
                "Display free [available] space within Disk Image System [DIS]",
                "",
                dis_free,
                DIS_cmds
    );
local_cmd_entry("delete",
                "Delete an image from Disk Image System [DIS]",
                "name",
                dis_delete,
                DIS_cmds
    );
local_cmd_entry("load",
                "Load image from Disk Image System [DIS] into RAM",
                "[-b <memory_load_address>] [-c] name",
                dis_load,
                DIS_cmds
    );
local_cmd_entry("create",
                "Create an image",
                "-b <mem_base> -l <image_length> [-s <data_length>] 
                [-f <flash_addr>] [-e <entry_point>] [-r <ram_addr>] [-n] <name>",
                dis_create,
                DIS_cmds
    );

// Define table boundaries
CYG_HAL_TABLE_BEGIN( __DIS_cmds_TAB__, DIS_cmds);
CYG_HAL_TABLE_END( __DIS_cmds_TAB_END__, DIS_cmds);

extern struct cmd __DIS_cmds_TAB__[], __DIS_cmds_TAB_END__;

static void
dis_usage(char *why)
{
    printf("*** invalid 'dis' command: %s\n", why);
    cmd_usage(__DIS_cmds_TAB__, &__DIS_cmds_TAB_END__, "dis ");
}

static void
dis_init(int argc, char *argv[])
{
    DSTAT dstat;
    TEXT fname[8 + 1 + 3 + 1]; /* "/boot/filename.ext" */
    UINT16 attr;
    bool full_init = false;
    struct option_info opts[1];
    
    init_opts(&opts[0], 'f', false, OPTION_ARG_TYPE_FLG, 
              (void **)&full_init, (bool *)0, "full initialization, erases all of disk");
    if (!scan_opts(argc, argv, 2, opts, 1, 0, 0, ""))
    {
        return;
    }

    if (!verify_action("About to initialize [format] disk image system")) {
        printf("** Aborted\n");
        return;
    }
    printf("*** Initialize Disk Image System\n");

    // Create image store directory
    pc_mkdir(IMAGE_STORE_NAME);
    pc_get_attributes(IMAGE_STORE_NAME, &attr);
    pc_set_attributes(IMAGE_STORE_NAME, attr |= (AHIDDEN | ASYSTEM));
		      
    if (pc_set_cwd(IMAGE_STORE_NAME) == NO) {
	printf("   initialization failed: could not cd to /boot\n");
	return;
    }
    
    // Erase everything in the /boot directory
    if (pc_gfirst(&dstat, "*.*") == YES) {
	do {
	    if (dstat.fname[0] == '.') {
		continue;
	    }
#ifdef CYGIMP_REDBOOT_CONFIG_STORE_DISK
	    if (strcmp(dstat.longFileName, CONFIG_DATA_NAME) == 0) {
		continue;
	    }
#endif
	    pc_mfile(fname, dstat.fname, dstat.fext);
	    if (pc_unlink(fname) == NO) {
		printf("   could not delete %s\n", fname);
	    } else {
		printf("   deleted %s\n", fname);
	    }
	} while (pc_gnext(&dstat) == YES);
	pc_gdone(&dstat);
    }

#if 0
    // Create a pseudo image for RedBoot
    memset(&img, 0, sizeof(img));
    strcpy(img.name, "RedBoot");
    img->flash_base = redboot_disk_start;
    img->mem_base = redboot_disk_start;
    img->size = redboot_image_size;
#endif
    
#if 0
    // And a backup image
    memset(img, 0, sizeof(*img));
    strcpy(img->name, "RedBoot[backup]");
    img->disk_base = redboot_disk_start+redboot_image_size;
    img->mem_base = redboot_disk_start+redboot_image_size;
    img->size = redboot_image_size;
    img++;  img_count++;
#ifdef CYGSEM_REDBOOT_CONFIG
    // And a descriptor for the configuration data
    memset(img, 0, sizeof(*img));
    strcpy(img->name, "RedBoot config");
    cfg_base = (void *)((unsigned long)disk_end - (2*block_size));
    img->disk_base = (unsigned long)cfg_base;
    img->mem_base = (unsigned long)cfg_base;
    img->size = block_size;
    img++;  img_count++;
#endif
    // And a descriptor for the descriptor table itself
    memset(img, 0, sizeof(*img));
    strcpy(img->name, "DIS directory");
    dis_base = (void *)((unsigned long)disk_end - block_size);
    img->disk_base = (unsigned long)dis_base;
    img->mem_base = (unsigned long)dis_base;
    img->size = block_size;
    img++;  img_count++;
#ifdef CYGSEM_REDBOOT_DISK_LOCK_SPECIAL
    // Insure [quietly] that the directory is unlocked before trying to update
    disk_unlock((void *)dis_base, block_size, (void **)&err_addr);
#endif
    if ((stat = disk_erase(dis_base, block_size, (void **)&err_addr)) != 0) {
            printf("   initialization failed %p: 0x%x(%s)\n", err_addr, stat, disk_errmsg(stat));
    } else {
        if ((stat = disk_program(dis_base, dis_work_block, 
                                  img_count*sizeof(*img), (void **)&err_addr)) != 0) {
            printf("Error writing image descriptors at %p: 0x%x(%s)\n", 
                   err_addr, stat, disk_errmsg(stat));
        }
    }
#ifdef CYGSEM_REDBOOT_DISK_LOCK_SPECIAL
    // Insure [quietly] that the directory is locked after the update
    disk_lock((void *)dis_base, block_size, (void **)&err_addr);
#endif
#endif
}

static void
dis_list(int argc, char *argv[])
{
    struct dis_image_desc img;
    bool show_cksums = false;
    struct option_info opts[1];

    PCFD fd;
    DSTAT dstat;
    TEXT fname[8 + 1 + 3 + 1];
    int stat;
    
    init_opts(&opts[0], 'c', false, OPTION_ARG_TYPE_FLG, 
              (void **)&show_cksums, (bool *)0, "display checksums");
    if (!scan_opts(argc, argv, 2, opts, 1, 0, 0, ""))
    {
        return;
    }

    if (pc_set_cwd(IMAGE_STORE_NAME) == NO) {
	printf("Could not enter image store directory\n");
	return;
    }

    printf("Name              FLASH addr   %s    Length      Entry point\n",
           show_cksums ? "Checksum" : "Mem addr");
    if (pc_gfirst(&dstat, "*.*")) {
	do {
	    if (dstat.fname[0] == '.') {
		continue;
	    }
#ifdef CYGIMP_REDBOOT_CONFIG_STORE_DISK
	    if (strcmp(dstat.longFileName, CONFIG_DATA_NAME) == 0) {
		continue;
	    }
#endif
	    pc_mfile(fname, dstat.fname, dstat.fext);
	    if ((fd = po_open(fname, PO_RDONLY, 0)) < 0) {
		printf("Could not open file %s\n", fname);
	    } else {
		stat = po_read(fd, (UCHAR *)&img, sizeof(img));
		if (stat == sizeof(img)) {
		    printf("%-16s  0x%08lX   0x%08lX  0x%08lX  0x%08lX\n", img.name, 
			   img.flash_base, 
			   show_cksums ? img.file_cksum : img.mem_base, 
			   img.size, img.entry_point);
		} else {
		    printf("Error reading image metadata: %d\n", stat);
		}
		po_close(fd);
	    }
	} while (pc_gnext(&dstat) == YES);
	pc_gdone(&dstat);
    }
}

static void
dis_free(int argc, char *argv[])
{
    ULONG space;

    space = pc_free(0);
    printf("  %d bytes free\n", (int)space);
}

static void
dis_create(int argc, char *argv[])
{
    int i, stat;
    unsigned long mem_addr, exec_addr, flash_addr, entry_addr, length, img_size;
    char *name;
    bool mem_addr_set = false;
    bool exec_addr_set = false;
    bool entry_addr_set = false;
    bool flash_addr_set = false;
    bool length_set = false;
    bool img_size_set = false;
    bool no_copy = false;
    struct dis_image_desc img;
    struct option_info opts[7];

    PCFD fd;
    UINT16 attr;
    int bsize;
    
    init_opts(&opts[0], 'b', true, OPTION_ARG_TYPE_NUM, 
              (void **)&mem_addr, (bool *)&mem_addr_set, "memory base address");
    init_opts(&opts[1], 'r', true, OPTION_ARG_TYPE_NUM, 
              (void **)&exec_addr, (bool *)&exec_addr_set, "ram base address");
    init_opts(&opts[2], 'e', true, OPTION_ARG_TYPE_NUM, 
              (void **)&entry_addr, (bool *)&entry_addr_set, "entry point address");
    // This is ignored for disk
    init_opts(&opts[3], 'f', true, OPTION_ARG_TYPE_NUM, 
              (void **)&flash_addr, (bool *)&flash_addr_set, "FLASH memory base address");
    init_opts(&opts[4], 'l', true, OPTION_ARG_TYPE_NUM, 
              (void **)&length, (bool *)&length_set, "image length [in FLASH]");
    // This is ignored for disk
    init_opts(&opts[5], 's', true, OPTION_ARG_TYPE_NUM, 
              (void **)&img_size, (bool *)&img_size_set, "image size [actual data]");
    // This is ignored for disk, should generate a warning
    init_opts(&opts[6], 'n', false, OPTION_ARG_TYPE_FLG, 
              (void **)&no_copy, (bool *)0, "don't copy from RAM to FLASH, just update directory");
    if (!scan_opts(argc, argv, 2, opts, 7, (void *)&name, OPTION_ARG_TYPE_STR, "file name"))
    {
        dis_usage("invalid arguments");
        return;
    }

    if ((!no_copy && !mem_addr_set) || (no_copy && !flash_addr_set) ||
        !length_set || !name) {
        dis_usage("required parameter missing");
        return;
    }
    if (!img_size_set) {
        img_size = length;
    }
    if (strlen(name) >= sizeof(img.name)) {
        printf("Name is too long, must be less than %d chars\n", (int)sizeof(img.name));
        return;
    }
    if (!no_copy) {
        if ((mem_addr < (unsigned long)ram_start) ||
            ((mem_addr+img_size) >= (unsigned long)ram_end)) {
            printf("** WARNING: RAM address: %p may be invalid\n", (void *)mem_addr);
            printf("   valid range is %p-%p\n", (void *)ram_start, (void *)ram_end);
        }
        if (!flash_addr_set && (length > pc_free(0))) {
            printf("Can't locate %ld bytes free on disk\n", length);
            return;
        }
    }
    // Find a slot in the directory for this entry
    // First, see if an image by this name is already present
    if (pc_set_cwd(IMAGE_STORE_NAME) == NO) {
	printf("Could not enter image store directory\n");
	return;
    }
    fd = po_open(name, PO_CREAT | PO_EXCL | PO_WRONLY | PO_TRUNC, PS_IWRITE);
    if (fd < 0 && errno == PEEXIST) {
	if (!verify_action("An image named '%s' exists", name)) {
	    return;
	} else {
	    fd = po_open(name, PO_CREAT | PO_WRONLY | PO_TRUNC, PS_IWRITE);
	}
    }
    if (fd < 0) {
	printf("Can't create file: %d\n", errno);
	return;
    }
    memset(&img, 0, sizeof(img));
    strcpy(img.name, name);
    img.flash_base = flash_addr;
    img.mem_base = exec_addr_set ? exec_addr : (flash_addr_set ? flash_addr : mem_addr);
    img.entry_point = entry_addr_set ? entry_addr : (unsigned long)entry_address;  // Hope it's been set
    img.size = length;
    img.data_length = img_size;
    img.file_cksum = crc32((unsigned char *)mem_addr, img_size);
    stat = po_write(fd, (UCHAR *)&img, sizeof(img));
    if (stat != sizeof(img)) {
	printf("Error writing image descriptor: %d %d %d\n", stat, (int)sizeof(img), errno);
    }
    else {
	for (i = 0; i < length; i += DEV_BSIZE) {
	    bsize = (length - i) < DEV_BSIZE ? (length - i) : DEV_BSIZE;
	    stat = po_write(fd, (char *)(mem_addr + i), bsize);
	    if (stat != bsize) {
		printf("Error writing image: %d\n", errno);
		break;
	    }
	}
    }
    po_close(fd);

    // Protect the file
    pc_get_attributes(name, &attr);
    pc_set_attributes(name, attr |= ARDONLY);
}

static void
dis_load(int argc, char *argv[])
{
    char *name;
    struct dis_image_desc img;
    unsigned long mem_addr;
    bool mem_addr_set = false;
    bool show_cksum = false;
    struct option_info opts[2];
    unsigned long cksum;

    int stat;
    int i;
    int bsize;
    PCFD fd;
    
    init_opts(&opts[0], 'b', true, OPTION_ARG_TYPE_NUM, 
              (void **)&mem_addr, (bool *)&mem_addr_set, "memory [load] base address");
    init_opts(&opts[1], 'c', false, OPTION_ARG_TYPE_FLG, 
              (void **)&show_cksum, (bool *)0, "display checksum");
    if (!scan_opts(argc, argv, 2, opts, 2, (void **)&name, OPTION_ARG_TYPE_STR, "image name"))
    {
        dis_usage("invalid arguments");
        return;
    }
    if (pc_set_cwd(IMAGE_STORE_NAME) == NO) {
	printf("Could not enter image store directory\n");
	return;
    }
    if ((fd = po_open(name, PO_RDONLY, 0)) < 0) {
	printf("Could not open file\n");
	return;
    }
    stat = po_read(fd, (UCHAR *)&img, sizeof(img));
    if (stat != sizeof(img)) {
	printf("Could not get image descriptor: %d\n", errno);
	po_close(fd);
	return;
    }
    if (!mem_addr_set) {
        mem_addr = img.mem_base;
    }
    // Load image from FLASH into RAM
    if ((mem_addr < (unsigned long)ram_start) ||
        ((mem_addr+img.size) >= (unsigned long)ram_end)) {
        printf("Not a loadable image\n");
	po_close(fd);
        return;
    }
    for (i = 0; i < img.size; i += DEV_BSIZE) {
	bsize = (img.size - i) < DEV_BSIZE ? (img.size - i) : DEV_BSIZE;
	stat = po_read(fd, (char *)(mem_addr + i), bsize);
	if (stat != bsize) {
	    printf("Error reading image: %d\n", errno);
	    break;
	}
    }
    po_close(fd);
    entry_address = (unsigned long *)img.entry_point;
    cksum = crc32((unsigned char *)mem_addr, img.data_length);
    if (show_cksum) {
        printf("Checksum: 0x%08lx\n", cksum);
    }
    if (img.file_cksum) {
        if (cksum != img.file_cksum) {
            printf("** Warning - checksum failure.  stored: 0x%08lx, computed: 0x%08lx\n",
                   img.file_cksum, cksum);
        }
    }
}

static void
dis_delete(int argc, char *argv[])
{
    char * name;
    STAT stat;
    UINT16 attr;
    
    if (!scan_opts(argc, argv, 2, 0, 0, (void **)&name, OPTION_ARG_TYPE_STR, "image name"))
    {
	dis_usage("invalid arguments");
	return;
    }

    if (pc_set_cwd(IMAGE_STORE_NAME) == NO) {
	printf("Could not enter image store directory\n");
	return;
    }

    if (pc_stat(name, &stat) == 0) {
	if (verify_action("Delete image '%s'", name)) {
	    pc_get_attributes(name, &attr);
	    pc_set_attributes(name, attr &= ~ARDONLY);
	    if (pc_unlink(name) == NO) {
		printf("Error deleting image\n");
	    }
	}
    } else {
	printf("No image '%s' found\n", name);
    }
}

static bool
disk_init(void)
{
    static int init = 0;
    
    if (!init) {
	if (pc_system_init(0) != YES) {
	    printf("FS: Could not initialize disk\n");
	    return false;
	}
        init = 1;
    }
    return true;
}

void
do_dis(int argc, char *argv[])
{
    struct cmd *cmd;

    if (argc < 2) {
        dis_usage("too few arguments");
        return;
    }
    if (!disk_init()) return;
    if ((cmd = cmd_search(__DIS_cmds_TAB__, &__DIS_cmds_TAB_END__, 
                          argv[1])) != (struct cmd *)0) {
        (cmd->fun)(argc, argv);
        return;
    }
    dis_usage("unrecognized command");
}

#ifdef CYGIMP_REDBOOT_CONFIG_STORE_DISK
#include <config.h>

bool
cs_init(void)
{
    return disk_init();
}

void
cs_write_config(struct _config *config)
{
    PCFD fd;
    UCOUNT i;
    UINT16 attr;
    
    // Make file writable
    pc_get_attributes(IMAGE_STORE_NAME "/" CONFIG_DATA_NAME, &attr);
    pc_set_attributes(IMAGE_STORE_NAME "/" CONFIG_DATA_NAME, attr &= ~ARDONLY);

    // Load data
    if ((fd = po_open(IMAGE_STORE_NAME "/" CONFIG_DATA_NAME, PO_CREAT|PO_WRONLY, PS_IWRITE)) < 0) {
	printf("Error opening config data: %d\n", fd);
    }
    else {
	if ((i = po_write(fd, (UCHAR *)config, sizeof(*config))) != (UCOUNT)sizeof(*config)) {
	    printf("Error writing config data: %d\n", i);
	}
	po_close(fd);
    }

    // Make file unwritable
    pc_set_attributes(IMAGE_STORE_NAME "/" CONFIG_DATA_NAME, attr |= ARDONLY);
}

void
cs_load_config(struct _config *config)
{
    PCFD fd;
    UCOUNT i;
    
    if ((fd = po_open(IMAGE_STORE_NAME "/" CONFIG_DATA_NAME, PO_RDONLY, PS_IREAD)) < 0) {
	printf("Error opening config data: %d\n", fd);
    }
    else {
	if ((i = po_read(fd, (UCHAR *)config, sizeof(*config))) != (UCOUNT)sizeof(*config)) {
	    printf("Error reading config data: %d\n", i);
	}
	po_close(fd);
    }
}

#endif
