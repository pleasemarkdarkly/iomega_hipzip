//
// PlayManagerImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "PlayManagerImp.h"

#include <codec/common/Codec.h>              // set_stream_event_data_t definition
#include <content/common/ContentManager.h>
#include <content/common/Metadata.h>
#include <core/events/SystemEvents.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/common/DataSource.h>
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/PlayStream.h>

#include <datasource/cddatasource/CDDataSource.h>
#include <main/main/DJPlayerState.h>

#include <_modules.h>
#include <util/debug/debug.h>

#if defined(DDOMOD_EXTRAS_CDMETADATA)
#include <extras/cdmetadata/CDMetadataEvents.h>
#include <extras/cdmetadata/DiskInfo.h>
#endif

DEBUG_USE_MODULE( PM );

CPlayManagerImp::CPlayManagerImp() 
{
    m_pContentManager = NULL;
    m_pPlaylist = NULL;
    m_pMediaPlayer = CMediaPlayer::GetInstance();
    m_pDataSourceManager = CDataSourceManager::GetInstance();
    m_playlistMode = IPlaylist::NORMAL;
    m_playState = m_pMediaPlayer->GetPlayState();
}

CPlayManagerImp::~CPlayManagerImp() 
{
    delete m_pPlaylist;
}

void CPlayManagerImp::AddDataSource( IDataSource* pDS ) 
{
    m_pDataSourceManager->AddDataSource( pDS );
}

void CPlayManagerImp::RefreshAllContent(IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    int count = m_pDataSourceManager->GetDataSourceCount();
    for( int i = 0; i < count; i++ )
    {
        if (IDataSource* pDS = m_pDataSourceManager->GetDataSourceByIndex( i ))
        {
            m_pDataSourceManager->RefreshContent(pDS->GetInstanceID(), mode, iUpdateChunkSize);
        }
    }
}

unsigned short CPlayManagerImp::RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    return m_pDataSourceManager->RefreshContent(iDataSourceID, mode, iUpdateChunkSize);
}

void CPlayManagerImp::NotifyContentUpdate( content_record_update_t* pContentUpdate )
{
    m_pContentManager->AddContentRecords( pContentUpdate );
    if (pContentUpdate->bTwoPass && !pContentUpdate->media.IsEmpty())
    {
        DEBUG(PM, DBGLEV_INFO, "Beginning second pass on DS %d\n", pContentUpdate->iDataSourceID);
        m_pDataSourceManager->GetContentMetadata(pContentUpdate);
    }
    else
        delete pContentUpdate;
}

void CPlayManagerImp::NotifyContentMetadataUpdate( content_record_update_t* pContentUpdate )
{
    m_pContentManager->AddContentRecords( pContentUpdate );
    delete pContentUpdate;
}

void CPlayManagerImp::SetContentManager( IContentManager* pCM ) 
{
    m_pContentManager = pCM;
}

IContentManager* CPlayManagerImp::GetContentManager() const 
{
    return m_pContentManager;
}

void CPlayManagerImp::SetPlaylist( IPlaylist* pPL ) 
{
    m_pPlaylist = pPL;
}

IPlaylist* CPlayManagerImp::GetPlaylist() const 
{
    return m_pPlaylist;
}

void CPlayManagerImp::SetPlaystream( const CPlayStreamSettings* pSettings, FNCreatePlayStream* pfnCreatePlayStream ) 
{
    // we can ditch this error code, it's probably not important :>
    m_pMediaPlayer->SetPlayStreamSettings( pSettings );
    m_pMediaPlayer->SetCreatePlayStreamFunction( pfnCreatePlayStream );
}

void CPlayManagerImp::SetPlaylistMode(IPlaylist::PlaylistMode mode)
{
    if (m_pPlaylist && ((mode == IPlaylist::RANDOM) || (mode == IPlaylist::REPEAT_RANDOM)))
    {
        m_pPlaylist->ReshuffleRandomEntries();
    }
    if( m_pPlaylist ) {
        if( m_playlistMode == IPlaylist::RANDOM || m_playlistMode == IPlaylist::REPEAT_RANDOM ||
            mode == IPlaylist::RANDOM || mode == IPlaylist::REPEAT_RANDOM ||
            (mode == IPlaylist::NORMAL && m_playlistMode == IPlaylist::REPEAT_ALL &&
                !m_pPlaylist->GetNextEntry(m_pPlaylist->GetCurrentEntry(), mode))) {
        
            // The playlist mode changed
            // The logic here is clunky; certain play mode transitions dont warrant clearing out the autoset track
            // basically, if we were in a random mode, or are switching to a random mode, assume that the next
            // track is no longer valid; otherwise, if we were in repeat all and are switching to normal, and are
            // on the last track, the next track (which is the first track in the playlist) is no longer valid
            m_pMediaPlayer->InvalidateNextSong();
        }
    }
            
    m_playlistMode = mode;
}

