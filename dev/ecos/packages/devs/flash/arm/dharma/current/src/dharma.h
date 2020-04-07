#ifndef CYGONCE_DEVS_FLASH_INTEL_STRATA_FLASH_H
#define CYGONCE_DEVS_FLASH_INTEL_STRATA_FLASH_H
//==========================================================================
//
//      strata.h
//
//      strataFlash programming - device constants, etc.
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

//#include <pkgconf/system.h>

// physical structure of flash parts
#include <cyg/io/dharma_strataflash.inl>

// ------------------------------------------------------------------------
// 
// It is expected that the above include defined all the properties of the
// device we want to drive: the choices this module supports include:
//
//                                 Buffered  Read     Block
//                                  write    query    locking
// 28FxxxB3 - Bootblock            - no      no       no
// 28FxxxC3 - StrataFlash          - no      yes      yes
// 28FxxxJ3 - Advanced StrataFlash - yes     yes      yes
// 
// These options are controlled by defining or not, in that include file,
// these symbols (not CDL options, just symbols - though they could be CDL
// in future)
//         CYGOPT_FLASH_IS_BOOTBLOCK     - for xxxB3 devices.
//         CYGOPT_FLASH_IS_NOT_ADVANCED  - for xxxC3 devices.
//         neither                       - for xxxJ3 devices.  
// (Advanced seems to be usual these days hence the sense of that opt)
//
// Other properties are controlled by these symbols:
//         CYGNUM_FLASH_DEVICES 	number of devices across the databus
//         CYGNUM_FLASH_WIDTH 	        number of bits in each device
//         CYGNUM_FLASH_BLANK           1 if blank is allones, 0 if 0
//         CYGNUM_FLASH_BASE 	        base address
//         CYGNUM_FLASH_BASE_MASK       a mask to get base address from any
// 
// for example, a 32-bit memory could be made from 1x32bit, 2x16bit or
// 4x8bit devices; usually 16bit ones are chosen in practice, so we would
// have CYGNUM_FLASH_DEVICES = 2, and CYGNUM_FLASH_WIDTH = 16.  Both
// devices would be handled simulataneously, via 32bit bus operations.
// Some CPUs can handle a single 16bit device as 32bit memory "by magic".
// In that case, CYGNUM_FLASH_DEVICES = 1 and CYGNUM_FLASH_WIDTH = 16, and
// the device is managed using only 16bit bus operations.


// ------------------------------------------------------------------------
//
// No mapping on this target - but these casts would be needed if some
// manipulation did occur.  An example of this might be:
// // First 4K page of flash at physical address zero is
// // virtually mapped at address 0xa0000000.
// #define FLASH_P2V(x) ((volatile flash_t *)(((unsigned)(x) < 0x1000) ?
//                            ((unsigned)(x) | 0xa0000000) :
//                            (unsigned)(x)))

#ifndef FLASH_P2V
#define FLASH_P2V( _a_ ) ((volatile flash_t *)((unsigned int)(_a_)))
#endif

// ------------------------------------------------------------------------
//
// This generic code is intended to deal with all shapes and orientations
// of Intel StrataFlash.  Trademarks &c belong to their respective owners.
//
// It therefore needs some trickery to define the constants and accessor
// types that we use to interact with the device or devices.
//
// The assumptions are that
//  o Parallel devices, we write to, with the "opcode" replicated per
//    device
//  o The "opcode" and status returns exist only in the low byte of the
//    device's interface regardless of its width.
//  o Hence opcodes and status are only one byte.
// An exception is the test for succesfully erased data.
//
// ------------------------------------------------------------------------

#if 8 == CYGNUM_FLASH_WIDTH

# if 1 == CYGNUM_FLASH_DEVICES
#  define FLASHWORD( k ) ((flash_t)(k)) // To narrow a 16-bit constant
typedef unsigned char flash_t;
# elif 2 == CYGNUM_FLASH_DEVICES
// 2 devices to make 16-bit
#  define FLASHWORD( k ) ((k)+((k)<<8))
typedef unsigned short flash_t;
# elif 4 == CYGNUM_FLASH_DEVICES
// 4 devices to make 32-bit
#  define FLASHWORD( k ) ((k)+((k)<<8)+((k)<<16)+((k)<<24))
typedef unsigned long flash_t;
# elif 8 == CYGNUM_FLASH_DEVICES
// 8 devices to make 64-bit - intermediate requires explicit widening
#  define FLASHWORD32( k ) ((flash_t)((k)+((k)<<8)+((k)<<16)+((k)<<24)))
#  define FLASHWORD( k ) (FLASHWORD32( k ) + (FLASHWORD32( k ) << 32));
typedef unsigned long long flash_t;
# else
#  error How many 8-bit flash devices?
# endif

#elif 16 == CYGNUM_FLASH_WIDTH

