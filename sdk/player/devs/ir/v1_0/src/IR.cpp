//
// IR.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <util/debug/debug.h>

#include <devs/ir/IR.h>
#include "IRImp.h"

DEBUG_MODULE(IR);
DEBUG_USE_MODULE(IR);

CIR* CIR::m_pSingleton;

CIR * CIR::GetInstance() 
{
    if (m_pSingleton == NULL) {
	m_pSingleton = new CIR;
    }
    return m_pSingleton;
}

CIR::CIR()
{
    m_pIRImp = CIRImp::GetInstance();
}

CIR::~CIR()
{
}

void CIR::SetIRMap( const ir_map_t* pIRmap ) 
{
    m_pIRImp->SetIRMap( pIRmap );
}

void CIR::LockIR() 
{
    m_pIRImp->LockIR();
}

void CIR::UnlockIR()
{
    m_pIRImp->UnlockIR();
}
