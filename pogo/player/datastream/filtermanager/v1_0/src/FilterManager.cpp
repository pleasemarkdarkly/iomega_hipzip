// FilterManager.cpp filter manager
// danc@iobjects.com 07/12/01
// (c) Interactive Objects

#include <datastream/filtermanager/FilterManager.h>
#include <datastream/filter/Filter.h>
#include <util/registry/Registry.h>

#define MAX_FILTERS 10

#define NULL 0

// local types
typedef struct filter_table_s 
{
    RegKey ID;
    IFilter* (*Create)();
} filter_table_t;


CFilterManager* CFilterManager::m_pSingleton = NULL;

CFilterManager* CFilterManager::GetInstance() 
{
    if( m_pSingleton == NULL ) {
        m_pSingleton = new CFilterManager;
    }
    return m_pSingleton;
}

CFilterManager::CFilterManager() 
{
    // build out the filter data table
    m_pRegistry = CRegistry::GetInstance();

    // Find all available filters
    RegKey KeyTable[ MAX_FILTERS ];

    m_iTableSize = m_pRegistry->FindByType(
        FILTER_TYPE_ID,
        &KeyTable[0],
        MAX_FILTERS );

    if( m_iTableSize > 0 ) {
        m_pFilterTable = new filter_table_t[ m_iTableSize ];

        for( int i = 0; i < m_iTableSize; i++ ) {
            m_pFilterTable[i].Create =
                (IFilter*(*)()) m_pRegistry->FindByKey( KeyTable[i] );
            m_pFilterTable[i].ID = KeyTable[i];
        }
    }
    else {
        m_pFilterTable = NULL;
    }
}

CFilterManager::~CFilterManager()
{
    if( m_iTableSize > 0 ) {
        delete [] m_pFilterTable;
    }
}

IFilter* CFilterManager::LoadFilter( unsigned int FilterID ) 
{
    RegKey key = FilterID;

    for( int i = 0; i < m_iTableSize; i++ ) {
        if( REGKEY_NAME( m_pFilterTable[i].ID ) == key ) {
            return m_pFilterTable[i].Create();
        }
    }
    return NULL;
}