ERESULT CPlayManagerImp::SetSong( IPlaylistEntry* pNewSong )
{
    ERESULT res = m_pMediaPlayer->SetSong(pNewSong);

    if (SUCCEEDED(res))
    {
        if (m_playState == CMediaPlayer::NOT_CONFIGURED || m_playState == CMediaPlayer::PAUSED)
            m_playState = CMediaPlayer::STOPPED;
        return PM_NO_ERROR;
    }
    else
        return PM_ERROR;
}

void CPlayManagerImp::Deconfigure()
{
    m_pMediaPlayer->Deconfigure();
    m_playState = CMediaPlayer::NOT_CONFIGURED;
}

ERESULT CPlayManagerImp::Play() 
{
    static char SpinBuf[2352]; /* Junk buffer to use in read to spin up CD drive. */
    
    if (m_pMediaPlayer->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
    {
        if (IPlaylistEntry* pEntry = m_pPlaylist->GetCurrentEntry())
        {
            ERESULT res = m_pMediaPlayer->SetSong(pEntry);
            if (SUCCEEDED(res))
            {
                
                m_playState = CMediaPlayer::PLAYING;
                /* Spin the drive to prevent a pause in audio when it comes out of sleep.  The pause
                   comes from the CD drive locking the ATA bus while the decoder is trying to write
                   ripped data to the HD.  Spinning now puts the pause before playback begins. */
                if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD) {
                    CDJPlayerState::GetInstance()->GetCDDataSource()->Read(0,1,SpinBuf);
                }
                if( FAILED(res = m_pMediaPlayer->Play()) ) {
                    m_playState = m_pMediaPlayer->GetPlayState();
                }
                return res;
            }
            else
            {
                DEBUG(PM, DBGLEV_WARNING, "Unable to set track: %s\n", pEntry->GetContentRecord()->GetURL());
                return res;
            }
        }
        else
            return PM_NO_GOOD_TRACKS;
    }
    else
    {
        m_playState = CMediaPlayer::PLAYING;
        /* See comment above. */
        if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD) {
            CDJPlayerState::GetInstance()->GetCDDataSource()->Read(0,1,SpinBuf);
        }
        ERESULT res = m_pMediaPlayer->Play();
        if( FAILED(res) )
            m_playState = m_pMediaPlayer->GetPlayState();
        return res;
    }
}

ERESULT CPlayManagerImp::Pause() 
{
    ERESULT res = m_pMediaPlayer->Pause();
    if (SUCCEEDED(res))
        m_playState = CMediaPlayer::PAUSED;
    else
        m_playState = m_pMediaPlayer->GetPlayState();
    return res;
}

ERESULT CPlayManagerImp::Stop() 
{
    ERESULT res = m_pMediaPlayer->Stop();
    if (SUCCEEDED(res))
    {
        // Reset the current track.
        if (m_pPlaylist)
        {
            IPlaylistEntry* pEntry = m_pPlaylist->GetCurrentEntry();
            if (pEntry)
                res = m_pMediaPlayer->SetSong(pEntry);
        }
    }
    
    // sync the playstate
    m_playState = m_pMediaPlayer->GetPlayState();

    // return a mapped error code
    if( SUCCEEDED(res) ) {
        return PM_NO_ERROR;
    }
    return PM_ERROR;
}

ERESULT CPlayManagerImp::Seek( unsigned long secSeek ) 
{
    return m_pMediaPlayer->Seek( secSeek );
}

CMediaPlayer::PlayState CPlayManagerImp::GetPlayState()
{
    // Resynchronize the playstate here if we are in the middle of a transition
    //    if (m_playState == CMediaPlayer::NOT_CONFIGURED)
    //        m_playState = m_pMediaPlayer->GetPlayState();
    return m_playState;
}

