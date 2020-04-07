//
// CodecManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The codec manager is responsible for locating and allocating codecs in the
//! system. It does this by querying the registry for available codecs, then
//! generating a list of supported file types at startup. This list is then
//! queried whenever a load by type is requested.

/** \addtogroup CodecManager Codec Manager */
//@{

#ifndef CODECMANAGER_H_
#define CODECMANAGER_H_

#include <util/registry/Registry.h>

class ICodec;
class CCodecManagerImp;

//! The CCodecManager singleton. 
class CCodecManager 
{
public:

    //! Returns a pointer to the singleton codec manager
    static CCodecManager* GetInstance();
    
    //! Given a specific CodecID, find the codec, allocate
    //! it, and return a pointer to the newly allocated object. If
    //! pAlloc is specified, allocate the codec in the given pool.
    ICodec* FindCodec( unsigned int iCodecID, void* pAlloc = 0, int iSize = 0 );

    //! Given a file extension, find a suitable codec, allocate
    //! it, and return a pointer to the object, or NULL if no
    //! suitable codec was found. If pAlloc is specified, allocate
    //! the codec in the given pool.
    ICodec* FindCodec( const char* szExtension, void* pAlloc = 0, int iSize = 0 );

    //! Rather than allocate the codec, simply see if one exists
    //! that claims to support this file type. This allows the
    //! codec ID to be associated with a content record, so when a
    //! track is actually loaded the codec manager doesn't have
    //! to perform a search for the appropriate codec.x
    RegKey  FindCodecID( const char* szExtension );

    //! Probe through available codecs looking for one that will
    //! support the given input stream.
    // An example of code that would utilize this:
    //
    //    CCodecManager* pCM = CCodecManager::GetInstance();
    //    ICodec* pCodec;
    //    for( int i = 0; ( pCodec = pCM->TryCodec(i) ); i++ ) {
    //       ERESULT err = pCodec->SetSong( pInputStream );
    //       if( SUCCESS( err ) ) {
    //         break;
    //       } else {
    //         delete pCodec;
    //       }
    //    }
    ICodec* TryCodec( int iCodecIndex, void* pAlloc = 0, int iSize = 0 );
    
private:

    CCodecManager();
    ~CCodecManager();

    CCodecManagerImp*   m_pImp;
};

//@}

#endif  // CODECMANAGER_H_
