//
// PlayManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The play manager singleton is the nerve center of the Dadio(tm) system and is
//! the usual point of contact between the system and application-specific client
//! code.
/** \addtogroup PlayManager Play Manager */
//@{

#ifndef PLAYMANAGER_H_
#define PLAYMANAGER_H_

#include <content/common/ContentManager.h>  // content_record_update_t
#include <core/mediaplayer/MediaPlayer.h>   // PlayState
#include <datasource/common/DataSource.h>   // RefreshMode
#include <playlist/common/Playlist.h>       // PlaylistMode

#include <util/eresult/eresult.h>           // ERESULT

//
// fdecl
//
class IPlaylist;
class CPlayManagerImp;

//
//! Define the play manager error zone for ERESULT error codes
//
#define PLAYMANAGER_ERROR_ZONE 0xac

#define MAKE_PMRESULT(x,y) MAKE_ERESULT(x,PLAYMANAGER_ERROR_ZONE,y)

//! No error was encountered during the operation. \hideinitializer
const ERESULT PM_NO_ERROR       = MAKE_PMRESULT( SEVERITY_SUCCESS, 0x0000 );
//! An unspecified error occured. \hideinitializer
const ERESULT PM_ERROR          = MAKE_PMRESULT( SEVERITY_FAILED,  0x0001 );
//! All tracks in the playlist are bad.
//! Returned from a call to NextTrack, PreviousTrack, or a stream end event. \hideinitializer
const ERESULT PM_NO_GOOD_TRACKS = MAKE_PMRESULT( SEVERITY_FAILED,  0x0002 );
//! No next/previous track.
//! Returned from a call to NextTrack, PreviousTrack, or a stream end event. \hideinitializer
const ERESULT PM_PLAYLIST_END   = MAKE_PMRESULT( SEVERITY_FAILED,  0x0003 );
//! The next/previous track was set and is playing.
//! Returned from a call to NextTrack, PreviousTrack, or a stream end event.
//! If the functions return PM_NO_ERROR, then the next/previous track was
//! set successfully but is not playing back. \hideinitializer
const ERESULT PM_PLAYING        = MAKE_PMRESULT( SEVERITY_SUCCESS, 0x0004 );

//! The play manager is the nerve center of the system.  It maintains pointers to the
//! content manager and current playlist.  It has a default event handler.  It exposes
//! functions for play control.  It's the object that kicks off content updates.
class CPlayManager 
{
public:

    //! Get a pointer to the one global instance of the play manager.
    static CPlayManager* GetInstance();

    //! Destroy the singular instance of the play manager
    static void Destroy();

    //! Given an arbitrary (opened) data source, add it to the system list of
    //! available data sources
    void AddDataSource( IDataSource* pDS );

    //! Starts a content refresh from the available data sources
    //! \param mode Which refresh should be used by each data source.
    //! \param iUpdateChunkSize How many records should be accumulated before sending off an update event.
    //!                         If 0, then only one event will be sent per data source.
    void RefreshAllContent(IDataSource::RefreshMode mode = IDataSource::DSR_DEFAULT,
                            int iUpdateChunkSize = DS_DEFAULT_CHUNK_SIZE);

