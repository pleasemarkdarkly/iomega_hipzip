// PlaylistFormatManager.cpp: system for loading and saving playlists of various types
// edwardm@iobjects.com 07/30/01
// (c) Interactive Objects

#include "PlaylistFormatManagerImp.h"

#include <content/common/ContentManager.h>
#include <playlist/plformat/common/PlaylistFormat.h>
#include <util/debug/debug.h>
#include <util/registry/Registry.h>


#define MAX_PLAYLIST_FORMATS            10

DEBUG_USE_MODULE( PFM );

//
// data structures
//


typedef struct playlist_format_table_s 
{
    RegKey ID;
    const char* const* pExtensionTable;
    playlist_format_function_map_t* pFunctions;
} playlist_format_table_t;



CPlaylistFormatManagerImp::CPlaylistFormatManagerImp() 
{
    CRegistry* pRegistry = CRegistry::GetInstance();
    
    // Find all the given codecs in the registry
    playlist_format_function_map_t* pFunctions;
    
    RegKey KeyTable[ MAX_PLAYLIST_FORMATS ];
    
    m_iTableSize = pRegistry->FindByType(
        PLAYLIST_FORMAT_TYPE_ID,
        &KeyTable[0],
        MAX_PLAYLIST_FORMATS );

    if (m_iTableSize)
    {
        m_pPlaylistFormatTable = new playlist_format_table_t[ m_iTableSize ];

        // we should have a list of codec keys. now step through the registry,
        // and get a list of supported extensions. also retain the function map
        for( unsigned int i = 0; i < m_iTableSize; i++ ) {
            pFunctions = (playlist_format_function_map_t*) pRegistry->FindByKey( KeyTable[i] );
            m_pPlaylistFormatTable[i].ID = KeyTable[i];
            m_pPlaylistFormatTable[i].pExtensionTable = pFunctions->GetExtensionList();
            m_pPlaylistFormatTable[i].pFunctions = pFunctions;
        }
    }
    else
        m_iTableSize = 0;
}

CPlaylistFormatManagerImp::~CPlaylistFormatManagerImp() 
{
    delete [] m_pPlaylistFormatTable;
}

unsigned int CPlaylistFormatManagerImp::FindPlaylistFormat( const char* szExtension )
{
    if (playlist_format_table_t* pFormat = FindPlaylistFormatRecord(szExtension))
    {
        DEBUGP( PFM, DBGLEV_TRACE, "Matched extension %s\n", szExtension );
        return pFormat->ID;
    }
    else
        return 0;
}


ERESULT
CPlaylistFormatManagerImp::LoadPlaylist( int iPlaylistFormatID, const char* szURL, IMediaRecordInfoVector& records, int iMaxTracks = 0 ) 
{
    // Find a matching playlist ID.
    for( unsigned int i = 0; i < m_iTableSize; i++ )
    {
        if (m_pPlaylistFormatTable[i].ID == iPlaylistFormatID)
        {
            // Ask the matching playlist format to load all the track info into a vector.
            return m_pPlaylistFormatTable[i].pFunctions->pfnLoadPlaylist(records, szURL, iMaxTracks);
        }
    }

    return PLAYLIST_FORMAT_UNKNOWN_FORMAT;
}

ERESULT CPlaylistFormatManagerImp::SavePlaylist( int iPlaylistFormatID, const char* szURL, IPlaylist* pPlaylist )
{
    for( unsigned int i = 0; i < m_iTableSize; i++ )
    {
        if (m_pPlaylistFormatTable[i].ID == iPlaylistFormatID)
        {
            return m_pPlaylistFormatTable[i].pFunctions->pfnSavePlaylist(szURL, pPlaylist);
        }
    }

    return PLAYLIST_FORMAT_UNKNOWN_FORMAT;
}

playlist_format_table_t* CPlaylistFormatManagerImp::FindPlaylistFormatRecord( const char* szExtension )
{
    // enumerate through the registered codec, then step through
    // their extension tables, and attempt to find a match
    for( unsigned int i = 0; i < m_iTableSize; i++ )
    {
        const char*const* pExt = m_pPlaylistFormatTable[i].pExtensionTable;
        for( ; *pExt; pExt++ )
        {
            if( strnicmp( *pExt, szExtension, 3 ) == 0 )
                return &(m_pPlaylistFormatTable[i]);
        }
    }
    return 0;
}

