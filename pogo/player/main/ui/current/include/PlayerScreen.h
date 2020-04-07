//........................................................................................
//........................................................................................
//.. File Name: PlayerScreen.h															..
//.. Date: 09/04/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the definition of the CPlayerScreen class			..
//.. Usage: This class is derived from the CScreen class, and							..
//..		 contains the various windows that make up the main play screen display.	..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/04/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#ifndef PLAYERSCREEN_H_
#define PLAYERSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/ui/Screen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/ToneMenuScreen.h>
#include <main/playlist/pogoplaylist/PogoPlaylist.h>

#define METADATA_STR_SIZE 256 * 2

// forward decl
class IMediaContentRecord;
class IMetadata;
class CPlayManager;
class IContentManager;
struct set_track_message_s;
typedef struct set_track_message_s set_track_message_t;

class CPlayerScreen : public CScreen
{
public:
	// an enumeration to aid indexing into the Control_Symbol[] array
	typedef enum ControlSymbol { FAST_FORWARD = 0, REWIND, PREVIOUS_TRACK, PLAY, PAUSE, NEXT_TRACK, STOP, RECORD };

	// an enumeration for the menu items
	typedef enum MenuItem { SET = 0, ARTIST, ALBUM, TRACK, SETUP };

	static CScreen* GetPlayerScreen();
    static void Destroy() {
		if (s_pPlayerScreen)
			delete s_pPlayerScreen;
		s_pPlayerScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	void Draw();

	// Hides any visible menu screens.
	void HideMenus();

	// Tells the interface to pause any cycle-chomping activity while the buffers are read.
	void NotifyBufferReadBegin();
	// Tells the interface that the buffer read is over, so let the chomping begin!
	void NotifyBufferReadEnd();

	void SetTrack(set_track_message_t* pStm);
    void ClearTrack();

	//void SetTrackNumber(unsigned int uTrack);
	void SetVolume(unsigned int uVolume);
	void SetTime(int uTime);
	void SetBattery(unsigned int uLevel);
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);
	void SetPlayModeIcon(int iPlayMode);
	void SetEqualizerMode(tEqualizerMode eEqualizerMode);
	void SetTimeViewMode(CTimeMenuScreen::TimeViewMode eTimeViewMode);
	void SetControlSymbol(ControlSymbol eControlSymbol);
	void SetLockIcon(bool on);
	void SetSetText(const TCHAR* szText);
	void SetArtistText(const TCHAR* szText);
	void SetAlbumText(const TCHAR* szText);
	void SetTrackText(const TCHAR* szText);
	void SetBitrateText(const TCHAR* szText);
	void SetCharging(bool charging);
    void UpdateConstraintDots(bool bSet, bool bArtist, bool bAlbum, bool bTrack);
    void DecayAlbumTimeDisplay();
    // set up the track start time according to the time display mode.
    void InitTrackStartTime();
    int RestoreFromRegistry();
    int SaveToRegistry();
    void SetPlaylistMode(int iPlayMode);
    void UpdateTrackDuration();
    // flutter the control symbol between two values
    void StartFlutterTimer(ControlSymbol eOne, ControlSymbol eTwo);
    void KillFlutterTimer();
    bool IsRecording();
    void ExitRecordingSession();
	// Query the play manager for its current play state
	// and display the appropriate control symbol.
	void SynchControlSymbol();
    // notify the user that a clip has occured
    void HandleClipDetected();

private:

    // playback event handlers
    SIGNED HandleKeyRefreshContent(const PegMessage &Mesg);
    SIGNED HandleTrackProgress(const PegMessage &Mesg);
    SIGNED HandleTimerCheckBattery(const PegMessage &Mesg);
    SIGNED HandleTimerCheckCharging(const PegMessage &Mesg);
    SIGNED HandleTimerFlutterCtrlSymbol(const PegMessage &Mesg);
    SIGNED HandleTimerScrollEnd(const PegMessage &Mesg);
    SIGNED HandleTimerScrollTitle(const PegMessage &Mesg);
    SIGNED HandleMsgNewTrack(const PegMessage &Mesg);
    SIGNED HandleMsgStop(const PegMessage &Mesg);
    SIGNED HandleKeyReleasePrevious(const PegMessage &Mesg);
    SIGNED HandleKeyReleaseMenu(const PegMessage &Mesg);
    SIGNED HandleKeyReleaseNext(const PegMessage &Mesg);
    SIGNED HandleKeyNext(const PegMessage &Mesg);
    SIGNED HandleKeyPlayPause(const PegMessage &Mesg);
    SIGNED HandleKeyPrevious(const PegMessage &Mesg);
    SIGNED HandleKeyUp(const PegMessage &Mesg);
    SIGNED HandleKeyDown(const PegMessage &Mesg);
    SIGNED HandleKeyMenu(const PegMessage &Mesg);
    SIGNED HandleKeyDialIn(const PegMessage &Mesg);
    SIGNED HandleKeyDialUp(const PegMessage &Mesg);
    SIGNED HandleKeyDialDown(const PegMessage &Mesg);
    // recording event handlers
    SIGNED HandleKeyStopRecording(const PegMessage &Mesg);
    SIGNED HandleKeyUpRecording(const PegMessage &Mesg);
    SIGNED HandleKeyDownRecording(const PegMessage &Mesg);
    SIGNED HandleKeyDialUpRecording(const PegMessage &Mesg);
    SIGNED HandleKeyDialDownRecording(const PegMessage &Mesg);
    // event dispatchers
    SIGNED DispatchRecordingMessage(const PegMessage &Mesg);
    SIGNED DispatchPlaybackMessage(const PegMessage &Mesg);

