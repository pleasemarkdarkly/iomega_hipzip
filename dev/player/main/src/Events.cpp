// Events.cpp: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#include <_modules.h>

#include <stdio.h>    // sprintf

#include <core/events/SystemEvents.h>
#include <io/audio/VolumeControl.h>        // interface to volume
#include <main/ui/common/UserInterface.h>
#include <util/debug/debug.h>              // debugging hooks
#include <util/tchar/tchar.h>              // for CreatePlaystream

#include "Events.h"

#include <codec/common/Codec.h>              // set_stream_event_data_t definition
#include <core/playmanager/PlayManager.h>    // interface to playmanager
#include <core/mediaplayer/MediaPlayer.h>    // for SetBufferMultiplier

//
// These are used for loading/saving state to file
//
#include <datasource/datasourcemanager/DataSourceManager.h>   // TEST state retention
#include <datastream/fatfile/FileInputStream.h>               // TEST state retention
#include <datastream/fatfile/FileOutputStream.h>              // TEST state retention
#include <playlist/plformat/manager/PlaylistFormatManager.h>  // TEST state retention

//
// Keys for the playstream_settings_t structure
//
#include <datastream/waveout/WaveOutKeys.h>

//
// We handle CD tracks specially, so we need to ID the cd data source
//
#include <datasource/cddatasource/CDDataSource.h>

// TODO test
#ifdef DDOMOD_DATASTREAM_FATFILE
#include <datastream/fatfile/FileOutputStream.h>
#endif

// TODO test
#ifdef DDOMOD_DATASTREAM_PEMPIPE
//#include <datastream/pempipe/PEMPipeKeys.h>
#undef DDOMOD_DATASTREAM_PEMPIPE
#endif

#ifdef DDOMOD_EXTRAS_IDLECODER
#include <extras/idlecoder/IdleCoder.h>
#endif

// TODO: content manager switching not fully supported
#ifdef DDOMOD_CONTENT_METAKITCONTENTMANAGER
#include <content/metakitcontentmanager/MetakitContentManager.h>
#else
#include <content/common/ContentManager.h>
#endif


//
// Configuration
//

// If CPU usage tracking is available, enable this to
// print cpu usage after each button press
#ifdef CYGFUN_KERNEL_THREADS_CPU_USAGE

#define DUMP_THREAD_DATA
#include <util/diag/diag.h>
#endif   // CYGFUN_KERNEL_THREADS_CPU_USAGE

// Uncomment these two lines to print mem usage statistics
#include <util/diag/diag.h>
#define DUMP_MEM_USAGE


#ifndef NULL
#define NULL 0
#endif

#define PLAYLIST_STRING_SIZE 128

