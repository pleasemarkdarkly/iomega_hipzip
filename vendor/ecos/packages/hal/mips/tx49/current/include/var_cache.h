#ifndef CYGONCE_VAR_CACHE_H
#define CYGONCE_VAR_CACHE_H

//=============================================================================
//
//      var_cache.h
//
//      HAL cache control API
//
//=============================================================================
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
// Copyright (C) 1998, 1999, 2000 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   nickg
// Contributors:nickg, jskov
// Date:        2000-05-09
// Purpose:     Cache control API
// Description: The macros defined here provide the HAL APIs for handling
//              cache control operations.
// Usage:
//              #include <cyg/hal/imp_cache.h>
//              ...
//              
//
//####DESCRIPTIONEND####
//
//=============================================================================

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>

#include <cyg/hal/mips-regs.h>

#include <cyg/hal/plf_cache.h>

//=============================================================================
// Toshiba TX4955

#ifdef CYGPKG_HAL_MIPS_TX4955

//-----------------------------------------------------------------------------
// Cache dimensions

// Data cache
#define HAL_DCACHE_SIZE                 CYGHWR_HAL_DCACHE_SIZE // size in bytes
#define HAL_DCACHE_LINE_SIZE            32      // Size of a data cache line
#define HAL_DCACHE_WAYS                 4       // Associativity of the cache

// Instruction cache
#define HAL_ICACHE_SIZE                 CYGHWR_HAL_ICACHE_SIZE // size in bytes
#define HAL_ICACHE_LINE_SIZE            32      // Size of a cache line
#define HAL_ICACHE_WAYS                 4       // Associativity of the cache

#define HAL_DCACHE_SETS (HAL_DCACHE_SIZE/(HAL_DCACHE_LINE_SIZE*HAL_DCACHE_WAYS))
#define HAL_ICACHE_SETS (HAL_ICACHE_SIZE/(HAL_ICACHE_LINE_SIZE*HAL_ICACHE_WAYS))

#define HAL_MIPS_CACHE_INSN_USES_LSB

//-----------------------------------------------------------------------------
// Cache controls

// Register $16 is Config register (controls cache state)

// Config Register fields
#define CYGARC_REG_CONFIG_ICE      0x00020000 // Instruction cache enable
#define CYGARC_REG_CONFIG_DCE      0x00010000 // Data cache enable


//-----------------------------------------------------------------------------
// Global control of data cache

// Enable the data cache
// This uses a bit in the config register, which is TX49 specific.
#define HAL_DCACHE_ENABLE_DEFINED
#define HAL_DCACHE_ENABLE()                             \
    CYG_MACRO_START                                     \
    cyg_uint32 tmp;                                     \
    asm volatile ("mfc0 %0,$16;"                        \
                  "and  %0,%0,%1;"                      \
                  "mtc0 %0,$16;"                        \
                  : "=&r" (tmp)                         \
                  : "r" (~CYGARC_REG_CONFIG_DCE)        \
                 );                                     \
    CYG_MACRO_END

// Disable the data cache
#define HAL_DCACHE_DISABLE_DEFINED
#define HAL_DCACHE_DISABLE()                            \
    CYG_MACRO_START                                     \
    cyg_uint32 tmp;                                     \
    asm volatile ("mfc0 %0,$16;"                        \
                  "or   %0,%0,%1;"                      \
                  "mtc0 %0,$16;"                        \
                  : "=&r" (tmp)                         \
                  : "r" (CYGARC_REG_CONFIG_DCE)         \
                 );                                     \
    CYG_MACRO_END

#define HAL_DCACHE_IS_ENABLED_DEFINED
#define HAL_DCACHE_IS_ENABLED(_state_)          \
    CYG_MACRO_START                             \
    cyg_uint32 _cstate_;                        \
    asm volatile ( "mfc0   %0,$16\n"            \
                   : "=r"(_cstate_)             \
                   );                           \
    if( _cstate_ & CYGARC_REG_CONFIG_DCE)       \
       _state_ = 0;                             \
    else                                        \
      _state_ = 1;                              \
    CYG_MACRO_END

// Architecture HAL defines other operations


//-----------------------------------------------------------------------------
// Global control of Instruction cache

// Enable the instruction cache
#define HAL_ICACHE_ENABLE_DEFINED
#define HAL_ICACHE_ENABLE()                             \
    CYG_MACRO_START                                     \
    cyg_uint32 tmp;                                     \
    asm volatile ("mfc0 %0,$16;"                        \
                  "and  %0,%0,%1;"                      \
                  "mtc0 %0,$16;"                        \
                  : "=&r" (tmp)                         \
                  : "r" (~CYGARC_REG_CONFIG_ICE)        \
                 );                                     \
    CYG_MACRO_END

// Disable the instruction cache
#define HAL_ICACHE_DISABLE_DEFINED
#define HAL_ICACHE_DISABLE()                            \
    CYG_MACRO_START                                     \
    cyg_uint32 tmp;                                     \
    asm volatile ("mfc0 %0,$16;"                        \
                  "or   %0,%0,%1;"                      \
                  "mtc0 %0,$16;"                        \
                  : "=&r" (tmp)                         \
                  : "r" (CYGARC_REG_CONFIG_ICE)         \
                 );                                     \
    CYG_MACRO_END

#define HAL_ICACHE_IS_ENABLED_DEFINED
#define HAL_ICACHE_IS_ENABLED(_state_)          \
    CYG_MACRO_START                             \
    cyg_uint32 _cstate_;                        \
    asm volatile ( "mfc0   %0,$16\n"            \
                   : "=r"(_cstate_)             \
                   );                           \
    if( _cstate_ & CYGARC_REG_CONFIG_ICE)       \
       _state_ = 0;                             \
    else                                        \
      _state_ = 1;                              \
    CYG_MACRO_END

// TX49 cache instruction must not affect the line it executes out of,
// so disable instruction cache before invalidating it.
#define HAL_ICACHE_INVALIDATE_ALL_DEFINED
#define HAL_ICACHE_INVALIDATE_ALL()                                           \
    CYG_MACRO_START                                                           \
    register CYG_ADDRESS _baddr_ = 0x80000000;                                \
    register CYG_ADDRESS _addr_ = 0x80000000;                                 \
    register CYG_WORD _state_;                                                \
    _HAL_ASM_SET_MIPS_ISA(3);                                                 \
    HAL_ICACHE_IS_ENABLED( _state_ );                                         \
    HAL_ICACHE_DISABLE();                                                     \
    for( ; _addr_ < _baddr_+HAL_ICACHE_SIZE; _addr_ += HAL_ICACHE_LINE_SIZE ) \
    { _HAL_ASM_ICACHE_ALL_WAYS(0x00, _addr_); }                               \
    if( _state_ ) HAL_ICACHE_ENABLE();                                        \
    _HAL_ASM_SET_MIPS_ISA(0);                                                 \
    CYG_MACRO_END

// Architecture HAL defines other operations

#endif // CYGPKG_HAL_MIPS_TX4955


//-----------------------------------------------------------------------------
#endif // ifndef CYGONCE_VAR_CACHE_H
// End of var_cache.h