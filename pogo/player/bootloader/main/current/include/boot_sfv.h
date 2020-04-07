/////////////////////////////////////////////////////////////////
// File Name: boot_sfv.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper class for Intertrust 
// Usage: call Decrypt on memory buffer. Very simple, wrapped for consistency
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
////////////////////////////////////////////////////////////////
#ifndef _BOOT_SFV_H_
#define _BOOT_SFV_H_

#include <bootloader/main/bootloader.h>


#define SWAP_32(x) (((x) >> 24) | ((x) << 24) | (((x) & 0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))
#define MAJOR_VERSION(x) (x >> 16)
#define MINOR_VERSION(x) (x & 0xffff)

/* firmware module */
typedef struct metadata_s 
{
    unsigned int crc;
    unsigned int len;
    unsigned short version_major;
    unsigned short version_minor;
} metadata_t;


class CBootIntertrust
{
  CBootIntertrust();
  ~CBootIntertrust();

  bool Decrypt(void* pBase, unsigned long ulSize, 
	       void* pTarget, unsigned long *newsize);

  
};

#endif /* _BOOT_SFV_H_ */
