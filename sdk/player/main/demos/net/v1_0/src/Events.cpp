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
#include <io/audio/VolumeControl.h>
#include <main/ui/common/UserInterface.h>
#include <util/debug/debug.h>

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

static int s_aryVolume[21] = {
  -96,
  -42, -39, -36,  -33, -30,
  -27, -24, -21,  -18, -15,
  -13, -11,  -9,   -7,  -5,
   -3,  -1,   1,    3,	 5 };

    m_pVolumeControl->SetVolumeRange(21, s_aryVolume);
    m_pVolumeControl->SetVolume(18);
}

CEvents::~CEvents() 
{
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
        if (m_pPlayManager->GetPlayState() == CMediaPlayer::PLAYING)
            m_pUserInterface->NotifyPlaying();
        else if (m_pPlayManager->GetPlayState() == CMediaPlayer::PAUSED)
            m_pUserInterface->NotifyPaused();
        else
            m_pUserInterface->NotifyStopped();
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
                    m_pPlayManager->Stop();
                    m_iTrackTime = 0;
                    m_pUserInterface->NotifyStopped();
                    return;


                case KEY_PREVIOUS:
                    if (SUCCEEDED(m_pPlayManager->PreviousTrack()))
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                    }
                    return;

                case KEY_NEXT:
                    if (SUCCEEDED(m_pPlayManager->NextTrack()))
                    {
                        DEBUGP(EV, DBGLEV_INFO, "Track %d of %d\n", m_pPlayManager->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, m_pPlayManager->GetPlaylist()->GetSize());
                    }
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
            }
            break;
        }

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

        default:
            break;
    };

    m_pPlayManager->HandleEvent(key, data);
}
