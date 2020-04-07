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

#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

// fdecl
class CMetakitContentManager;
class CPlayManager;
class IUserInterface;
class CVolumeControl;

// buttons
// (16) (14) (12) (10) ( 8) ( 6) ( 4) ( 2)
// (15) (13) (11) ( 9) ( 7) ( 5) ( 3) ( 1)
#define KEY_PLAY_PAUSE          16  // Toggle play/pause
#define KEY_STOP                15  // Stop playback
#define KEY_PREVIOUS            14  // Go to the previous track in the playlist
#define KEY_NEXT                13  // Go to the next track in the playlist
#define KEY_VOLUME_UP           12  // Volume up
#define KEY_VOLUME_DOWN         11  // Volume down
#define KEY_LIST_BY_ARTIST      10  // Construct a playlist based on a single artist
#define KEY_LIST_BY_GENRE       9   // Construct a playlist based on a single genre
#define KEY_LIST_BY_ALBUM       8   // Construct a playlist based on a single album
#define KEY_LIST_BY_PLAYLIST    7   // Load a playlist found on a data source
#define KEY_PLAYLIST_MODE       6   // Cycle through the available playlist modes
#define KEY_REFRESH_CONTENT     5   // Ask all data sources to list their content
#define KEY_SAVE_PLAYLIST       4   // Save the current playlist
#define KEY_LOAD_PLAYLIST       3   // Load the playlist that was saved by the previous command
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
    // Set the URL of the stream that will be used for loading and saving the content manager
    // database.
    void SetContentStateURL(const char* szURL);
    // Set the URL of the stream that will be used for loading and saving the current playlist.
    void SetPlaylistURL(const char* szURL);

    // Redraw the interface with the current settings.
    void RefreshInterface();

    // The application's event handler.
    void Event(int key, void* data);

    //
    // Helpful debugging functions:
    //
    // Prints a list of artists, albums, genres, and playlists in the content manager.
    void PrintCategories();
    // Prints the contents of the current playlist.
    void PrintPlaylist();

private:

    // Match the play state icon to the play manager's play state.
    void SynchPlayState();

    //
    // Class data
    //
    IUserInterface* m_pUserInterface;
    CMetakitContentManager* m_pContentManager;
    CPlayManager* m_pPlayManager;
    CVolumeControl* m_pVolumeControl;

    int m_iTrackTime;

    ContentKeyValueVector m_Artists;
    int m_iArtistIndex;
    bool m_bRefreshArtists;
    ContentKeyValueVector m_Albums;
    int m_iAlbumIndex;
    bool m_bRefreshAlbums;
    ContentKeyValueVector m_Genres;
    int m_iGenreIndex;
    bool m_bRefreshGenres;
    PlaylistRecordList m_Playlists;
    PlaylistRecordIterator m_itPlaylist;
    bool m_bRefreshPlaylists;

    char* m_szSettingsURL;
    char* m_szContentStateURL;
    char* m_szPlaylistURL;
};

#endif  // EVENTS_H_
