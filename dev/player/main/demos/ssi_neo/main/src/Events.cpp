// Events.cpp: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#include <core/events/SystemEvents.h>
#include <io/audio/VolumeControl.h>        // interface to volume
#include <main/ui/common/UserInterface.h>
#include <util/debug/debug.h>          // debugging hooks
#include <main/demos/ssi_neo/ui/Keys.h>
#include "Events.h"
#include <stdio.h>

//
// Debug interface
//
DEBUG_MODULE_S( EV, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( EV );

#include <codec/common/Codec.h>              // set_stream_event_data_t definition
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>  // interface to playmanager
#include <datastream/fatfile/FileInputStream.h>     // TEST
#include <datastream/fatfile/FileOutputStream.h>     // TEST
#include <playlist/plformat/manager/PlaylistFormatManager.h>   // TEST
#include <gui/peg/peg.hpp>

#ifndef NULL
#define NULL 0
#endif

CEvents::CEvents() 
{
    m_pUserInterface = NULL;
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = (IQueryableContentManager*)m_pPlayManager->GetContentManager();
    m_pVolumeControl = CVolumeControl::GetInstance();

    m_iTrackTime = 0;
    m_iArtistIndex = m_iAlbumIndex = m_iGenreIndex = 0;
    m_bRefreshArtists = m_bRefreshAlbums = m_bRefreshGenres = m_bRefreshPlaylists = false;
}

CEvents::~CEvents() 
{}

void CEvents::SetUserInterface( IUserInterface* pUI ) 
{
    m_pUserInterface = pUI;
}

void CEvents::LoadState()
{
    DEBUGP( EV, DBGLEV_INFO, "Loading settings\n");
    CFatFileInputStream file;
    file.Open("a:/settings.ddo");
    CRegistry::GetInstance()->RestoreState( &file );
    m_pVolumeControl->RestoreFromRegistry();
}

void CEvents::RefreshInterface()
{
    if (m_pUserInterface)
    {
        m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
        m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
        m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
        m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
        //SynchPlayState();
        m_pUserInterface->SetTrackTime(m_iTrackTime);
    }
}

void CEvents::SynchPlayState()
{
    if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        m_pUserInterface->NotifyPlaying();
    else if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PAUSED)
        m_pUserInterface->NotifyPaused();
    else
    {
        m_iTrackTime = 0;
        m_pUserInterface->NotifyStopped();
    }
}

// Used for sorting a ContentKeyValueVector.
static bool
CompareKeyValueRecord(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstrcmp(a.szValue, b.szValue) <= 0;
}
	
