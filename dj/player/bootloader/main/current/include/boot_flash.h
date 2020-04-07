/////////////////////////////////////////////////////////////////
// File Name: boot_flash.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for redboot flash commands
// Usage: Call boot_flash_init() before any of the commands.
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

#ifndef _BOOT_FLASH_H_
#define _BOOT_FLASH_H_

#define MAXFLASHLEN 64 // maximum flash name length


enum eImageType {itBin, itSfv, itGzip, itUnknown, itConfig };

typedef struct
{
  unsigned long ulBase;
  unsigned long ulSize;
  eImageType eType;
  char * szName;
} FlashInfo;
  
class CBootFlash  {
 public:
  CBootFlash();
  virtual ~CBootFlash() {}
  FlashInfo* GetFirstImage();
  FlashInfo* GetNextImage();
  bool DeleteImage(const char * szImageName);
  bool WriteImage(const char *szImageName,unsigned long ulSourceRam,unsigned long ulSourceSize);
  bool FindFree(unsigned long *addr, unsigned long length);  
  virtual void ProgressCallback(int iPercent) {};

 private:
  static char m_szFlashReturnBuffer[MAXFLASHLEN]; // used to return strings
  FlashInfo m_fi;
  

  // Local data used by these routines (from flash.c)
  void *flash_start, *flash_end;
 
  int block_size, blocks;
  void *fis_work_block;
  struct fis_image_desc *m_pfis;
  int m_fiscount;
  void *fis_base, *err_addr;
  void *fis_addr;
};

#if 0
// configure flash, format if uninitialized
void boot_flash_init();

// get first flash image info
int boot_flash_gfirst(FlashInfo *pfi);

// get next flash image info
int boot_flash_gnext(FlashInfo *pfi);

// cleanup flashinfo struct
int boot_flash_gdone(FlashInfo *pfi);

// delete a flash image
void boot_flash_delete_image(const char * szImageName);

// write new flash image from ram
int boot_flash_write_image
// get flash free space
int boot_flash_get_free_space();

// verify from flash to RAM
int boot_flash_verify(FlashInfo *pfi,unsigned long ulMembase);

// uncompress from flash to RAM
int boot_flash_ungz(FlashInfo *pfi,unsigned long ulMembase);
#endif


#endif /* _BOOT_FLASH_H_ */
