#ifndef CYGONCE_IMP_CACHE_H
#define CYGONCE_IMP_CACHE_H

//=============================================================================
//
//      imp_cache.h
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
// Contributors:        nickg, dmoseley
// Date:        1998-02-17
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
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/plf_cache.h>
#include <cyg/hal/var_arch.h>

#ifdef CYGHWR_HAL_MIPS_MIPS64_CORE_5K

//-----------------------------------------------------------------------------
// Cache dimensions

// Data cache
#define HAL_DCACHE_SIZE                 8192    // Size of data cache in bytes
#define HAL_DCACHE_LINE_SIZE            32      // Size of a data cache line
#define HAL_DCACHE_WAYS                 2       // Associativity of the cache

// Instruction cache
#define HAL_ICACHE_SIZE                 8192    // Size of cache in bytes
#define HAL_ICACHE_LINE_SIZE            32      // Size of a cache line
#define HAL_ICACHE_WAYS                 2       // Associativity of the cache

#define HAL_DCACHE_SETS (HAL_DCACHE_SIZE/(HAL_DCACHE_LINE_SIZE*HAL_DCACHE_WAYS))
#define HAL_ICACHE_SETS (HAL_ICACHE_SIZE/(HAL_ICACHE_LINE_SIZE*HAL_ICACHE_WAYS))

#define HAL_DCACHE_WRITETHRU_MODE       1
#define HAL_DCACHE_WRITEBACK_MODE       0

#else

#error Unknown MIPS32 Variant

#endif

//-----------------------------------------------------------------------------
// General cache defines.
#define HAL_CLEAR_TAGLO()  asm volatile (" mtc0 $0, $28;" \
                                             " nop;"      \
                                             " nop;"      \
                                             " nop;")
#define HAL_CLEAR_TAGHI()  asm volatile (" mtc0 $0, $29;" \
                                             " nop;"      \
                                             " nop;"      \
                                             " nop;")

/* Cache instruction opcodes */
#define HAL_CACHE_OP(which, op)             (which | (op << 2))

#define HAL_WHICH_ICACHE                    0x0
#define HAL_WHICH_DCACHE                    0x1

#define HAL_INDEX_INVALIDATE                0x0
#define HAL_INDEX_LOAD_TAG                  0x1
#define HAL_INDEX_STORE_TAG                 0x2
#define HAL_HIT_INVALIDATE                  0x4
#define HAL_ICACHE_FILL                     0x5
#define HAL_DCACHE_HIT_INVALIDATE           0x5
#define HAL_DCACHE_HIT_WRITEBACK            0x6
#define HAL_FETCH_AND_LOCK                  0x7

//-----------------------------------------------------------------------------
// Global control of data cache

// Invalidate the entire cache
#define HAL_DCACHE_INVALIDATE_ALL_DEFINED
#define HAL_DCACHE_INVALIDATE_ALL()                                                     \
    CYG_MACRO_START                                                                     \
    register volatile CYG_BYTE *addr;                                                   \
    HAL_CLEAR_TAGLO();                                                                  \
    HAL_CLEAR_TAGHI();                                                                  \
    for (addr = (CYG_BYTE *)CYGARC_KSEG_CACHED_BASE;                                    \
         addr < (CYG_BYTE *)(CYGARC_KSEG_CACHED_BASE + HAL_DCACHE_SIZE);                \
         addr += HAL_DCACHE_LINE_SIZE )                                                 \
    {                                                                                   \
        asm volatile (" cache %0, 0(%1)"                                                \
                      :                                                                 \
                      : "I" (HAL_CACHE_OP(HAL_WHICH_DCACHE, HAL_INDEX_STORE_TAG)),      \
                        "r"(addr));                                                     \
    }                                                                                   \
    CYG_MACRO_END

// Synchronize the contents of the cache with memory.
extern void hal_dcache_sync(void);
#define HAL_DCACHE_SYNC_DEFINED
#define HAL_DCACHE_SYNC() hal_dcache_sync()

// Set the data cache refill burst size
//#define HAL_DCACHE_BURST_SIZE(_asize_)

// Set the data cache write mode
//#define HAL_DCACHE_WRITE_MODE( _mode_ )

// Load the contents of the given address range into the data cache
// and then lock the cache so that it stays there.
#define HAL_DCACHE_LOCK_DEFINED
#define HAL_DCACHE_LOCK(_base_, _asize_)                                                \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_DCACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_DCACHE, HAL_FETCH_AND_LOCK)),         \
                      "r"(_addr_));                                                     \
    CYG_MACRO_END

// Undo a previous lock operation
#define HAL_DCACHE_UNLOCK_DEFINED
#define HAL_DCACHE_UNLOCK(_base_, _asize_)                                              \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_DCACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_DCACHE, HAL_HIT_INVALIDATE)),         \
                      "r"(_addr_));                                                     \
    CYG_MACRO_END

// Unlock entire cache
#define HAL_DCACHE_UNLOCK_ALL_DEFINED
#define HAL_DCACHE_UNLOCK_ALL() HAL_DCACHE_UNLOCK(0,HAL_DCACHE_SIZE)


