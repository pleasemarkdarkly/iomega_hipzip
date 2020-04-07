/////////////////////////////////////////////////////////////////
// File Name: boot_ide.cpp
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Wrapper for fs/fat (in polling mode) ide commands
// Usage: Call boot_ide_init() before any of the commands.
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
/////////////////////////////////////////////////////////////////

#include <bootloader/main/boot_ide.h>

const char extList[7][4] = {"DAD","DDO","DGZ","DSF","BIN","DSZ",""};
const char szDirPath[] = "A:\\*.*";
extern int strcmp(const char *s1, const char *s2);
// initialize IDE subsystem
#define WAIT_1MS 0x00001388
#define WAIT_1US 0x00000005

/* Local Wait Routine */
static void 
Wait(unsigned int cnt)	
{
    unsigned long i;
    
    i = cnt; 
    while(i--)
        ;
}

// FIXME: this sucks!
int
strcmp(const char *s1, const char *s2)
{
    char c1, c2;

    while ((c1 = *s1++) == (c2 = *s2++))
        if (c1 == 0)
            return (0);
    return ((unsigned char)c1 - (unsigned char)c2);
}

CBootDisk::CBootDisk()
: m_bFormat(false)
{
	int status = 0;
	int timeout = 100;

#ifndef __POGO
	DEBUG_BOOTLOADER("Spinup time wait - 1 second\n");
	Wait(WAIT_1MS * 10000);
#endif

	DEBUG_BOOTLOADER("+pc_system_init\n");
	// 0 is hd on dharma
	status = pc_system_init(0);
	DEBUG_BOOTLOADER("-pc_system_init\n");


	if (!status) 
	{				
		unsigned int err;
		err = pc_get_error(0);
		DEBUG_BOOTLOADER("Could not initialize drive 0 - err = %x\n",err);
		return;
	}
	else
	{
		DEBUG_BOOTLOADER("CBootDisk::Init Success\n");

		if(!pc_isdir("A:\\SYSTEM"))
		{
			pc_mkdir("A:\\SYSTEM");
		}
		m_bFormat = true;
	}

}

void CBootDisk::Close()
{

	 pc_system_close(0);

}

CBootDisk::~CBootDisk()
{

	Close();
 

}

void CBootDisk::InitDisk()
{

	// hmmm, I wonder what this will do.. :)
	DEBUG_BOOTLOADER("CBootDisk::Formatting disk 0\n");
	
	if(pc_format(0))
		DEBUG_BOOTLOADER("CBootDisk::Formatting success\n");
	else
		DEBUG_BOOTLOADER("CBootDisk::Formatting failed\n");
	
	
	if(!pc_isdir("A:\\SYSTEM"))
	{
		pc_mkdir("A:\\SYSTEM");
	}

}

// get first boot file in directoy
DSTAT* CBootDisk::GetFirstImage()
{

  int i;

  if(pc_gfirst(&m_dsFile,(TEXT*)szDirPath)) {

    while(1) {

      if(!(m_dsFile.fattribute & (ADIRENT | AVOLUME))) {
	DEBUG_BOOTLOADER("\t-- %s.%s\n",m_dsFile.fname,m_dsFile.fext);
	i = 0;
	while(extList[i][0] != '\0') {
	  if(strcmp(m_dsFile.fext,extList[i]) == 0) {
	    // found a match, return
	    DEBUG_BOOTLOADER("match found %s\n",extList[i]);
	    return &m_dsFile;
	  }
	  i++;
	}
      }

      // at the end of the list
      if(!pc_gnext(&m_dsFile)) {
	pc_gdone(&m_dsFile);
	return NULL;
      }
    }
  }
 
  // FIXME: call pc_gdone here?
  return NULL;

}

// get next boot file in directory
DSTAT* CBootDisk::GetNextImage()
{
  int i;

  // get next valid file
  if(pc_gnext(&m_dsFile)) {

    while(1) {
      if(!(m_dsFile.fattribute & (ADIRENT | AVOLUME))) {
	DEBUG_BOOTLOADER("\t-- %s.%s\n",m_dsFile.fname,m_dsFile.fext);
	i = 0;
	while(extList[i][0] != '\0') {
	  if(strcmp(m_dsFile.fext,extList[i]) == 0) {
	    DEBUG_BOOTLOADER("match found %s\n",extList[i]);
	    // found a match, return
	    return &m_dsFile;
	  }
	  i++;
	}
      }

      // at the end of the list
      if(!pc_gnext(&m_dsFile)) {
	pc_gdone(&m_dsFile);
	return NULL;
      }
    }
  }
 
  pc_gdone(&m_dsFile);
  return NULL;
  
}          

bool CBootDisk::ReadFile(const char * szPath, void *pBase, unsigned long * pulRead)
{

  // open file
  PCFD fd;
  DSTAT ds;
  UCOUNT ret,read;
  unsigned long remaining;

  unsigned long ulFileSize;
  unsigned char * pWrite; // write pointer
    
  fd  = po_open(const_cast<char *>(szPath), PO_RDONLY, PS_IREAD );
  if( fd < 0 ) {
    DEBUG_BOOTLOADER("Unable to open %s\n", szPath );
    *pulRead = 0;
    return FALSE;
  }

  // get file size
  pc_gfirst(&ds,const_cast<char *>(szPath));
 DEBUG_BOOTLOADER("ds.fsize = %d\n", ds.fsize);

 if(ds.fsize <= 0)
 {
	 return FALSE;
 }

  // read entire file into memory
  remaining = ds.fsize;
  pWrite = reinterpret_cast<unsigned char *>(pBase);

  while(remaining > 0) {

    if(remaining < 0x8000)
      read = remaining;
    else // do 32k reads, for the moment.
      read = 0x8000;
	
    ret = po_read( fd,pWrite, read );
    
    remaining -= ret; 
    pWrite += ret; 
  }

  ulFileSize = ds.fsize;

  // free stat struct
  pc_gdone(&ds);

  // close file  
  po_close( fd );

  *pulRead = ulFileSize;


  DEBUG_BOOTLOADER("Read %d bytes from %s\n",ulFileSize,szPath);
   return TRUE;
}


