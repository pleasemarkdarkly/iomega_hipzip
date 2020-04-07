//
// DJPlayerState.h
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//

#ifndef DJPLAYERSTATE_H_
#define DJPLAYERSTATE_H_

#include <util/timer/Timer.h>  // timer_handle_t
#include <util/eresult/eresult.h>
#include <main/iml/iml/IML.h>


// fdecl
class CCDDataSource;
class CFatDataSource;
class IMediaContentRecord;
class CNetDataSource;
class CPlayManager;
class IUserInterface;
typedef struct media_record_info_s media_record_info_t;

class CDJPlayerState
{
public:

    //! Returns a pointer to the global DJ state instance.
    static CDJPlayerState* GetInstance();

    //! Destroy the singleton global DJ state.
    static void Destroy();

    //! Set/get the CD data source.
    void SetCDDataSource(CCDDataSource* pCDDataSource)
        { m_pCDDS = pCDDataSource; }
    CCDDataSource* GetCDDataSource()
        { return m_pCDDS; }

    //! Set/get the FAT data source.
    void SetFatDataSource(CFatDataSource* pFatDataSource)
        { m_pFatDS = pFatDataSource; }
    CFatDataSource* GetFatDataSource()
        { return m_pFatDS; }

    //! Set/get the net data source.
    void SetNetDataSource(CNetDataSource* pNetDataSource)
        { m_pNetDS = pNetDataSource; }
    CNetDataSource* GetNetDataSource()
        { return m_pNetDS; }

    //! Set/get a pointer to the currently selected FML
    void SetCurrentIML(CIML* pIML)
        { m_pCurrentIML = pIML; }
    CIML* GetCurrentIML()
        { return m_pCurrentIML; }

    //! If the current source is an FML and it sends a byebye then store the FML's UDN so
    //! we can make it the current source again if it comes back.
    void SetIMLToRestore(const char* szUDN);
    char* GetIMLToRestore() const
        { return m_szIMLUDN; }
    //! If the user manually changes source while the FML is gone, then forget about it.
    void ClearIMLToRestore();

    //! Save/load player state.
    void SaveState(bool bSaveRegistry = true, bool bSaveCurrentPlaylist = true, bool bSaveContentDB = true);
    void LoadState();

    //! Save the current playlist.
    //! Saves the registry as a side effect.
    ERESULT SaveCurrentPlaylist();
    //! Load the current playlist.
    ERESULT LoadCurrentPlaylist();

    //! Initialize the UI based on the current settings,
    //! call LoadState first
    void Initialize();

    //! Save the registry.
    void SaveRegistry();

    typedef enum ESourceMode { CD = 0, HD, FML, LINE_IN };
    //! Get the input source state.
    ESourceMode GetSource() const
        { return m_eSource; }
    //! Set the input source state.
    //! Returns true if the playlist is cleared during transition.
    bool SetSource(ESourceMode eSource, bool bSendSourceChangedEvent = true, bool bSendPlaylistClearedEvent = true);

    //! Get the current playlist entry source
    ESourceMode GetCurrentSongSource();

    //! Tells the state manager that a CD has started or finished the process
    //! of mounting and metadata retrieval.
    void SetScanningCD(bool bMounting)
        { m_bScanningCD = bMounting; }

    //! Returns true if the DJ is currently mounting a CD and getting its metadata.
    bool IsScanningCD() const
        { return m_bScanningCD; }

    //! Tells the state manager that the HD has started or finished the process
    //! of scanning and metadata retrieval.
    void SetScanningHD(bool bScanning)
        { m_bScanningHD = bScanning; }

    //! Returns true if the DJ is currently mounting a CD and getting its metadata.
    bool IsScanningHD() const
        { return m_bScanningHD; }
    

