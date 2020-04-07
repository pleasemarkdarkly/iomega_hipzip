// Events.h: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <_modules.h>

// TODO- if you dont use metakitcontentmanager, but have
//  a queryablecontentmanager derived class, just change this macro
#ifdef DDOMOD_CONTENT_METAKITCONTENTMANAGER
  #include <content/common/QueryableContentManager.h>
  #define SUPPORT_CM_QUERIES
  #define ContentManagerType CMetakitContentManager 
#else
  #define ContentManagerType IContentManager
#endif

// fdecl
class ContentManagerType;
class CPlayManager;
class IUserInterface;
class CVolumeControl;

// buttons - these are just numbers to uniquely
//  identify the events, and dont relate to the
//  physical mapping

#define KEY_BATCH_ENCODE        22  // Batch encode
#define KEY_BATCH_RIP           21  // Batch rip, no encode
#define KEY_BASS_UP             20  // Bass up
#define KEY_BASS_DOWN           19  // Bass down
#define KEY_TREBLE_UP           18  // Treble up
#define KEY_TREBLE_DOWN         17  // Treble down
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

    // Set the URL of the stream that will be used for loading and saving
    // volume, bass, and treble levels.
    void SetSettingsURL(const char* szURL);
    // Set the URL of the stream that will be used for loading and saving the content manager
    // database.
    void SetContentStateURL(const char* szURL);
    // Set the URL of the stream that will be used for loading and saving the current playlist.
    void SetPlaylistURL(const char* szURL);

    // Set the UI
    void SetUserInterface( IUserInterface* pUI );

    // Process an event
    void Event( int key, void* data );

    // Redraw the UI
    void RefreshInterface();
    
    //
    // Helpful debugging functions:
    //
    // Prints a list of artists, albums, genres, and playlists in the content manager.
    void PrintCategories();
    // Prints the contents of the current playlist.
    void PrintPlaylist();

  private:

    void SynchPlayState();

    int m_iTrackTime;

#ifdef SUPPORT_CM_QUERIES
    int  m_iArtistIndex,    m_iAlbumIndex,    m_iGenreIndex;
    bool m_bRefreshArtists, m_bRefreshAlbums, m_bRefreshGenres, m_bRefreshPlaylists;
    ContentKeyValueVector m_Artists, m_Albums, m_Genres;
    PlaylistRecordList m_Playlists;
    PlaylistRecordIterator m_itPlaylist;
#endif

#ifdef DDOMOD_EXTRAS_IDLECODER
    class ::CIdlerCoder;
    CIdlerCoder* m_pIdleCoder;
#endif

    char* m_szSettingsURL, * m_szContentStateURL, * m_szPlaylistURL;
    
    //
    // Class data
    //
    IUserInterface* m_pUserInterface;
    ContentManagerType * m_pContentManager;
    CPlayManager* m_pPlayManager;
    CVolumeControl* m_pVolumeControl;
};

#endif // __EVENTS_H__
