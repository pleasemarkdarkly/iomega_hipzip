/////////////////////////////////////////////////////////////////
// File Name: flashmanager.cpp
// Date: 02/20/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for flash commands
// Usage: 
// These provide abstracted wrappings of the flash file system
// manipulation commands. If you want to read from flash, just
// get the addresses and read/copy it directly.
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
/////////////////////////////////////////////////////////////////

#include <fs/flash/flashmanager.h>

#include <pkgconf/system.h>
#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_intr.h>
#include <stdlib.h>
#include <string.h>

#include <util/utils/utils.h>   // crc32

#ifdef CYGPKG_IO_FLASH_dj072602
#define NO_FLASH_PRINTF
#endif

#ifdef NOKERNEL

// get workspace from boot_init when in bootloader
extern unsigned char* workspace_end;
extern unsigned long workspace_size;
unsigned char* workspace;

extern "C" {
extern int strcmp(const char*, const char*);
};

#else

#include <util/diag/diag.h>

// hack for normal app
unsigned long workspace_size = FLASH_MIN_WORKSPACE;
unsigned char workspace[FLASH_MIN_WORKSPACE];

#endif

extern "C" struct fis_image_desc* flash_fs_find_image( const char* name ) 
{
    return CFlashManager::GetInstance()->FindImage( name );
}

progress_cb CFlashManager::m_cb = CFlashManager::progress;

CFlashManager* CFlashManager::s_pInstance = NULL;

CFlashManager* CFlashManager::GetInstance()
{
	if(s_pInstance == NULL)
	{
		s_pInstance = new CFlashManager();
	}

	return s_pInstance;

}

// get flash statistics from driver
CFlashManager::CFlashManager()
{
#ifdef NOKERNEL
    // horrible hackery - allow a non-singleton instance to map back to the singleton interface
    // hope that it's a global. :>
    CFlashManager::s_pInstance = this;
    
    workspace = workspace_end - FLASH_MIN_WORKSPACE;
#endif

	int stat;

#if DEBUG_LEVEL != 0
	diag_printf("workspace at 0x%x, size = %d\n",workspace,FLASH_MIN_WORKSPACE);
#endif

#ifdef NO_FLASH_PRINTF
    if((stat = flash_init(workspace,FLASH_MIN_WORKSPACE)) != 0)
#else
	if((stat = flash_init(workspace,FLASH_MIN_WORKSPACE,flash_print)) != 0)
#endif
	{
        diag_printf("FLASH: driver init failed!, status: 0x%x\n", stat);
        m_bInit = false;
	}

    // the FIS table resides in the last block of flash; get info about the flash size and block size
	flash_get_limits(NULL, (void **)&m_ulFlashStart, (void **)&m_ulFlashEnd);
	flash_get_block_info((int *)&m_ulBlockSize, (int *)&m_ulBlocks);

#if DEBUG_LEVEL != 0
	diag_printf("\nFLASH: 0x%x - 0x%x, %d blocks of 0x%x bytes each.\n", m_ulFlashStart, m_ulFlashEnd, m_ulBlocks, m_ulBlockSize);
#endif
    
    m_pFisWorkBlock = m_pFisCurrentBlock = 0;

    m_ulFisIndex = 0;
    m_ulFisTotalEntries = m_ulBlockSize / sizeof( struct fis_image_desc );

    m_bFisTableDirty = false;
    
    // then load the table to ram
    LoadFisTable();
    
	m_bInit = true;
    
    // then set pointers to the physical block and allocate a RAM copy (working block)
}

CFlashManager::~CFlashManager()
{
    if( m_bFisTableDirty ) {
        diag_printf("Shutting down with a dirty fis table\n");
    }

    if( m_pFisWorkBlock ) {
        free((void*)m_pFisWorkBlock);
    }
}

void CFlashManager::OpenSession( progress_cb cb )
{
    HAL_DISABLE_INTERRUPTS(m_iOldInts);
    HAL_DCACHE_IS_ENABLED( m_cache_on );
    if( m_cache_on ) {
        HAL_DCACHE_SYNC();
        HAL_DCACHE_INVALIDATE_ALL();
        HAL_DCACHE_DISABLE();
        HAL_DCACHE_INVALIDATE_ALL();
    }

    if( cb ) m_cb = cb;
    else m_cb = progress;
}

void CFlashManager::CloseSession()
{
    if( m_cache_on ) {
        HAL_DCACHE_ENABLE();
    }
    HAL_RESTORE_INTERRUPTS(m_iOldInts);
}