    //! Set/get the CD state.
    typedef enum ECDState
    {
        NONE = 0,   //!< No CD in the drive.
        AUDIO,      //!< Audio CD in the drive
        DATA        //!< Data CD in the drive
    };
    ECDState GetCDState() const
        { return m_eCDState; }
    void SetCDState(ECDState eCDState)
        { m_eCDState = eCDState; }

    //! Clear the play manager's playlist, fill it with all the content on the CD, and set the first track.
    //! Returns true if a track was set in the media player, false otherwise.
    bool CreateCDPlaylist(bool bSendPlaylistClearedEvent = true);

	// gets a unique identifier for playlist generation
	unsigned int GetUniquePlaylistID();

	// returns player version/build number 
	int CDJPlayerState::GetPlayerVersion() const;

    //! Set/get the power state.
    typedef enum EPowerState
    {
        POWER_ON = 0,   //!< On, interactive, etc.
        SOFT_POWER_OFF, //!< Off, but with idle encoding enabled
        HARD_POWER_OFF  //!< Off, with no other threads running.  Ready to be unplugged.
    };
    EPowerState GetPowerState() const
        { return m_ePowerState; }
    void SetPowerState(EPowerState ePowerState);

	static void SetDrivesPower(bool on);
    static void SafeReset( bool bPowerOffFirst = true );

    void NotifyDeviceActive();

    //! Get the encoding state
    typedef enum EEncodingState
        {
            NOT_ENCODING = 0, //!< Not currently doing anything
            PAUSED_ENCODING,  //!< Encoder is paused with a track
            ENCODING          //!< Encoder is processing a track
        };
    EEncodingState GetEncodingState() const
        { return m_eEncodingState;    }
    
    //! Get the next CD session index.
    int GetNextCDSession()
        { return m_iCDSessionIndex++; }

    //! Get the current bitrate that the encoder will use.
    int GetEncodeBitrate()
        { return m_iEncodeBitrate; }

    //! Set the current bitrate for the encoder.
    void SetEncodeBitrate(int iBitrate, bool bSave = true);

    //! The available UI view modes
    typedef enum EUIViewMode
    {
        NORMAL = 0,             //!< The standard ui
        ZOOM,                   //!< Zoom mode
        ZOOM_EXT
    };

    //! Get the current UI view state
    EUIViewMode GetUIViewMode() const
        { return m_eUIViewMode; }

    //! Set the current UI view state
    void SetUIViewMode(EUIViewMode eUIViewMode, bool bKeyPress = false);

    //! Set the pointer the the user interface object
    void SetUserInterface( IUserInterface* pUI );
    IUserInterface* GetUserInterface( void ) 
        {  return m_pUserInterface; }
    
    //! Given a media record this function either searhes the local content
    //! manager for a matching media content object or genrates a new object
    //! for net content.
    //! If a record can't be found or generated, then 0 is returned.
    static IMediaContentRecord* GetMediaContentRecord(media_record_info_t* pMediaRecord);

    //! Get/Set the UI user settings
    bool GetUIEjectCDAfterRip()
        { return m_bUIEjectCDAfterRip; }
    void SetUIEjectCDAfterRip(bool bUIEjectCDAfterRip);

    bool GetUIEnableExtChars()
        { return m_bUIEnableExtChars; }
    void SetUIEnableExtChars(bool bUIEnableExtChars);

    bool GetUIEnableWebControl()
        { return m_bUIEnableWebControl; }
    void SetUIEnableWebControl(bool bUIEnableWebControl);

    bool GetUIShowTrackNumInTitle()
        { return m_bUIShowTrackNumInTitle; }
    void SetUIShowTrackNumInTitle(bool bUIShowTrackNumInTitle);

    bool GetUIShowAlbumWithArtist()
        { return m_bUIShowAlbumWithArtist; }
    void SetUIShowAlbumWithArtist(bool bUIShowAlbumWithArtist);
    
