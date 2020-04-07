//
// PlayManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <core/playmanager/PlayManager.h>
#include "PlayManagerImp.h"

#include <util/debug/debug.h>

DEBUG_MODULE( PM );
DEBUG_USE_MODULE( PM );

// The global singleton play manager.
static CPlayManager* s_pPlayManager = 0;

CPlayManager* CPlayManager::GetInstance() 
{
    if (s_pPlayManager == NULL)
        s_pPlayManager = new CPlayManager;
    return s_pPlayManager;
}

void CPlayManager::Destroy()
{
    if( s_pPlayManager ) delete s_pPlayManager;
    s_pPlayManager = NULL;
}

CPlayManager::CPlayManager() 
{
    m_pPlayManagerImp = new CPlayManagerImp;
    DBASSERT(PM, m_pPlayManagerImp, "Error constructing implementation class");
}

CPlayManager::~CPlayManager() 
{
    Deconfigure();
    delete m_pPlayManagerImp;
}

void CPlayManager::AddDataSource( IDataSource* pDS ) 
{
    m_pPlayManagerImp->AddDataSource( pDS );
}

void CPlayManager::RefreshAllContent(IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    m_pPlayManagerImp->RefreshAllContent(mode, iUpdateChunkSize);
}

unsigned short CPlayManager::RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    return m_pPlayManagerImp->RefreshContent(iDataSourceID, mode, iUpdateChunkSize);
}

void CPlayManager::NotifyContentUpdate( content_record_update_t* pContentUpdate )
{
    m_pPlayManagerImp->NotifyContentUpdate(pContentUpdate);
}

void CPlayManager::NotifyContentMetadataUpdate( content_record_update_t* pContentUpdate )
{
    m_pPlayManagerImp->NotifyContentMetadataUpdate(pContentUpdate);
}

void CPlayManager::SetContentManager( IContentManager* pCM ) 
{
    m_pPlayManagerImp->SetContentManager(pCM);
}

IContentManager* CPlayManager::GetContentManager() const 
{
    return m_pPlayManagerImp->GetContentManager();
}

void CPlayManager::SetPlaylist( IPlaylist* pPL ) 
{
    m_pPlayManagerImp->SetPlaylist(pPL);
}

IPlaylist* CPlayManager::GetPlaylist() const 
{
    return m_pPlayManagerImp->GetPlaylist();
}

void CPlayManager::SetPlaystream( const CPlayStreamSettings* pSettings, FNCreatePlayStream* pfnCreatePlayStream ) 
{
    m_pPlayManagerImp->SetPlaystream( pSettings, pfnCreatePlayStream );
}

IPlaylist::PlaylistMode CPlayManager::GetPlaylistMode() const
{
    return m_pPlayManagerImp->GetPlaylistMode();
}

void CPlayManager::SetPlaylistMode(IPlaylist::PlaylistMode mode)
{
    m_pPlayManagerImp->SetPlaylistMode(mode);
}

ERESULT CPlayManager::SetSong( IPlaylistEntry* pNewSong )
{
    m_pPlayManagerImp->SetSong(pNewSong);
}

void CPlayManager::Deconfigure()
{
    m_pPlayManagerImp->Deconfigure();
}

ERESULT CPlayManager::Play() 
{
    return m_pPlayManagerImp->Play();
}

ERESULT CPlayManager::Pause() 
{
    return m_pPlayManagerImp->Pause();
}

ERESULT CPlayManager::Stop() 
{
    return m_pPlayManagerImp->Stop();
}

ERESULT CPlayManager::Seek( unsigned long secSeek ) 
{
    return m_pPlayManagerImp->Seek( secSeek );
}

CMediaPlayer::PlayState CPlayManager::GetPlayState() const
{
    return m_pPlayManagerImp->GetPlayState( );
}

ERESULT CPlayManager::NextTrack()
{
    return m_pPlayManagerImp->NextTrack();
}

ERESULT CPlayManager::PreviousTrack()
{
    return m_pPlayManagerImp->PreviousTrack();
}

// The default event handler.
ERESULT CPlayManager::HandleEvent( int key, void* data )
{
    return m_pPlayManagerImp->HandleEvent(key, data);
}

// Called from the event handler when a data source's media is removed.
void CPlayManager::NotifyMediaRemoved(int iDataSourceID)
{
    m_pPlayManagerImp->NotifyMediaRemoved(iDataSourceID);
}

// Called from the event handler when a data source's media is changed.
void CPlayManager::NotifyMediaInserted(int iDataSourceID)
{
    m_pPlayManagerImp->NotifyMediaInserted(iDataSourceID);
}

