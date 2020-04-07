/////////////////////////////////////////////////////////////////
// File Name: flashmanager.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for FIS/flash write operations
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

#ifndef _FLASHMANAGER_H_
#define _FLASHMANAGER_H_

#include <fs/flash/fis.h>
#ifdef __cplusplus

typedef void (*progress_cb)(void);

class CFlashManager  
{
    
public:

	CFlashManager();
	~CFlashManager();

    // start/finish a burn session. these calls will disable interrupts/caching, and should
    // be placed around any section of code calling clearspace/updateflash
    void OpenSession( progress_cb cb = 0 );
    void CloseSession();
    
    // copy the fis table to ram to work on it
    void LoadFisTable();

    // commit the fis table to flash
    //  this will also coalesce the table prior to burning it
    void BurnFisTable();
    
    // clear a space for a given image; this can in turn delete fis entries and blocks
    //  in this space
    bool ClearSpace( const char* name, unsigned long startaddr, unsigned long length );

    // burn an image at the given location
    bool UpdateFlash( const char* name, const char* buff, unsigned long size, unsigned long burnaddr,
        unsigned long entryaddr, unsigned long loadaddr );

	// only one procedure supported right now, updating an exiting image
	bool UpdateImage( const char* name, char* buffer, unsigned long buffersize );

	// TODO:
	// CreateImage
	// DeleteImage

    unsigned int GetBlockSize() const { return m_ulBlockSize; }
    
    
	struct fis_image_desc* GetFirstImage();  
	struct fis_image_desc * GetNextImage();
	struct fis_image_desc * FindImage(const char* name);
	
	static CFlashManager* GetInstance();


private:
    static CFlashManager* s_pInstance;
    
    // callback for diag output
    static int flash_print( const char* fmt, ... );

    // progress state
    static void progress( void );
    static progress_cb m_cb;
    
    // info about physical flash
    unsigned long m_ulFlashStart, m_ulFlashEnd, m_ulBlockSize, m_ulBlocks;
    unsigned long m_ulFisIndex, m_ulFisTotalEntries;

    // fis blocks
    struct fis_image_desc *m_pFisFlashBlock, *m_pFisWorkBlock, *m_pFisCurrentBlock;

    // cache/int control
    int m_i_cache, m_d_cache, m_cache_on;
    int m_iOldInts;
    
	bool m_bInit, m_bFisTableDirty;
};

extern "C" {
#endif

// C api
struct fis_image_desc* flash_fs_find_image( const char* name );

#ifdef __cplusplus
};
#endif
    
#endif /* _FLASHMANAGER_H_*/