    void SetPlayModeIconByPlaylistMode(CPogoPlaylist::PogoPlaylistMode ePLMode);
    void TogglePlayPause();
    // volume control
    void IncrementVolume();
    void DecrementVolume();
    // gain control
    void SetGain(unsigned int uGain);
    void IncrementGain();
    void DecrementGain();
    // registry management
    void SaveState(void* buf, int len);
    void RestoreState(void* buf, int len);
    int GetStateSize();
    void InitRegistry();

    CPlayerScreen(CScreen* pParent);
	virtual ~CPlayerScreen();

	void BuildScreen();

	// Checks to see if any of the strings need scrolling and configures the peg alarms to do so.
	void SynchTextScrolling();

	// This function will scroll all text fields in unison one letter.
	// Returns true if some scrolling happend.
	bool ScrollTextFields();

	// Scrolls from menu items
	void ScrollMenu(int iMenuIndex);

	// Select or deselect a menu item
	void SelectMenuItem(MenuItem eMenuIndex);

	// Sets normal play mode in the playlist, play screen, and play mode menu.
	void SetNormalPlayMode();

    // if the Set constraint dot isn't set, then update the set text to the current genre.
    void UpdateSetText();

    // anchor the time to display at the start of the track, before which the user can't seek, etc.
    void SetTrackStartTime(int iTrackStartTime);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(MenuItem eMenuIndex);
    void SetProxiedTextField(TCHAR* szSource, PegString* pString, TCHAR* szProxy, TCHAR* szScratch);
    void SetMetadataString(IMetadata* metadata, int md_type, PegString* pString, TCHAR* szProxy, TCHAR* szScratch);
	PegString *m_pSetString;
	PegString *m_pArtistString;
	PegString *m_pAlbumString;
	PegString *m_pTrackString;
	PegString *m_pSetupString;
	PegString *m_pSetTextString;
	PegString *m_pArtistTextString;
	PegString *m_pAlbumTextString;
	PegString *m_pTrackTextString;
	PegString *m_pVolumeTextString;
	PegString *m_pTimeTextString;
	PegString *m_pBitrateTextString;

	TCHAR	m_szSetTextString[METADATA_STR_SIZE];
	TCHAR	m_szArtistTextString[METADATA_STR_SIZE];
	TCHAR	m_szAlbumTextString[METADATA_STR_SIZE];
	TCHAR	m_szTrackTextString[METADATA_STR_SIZE];
	TCHAR	m_szBitrateTextString[10];

	PegIcon	*m_aryArrowIcon[4];
	PegIcon	*m_pBatteryIcon;
	PegIcon *m_pBatteryFullIcon;
	PegIcon *m_pVolumeBarIcon;
	PegIcon *m_pVolumeBarFullIcon;
	PegIcon *m_pControlSymbolIcon;
	PegIcon *m_pPlayModeIcon;
	PegIcon *m_pEqualizerModeIcon;
	PegIcon *m_pLockIcon;
	PegIcon *m_pScreenBarIcon;
	PegIcon *m_pScreenVerticalDottedBarIcon;
	PegIcon *m_pTimeByAlbumIcon;

	int		m_iBatteryLevel;
	int		m_iSec1s;
	int		m_iSec10s;
	int		m_iMin1s;
	int		m_iMin10s;
	int		m_iHr1s;
	int		m_iHr10s;
	int		m_iTrackTime;
	unsigned int		m_uTrackDuration;
    int     m_iTrackStartTime;          // What time is displayed at the start of the current track?

	bool	m_bScanningForwardPrimed;
	bool	m_bScanningForward;
	bool	m_bScanningBackwardPrimed;
	bool	m_bScanningBackward;
	int     m_iScanTime;
	bool	m_bIgnoreNextKeyup;

	bool	m_bScrollingTitle;			// Should the title be scrolled?
	bool	m_bInitialScrollPause;		// Are we waiting on the initial scrolling timeout?

	bool	m_bCharging;				// True if the battery charging animation is enabled.

	bool	m_bMenuMode;
	int		m_iMenuIndex;				// Currently selected menu item

	bool	m_aryMenuItemSelected[4];		// Is the menu item selected?

	ControlSymbol					m_eControlSymbol;
    ControlSymbol                   m_eFlutterCtrlSymbolOne;
    ControlSymbol                   m_eFlutterCtrlSymbolTwo;
	tEqualizerMode		            m_eEqualizerMode;
	CTimeMenuScreen::TimeViewMode	m_eTimeViewMode;
	MenuItem						m_eMenuIndex;

	PegRect			m_aryMenuItemsHighlightRect[5];

	static const int sc_iBatteryCheckInterval;
	static const int sc_iBatteryChargingCheckInterval;

	static const int sc_iScrollStartInterval;
	static const int sc_iScrollEndInterval;
	static const int sc_iScrollContinueInterval;

	static const int sc_iScanSpeedupInterval;	// Amount of ticks to wait before moving up to next scan speed.
	static const int sc_aryScanSpeeds[4];		// Array of possible scan speeds.
	static const int sc_iScanMaxIndex;		// Maximum index into the array of scan speeds.
	int m_iScanCount;	// Current number of ticks spent at the current scan rate.
	int m_iScanIndex;	// Current index into the array of scan rates.

	// Amount of ticks to wait before sleeping the drive after a next/previous track button press.
	static const int sc_iTrackChangeSleepInterval;
	bool	m_bWaitingForTrackChangeSleep;
    bool m_bConfigured;

    // recording support members
    bool m_bRecording;
    short m_cMenuHolds;

	static CPlayerScreen* s_pPlayerScreen;
};

#endif  // PLAYERSCREEN_H_
