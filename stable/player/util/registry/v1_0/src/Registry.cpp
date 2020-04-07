// Registry.cpp: system wide class registry
// danc@iobjects.com 07/08/01
// (c) Interactive Objects

#include "RegistryImp.h"
#include <util/debug/debug.h>

//
// C style API
// note that these routines have C linkage
//

int registry_add_item( RegKey key, void* data, short flags, short length ) 
{
    return CRegistry::GetInstance()->AddItem( key, data, flags, length );
}

void* registry_find_by_key( RegKey key ) 
{
    return CRegistry::GetInstance()->FindByKey( key );
}

ERESULT registry_find_by_type( RegKey type, RegKey* table, int size ) 
{
    return CRegistry::GetInstance()->FindByType( type, table, size );
}

void* registry_remove_item( RegKey key ) 
{
    return CRegistry::GetInstance()->RemoveItem( key );
}


//
// CRegistry implementation
//

static CRegistry* s_pSingleton = 0;

CRegistry* CRegistry::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CRegistry;
    }
    return s_pSingleton;
}

CRegistry::CRegistry() 
{
    m_pImp = new CRegistryImp;
}
CRegistry::~CRegistry() 
{
    delete m_pImp;
}

int CRegistry::AddItem( RegKey key, void* data, short flags, short length ) 
{
    return m_pImp->AddItem(key, data, flags, length);
}

void* CRegistry::FindByKey( RegKey key ) 
{
    return m_pImp->FindByKey(key);
}

ERESULT CRegistry::FindByKey( RegKey key, void** ppData )
{
    return m_pImp->FindByKey(key, ppData);
}

int CRegistry::FindByType( RegKey type, RegKey* pTable, int size )
{
    return m_pImp->FindByType(type, pTable, size );
}


void* CRegistry::RemoveItem( RegKey key ) 
{
    return m_pImp->RemoveItem(key);
}

ERESULT CRegistry::SaveState( IOutputStream* pOutput ) 
{
    return m_pImp->SaveState(pOutput);
}

ERESULT CRegistry::RestoreState( IInputStream* pInput ) 
{
    return m_pImp->RestoreState(pInput);
}
