//........................................................................................
//........................................................................................
//.. File Name: bitmaps.h																..
//.. Date: 7/24/2000																	..
//.. Author(s): InterTrust																..
//.. Description of content: miniDSA key								 				..
//.. Usage: SFVerifier																	..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include "miniDSA.h"

const Uint32 TheSignatureKeyId = 1;



const miniDSA1024Key TheSignatureKey = {
    /* fd7f53811d75122952df4a9c2eece4e7f611b7523cef4400c31e3f80b6512669455d402251fb593d8d58fabfc5f5ba30f6cb9b556cd7813b801d346ff26660b76b9950a5a49f9fe8047b1022c24fbba9d7feb7c61bf83b57e7c6a8a6150f04fb83f6d3c51ec3023554135a169132f675f3ae2b61d72aeff22203199dd14801c7 */
    0xd14801c7,0x2203199d,0xd72aeff2,0xf3ae2b61,0x9132f675,0x54135a16,0x1ec30235,0x83f6d3c5,0x150f04fb,0xe7c6a8a6,0x1bf83b57,0xd7feb7c6,0xc24fbba9,0x047b1022,0xa49f9fe8,0x6b9950a5,0xf26660b7,0x801d346f,0x6cd7813b,0xf6cb9b55,0xc5f5ba30,0x8d58fabf,0x51fb593d,0x455d4022,0xb6512669,0xc31e3f80,0x3cef4400,0xf611b752,0x2eece4e7,0x52df4a9c,0x1d751229,0xfd7f5381,

    /* 9760508f15230bccb292b982a2eb840bf0581cf5 */
    0xf0581cf5,0xa2eb840b,0xb292b982,0x15230bcc,0x9760508f,

    /* f7e1a085d69b3ddecbbcab5c36b857b97994afbbfa3aea82f9574c0b3d0782675159578ebad4594fe67107108180b449167123e84c281613b7cf09328cc8a6e13c167a8b547c8d28e0a3ae1e2bb3a675916ea37f0bfa213562f1fb627a01243bcca4f1bea8519089a883dfe15ae59f06928b665e807b552564014c3bfecf492a */
    0xfecf492a,0x64014c3b,0x807b5525,0x928b665e,0x5ae59f06,0xa883dfe1,0xa8519089,0xcca4f1be,0x7a01243b,0x62f1fb62,0x0bfa2135,0x916ea37f,0x2bb3a675,0xe0a3ae1e,0x547c8d28,0x3c167a8b,0x8cc8a6e1,0xb7cf0932,0x4c281613,0x167123e8,0x8180b449,0xe6710710,0xbad4594f,0x5159578e,0x3d078267,0xf9574c0b,0xfa3aea82,0x7994afbb,0x36b857b9,0xcbbcab5c,0xd69b3dde,0xf7e1a085,

    /* fbc8f21f7eebcb075e94967e7237e66413f63cf81112da151eae0cb35d96d04163a0d5a758f9ef2bf39b3b1fa333e25394f361f35c3e8dfa36144ff5b62d258e644694b97cf9badd1c2ee9fbff95f747a39e55a3fd4e19e9a7c13eff53c2dd6fce94882f5de5990453e1f2fedd2ddb59726a1c66717d8254eaa3e556fcff33 */
    0x56fcff33,0x54eaa3e5,0x66717d82,0x59726a1c,0xfedd2ddb,0x0453e1f2,0x2f5de599,0x6fce9488,0xff53c2dd,0xe9a7c13e,0xa3fd4e19,0x47a39e55,0xfbff95f7,0xdd1c2ee9,0xb97cf9ba,0x8e644694,0xf5b62d25,0xfa36144f,0xf35c3e8d,0x5394f361,0x1fa333e2,0x2bf39b3b,0xa758f9ef,0x4163a0d5,0xb35d96d0,0x151eae0c,0xf81112da,0x6413f63c,0x7e7237e6,0x075e9496,0x1f7eebcb,0xfbc8f2,

    /* 13e244636fc30450797b31178ff37646307c64b090710ce7351cd7109d8822a87764c637713a8ac648fa0124ef3ad13345b58675e08f9d8db09e4e2604e5e77c98171814827138be1ee1b19491aef454765d3f6bdce1d24a4494d2a398830fda35496dfcd9ebe90f3af4ad1bce90a90517b245c18f254fbf1a7cdcb68f228d83 */
    0x8f228d83,0x1a7cdcb6,0x8f254fbf,0x17b245c1,0xce90a905,0x3af4ad1b,0xd9ebe90f,0x35496dfc,0x98830fda,0x4494d2a3,0xdce1d24a,0x765d3f6b,0x91aef454,0x1ee1b194,0x827138be,0x98171814,0x04e5e77c,0xb09e4e26,0xe08f9d8d,0x45b58675,0xef3ad133,0x48fa0124,0x713a8ac6,0x7764c637,0x9d8822a8,0x351cd710,0x90710ce7,0x307c64b0,0x8ff37646,0x797b3117,0x6fc30450,0x13e24463,

    /* 54a71a76b81fba104ec122b7c2a8c1afc9462170 */
    0xc9462170,0xc2a8c1af,0x4ec122b7,0xb81fba10,0x54a71a76,

    /* 92cb23d019c9cfcb28af064a808417ed844d4e0c */
    0x844d4e0c,0x808417ed,0x28af064a,0x19c9cfcb,0x92cb23d0,

};
