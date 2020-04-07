//
// Events.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "Events.h"
#include <content/metakitcontentmanager/MetakitContentManager.h>
#include <core/events/SystemEvents.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <io/audio/VolumeControl.h>
#include <main/ui/common/UserInterface.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/debug/debug.h>
#include <stdlib.h>

//
// Debug interface
//
DEBUG_MODULE_S(EV, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(EV);

CEvents::CEvents() 
{
    m_pUserInterface = 0;
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = (CMetakitContentManager*)m_pPlayManager->GetContentManager();
    m_pVolumeControl = CVolumeControl::GetInstance();

    m_iTrackTime = 0;

    m_iArtistIndex = m_iAlbumIndex = m_iGenreIndex = 0;
    m_bRefreshArtists = m_bRefreshAlbums = m_bRefreshGenres = m_bRefreshPlaylists = false;

    m_szSettingsURL = m_szContentStateURL = m_szPlaylistURL = 0;

static int s_aryVolume[21] = {
  -96,
  -42, -39, -36,  -33, -30,
  -27, -24, -21,  -18, -15,
  -13, -11,  -9,   -7,  -5,
   -3,  -1,   1,    3,	 5 };

    m_pVolumeControl->SetVolumeRange(21, s_aryVolume);
    m_pVolumeControl->SetVolume(15);
}

CEvents::~CEvents() 
{
    free(m_szSettingsURL);
    free(m_szContentStateURL);
    free(m_szPlaylistURL);
}

// Set the URL of the stream that will be used for loading and saving volume, bass, and treble levels.
void CEvents::SetSettingsURL(const char* szURL)
{
    free(m_szSettingsURL);
    m_szSettingsURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szSettingsURL, szURL);
}

// Set the URL of the stream that will be used for loading and saving the content manager database.
void CEvents::SetContentStateURL(const char* szURL)
{
    free(m_szContentStateURL);
    m_szContentStateURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szContentStateURL, szURL);
}

// Set the URL of the stream that will be used for loading and saving the current playlist.
void CEvents::SetPlaylistURL(const char* szURL)
{
    free(m_szPlaylistURL);
    m_szPlaylistURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szPlaylistURL, szURL);
}


void CEvents::SetUserInterface(IUserInterface* pUI) 
{
    m_pUserInterface = pUI;
}

// Redraw the interface with the current settings.
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

// Used for sorting a ContentKeyValueVector.
static bool
CompareKeyValueRecord(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstrcmp(a.szValue, b.szValue) <= 0;
}

#define PLAYLIST_STRING_SIZE	128

// Prints a list of artists, albums, genres, and playlists in the content manager.
void CEvents::PrintCategories()
{
    ContentKeyValueVector artists;
    m_pContentManager->GetArtists(artists);
    artists.Sort(CompareKeyValueRecord);
    DEBUGP(EV, DBGLEV_INFO, "Artists:\n");
    for (int i = 0; i < artists.Size(); ++i)
    {
        char szValue[PLAYLIST_STRING_SIZE];
        DEBUGP(EV, DBGLEV_INFO, "  Key: %d Artist: %s\n", artists[i].iKey, TcharToChar(szValue, artists[i].szValue));
    }
    ContentKeyValueVector albums;
    m_pContentManager->GetAlbums(albums);
    albums.Sort(CompareKeyValueRecord);
    DEBUGP(EV, DBGLEV_INFO, "Albums:\n");
    for (int i = 0; i < albums.Size(); ++i)
    {
        char szValue[PLAYLIST_STRING_SIZE];
        DEBUGP(EV, DBGLEV_INFO, "  Key: %d Album: %s\n", albums[i].iKey, TcharToChar(szValue, albums[i].szValue));
    }
    ContentKeyValueVector genres;
    m_pContentManager->GetGenres(genres);
    genres.Sort(CompareKeyValueRecord);
    DEBUGP(EV, DBGLEV_INFO, "Genres:\n");
    for (int i = 0; i < genres.Size(); ++i)
    {
        char szValue[PLAYLIST_STRING_SIZE];
        DEBUGP(EV, DBGLEV_INFO, "  Key: %d Genre: %s\n", genres[i].iKey, TcharToChar(szValue, genres[i].szValue));
    }
    DEBUGP(EV, DBGLEV_INFO, "All playlists:\n");
    PlaylistRecordList records;
    m_pContentManager->GetAllPlaylistRecords(records);
    for (PlaylistRecordIterator it = records.GetHead(); it != records.GetEnd(); ++it)
        DEBUGP(EV, DBGLEV_INFO, "  URL: %s DS: %d Format: %d\n", (*it)->GetURL(), (*it)->GetDataSourceID(), (*it)->GetPlaylistFormatID());
}


