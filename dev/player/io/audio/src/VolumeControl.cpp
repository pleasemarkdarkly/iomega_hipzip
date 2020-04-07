//
// VolumeControl.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <io/audio/VolumeControl.h>
#include "VolumeControlImp.h"

static CVolumeControl* s_pSingleton = 0;

CVolumeControl* CVolumeControl::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CVolumeControl;
    }
    return s_pSingleton;
}

void CVolumeControl::Destroy() 
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

CVolumeControl::CVolumeControl() 
{
    m_pImp = new CVolumeControlImp;    
}

CVolumeControl::~CVolumeControl() 
{
    delete m_pImp;
}

int CVolumeControl::SaveToRegistry() 
{
    return m_pImp->SaveToRegistry();
}

int CVolumeControl::RestoreFromRegistry() 
{
    return m_pImp->RestoreFromRegistry();
}

void CVolumeControl::SetVolumeRange( int iRange ) 
{
    m_pImp->SetVolumeRange(iRange);
}

void CVolumeControl::SetVolumeRange( int iValueCount, int* iValues )
{
    m_pImp->SetVolumeRange(iValueCount, iValues);
}

int CVolumeControl::GetVolumeRange() const 
{
    return m_pImp->GetVolumeRange();
}

int CVolumeControl::VolumeUp()
{
    return m_pImp->VolumeUp();
}

int CVolumeControl::VolumeDown() 
{
    return m_pImp->VolumeDown();
}

int CVolumeControl::GetVolume() const 
{
    return m_pImp->GetVolume();
}

int CVolumeControl::SetVolume( int level ) 
{
    return m_pImp->SetVolume(level);
}


void CVolumeControl::SetTrebleRange( int iRange ) 
{
    m_pImp->SetTrebleRange(iRange);
}

void CVolumeControl::SetTrebleRange( int iValueCount, int* iValues )
{
    m_pImp->SetTrebleRange(iValueCount, iValues);
}

int CVolumeControl::GetTrebleRange() const 
{
    return m_pImp->GetTrebleRange();
}

int CVolumeControl::TrebleUp()
{
    return m_pImp->TrebleUp();
}

int CVolumeControl::TrebleDown() 
{
    return m_pImp->TrebleDown();
}

int CVolumeControl::GetTreble() const 
{
    return m_pImp->GetTreble();
}

int CVolumeControl::SetTreble( int level ) 
{
    return m_pImp->SetTreble(level);
}


void CVolumeControl::SetBassRange( int iRange ) 
{
    m_pImp->SetBassRange(iRange);
}

void CVolumeControl::SetBassRange( int iValueCount, int* iValues )
{
    m_pImp->SetBassRange(iValueCount, iValues);
}

int CVolumeControl::GetBassRange() const 
{
    return m_pImp->GetBassRange();
}

int CVolumeControl::BassUp()
{
    return m_pImp->BassUp();
}

int CVolumeControl::BassDown() 
{
    return m_pImp->BassDown();
}

int CVolumeControl::GetBass() const 
{
    return m_pImp->GetBass();
}

int CVolumeControl::SetBass( int level ) 
{
    return m_pImp->SetBass(level);
}

bool CVolumeControl::IsMuted()
{
    return m_pImp->IsMuted();
}

void CVolumeControl::ToggleMute(int rate)
{
    m_pImp->ToggleMute(rate);
}




