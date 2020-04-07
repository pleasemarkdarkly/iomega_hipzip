/////////////////////////////////////////////////////////////////
// File Name: boot_compress.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for Redboot GZip tools
// Usage: TBD
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
//////////////////////////////////////////////////////////////////

#ifndef _BOOT_COMPRESS_H_
#define _BOOT_COMPRESS_H_


int boot_gzip_init();
int boot_mem_ungz(void *pBase, unsigned long ulSize, void *pTarget, unsigned long * pulWritten);

#endif /* _BOOT_COMPRESS_H_ */