// returns -1 if event not handled
//          0 if handled and no further processing needed (override)
//          1 if handled and further processing needed
int CEvents::Event( int key, void* data ) 
{
    if( !m_pUserInterface ) return -1;

    switch( key ) {

        case EVENT_KEY_HOLD:
        {
            DEBUGP( EV, DBGLEV_FATAL, "Key hold %d\n", (unsigned int)data );
            PegThing* pt = 0;
            PegMessage Mesg;
            Mesg.wType = PM_KEY;
            Mesg.iData = (unsigned int)data;
            pt->MessageQueue()->Push(Mesg);
            break;
            /*
            unsigned int keycode = (unsigned int) data;
            switch( keycode )
            {
                case KEY_UP:
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return 0;
                case KEY_DOWN:
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return 0;
                default:
                    return -1;
            };
            break;
            */
        }
        case EVENT_KEY_PRESS:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( EV, DBGLEV_INFO, "Key press: %d\n", keycode );
            switch( keycode )
            {
            /*
                case KEY_PLAY_PAUSE:
                    if (CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING)
                    {
                        m_pPlayManager->Play();
                        m_pUserInterface->NotifyPlaying();
                    }
                    else
                    {
                        m_pPlayManager->Pause();
                        m_pUserInterface->NotifyPaused();
                    }
                    return 0;
                case KEY_STOP:
                    m_pPlayManager->Stop();
                    m_iTrackTime = 0;
                    m_pUserInterface->NotifyStopped();
                    return 0;
                case KEY_PREVIOUS:
                    // If the track time is five seconds or less, then go to the previous track.
                    if (m_iTrackTime <= 5)
                    {
                        if (SUCCEEDED(m_pPlayManager->PreviousTrack()))
                        {
                            DEBUGP( EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                        }
                    }
                    // Otherwise, seek to the beginning of this track.
                    else
                    {
                        if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
                        {
                            m_pPlayManager->Stop();
                            m_pPlayManager->Play();
                        }
                        else
                        {
                            m_pPlayManager->Stop();
                            m_pUserInterface->SetTrackTime(0);
                        }
                    }
                    return 0;
                case KEY_NEXT:
                    if (SUCCEEDED(m_pPlayManager->NextTrack()))
                    {
                        DEBUGP( EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                    }
                    return 0;
                case KEY_UP:
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return 0;
                case KEY_DOWN:
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return 0;
                    */
                case KEY_LIST_BY_ARTIST:
                    if (m_bRefreshArtists)
                    {
                        m_Artists.Clear();
                        m_pContentManager->GetArtists(m_Artists);
                        m_Artists.Sort(CompareKeyValueRecord);
                        m_bRefreshArtists = false;
                    }
                    if (m_Artists.Size())
                    {
                        m_iArtistIndex = (m_iArtistIndex + 1) % m_Artists.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Artist: %s\n", m_Artists[m_iArtistIndex].iKey, TcharToChar(szValue, m_Artists[m_iArtistIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, m_Artists[m_iArtistIndex].iKey);
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Artists[m_iArtistIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();
                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;
                case KEY_LIST_BY_ALBUM:
                    if (m_bRefreshAlbums)
                    {
                        m_Albums.Clear();
                        m_pContentManager->GetAlbums(m_Albums);
                        m_Albums.Sort(CompareKeyValueRecord);
                        m_bRefreshAlbums = false;
                    }
                    if (m_Albums.Size())
                    {
                        m_iAlbumIndex = (m_iAlbumIndex + 1) % m_Albums.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Album: %s\n", m_Albums[m_iAlbumIndex].iKey, TcharToChar(szValue, m_Albums[m_iAlbumIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, CMK_ALL, m_Albums[m_iAlbumIndex].iKey);

                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();
                        // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Albums[m_iAlbumIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();
                     
                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;
                case KEY_LIST_BY_GENRE:
                    if (m_bRefreshGenres)
                    {
                        m_Genres.Clear();
                        m_pContentManager->GetGenres(m_Genres);
                        m_Genres.Sort(CompareKeyValueRecord);
                        m_bRefreshGenres = false;
                    }
                    if (m_Genres.Size())
                    {
                        m_iGenreIndex = (m_iGenreIndex + 1) % m_Genres.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "Next artist: Key: %d Genre: %s\n", m_Genres[m_iGenreIndex].iKey, TcharToChar(szValue, m_Genres[m_iGenreIndex].szValue));

                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, CMK_ALL, CMK_ALL, m_Genres[m_iGenreIndex].iKey);

                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);
                        m_pPlayManager->Play();

                       // set the playlist info string
                        TcharToChar(m_sCurrentPlaylistName, m_Genres[m_iGenreIndex].szValue);
                        m_iCurrentPlaylistCount = mrlTracks.Size();

                        m_pUserInterface->NotifyPlaying();
                    }
                    return 0;
                    /*
                case KEY_LIST_CONTENT_TYPES:
                {
                    ContentKeyValueVector artists = m_pContentManager->GetArtists();
                    artists.Sort(CompareKeyValueRecord);
                    DEBUGP( EV, DBGLEV_INFO, "Artists:\n");
                    for (int i = 0; i < artists.Size(); ++i)
                    {
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "  Key: %d Artist: %s\n", artists[i].iKey, TcharToChar(szValue, artists[i].szValue));
                    }
                    ContentKeyValueVector albums = m_pContentManager->GetAlbums();
                    albums.Sort(CompareKeyValueRecord);
                    DEBUGP( EV, DBGLEV_INFO, "Albums:\n");
                    for (int i = 0; i < albums.Size(); ++i)
                    {
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "  Key: %d Album: %s\n", albums[i].iKey, TcharToChar(szValue, albums[i].szValue));
                    }
                    ContentKeyValueVector genres = m_pContentManager->GetGenres();
                    genres.Sort(CompareKeyValueRecord);
                    DEBUGP( EV, DBGLEV_INFO, "Genres:\n");
                    for (int i = 0; i < genres.Size(); ++i)
                    {
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP( EV, DBGLEV_INFO, "  Key: %d Genre: %s\n", genres[i].iKey, TcharToChar(szValue, genres[i].szValue));
                    }
                    DEBUGP( EV, DBGLEV_INFO, "All playlists:\n");
                    PlaylistRecordList records;
                    m_pContentManager->GetAllPlaylistRecords(records);
                    for (PlaylistRecordIterator it = records.GetHead(); it != records.GetEnd(); ++it)
                        DEBUGP( EV, DBGLEV_INFO, "  URL: %s DS: %d Format: %d\n", (*it)->GetURL(), (*it)->GetDataSourceID(), (*it)->GetPlaylistFormatID());

                    return 0;
                }
                case KEY_SAVE_PLAYLIST:
                {
                    DEBUGP( EV, DBGLEV_INFO, "Saving playlist\n");
                    CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
//                    int iPFID = pPFM->FindPlaylistFormat("m3u");
                    int iPFID = pPFM->FindPlaylistFormat("dpl");
                    if (iPFID)
                    {
                        CFatFileOutputStream outstream;
//                        outstream.Open("b:/mysave.m3u");
                        outstream.Open("b:/mysave.dpl");
                        pPFM->SavePlaylist(iPFID, &outstream, m_pPlayManager->GetPlaylist());
                    }
                    return 0;
                }
                case KEY_LOAD_PLAYLIST:
                    if (m_bRefreshPlaylists)
                    {
                        m_pContentManager->GetAllPlaylistRecords(m_Playlists);
                        m_itPlaylist = m_Playlists.GetHead();
                        m_bRefreshPlaylists = false;
                    }

                    if (!m_Playlists.IsEmpty())
                    {
                        DEBUGP( EV, DBGLEV_INFO, "Loading playlist: URL: %s DS: %d Format: %d\n", (*m_itPlaylist)->GetURL(), (*m_itPlaylist)->GetDataSourceID(), (*m_itPlaylist)->GetPlaylistFormatID());
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        CPlaylistFormatManager::GetInstance()->LoadPlaylist(*m_itPlaylist, m_pPlayManager->GetPlaylist(), true);
                        m_pPlayManager->Play();
                        m_pUserInterface->NotifyPlaying();
                        if (++m_itPlaylist == m_Playlists.GetEnd())
                            m_itPlaylist = m_Playlists.GetHead();
                    }
                    else
                    {
                        DEBUGP( EV, DBGLEV_INFO, "No playlist to load\n");
                    }
                    return 0;
                case KEY_PLAYLIST_MODE:
                    m_pPlayManager->SetPlaylistMode((IPlaylist::PlaylistMode)((m_pPlayManager->GetPlaylistMode() + 1) % 5));
                    m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
                    return 0;

                case KEY_REFRESH_CONTENT:
                    DEBUGP( EV, DBGLEV_INFO, "Refreshing playlist\n");
                    m_pPlayManager->RefreshAllContent( );   // Two pass
//                    m_pPlayManager->RefreshAllContent( true, false );    // One pass, metadata
//                    m_pPlayManager->RefreshAllContent( false, false );    // One pass, no metadata

                    return 0;
                case KEY_SAVE_STATE:
                {
                    DEBUGP( EV, DBGLEV_INFO, "Saving list\n");
                    CFatFileOutputStream FFOS;
                    FFOS.Open("a:/testfile1.dat");
                    m_pContentManager->SaveStateToStream(&FFOS);
                    FFOS.Close();
                    
                    DEBUGP( EV, DBGLEV_INFO, "Saving settings\n");
                    m_pVolumeControl->SaveToRegistry();
                    CFatFileOutputStream file;
                    file.Open("a:/settings.ddo");
                    CRegistry::GetInstance()->SaveState( &file );
                    return 0;
                }
                case KEY_LOAD_STATE:
                    DEBUGP( EV, DBGLEV_INFO, "Loading list\n");
                    CFatFileInputStream FFIS;
                    FFIS.Open("a:/testfile1.dat");
                    m_pContentManager->LoadStateFromStream(&FFIS);
                    FFIS.Close();
                    
                    MediaRecordList records;
                    m_pContentManager->GetAllMediaRecords(records);
                    m_pPlayManager->GetPlaylist()->AddEntries(records);

                    DEBUGP( EV, DBGLEV_INFO, "Loading settings\n");
                    CFatFileInputStream file;
                    file.Open("a:/settings.ddo");
                    CRegistry::GetInstance()->RestoreState( &file );
                    m_pVolumeControl->RestoreFromRegistry();

                    m_bRefreshArtists = true;
                    m_bRefreshAlbums = true;
                    m_bRefreshGenres = true;
                    m_bRefreshPlaylists = true;
                    
                    return 0;
            */
                default:
                    PegThing* pt = 0;
                    PegMessage Mesg;
                    Mesg.wType = PM_KEY;
                    Mesg.iData = (unsigned int)data;
                    pt->MessageQueue()->Push(Mesg);
                    break;
            }
            break;
        }

        case EVENT_KEY_RELEASE:
        {
            DEBUGP( EV, DBGLEV_FATAL, "Key release %d\n", (unsigned int)data );
            PegThing* pt = 0;
            PegMessage Mesg;
            Mesg.wType = PM_KEY_RELEASE;
            Mesg.iData = (unsigned int) data;
            pt->MessageQueue()->Push(Mesg);
            break;
        }

        case EVENT_STREAM_SET:
        {
            set_stream_event_data_t* pSSED = (set_stream_event_data_t*)data;
            m_pUserInterface->SetTrack(pSSED);

            sprintf(m_sCurrentPlaylistString,"%d/%d : %s",pSSED->pPlaylistEntry->GetIndex()+1,m_iCurrentPlaylistCount,m_sCurrentPlaylistName);
            m_pUserInterface->SetMessage(m_sCurrentPlaylistString);

            m_pPlayManager->HandleEvent(key, data);
            return 1;
        }
        case EVENT_STREAM_PROGRESS:
            m_pUserInterface->SetTrackTime( (int) data );
            return 1;

        case EVENT_STREAM_END:
        case EVENT_STREAM_FAIL:
            m_pPlayManager->HandleEvent(key, data);
            SynchPlayState();
            return 1;
        case EVENT_MEDIA_REMOVED:
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaRemoved((int)data);
            return 1;
        case EVENT_MEDIA_INSERTED:
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaInserted((int)data);
            return 1;
        case EVENT_CONTENT_UPDATE_BEGIN:
            //m_pUserInterface->SetMessage("Starting content update on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return 1;

        case EVENT_CONTENT_UPDATE_END:
            //m_pUserInterface->SetMessage("Content update finished on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return 1;

        case EVENT_CONTENT_UPDATE_ERROR:
            //m_pUserInterface->SetMessage("Error updating content on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return 1;

        case EVENT_CONTENT_UPDATE:
            DEBUGP( EV, DBGLEV_INFO, "Received content update\n");
            m_bRefreshArtists = true;
            m_bRefreshAlbums = true;
            m_bRefreshGenres = true;
            m_bRefreshPlaylists = true;
            break;

        default:
            m_pPlayManager->HandleEvent(key, data);
            return -1;
    };

    m_pPlayManager->HandleEvent(key, data);
    return -1;
}
