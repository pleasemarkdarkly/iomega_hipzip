// Events.h: how we get events
// danc@fullplaymedia.com 08/08/01
// (c) Fullplay Media Systems

#ifndef __EVENTS_H__
#define __EVENTS_H__

#define PLAYLIST_STRING_SIZE 128

#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <main/main/AppSettings.h> // PLAYLIST_STRING_SIZE
#include <main/ui/common/UserInterface.h>
#include <main/ui/Keys.h>
#include <core/playmanager/PlayManager.h> // interface to playmanager
#include <devs/lcd/lcd.h>

// fdecl
class IContentManager;
class CPlayManager;
class CVolumeControl;
class CEvents;

extern CEvents* g_pEvents;

class CEvents
{
public:
    CEvents();
    ~CEvents();

    void SetUserInterface( IUserInterface* pUI );
    void LoadState();
    void SaveState();
    void RefreshInterface();
    int Event( int key, void* data );
    // set the current playlist song in the mediaplayer, or the nearest available good track, or display the invalid playlist screen.
    bool SetCurrentSong();
    void ShutdownPlayer();

    // interface to handle shutting down the player when inactive
    void TestForRecentActivity();
    // the player isn't doing anything special, so shutdown if idle for too long.
    void EnterIdleShutdownMode();
    // the player is performing non-interactive operations that shouldn't be subject to idle shutdown.
    void ExitIdleShutdownMode();

    // en / dis able backlight functionality
    void EnableBacklight();
    void DisableBacklight();

    void SetKeyboardLock(bool bLocked);

private:
    void PrepareToYield();
    // event handlers
    int HandleShutdown(int key, void* data);
    int HandleKeyHold(int key, void* data);
    int HandleKeyPress(int key, void* data);
    int HandleKeyRelease(int key, void* data);
    int HandleFilesystemUpdate(int key, void* data);
    int HandleStreamSet(int key, void* data);
    int HandleStreamProgress(int key, void* data);
    int HandleStreamEnd(int key, void* data);
    int HandleMediaRemoved(int key, void* data);
    int HandleMediaInserted(int key, void* data);
    int HandleContentUpdateBegin(int key, void* data);
    int HandleContentUpdateEnd(int key, void* data);
    int HandleContentMetadataUpdate(int key, void* data);
    int HandleContentMetadataUpdateEnd(int key, void* data);
    int HandleContentUpdateError(int key, void* data);
    int HandleContentUpdate(int key, void* data);
    int HandleUSBConnect(int key, void* data);
    int HandleUSBDisconnect(int key, void* data);
    int HandleClipDetected(int key, void* data);
#ifdef DDOMOD_MAIN_TESTHARNESS
    void StartTestStimulator();
    void StopTestStimulator();
#endif
    void SynchPlayState();
    void BreakPointOnKey11();
    //
    // Class data
    //
    IUserInterface* m_pUserInterface;
    CPlayManager* m_pPlayManager;

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
    bool m_bCurrentTrackSet;

    // state for idle shutdown functionality
    bool m_bIdleShutdownMode;
    cyg_tick_count_t m_nTimeOfLastActivity;

    // state for backlight functionality
    bool m_bBacklightEnabled;
    bool m_bBacklightOn;

    // state for keyboard lock
    bool m_bKbdLocked;
};

#include <main/main/Events.inl>

#endif // __EVENTS_H__
