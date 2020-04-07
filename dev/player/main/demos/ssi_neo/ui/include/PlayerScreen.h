//........................................................................................
//........................................................................................
//.. File Name: PlayerScreen.h															..
//.. Date: 09/04/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the definition of the CPlayerScreen class			..
//.. Usage: This class is derived from the CScreen class, and							..
//..		 contains the various windows that make up the main play screen display.	..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/04/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef PLAYERSCREEN_H_
#define PLAYERSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/demos/ssi_neo/ui/Screen.h>
//#include "Animation.h"
//#include "Playlist.h"
//#include "MediaPlayer.h"
#include <main/demos/ssi_neo/ui/DACManager.h>
#include <main/demos/ssi_neo/ui/OnScreenMenu.h>

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

	static CScreen* GetPlayerScreen();
    static void Destroy() {
		if (s_pPlayerScreen)
			delete s_pPlayerScreen;
		s_pPlayerScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	void Draw();

	// Hides any visible menu screens.
	void HideMenu();

	// Tells the interface to pause any cycle-chomping activity while the buffers are read.
	void NotifyBufferReadBegin();
	// Tells the interface that the buffer read is over, so let the chomping begin!
	void NotifyBufferReadEnd();

	// Tells the play screen that the track is about to change.
	void NotifyTrackAboutToChange(bool bShowResult = false);

	// Asks the play screen to update the current track's track number, in case it changed.
	// This is called when the user leaves the play list screen or play list manager screen.
	void UpdateTrackNumber();

	void SetTrack(set_track_message_t* ptim);

	void SetTrackNumber(unsigned int uTrack);
	void SetVolume(unsigned int uVolume);
	void SetTime(unsigned int uTime);
	void SetBattery(unsigned int uLevel);
	//void SetPlayMode(IPlaylist::PlaylistMode ePlaylistMode);
	void SetPlayMode(int iPlayMode);
	void SetControlSymbol(ControlSymbol eControlSymbol);
	void SetLockIcon(bool on);
	void SetText1(const TCHAR* szText);
	void SetText2(const TCHAR* szText);
	void SetText3(const TCHAR* szText);
	void SetCharging(bool charging);

private:

	CPlayerScreen(CScreen* pParent);
	virtual ~CPlayerScreen();

	void BuildScreen();
	// Query the media player for its current play state
	// and display the appropriate control symbol.
	void SynchControlSymbol();

	// Sets normal play mode in the playlist, play screen, and play mode menu.
	void SetNormalPlayMode();

	PegFont *p_Verdana;

	PegString *m_pText1String;
	PegString *m_pText2String;
	PegString *m_pText3String;

	PegIcon	*m_pBatteryIcon;
	PegIcon *m_pBatteryFullIcon;
	PegIcon *m_pSpeakerIcon;
	PegIcon *m_pVolumeNumberTensIcon;
	PegIcon *m_pVolumeNumberOnesIcon;
	PegIcon *m_pControlSymbolIcon;
	PegIcon *m_pTrackNumberTensIcon;
	PegIcon *m_pTrackNumberOnesIcon;
	PegIcon *m_pTimeNumberHoursTensIcon;
	PegIcon *m_pTimeNumberHoursOnesIcon;
	PegIcon *m_pTimeNumberMinutesTensIcon;
	PegIcon *m_pTimeNumberMinutesOnesIcon;
	PegIcon *m_pTimeNumberSecondsTensIcon;
	PegIcon *m_pTimeNumberSecondsOnesIcon;
	PegIcon *m_pTimeColonIcon;
	PegIcon *m_pTimeColonHoursIcon;
	PegIcon *m_pPlayModeIcon;
	PegIcon *m_pLockIcon;
	PegIcon *m_pScreenIcon;
	PegIcon *m_pScrennTopIcon;

	int		m_iBatteryLevel;
	int		m_iSec1s;
	int		m_iSec10s;
	int		m_iMin1s;
	int		m_iMin10s;
	int		m_iHr1s;
	int		m_iHr10s;
	unsigned int		m_uTrackTime;
	unsigned int		m_uTrackDuration;

	bool	m_bScanningForwardPrimed;
	bool	m_bScanningForward;
	bool	m_bScanningBackwardPrimed;
	bool	m_bScanningBackward;
	int		m_iScanTime;
	bool	m_bIgnoreNextKeyup;

	bool	m_bScrollingTitle;			// Should the title be scrolled?
	bool	m_bInitialScrollPause;		// Are we waiting on the initial scrolling timeout?
	bool	m_bBuffering;				// Is the buffering thread reading in data?

	bool	m_bCharging;				// True if the battery charging animation is enabled.

	bool	m_bMenuButtonPress;

	ControlSymbol m_eControlSymbol;

	static const int sc_iBatteryCheckInterval;
	static const int sc_iBatteryChargingCheckInterval;

	static const int sc_iScrollStartInterval;
	static const int sc_iScrollContinueInterval;
	static const int sc_iScrollOffset;
	int m_iOffsetReset;

	static const int sc_iScanSpeedupInterval;	// Amount of ticks to wait before moving up to next scan speed.
	static const int sc_aryScanSpeeds[4];		// Array of possible scan speeds.
	static const int sc_iScanMaxIndex;		// Maximum index into the array of scan speeds.
	int m_iScanCount;	// Current number of ticks spent at the current scan rate.
	int m_iScanIndex;	// Current index into the array of scan rates.

	// Amount of ticks to wait before sleeping the drive after a next/previous track button press.
	static const int sc_iTrackChangeSleepInterval;
	bool	m_bWaitingForTrackChangeSleep;

	COnScreenMenu* m_pOnScreenMenu;

	static CPlayerScreen* s_pPlayerScreen;

    CPlayManager* m_pPlayManager;
    IContentManager* m_pContentManager;

};

#endif  // PLAYERSCREEN_H_