//-----------------------------------------------------------------------------
// Data cache line control

// Allocate cache lines for the given address range without reading its
// contents from memory.
//#define HAL_DCACHE_ALLOCATE( _base_ , _asize_ )

// Write dirty cache lines to memory and invalidate the cache entries
// for the given address range.
#define HAL_DCACHE_FLUSH_DEFINED
#if HAL_DCACHE_WRITETHRU_MODE == 1
// No need to flush a writethrough cache
#define HAL_DCACHE_FLUSH( _base_ , _asize_ )
#else
#error HAL_DCACHE_FLUSH undefined for MIPS32 writeback cache
#endif

// Write dirty cache lines to memory for the given address range.
#define HAL_DCACHE_STORE_DEFINED
#if HAL_DCACHE_WRITETHRU_MODE == 1
// No need to store a writethrough cache
#define HAL_DCACHE_STORE( _base_ , _asize_ )
#else
#error HAL_DCACHE_STORE undefined for MIPS32 writeback cache
#endif

// Invalidate cache lines in the given range without writing to memory.
#define HAL_DCACHE_INVALIDATE_DEFINED
#define HAL_DCACHE_INVALIDATE( _base_ , _asize_ )                                       \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_DCACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_DCACHE, HAL_HIT_INVALIDATE)),         \
                      "r"(_addr_));                                                       \
    CYG_MACRO_END






//-----------------------------------------------------------------------------
// Global control of Instruction cache

// Invalidate the entire cache
#define HAL_ICACHE_INVALIDATE_ALL_DEFINED
#define HAL_ICACHE_INVALIDATE_ALL()                                                     \
    CYG_MACRO_START                                                                     \
    register volatile CYG_BYTE *addr;                                                   \
    HAL_CLEAR_TAGLO();                                                                  \
    HAL_CLEAR_TAGHI();                                                                  \
    for (addr = (CYG_BYTE *)CYGARC_KSEG_CACHED_BASE;                                    \
         addr < (CYG_BYTE *)(CYGARC_KSEG_CACHED_BASE + HAL_ICACHE_SIZE);                \
         addr += HAL_ICACHE_LINE_SIZE )                                                 \
    {                                                                                   \
        asm volatile (" cache %0, 0(%1)"                                                \
                      :                                                                 \
                      : "I" (HAL_CACHE_OP(HAL_WHICH_ICACHE, HAL_INDEX_STORE_TAG)),      \
                        "r"(addr));                                                     \
    }                                                                                   \
    CYG_MACRO_END

// Synchronize the contents of the cache with memory.
extern void hal_icache_sync(void);
#define HAL_ICACHE_SYNC_DEFINED
#define HAL_ICACHE_SYNC() hal_icache_sync()

// Set the instruction cache refill burst size
//#define HAL_ICACHE_BURST_SIZE(_asize_)

// Load the contents of the given address range into the data cache
// and then lock the cache so that it stays there.
#define HAL_ICACHE_LOCK_DEFINED
#define HAL_ICACHE_LOCK(_base_, _asize_)                                                \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_ICACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_ICACHE, HAL_FETCH_AND_LOCK)),         \
                      "r"(_addr_));                                                     \
    CYG_MACRO_END

// Undo a previous lock operation
#define HAL_ICACHE_UNLOCK_DEFINED
#define HAL_ICACHE_UNLOCK(_base_, _asize_)                                              \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_ICACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_ICACHE, HAL_HIT_INVALIDATE)),         \
                      "r"(_addr_));                                                     \
    CYG_MACRO_END

// Unlock entire cache
#define HAL_ICACHE_UNLOCK_ALL_DEFINED
#define HAL_ICACHE_UNLOCK_ALL() HAL_ICACHE_UNLOCK(0,HAL_ICACHE_SIZE)

//-----------------------------------------------------------------------------
// Instruction cache line control

// Invalidate cache lines in the given range without writing to memory.
#define HAL_ICACHE_INVALIDATE_DEFINED
#define HAL_ICACHE_INVALIDATE( _base_ , _asize_ )                                       \
    CYG_MACRO_START                                                                     \
    register CYG_ADDRESS _baddr_ = (CYG_ADDRESS)(_base_);                               \
    register CYG_ADDRESS _addr_ = (CYG_ADDRESS)(_base_);                                \
    register CYG_WORD _size_ = (_asize_);                                               \
    for( ; _addr_ <= _baddr_+_size_; _addr_ += HAL_ICACHE_LINE_SIZE )                   \
      asm volatile (" cache %0, 0(%1)"                                                  \
                    :                                                                   \
                    : "I" (HAL_CACHE_OP(HAL_WHICH_ICACHE, HAL_HIT_INVALIDATE)),         \
                      "r"(_addr_));                                                     \
    CYG_MACRO_END

//-----------------------------------------------------------------------------
#endif // ifndef CYGONCE_IMP_CACHE_H
// End of imp_cache.h
