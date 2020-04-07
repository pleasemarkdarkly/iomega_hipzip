//
// CodecManagerImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "CodecManagerImp.h"

#include <codec/common/Codec.h>
#include <util/debug/debug.h>

#include <string.h> /* strnicmp */

#define MAX_CODECS 10

DEBUG_MODULE( CM );
DEBUG_USE_MODULE( CM );


//
// data structures
//


typedef struct codec_table_s 
{
    RegKey ID;
    const char* const* pExtensionTable;
    CodecFunctionMap* pFunctions;
} codec_table_t;

CCodecManagerImp::CCodecManagerImp() 
{
    m_iMapSize = 0;
    
    m_pRegistry = CRegistry::GetInstance();
    
    // Find all the given codecs in the registry
    CodecFunctionMap* pFunctions;
    
    RegKey KeyTable[ MAX_CODECS ];
    
    m_iTableSize = m_pRegistry->FindByType(
        CODEC_TYPE_ID,
        &KeyTable[0],
        MAX_CODECS );

    
    DBASSERT( CM, m_iTableSize > 0, "No Codecs found in system\n" );

    m_pCodecTable = new codec_table_t[ m_iTableSize ];

    // we should have a list of codec keys. now step through the registry,
    // and get a list of supported extensions. also retain the function map
    for( unsigned int i = 0; i < m_iTableSize; i++ ) {
        pFunctions = (CodecFunctionMap*) m_pRegistry->FindByKey( KeyTable[i] );
        m_pCodecTable[i].ID = KeyTable[i];
        m_pCodecTable[i].pExtensionTable = pFunctions->GetExtensionList();
        m_pCodecTable[i].pFunctions = pFunctions;

        for( int z = 0; m_pCodecTable[i].pExtensionTable && m_pCodecTable[i].pExtensionTable[z]; z++ ) {
            m_iMapSize++;
        }
    }
}

CCodecManagerImp::~CCodecManagerImp() 
{
    if( m_iTableSize ) {
        delete [] m_pCodecTable;
    }
}

ICodec* CCodecManagerImp::FindCodec( unsigned int iCodecID, void* pAlloc, int iSize ) 
{
    RegKey key = iCodecID;

    for( unsigned int i = 0; i < m_iTableSize; i++ ) {
        if( m_pCodecTable[i].ID == key ) {
            return m_pCodecTable[i].pFunctions->Create(pAlloc, iSize);
        }
    }
    
    return NULL;
}

ICodec* CCodecManagerImp::FindCodec( const char* szExtension, void* pAlloc, int iSize )
{
    // enumerate through the registered codec, then step through
    // their extension tables, and attempt to find a match
    for( unsigned int i = 0; i < m_iTableSize; i++ )
    {
        const char*const* pExt = m_pCodecTable[i].pExtensionTable;
        for( ; *pExt; pExt++ )
        {
            // case insentivity could be accomplished by inverting
            // the case of pExt and comparing that
            if( strnicmp( *pExt, szExtension, 3 ) == 0 )
            {
                return m_pCodecTable[i].pFunctions->Create(pAlloc, iSize);
            }
        }
    }

    return NULL;
}

RegKey CCodecManagerImp::FindCodecID( const char* szExtension )
{
    // enumerate through the registered codec, then step through
    // their extension tables, and attempt to find a match
    for( unsigned int i = 0; i < m_iTableSize; i++ )
    {
        const char*const* pExt = m_pCodecTable[i].pExtensionTable;
        for( ; *pExt; pExt++ )
        {
            // case insentivity could be accomplished by inverting
            // the case of pExt and comparing that
            if( strnicmp( *pExt, szExtension, 3 ) == 0 )
            {
                return m_pCodecTable[i].ID;
            }
        }
    }

    return 0;
}

// Codec probing is untested

ICodec* CCodecManagerImp::TryCodec( int iCodecIndex, void* pAlloc, int iSize ) 
{
    int iIndex = 0;
    for( unsigned int i = 0; i < m_iTableSize; i++ ) {
        if( m_pCodecTable[ i ].pFunctions->CanProbe() ) {
            if( iIndex == iCodecIndex ) {
                return m_pCodecTable[ i ].pFunctions->Create(pAlloc, iSize);
            }
            else {
                iIndex++;
            }
        }
    }
    return NULL;
}