void CFlashManager::LoadFisTable() 
{
    if( m_pFisWorkBlock != 0 ) {
        if( m_bFisTableDirty ) {
            diag_printf("Loading fis table from flash on top of a dirty table\n");
        }
    } else {
        m_pFisWorkBlock = (struct fis_image_desc *)malloc(sizeof(unsigned char)*m_ulBlockSize);
    }
        
	m_pFisFlashBlock = (struct fis_image_desc *)(m_ulFlashEnd - m_ulBlockSize);
    memcpy((void*)m_pFisWorkBlock,(void*)m_pFisFlashBlock,m_ulBlockSize);
    m_bFisTableDirty = false;
}

void CFlashManager::BurnFisTable()
{
    if( !m_pFisWorkBlock ) {
        diag_printf(" no fis work block !!\n");
        return ;
    }
    if( !m_bFisTableDirty ) {
        diag_printf("Burn requested but table is clean, skipping\n");
        return ;
    }

    // TODO coalesce the fis table
    
    void* err_addr;
    // erase fis table
    diag_printf("reburning fis table\n");
    if ((flash_erase((void *)m_pFisFlashBlock, m_ulBlockSize, (void **)&err_addr)) != 0) 
    {
        diag_printf("Error erasing fis table \n");
    }
    
    if ((flash_program((void *)m_pFisFlashBlock, (void *)m_pFisWorkBlock, m_ulBlockSize, (void **)&err_addr)) != 0) 
    {
        diag_printf("Error programming fis table\n");
    }
    
    m_bFisTableDirty = false;
}

// get first flash image info (emulates lame-o system usage)
struct fis_image_desc * CFlashManager::GetFirstImage()
{
    if( !m_bInit ) return NULL;
    
	// reset m_fisimg to base address
	m_pFisCurrentBlock = m_pFisWorkBlock;
	m_ulFisIndex = 0;

	while(m_ulFisIndex < m_ulFisTotalEntries )
	{
		if(m_pFisCurrentBlock->name[0] != 0xFF)
		{

			return m_pFisCurrentBlock;
		}
		m_pFisCurrentBlock++;
		m_ulFisIndex++;
			
	}

	return NULL;
}

struct fis_image_desc * CFlashManager::GetNextImage()
{
    if( !m_bInit ) return NULL;

	// reset m_fisimg to base address

	m_pFisCurrentBlock++;
	m_ulFisIndex++;

	while(m_ulFisIndex < m_ulFisTotalEntries)
	{
		if(m_pFisCurrentBlock->name[0] != 0xFF)
		{
			return m_pFisCurrentBlock;
		}

		m_pFisCurrentBlock++;
		m_ulFisIndex++;
	}

	return NULL;
}


// returns null if it can't
struct fis_image_desc * CFlashManager::FindImage(const char* name)
{
    if( !m_bInit ) return NULL;

	// reset m_fisimg to base address
	struct fis_image_desc * pFisCurrentBlock = m_pFisWorkBlock;
	unsigned long ulFisPos = 0;

	while(ulFisPos < m_ulFisTotalEntries)
	{
        // it's fine to scan empty entries here
        if( strcmp(name, (char*)pFisCurrentBlock->name) == 0 ) {
            return pFisCurrentBlock;
        }

		pFisCurrentBlock++;
		ulFisPos++;
    }

	return NULL;
}

bool CFlashManager::ClearSpace( const char* name, unsigned long startaddr, unsigned long length ) 
{
    struct fis_image_desc* fis = GetFirstImage();
    unsigned long endaddr = startaddr + length;
    while( fis ) {
        // determine if the current entry falls within the range we are going to burn
        // assumption: size is a multiple of blocks
        unsigned int fis_endaddr = fis->flash_base + fis->size;
        if( (fis->flash_base >= startaddr && fis->flash_base <= endaddr) ||
            (fis_endaddr     >= startaddr && fis_endaddr     <= endaddr) ||
            strcmp( (const char*)fis->name, name ) == 0 ) {

            // this entry overlaps the range we want or has a name collision, so delete it
            fis->name[0] = 0xff;
            // dont worry about erasing the blocks, since the blocks we will use will be erased
            // when we program
        }
        fis = GetNextImage();
    }
    m_bFisTableDirty = true;
    return true;
}

