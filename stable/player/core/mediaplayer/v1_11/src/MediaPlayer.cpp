//
// MediaPlayer.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <core/mediaplayer/MediaPlayer.h>
#include <core/mediaplayer/PlayStream.h>
#include "MediaPlayerImp.h"

//
// singleton support
//
static CMediaPlayer* s_pSingleton = 0;

CMediaPlayer* CMediaPlayer::GetInstance() 
{
    if( s_pSingleton == 0 ) {
        s_pSingleton = new CMediaPlayer;
    }
    return s_pSingleton;
}

void CMediaPlayer::Destroy()
{
    if( s_pSingleton ) delete s_pSingleton;
    s_pSingleton = NULL;
}

CMediaPlayer::CMediaPlayer() 
{
    m_pImp = CMediaPlayerImp::GetInstance();
}

CMediaPlayer::~CMediaPlayer() 
{
    m_pImp->Destroy();
}


ERESULT CMediaPlayer::SetPlayStreamSettings( const CPlayStreamSettings* pSettings ) 
{
    return m_pImp->SetPlayStreamSettings(pSettings);
}

void CMediaPlayer::SetCreateMetadataFunction(FNCreateMetadata* pfnCreateMetadata)
{
    m_pImp->SetCreateMetadataFunction(pfnCreateMetadata);
}

void CMediaPlayer::SetCreateInputStreamFunction(FNCreateInputStream* pfnCreateInputStream)
{
    m_pImp->SetCreateInputStreamFunction(pfnCreateInputStream);
}

void CMediaPlayer::SetCreatePlayStreamFunction(FNCreatePlayStream* pfnCreatePlayStream)
{
    m_pImp->SetCreatePlayStreamFunction(pfnCreatePlayStream);
}

void CMediaPlayer::Deconfigure() 
{
    m_pImp->Deconfigure();
}

void CMediaPlayer::SetCodecPool( unsigned int* pPoolAddress, unsigned int iPoolSize ) 
{
    m_pImp->SetCodecPool( pPoolAddress, iPoolSize );
}

ERESULT CMediaPlayer::SetSRCBlending( int MinSoftwareSampleRate ) 
{
    return m_pImp->SetSRCBlending( MinSoftwareSampleRate );
}

int CMediaPlayer::QuerySRCBlending() const
{
    return m_pImp->QuerySRCBlending();
}

ERESULT CMediaPlayer::SetSong( IPlaylistEntry* pNewSong ) 
{
    return m_pImp->SetSong(pNewSong);
}

ERESULT CMediaPlayer::SetNextSong( IPlaylistEntry* pNewSong ) 
{
    return m_pImp->SetNextSong(pNewSong);
}
ERESULT CMediaPlayer::InvalidateNextSong()
{
    return m_pImp->InvalidateNextSong();
}
#if 0
StreamNode_t* CMediaPlayer::GetRoot() const 
{
    return m_pImp->GetRoot();
}

StreamNode_t* CMediaPlayer::AddNode( StreamNode_t* pRoot, unsigned int ChildID ) 
{
    return m_pImp->AddNode( pRoot, ChildID );
}

ERESULT CMediaPlayer::RemoveNode( unsigned int NodeID ) 
{
    return m_pImp->RemoveNode( NodeID );
}

ERESULT CMediaPlayer::RemoveNode( StreamNode_t* pNode ) 
{
    return m_pImp->RemoveNode( pNode );
}
#endif
ERESULT CMediaPlayer::Play() 
{
    return m_pImp->Play();
}

ERESULT CMediaPlayer::Pause() 
{
    return m_pImp->Pause();
}

ERESULT CMediaPlayer::Stop() 
{
    return m_pImp->Stop();
}

ERESULT CMediaPlayer::Seek( unsigned long secSeek ) 
{
    return m_pImp->Seek(secSeek);
}

CMediaPlayer::PlayState CMediaPlayer::GetPlayState() const 
{
    return m_pImp->GetPlayState();
}

unsigned long CMediaPlayer::GetTrackTime() const 
{
    return m_pImp->GetTrackTime();
}

unsigned long CMediaPlayer::GetTrackLength() const 
{
    return m_pImp->GetTrackLength();
}
