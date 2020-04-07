//==========================================================================
//
//      flash_query.c
//
//      Flash programming - query device
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
#include CYGHWR_MEMORY_LAYOUT_H

//
// CAUTION!  This code must be copied to RAM before execution.  Therefore,
// it must not contain any code which might be position dependent!
//

#define CNT 20*1000*10  // Approx 20ms

int
flash_query(struct FLASH_query *data)
{
    volatile flash_t *ROM;
    int i, cnt;
    int cache_on;

    HAL_DCACHE_IS_ENABLED(cache_on);
    if (cache_on) {
        HAL_DCACHE_SYNC();

	// invalidate dcache entries
	HAL_DCACHE_INVALIDATE_ALL();
        HAL_DCACHE_DISABLE();
    }

    ROM = FLASH_P2V(CYGNUM_FLASH_BASE);

    // enter query mode
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x00AA;
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0x5554) = 0x0055;
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x0090;

   // wait a little bit
   i = 1000;
   while(i > 0)
     i--;
   
   data->manuf_code =  ROM[0]; //*FLASH_P2V(CYGNUM_FLASH_BASE | 0x0000);
   data->device_code = ROM[1]; //*FLASH_P2V(CYGNUM_FLASH_BASE | 0x0001);

   // exit query mode
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x00AA;
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0x5554) = 0x0055;
   *FLASH_P2V(CYGNUM_FLASH_BASE | 0xAAAA) = 0x00F0;

    if (cache_on) {
        HAL_DCACHE_ENABLE();
     }

    return 0;
}
