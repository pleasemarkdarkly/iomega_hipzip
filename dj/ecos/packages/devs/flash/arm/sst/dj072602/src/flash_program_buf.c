//==========================================================================
//
//      flash_program_buf.c
//
//      Flash programming
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
// Author(s):    gthomas, hmt
// Contributors: gthomas
// Date:         2001-02-14
// Purpose:      
// Description:  
//              
//####DESCRIPTIONEND####
//
//==========================================================================

#include "dharma.h"

#include <pkgconf/hal.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_cache.h>

//
// CAUTION!  This code must be copied to RAM before execution.  Therefore,
// it must not contain any code which might be position dependent!
//


int
flash_program_buf(volatile flash_t *addr, flash_t *data, int len,
                  unsigned long block_mask, int buffer_size)
{


    volatile flash_t *ROM;
    volatile flash_t *BA;
    flash_t stat = 0;
    int timeout = 50000;
    int cache_on;
    flash_t test;
    int i;


    HAL_DCACHE_IS_ENABLED(cache_on);
    if (cache_on) {
        HAL_DCACHE_SYNC();
	HAL_DCACHE_INVALIDATE_ALL();
        HAL_DCACHE_DISABLE();
    }

    // Get base address and map addresses to virtual addresses
    
    ROM = FLASH_P2V( CYGNUM_FLASH_BASE_MASK & (unsigned int)addr );    
    BA = FLASH_P2V( block_mask & (unsigned int)addr );
    addr = FLASH_P2V(addr);


    while (len > 0) {

      // program
      *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x00AA;
      *FLASH_P2V(CYGNUM_FLASH_BASE | 0x5554) = 0x0055;
      *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x00A0;
      *addr = *data;
	

      timeout = 5000;
      test = *data & 0xFFFF;
      while(*addr != test) {
	if (--timeout == 0) {
	  goto bad; 
	}
      }

      if (*addr++ != *data++) {
	stat = FLASH_ErrorProgram;
	break;
      }
      len -= sizeof( flash_t );
    }
    
    return stat;

 bad:
    
    if (cache_on) {
      HAL_DCACHE_ENABLE();
    }
    
    return FLASH_ErrorProgram;
}
