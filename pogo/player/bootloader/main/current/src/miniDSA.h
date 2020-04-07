//........................................................................................
//........................................................................................
//.. File Name: miniDSA.h																..
//.. Date: 7/17/2000																	..
//.. Author(s): Todd Malsbary															..
//.. Description of content:Definition of the structure of the miniDSAkey  				..
//.. Usage: Used in SFVerification of images											..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/14/2000														..
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

#ifndef MINIDSA_H
#define MINIDSA_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned char BYTE;
typedef unsigned long DIGIT;

typedef struct miniDSA1024Key {
    DIGIT P[32];
    DIGIT Q[5];
    DIGIT G[32];
    DIGIT Y[32];
    DIGIT RRP[32];
    DIGIT RRQ[5];
    DIGIT RRQP[5];
} miniDSA1024Key;

int DSA_verify(const miniDSA1024Key *key, DIGIT *DSA_sigS, DIGIT *DSA_sigR, DIGIT *DSA_hashM);

#if defined(__cplusplus)
}
#endif


#endif
