//
// UpdateApp.cpp
//
// Copyright (c) 1998 - 2002 Fullplay

#include <cyg/compress/zlib.h>
#include <cyg/infra/diag.h>

#include <main/util/update/UpdateApp.h>
#include <main/util/update/ParseConfig.h>

#include <fs/fat/sdapi.h>
#include <fs/flash/flashmanager.h>

#include <util/debug/debug.h>          // debugging hooks
#include <util/utils/utils.h>          // crc32

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#define UPDATE_PATH "A:\\UPDATES\\*.IMG"

DEBUG_MODULE_S( DBG_UPDATE_APP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE( DBG_UPDATE_APP );

// structure of firmware update process
//
//  a .img firmware update file is layed out as follows:
//
//  <     file header     >
//  [  zero or more bytes ]
//  <  image entry table  >
//  <       img 0         >
//  <       img 1         >
//  <        ...          >
//
// where appropriate structures for these are mapped out in imglayout.h
//
#include "imglayout.h"  // headers for firmware

static 
size_t strtrunk(const char *s, size_t maxlen)
{
    size_t x;
    for(x=0; (s[x]) && (s[x] != ' ') && (x < maxlen); x++)
        ;
    return x;
}

static
char *MakePath(char *target, const char *path, const char *name, const char *ext)
{
    int pathlen = strtrunk(path,EMAXPATH);
    int namelen = strtrunk(name,8);
    int extlen = strtrunk(ext,3);
    char* str = target;
//  memcpy(str, "file://",7);
//  str += 7;
    memcpy(str, path, pathlen);
    str += pathlen;
    memcpy(str,"\\",1);
    str += 1;
    memcpy(str, name, namelen);
    str += namelen;
    *str++ = '.';
    memcpy(str, ext, extlen);
    str += extlen;
    *str = '\0';
    return target;
}



CUpdateApp* CUpdateApp::s_pInstance = NULL;

CUpdateApp* CUpdateApp::GetInstance()
{
    
    if(s_pInstance == NULL)
        s_pInstance = new CUpdateApp();

    return s_pInstance;
}

void CUpdateApp::Destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

CUpdateApp::CUpdateApp()
{
    m_bCDUpdateAvailable = false;
}

CUpdateApp::~CUpdateApp()
{
}


int CUpdateApp::CheckFileVersion(const char* buff, int buffsize)
{
    const img_file_header_t* header = (const img_file_header_t*) buff;

    // Verify header magic
    if( memcmp( header->magic_bytes, szMagicBytes, IMG_NUM_MAGIC_BYTES ) != 0 ) {
        DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Check file version failed on magic bytes miscompare\n");
        return 0;
    }
    
    DEBUGP( DBG_UPDATE_APP, DBGLEV_TRACE, "Comment = %s\n", header->comment );
    DEBUGP( DBG_UPDATE_APP, DBGLEV_TRACE, "Version = %d\n", header->version );

    return header->version;
}

bool CUpdateApp::VerifyImage(const char* buff, unsigned long size)
{
    const img_file_header_t* header = (const img_file_header_t*)buff;

    // step 1) verify magic bytes at start of file
    if( memcmp( header->magic_bytes, szMagicBytes, IMG_NUM_MAGIC_BYTES ) != 0 ) {
        DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Verify file failed on magic bytes miscompare\n");
        return false;
    }
    // step 2) perform file CRC
    unsigned int crc = crc32( (const unsigned char*)(buff + header->crc_offset), size - header->crc_offset );
    if( crc != header->crc32 ) {
        DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "CRC32 failed on firmware\n");
        return false;
    }
    // step 3) verify image has entries
    if( header->fw_entries == 0 ) {
        DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Empty firmware update\n");
        return false;
    }

    // step 4) verify we can decrypt this image
    const fw_image_header_t* img_header = (const fw_image_header_t*) (buff + header->fw_offset);
    for( unsigned int i = 0; i < header->fw_entries; i++, img_header++ ) {
        if( img_header->magic != FW_MAGIC ) {
            DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Entry %d has failed magic\n", i );
            return false;
        }
        // make sure it's not using crypt we can't handle
        if( img_header->flags & FW_CRYPT_RESERVED ) {
            DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Entry %d has unknown crypt settings (0x%02x)\n", i, img_header->flags);
            return false;
        }
    }
    // image is now considered valid
    return true;
}

