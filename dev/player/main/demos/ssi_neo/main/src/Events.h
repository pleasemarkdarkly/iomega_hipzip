// Events.h: how we get events
// danc@iobjects.com 08/08/01
// (c) Interactive Objects

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <main/demos/ssi_neo/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

// fdecl
class IContentManager;
class CPlayManager;
class IUserInterface;
class CVolumeControl;

class CEvents
{
public:
    CEvents();
    ~CEvents();

    void SetUserInterface( IUserInterface* pUI );
    void LoadState();
    void RefreshInterface();
    int Event( int key, void* data );
    
private:

    void SynchPlayState();

    //
    // Class data
    //
    IUserInterface* m_pUserInterface;
    IQueryableContentManager* m_pContentManager;
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

    short m_iCurrentPlaylistCount;
    char m_sCurrentPlaylistName[PLAYLIST_STRING_SIZE];
    char m_sCurrentPlaylistString[PLAYLIST_STRING_SIZE];
    bool m_bCurrentTrackSet;
};

#endif // __EVENTS_H__