//
// Debug interface
//
DEBUG_MODULE_S( EV, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( EV );

//
// Playstream settings - we hold this here so we can
//  adjust them based on various events
//
static char szConfigurableStreamName[256];
static const unsigned int sDefaultOutputList[] =  { WAVEOUT_KEY, 0,    };
static unsigned int sConfigurableOutputList[3] =  { WAVEOUT_KEY, 0, 0, };
static const unsigned int sDefaultFilterList[] =  { 0 };
static unsigned int sConfigurableFilterList[3] =  { 0, 0, 0, };
    

static const playstream_settings_t PlaybackPS = 
{
    szStreamName:                 0,   // name of stream 
    OutputList:  sDefaultOutputList,   // output list
    FilterList:  sDefaultFilterList,   // filter list
};
static playstream_settings_t EncodePS =
{
    szStreamName: 0,
    OutputList:   0,
    FilterList:   0,
};
static const playstream_settings_t* CreatePlaystream( IMediaContentRecord* );

CEvents::CEvents() 
{
    m_pUserInterface = NULL;

    //
    // Initialize the playmanager
    //
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = (ContentManagerType*)m_pPlayManager->GetContentManager();

    // Set up the default (playback) playstream
    m_pPlayManager->SetPlaystream( &PlaybackPS, &CreatePlaystream );

    
    //
    // Initialize volume control
    //
    m_pVolumeControl = CVolumeControl::GetInstance();

    // Force a custom volume map
    static int s_aryVolume[21] = {
        -96,
        -42, -39, -36,  -33, -30,
        -27, -24, -21,  -18, -15,
        -13, -11,  -9,   -7,  -5,
        -3,  -1,   1,    3,	 5 };

    m_pVolumeControl->SetVolumeRange(21, s_aryVolume);
    m_pVolumeControl->SetVolume(15);


    //
    // Use software sample rate conversion to get to 24khz
    //
    if( FAILED((CMediaPlayer::GetInstance())->SetSRCBlending( 24000 )) ) {
        DEBUG( EV, DBGLEV_ERROR, "Failed to set SRC blending\n");
    }
    
    //
    // Null out some members
    //
    m_szSettingsURL = m_szContentStateURL = m_szPlaylistURL = 0;
}

CEvents::~CEvents() 
{}

void CEvents::SetUserInterface( IUserInterface* pUI ) 
{
    m_pUserInterface = pUI;

}
void CEvents::RefreshInterface()
{
    if (m_pUserInterface)
    {
        m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
        m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
        m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
        m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
        SynchPlayState();
        m_pUserInterface->SetTrackTime(m_iTrackTime);
    }
}

// Match the play state icon to the play manager's play state.
void CEvents::SynchPlayState()
{
    if (m_pPlayManager->GetPlayState() == CMediaPlayer::PLAYING)
        m_pUserInterface->NotifyPlaying();
    else if (m_pPlayManager->GetPlayState() == CMediaPlayer::PAUSED)
        m_pUserInterface->NotifyPaused();
    else
    {
        m_iTrackTime = 0;
        m_pUserInterface->NotifyStopped();
    }
}

static bool
CompareKeyValueRecord(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstrcmp(a.szValue, b.szValue) <= 0;
}

static const playstream_settings_t*
CreatePlaystream( IMediaContentRecord* pContentRecord ) 
{
    // This gets called from SetSong prior to setting the specified track
    int DSID = pContentRecord->GetDataSourceID();
    IDataSource* pDS = (CDataSourceManager::GetInstance())->GetDataSourceByID( DSID );

    // See if this track is on the CD
    if( pDS->GetClassID() == CD_DATA_SOURCE_CLASS_ID ) {
        // TODO: properly handle data CDs

        // See if we have artist and track name available
        char* pExt;
        void* pArtist, *pTitle;
        char szArtist[65];
        char szTitle[65];

#ifdef DDOMOD_DATASTREAM_PEMPIPE
        pExt = "mp3";
        (CMediaPlayer::GetInstance())->SetBufferMultiplier(48);
#else
        pExt = "raw";
#endif

        // it's safe to cast up since we have checked the class id
        CCDDataSource* pCDDS = (CCDDataSource*) pDS;
        int iTrackIndex = pCDDS->GetAudioTrackIndex( pContentRecord );
        
        if( SUCCEEDED( pContentRecord->GetAttribute(MDA_ARTIST, &pArtist) ) &&
            SUCCEEDED( pContentRecord->GetAttribute(MDA_TITLE,  &pTitle ) ) ) {

            TcharToCharN( szArtist, (TCHAR*)pArtist, 64 );
            TcharToCharN( szTitle,  (TCHAR*)pTitle , 64 );
            sprintf( szConfigurableStreamName, "A:\\%s - %d - %s.%s", szArtist, iTrackIndex, szTitle, pExt );
        }
        else {
            // No artist and track name, so use CDID
            int iDiscID = pCDDS->GetDiscID();
            
            sprintf( szConfigurableStreamName, "A:\\Track %d.%s", iTrackIndex, pExt );
        }

        // Pick a playstream for either encoding or raw ripping
#ifdef DDOMOD_DATASTREAM_PEMPIPE
        sConfigurableOutputList[0] = WAVEOUT_KEY;
        sConfigurableOutputList[1] = PEMPIPE_KEY;
        sConfigurableOutputList[2] = 0;
#else
        sConfigurableOutputList[0] = WAVEOUT_KEY;
        sConfigurableOutputList[1] = FATFILE_OUTPUT_ID;
        sConfigurableOutputList[2] = 0;
#endif
        
        EncodePS.szStreamName = szConfigurableStreamName;
        EncodePS.OutputList   = sConfigurableOutputList;
        return &EncodePS;
    }
    // This is an artifact of the increased buffer multiplier being used above :/
    (CMediaPlayer::GetInstance())->SetBufferMultiplier(2);
    return NULL;
}

void CEvents::Event( int key, void* data ) 
{
    static const char* s_szProgressChars = "/-\\|";
    static int s_iProgressCharCount = 4;
    static int s_iContentProgress = 0;
    static int s_iMetadataProgress = 0;
    
    if( !m_pUserInterface ) return ;

    switch( key ) {

        case EVENT_KEY_HOLD:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUG( EV, DBGLEV_INFO, "Key hold: %d\n", keycode );
            switch( keycode )
            {
                case KEY_VOLUME_UP:
                {
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;
                }
                case KEY_VOLUME_DOWN:
                {
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;
                }
                case KEY_TREBLE_UP:
                {
                    m_pVolumeControl->TrebleUp();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
                    return;
                }
                case KEY_TREBLE_DOWN:
                {
                    m_pVolumeControl->TrebleDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetTreble());
                    return;
                }
                case KEY_BASS_UP:
                {
                    m_pVolumeControl->BassUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetBass());
                    return;
                }
                case KEY_BASS_DOWN:
                {
                    m_pVolumeControl->BassDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetBass());
                    return;
                }
                default:
                {
                    return;
                }
            }
            break;
        }
        
        case EVENT_KEY_PRESS:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP( EV, DBGLEV_INFO, "Key press: %d\n", keycode );