// update the device firmware with the provided image
bool CUpdateApp::UpdateImage( const char* buff, unsigned long size, void (*progress_cb)() ) 
{
    // 0) make absolutely sure this is a valid buffer
    if( !VerifyImage( buff, size ) ) {
        DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Update called on invalid image\n");
        return false;
    }

    // 1) set up our data stuff
    const img_file_header_t* header = (const img_file_header_t*) buff;
    const fw_image_header_t* img_header = (const fw_image_header_t*) (buff + header->fw_offset);
    
    CFlashManager* pFlash = CFlashManager::GetInstance();
    
    pFlash->OpenSession(progress_cb);
    
    // 2) start running the burn loop
    for( unsigned int i = 0; i < header->fw_entries; i++, img_header++ ) {
        const char* p;
        bool release = false;
        // check magic
        if( img_header->magic != FW_MAGIC ) {
            // failed magic on subheader, oops
            // probably bad, since we are likely half way through a burn
            DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Entry %d has failed magic\n", i );
            continue;
        }
        // check for xor
        if( img_header->flags & (FW_CRYPT_XOR2 | FW_CRYPT_XOR1) ) {
            unsigned int xorval = XOR_VALUE;
            char* v;
            if( img_header->flags & FW_CRYPT_XOR2 ) {
                xorval = xorval + (i << i);
            }

            p = v = (char*) malloc( img_header->fis_length );
            if( !p ) {
                DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Failed to allocate %d bytes for decrypt buffer\n", img_header->fis_length );
                continue;
            }
            release = true;

            _xor( (buff + img_header->offset), v, img_header->fis_length, xorval );
        } else {
            p = (buff + img_header->offset);
        }
                    
        // clear space
        DEBUGP( DBG_UPDATE_APP, DBGLEV_INFO, "Burning image %s\n", img_header->fis_name );
        if( !pFlash->ClearSpace( img_header->fis_name, img_header->fis_burnaddr, img_header->fis_length ) ) {
            DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Failed to clear space for image %d\n", i );
            continue;
        }
        // burn
        if( !pFlash->UpdateFlash( img_header->fis_name, p, img_header->fis_length,
                img_header->fis_burnaddr, img_header->fis_entry, img_header->fis_loadaddr ) ) {
            DEBUG( DBG_UPDATE_APP, DBGLEV_ERROR, "Failed to burn image %d\n", i );
            continue;
        }

        if( release ) {
            free( (void*) p );
        }
    }
    
    // save the fis table to flash
    pFlash->BurnFisTable();

    pFlash->CloseSession();
    
    return true;
}

// delete all files in the update directory, for cleaning after an update
void CUpdateApp::DeleteUpdates()
{
    
    DSTAT ds;
    
    if(pc_gfirst(&ds,UPDATE_PATH)) 
    {
        do 
        {
            if(!(ds.fattribute & (ADIRENT | AVOLUME))) 
            {
                char tempPath[EMAXPATH];
                MakePath(tempPath,ds.path,ds.fname,ds.fext);            
                DEBUGP( DBG_UPDATE_APP, DBGLEV_TRACE,"deleting %s\n",tempPath);
                pc_unlink(tempPath);
            }

        } while(pc_gnext(&ds));
    }

    pc_gdone(&ds);

}

void CUpdateApp::_xor( const char* src, char* dest, int len, unsigned int val)
{
    if( ((unsigned int)src) & 0x03 ||
        ((unsigned int)dest) & 0x03 )
    {
        DEBUGP( DBG_UPDATE_APP, DBGLEV_TRACE, "_xor can't operate on unaligned buffers\n");
        return ;
    }
    unsigned int* s = (unsigned int*)src;
    unsigned int* d = (unsigned int*)dest;

    len >>= 2; // div sizeof(unsigned int)
    for( int i = 0; i < len; i++ )
    {
        d[i] = s[i] ^ val;
    }
}