# if 1 == CYGNUM_FLASH_DEVICES
#  define FLASHWORD( k ) (k)
typedef unsigned short flash_t;
# elif 2 == CYGNUM_FLASH_DEVICES
// 2 devices to make 32-bit
#  define FLASHWORD( k ) ((k)+((k)<<16))
typedef unsigned long flash_t;
# elif 2 == CYGNUM_FLASH_DEVICES
// 4 devices to make 64-bit - intermediate requires explicit widening
#  define FLASHWORD32( k ) ((flash_t)((k)+((k)<<16)))
#  define FLASHWORD( k ) (FLASHWORD32( k ) + (FLASHWORD32( k ) << 32));
typedef unsigned long long flash_t;
# else
#  error How many 16-bit flash devices?
# endif

#elif 32 == CYGNUM_FLASH_WIDTH

# if 1 == CYGNUM_FLASH_DEVICES
#  define FLASHWORD( k ) (k)
typedef unsigned long flash_t;
# elif 2 == CYGNUM_FLASH_DEVICES
// 2 devices to make 64-bit - intermediate requires explicit widening
#  define FLASHWORD32( k ) ((flash_t)(k))
#  define FLASHWORD( k ) (FLASHWORD32( k ) + (FLASHWORD32( k ) << 32));
typedef unsigned long long flash_t;
# else
#  error How many 32-bit flash devices?
# endif

#else
# error What flash width?
#endif

// Data (not) that we read back:
#if 0 == CYGNUM_FLASH_BLANK
# define FLASH_BlankValue ((flash_t)0)
#elif 1 == CYGNUM_FLASH_BLANK
# define FLASH_BlankValue ((flash_t)(-1ll))
#else
# error What blank value?
#endif

// ------------------------------------------------------------------------

#define FLASH_Read_ID      		FLASHWORD( 0x90 )
#ifndef CYGOPT_FLASH_IS_BOOTBLOCK
#define FLASH_Read_Query   		FLASHWORD( 0x98 ) // Strata only
#endif
// Status register cmd and bits
#define FLASH_Read_Status  		FLASHWORD( 0x70 )
#define FLASH_Clear_Status 		FLASHWORD( 0x50 )
#define FLASH_Status_Ready 		FLASHWORD( 0x80 )

#ifdef CYGOPT_FLASH_IS_BOOTBLOCK
#define FLASH_Program      		FLASHWORD( 0x40 ) // BootBlock only
#else
#define FLASH_Program      		FLASHWORD( 0x10 )
#endif
#define FLASH_Block_Erase  		FLASHWORD( 0x20 )
#ifndef CYGOPT_FLASH_IS_BOOTBLOCK
//#ifndef CYGOPT_FLASH_IS_NOT_ADVANCED
// dc- the write buffer command is available on regular strata parts. sweet!
//  additionally, the docs indicate that you need to read the extended status register
//  and determine if the write buffer is available after issuing the write to buffer
//  cmd. however, on the same page they indicate that the same command is not needed.
//  this is based on aug 2001 29066709.pdf 3V Intel strataflash 28F320J3A docs
#define FLASH_Write_Buffer 		FLASHWORD( 0xE8 ) // *Advanced* Strata only
//#endif // flash is advanced ie. has Write Buffer command

#define FLASH_Set_Lock     		FLASHWORD( 0x60 ) // Strata only
#define FLASH_Set_Lock_Confirm 		FLASHWORD( 0x01 ) // Strata only
#define FLASH_Clear_Locks  		FLASHWORD( 0x60 ) // Strata only
#define FLASH_Clear_Locks_Confirm	FLASHWORD( 0xD0 ) // Strata only
#endif
#define FLASH_Confirm      		FLASHWORD( 0xD0 )
//#define FLASH_Configure    			FLASHWORD( 0xB8 )
//#define FLASH_Configure_ReadyWait      	FLASHWORD( 0x00 )
//#define FLASH_Configure_PulseOnErase   	FLASHWORD( 0x01 )
//#define FLASH_Configure_PulseOnProgram 	FLASHWORD( 0x02 )
//#define FLASH_Configure_PulseOnBoth    	FLASHWORD( 0x03 )
#define FLASH_Reset        		FLASHWORD( 0xFF )
                                                     
// Status that we read back:                         
#define FLASH_ErrorMask			FLASHWORD( 0x7E )
#define FLASH_ErrorProgram		FLASHWORD( 0x10 )
#define FLASH_ErrorErase		FLASHWORD( 0x20 )

#define FLASH_ErrorNotVerified		FLASHWORD( 0x9910 ) // made-up number

// ------------------------------------------------------------------------

#define FLASH_Intel_code   0x89 // NOT mapped to 16+16

// Extended query information
struct FLASH_query {
    unsigned char manuf_code;    // FLASH_Intel_code
    unsigned char device_code;
    unsigned char _unused0[14];
    unsigned char id[3];  // Q R Y
    unsigned char _unused1[20];
    unsigned char device_size;
    unsigned char device_interface[2];
    unsigned char buffer_size[2];
    unsigned char is_block_oriented;
    unsigned char num_regions[2];
    unsigned char region_size[2];
};

#endif  // CYGONCE_DEVS_FLASH_INTEL_STRATA_FLASH_H
// ------------------------------------------------------------------------
// EOF strata.h
