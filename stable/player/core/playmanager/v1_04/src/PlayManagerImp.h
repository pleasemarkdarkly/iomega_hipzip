//
// PlayManagerImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef PLAYMANAGERIMP_H_
#define PLAYMANAGERIMP_H_

#include <content/common/ContentManager.h>  // content_record_update_t
#include <core/mediaplayer/MediaPlayer.h>   // PlayState
#include <datasource/common/DataSource.h>   // RefreshMode
#include <playlist/common/Playlist.h>       // PlaylistMode

//
// fdecl
//
class CDataSourceManager;
class IDataSource;
class IPlaylist;
class IPlaylistEntry;

class CPlayManagerImp
{
public:

    CPlayManagerImp();
    ~CPlayManagerImp();

    // AddDataSource()
    //  Given an arbitrary (opened) data source, add it to the system list of
    //  available data sources
    void AddDataSource( IDataSource* pDS );

    // RefreshAllContent()
    //  starts a content refresh from the available data sources
    void RefreshAllContent(IDataSource::RefreshMode mode = IDataSource::DSR_DEFAULT, int iUpdateChunkSize = 0);

    // RefreshContent()
    //  starts a content refresh on the specified data source
    unsigned short RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode = IDataSource::DSR_DEFAULT, int iUpdateChunkSize = 0);

    // NotifyContentUpdate()
    //  updates the content manager with content received from a source
    void NotifyContentUpdate( content_record_update_t* pContentUpdate );

    // NotifyContentMetadataUpdate()
    //  updates existing content records with metadata retrieved from a data source and a codec
    void NotifyContentMetadataUpdate( content_record_update_t* pContentUpdate );

    // SetContentManager()
    //  Associates the play manager with a content manager
    void SetContentManager( IContentManager* pCM );
    IContentManager* GetContentManager() const;

    // SetPlaylist()
    //  Associates the play manager with a playlist
    void SetPlaylist( IPlaylist* pPL );
    IPlaylist* GetPlaylist() const;

    //! Sets the current playstream used for playback
    void SetPlaystream(const CPlayStreamSettings* pSettings, FNCreatePlayStream* pfnCreatePlayStream);

    // play mode
    IPlaylist::PlaylistMode GetPlaylistMode() const
        { return m_playlistMode; }
    // Sets the play mode.  If the mode is RANDOM or REPEAT_RANDOM, then the playlist is reshuffled
    // with the current entry set as the first entry in the random list.
    void SetPlaylistMode(IPlaylist::PlaylistMode mode);

    // SetSong
    //!  SetSong will use the playstream settings to generate a
    //!  Playstream. if there are  no settings it will fail
    ERESULT SetSong( IPlaylistEntry* pNewSong );

    // Deconfigure
    //!  Deconfigure will stop playback, unload the current track in the media player,
    //!  and return the play manager to the NOT_CONFIGURED state.
    void Deconfigure();

    // play control
    ERESULT Play();
    ERESULT Pause();
    ERESULT Stop();
    ERESULT Seek( unsigned long secSeek );

    // Query for current play state
    CMediaPlayer::PlayState GetPlayState();

    // track control
    // Set the next or previous track
    ERESULT NextTrack(bool bBacktrackIfNeeded = false);
    ERESULT PreviousTrack(bool bBacktrackIfNeeded = false);
    
    // The default event handler.
    ERESULT HandleEvent( int key, void* data );

    // Called from the event handler when a data source's media is removed.
    void NotifyMediaRemoved(int iDataSourceID);

    // Called from the event handler when a data source's media is changed.
    void NotifyMediaInserted(int iDataSourceID);

private:

    CMediaPlayer* m_pMediaPlayer;
    CDataSourceManager* m_pDataSourceManager;
    IContentManager* m_pContentManager;
    IPlaylist* m_pPlaylist;
    IPlaylist::PlaylistMode m_playlistMode;
    CMediaPlayer::PlayState m_playState;
};

#endif  // PLAYMANAGERIMP
