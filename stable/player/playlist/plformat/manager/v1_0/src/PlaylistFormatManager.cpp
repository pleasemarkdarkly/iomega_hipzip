//
// PlaylistFormatManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <playlist/plformat/manager/PlaylistFormatManager.h>

#include "PlaylistFormatManagerImp.h"
#include <playlist/plformat/common/PlaylistFormat.h>
#include <util/debug/debug.h>

DEBUG_MODULE_S( PFM, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( PFM );

// implicit is that the playlist format manager should never
// be instantiated as a global, or from a global constructor

static CPlaylistFormatManager* s_pPlaylistFormatManager = 0;


CPlaylistFormatManager* CPlaylistFormatManager::GetInstance() 
{
    if( s_pPlaylistFormatManager == 0 ) {
        s_pPlaylistFormatManager = new CPlaylistFormatManager;
    }
    return s_pPlaylistFormatManager;
}

CPlaylistFormatManager::CPlaylistFormatManager() 
{
    m_pPlaylistFormatManagerImp = new CPlaylistFormatManagerImp;
    DBASSERT(PFM, m_pPlaylistFormatManagerImp, "Error constructing implementation class");
}

CPlaylistFormatManager::~CPlaylistFormatManager() 
{
    delete m_pPlaylistFormatManagerImp;
}

unsigned int CPlaylistFormatManager::FindPlaylistFormat( const char* szExtension )
{
    return m_pPlaylistFormatManagerImp->FindPlaylistFormat(szExtension);
}

ERESULT CPlaylistFormatManager::LoadPlaylist( int iPlaylistFormatID, const char* szURL, IMediaRecordInfoVector& records, int iMaxTracks = 0 ) 
{
    return m_pPlaylistFormatManagerImp->LoadPlaylist(iPlaylistFormatID, szURL, records, iMaxTracks );
}

ERESULT CPlaylistFormatManager::SavePlaylist( int iPlaylistFormatID, const char* szURL, IPlaylist* pPlaylist )
{
    return m_pPlaylistFormatManagerImp->SavePlaylist(iPlaylistFormatID, szURL, pPlaylist);
}