    //! Starts a content refresh on the specified data source
    //! \param mode Which refresh should be used by the data source.
    //! \param iUpdateChunkSize How many records should be accumulated before sending off an update event.
    //! If 0, then the data source will populate all content before sending an event.
    //! \retval The scan ID of the refresh cycle.
    unsigned short RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode = IDataSource::DSR_DEFAULT,
                            int iUpdateChunkSize = DS_DEFAULT_CHUNK_SIZE);

    //! Updates the content manager with content received from a source.
    //! Called by the default handler.  Only call this function if you are bypassing the default
    //! event handler during content update.
    void NotifyContentUpdate(content_record_update_t* pContentUpdate);

    //! Updates existing content records with metadata retrieved from a data source and a codec.
    //! Called by the default handler.  Only call this function if you are bypassing the default
    //! event handler during content update.
    void NotifyContentMetadataUpdate(content_record_update_t* pContentUpdate);

    //! Associates the play manager with a content manager.
    //! A content manager must be set before content refreshes can begin.
    void SetContentManager(IContentManager* pCM);
    //! Returns a pointer to the content manager.
    IContentManager* GetContentManager() const;

    //! Associates the play manager with a playlist.
    void SetPlaylist(IPlaylist* pPL);
    //! Returns a pointer to the current playlist.
    IPlaylist* GetPlaylist() const;

    //! Sets the current playstream used for playback
    void SetPlaystream(const CPlayStreamSettings* pSettings, FNCreatePlayStream* pfnCreatePlayStream = 0);

    //! Returns the current mode for traversing the playlist.
    IPlaylist::PlaylistMode GetPlaylistMode() const;

    //! Sets the play mode.  If the mode is RANDOM or REPEAT_RANDOM, then the playlist is reshuffled
    //! with the current entry set as the first entry in the random list.
    void SetPlaylistMode(IPlaylist::PlaylistMode mode);

    // SetSong
    //!  SetSong will use the playstream settings to generate a
    //!  Playstream. if there are  no settings it will fail
    ERESULT SetSong( IPlaylistEntry* pNewSong );

    // Deconfigure
    //!  Deconfigure will stop playback, unload the current track in the media player,
    //!  and return the play manager to the NOT_CONFIGURED state.
    void Deconfigure();

    //! Begin playback of the current entry in the media player.
    //! If the media player hasn't been configured, then the current playlist entry
    //! is set as the current entry.
    ERESULT Play();
    //! Pause playback of the current entry in the media player.
    ERESULT Pause();
    //! Stop playback of the current entry in the media player.
    ERESULT Stop();
    //! Seek to the specified time offset in the file.
    ERESULT Seek( unsigned long secSeek );

    //! Query for current play state
    CMediaPlayer::PlayState GetPlayState() const;

    //! Go to the next track in the playlist.
    //! If an entry is unable to be set in the media player, then the next track will be tried.
    //! If all tracks following the current one are bad, then the current track is re-set.
    //! If the current track can't be set, then the play manager attempts to set a track previous
    //! to the current one.
    //! \retval PM_PLAYING The next track was set successfully and is currently playing.
    //! \retval PM_NO_ERROR The next track was set successfully.
    //! \retval PM_NO_GOOD_TRACKS All tracks in the playlist are invalid.
    //! \retval PM_PLAYLIST_END The next track couldn't be set because the end of the playlist
    //! was reached.
    ERESULT NextTrack();

    //! Go to the previous track in the playlist.
    //! If an entry is unable to be set in the media player, then the previous track will be tried.
    //! If all tracks prior the current one are bad, then the current track is re-set.
    //! If the current track can't be set, then the play manager attempts to set a track following
    //! the current one.
    //! \retval PM_PLAYING The previous track was set successfully and is currently playing.
    //! \retval PM_NO_ERROR The previous track was set successfully.
    //! \retval PM_NO_GOOD_TRACKS All tracks in the playlist are invalid.
    //! \retval PM_PLAYLIST_END The previous track couldn't be set because the end of the playlist
    //! was reached.
    ERESULT PreviousTrack();
    
    //! The system event handler.  This should be called from the client event loop to
    //! process system-specific events and to handle event cleanup.
    ERESULT HandleEvent(int key, void* data);

    //! Informs the play manager that a data source's media has been removed.
    //! Called by the default handler.  Only call this function if you are bypassing the default
    //! event handler during content update.
    void NotifyMediaRemoved(int iDataSourceID);

    //! Informs the play manager that a data source's media has been inserted.
    //! Called by the default handler.  Only call this function if you are bypassing the default
    //! event handler during content update.
    void NotifyMediaInserted(int iDataSourceID);
    
private:

    CPlayManager();
    ~CPlayManager();

    CPlayManagerImp*    m_pPlayManagerImp;
};

//@}

#endif  // PLAYMANAGER_H_