bool CBootDisk::WriteFile(const char * szPath, void *pBase, unsigned long ulSize)
{
  PCFD fd;


  // existing file must be deleted, if it isn't return false
  fd = po_open( const_cast<char *>(szPath),PO_WRONLY | PO_CREAT | PO_TRUNC,PS_IWRITE);

  if( fd < 0) {
    DEBUG_BOOTLOADER("Cannot create file. Already exists?\n");
    return FALSE;
  }

  unsigned long size = 0;

//  DEBUG_BOOTLOADER("starting write\n");
   
  while( ulSize > 0 ) {
        int BytesToWrite = (ulSize > 32768 ? 32768 : ulSize);
        po_write( (PCFD) fd, (UCHAR*) pBase, BytesToWrite );

        ulSize -= BytesToWrite;
        pBase = ((UCHAR*)pBase) + BytesToWrite;
    }

//  DEBUG_BOOTLOADER("%d\n",size);


  po_close(fd);

  DEBUG_BOOTLOADER("Wrote %d bytes to %s\n",ulSize,szPath);

  return TRUE;

}

bool CBootDisk::DeleteFile(const char * szPath)
{
  return pc_unlink(const_cast<char *>(szPath));
}




// TODO: add get version info to class - move ver info to intertrust?

#if 0

bool
get_firmware_image_metadata(char * image_name, metadata_t * md)
{
    PCFD fd;
    UCOUNT i;
    INT16 status;
#ifdef INTERTRUST_SUPPORT
    SFBlockHeader sf_block_header;
    int sf_status;
    unsigned int version;
#endif /* INTERTRUST_SUPPORT */
    bool ret_val;

    fd = po_open(image_name, PO_BINARY | PO_RDONLY, PS_IREAD);
    if (fd == -1) {
	DEBUG_BOOTLOADER("Could not open image from disk\n");
	return false;		/* no cleanup necessary, so exit here */
    }
    ret_val = true;

    /* TODO read our header */

#ifdef INTERTRUST_SUPPORT
    /* read intertrust header */
    sf_status = SFVerifier_init(&sf_verifier, TheSignatureKeyId, &TheSignatureKey, TheEncryptKeyId, TheEncryptKey);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error opening verifier %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }

    i = po_read(fd, (char *)&sf_block_header, sizeof(sf_block_header));
    if (i != sizeof(sf_block_header)) {
	DEBUG_bool
get_firmware_image_metadata(char * image_name, metadata_t * md)
{
    PCFD fd;
    UCOUNT i;
    INT16 status;
#ifdef INTERTRUST_SUPPORT
    SFBlockHeader sf_block_header;
    int sf_status;
    unsigned int version;
#endif /* INTERTRUST_SUPPORT */
    bool ret_val;

    fd = po_open(image_name, PO_BINARY | PO_RDONLY, PS_IREAD);
    if (fd == -1) {
	DEBUG_BOOTLOADER("Could not open image from disk\n");
	return false;		/* no cleanup necessary, so exit here */
    }
    ret_val = true;

    /* TODO read our header */

#ifdef INTERTRUST_SUPPORT
    /* read intertrust header */
    sf_status = SFVerifier_init(&sf_verifier, TheSignatureKeyId, &TheSignatureKey, TheEncryptKeyId, TheEncryptKey);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error opening verifier %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }

    i = po_read(fd, (char *)&sf_block_header, sizeof(sf_block_header));
    if (i != sizeof(sf_block_header)) {
	DEBUG_BOOTLOADER("Error reading block header\n");
	ret_val = false;
	goto close_file;
    }
    sf_status = SFVerifier_verifyBlockHeader(&sf_verifier, &sf_block_header);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error verifying header %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }
    version = SWAP_32(sf_block_header.firmwareVersion);
    md->version_major = MAJOR_VERSION(version);
    md->version_minor = MINOR_VERSION(version);

    sf_status = SFVerifier_terminate(&sf_verifier);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error terminating verifier %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }
#endif /* INTERTRUST_SUPPORT */

  close_file:
    status = po_close(fd);
    if (status == -1) {
	DEBUG_BOOTLOADER("Could not close disk image. Continuing anyway\n");
	ret_val = false;
    }

    return ret_val;
}BOOTLOADER("Error reading block header\n");
	ret_val = false;
	goto close_file;
    }
    sf_status = SFVerifier_verifyBlockHeader(&sf_verifier, &sf_block_header);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error verifying header %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }
    version = SWAP_32(sf_block_header.firmwareVersion);
    md->version_major = MAJOR_VERSION(version);
    md->version_minor = MINOR_VERSION(version);

    sf_status = SFVerifier_terminate(&sf_verifier);
    if (sf_status) {
	DEBUG_BOOTLOADER("Error terminating verifier %d\n", sf_status);
	ret_val = false;
	goto close_file;
    }
#endif /* INTERTRUST_SUPPORT */

  close_file:
    status = po_close(fd);
    if (status == -1) {
	DEBUG_BOOTLOADER("Could not close disk image. Continuing anyway\n");
	ret_val = false;
    }

    return ret_val;
}

#endif
