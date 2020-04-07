/////////////////////////////////////////////////////////////////
// File Name: bootloader.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Contains generic/universal definitions for Dadio boot loader
// Usage: Call boot_init() to initialize the serial console. 
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
//////////////////////////////////////////////////////////////////

#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

// redboot includes
#define  DEFINE_VARS


#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_cache.h>
#include <fs/fat/sdapi.h>
#include CYGHWR_MEMORY_LAYOUT_H

#include <cyg/hal/hal_tables.h>
#ifdef CYGBLD_HAL_PLATFORM_STUB_H
#include CYGBLD_HAL_PLATFORM_STUB_H
#else
#include <cyg/hal/plf_stub.h>
#endif

#include <bootloader/main/boot_ide.h>
#include <bootloader/main/boot_mem.h>
// #include <bootloader/main/boot_sfv.h>



void boot_init(); // init serial, etc.
void boot_exec(unsigned long ulJumpAddr);

#endif /* _BOOTLOADER_H_ */