ERESULT CPlayManagerImp::NextTrack(bool bBacktrackIfNeeded)
{
    IPlaylistEntry* pNextEntry = m_pPlaylist->SetNextEntry(m_playlistMode);
    ERESULT res = PM_ERROR;
    CMediaPlayer::PlayState lastState = m_playState;
    if( pNextEntry ) {
        IPlaylistEntry* pCurrentEntry = m_pPlaylist->GetCurrentEntry();
        do {
            // Determine what the next play state should be prior to setting the track
            m_playState = ( m_playState == CMediaPlayer::PLAYING ? m_playState : CMediaPlayer::STOPPED );
            
            res = m_pMediaPlayer->SetSong( pNextEntry );
            if (SUCCEEDED(res))
            {
                if ((lastState == CMediaPlayer::PLAYING) && (m_pMediaPlayer->GetPlayState() != CMediaPlayer::PLAYING))
                {
                    if (SUCCEEDED(m_pMediaPlayer->Play()))
                        return PM_PLAYING;
                }
                else if (m_playState == CMediaPlayer::NOT_CONFIGURED)
                    m_playState = m_pMediaPlayer->GetPlayState();
                return res;
            }
            else
                DEBUG(PM, DBGLEV_WARNING, "Unable to set track: %s\n", pNextEntry->GetContentRecord()->GetURL());
            pNextEntry = m_pPlaylist->SetNextEntry(m_playlistMode);
        } while (pNextEntry && (pNextEntry != pCurrentEntry));
        
        // If a good track wasn't found, then return to the original track.
        if (FAILED(res) && bBacktrackIfNeeded)
        {
            DEBUG(PM, DBGLEV_WARNING, "No good next track found, returning to previous good track\n");
            if (m_pPlaylist->SetCurrentEntry(pCurrentEntry))
            {
                if (SUCCEEDED(res = m_pMediaPlayer->SetSong(pCurrentEntry)))
                {
                    if ((lastState == CMediaPlayer::PLAYING) && (m_pMediaPlayer->GetPlayState() != CMediaPlayer::PLAYING))
                    {
                        if (SUCCEEDED(m_pMediaPlayer->Play()))
                            return PM_PLAYING;
                    }
                    else if (m_playState == CMediaPlayer::NOT_CONFIGURED)
                        m_playState = m_pMediaPlayer->GetPlayState();
                    return res;
                }
                
                // Couldn't even set the current song.  This is bad.
                DEBUG(PM, DBGLEV_WARNING, "Couldn't set original track: %s\n", pCurrentEntry->GetContentRecord()->GetURL());
                
                // Try to go to the previous track.
                if (FAILED(res = PreviousTrack(false)))
                {
                    DEBUG(PM, DBGLEV_WARNING, "No good files found\n");
                    // This playlist is invalid.  Tell someone.
                    return PM_NO_GOOD_TRACKS;
                }
            }
        }
        
        return res;
    }
    else
        return PM_PLAYLIST_END;
}

ERESULT CPlayManagerImp::PreviousTrack(bool bBacktrackIfNeeded)
{
    IPlaylistEntry* pPreviousEntry = m_pPlaylist->SetPreviousEntry(m_playlistMode);
    ERESULT res = PM_ERROR;
    CMediaPlayer::PlayState lastState = m_playState;
    if( pPreviousEntry ) {
        IPlaylistEntry* pCurrentEntry = m_pPlaylist->GetCurrentEntry();
        do {
            // Determine what the next play state should be prior to setting the track
            m_playState = ( m_playState == CMediaPlayer::PLAYING ? m_playState : CMediaPlayer::STOPPED );
            res = m_pMediaPlayer->SetSong( pPreviousEntry );
            if (SUCCEEDED(res))
            {
                if ((lastState == CMediaPlayer::PLAYING) && (m_pMediaPlayer->GetPlayState() != CMediaPlayer::PLAYING))
                {
                    if (SUCCEEDED(m_pMediaPlayer->Play()))
                        return PM_PLAYING;
                }
                else if (m_playState == CMediaPlayer::NOT_CONFIGURED)
                    m_playState = m_pMediaPlayer->GetPlayState();
                return res;
            }
            else
                DEBUG(PM, DBGLEV_WARNING, "Unable to set track: %s\n", pPreviousEntry->GetContentRecord()->GetURL());
            pPreviousEntry = m_pPlaylist->SetPreviousEntry(m_playlistMode);
        } while (pPreviousEntry && (pPreviousEntry != pCurrentEntry));
        
        // If a good track wasn't found, then return to the original track.
        if (FAILED(res) && bBacktrackIfNeeded)
        {
            DEBUG(PM, DBGLEV_WARNING, "No good next track found, returning to previous good track\n");
            if (m_pPlaylist->SetCurrentEntry(pCurrentEntry))
            {
                if (SUCCEEDED(res = m_pMediaPlayer->SetSong(pCurrentEntry)))
                {
                    if ((lastState == CMediaPlayer::PLAYING) && (m_pMediaPlayer->GetPlayState() != CMediaPlayer::PLAYING))
                    {
                        if (SUCCEEDED(m_pMediaPlayer->Play()))
                            return PM_PLAYING;
                    }
                    else if (m_playState == CMediaPlayer::NOT_CONFIGURED)
                        m_playState = m_pMediaPlayer->GetPlayState();
                    return res;
                }
                
                // Couldn't even set the current song.  This is bad.
                DEBUG(PM, DBGLEV_WARNING, "Couldn't set original track: %s\n", pCurrentEntry->GetContentRecord()->GetURL());
                
                // Try to go to the next track.
                if (FAILED(res = NextTrack(false)))
                {
                    DEBUG(PM, DBGLEV_WARNING, "No good files found\n");
                    // This playlist is invalid.  Tell someone.
                    return PM_NO_GOOD_TRACKS;
                }
            }
        }
        
        return res;
    }
    else
        return PM_PLAYLIST_END;
}

