//
// MediaPlayer.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Dadio(tm) uses the Media Player as a central control for audio playback
//! and output. The Media Player is responsible for opening new tracks,
//! decoding the bitstream, and providing track progress. Events are generated
//! from the media player for state changes (such as stream end) and track
//! progress. Much of the functionality provided by the media player is
//! wrapped by the play manager, allowing the play manager to coordinate
//! operations that require multiple modules.

/** \addtogroup MediaPlayer Media Player */
//@{

#ifndef MEDIAPLAYER_H_
#define MEDIAPLAYER_H_

#include <cyg/kernel/kapi.h>                // for cyg_thread, cyg_handle_t
#include <util/eresult/eresult.h>           // for ERESULT

//
//! Define the error zone for media player eresult codes
//
#define MEDIAPLAYER_ERROR_ZONE 0x0c

#define MAKE_MPRESULT(x,y) MAKE_ERESULT(x,MEDIAPLAYER_ERROR_ZONE,y)

//! No error was encountered during the operation. \hideinitializer
const ERESULT MP_NO_ERROR       = MAKE_MPRESULT( SEVERITY_SUCCESS, 0x0000 );
//! A generic failure occurred. \hideinitializer
const ERESULT MP_ERROR          = MAKE_MPRESULT( SEVERITY_FAILED,  0x0001 );
//! A play control was issued on the media player prior to
//! configuration. \hideinitializer
const ERESULT MP_NOT_CONFIGURED = MAKE_MPRESULT( SEVERITY_FAILED,  0x0002 );
//! Returned when a "next track" wasn't set
const ERESULT MP_DIDNT_ENQUEUE  = MAKE_MPRESULT( SEVERITY_FAILED, 0x0003 );
//! Returned when the requested feature is unavailable.
const ERESULT MP_UNAVAILABLE    = MAKE_MPRESULT( SEVERITY_FAILED, 0x0004 );
//! Used by PlayStream, indicates that a candidate codec didn't recognize
//! the given stream. \hideinitializer
const ERESULT MP_TRY_NEXT_CODEC = MAKE_MPRESULT( SEVERITY_FAILED, 0x1000 );

//
// fdecl classes we use
//

class CEventQueue;
class CMediaPlayerImp;
class IMetadata;
class IPlaylistEntry;
class IMediaContentRecord;
class IInputStream;
class CPlayStreamSettings;

//
// FNCreateMetadata()
//! Prototype for a function the mediaplayer can call to create
//! an appropriate IMetadata object.
//
typedef IMetadata* FNCreateMetadata();

//
// FNCreateInputStream()
//! Prototype for a function the mediaplayer can call to create
//! an appropriate IInputStream object.
//
typedef IInputStream* FNCreateInputStream(IMediaContentRecord*);

//
// FNCreatePlaystream()
//! Prototype for a function to populate a CPlayStreamSettings
//! class to use for this track; returns true if settings were
//! set, or false to use the mediaplayer default settings
//
typedef bool FNCreatePlayStream(IMediaContentRecord*, CPlayStreamSettings*);


//
// CMediaPlayer singleton
//! CMediaPlayer is the basic interface for the media player.
//! This is a singleton class that can be referenced through
//! the GetInstance() routine.
//
class CMediaPlayer 
{
public:
    //! Get a pointer to the singleton media player.
    static CMediaPlayer* GetInstance();

    //! Properly destroy the singleton MP
    static void Destroy();
    
    //! Enumeration to indicate the current playstate
    enum PlayState 
    {
        PLAYING=0,
        PAUSED,
        STOPPED,
        NOT_CONFIGURED,
    };

    //! Sets the current playstream. This will take effect when
    //! the next call to SetSong occurs.
    ERESULT SetPlayStreamSettings( const CPlayStreamSettings* pSettings );

    // SetCreateMetadataFunction
    //! Sets the function that will be called to generate a new IMetadata record to be populated
    //! during SetSong.
    //! If pfnCreateMetadata is 0, then no metadata record will be generated during SetSong (and no
    //! metadata will be retrieved).
    void SetCreateMetadataFunction(FNCreateMetadata* pfnCreateMetadata);
    
