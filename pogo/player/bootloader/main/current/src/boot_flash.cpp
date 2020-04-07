/////////////////////////////////////////////////////////////////
// File Name: boot_flash.cpp
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for flash commands
// Usage: 
// These provide abstracted wrappings of the flash file system
// manipulation commands. If you want to read from flash, just
// get the addresses and read/copy it directly.
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
/////////////////////////////////////////////////////////////////

#include <bootloader/main/boot_flash.h>
#include "fis.h"
#include <cyg/io/flash.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <bootloader/main/boot_types.h>
#define MAXFLASHLEN 64 // maximum flash name length

static char szFlashReturnBuffer[MAXFLASHLEN]; // used to return strings

const char *szBootName = "boot";
/*

workspace_end
FLASH_MIN_WORKSPACE
int flash_init();
int flash_get_limits();
int flash_load_block_info();
memset
erase_start
erase_size
err_addr



*/ 
extern unsigned char *workspace_start, *workspace_end;
static inline void*
memcpy(void* dest, void* src, int size)
{
    unsigned char* __d = (unsigned char*) dest;
    unsigned char* __s = (unsigned char*) src;

    while(size--)
        *__d++ = *__s++;

    return dest;
}

static inline void*
memset(void* s, int c, int size)
{
    unsigned char* __s = (unsigned char*) s;
    unsigned char __c = (unsigned char) c;

    while(size--)
        *__s++ = __c;

    return s;
}

// End local data

// configure flash, format if uninitialized
CBootFlash::CBootFlash()
{

  // do_flash_init code (TODO: try and eliminate more parts of redboot/src)
  int stat;
  struct fis_image_desc *img;  
  unsigned long redboot_image_size, redboot_flash_start;
  unsigned long erase_start, erase_size;

 

  //#define MIN_REDBOOT_IMAGE_SIZE CYGBLD_REDBOOT_MIN_IMAGE_SIZE
  //redboot_image_size = block_size > MIN_REDBOOT_IMAGE_SIZE ? block_size : MIN_REDBOOT_IMAGE_SIZE;

  // check free space
  // 

    if ((stat = flash_init((void *)(workspace_end-FLASH_MIN_WORKSPACE), 
			   FLASH_MIN_WORKSPACE)) != 0) 
	{
      DEBUG_BOOTLOADER("FLASH: driver init failed!, status: 0x%x\n", stat);
    }


    flash_get_limits((void *)0, (void **)&flash_start, (void **)&flash_end);
    flash_get_block_info(&block_size, &blocks);

// use for bootloader testing on different platforms (2mb is much more managable)

#ifdef FLASH_SIZE_OVERRIDE

	flash_end = (void *)((unsigned long)flash_start + FLASH_SIZE_OVERRIDE);

#endif

#ifdef BLOCK_SIZE_OVERRIDE

	block_size = BLOCK_SIZE_OVERRIDE;
	blocks = ((unsigned long)flash_end - (unsigned long)flash_start) / block_size;

#endif

    DEBUG_BOOTLOADER("\nFLASH: 0x%x - 0x%x, %d blocks of 0x%x bytes each.\n", flash_start, flash_end, blocks, (void *)block_size);
    fis_work_block = (unsigned char *)(workspace_end-FLASH_MIN_WORKSPACE-block_size);
    workspace_end = (unsigned char *)fis_work_block;
   	redboot_flash_start = (unsigned long)flash_start;

#if 0

 // try and find FIS file system
  // FIXME: Make this actually reflect build size. 128k should be enough for now
  redboot_image_size = 0x40000;
  DEBUG_BOOTLOADER("warning: boot image size manually set to 0x%x\n",redboot_image_size);



  // first block should be = szBootName
  img = (struct fis_image_desc *)((unsigned long)flash_end - block_size);
  if(strcmp((char *)img->name,szBootName) != 0) {


    // reset fis area
    memcpy(fis_work_block, fis_addr, block_size);

    

    // create entry for exiting boot image  
    img = (struct fis_image_desc *)fis_work_block;
    memset(fis_work_block,0xFF,block_size);
    memset(img, 0, sizeof(*img));
    strcpy((char *)img->name, szBootName); // "DadioBoot"
    img->flash_base = redboot_flash_start;
    img->mem_base = redboot_flash_start;
    img->size = redboot_image_size;
    img++;

    redboot_flash_start += redboot_image_size;
    
    // create entry for FIS table
    memset(img, 0, sizeof(*img));
    strcpy((char *)img->name, "FIS directory");
    fis_base = (void *)((unsigned long)flash_end - block_size);
    img->flash_base = (unsigned long)fis_base;
    img->mem_base = (unsigned long)fis_base;
    img->size = block_size;
    img++;

    
    // format free blocks
    erase_start = redboot_flash_start; // high water of created images
    erase_size = (unsigned long)fis_base - erase_start; // the gap between HWM and fis data

    if (flash_erase((void *)erase_start, erase_size,
			    (void **)&err_addr) != 0) {
      // FIXME: init failed, should report it
      DEBUG_BOOTLOADER("flash init failed\n");
    }

    if (flash_erase(fis_base, block_size, (void **)&err_addr) != 0) {
      //      DEBUG_BOOTLOADER("   initialization failed %p: 0x%x(%s)\n", err_addr, stat, flash_errmsg(stat));
      // FIXME: replace with error ret
      DEBUG_BOOTLOADER("flash init failed\n");
      
    } else {
      if (flash_program(fis_base, fis_work_block, 
				(unsigned long)img - (unsigned long)fis_work_block,
			(void **)&err_addr) != 0) {
      DEBUG_BOOTLOADER("flash init failed\n");
      }

    }

  }
#endif

}