// The default event handler.
ERESULT CPlayManagerImp::HandleEvent( int key, void* data )
{
    switch( key ) {
        
        case EVENT_CONTENT_UPDATE_BEGIN:
        {
            DEBUGP( PM, DBGLEV_INFO, "Starting content update on data source %d\n", GET_DATA_SOURCE_ID((int)data));
            m_pContentManager->MarkRecordsFromDataSourceUnverified(GET_DATA_SOURCE_ID((int)data));
            break;
        }
        
        case EVENT_CONTENT_UPDATE:
        {
            NotifyContentUpdate( (content_record_update_t*)data );
            break;
        }
        
        case EVENT_CONTENT_UPDATE_END:
        {
            DEBUGP( PM, DBGLEV_INFO, "Finished content update on data source %d\n", GET_DATA_SOURCE_ID((int)data));
            m_pContentManager->DeleteUnverifiedRecordsFromDataSource(GET_DATA_SOURCE_ID((int)data));
            break;
        }
        
        case EVENT_CONTENT_UPDATE_ERROR:
        {
            DEBUGP( PM, DBGLEV_INFO, "Error updating content update on data source %d\n", GET_DATA_SOURCE_ID((int)data));
            m_pContentManager->DeleteUnverifiedRecordsFromDataSource(GET_DATA_SOURCE_ID((int)data));
            break;
        }
        
        case EVENT_CONTENT_METADATA_UPDATE_BEGIN:
        {
            DEBUGP( PM, DBGLEV_INFO, "Metadata update begin message from data source %d, scan ID %d\n", GET_DATA_SOURCE_ID((int)data), GET_SCAN_ID((int)data));
            m_pDataSourceManager->NotifyContentMetadataUpdateEnd((int)data);
            break;
        }
        
        case EVENT_CONTENT_METADATA_UPDATE:
        {
            NotifyContentMetadataUpdate( (content_record_update_t*)data );
            break;
        }

        case EVENT_STREAM_AUTOSET:
        {
            change_stream_event_data_t* pChangeEvent = (change_stream_event_data_t*)data;
            // Find the next entry with the given URL.
            if (m_pPlaylist)
            {
                IPlaylistEntry *pPE = m_pPlaylist->GetCurrentEntry();
                if (pPE)
                {
                    do
                    {
                        pPE = m_pPlaylist->GetNextEntry(pPE, m_playlistMode);
                    } while (pPE && strcmp(pPE->GetContentRecord()->GetURL(), pChangeEvent->szURL));

                    delete [] pChangeEvent->szURL;
                    //                    delete pChangeEvent->pMediaPlayerMetadata;
                    delete pChangeEvent->pPreviousStream;
                    delete pChangeEvent;

                    if (pPE != NULL)
                    {
                        m_pPlaylist->SetCurrentEntry(pPE);
                        IPlaylistEntry* pNE = m_pPlaylist->GetNextEntry(pPE, m_playlistMode);

                        if( pNE != pPE ) {
                            // Try to get the next track ready. No big deal if it fails.
                            m_pMediaPlayer->SetNextSong(pNE);
                        }
                        return PM_PLAYING;
                    }
                }
            }

            delete [] pChangeEvent->szURL;
            //            delete pChangeEvent->pMediaPlayerMetadata;
            delete pChangeEvent->pPreviousStream;
            delete pChangeEvent;

            // Weren't able to set the playlist to the currently playing entry. That really shouldn't happen.
            DEBUGP( PM, DBGLEV_ERROR, "EVENT_STREAM_AUTOSET handler couldn't set playlist to new playlist entry\n");
            return PM_ERROR;
        }
        
        case EVENT_STREAM_SET:
        {
            set_stream_event_data_t* pSSED = (set_stream_event_data_t*)data;
            delete [] pSSED->szURL;
            //            delete pSSED->pMediaPlayerMetadata;
            delete pSSED;

            // Try to get the next track ready. No big deal if it fails.
            IPlaylistEntry *pPE = m_pPlaylist->GetCurrentEntry();
            if (pPE)
            {
                IPlaylistEntry* pNE = m_pPlaylist->GetNextEntry(pPE, m_playlistMode);
                if( pPE != pNE ) {
                    m_pMediaPlayer->SetNextSong(pNE);
                }
            }
            break;
        }
        
        case EVENT_STREAM_PAUSED:
        {
            return PM_NO_ERROR;
        }

        case EVENT_STREAM_STOPPED:
        {
            change_stream_event_data_t* pChangeEvent = (change_stream_event_data_t*)data;
            delete pChangeEvent->pPreviousStream;
            delete [] pChangeEvent->szURL;
            delete pChangeEvent;
            return PM_NO_ERROR;
        }
        
        case EVENT_STREAM_END:
        case EVENT_STREAM_FAIL:
        {
            change_stream_event_data_t* pChangeEvent = (change_stream_event_data_t*)data;
            if (pChangeEvent)
            {
                //                delete pChangeEvent->pMediaPlayerMetadata;
                delete pChangeEvent->pPreviousStream;
                delete pChangeEvent;
            }
            ERESULT res = NextTrack();
            if (SUCCEEDED(res))
            {
                if (m_playState == CMediaPlayer::PLAYING)
                {
                    if (SUCCEEDED(Play()))
                        return PM_PLAYING;
                    else
                    {
                        m_playState = m_pMediaPlayer->GetPlayState();
                        return PM_NO_ERROR;
                    }
                }
                else
                    return PM_NO_ERROR;
            }
            else
            {
                m_playState = m_pMediaPlayer->GetPlayState();
                return res;
            }
        }
        
        case EVENT_MEDIA_REMOVED:
        {
            DEBUGP( PM, DBGLEV_INFO, "Data source %d media removed\n", (int)data);
            NotifyMediaRemoved((int)data);
            break;
        }
        
        case EVENT_MEDIA_INSERTED:
        {
            DEBUGP( PM, DBGLEV_INFO, "Data source %d media changed\n", (int)data);
            NotifyMediaInserted((int)data);
            break;
        }
        
#if defined(DDOMOD_EXTRAS_CDMETADATA)
        case EVENT_CD_METADATA_MULTIPLE_HITS:
        {
            cd_multiple_hit_event_t* pEvent = (cd_multiple_hit_event_t*)data;
            ClearDiskList(pEvent->svDisks);
            delete pEvent;
            break;
        }
#endif  // DDOMOD_EXTRAS_CDMETADATA
        
        default:
        {
            DEBUG( PM, DBGLEV_INFO, "Unhandled event %x\n", key );
            break;
        }
    }
    return PM_NO_ERROR;
}