    // SetCreateInputStreamFunction
    //! Sets the function that will be called to generate a new IStream input stream to be used
    //! during playback.
    //! If pfnCreateInputStream is 0, then the media player will default to a basic unbuffered stream.
    void SetCreateInputStreamFunction(FNCreateInputStream* pfnCreateInputStream);

    // SetCreatePlayStreamFunction
    //! Sets the function called during SetSong to generate a new CPlayStreamSettings to be used
    //! for the current song
    //! If called with 0, the media player will use the default play settings
    void SetCreatePlayStreamFunction(FNCreatePlayStream* pfnCreatePlayStream);

    // SetCodecPool
    //! Sets a codec pool for the media player. This will cause the media player to allocate codecs
    //! in-place in the pool (if possible) whenever an allocation is requested. If the codec pool
    //! is not set, the allocation will come from the heap
    void SetCodecPool( unsigned int* pPoolAddress, unsigned int iPoolSize );

    // SetSRCBlending
    //! Forces the mediaplayer to try and use a mix of hardware and software sample rate
    //! conversion. This is only available if software sample rate conversion has been compiled
    //! in. The mediaplayer will attempt to use software src to convert the audio stream to
    //! _at least_ the specified sample rate. It will then rely on hardware sample rate conversion
    //! to handle the remainder of the conversion. If software SRC is unavailable, this will fail.
    //! Call with 0 to disable SRC blending.
    ERESULT SetSRCBlending( int MinSoftwareSampleRate );

    // QuerySRCBlending
    //! Determines the current sample rate blending level. See the CMediaPlayerImp::SetSRCBlending.
    //! Returns 0 if SRC blending is not being used.
    int QuerySRCBlending() const;
    
    // SetSong
    //!  SetSong will use the playstream settings to generate a
    //!  Playstream. if there are  no settings it will fail
    ERESULT SetSong( IPlaylistEntry* pNewSong );

    // SetNextSong
    //! SetNextSong uses the playstream settings to generate a playstream which will
    //! be played immediately after the current song. If there is no current song,
    //! it will fall back to SetSong. If there is already a "next" playstream, it will
    //! be deleted and replaced.
    ERESULT SetNextSong( IPlaylistEntry *pNewSong );

    // InvalidateNextSong
    //! The mediaplayer can queue up playstreams for fast transitions, but in the event
    //! that the "next" song changes (i.e. playlist mode changes), the next track can become
    //! invalid. As such, provide an API for the playmanager to junk the next track
    ERESULT InvalidateNextSong();
    
    // Deconfigure
    //!  Deconfigure will unload any objects that are currently loaded,
    //!  and return the mediaplayer to the NOT_CONFIGURED state
    void Deconfigure();
    
    // Play tree control
    //  After SetSong has been called, you can manually alter the tree using
    //  the below routines. GetRoot() effectively returns the codec, since the
    //  tree is formed below that.
    //! Not currently implemented
    //    StreamNode_t* GetRoot() const;
    //! Not currently implemented
    //    StreamNode_t* AddNode( StreamNode_t* pRoot, unsigned int ChildID );
    //! Not currently implemented
    //    ERESULT RemoveNode( unsigned int NodeID );
    //! Not currently implemented
    //    ERESULT RemoveNode( StreamNode_t* pNode  );
    
    // Play control functions
    //! Start playback, or resume if paused.
    ERESULT Play();
    //! Pause playback.
    ERESULT Pause();
    //! Stop playback.
    ERESULT Stop();
    //! Seek to a new offset in the song
    ERESULT Seek( unsigned long secSeek );

    //! Query for current play state
    PlayState GetPlayState() const;

    // Track length, current track time
    // The track length call is provided for consistency - the call itself
    // is proxied through the current playlist entry
    //! Returns the current playing time of the track.
    unsigned long GetTrackTime() const;
    //! Returns the total playing time of the track.
    unsigned long GetTrackLength() const;

private:

    CMediaPlayer();
    ~CMediaPlayer();

    CMediaPlayerImp*    m_pImp;
};

//@}

#endif  // MEDIAPLAYER_H_
