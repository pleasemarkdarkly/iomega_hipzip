//........................................................................................
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*==========================================================================
  File name:  verifier.h
  Description:
    Secure Firmware Verifier API
	
  Notes:
  Porting issues:
	Any of the cryptographic code modules may need to be ported to various
    platforms.
============================================================================
   Original author:  Jeff Faust
     Creation date:  1999
============================================================================
  This software is provided pursuant to an agreement with InterTrust
  Technologies Corporation ("InterTrust"). This software may be used only
  as expressly allowed in such agreement.

  Copyright (c) 1999 by InterTrust. All rights reserved.
==========================================================================*/

#ifndef VERIFIER_H
#define VERIFIER_H

#include "BlockHeader.h"
#include "miniDSA.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define ERROR_NO_VERIFY_KEY					1
#define ERROR_NO_DECRYPT_KEY				2
#define ERROR_MISMATCHED_FIRMWARE_VERSION	3
#define ERROR_BLOCK_OUT_OF_ORDER			4
#define ERROR_BLOCK_CORRUPTED				5
#define ERROR_WRONG_FORMAT_VERSION			6
#define ERROR_BLOCK_IS_ENCRYPTED			7
#define ERROR_INVALID_HEADER_SIGNATURE      8

typedef struct {
	Uint32 firstBlock;
	Uint32 firmwareVersion;
	Uint32 block;
	Uint32 signKeyId;
    const miniDSA1024Key *signKey;
	Uint32 encryptKeyId;
} SFVerifier;


int SFVerifier_init(SFVerifier *self,
                    Uint32 SignKeyId,    const miniDSA1024Key *SignKey,
                    Uint32 DecryptKeyId, const Byte *DecryptKey);
/*==========================================================================
Initializes the Secure Firmware Verifier
self	        ptr to a SFVerifier object to initialize
SignKeyId       an identifying number for the DSA signature key
SignKey         ptr to a key structure for verifying the DSA signature
DecryptKeyId    an identifying number for the key
DecryptKey      ptr to a key for decrypting the blocks
returns 0 if successful

This function must be called before any of the other verifier functions
are called.
==========================================================================*/

int SFVerifier_verifyBlockHeader(SFVerifier *self, const SFBlockHeader *bh);
/*==========================================================================
Verifies a block header
self	ptr to an initialized verifier state object
bh		ptr to an unverified block header
returns 0 if successful

This function verifies and fixes the endianess of a block header. Block
headers are fixed size objects that contain information about the block
that follows. This function should be called once for each block header.
==========================================================================*/

int SFVerifier_verifyBlock(const SFBlockHeader *bh, void *block);
/*==========================================================================
Verifies a block
bh		ptr to a verified block header
block	ptr to the block to verify
returns 0 if successful

This function uses information in the block header to determine the size
of the block. The caller must make sure that the actual block size matches
the size specified in the block header. The data in the block is hashed.
The hash value is then compared to the hash value in the block header.
==========================================================================*/

int SFVerifier_terminate(SFVerifier *self);
/*==========================================================================
Terminates the verifier
returns 0 if successful

This function should be called when the verifier is no longer needed.
==========================================================================*/

#if defined(__cplusplus)
}
#endif

#endif
