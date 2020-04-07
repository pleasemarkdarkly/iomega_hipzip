//........................................................................................
//........................................................................................
//.. File Name: BlockHeader.h																..
//.. Date: 8/2/2000																		..
//.. Author(s): Intertrust																..
//.. Description of content: Secure Firmware Verifier tag structure definition			..
//.. Usage: Firmware upgrades use SFV to verify a valid software upgrade				..
//.. Last Modified By: Todd Malsbary toddm@iobjects.com									..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

/*==========================================================================
  This software is provided pursuant to an agreement with InterTrust
  Technologies Corporation ("InterTrust"). This software may be used only
  as expressly allowed in such agreement.

  Copyright (c) 1999 by InterTrust. All rights reserved.
==========================================================================*/

#ifndef BLOCK_HEADER_H
#define BLOCK_HEADER_H

#include "Platform.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define DSA_SIGNATURE_SIZE 40
#define SHA_HASH_SIZE 20

typedef struct {
	
	Uint32 formatVersion;	/* version number of the format */	
	Uint32 firmwareVersion;	/* version number of the firmware download */
	Uint32 signKeyId;		/* ID of the key used to sign the firmware */
	Uint32 encryptKeyId;	/* ID of the key used to encrypt the firmware */
	Uint32 totalSize;	/* total size of the firmware download */
	Uint32 block;		/* block number - first block is number zero */
	Uint32 blockSize;	/* size of the block in bytes */
	Uint32 lastBlock;	/* nonzero if this is the last block */
	Byte blockHash[SHA_HASH_SIZE];  /* hash of the block data */
	Byte signature[DSA_SIGNATURE_SIZE];  /* signature of header */

} SFBlockHeader;

#if defined(__cplusplus)
}
#endif

#endif
