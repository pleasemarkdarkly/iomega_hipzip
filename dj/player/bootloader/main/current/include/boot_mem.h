/////////////////////////////////////////////////////////////////
// File Name: boot_mem.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Memory management tools
// Usage: used to work top-down to generate images in memory
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
//////////////////////////////////////////////////////////////////
#ifndef _BOOT_MEM_H_
#define _BOOT_MEM_H_

#define IMAGEBASE 0x20000
#define IMAGESTART 0x20040
#define MAXIMAGES 16 // maximum entries in memory

#include <bootloader/main/boot_types.h>



externC _decompress_fun_init* _dc_init;
externC _decompress_fun_inflate* _dc_inflate;
externC _decompress_fun_close* _dc_close;

// intertrust firmware module 
typedef struct metadata_s 
{
    unsigned int crc;
    unsigned int len;
    unsigned short version_major;
    unsigned short version_minor;
} metadata_t;

#define SWAP_32(x) (((x) >> 24) | ((x) << 24) | (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))
#define MAJOR_VERSION(x) (x >> 16)
#define MINOR_VERSION(x) (x & 0xffff)

class CBootMemory {

 public:
  CBootMemory(void *pTop,void *pBottom);
  ~CBootMemory();
  void* FreeBlock(); 
  unsigned int FreeSize(); // free k in ram
  unsigned int BlockCount() {return m_iCount;}

  // retrieve block information from index
  void* GetBlock(int i) {return m_rgpImages[i];}
  unsigned long GetSize(int i) {return m_rguiSize[i];}
  
  
  void NotifyImage(unsigned int Size); // say you used ram, so pFree can be adjusted
  
  void Exec(); // relocate last image to base, jumps to imagestart

  bool Decompress(); // decompress last image
  bool Verify(); // verify last image
  // GetVersion(); // version of last image
 static int UnGzip(void * pBase, unsigned long ulSize, void * pTarget, unsigned long * pulWritten);
  int UnSFV(void* pBase, unsigned long ulSize, void* pTarget, unsigned long *newsize);

 protected:

  // contains full implementations of gzip, sfv, 
  // sfvver without any assumptions

  // friend classes can use these to read small sections safely into RAM
  // flash just directly wraps these, ide can wrap SfvVer for quicker ver reads
 
  //  SfvVer();

 private:

  int m_iCount;
  void *m_pFree;
  int m_iFree;
  void *m_pTop;
  void *m_pBottom;

  void *m_rgpImages[MAXIMAGES];
  unsigned long m_rguiSize[MAXIMAGES];

};

#endif