#ifdef DUMP_THREAD_DATA
            print_thread_cpu_usage( 1 );
#endif
#ifdef DUMP_MEM_USAGE
            print_mem_usage();
#endif
            switch( keycode )
            {
                case KEY_PLAY_PAUSE:
                {
                    if (m_pPlayManager->GetPlayState() != CMediaPlayer::PLAYING)
                    {
                        if (SUCCEEDED(m_pPlayManager->Play()))
                            m_pUserInterface->NotifyPlaying();
                        else
                            SynchPlayState();
                    }
                    else
                    {
                        if (SUCCEEDED(m_pPlayManager->Pause()))
                            m_pUserInterface->NotifyPaused();
                        else
                            SynchPlayState();
                    }
                    return;
                }
                case KEY_STOP:
                {
                    
                    if (SUCCEEDED(m_pPlayManager->Stop()))
                    {
                        m_iTrackTime = 0;
                        m_pUserInterface->NotifyStopped();
                    }
                    else
                        SynchPlayState();
                    return;
                }
                case KEY_PREVIOUS:
                {
                    // If the track time is five seconds or less, then go to the previous track.
                    if (m_iTrackTime <= 5)
                    {
                        if (SUCCEEDED(m_pPlayManager->PreviousTrack())) {
                            DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n",
                                   m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1,
                                   m_pPlayManager->GetPlaylist()->GetSize());
                        } else {
                            SynchPlayState();
                        }
                    }
                    // Otherwise, seek to the beginning of this track.
                    else
                    {
                        if (SUCCEEDED(m_pPlayManager->Seek(0))) {
                            m_pUserInterface->SetTrackTime(0);
                        } else {
                            SynchPlayState();
                        }
                    }
                    return;
                }
                case KEY_NEXT:
                {
                    if (SUCCEEDED(m_pPlayManager->NextTrack())) {
                        DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n",
                               m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1,
                               m_pPlayManager->GetPlaylist()->GetSize());
                    } else {
                        SynchPlayState();
                    }
                    return;
                }
                case KEY_VOLUME_UP:
                {
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;
                }
                case KEY_VOLUME_DOWN:
                {
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;
                }
                case KEY_TREBLE_UP:
                {
                    m_pVolumeControl->TrebleUp();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
                    return;
                }
                case KEY_TREBLE_DOWN:
                {
                    m_pVolumeControl->TrebleDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetTreble());
                    return;
                }
                case KEY_BASS_UP:
                {
                    m_pVolumeControl->BassUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetBass());
                    return;
                }
                case KEY_BASS_DOWN:
                {
                    m_pVolumeControl->BassDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetBass());
                    return;
                }
                case KEY_LIST_BY_ARTIST:
                    if (m_bRefreshArtists)
                    {
                        // The content manager's database has been updated, so rebuild the list of artists.
                        m_Artists.Clear();
                        m_pContentManager->GetArtists(m_Artists);
                        m_Artists.Sort(CompareKeyValueRecord);
                        m_bRefreshArtists = false;
                    }
                    if (m_Artists.Size())
                    {
                        m_iArtistIndex = (m_iArtistIndex + 1) % m_Artists.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP(EV, DBGLEV_INFO, "Next artist: Key: %d Artist: %s\n", m_Artists[m_iArtistIndex].iKey, TcharToChar(szValue, m_Artists[m_iArtistIndex].szValue));
                        m_pUserInterface->SetMessage("Created playlist of artist %s", szValue);

                        // Get a list of media records by the current artist in the list.
                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, m_Artists[m_iArtistIndex].iKey);

                        // Create a new playlist of that content and start it playing.
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);

                        if (SUCCEEDED(m_pPlayManager->Play()))
                            m_pUserInterface->NotifyPlaying();
                        else
                            SynchPlayState();
                    }
                    else
                        m_pUserInterface->SetMessage("No content found");
                    return;

                case KEY_LIST_BY_ALBUM:
                    if (m_bRefreshAlbums)
                    {
                        // The content manager's database has been updated, so rebuild the list of albums.
                        m_Albums.Clear();
                        m_pContentManager->GetAlbums(m_Albums);
                        m_Albums.Sort(CompareKeyValueRecord);
                        m_bRefreshAlbums = false;
                    }
                    if (m_Albums.Size())
                    {
                        m_iAlbumIndex = (m_iAlbumIndex + 1) % m_Albums.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP(EV, DBGLEV_INFO, "Next artist: Key: %d Album: %s\n", m_Albums[m_iAlbumIndex].iKey, TcharToChar(szValue, m_Albums[m_iAlbumIndex].szValue));
                        m_pUserInterface->SetMessage("Created playlist of album %s", szValue);

                        // Get a list of media records from the current album in the list.
                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, CMK_ALL, m_Albums[m_iAlbumIndex].iKey);

                        // Create a new playlist of that content and start it playing.
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);

                        if (SUCCEEDED(m_pPlayManager->Play()))
                            m_pUserInterface->NotifyPlaying();
                        else
                            SynchPlayState();
                    }
                    else
                        m_pUserInterface->SetMessage("No content found");
                    return;

                case KEY_LIST_BY_GENRE:
                    if (m_bRefreshGenres)
                    {
                        // The content manager's database has been updated, so rebuild the list of genres.
                        m_Genres.Clear();
                        m_pContentManager->GetGenres(m_Genres);
                        m_Genres.Sort(CompareKeyValueRecord);
                        m_bRefreshGenres = false;
                    }
                    if (m_Genres.Size())
                    {
                        m_iGenreIndex = (m_iGenreIndex + 1) % m_Genres.Size();
                        char szValue[PLAYLIST_STRING_SIZE];
                        DEBUGP(EV, DBGLEV_INFO, "Next artist: Key: %d Genre: %s\n", m_Genres[m_iGenreIndex].iKey, TcharToChar(szValue, m_Genres[m_iGenreIndex].szValue));
                        m_pUserInterface->SetMessage("Created playlist of genre %s", szValue);

                        // Get a list of media records in the current genre in the list.
                        MediaRecordList mrlTracks;
                        m_pContentManager->GetMediaRecords(mrlTracks, CMK_ALL, CMK_ALL, m_Genres[m_iGenreIndex].iKey);

                        // Create a new playlist of that content and start it playing.
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();
                        m_pPlayManager->GetPlaylist()->AddEntries(mrlTracks);

                        if (SUCCEEDED(m_pPlayManager->Play()))
                            m_pUserInterface->NotifyPlaying();
                        else
                            SynchPlayState();
                    }
                    else
                        m_pUserInterface->SetMessage("No content found");
                    return;

                case KEY_LIST_BY_PLAYLIST:
                {
                    if (m_bRefreshPlaylists)
                    {
                        // The content manager's database has been updated, so rebuild the list of playlists.
                        m_pContentManager->GetAllPlaylistRecords(m_Playlists);
                        m_itPlaylist = m_Playlists.GetHead();
                        m_bRefreshPlaylists = false;
                    }

                    if (!m_Playlists.IsEmpty())
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Loading playlist: URL: %s DS: %d Format: %d\n",
                               (*m_itPlaylist)->GetURL(),
                               (*m_itPlaylist)->GetDataSourceID(),
                               (*m_itPlaylist)->GetPlaylistFormatID());
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();

                        // Use the playlist format ID and URL from the playlist content record to find the
                        // appropriate function to load.
                        CPlaylistFormatManager::GetInstance()->LoadPlaylist((*m_itPlaylist)->GetPlaylistFormatID(),
                                                                            (*m_itPlaylist)->GetURL(),
                                                                            m_pPlayManager->GetPlaylist(),
                                                                            true);
                        if (!m_pPlayManager->GetPlaylist()->IsEmpty())
                        {
                            m_pUserInterface->SetMessage("Loaded playlist %s", (*m_itPlaylist)->GetURL());

                            if (SUCCEEDED(m_pPlayManager->Play()))
                                m_pUserInterface->NotifyPlaying();
                            else
                                SynchPlayState();
                        }
                        else
                            m_pUserInterface->SetMessage("Empty/invalid playlist: %s", (*m_itPlaylist)->GetURL());
                        if (++m_itPlaylist == m_Playlists.GetEnd())
                            m_itPlaylist = m_Playlists.GetHead();
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "No playlist to load\n");
                    }
                    return;
                }

                case KEY_SAVE_PLAYLIST:
                {
                    // Save out the current playlist to a file.
                    if (m_szPlaylistURL)
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Saving playlist\n");
                        m_pUserInterface->SetMessage("Saving playlist");
                        CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
                        // Use the file extension in the playlist URL to find the correct playlist format.
                        int iPFID = pPFM->FindPlaylistFormat(m_szPlaylistURL + strlen(m_szPlaylistURL) - 3);
                        if (iPFID)
                        {
                            if (SUCCEEDED(pPFM->SavePlaylist(iPFID, m_szPlaylistURL, m_pPlayManager->GetPlaylist())))
                            {
                                m_pUserInterface->SetMessage("Saved playlist");
                                return;
                            }
                        }
                        m_pUserInterface->SetMessage("Error saving playlist");
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "No playlist URL specified\n");
                        m_pUserInterface->SetMessage("No playlist URL specified");
                    }

                    return;
                }

                case KEY_LOAD_PLAYLIST:
                {
                    // Load a playlist file and set it as the current playlist.
                    if (m_szPlaylistURL)
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Loading playlist\n");
                        m_pUserInterface->SetMessage("Loading playlist");
                        CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
                        // Use the file extension in the playlist URL to find the correct playlist format.
                        int iPFID = pPFM->FindPlaylistFormat(m_szPlaylistURL + strlen(m_szPlaylistURL) - 3);
                        if (iPFID)
                        {
                            m_pPlayManager->GetPlaylist()->Clear();
                            if (SUCCEEDED(pPFM->LoadPlaylist(iPFID, m_szPlaylistURL, m_pPlayManager->GetPlaylist(), false)))
                            {
                                m_pUserInterface->SetMessage("Loaded playlist");
                                if (SUCCEEDED(m_pPlayManager->Play()))
                                    m_pUserInterface->NotifyPlaying();
                                else
                                    SynchPlayState();
                                return;
                            }
                        }
                        m_pUserInterface->SetMessage("Error loading playlist");
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "No playlist URL specified\n");
                        m_pUserInterface->SetMessage("No playlist URL specified");
                    }

                    return;
                }

                case KEY_PLAYLIST_MODE:
                    m_pPlayManager->SetPlaylistMode((IPlaylist::PlaylistMode)((m_pPlayManager->GetPlaylistMode() + 1) % 5));
                    m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
                    return;

                case KEY_REFRESH_CONTENT:
                    DEBUGP(EV, DBGLEV_INFO, "Refreshing content\n");
                    m_pPlayManager->RefreshAllContent(IDataSource::DSR_DEFAULT, DS_DEFAULT_CHUNK_SIZE);
                    return;

                case KEY_SAVE_STATE:
                {
                    DEBUGP(EV, DBGLEV_INFO, "Saving list\n");
                    m_pUserInterface->SetMessage("Saving state");
                    IOutputStream* pOS;
                    if (m_szContentStateURL && (pOS = CDataSourceManager::GetInstance()->OpenOutputStream(m_szContentStateURL)))
                    {
                        m_pContentManager->SaveStateToStream(pOS);
                        delete pOS;
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Error saving content state\n");
                        m_pUserInterface->SetMessage("Error opening content state file");
                    }

                    DEBUGP(EV, DBGLEV_INFO, "Saving settings\n");
                    m_pVolumeControl->SaveToRegistry();
                    if (m_szSettingsURL && (pOS = CDataSourceManager::GetInstance()->OpenOutputStream(m_szSettingsURL)))
                    {
                        CRegistry::GetInstance()->SaveState(pOS);
                        delete pOS;
                        m_pUserInterface->SetMessage("Saved state");
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Error saving settings\n");
                        m_pUserInterface->SetMessage("Error opening settings file");
                    }
                    return;
                }

                case KEY_LOAD_STATE:
                {
                    DEBUGP(EV, DBGLEV_INFO, "Loading content state\n");
                    m_pUserInterface->SetMessage("Loading content state");
                    IInputStream* pIS;
                    if (m_szContentStateURL && (pIS = CDataSourceManager::GetInstance()->OpenInputStream(m_szContentStateURL)))
                    {
                        m_pContentManager->LoadStateFromStream(pIS);
                        delete pIS;
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Error loading content state\n");
                        m_pUserInterface->SetMessage("Error opening content state file");
                    }

                    MediaRecordList records;
                    m_pContentManager->GetAllMediaRecords(records);
                    m_pPlayManager->GetPlaylist()->AddEntries(records);

                    DEBUGP(EV, DBGLEV_INFO, "Loading settings\n");
                    if (m_szSettingsURL && (pIS = CDataSourceManager::GetInstance()->OpenInputStream(m_szSettingsURL)))
                    {
                        CRegistry::GetInstance()->RestoreState(pIS);
                        m_pVolumeControl->RestoreFromRegistry();
                        RefreshInterface();
                        delete pIS;
                        m_pUserInterface->SetMessage("Loaded state");
                    }
                    else
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Error loading settings\n");
                        m_pUserInterface->SetMessage("Error opening settings file");
                    }

                    // The content manager's database has changed, so remember to update
                    // the lists of artists, albums, genres, and playlists.
                    m_bRefreshArtists = true;
                    m_bRefreshAlbums = true;
                    m_bRefreshGenres = true;
                    m_bRefreshPlaylists = true;

                    return;
                }
            }
            break;
        }
        case EVENT_STREAM_SET:
        {
            set_stream_event_data_t* pSSED = (set_stream_event_data_t*)data;
            m_pUserInterface->SetTrack(pSSED, m_pPlayManager->GetPlaylist()->GetSize());
            m_pPlayManager->HandleEvent(key, data);
            return;
        }

        case EVENT_STREAM_PROGRESS:
            diag_printf("track time %d\n", (int)data);
            m_pUserInterface->SetTrackTime((int)data);
            return;

        case EVENT_STREAM_END:
        case EVENT_STREAM_FAIL:
        {
            ERESULT res = m_pPlayManager->HandleEvent(key, data);
            if (res == PM_PLAYLIST_END)
            {
                // The end of the playlist was reached, so cycle back to the first track.
                if (IPlaylistEntry* pEntry = m_pPlayManager->GetPlaylist()->GetEntry(0, m_pPlayManager->GetPlaylistMode()))
                {
                    m_pPlayManager->GetPlaylist()->SetCurrentEntry(pEntry);
                    if (FAILED(CMediaPlayer::GetInstance()->SetSong(pEntry)))
                    {
                        // The first track was bad, so try to find a good track.
                        if (FAILED(m_pPlayManager->NextTrack()))
                        {
                            // No good tracks were found, so clear the UI.
                            m_pUserInterface->ClearTrack();
                            return;
                        }
                    }
                    // A track was set successfully, so show the player in the stopped state.
                    m_iTrackTime = 0;
                    m_pUserInterface->NotifyStopped();
                    return;
                }
            }
            else if (res != PM_PLAYING)
            {
                // The next track was successfully set, but playback didn't continue for some reason.
                SynchPlayState();
                return;
            }
            return;
        }

        case EVENT_MEDIA_REMOVED:
            // The content manager's database has changed, so remember to update
            // the lists of artists, albums, genres, and playlists.
            m_bRefreshArtists = true;
            m_bRefreshAlbums = true;
            m_bRefreshGenres = true;
            m_bRefreshPlaylists = true;
            // The system event handler will remove all entries belonging to this data source
            // from the content manager.
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaRemoved((int)data);
            // If the playlist is now empty, then clear the interface.
            if (m_pPlayManager->GetPlaylist()->IsEmpty())
            {
                m_pUserInterface->ClearTrack();
            }
            SynchPlayState();
            return;

        case EVENT_MEDIA_INSERTED:
            // The content manager's database has changed, so remember to update
            // the lists of artists, albums, genres, and playlists.
            m_bRefreshArtists = true;
            m_bRefreshAlbums = true;
            m_bRefreshGenres = true;
            m_bRefreshPlaylists = true;
            // The system event handler will kick off a content refresh using the default
            // refresh mode and update chunk size of the data source.
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaInserted((int)data);
            return;

        case EVENT_CONTENT_UPDATE_BEGIN:
            m_pUserInterface->SetMessage("Starting content update on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return;

        case EVENT_CONTENT_UPDATE_END:
            m_pUserInterface->SetMessage("Content update finished on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return;

        case EVENT_CONTENT_UPDATE_ERROR:
            m_pUserInterface->SetMessage("Error updating content on data source %d", (int)data);
            m_pPlayManager->HandleEvent(key, data);
            return;

        case EVENT_CONTENT_UPDATE:
            DEBUGP(EV, DBGLEV_INFO, "Received content update from data source %d\n", ((content_record_update_t*)data)->iDataSourceID);
            m_pUserInterface->SetMessage("Scanning content on data source %d %c", ((content_record_update_t*)data)->iDataSourceID, s_szProgressChars[++s_iContentProgress % s_iProgressCharCount]);
            // The content manager's database has changed, so remember to update
            // the lists of artists, albums, genres, and playlists.
            m_bRefreshArtists = true;
            m_bRefreshAlbums = true;
            m_bRefreshGenres = true;
            m_bRefreshPlaylists = true;
            // The default event hanlder will add these entries to the content manager.
            break;

        case EVENT_CONTENT_METADATA_UPDATE:
            DEBUGP(EV, DBGLEV_INFO, "Received metadata from data source %d\n", ((content_record_update_t*)data)->iDataSourceID);
            m_pUserInterface->SetMessage("Scanning metadata on data source %d %c", ((content_record_update_t*)data)->iDataSourceID, s_szProgressChars[++s_iContentProgress % s_iProgressCharCount]);
            // The content manager's database has changed, so remember to update
            // the lists of artists, albums, genres, and playlists.
            m_bRefreshArtists = true;
            m_bRefreshAlbums = true;
            m_bRefreshGenres = true;
            m_bRefreshPlaylists = true;
            // The default event hanlder will update the metadata for these entries in the content manager.
            break;

        case EVENT_CONTENT_METADATA_UPDATE_END:
            DEBUGP(EV, DBGLEV_INFO, "Metadata update end message from data source %d\n", (int)data);
            m_pUserInterface->SetMessage("Metadata update finished on data source %d", (int)data);
            break;

        default:
            break;
    };

    m_pPlayManager->HandleEvent(key, data);
}