// Prints the contents of the current playlist.
void CEvents::PrintPlaylist()
{
    DEBUGP(EV, DBGLEV_INFO, "\nCurrent playlist:\n");
    if (IPlaylist* pPlaylist = m_pPlayManager->GetPlaylist())
    {
        for (int i = 0; i < pPlaylist->GetSize(); ++i)
            if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(i))
                if (IMediaContentRecord* pRecord = pEntry->GetContentRecord())
                {
                    TCHAR* tszValue;
                    DEBUGP(EV, DBGLEV_INFO, "  Entry: %3d URL: %s\n", i, pRecord->GetURL());
                    char szValue[PLAYLIST_STRING_SIZE];
                    if (SUCCEEDED(pRecord->GetAttribute(MDA_TITLE, (void**)&tszValue)))
                        DEBUGP(EV, DBGLEV_INFO, "             TITLE: %s\n", TcharToChar(szValue, tszValue));
                    if (SUCCEEDED(pRecord->GetAttribute(MDA_ARTIST, (void**)&tszValue)))
                        DEBUGP(EV, DBGLEV_INFO, "             ARTIST: %s\n", TcharToChar(szValue, tszValue));
                    if (SUCCEEDED(pRecord->GetAttribute(MDA_ALBUM, (void**)&tszValue)))
                        DEBUGP(EV, DBGLEV_INFO, "             ALBUM: %s\n", TcharToChar(szValue, tszValue));
                    if (SUCCEEDED(pRecord->GetAttribute(MDA_GENRE, (void**)&tszValue)))
                        DEBUGP(EV, DBGLEV_INFO, "             GENRE: %s\n", TcharToChar(szValue, tszValue));
                }
    }
    DEBUGP(EV, DBGLEV_INFO, "\n");
}

void
CEvents::Event(int key, void* data) 
{
static const char* s_szProgressChars = "/-\\|";
static int s_iProgressCharCount = 4;
static int s_iContentProgress = 0;
static int s_iMetadataProgress = 0;

    switch (key)
    {
        case EVENT_KEY_HOLD:
        {
            unsigned int keycode = (unsigned int) data;
            switch (keycode)
            {
                case KEY_VOLUME_UP:
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

                case KEY_VOLUME_DOWN:
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

                default:
                    return;
            };
            break;
        }
        case EVENT_KEY_PRESS:
        {
            unsigned int keycode = (unsigned int)data;
            DEBUGP(EV, DBGLEV_INFO, "Key press: %d\n", keycode);
            switch (keycode)
            {
                case KEY_PLAY_PAUSE:
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

                case KEY_STOP:
                    if (SUCCEEDED(m_pPlayManager->Stop()))
                    {
                        m_iTrackTime = 0;
                        m_pUserInterface->NotifyStopped();
                    }
                    else
                        SynchPlayState();
                    return;

                case KEY_PREVIOUS:
                    // If the track time is five seconds or less, then go to the previous track.
                    if (m_iTrackTime <= 5)
                    {
                        if (SUCCEEDED(m_pPlayManager->PreviousTrack()))
                            DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                        else
                            SynchPlayState();
                    }
                    // Otherwise, seek to the beginning of this track.
                    else
                    {
                        if (SUCCEEDED(m_pPlayManager->Seek(0)))
                            m_pUserInterface->SetTrackTime(0);
                        else
                            SynchPlayState();
                    }
                    return;

                case KEY_NEXT:
                    if (SUCCEEDED(m_pPlayManager->NextTrack()))
                        DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                    else
                        SynchPlayState();
                    return;

                case KEY_VOLUME_UP:
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

                case KEY_VOLUME_DOWN:
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

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
                        DEBUGP(EV, DBGLEV_INFO, "Loading playlist: URL: %s DS: %d Format: %d\n", (*m_itPlaylist)->GetURL(), (*m_itPlaylist)->GetDataSourceID(), (*m_itPlaylist)->GetPlaylistFormatID());
                        CMediaPlayer::GetInstance()->Deconfigure();
                        m_pPlayManager->GetPlaylist()->Clear();

                        // Use the playlist format ID and URL from the playlist content record to find the
                        // appropriate function to load.
                        CPlaylistFormatManager::GetInstance()->LoadPlaylist((*m_itPlaylist)->GetPlaylistFormatID(), (*m_itPlaylist)->GetURL(), m_pPlayManager->GetPlaylist(), true);
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
