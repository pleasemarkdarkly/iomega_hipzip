// OutputManager.cpp: output manager
// danc@iobjects.com 07/13/01
// (c) Interactive Objects

#include <datastream/outputmanager/OutputManager.h>
#include <datastream/output/OutputStream.h>

#include <util/registry/Registry.h>

#define MAX_OUTPUTS 10

#define NULL 0

typedef struct output_table_s 
{
    RegKey ID;
    IOutputStream* (*Create)();
} output_table_t;


COutputManager* COutputManager::m_pSingleton = NULL;

COutputManager* COutputManager::GetInstance() 
{
    if( m_pSingleton == NULL ) {
        m_pSingleton = new COutputManager;
    }
    return m_pSingleton;
}

COutputManager::COutputManager() 
{
    m_pRegistry = CRegistry::GetInstance();

    // find all available output streams
    RegKey KeyTable[ MAX_OUTPUTS ];

    m_iTableSize = m_pRegistry->FindByType(
        OUTPUT_TYPE_ID,
        &KeyTable[0],
        MAX_OUTPUTS );

    if( m_iTableSize > 0 ) {
        m_pOutputTable = new output_table_t[ m_iTableSize ];

        for( int i = 0; i < m_iTableSize; i++ ) {
            m_pOutputTable[i].Create =
                (IOutputStream*(*)()) m_pRegistry->FindByKey( KeyTable[i] );
            m_pOutputTable[i].ID = KeyTable[i];
        }
    }
    else {
        m_pOutputTable = NULL;
    }
}

COutputManager::~COutputManager() 
{
    if( m_iTableSize > 0 ) {
        delete [] m_pOutputTable;
    }
}

IOutputStream* COutputManager::LoadOutputStream( unsigned int OutputID ) 
{
    RegKey key = OutputID;
    for( int i = 0; i < m_iTableSize; i++ ) {
        if( REGKEY_NAME( m_pOutputTable[i].ID ) == key ) {
            return m_pOutputTable[i].Create();
        }
    }
    return NULL;
}