// get first flash image info (emulates file system usage)


FlashInfo* CBootFlash::GetFirstImage()
{
  // reset m_fisimg to base address
  m_pfis = (struct fis_image_desc *)((unsigned long)flash_end - block_size);
  m_fiscount = 0;

  while(m_fiscount < block_size/sizeof(*m_pfis)) {
    if(m_pfis->name[0] != 0xFF) {
      m_fi.ulBase = m_pfis->flash_base;
      m_fi.ulSize = m_pfis->size;
      m_fi.szName = (char *)m_pfis->name;
      return &m_fi;
    }
    m_pfis++;
    m_fiscount++;
  }
      
  return NULL;

}

// get next flash image info
FlashInfo* CBootFlash::GetNextImage()
{

  m_pfis++;
  m_fiscount++;

  while(m_fiscount < block_size/sizeof(*m_pfis)) {
    if(m_pfis->name[0] != 0xFF) {
      m_fi.ulBase = m_pfis->flash_base;
      m_fi.ulSize = m_pfis->size;
      m_fi.szName = (char *)m_pfis->name;
      return &m_fi;
    }
    m_pfis++;
    m_fiscount++;
  }
  
      
  return NULL;

}

// delete a flash image
bool CBootFlash::DeleteImage(const char * szImageName)
{
  int i;
  bool ret = false;
  struct fis_image_desc *img;
  void *fis_addr;
  
  fis_addr = (void *)((unsigned long)flash_end - block_size);
  img = (struct fis_image_desc *)fis_work_block;
  memcpy(fis_work_block, fis_addr, block_size);
  
  i = 0;
   
  // check all the images
  for ( /* i, img */;  i < block_size/sizeof(*img);  i++, img++) {
    if ((img->name[0] != (unsigned char)0xFF) && (strcmp(szImageName, (char *)img->name) == 0)) {
      ret = true;
      break;
    }
  }

  if(ret) {
    
    // Erase Data blocks (free space)
    if (flash_erase((void *)img->flash_base, img->size, (void **)&err_addr) != 0) {
        DEBUG_BOOTLOADER("Error erasing \n");
    }
    
    memset(img,0xFF,sizeof(*img));

    // remove from table
     
    if ((flash_erase((void *)fis_addr, block_size, (void **)&err_addr)) != 0) {
      DEBUG_BOOTLOADER("Error erasing \n");
      // Don't try to program if the erase failed
    } else {
      // Now program it
      if ((flash_program((void *)fis_addr, (void *)fis_work_block, block_size, (void **)&err_addr)) != 0) {
	DEBUG_BOOTLOADER("delete: Error programming fis table\n");
      }
    }

  }
  else {
    DEBUG_BOOTLOADER("delete - image not found\n");
  }
  
  return ret;

}