bool CFlashManager::UpdateFlash( const char* name, const char* buff, unsigned long size, unsigned long burnaddr,
    unsigned long entryaddr, unsigned long loadaddr )
{
    // first, find a free fis entry, jumping around reserved entries
    struct fis_image_desc* img = m_pFisWorkBlock;
    unsigned long i;
    void* err_addr;
    bool res = true;

    for( i = 0; i < m_ulFisTotalEntries; i++ ) {
        if( img->name[0] == 0xff ) {
            break;
        }
        img++;
    }
    if( i == m_ulFisTotalEntries ) {
        diag_printf("No free spots in fis table\n");
        return false;
    }
    
    // now that we have an entry, copy in the appropriate info
    strcpy( (char*)img->name, name );
    img->flash_base  = burnaddr;
    img->mem_base    = loadaddr;
    img->size        = size + (m_ulBlockSize - (size % m_ulBlockSize));
    img->entry_point = entryaddr;
    img->data_length = size;
    img->desc_cksum  = 0;  // this is unused by redboot
    img->file_cksum  = (unsigned int)crc32((const unsigned char*)buff, img->data_length);

    // TODO retry burns here
    if( flash_erase((void*)img->flash_base, img->size, &err_addr) != 0 ) {
        diag_printf("Error erasing image\n");
        res = false;
    }
    if( flash_program((void*)img->flash_base, (void*) buff, img->size, &err_addr) != 0) {
        diag_printf("Error programming image\n");
        res = false;
    }
    m_bFisTableDirty = true;
    
    return res;
}

// name is name of image in FIS table, buffer is data to write
bool CFlashManager::UpdateImage(const char* name, char* buffer, unsigned long buffersize)
{

	struct fis_image_desc * img;
	struct fis_image_desc * current;
	unsigned long newsize, newdata_length;
	void *err_addr;
	if((img = FindImage(name)) != NULL)
	{
		// overlap check (just if circumstances are worse?)

		newdata_length = buffersize;
		newsize = buffersize + (m_ulBlockSize - (buffersize % m_ulBlockSize));

		if((current = GetFirstImage()) != NULL)
		{
			while(current != NULL)
			{
				// probably overlaps in some way with itself - that's ok. 
				if(strcmp((char*)current->name, (char*)img->name) != 0)
				{
					// FIXME: think about boundary conditions harder - block size?
					if((current->flash_base < (img->flash_base + newsize)) && (current->flash_base > (img->flash_base + img->size)))
					{
					   // bad
					   diag_printf("Overlap detected, %s will overlap with %s\n",img->name,current->name);
					   return false;
					}
				}

				current = GetNextImage();
			}
		}
		else
		{
			diag_printf("No valid FIS entries\n");
			return false;
		}



		// calculate new size
		img->data_length = newdata_length;
		img->size = newsize;
        
        img->desc_cksum  = 0;  // this is unused by redboot
        img->file_cksum  = crc32((const unsigned char*)buffer, img->data_length);

		diag_printf("Programming new image - DANGER!\n");
#ifndef NOKERNEL
		print_mem_usage();
#endif
        
		// FIXME: pretty much fucked if any of these fail. Try them again on a failure?

		// disable interrupts?

		// erase image space
		if (flash_erase((void *)img->flash_base, img->size, (void **)&err_addr) != 0) 
		{
			diag_printf("Error erasing image\n");
		}
    
		// write new image
		if ((flash_program((void *)img->flash_base, (void *)buffer, img->size, (void **)&err_addr)) != 0) 
		{
			diag_printf("delete: Error programming image\n");
		}

		// erase fis table
		if ((flash_erase((void *)m_pFisFlashBlock, m_ulBlockSize, (void **)&err_addr)) != 0) 
		{
			diag_printf("Error erasing fis table \n");
		}
 
		if ((flash_program((void *)m_pFisFlashBlock, (void *)m_pFisWorkBlock, m_ulBlockSize, (void **)&err_addr)) != 0) 
		{
			diag_printf("delete: Error programming fis table\n");
		}

		// enable interrupts?
        m_bFisTableDirty = true;
	}
	else
	{
		diag_printf("Image not found/Invalid FIS table\n");
		return false;
	}

	return true;
	

}

int CFlashManager::flash_print( const char* fmt, ... ) 
{
    if( strcmp( fmt, "." ) == 0 ) {
        m_cb();
    }
    return 0;
}

void CFlashManager::progress( void )
{
    diag_write_char('.');
}
