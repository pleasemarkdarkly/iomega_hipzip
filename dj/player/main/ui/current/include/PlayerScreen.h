// IPAddressString.h: This class is derived from the CScreen class, and
//  contains the various windows that make up the main play screen display
// danb@iobjects.com 09/04/2001
// (c) Interactive Objects

#ifndef PLAYERSCREEN_H_
#define PLAYERSCREEN_H_

//#include <gui/peg/peg.hpp>
#include <main/ui/Screen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/SystemMessageString.h>
#include <playlist/common/Playlist.h>

#include <main/ui/PlaybackEventHandler.h>

#ifdef LINE_RECORDER_ENABLED
#include <main/ui/LineInEventHandler.h>
#include <main/ui/LineRecEventHandler.h>
#endif

#define METADATA_STR_SIZE 256 * 2

// forward decl
class IMediaContentRecord;
class IMetadata;
class CPlayManager;
class IContentManager;

struct set_track_message_s;
typedef struct set_track_message_s set_track_message_t;

// track modal responsibility for responding to events
typedef enum tPSEventHandlerMode { ePSPlaybackEvents = 0, ePSLineRecEvents, ePSLineInEvents };

class CPlayerScreen : public CScreen
{
public:
    // an enumeration to aid indexing into the Control_Symbol[] array
    typedef enum ControlSymbol { FAST_FORWARD = 0, REWIND, PREVIOUS_TRACK, PLAY, PAUSE, NEXT_TRACK, STOP, RECORD };
    
    static CPlayerScreen* GetPlayerScreen();
    static void Destroy() {
        if (s_pPlayerScreen)
            delete s_pPlayerScreen;
        s_pPlayerScreen = 0;
    }
    
    SIGNED Message(const PegMessage &Mesg);
    
    void Draw();
    void ForceRedraw();
    void SetViewMode(CDJPlayerState::EUIViewMode eViewMode, bool bKeyPress);
    
    // Hides any visible menu screens.
    void HideMenus();
    
    void StopScrollingText();
    void ResumeScrollingText();
    
    void PowerOn();
    void PowerOff();
    
    void SetTrack(set_track_message_t* pSTM);
    void SetTrackInfo(IPlaylistEntry* pEntry);
    void SetTrackInfo(IMetadata* pMetadata, IPlaylistEntry* pEntry = NULL);
    void ClearTrack();
    // Stops ripping and recording.
    // Returns true if the player was actually ripping or recording.
    bool StopRipping();
    // Stops playback and resets the current song.
    // If bForceStop is true, then CPlayManager::Stop is called even if no track is currently being played.
    // bForceStop is used when ripping fml tracks, forcing the song to be reset with buffering turned on.
    void StopPlayback(bool bForceStop = false);
    
    void DisplayInsertCDScreen();
    void DisplayNoContentScreen();
    void DisplayNoStationScreen();
    void DisplaySelectTracksScreen();
    void NotifyCDInserted();
    void NotifyCDRemoved();
    void NotifyCDTrayOpened();
    void NotifyCDTrayClosed();
    void NotifyMusicSourceChanged(CDJPlayerState::ESourceMode eSource);
    
    bool SetTime(int uTime, bool bForceRedraw = false);    // Returns true if the time set is different from the current time.
    void SetTimeViewMode(CTimeMenuScreen::TimeViewMode eTimeViewMode);
    CTimeMenuScreen::TimeViewMode GetTimeViewMode();
    void SetControlSymbol(ControlSymbol eControlSymbol);
    void SetArtistText(const TCHAR* szText, bool bDraw = false);
    void SetTrackText(const TCHAR* szText, bool bDraw = false);
    // timeout length is in peg ticks.  no value, or zero, means there's no timeout, and the message stays until it's changed.
    void SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void ResetProgressBar(int iProgress = 0, int iTotal = 0);
    void UpdateProgressBar(int iProgress = 0);
    // set up the track start time according to the time display mode.
    void InitTrackStartTime();
    int RestoreFromRegistry();
    int SaveToRegistry();
    void SetPlaylistMode(int iPlayMode);
    // remote activity light.  this will draw on top of all screens
    void DrawIRActivity();
    void SetPlayModeTextByPlaylistMode(IPlaylist::PlaylistMode ePLMode);
    bool Configured();
    // Set event handling mode
    void SetEventHandlerMode(tPSEventHandlerMode eMode);
    // Get event handling mode
    tPSEventHandlerMode GetEventHandlerMode();
    // Be smart about not doing unecessary SetSong() calls on next and previous track button presses
    void DoTrackChange();
    void DebounceNextTrack();
    void DebouncePreviousTrack();
    // tell the user that their recording was stopped due to low space.
    void NotifyUserRecordingCutoff();
    // show the info screen with the current tracks info
    void ShowCurrentTrackInfo();
    void RefreshCurrentTrackMetadata();
    // Mute audio output
    void MutePlayback();
    // Toggle through all the zoom modes
    void ToggleZoom();
    // Remove what is currently playing.  Uses DebounceNextTrack() to debounce SetTrack() calls.
    void RemoveCurrentPlaylistEntry();
    // Query the player for its current state
    // and display the appropriate status message is the message text area.
    void SynchStatusMessage();
    
    // Enable/disable use of the progress bar for prebuffer status. needed for radio
    void EnablePrebufferProgress();
    void DisablePrebufferProgress();

    CLineRecEventHandler* GetLineRecEventHandler();
    CLineInEventHandler* GetLineInEventHandler();
    CPlaybackEventHandler* GetPlaybackEventHandler();
    
private:
    static void PrebufferProgressCallback( int PercentDone );
    void PrebufferProgressCB( int PercentDone );
    
