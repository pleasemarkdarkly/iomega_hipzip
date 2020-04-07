#ifndef CYGONCE_DEVS_FLASH_SST_39_H
#define CYGONCE_DEVS_FLASH_SST_39_H
//==========================================================================
//
//      dharma.h
//
//      SST programming
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
#include <cyg/io/dharma_sst.inl>

// ------------------------------------------------------------------------
// 
// It is expected that the above include defined all the properties of the
// device we want to drive: the choices this module supports include:
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


#if 16 == CYGNUM_FLASH_WIDTH
# if 1 == CYGNUM_FLASH_DEVICES
#  define FLASHWORD( k ) (k)
typedef unsigned short flash_t;
#else
#error Only one SST flash chip is supported
#endif
#else
#error Only SST 16-bit wide access is supported 
#endif

// identifiers

#define FLASH_SST_code 0x00BF
#define FLASH_SST_160_code 0x2782
#define FLASH_SST_800A_code 0x2781


#define FLASH_ErrorMask 0x0001
#define FLASH_ErrorProgram 0x0002
#define FLASH_ErrorErase 0x0004


struct FLASH_query {
  unsigned short device_code;
  unsigned short manuf_code;
};

#endif  // CYGONCE_DEVS_FLASH_SST_39_H
// ------------------------------------------------------------------------
// EOF dharma.h
