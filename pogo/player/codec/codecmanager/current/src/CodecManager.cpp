//
// CodecManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <codec/codecmanager/CodecManager.h>
#include "CodecManagerImp.h"

// implicit is that the codec manager should never
// be instantiated as a global, or from a global constructor

static CCodecManager* s_pSingleton = 0;


CCodecManager* CCodecManager::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CCodecManager;
    }
    return s_pSingleton;
}

CCodecManager::CCodecManager() 
{
    m_pImp = new CCodecManagerImp;
}

CCodecManager::~CCodecManager() 
{
    delete m_pImp;
}

ICodec* CCodecManager::FindCodec( unsigned int iCodecID, void* pAlloc, int iSize ) 
{
    return m_pImp->FindCodec(iCodecID, pAlloc, iSize);
}

ICodec* CCodecManager::FindCodec( const char* szExtension, void* pAlloc, int iSize )
{
    return m_pImp->FindCodec(szExtension, pAlloc, iSize);
}

RegKey CCodecManager::FindCodecID( const char* szExtension )
{
    return m_pImp->FindCodecID(szExtension);
}

// Codec probing is untested

ICodec* CCodecManager::TryCodec( int iCodecIndex, void* pAlloc, int iSize ) 
{ 
    return m_pImp->TryCodec( iCodecIndex, pAlloc, iSize);
}