    void SetTimeText(const TCHAR* szText);
    void TogglePlayPause();
    void TogglePause();
    void ToggleCDRip();
    void EndScanOrNextTrack();
    void EndScanOrPrevTrack();
    
    void SaveState(void* buf, int len);
    void RestoreState(void* buf, int len);
    int GetStateSize();
    void InitRegistry();
    
    CPlayerScreen(CScreen* pParent);
    virtual ~CPlayerScreen();
    
    void BuildScreen();
    void SynchWithViewMode();
    
    // Query the player manager and recording manager for its current play state
    // and display the appropriate control symbol.
    void SynchControlSymbol();
    
    
    // Checks to see if any of the strings need scrolling and configures the peg alarms to do so.
    void SynchTextScrolling();
    
    // This function will scroll all text fields in unison one letter.
    // Returns true if some scrolling happend.
    bool ScrollTextFields();
    
    // Scrolls from menu items
    void ScrollMenu(int iMenuIndex);
    
    // anchor the time to display at the start of the track, before which the user can't seek, etc.
    void SetTrackStartTime(int iTrackStartTime);

    // retrieves the metadata string for the md_type and metadata pointer passed
    // returns 0 if it was unsuccessful
    TCHAR* GetMetadataString(IMetadata* pMetadata, int md_type);
    
    // Called in Draw() and used to calculate the drawing of the progress bar
    void DrawProgressBar();
    
    void ScanForward();
    void ScanBackward();
    void EnterQuickBrowseMenu(int iCurrentTrackOffset = 0);
    void EnterSavePlaylistScreen();
    void SynchSymbols();
    void SynchSymbolsForStop();
    void SetTimerForScrollEnd();
    void PerformFunkyIRResponse();
    void UpdateTrackProgress(const PegMessage &Mesg);
    
    
    // Event Handlers
    SIGNED HandleTimerScrollTitle(const PegMessage& Mesg);
    SIGNED HandleTimerScrollEnd(const PegMessage& Mesg);
    SIGNED HandleTrackProgress(const PegMessage& Mesg);
    
    
    PegString *m_pRepeatPlayModeTextString;
    PegString *m_pRandomPlayModeTextString;
    PegString *m_pArtistTextString;
    PegString *m_pTrackTextString;
    PegString *m_pTimeTextString;
    PegString *m_pZoomTextString;
    PegString *m_pZoomTimeTextString;
    
    CSystemMessageString *m_pMessageTextString;
    
    PegIcon *m_pControlSymbolIcon;
    PegIcon *m_pControlSymbolLargeIcon;
    PegIcon *m_pScreenHorizontalDottedBarIcon;
    PegIcon *m_pTimeByAlbumIcon;
    
    PegRect m_ProgressBarRect;
    
    PegWindow *m_pBlankWindow;
    
    int		m_iSec1s;
    int		m_iSec10s;
    int		m_iMin1s;
    int		m_iMin10s;
    int		m_iHr1s;
    int		m_iHr10s;
    int		m_iTrackTime;
    int     m_iTrackStartTime;          // What time is displayed at the start of the current track?
    int		m_iTrackDuration;
    int     m_iProgressBarTotal;
    
    bool	m_bScanningForwardPrimed;
    bool	m_bScanningForward;
    bool	m_bScanningBackwardPrimed;
    bool	m_bScanningBackward;
    int     m_iScanTime;
    bool	m_bIgnoreNextKeyup;
    bool    m_bIgnoreScan;
    
    bool	m_bScrollingText;			// Are we scrolling any text on the screen?
    bool	m_bInitialScrollPause;		// Are we waiting on the initial scrolling timeout?
    
    bool    m_bDebounceTrackChange;     // Should the next track change request be debounced?
    bool    m_bBacktrackIfNeeded;       // When debouncing NT or PT, which way should we go if a song fails?
    
    ControlSymbol					m_eControlSymbol;
    CTimeMenuScreen::TimeViewMode	m_eTimeViewMode;
    
    // Number of peg ticks to wait before firing a set song
    // Used to debounce NT & PT
    static const int sc_iShortTrackChangeInterval;  
    static const int sc_iLongTrackChangeInterval;   
    
    static const int sc_iScanSpeedupInterval;	// Amount of ticks to wait before moving up to next scan speed.
    static const int sc_aryScanSpeeds[4];		// Array of possible scan speeds.
    static const int sc_iScanMaxIndex;		// Maximum index into the array of scan speeds.
    int m_iScanCount;	// Current number of ticks spent at the current scan rate.
    int m_iScanIndex;	// Current index into the array of scan rates.
    bool    m_bSeeked; // Has the user seeked in the current track?
    
    // Amount of ticks to wait before sleeping the drive after a next/previous track button press.
    static const int sc_iTrackChangeSleepInterval;
    bool m_bDoingTrackChange;
    bool m_bConfigured;
    
    // Time for the ir activity light
    static const int sc_iIREndInterval;
    
    static CPlayerScreen* s_pPlayerScreen;
    
    // support members for line recording
    tPSEventHandlerMode m_eEventHandlerMode;
    CPlaybackEventHandler m_PlaybackEventHandler;
    friend class CPlaybackEventHandler;
    friend class CCDTriageScreen;
#ifdef LINE_RECORDER_ENABLED
    friend class CLineInEventHandler;
    friend class CLineRecEventHandler;
    CLineInEventHandler m_LineInEventHandler;
    CLineRecEventHandler m_LineRecEventHandler;
#endif
};

#endif  // PLAYERSCREEN_H_