// write new flash image from ram
// fails if overwriting is a problem, make sure to delete images before hand
bool CBootFlash::WriteImage(const char *szImageName, // name for image
			    unsigned long ulSourceRam, // source location in RAM
			    unsigned long ulSourceSize // source size
			    ) 
{
  int i;
  unsigned long length;
  void* pFlashImage,*fis_addr;

  struct fis_image_desc* img;

  fis_addr = (void *)((unsigned long)flash_end - block_size);
  img = (struct fis_image_desc *)fis_work_block;
  memcpy(fis_work_block, fis_addr, block_size);

  // round length up to nearest block
  length = ulSourceSize;
  length = ((length + block_size - 1) / block_size) * block_size;
  if (length < ulSourceSize) {
    DEBUG_BOOTLOADER("Invalid FLASH image size/length combination\n");
    return false;
  }

  // check to make sure that it'll fit where we want it
  
  // make sure name doesn't already exist

  for (i = 0;  i < block_size/sizeof(*img);  i++, img++) {
    if ((img->name[0] != (unsigned char)0xFF) && (strcmp(szImageName, (char *)img->name) == 0)) {
      DEBUG_BOOTLOADER("create - duplicate image found");
      return false;
    }
  }

  // find free image, set params 
  img = (struct fis_image_desc *)fis_work_block;
  for (i = 0;  i < block_size/sizeof(*img);  i++, img++) {
    if(img->name[0] == (unsigned char)0xFF) {
      DEBUG_BOOTLOADER("create - image found");
      break;
    }
  }

  if(img->name[0] == 0xFF) {
    
    // check for free space
    if(FindFree((unsigned long *)&pFlashImage,length)) {
      
      // write image
      bool prog_ok = true;
      if (prog_ok) {
	  // Erase area to be programmed
	  if(flash_erase((void *)pFlashImage, length, (void **)&err_addr) != 0) {
	    DEBUG_BOOTLOADER("program: error erasing flash\n");
	    prog_ok = false;
	  }
        }
        if (prog_ok) {
            // Now program it
            if (flash_program((void *)pFlashImage, (void *)ulSourceRam, ulSourceSize, (void **)&err_addr) != 0) {
	      DEBUG_BOOTLOADER("program: error programming new image\n");
                prog_ok = false;
            }
        }

	// update fis directory
	memset(img, 0, sizeof(*img));
	strcpy((char *)img->name,szImageName);
	img->flash_base = (unsigned long)pFlashImage;
	img->mem_base = 0; // FIXME: do anything here? (for codecs, perhaps)
	img->entry_point = 0; // do anything here? (shouldn't be needed)
	img->size = length;
	img->data_length = ulSourceSize; // redundant?

	if (flash_erase((void *)fis_addr, block_size, (void **)&err_addr) != 0) {
	  DEBUG_BOOTLOADER("program: error erasing fis table\n");
	  // Don't try to program if the erase failed
	} else {
	  // Now program it
	  if(flash_program((void *)fis_addr, (void *)fis_work_block, block_size, (void **)&err_addr) != 0) {
	  DEBUG_BOOTLOADER("program: error programming fis table\n");
	  }
	}

    }
    else {
      DEBUG_BOOTLOADER("create: no image space available\n");
      return false;
    }

  }
  else {
    DEBUG_BOOTLOADER("create:!!PANIC!! no fis entries available\n");
    return false;
  }

  return true;


}


// Find the first unused area of flash which is long enough
bool CBootFlash::FindFree(unsigned long *addr, unsigned long length)
{
    unsigned long *fis_ptr, *fis_end;
    unsigned long *area_start;

    // Do not search the area reserved for pre-RedBoot systems:
    fis_ptr = (unsigned long *)((unsigned long)flash_start); // + CYGNUM_REDBOOT_FLASH_RESERVED_BASE);
    fis_end = (unsigned long *)(unsigned long)flash_end;
    area_start = fis_ptr;
    while (fis_ptr < fis_end) {
        if (*fis_ptr != (unsigned long)0xFFFFFFFF) {
            if (area_start != fis_ptr) {
                // Assume that this is something
                if ((fis_ptr-area_start) >= length) {
                    *addr = (unsigned long)area_start;
                    return true;
                }
            }
            // Find next blank block
            area_start = fis_ptr;
            while (area_start < fis_end) {
                if (*area_start == (unsigned long)0xFFFFFFFF) {
                    break;
                }
                area_start += block_size / sizeof(unsigned long);
            }
            fis_ptr = area_start;
        } else {
            fis_ptr += block_size / sizeof(unsigned long);
        }
    }
    if (area_start != fis_ptr) {
        if ((fis_ptr-area_start) >= length) {
            *addr = (unsigned long)area_start;
            return true;
        }
    }
    return false;
}

