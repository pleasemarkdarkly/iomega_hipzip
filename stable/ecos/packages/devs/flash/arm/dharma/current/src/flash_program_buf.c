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

//#undef FLASH_Write_Buffer //FIXME: strataflash buffer write currently broken 


int
flash_program_buf(volatile flash_t *addr, flash_t *data, int len,
                  unsigned long block_mask, int buffer_size)
{
    volatile flash_t * ROM;
    flash_t stat = 0;
    int timeout = 50000;
    int cache_on;

#ifdef FLASH_Write_Buffer
    volatile flash_t * BA;
    int i, wc, bc;
#endif

    HAL_DCACHE_IS_ENABLED(cache_on);
    if (cache_on) {
        HAL_DCACHE_SYNC();
        HAL_DCACHE_DISABLE();
        HAL_DCACHE_INVALIDATE_ALL();
    }

    // Get base address and map addresses to virtual addresses
    
    ROM = FLASH_P2V( CYGNUM_FLASH_BASE_MASK & (unsigned int)addr );

#ifdef FLASH_Write_Buffer
    BA = FLASH_P2V( block_mask & (unsigned int)addr );
#endif

    addr = FLASH_P2V(addr);

    // Clear any error conditions if ones exist. Make sure to use FLASH_Read_Status command
    ROM[0] = FLASH_Read_Status;
    
    if(ROM[0] & FLASH_ErrorMask)
    	ROM[0] = FLASH_Clear_Status;

    // explicitly reset flash before starting write sequence
    ROM[0] = FLASH_Reset;

#ifdef FLASH_Write_Buffer
    bc = buffer_size / (CYGNUM_FLASH_DEVICES*2);

    // dc- Process all the data in using Write_Buffer, looping
    //     until we run out of things to write
    while (len > 0) {

        // Calculate the number of words to write, possibly less
        //  than the number of words in the write buffer
        if( len < buffer_size ) {
            // this forces a round up
            wc = len + ((CYGNUM_FLASH_DEVICES*2)-1);
        }
        else {
            wc = buffer_size;
        }
        wc = wc / (CYGNUM_FLASH_DEVICES*2);  // Word count


	timeout = 5000000;

#if 0
        // Issue the command until the write buffer is available
        do {
            if( --timeout == 0 ) {
                goto bad;
            }

            *BA = FLASH_Write_Buffer;
        } while( ((stat = ROM[0]) & FLASH_Status_Ready) != FLASH_Status_Ready );
#else
	// issue write to buffer command e8h, at block address
        *BA = FLASH_Write_Buffer;

	// while buffer not available, resend command
        while(((stat = ROM[0]) & FLASH_Status_Ready) != FLASH_Status_Ready) {
            if (--timeout == 0) {
                goto bad;
            }
            *BA = FLASH_Write_Buffer;
        }
#endif
        
        // dc- specify the number of words to write out, such that
        // the value we pass is implicitly one less then the number
        // of words we will write. sweet!
        
        *BA = FLASHWORD(wc-1);  // Count is 0..N-1


        for( i = 0; i < wc; i++ ) {
            *addr++ = *data++;
        }
        
        // 0-fill partial blocks
        if( wc < bc ) {
            for( ; i < bc; i++ ) {
                *addr++ = 0;
            }
        }

        // Issue the program confirm command
        *BA = FLASH_Confirm;
    
        timeout = 5000000;

#if 0
        // Loop on the status until the ready mask is set
        do {
            if( --timeout == 0 ) {
                goto bad;
            }

            ROM[0] = FLASH_Read_Status;
        } while( ((stat = ROM[0]) & FLASH_Status_Ready) != FLASH_Status_Ready );
        
#else
        ROM[0] = FLASH_Read_Status;
        while(((stat = ROM[0]) & FLASH_Status_Ready) != FLASH_Status_Ready) {
            if (--timeout == 0) {
                goto bad;
            }
            // reread the status here
            ROM[0] = FLASH_Read_Status;
        }
#endif
        
        if( (stat & FLASH_ErrorMask) )
            ROM[0] = FLASH_Clear_Status;

        // add the number of words we wrote
        // this should be safe pointer arith here
        BA += wc;
        
        // subtract the number of bytes we wrote
	len -= buffer_size;
    }
#else
    // this code should be mutually exclusive - that is, if FLASH_Write_Buffer
    //  is available, that should be used in preference to FLASH_Program
    while (len > 0) {
        *addr = FLASH_Program;
        *addr = *data;

	
        timeout = 5000000; // FIXME: use processor timers here instead of crappy loops?

        ROM[0] = FLASH_Read_Status;  
        do {
            if (--timeout == 0) {
	       goto bad; 
            }
        } while(((stat = ROM[0]) & FLASH_Status_Ready) != FLASH_Status_Ready);

	// error check only valid when stat & FLASH_Status_Ready
	ROM[0] = FLASH_Read_Status;
        if ( ((stat = ROM[0]) & FLASH_ErrorMask) ) {
            break;
        }

	// check each word to ensure valid writes
        //      ROM[0] = FLASH_Reset;            
        //        if (*addr++ != *data++) {
        //            stat = FLASH_ErrorNotVerified;
        //            break;
        //        }
        len -= sizeof( flash_t );
    }
#endif // FLASH_Write_Buffer

 bad:
    // should re-enable read access
    ROM[0] = FLASH_Reset;            

    if (cache_on) {
        HAL_DCACHE_ENABLE();
    }

    return stat;
}
