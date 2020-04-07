// RegistryImp.cpp: system wide class registry
// danc@iobjects.com 07/08/01
// (c) Interactive Objects

#include "RegistryImp.h"
#include <util/registry/Registry.h>

#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <util/debug/debug.h>
#include <string.h>   // memset

DEBUG_MODULE( REGISTRY );
DEBUG_USE_MODULE( REGISTRY );

//
// definition for registry_item_t
//
typedef struct registry_item_s 
{
    RegKey Key;
    unsigned short Flags;
    unsigned short Length;
    void* Data;
} registry_item_t;


//
// CRegistryImp implementation
//

CRegistryImp::CRegistryImp() 
{
}
CRegistryImp::~CRegistryImp() 
{
}

int CRegistryImp::AddItem( RegKey key, void* data, short flags, short length ) 
{
    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( m_registry[i].Key == key ) {
            DEBUG( REGISTRY, DBGLEV_ERROR, "Key overlap in registry, dropped AddItem request\n");
            return REG_ERR_OVERLAP;
        }
    }
    registry_item_t reg_item = { Key : key, Flags : flags, Length : length, Data : data };
    m_registry.PushBack(reg_item);
    return REG_ERR_NONE;
}

void* CRegistryImp::FindByKey( RegKey key ) 
{
    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( m_registry[i].Key == key ) {
            return m_registry[i].Data;
        }
    }
    return NULL;
}

ERESULT CRegistryImp::FindByKey( RegKey key, void** ppData )
{
    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( m_registry[i].Key == key ) {
            *ppData = m_registry[i].Data;
            return REG_ERR_NONE;
        }
    }
    return REG_ERR_KEY_NOT_FOUND;
}

int CRegistryImp::FindByType( RegKey type, RegKey* pTable, int size )
{
    int count = 0;
    
    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( REGKEY_TYPE( m_registry[i].Key ) == type ) {
            pTable[ count++ ] = m_registry[i].Key;
            
            if( count == size ) {
                break;
            }
        }
    }

    return count;
}


void* CRegistryImp::RemoveItem( RegKey key ) 
{
    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( m_registry[i].Key == key ) {
            void* pData = m_registry[i].Data;
            m_registry.Remove(i);
            return pData;
        }
    }
    return 0;
}

ERESULT CRegistryImp::SaveState( IOutputStream* pOutput ) 
{
    ERESULT res = REG_ERR_NONE;

    for( int i = 0; i < m_registry.Size(); i++ ) {
        if( m_registry[i].Flags & REGFLAG_PERSISTENT ) {
            int len = sizeof( RegKey ) + 4;
            char* buf = new char[ len + m_registry[i].Length ];

            DBASSERT( REGISTRY, buf != 0, "Failed to allocate buffer\n");
            memcpy( buf, &m_registry[i], len );
            if( m_registry[i].Length <= 4 ) {
                memcpy( (buf+len), &( m_registry[i].Data ), 4 );
                len += 4;
            } else {
                memcpy( (buf+len), m_registry[i].Data, m_registry[i].Length );
                len += m_registry[i].Length;
            }
            if( len != pOutput->Write( buf, len ) ) {
                res = REG_ERR_IO_FAIL;
            }
            delete [] buf;
        }
    }
    
    return res;
}

static void CopyRegItem( registry_item_t* dest, const registry_item_t* src ) 
{
    // implicit in the memcpy is that the data ptr is passed off
    memcpy( dest, src, sizeof( registry_item_t ) );
}

ERESULT CRegistryImp::RestoreState( IInputStream* pInput ) 
{
    ERESULT res = REG_ERR_NONE;

    registry_item_t tmp_item;
    int free_index = -1;
    while( pInput->Read( &tmp_item, sizeof( tmp_item ) ) == sizeof( tmp_item ) ) {
        if( tmp_item.Length > 4 ) {
            unsigned int word = (unsigned int)tmp_item.Data;
            tmp_item.Data = (void*) new char[tmp_item.Length];
            DBASSERT( REGISTRY, tmp_item.Data != 0, "Failed to allocate data\n");
            memcpy( tmp_item.Data, &word, 4 );
            pInput->Read( ((unsigned char*)tmp_item.Data)+4, tmp_item.Length-4 );
        }

        int i = 0;
        for( ; i < m_registry.Size(); i++ ) {
            if( m_registry[i].Key == tmp_item.Key ) {
                CopyRegItem( &m_registry[i], &tmp_item );
                break;
            }
        }

        if (i == m_registry.Size())
            m_registry.PushBack(tmp_item);
    }

    return res;
}
