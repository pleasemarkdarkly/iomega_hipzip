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
#include <core/events/SystemEvents.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <io/audio/VolumeControl.h>
#include <main/ui/common/UserInterface.h>
#include <util/debug/debug.h>
#include <stdlib.h>

// Set up the debug module used for the app's event handler.
DEBUG_MODULE_S(EV, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(EV);

CEvents::CEvents() 
{
    m_pUserInterface = 0;
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = m_pPlayManager->GetContentManager();
    m_pVolumeControl = CVolumeControl::GetInstance();
    m_iTrackTime = 0;
    m_szSettingsURL = 0;

static int s_aryVolume[21] = {
  -96,
  -42, -39, -36,  -33, -30,
  -27, -24, -21,  -18, -15,
  -13, -11,  -9,   -7,  -5,
   -3,  -1,   1,    3,	 5 };

    m_pVolumeControl->SetVolumeRange(21, s_aryVolume);
}

CEvents::~CEvents() 
{
    free(m_szSettingsURL);
}

void CEvents::SetUserInterface(IUserInterface* pUI) 
{
    m_pUserInterface = pUI;
}

// Set the URL of the stream that will be used for loading and saving
// volume, bass, and treble levels.
void CEvents::SetSettingsURL(const char* szURL)
{
    free(m_szSettingsURL);
    m_szSettingsURL = (char*)malloc(strlen(szURL) + 1);
    strcpy(m_szSettingsURL, szURL);
}

// Load the volume, bass, and treble levels from the settings file.
bool CEvents::LoadState()
{
    DEBUGP(EV, DBGLEV_INFO, "Loading settings\n");
    IInputStream* pIS;
    if (m_szSettingsURL && (pIS = CDataSourceManager::GetInstance()->OpenInputStream(m_szSettingsURL)))
    {
        CRegistry::GetInstance()->RestoreState(pIS);
        delete pIS;
        m_pVolumeControl->RestoreFromRegistry();
        return true;
    }
    else
    {
        DEBUGP(EV, DBGLEV_INFO, "Unable to open settings file\n");
        return false;
    }
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

// The application's event handler.
void CEvents::Event(int key, void* data) 
{
    switch (key)
    {
        case EVENT_KEY_HOLD:
        {
            unsigned int keycode = (unsigned int) data;
            switch(keycode)
            {
                case KEY_VOLUME_UP:
                    m_pVolumeControl->VolumeUp();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

                case KEY_VOLUME_DOWN:
                    m_pVolumeControl->VolumeDown();
                    m_pUserInterface->SetVolume(m_pVolumeControl->GetVolume());
                    return;

                case KEY_BASS_UP:
                    m_pVolumeControl->BassUp();
                    m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
                    return;

                case KEY_BASS_DOWN:
                    m_pVolumeControl->BassDown();
                    m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
                    return;

                case KEY_TREBLE_UP:
                    m_pVolumeControl->TrebleUp();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
                    return;

                case KEY_TREBLE_DOWN:
                    m_pVolumeControl->TrebleDown();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
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
            switch(keycode)
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

                case KEY_BASS_UP:
                    m_pVolumeControl->BassUp();
                    m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
                    return;

                case KEY_BASS_DOWN:
                    m_pVolumeControl->BassDown();
                    m_pUserInterface->SetBass(m_pVolumeControl->GetBass());
                    return;

                case KEY_TREBLE_UP:
                    m_pVolumeControl->TrebleUp();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
                    return;

                case KEY_TREBLE_DOWN:
                    m_pVolumeControl->TrebleDown();
                    m_pUserInterface->SetTreble(m_pVolumeControl->GetTreble());
                    return;

                case KEY_PLAYLIST_MODE:
                    m_pPlayManager->SetPlaylistMode((IPlaylist::PlaylistMode)((m_pPlayManager->GetPlaylistMode() + 1) % 5));
                    m_pUserInterface->SetPlaylistMode(m_pPlayManager->GetPlaylistMode());
                    return;

                case KEY_REFRESH_CONTENT:
                    DEBUGP(EV, DBGLEV_INFO, "Refreshing playlist\n");
                    m_pPlayManager->RefreshAllContent(IDataSource::DSR_ONE_PASS);
                    return;

                case KEY_SAVE_STATE:
                {
                    DEBUGP(EV, DBGLEV_INFO, "Saving state\n");
                    m_pUserInterface->SetMessage("Saving state");
                    m_pVolumeControl->SaveToRegistry();
                    IOutputStream* pOS;
                    if (m_szSettingsURL && (pOS = CDataSourceManager::GetInstance()->OpenOutputStream(m_szSettingsURL)))
                    {
                        CRegistry::GetInstance()->SaveState(pOS);
                        delete pOS;
                        m_pUserInterface->SetMessage("Saved state");
                    }
                    else
                        m_pUserInterface->SetMessage("Unable to open settings file");
                    return;
                }

                case KEY_LOAD_STATE:
                    DEBUGP(EV, DBGLEV_INFO, "Loading state\n");
                    m_pUserInterface->SetMessage("Loading state");
                    if (LoadState())
                    {
                        RefreshInterface();
                        m_pUserInterface->SetMessage("Loaded state");
                    }
                    else
                        m_pUserInterface->SetMessage("Unable to open settings file");
                    return;

            }
            break;
        }

        case EVENT_CONTENT_UPDATE_BEGIN:
            m_pUserInterface->SetMessage("Starting content update");
            m_pPlayManager->HandleEvent(key, data);
            return;

        case EVENT_CONTENT_UPDATE_END:
        {
            m_pUserInterface->SetMessage("Content update finished");
            // The default event hanlder will add these entries to the content manager.
            m_pPlayManager->HandleEvent(key, data);

            // Create a playlist using all the records available in the content manager.
            MediaRecordList records;
            m_pContentManager->GetAllMediaRecords(records);
            m_pPlayManager->GetPlaylist()->AddEntries(records);
            if (SUCCEEDED(m_pPlayManager->Play()))
                m_pUserInterface->NotifyPlaying();
            else {
                if (SUCCEEDED(m_pPlayManager->NextTrack()) && SUCCEEDED(m_pPlayManager->Play())) {
                    m_pUserInterface->NotifyPlaying();
                } else {
                    m_pUserInterface->SetMessage("No content found");
                }
            }
            return;
        }

        case EVENT_CONTENT_UPDATE_ERROR:
            m_pUserInterface->SetMessage("Error updating content");
            m_pPlayManager->HandleEvent(key, data);
            return;

        case EVENT_STREAM_SET:
        {
            m_iTrackTime = 0;
            set_stream_event_data_t* pSSED = (set_stream_event_data_t*)data;
            m_pUserInterface->SetTrack(pSSED, m_pPlayManager->GetPlaylist()->GetSize());
            m_pPlayManager->HandleEvent(key, data);
            return;
        }

		case EVENT_STREAM_PROGRESS:
            m_iTrackTime = (int)data;
			m_pUserInterface->SetTrackTime((int) data);
			return;

        case EVENT_MEDIA_REMOVED:
            // The content manager's database has changed, so remember to update
            // the lists of artists, albums, genres, and playlists.
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaRemoved((int)data);
            return;

        case EVENT_MEDIA_INSERTED:
            // The system event handler will kick off a content refresh using the default
            // refresh mode and update chunk size of the data source.
            m_pPlayManager->HandleEvent(key, data);
            m_pUserInterface->NotifyMediaInserted((int)data);
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

        default:
            break;
    };

    m_pPlayManager->HandleEvent(key, data);
}
