//
// MediaPlayerImp.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef MEDIAPLAYERIMP_H_
#define MEDIAPLAYERIMP_H_

#include <core/mediaplayer/MediaPlayer.h>
#include <codec/common/Codec.h>
#include <datastream/filter/Filter.h>
#include <cyg/kernel/kapi.h>                // for cyg_thread, cyg_handle_t

#include <util/eresult/eresult.h>           // for ERESULT
#include <_modules.h>

// Hardware/Software SRC support; used in playstream.cpp and mediaplayerimp.cpp
#if defined(DDOMOD_FILTER_SRC)
#include <datastream/srcfilter/SRCFilterKeys.h>
#define SUPPORT_SOFTWARE_SRC
#endif // DDOMOD_FILTER_SRC

#if defined(DDOMOD_DATASTREAM_WAVEOUT)
#include <datastream/waveout/WaveOutKeys.h>
#define SUPPORT_HARDWARE_SRC
#endif // DDOMOD_DATASTREAM_WAVEOUT

// TODO migrate this out (config?)
#define DECODER_THREAD_STACK_SIZE    8192*3
#define DECODER_THREAD_PRIORITY        9

typedef struct stream_info_s stream_info_t;
typedef struct filter_stream_info_s filter_stream_info_t;

class CPlayStream;

class CMediaPlayerImp
{
public:
    static CMediaPlayerImp* GetInstance();
    static void Destroy();

    // PlayStream control
    //  SetPlayStream() sets the current playstream configuration
    ERESULT SetPlayStreamSettings( const CPlayStreamSettings* pSettings );

    // SetCreatePlayStreamFunction
    //  Sets the function called during SetSong to generate a new playstream_settings_t to be used
    //  for the current song
    //  If called with 0, the media player will use the default play settings
    void SetCreatePlayStreamFunction(FNCreatePlayStream* pfnCreatePlayStream);
    
    // SetCreateMetadataFunction
    //  Sets the function that will be called to generate a new IMetadata record to be populated
    //  during SetSong.
    //  If pfnCreateMetadata is 0, then no metadata record will be generated during SetSong (and no
    //  metadata will be retrieved).
    void SetCreateMetadataFunction(FNCreateMetadata* pfnCreateMetadata);

    // SetCreateInputStreamFunction
    //  Sets the function that will be called to generate a new IStream input stream to be used
    //  during playback.
    //  If pfnCreateInputStream is 0, then the media player will default to a basic unbuffered stream.
    void SetCreateInputStreamFunction(FNCreateInputStream* pfnCreateInputStream);


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
    //! Call with 44100 to use software SRC exclusively, or with 0 to use hardware or no SRC.
    ERESULT SetSRCBlending( int MinSoftwareSampleRate );

    // QuerySRCBlending
    //! Determines the current sample rate blending level. See the CMediaPlayerImp::SetSRCBlending.
    //! Returns 44100 if software SRC is being used exclusively, 0 if hardware (or no) SRC is being
    //! used.
    int QuerySRCBlending() const;
    
    // SetSong
    //!  SetSong will use the playstream settings to generate a
    //!  Playstream. If there are no settings it will fail.
    ERESULT SetSong( IPlaylistEntry* pNewSong );

    // SetStream
    //!  Starts an already created playstream, interrupting the current song
    ERESULT SetStream( CPlayStream* pNewStream );

    // SetNextSong
    //! SetNextSong uses the playstream settings to generate a playstream which will
    //! be played immediately after the current song. If there is no current song,
    //! it will fall back to SetSong. If there is already a "next" playstream, it will
    //! be deleted and replaced.
    ERESULT SetNextSong(IPlaylistEntry *pNewSong);

    // InvalidateNextSong
    //! The mediaplayer can queue up playstreams for fast transitions, but in the event
    //! that the "next" song changes (i.e. playlist mode changes), the next track can become
    //! invalid. As such, provide an API for the playmanager to junk the next track
    ERESULT InvalidateNextSong();
    
    // Deconfigure
    //  Deconfigure will unload any objects that are currently loaded,
    //  and return the mediaplayer to the NOT_CONFIGURED state
    void Deconfigure();
    
    // Play tree control
    //  After SetSong has been called, you can manually alter the tree using
    //  the below routines. GetRoot() effectively returns the codec, since the
    //  tree is formed below that. (NOT IMPLEMENTED)
    //    StreamNode_t* GetRoot() const;
    //    StreamNode_t* AddNode( StreamNode_t* pRoot, unsigned int ChildID );
    //    ERESULT RemoveNode( unsigned int NodeID );
    //    ERESULT RemoveNode( StreamNode_t* pNode  );
    
    // Play control functions
    ERESULT Play();
    ERESULT Pause();
    ERESULT Stop();
    ERESULT Seek( unsigned long secSeek );

    // Query for current play state
    CMediaPlayer::PlayState GetPlayState() const;