// Called from the event handler when a data source's media is removed.
void CPlayManagerImp::NotifyMediaRemoved(int iDataSourceID)
{
    IPlaylistEntry* pCurrentEntry = m_pPlaylist->GetCurrentEntry();
    if (pCurrentEntry && (pCurrentEntry->GetContentRecord()->GetDataSourceID() == iDataSourceID))
    {
        m_pMediaPlayer->Deconfigure();
        pCurrentEntry = 0;
        m_playState = CMediaPlayer::STOPPED;
    }
    m_pPlaylist->DeleteEntriesFromDataSource(iDataSourceID);
    if (!pCurrentEntry && (pCurrentEntry = m_pPlaylist->GetCurrentEntry()))
    {
        if (FAILED(m_pMediaPlayer->SetSong(pCurrentEntry)))
        {
            DEBUG(PM, DBGLEV_WARNING, "Unable to set track: %s\n", pCurrentEntry->GetContentRecord()->GetURL());
            m_playState = CMediaPlayer::STOPPED;
        }
        else if (m_playState == CMediaPlayer::PLAYING)
        {
            if (FAILED(m_pMediaPlayer->Play()))
                m_playState = m_pMediaPlayer->GetPlayState();
        }
    }
    m_pContentManager->DeleteRecordsFromDataSource(iDataSourceID);
}

// Called from the event handler when a data source's media is changed.
void CPlayManagerImp::NotifyMediaInserted(int iDataSourceID)
{
    m_pDataSourceManager->RefreshContent(iDataSourceID, IDataSource::DSR_DEFAULT, DS_DEFAULT_CHUNK_SIZE);
}

