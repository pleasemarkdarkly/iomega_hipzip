/////////////////////////////////////////////////////////////////
// File Name: boot_ide.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for redboot ide commands
// Usage: Call boot_ide_init() before any of the commands.
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
/////////////////////////////////////////////////////////////////

#ifndef _BOOT_IDE_H_
#define _BOOT_IDE_H_

#include <fs/fat/sdapi.h>
#include <bootloader/main/boot_types.h>
// #include <redboot.h>

#define MAXPATHLEN 64 // maximum HD file name length

// #define IDEDEVICENO 0 // master 
// #define IDEDEVICENO 0 // slave


class CBootDisk {

 public:
  CBootDisk();
  ~CBootDisk();

  DSTAT* GetFirstImage();
  DSTAT* GetNextImage();
  void InitDisk();
  bool ReadFile(const char * szPath, void * pBase,unsigned long * pulRead); 
  bool WriteFile(const char * szPath, void * pBase, unsigned long ulSize);
  bool DeleteFile(const char * szPath);
  void Close();
 private:
  
  static char szPathReturnBuffer[MAXPATHLEN]; 
  DSTAT m_dsFile;
  bool m_bFormat;

};

#endif /* _BOOT_IDE_H_ */