    // Track length, current track time
    // The track length call is provided for consistency - the call itself
    // is proxied through the current playlist entry
    unsigned long GetTrackTime() const;
    unsigned long GetTrackLength() const;
    
private:
    CMediaPlayerImp();
    ~CMediaPlayerImp();

    static CMediaPlayerImp* s_pSingleton;

    // Thread entry routines
    static void MediaPlayerEntryPoint( cyg_addrword_t data );
    void MediaPlayerThread();

    // Routine to clean up playstream objects
    //    void CleanupPlaystream( StreamNode_t* pPlaystream );

    // Routine to stop playback
    void StopPlayback( );
    void PausePlayback( bool bPassEvent = true );

    //
    // Data
    //

    //
    // Pointer to the system event queue
    //
    CEventQueue* m_pEventQueue;

    // Pointer to the function used to create metadata.
    FNCreateMetadata* m_pfnCreateMetadata;

    // Pointer to the function used to create input streams for playback.
    FNCreateInputStream* m_pfnCreateInputStream;

    // Pointer to the function used to create play streams
    FNCreatePlayStream* m_pfnCreatePlayStream;
    
    // Cached metadata pointer to use in SetSong (to cut down on unnecessary allocation).
    //    IMetadata*  m_pMetadata;
    
    // Info about the current track
    //  the stream_info_t has all the information about the stream just out of the
    //  decode step; filter_stream_info_t has all the information for the stream at the
    //  end of the filter chain
    //    stream_info_t* m_pStreamInfo;
    //    filter_stream_info_t* m_pFilterStreamInfo;
    
    //    IPlaylistEntry* m_pCurrentSong;

    // Track progress data
    //    unsigned long m_ulTrackTime;
    //    unsigned long m_ulLastTrackTime;
    
    // Default play settings
    CPlayStreamSettings* m_pSettings;

    cyg_mutex_t m_PlayStreamMutex;
    CPlayStream *m_pCurrentStream;
    CPlayStream *m_pNextStream;

    // Codec pool data
    unsigned int* m_pCodecPoolAddress;
    unsigned int m_iCodecPoolSize;

    // SRC Blending
    int m_iSRCBlendLevel;
    
    // If the current codec was allocated from the heap, the following
    // flag will be true
    bool m_bCodecFromHeap;
    
    // Current play state
    CMediaPlayer::PlayState m_ePlayState;
    
    //
    // Internal data
    //

    // Thread handle, data, and stack
    cyg_handle_t m_hDecoderThread;
    cyg_thread   m_DecoderThreadData;
    char         m_DecoderThreadStack[ DECODER_THREAD_STACK_SIZE ];

    // Thread control flag
    cyg_flag_t m_Flag;

    // Flag settings
    //  External->Internal commands
    static const cyg_flag_value_t FLAG_STOP             = 0x0001;
    static const cyg_flag_value_t FLAG_PLAY             = 0x0002;
    static const cyg_flag_value_t FLAG_TERMINATE        = 0x0004;
    
    //  State and internal->external synch
    static const cyg_flag_value_t FLAG_STOPPED          = 0x0010;
    static const cyg_flag_value_t FLAG_PLAYING          = 0x0020;
    static const cyg_flag_value_t FLAG_TERMINATED       = 0x0040;
    static const cyg_flag_value_t FLAG_ERROR            = 0x0080;

    //  Wakeup conditions internal to the thread
    static const cyg_flag_value_t FLAG_INTERNAL_WAIT_ALL   = (FLAG_PLAY | FLAG_STOP | FLAG_TERMINATE | FLAG_PLAYING);

    //  Wakeup conditions external to the thread
    static const cyg_flag_value_t FLAG_EXTERNAL_WAIT_PLAY  = (FLAG_PLAYING | FLAG_ERROR);
    static const cyg_flag_value_t FLAG_EXTERNAL_WAIT_STOP  = (FLAG_STOPPED | FLAG_ERROR);

    // Helper routines to simplify flag access
    inline cyg_flag_value_t CheckBits( cyg_flag_value_t flg )
        {   return cyg_flag_poll( &m_Flag, flg, CYG_FLAG_WAITMODE_OR );   }
    inline cyg_flag_value_t WaitForBits( cyg_flag_value_t flg )
        {   return cyg_flag_wait( &m_Flag, flg, CYG_FLAG_WAITMODE_OR );   }
    inline cyg_flag_value_t TimedWaitForBits( cyg_flag_value_t flg, cyg_tick_count_t abstime )
        {   return cyg_flag_timed_wait( &m_Flag, flg, CYG_FLAG_WAITMODE_OR, abstime );   }
    inline void SetBits( cyg_flag_value_t flg )
        {   cyg_flag_setbits( &m_Flag, flg );     }
    inline void ClearBits( cyg_flag_value_t flg )
        {   cyg_flag_maskbits( &m_Flag, ~flg );   }
    
    friend class CPlayStream;
};


#endif  // MEDIAPLAYERIMP_H_