    bool GetUIPlayCDWhenInserted()
        { return m_bUIPlayCDWhenInserted; }
    void SetUIPlayCDWhenInserted(bool bUIPlayCDWhenInserted);
    
    //! The available UI text scroll speed
    typedef enum EUITextScrollSpeed
    {
        FAST = 0,   //!< Fast
        SLOW,       //!< Slow (default)
        OFF         //!< Off
    };
    //! Get/Set the UI text scroll speed
    EUITextScrollSpeed GetUITextScrollSpeed() const
        { return m_eUITextScrollSpeed; }
    void SetUITextScrollSpeed(EUITextScrollSpeed eUITextScrollSpeed);

    //! Get/Set the UI enable record led
    bool GetUIEnableRecordLED() const
        { return m_bUIEnableRecordLED; }
    void SetUIEnableRecordLED(bool bUIEnableRecordLED);

    //! The available UI brightness levels
    typedef enum EUIBrightness
    {
        BRIGHT = 0,   //!< Bright (default)
        DIM           //!< Dim
    };
    //! Get/Set the UI LCD brightness
    EUIBrightness GetUILCDBrightness() const
        { return m_eUILCDBrightness; }
    void SetUILCDBrightness(EUIBrightness eUILCDBrightness);

    void ShutdownSystem();

private:

    CDJPlayerState();
    ~CDJPlayerState();

    //! Save the current playlist without saving the registry.
    ERESULT SaveCurrentPlaylistInternal();

    void StartIdleEncoding( bool bEnableLED = true );
    void PauseIdleEncoding();
    void StopIdleEncoding();
    
    static void IdleTimerCallback(void* pObj);
    void IdleTimerExpired();
    timer_handle_t m_TimerHandle;

    static void ShutdownSystemTimerCallback(void* pObj);
    timer_handle_t m_SystemShutdownTimerHandle;
    
    static CDJPlayerState* s_pSingleton;   // The global singleton DJ state.

    // Pointers to the data sources required by the system.
    CCDDataSource*  m_pCDDS;
    CFatDataSource* m_pFatDS;
    CNetDataSource* m_pNetDS;

    // Pointer to the current FML
    CIML* m_pCurrentIML;
    // Stores the UDN of an FML if it disappears when it's the current source.
    char*   m_szIMLUDN;

    // Cached pointers to frequently used singletons.
    CPlayManager*   m_pPlayManager;
    IUserInterface* m_pUserInterface;

    ESourceMode m_eSource;      // What source is our content coming from?

    EPowerState m_ePowerState;  // On, soft off, hard off

    bool        m_bScanningCD;  // True if a CD has been inserted and we're still processing it.
    ECDState    m_eCDState;     // None, audio, data

    bool        m_bScanningHD;  // True if the HD is being scanned for content.

    bool        m_bSystemIsShuttingDown;  // True if the system is being shut down

    EUIViewMode m_eUIViewMode;  // What view mode/state is the UI in?

    EEncodingState m_eEncodingState; // Encoding, paused, stopped
    
    int     m_iCDSessionIndex;  // An index to assign session numbers to CDs with unknown metadata.

    int     m_iEncodeBitrate; // the bitrate that gets 

	unsigned int m_uiPlaylistCounter; // unique playlist name counter

    unsigned int m_uiPlaylistIndex; // Current index in the saved system playlist
    
    // Returns the number of bytes this class uses in the registry.
    int GetStateSize() const;

    // Some User Settings
    bool m_bUIEjectCDAfterRip; 
    bool m_bUIEnableExtChars; 
    bool m_bUIEnableWebControl; 
    bool m_bUIShowTrackNumInTitle; 
    bool m_bUIShowAlbumWithArtist;
    bool m_bUIPlayCDWhenInserted;
    EUITextScrollSpeed m_eUITextScrollSpeed;
    bool m_bUIEnableRecordLED;
    EUIBrightness m_eUILCDBrightness;
};

#endif  // DJPLAYERSTATE_H_
