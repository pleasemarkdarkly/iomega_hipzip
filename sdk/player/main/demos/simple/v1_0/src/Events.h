//
// Events.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//
// The CEvents class acts as a simple, application-specific event handler.
//

#ifndef EVENTS_H_
#define EVENTS_H_

// fdecl
class IContentManager;
class CPlayManager;
class IUserInterface;
class CVolumeControl;

// physical layout to key map mapping - i.e. how the buttons correspond to elements
// in the key maps we give to the keyboard driver
// buttons on v1
// (16) (14) (12) (10) ( 8) ( 6) ( 4) ( 2)
// (15) (13) (11) ( 9) ( 7) ( 5) ( 3) ( 1)
// buttons on v2
// (16) (14) (12) (11) (10) ( 9) ( 8) ( 7)
// (15) (13) ( 6) ( 5) ( 4) ( 3) ( 2) ( 1)
#define KEY_PLAY_PAUSE          16  // Toggle play/pause
#define KEY_STOP                15  // Stop playback
#define KEY_PREVIOUS            14  // Go to the previous track in the playlist
#define KEY_NEXT                13  // Go to the next track in the playlist
#define KEY_VOLUME_UP           12  // Volume up
#define KEY_VOLUME_DOWN         11  // Volume down
#define KEY_BASS_UP             10  // Bass up
#define KEY_BASS_DOWN           9   // Bass down
#define KEY_TREBLE_UP           8   // Treble up
#define KEY_TREBLE_DOWN         7   // Treble down
#define KEY_PLAYLIST_MODE       6   // Cycle through the available playlist modes
#define KEY_REFRESH_CONTENT     5   // Ask all data sources to list their content
#define KEY_SAVE_STATE          2   // Save the state of the system and the content manager
#define KEY_LOAD_STATE          1   // Load the state of the system and the content manager

class CEvents
{
public:
    CEvents();
    ~CEvents();

    void SetUserInterface(IUserInterface* pUI);

    // Set the URL of the stream that will be used for loading and saving
    // volume, bass, and treble levels.
    void SetSettingsURL(const char* szURL);

    // Load the volume, bass, and treble levels from the settings file.
    bool LoadState();

    // Redraw the interface with the current settings.
    void RefreshInterface();

    // The application's event handler.
    void Event(int key, void* data);

private:

    // Match the play state icon to the play manager's play state.
    void SynchPlayState();

    //
    // Class data
    //
    IUserInterface* m_pUserInterface;
    IContentManager* m_pContentManager;
    CPlayManager* m_pPlayManager;
    CVolumeControl* m_pVolumeControl;

    int m_iTrackTime;

    char* m_szSettingsURL;
};

#endif  // EVENTS_H_
