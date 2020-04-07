//==========================================================================
//
//      dharma.c
//
//      SST Flash programming
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
// Author(s):    tlindh
// Contributors: tlindh
// Date:         2001-02-14
// Purpose:      
// Description:  
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#include <pkgconf/hal.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>

#include <cyg/infra/diag.h> // diag_printf

#define  _FLASH_PRIVATE_
#include <cyg/io/flash.h>

#include "dharma.h"

#define _si(p) ((p[1]<<8)|p[0])

extern void diag_dump_buf(void *buf, CYG_ADDRWORD len);

extern int strncmp(const char *s1, const char *s2, int len);

int
flash_hwr_init(void)
{
    struct FLASH_query data, *qp;
    extern char flash_query, flash_query_end;
    typedef int code_fun(struct FLASH_query *);
    code_fun *_flash_query;
    int code_len, stat, num_regions, region_size, buffer_size;

    //  diag_printf("SST Flash Initializing...");

    // Copy 'program' code to RAM for execution
    code_len = (unsigned long)&flash_query_end - (unsigned long)&flash_query;
    _flash_query = (code_fun *)flash_info.work_space;
    memcpy(_flash_query, &flash_query, code_len);
    HAL_DCACHE_SYNC();  // Should guarantee this code will run
    HAL_ICACHE_DISABLE(); // is also required to avoid old contents

    stat = (*_flash_query)((struct FLASH_query *)&data);
    HAL_ICACHE_ENABLE();

    qp = &data;

    // only 2 supported flash parts right now

    // a braver man than I could get this info out
    // of the CFI block on the flash, but that just
    // isn't worth the time right now.

    if(qp->manuf_code == FLASH_SST_code) {
	
      if(qp->device_code == FLASH_SST_160_code) {
	// total size = 2mb
	// blocks = 512, size = 4kb
	//	diag_printf(" SSTVF/LF160 Found!\n");
	region_size = 4*1024;
	num_regions = 512;
	buffer_size = 0;

      } 
      else if(qp->device_code == FLASH_SST_800A_code) {
	// total size = 1mb
	// blocks = 256, size = 4kb
	//	diag_printf(" SSTVF/LF800A Found!\n");
	region_size = 4*1024;
	num_regions = 256;
	buffer_size = 0;



      }
      else {
	// error, unsupported SST flash part
	goto flash_type_unknown;
      }
      
    }
    else {
      // error, manufacturer ID failed
      goto flash_type_unknown;
    }
	 
    
     flash_info.block_size = region_size*CYGNUM_FLASH_DEVICES;
     flash_info.buffer_size = buffer_size;
     flash_info.blocks = num_regions; 
     flash_info.start = (void *)CYGNUM_FLASH_BASE;
     flash_info.end = (void *)(CYGNUM_FLASH_BASE +
			       (num_regions*region_size*CYGNUM_FLASH_DEVICES));
#if 0
	diag_printf("Flash Found -  %x, dev %x, blocksize %x buffersize %x blocks %x start %x end %x\n",
           qp->manuf_code, qp->device_code,flash_info.block_size,
	       flash_info.buffer_size,flash_info.blocks,flash_info.start,flash_info.end );

#endif

        return FLASH_ERR_OK;

 flash_type_unknown:
    diag_printf("Can't identify FLASH, sorry, man %x, dev %x\n",
           qp->manuf_code, qp->device_code);   
    return FLASH_ERR_HWR;
}

// Map a hardware status to a package error
int
flash_hwr_map_error(int err)
{
    if (err & FLASH_ErrorMask) {
        diag_printf("Err = %x\n", err);
        if (err & FLASH_ErrorProgram)
            return FLASH_ERR_PROGRAM;
        else if (err & FLASH_ErrorErase)
            return FLASH_ERR_ERASE;
        else 
            return FLASH_ERR_HWR;  // FIXME
    } else {
        return FLASH_ERR_OK;
    }
}

// See if a range of FLASH addresses overlaps currently running code
bool
flash_code_overlaps(void *start, void *end)
{
    extern char _stext[], _etext[];

    return ((((unsigned long)&_stext >= (unsigned long)start) &&
             ((unsigned long)&_stext < (unsigned long)end)) ||
            (((unsigned long)&_etext >= (unsigned long)start) &&
             ((unsigned long)&_etext < (unsigned long)end)));
}

// EOF strata.c
