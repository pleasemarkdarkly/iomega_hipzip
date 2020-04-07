/////////////////////////////////////////////////////////////////
// File Name: boot_mem.cpp
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: image memory management, makes life easier
// Usage: call get
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
/////////////////////////////////////////////////////////////////


#include <bootloader/main/boot_mem.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_cache.h>
#include CYGHWR_MEMORY_LAYOUT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cyg/hal/hal_tables.h>
#ifdef CYGBLD_HAL_PLATFORM_STUB_H
#include CYGBLD_HAL_PLATFORM_STUB_H
#else
#include <cyg/hal/plf_stub.h>
#endif

#if 0
// #include <redboot.h>
// Intertrust Support
#include "verifier.h"
#include "miniDSAkey.h"
#include "BlockHeader.h"
#include "Chuck.h" 

static SFVerifier sf_verifier;
#endif

extern int gzip_init(_pipe_t* p);
extern int gzip_inflate(_pipe_t* p);
extern int gzip_close(_pipe_t* p, int err);

CBootMemory::CBootMemory(void *pTop,void *pBottom) {
  
  m_pTop = pTop;
  m_pBottom = pBottom;
  m_iFree = reinterpret_cast<unsigned long>(pTop) - reinterpret_cast<unsigned long>(pBottom);
  m_pFree = pBottom;
  m_iCount = 0;

}


CBootMemory::~CBootMemory() {



}

void* CBootMemory::FreeBlock() {
  
  return m_pFree;
  
}

unsigned int CBootMemory::FreeSize() { 

  return m_iFree;

}

  
  
void CBootMemory::NotifyImage(unsigned int Size) {

  m_rgpImages[m_iCount] = m_pFree;
  m_rguiSize[m_iCount] = Size;

  unsigned long ipFree;

  ipFree = reinterpret_cast<unsigned long>(m_pFree);
  
  ipFree += Size;
  ipFree += ipFree % 4; // align next free section to 32-bits

  m_iFree = reinterpret_cast<unsigned long>(m_pTop) - ipFree;
  m_pFree = reinterpret_cast<void *>(ipFree);

  m_iCount++;

}
  
void CBootMemory::Exec() {
   typedef void code_fun(void);
    unsigned long entry;   
	unsigned long oldints;
    code_fun *fun;

//  diag_printf("moving image at %x size = %d for exec\n",m_rgpImages[m_iCount-1],m_rguiSize[m_iCount-1]);
  memcpy((void *)0x20000,m_rgpImages[m_iCount-1],m_rguiSize[m_iCount-1]);
  
  // diag_printf("jump to 0x20040 here");
  entry = 0x20040;
  fun = (code_fun *)entry;


    HAL_DISABLE_INTERRUPTS(oldints);
    
#ifdef HAL_ARCH_PROGRAM_NEW_STACK
    HAL_ARCH_PROGRAM_NEW_STACK(fun);
#else
    (*fun)();
#endif


}

bool CBootMemory::Decompress() {

  unsigned long ulSize;
  
  if(UnGzip(m_rgpImages[m_iCount-1],m_rguiSize[m_iCount-1],m_pFree,&ulSize) < 0)
  {
	  return false;
  }

  if(ulSize == 0)
  {
	  return false;
  }

  NotifyImage(ulSize);

  return true;
}

bool CBootMemory::Verify() {

  unsigned long ulSize;
  
  UnSFV(m_rgpImages[m_iCount-1],m_rguiSize[m_iCount-1],m_pFree,&ulSize);
  NotifyImage(ulSize);

  return true;
}

int CBootMemory::UnGzip(void * pBase, unsigned long ulSize, void * pTarget, unsigned long * pulWritten)
{

  int err;

  _pipe_t pipe;
 
  // setup pipe
  pipe.in_buf = (unsigned char *) pBase;
  pipe.in_avail = ulSize;
  pipe.out_buf = (unsigned char *) pTarget;
  pipe.out_size = 0;

  // init gzip
  err = (*_dc_init)(&pipe);

  DEBUG_BOOTLOADER("ungzip:base = %x, size = %d, target = %x\n",pBase,ulSize,pTarget);
 
  // decompress file to target location and close gzip
  DEBUG_BOOTLOADER("init - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);

  err = (*_dc_inflate)(&pipe);

  DEBUG_BOOTLOADER("inflate - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);

  *pulWritten = pipe.out_size;

  err = (*_dc_close)(&pipe,err);

  DEBUG_BOOTLOADER("inflate - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);

  return err;

}




// decrypt and copy (destructive to original image)
int CBootMemory::UnSFV(void* pBase, unsigned long ulSize, void* pTarget, unsigned long *newsize)
{

#if 0
  metadata_t  md;
  unsigned int version;
  SFBlockHeader * sf_block_header;
  void * sf_block;
  int sf_status;
  char * ram_buffer;
  ram_buffer = reinterpret_cast<char *>(pBase);

  /* authenticate, unencrypt */
  sf_status = SFVerifier_init(&sf_verifier, TheSignatureKeyId, &TheSignatureKey, TheEncryptKeyId, TheEncryptKey);
  if (sf_status) {
    DEBUG_BOOTLOADER("Error initializing verifier %d\n", sf_status);
    return FALSE;
  }
  
  sf_block_header = (SFBlockHeader *)ram_buffer;
  sf_status = SFVerifier_verifyBlockHeader(&sf_verifier, sf_block_header);
  if (sf_status) {
    DEBUG_BOOTLOADER("Error verifying block header %d\n", sf_status);
    return FALSE;
  }
  md.len = SWAP_32(sf_block_header->totalSize);
  version = SWAP_32(sf_block_header->firmwareVersion);
  md.version_major = MAJOR_VERSION(version);
  md.version_minor = MINOR_VERSION(version);

  sf_block = (void *)(ram_buffer + sizeof(SFBlockHeader));
  sf_status = SFVerifier_verifyBlock(sf_block_header, sf_block);
  if (sf_status) {
    DEBUG_BOOTLOADER("Error verifying block %d\n", sf_status);
    return FALSE;
  }
  
  sf_status = SFVerifier_terminate(&sf_verifier);
  if (sf_status) {
    DEBUG_BOOTLOADER("Error terminiating verifier %d\n", sf_status);
    return FALSE;
  }
  DEBUG_BOOTLOADER("Firmware succesfully verified\n");

  // sf_block contains decrypted/verified data
  memcpy((void *)pTarget,(void *)sf_block,md.len);
  *newsize = md.len;
#endif

  return TRUE;

}


/*

int m_iCount;
void *pFree;
int m_iFree;

*/
