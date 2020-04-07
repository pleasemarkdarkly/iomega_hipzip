//........................................................................................
//........................................................................................
//.. File Name: PlayerScreen.cpp														..
//.. Date: 09/04/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of the CPlayerScreen class		..
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
#include <stdio.h>

#include <main/ui/PlayerScreen.h>

#include <main/ui/UI.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/main/PlaylistConstraint.h>

#include <main/main/AppSettings.h>
#include <io/audio/VolumeControl.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <main/main/FatHelper.h>
#include <main/main/Events.h>
#include <main/main/EventTypes.h>
#include <main/main/Recorder.h>
#include <main/content/metakitcontentmanager/MetakitContentManager.h>
#include <devs/audio/cs5332.h>

#include <util/registry/Registry.h>
static const RegKey PlayerScreenRegKey = REGKEY_CREATE( PLAYER_SCREEN_REGISTRY_KEY_TYPE, PLAYER_SCREEN_REGISTRY_KEY_NAME );

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYER_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE(DBG_PLAYER_SCREEN);

#define TIMER_SCROLL_TITLE				201
#define TIMER_SCROLL_END				202
#define TIMER_CHECK_BATTERY				203
#define TIMER_CHECK_CHARGING			204
#define TIMER_SLEEP_AFTER_TRACK_CHANGE	205
#define TIMER_FLUTTER_CTRL_SYMBOL       897
#define METADATA_STRING_SIZE 128
#define MENU_HOLD_RECORDING_THRESHOLD 5
#define VOLUME_RANGE 20
#define VOLUME_DEFAULT 10


struct tPlayerScreenSavedSettings 
{
    CTimeMenuScreen::TimeViewMode eTimeViewMode;
    CPogoPlaylist::PogoPlaylistMode ePogoPlaylistMode;
    int nRandomNumberSeed;
    tEqualizerMode eEqualizerMode;
    int nRecordingBitrate;
    bool bBacklightOn;
    int nRecordingInputSource;
    int nRecordingGain;
};

const int sc_iCtrlSymbolChangeInterval = 15;

const int CPlayerScreen::sc_iBatteryCheckInterval = 15;
const int CPlayerScreen::sc_iBatteryChargingCheckInterval = 15;

const int CPlayerScreen::sc_iScrollStartInterval = 100;
const int CPlayerScreen::sc_iScrollEndInterval = 25;
const int CPlayerScreen::sc_iScrollContinueInterval = 5;

// Volume values (20 of them, starting with mute, ending with 18db)
//int iVolValues[20] = {-97,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
//int iVolValues[20] = {-97,-42,-40,-38,-36,-34,-32,-30,-28,-26,-24,-22,-20,-18,-16,-14,-12,-10,-8,-6};
int iVolValues[20] = {-97,-43,-33,-27,-23,-20,-17,-15,-13,-12,-10,-9,-7,-6,-5,-4,-3,-2,-1,0};
// Amount of ticks to wait before moving up to next scan speed.
const int CPlayerScreen::sc_iScanSpeedupInterval = 11;
// Array of possible scan speeds.
const int CPlayerScreen::sc_aryScanSpeeds[4] = { 5, 60, 600, 1800 };
// Maximum index into the array of scan speeds.
const int CPlayerScreen::sc_iScanMaxIndex = 3;

// Amount of ticks to wait before sleeping the drive after a next/previous track button press.
const int CPlayerScreen::sc_iTrackChangeSleepInterval = 20;

//extern CPlayScreen* g_pMainWindow;
CPlayerScreen* CPlayerScreen::s_pPlayerScreen = 0;

// This is a singleton class.
CScreen*
CPlayerScreen::GetPlayerScreen()
{
	if (!s_pPlayerScreen) {
		s_pPlayerScreen = new CPlayerScreen(NULL);
	}
	return s_pPlayerScreen;
}

CPlayerScreen::CPlayerScreen(CScreen* pParent)
  : CScreen(pParent), 
  // Time tracking
  m_iTrackTime(0), m_uTrackDuration(0), m_iTrackStartTime(0), m_iSec1s(0), m_iSec10s(0), m_iMin1s(0), m_iMin10s(0), 
  // Scanning forward/backward support
  m_bScanningForward(false), m_bScanningForwardPrimed(false), m_bScanningBackward(false), 
  m_bScanningBackwardPrimed(false), m_bIgnoreNextKeyup(false), 
  // Menu support
  m_bMenuMode(false), m_eMenuIndex(SET), 
  // Battery level
  m_iBatteryLevel(2), 
  // Scrolling and animation
  m_bScrollingTitle(false), m_bInitialScrollPause(false), m_bCharging(false),
  m_eControlSymbol(STOP),
  m_eEqualizerMode(EQ_NORMAL),
  m_eTimeViewMode(CTimeMenuScreen::TRACK_ELAPSED),
  m_bConfigured(false),
  m_bRecording(false),
  m_cMenuHolds(0)
{
	BuildScreen();

	// Set the current battery level and start a timer to check the battery level every minute.
	SetBattery(m_iBatteryLevel);
	SetTimer(TIMER_CHECK_BATTERY, sc_iBatteryCheckInterval, sc_iBatteryCheckInterval);

	CBrowseMenuScreen::GetBrowseMenuScreen()->SetParent(this);
	CSetMenuScreen::GetSetMenuScreen()->SetParent(this);
	CSetupMenuScreen::GetSetupMenuScreen()->SetParent(this);

    CVolumeControl::GetInstance()->SetVolumeRange(VOLUME_RANGE,iVolValues);
    SetVolume(CVolumeControl::GetInstance()->GetVolume());
}

CPlayerScreen::~CPlayerScreen()
{
    SaveToRegistry();
}

// load registry settings if possible, or else create the entry in the registry for next time.
// I had this code in the ctor, but that creates issues with order of creation (playlist queried for mode before its creation).
void CPlayerScreen::InitRegistry()
{
    int* settings = (int*) new unsigned char[ GetStateSize() + 1 ];
    SaveState((void*)settings, GetStateSize());
    CRegistry::GetInstance()->AddItem( PlayerScreenRegKey, (void*)settings, REGFLAG_PERSISTENT, GetStateSize() );
}

int CPlayerScreen::SaveToRegistry() 
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SaveToReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlayerScreenRegKey );
    if( ! buf ) {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_WARNING, "PS:Couldn't Find Registry Key\n");
    } else {
        SaveState( buf, GetStateSize() );
        return 1;
    }
}
int CPlayerScreen::RestoreFromRegistry() 
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RestFrReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlayerScreenRegKey );
    if( !buf ) {
        InitRegistry();
    } else {
        RestoreState( buf, GetStateSize() );
        return 1;
    }
}

void
CPlayerScreen::Draw()
{
	BeginDraw();
	CScreen::Draw();

	// Highlight the selected line
	if (m_bMenuMode && m_eMenuIndex <= SETUP)
		Screen()->InvertRect(this, m_aryMenuItemsHighlightRect[m_eMenuIndex]);

	EndDraw();
}

void CPlayerScreen::TogglePlayPause()
{
    if (!m_bConfigured)
        return;
    if (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:play\n"); 
        CPlayManager::GetInstance()->Play();
        SetControlSymbol(PLAY);
    }
    else
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:pause\n"); 
        CPlayManager::GetInstance()->Pause();
        SetControlSymbol(PAUSE);
    }
}

void CPlayerScreen::IncrementVolume()
{
    SetVolume(CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() + 1));
}

void CPlayerScreen::DecrementVolume()
{
    SetVolume(CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() - 1));
}

void CPlayerScreen::ExitRecordingSession()
{
    m_bRecording = false;
    if (CRecorder::GetInstance()->InSession())
        CRecorder::GetInstance()->CloseSession();
}

SIGNED CPlayerScreen::HandleKeyRefreshContent(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:refresh content\n");
    g_pEvents->Event(EVENT_USB_CONNECT,0);
    cyg_thread_delay(200);
    g_pEvents->Event(EVENT_USB_DISCONNECT,0);
    return 0;
}

SIGNED CPlayerScreen::HandleTrackProgress(const PegMessage &Mesg)
{
	if (!m_bScanningBackward && !m_bScanningForward /*&& (CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::STOPPED)*/)
		SetTime(Mesg.lData + m_iTrackStartTime);
    return 0;
}

SIGNED CPlayerScreen::HandleTimerCheckBattery(const PegMessage &Mesg)
{
  	static bool s_bLowBatteryFlash = true;
	int iBatteryLevel = 2; // get the charge level
	bool bPowerSource = true;  // is the unit plugged in to ac power?
	if (!bPowerSource)
	{
		// only change the battery level down, so that once it gets a low reading, it stays
		// there.  this is done so that the battery level doesn't fluctuate back and fourth
		// when it's near a boundary in charge levels.
		if (iBatteryLevel < m_iBatteryLevel)
		{
			m_iBatteryLevel = iBatteryLevel;
			SetBattery(m_iBatteryLevel);
		}
		
		if (m_iBatteryLevel == 0)
		{
			if(s_bLowBatteryFlash)
			{ // erase battery for blinking effect
				s_bLowBatteryFlash = false;
				SetBattery(1111);  // no battery picture
			}
			else
			{ // make sure the battery drawing is there
				s_bLowBatteryFlash = true;
				SetBattery(0);
			}
			Invalidate(m_pBatteryIcon->mReal);
			Draw();
		}
		
	}
	else
	{
		if (iBatteryLevel != 4) // MAX_CHARGE)
		{
			m_pBatteryIcon->SetIcon(&gb_Battery_Empty_Bitmap);
			Invalidate(m_pBatteryIcon->mReal);
			Draw();

            SetTimer(TIMER_CHECK_CHARGING, sc_iBatteryChargingCheckInterval, sc_iBatteryChargingCheckInterval);
			KillTimer(TIMER_CHECK_BATTERY);
			m_bCharging = true;
			m_iBatteryLevel = 3;  // do this so that the battery level starts full and goes down evenly
		} 
	}
}

SIGNED CPlayerScreen::HandleTimerCheckCharging(const PegMessage &Mesg)
{
	static int s_iBatteryLevel = 0;
	static bool s_bFullyCharged = false;
	int iBatteryLevel = 2; // get the battery level...  faked right now.
	bool bPowerSource = true;  // is the unit plugged in to ac power?

    if (!bPowerSource)
	{
		SetTimer(TIMER_CHECK_BATTERY, sc_iBatteryCheckInterval, sc_iBatteryCheckInterval);
		KillTimer(TIMER_CHECK_CHARGING);
		m_bCharging = false;
		SetBattery(m_iBatteryLevel);
		s_bFullyCharged = false;
	}
	else if (iBatteryLevel == 4)//MAX_CHARGE)
	{
		s_bFullyCharged = true;
		m_iBatteryLevel = 3;
		SetBattery(m_iBatteryLevel);
	}
	else if (!s_bFullyCharged)
	{
		// do the animation
		if(++s_iBatteryLevel <= 3)
			SetBattery(s_iBatteryLevel);
		else
		{
			s_iBatteryLevel = 0;
			SetBattery(s_iBatteryLevel);
		}
	}
}

SIGNED CPlayerScreen::HandleTimerFlutterCtrlSymbol(const PegMessage &Mesg)
{
    static bool s_bShowingOne = true;
    if (s_bShowingOne)
    {
        s_bShowingOne = false;
        SetControlSymbol(m_eFlutterCtrlSymbolTwo);
    }
    else
    {
        s_bShowingOne = true;
        SetControlSymbol(m_eFlutterCtrlSymbolOne);
    }
    return 0;
}

SIGNED CPlayerScreen::HandleTimerScrollEnd(const PegMessage &Mesg)
{
	SynchTextScrolling();
	Draw();
    return 0;
}

SIGNED CPlayerScreen::HandleTimerScrollTitle(const PegMessage &Mesg)
{
	if (!ScrollTextFields())
	{
		KillTimer(TIMER_SCROLL_TITLE);
		SetTimer(TIMER_SCROLL_END, sc_iScrollEndInterval, 0);
	}
	else
		Draw();
    return 0;
}

SIGNED CPlayerScreen::HandleMsgNewTrack(const PegMessage &Mesg)
{
    set_track_message_t* stm = (set_track_message_t*)Mesg.pData;
    SetTrack(stm);
    delete stm;
}

SIGNED CPlayerScreen::HandleMsgStop(const PegMessage &Mesg)
{
	SynchControlSymbol();
	
	if (CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
	{
		m_iTrackTime = m_iTrackStartTime;
		SetTime(m_iTrackStartTime);
	}
    return 0;
}

SIGNED CPlayerScreen::HandleKeyReleasePrevious(const PegMessage &Mesg)
{
    if (!m_bConfigured)
        return 0;
	if (!m_bMenuMode)
    {
   		if (m_bScanningBackward)
		{
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:seek bwd\n"); 
			m_bScanningBackward = false;
            m_bScanningBackwardPrimed = false;
			SynchControlSymbol();
            if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime-m_iTrackStartTime+1)))
				m_iTrackTime = m_iScanTime;
			else
				SetTime(m_iTrackTime);
		}
		else
		{
            if (m_iTrackTime <= (m_iTrackStartTime+5))
            {
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:prev track\n"); 
				ControlSymbol eCrntSymbol = m_eControlSymbol;
                SetControlSymbol(PREVIOUS_TRACK);
                ERESULT res = CPlayManager::GetInstance()->PreviousTrack();
                if (res == PM_PLAYLIST_END)
                {
                    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
                    pl->SetCurrentEntry(pl->GetEntry(pl->GetSize()-1,IPlaylist::NORMAL));
                    if (g_pEvents->SetCurrentSong())
                        CPlayManager::GetInstance()->Play();
                }
                else if (SUCCEEDED(res))
                {
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Track %d of %d\n", CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex() - 1, CPlayManager::GetInstance()->GetPlaylist()->GetSize());
                }
                else
                    SetControlSymbol(eCrntSymbol);
            }
            else
            {
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:seek bof\n"); 
                if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
                {
                    CPlayManager::GetInstance()->Stop();
                    CPlayManager::GetInstance()->Play();
                }
                else
                {
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "3\n"); 
                    CPlayManager::GetInstance()->Stop();
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "4\n"); 
                    CPlayManager::GetInstance()->Seek(0);
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "5\n"); 
                    SetTime(m_iTrackStartTime);
                    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "6\n"); 
                }
            }
            m_bScanningBackwardPrimed = false;
        }
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyReleaseMenu(const PegMessage &Mesg)
{
    // if the user pressed the menu button from the player screen, then there will be at least one menu hold.  if the user pressed menu in a menu
    // screen, otoh, then this will be zero, and we shouldn't enter menu mode.  they'll still get in if they hold the button longer, but not in general.
    if (m_cMenuHolds > 0)
    {
  		if (m_bMenuMode)
        {
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:exit menu mode\n"); 
			m_bMenuMode = false;
        }
		else
        {
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:enter menu mode\n"); 
			m_bMenuMode = true;
        }
		ScrollMenu(m_eMenuIndex);
    }
    m_cMenuHolds = 0;
    return 0;
}

SIGNED CPlayerScreen::HandleKeyReleaseNext(const PegMessage &Mesg)
{
    if (!m_bConfigured)
        return 0;
    if (!m_bMenuMode)
    {
        if (m_bScanningForward)
	    {
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:seek fwd\n"); 
		    m_bScanningForward = false;
            m_bScanningForwardPrimed = false;
		    SynchControlSymbol();
		    if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime-m_iTrackStartTime+1)))
			    m_iTrackTime = m_iScanTime;
		    else
			    SetTime(m_iTrackTime);
	    }
        else
        {
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:next track\n"); 
            m_bScanningForwardPrimed = false;
            ControlSymbol eCrntSymbol = m_eControlSymbol;
            SetControlSymbol(NEXT_TRACK);
            ERESULT res = CPlayManager::GetInstance()->NextTrack();
            if (res == PM_PLAYLIST_END)
            {
                IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
                pl->SetCurrentEntry(pl->GetEntry(0,IPlaylist::NORMAL));
                if (g_pEvents->SetCurrentSong())
                    CPlayManager::GetInstance()->Play();
            }
            else if (SUCCEEDED(res)) {
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Track %d of %d\n", CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1, CPlayManager::GetInstance()->GetPlaylist()->GetSize());
            }
            else
                SetControlSymbol(eCrntSymbol);
        }
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyNext(const PegMessage &Mesg)
{
	if (m_bMenuMode)
	{
		// add the menu screen with the current 
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:enter menu %d\n",(int)m_eMenuIndex); 
		ProcessMenuOption(m_eMenuIndex);
	}
	else
	{
        if (!m_bConfigured)
            return 0;
		if (m_bScanningForward)
		{
            if (m_iScanTime + sc_aryScanSpeeds[m_iScanIndex] < m_iTrackStartTime + (int)m_uTrackDuration - 5)
			{
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:scan fwd\n"); 
				m_iScanTime += sc_aryScanSpeeds[m_iScanIndex];
				// If enough ticks have passed (and the maximum scan index hasn't been reached)
				// then speed up the scan rate.
				if ((m_iScanIndex < sc_iScanMaxIndex) && (++m_iScanCount > sc_iScanSpeedupInterval))
				{
					++m_iScanIndex;
					m_iScanCount = 0;
				}
			}
			else
			{
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:scan fwd held\n"); 
				m_iScanTime = m_iTrackStartTime + m_uTrackDuration - 5;
			}
			SetTime(m_iScanTime);
		}
		else if (m_bScanningForwardPrimed)
		{
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:begin scan fwd\n"); 
			m_bScanningForward = true;
			m_bScanningForwardPrimed = false;
			m_bScanningBackward = false;
			m_bScanningBackwardPrimed = false;
			m_iScanTime = m_iTrackTime;
			SetControlSymbol(FAST_FORWARD);
		}
		else
		{
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:prime scan fwd\n"); 
			m_bScanningForwardPrimed = true;
			m_iScanCount = 0;
			// If the track is past the hour mark then start scanning in 60-second intervals.
			if (m_iTrackTime > (m_iTrackStartTime + 3600))
				m_iScanIndex = 1;
			else
				m_iScanIndex = 0;
		}
	}
	return 0;
}

SIGNED CPlayerScreen::HandleKeyPlayPause(const PegMessage &Mesg)
{
    if (m_bMenuMode)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:sel menu\n"); 
	    SelectMenuItem(m_eMenuIndex);
    }
    else
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:tggl play\n"); 
        TogglePlayPause();			
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyPrevious(const PegMessage &Mesg)
{
	if (!m_bMenuMode)
	{
        if (!m_bConfigured)
            return 0;
		if (m_bScanningBackward)
		{
            if (m_iScanTime - sc_aryScanSpeeds[m_iScanIndex] >= m_iTrackStartTime)
			{
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:scan bwd\n"); 
				m_iScanTime -= sc_aryScanSpeeds[m_iScanIndex];
				SetTime(m_iScanTime);
				// If enough ticks have passed (and the maximum scan index hasn't been reached)
				// then speed up the scan rate.
				if ((m_iScanIndex < sc_iScanMaxIndex) && (++m_iScanCount > sc_iScanSpeedupInterval))
				{
					++m_iScanIndex;
					m_iScanCount = 0;
				}
			}
			else if (m_iScanTime != m_iTrackStartTime)
			{
                DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:scan bwd held\n"); 
				m_iScanTime = m_iTrackStartTime;
				SetTime(m_iTrackStartTime);
			}
		}
		else if (m_bScanningBackwardPrimed)
		{
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:begin scan bwd\n"); 
			m_bScanningForward = false;
			m_bScanningForwardPrimed = false;
			m_bScanningBackward = true;
			m_bScanningBackwardPrimed = false;
			m_iScanTime = m_iTrackTime;
			SetControlSymbol(REWIND);
		}
		else
		{
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:prime scan bwd\n"); 
			m_bScanningBackwardPrimed = true;
			m_iScanCount = 0;
			// If the track is past the hour mark then start scanning in 60-second intervals.
			if (m_iTrackTime > (m_iTrackStartTime + 3600))
				m_iScanIndex = 1;
			else
				m_iScanIndex = 0;
		}
	}
	return 0;
}

SIGNED CPlayerScreen::HandleKeyUp(const PegMessage &Mesg)
{
    if (m_bMenuMode)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:menu up\n"); 
	    ScrollMenu(m_eMenuIndex - 1);
    }
    else
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:vol up\n"); 
        IncrementVolume();
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDown(const PegMessage &Mesg)
{
	if (m_bMenuMode)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:men dwn\n"); 
		ScrollMenu(m_eMenuIndex + 1);
    }
	else
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:vol dwn\n"); 
    	DecrementVolume();
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyMenu(const PegMessage &Mesg)
{
    // disallow recording from menu mode
    if (++m_cMenuHolds > MENU_HOLD_RECORDING_THRESHOLD && !m_bMenuMode)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:start rec\n"); 
        m_cMenuHolds = 0;
        m_bRecording = true;
        // display gain in place of volume
        int nGain = ADCGetGain();
        SetVolume(nGain);
        // throttle track start time to 0
        m_iTrackStartTime = 0;
        CMediaPlayer::GetInstance()->Deconfigure();
        CPlayManager::GetInstance()->GetPlaylist()->Clear();
        CRecorder* rec = CRecorder::GetInstance();
        int nSession = 0; 
        if (!rec->InSession())
        {
            // start a session.  this will have to call back into the ui to set up the correct onscreen fields, and take care of the current playlist.
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:open session\n"); 
            nSession = rec->OpenSession();
        }
        if (nSession == -1)
        {
            DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "PS:failed to open session\n"); 
            m_bRecording = false;
            TCHAR tszMsg[80];
            CharToTchar(tszMsg, NO_SESSIONS_MESSAGE);
            CSystemMessageScreen::GetSystemMessageScreen()->ShowScreen(this, CSystemMessageScreen::TEXT_MESSAGE, tszMsg);
        }
        else
            CRecorder::GetInstance()->StartRecording();
    }
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDialIn(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:dial tggl play\n"); 
    TogglePlayPause();					
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDialUp(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:dial vol up\n"); 
    IncrementVolume();
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDialDown(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:dial vol dwn\n"); 
	DecrementVolume();
    return 0;
}


SIGNED CPlayerScreen::HandleKeyStopRecording(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:stop rec\n"); 
    m_bRecording = false;
    // reset the volume icon to display volume.
    SetVolume(CVolumeControl::GetInstance()->GetVolume());
    // display stop icon
    SetControlSymbol(STOP);
    // stop recording and add the new file to player core.
    if (CRecorder::GetInstance()->StopRecording() > 0)
        CRecorder::GetInstance()->AddRecordingToPlayerCore();
    return 0;
}

SIGNED CPlayerScreen::HandleKeyUpRecording(const PegMessage &Mesg)
{
    IncrementGain();
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDownRecording(const PegMessage &Mesg)
{
    DecrementGain();
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDialUpRecording(const PegMessage &Mesg)
{
    CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() + 1);
    return 0;
}

SIGNED CPlayerScreen::HandleKeyDialDownRecording(const PegMessage &Mesg)
{
    CVolumeControl::GetInstance()->SetVolume(CVolumeControl::GetInstance()->GetVolume() - 1);
    return 0;
}

void CPlayerScreen::IncrementGain()
{
    int nGain = ADCGetGain();
    ++nGain;
    ADCSetGain(nGain,nGain);
    SetGain(nGain);
}

void CPlayerScreen::DecrementGain()
{
    int nGain = ADCGetGain();
    --nGain;
    ADCSetGain(nGain,nGain);
    SetGain(nGain);
}

SIGNED CPlayerScreen::DispatchRecordingMessage(const PegMessage &Mesg)
{
    switch (Mesg.wType)
	{
		case PM_KEY:
			switch (Mesg.iData)
			{
				case KEY_PLAY_PAUSE:
                    HandleKeyStopRecording(Mesg);
					break;
				case KEY_NEXT:
                    break;
				case KEY_PREVIOUS:
                    break;
                case KEY_UP:
                    HandleKeyUpRecording(Mesg);                    
                    break;
                case KEY_DOWN:
                    HandleKeyDownRecording(Mesg);
                    break;
                case KEY_MENU:
                    break;
                case KEY_REFRESH_CONTENT:
                    HandleKeyRefreshContent(Mesg);
                    break;
                case KEY_RECORD:
                    break;
                case KEY_DIAL_IN:
                    break;
                case KEY_DIAL_UP:
                    HandleKeyDialUpRecording(Mesg);
                    break;
                case KEY_DIAL_DOWN:
                    HandleKeyDialDownRecording(Mesg);
                    break;
				default:
					break;
			}
			break;
		case PM_KEY_RELEASE:
			switch (Mesg.iData)
				case KEY_NEXT:
                    break;
				case KEY_PREVIOUS:
                    break;
                case KEY_MENU:
                    break;
			break;
		case IOPM_PLAY:
			break;
		case IOPM_STOP:
			break;
        case IOPM_NEW_TRACK:
            break;
        case IOPM_CLEAR_TRACK:
            break;
		case IOPM_TRACK_PROGRESS:
            HandleTrackProgress(Mesg);
			break;
		case IOPM_PLAYLIST_LOADED:
			break;
        case IOPM_TRACK_END:
			break;
		case PM_TIMER:
			switch (Mesg.iData)
			{
			    case TIMER_SCROLL_TITLE:
                    HandleTimerScrollTitle(Mesg);
				    break;
			    case TIMER_SCROLL_END:
                    HandleTimerScrollEnd(Mesg);
				    break;
			    case TIMER_CHECK_BATTERY:
                    HandleTimerCheckBattery(Mesg);
					break;
			    case TIMER_CHECK_CHARGING:
                    HandleTimerCheckCharging(Mesg);
                    break;
                case TIMER_FLUTTER_CTRL_SYMBOL:
                    HandleTimerFlutterCtrlSymbol(Mesg);
                    break;
	    	}
            break;
		default:    
			return CScreen::Message(Mesg);
	}
	return 0;
}

SIGNED CPlayerScreen::DispatchPlaybackMessage(const PegMessage &Mesg)
{

    switch (Mesg.wType)
	{
		case PM_KEY:
			switch (Mesg.iData)
			{
				case KEY_PLAY_PAUSE:
                    HandleKeyPlayPause(Mesg);
					break;
				case KEY_NEXT:
                    HandleKeyNext(Mesg);
					break;
				case KEY_PREVIOUS:
                    HandleKeyPrevious(Mesg);
					break;
				case KEY_UP:
                    HandleKeyUp(Mesg);
					break;
				case KEY_DOWN:
                    HandleKeyDown(Mesg);
					break;
				case KEY_MENU:
                    HandleKeyMenu(Mesg);
					break;
                case KEY_REFRESH_CONTENT:
                    HandleKeyRefreshContent(Mesg);
                    break;
                case KEY_RECORD:
                    break;
                case KEY_DIAL_IN:
                    HandleKeyDialIn(Mesg);
                    break;
                case KEY_DIAL_UP:
                    HandleKeyDialUp(Mesg);
					break;
				case KEY_DIAL_DOWN:
                    HandleKeyDialDown(Mesg);
					break;
				default:
					break;
			}
			break;

		case PM_KEY_RELEASE:
			switch (Mesg.iData)
			{
				case KEY_NEXT:
                    HandleKeyReleaseNext(Mesg);
					break;
				case KEY_PREVIOUS:
                    HandleKeyReleasePrevious(Mesg);
					break;
                case KEY_MENU:
                    HandleKeyReleaseMenu(Mesg);
                    break;
			}
			break;

		case IOPM_PLAY:
			SynchControlSymbol();
			break;
		case IOPM_STOP:
            HandleMsgStop(Mesg);
			break;
        case IOPM_NEW_TRACK:
            HandleMsgNewTrack(Mesg);
            break;
        case IOPM_CLEAR_TRACK:
            ClearTrack();
            break;
		case IOPM_TRACK_PROGRESS:
			HandleTrackProgress(Mesg);
			break;
		case IOPM_PLAYLIST_LOADED:
			break;
		case IOPM_TRACK_END:
			break;
		case PM_TIMER:
			switch (Mesg.iData)
			{
			    case TIMER_SCROLL_TITLE:
                    HandleTimerScrollTitle(Mesg);
				    break;
			    case TIMER_SCROLL_END:
                    HandleTimerScrollEnd(Mesg);
				    break;
			    case TIMER_CHECK_BATTERY:
                    HandleTimerCheckBattery(Mesg);
				    break;
			    case TIMER_CHECK_CHARGING:
                    HandleTimerCheckCharging(Mesg);
				    break;
                case TIMER_FLUTTER_CTRL_SYMBOL:
                    HandleTimerFlutterCtrlSymbol(Mesg);
                    break;
                break;
		    }
		default:    
			return CScreen::Message(Mesg);
	}
	return 0;
}

SIGNED
CPlayerScreen::Message(const PegMessage &Mesg)
{
    
    if (m_bRecording)
        return DispatchRecordingMessage(Mesg);
    else
        return DispatchPlaybackMessage(Mesg);
}

// Hides any visible menu screens.
void
CPlayerScreen::HideMenus()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:HideMenus\n");
	// call the submenu screens and call HideScreen
    CSetMenuScreen::GetSetMenuScreen()->HideScreen();

    CSetupMenuScreen::GetSetupMenuScreen()->HideScreen();
    ((CGenreMenuScreen*)CGenreMenuScreen::GetGenreMenuScreen())->Minimize();

	CBrowseMenuScreen::GetBrowseMenuScreen()->HideScreen();
    ((CBrowseMenuScreen*)CBrowseMenuScreen::GetBrowseMenuScreen())->Minimize();
    //CFolderMenuScreen::GetFolderMenuScreen()->HideScreen();
    ((CFolderMenuScreen*)CFolderMenuScreen::GetFolderMenuScreen())->Minimize();
    //CPlaylistMenuScreen::GetPlaylistMenuScreen()->HideScreen();
    ((CPlaylistMenuScreen*)CPlaylistMenuScreen::GetPlaylistMenuScreen())->Minimize();
    Presentation()->MoveFocusTree(this);
	m_bMenuMode = false;
    m_cMenuHolds = 0;
	ScrollMenu(m_eMenuIndex);
}

// Tells the interface to pause any cycle-chomping activity while the buffers are read.
void
CPlayerScreen::NotifyBufferReadBegin()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_WARNING, "PS:Running unexpected NotifyBufferReadBegin\n");
    KillTimer(TIMER_SCROLL_TITLE);
	KillTimer(TIMER_CHECK_CHARGING);
	KillTimer(TIMER_CHECK_BATTERY);
}

// Tells the interface that the buffer read is over, so let the chomping begin!
void
CPlayerScreen::NotifyBufferReadEnd()
{   
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_WARNING, "PS:Running unexpected NotifyBufferReadEnd\n");
	if (m_bScrollingTitle)
		if (m_bInitialScrollPause)
			SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
		else
			SetTimer(TIMER_SCROLL_TITLE, sc_iScrollContinueInterval, sc_iScrollContinueInterval);

	if (m_bCharging)
		SetTimer(TIMER_CHECK_CHARGING, sc_iBatteryChargingCheckInterval, sc_iBatteryChargingCheckInterval);
	else
		SetTimer(TIMER_CHECK_BATTERY, sc_iBatteryCheckInterval, sc_iBatteryCheckInterval);
}

void 
CPlayerScreen::SetProxiedTextField(TCHAR* szSource, PegString* pString, TCHAR* szProxy, TCHAR* szScratch)
{
//#define TEST_FONT
#ifdef TEST_FONT
	TCHAR szUnicodeText[] = {0xb000, 0xb001, 0xb002, 0xb003, 0xb004, 0xb005, 0xb006, 0xb007, 0xb008, 0xb009, 0x0000};
	tstrcpy((TCHAR*)szSource, szUnicodeText);
#endif // TEST_FONT
	// make sure the title we just got can be represented correctly by our font
	// if we can't display it correctly, just show "Track #"
#if 0
    if (!Screen()->TextTest(szSource, &FONT_PLAYSCREEN))
	{
		DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "Can't Print the Track\n");
		char szTempTrackNum[10];
		tstrcpy(szSource, LS(SID_TRACK));
        sprintf(szTempTrackNum, " %d", (CPlayManager::GetInstance()->GetPlaylist()->GetEntryIndex(CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()) + 1));
		DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "szTempTrackNum [%s]\n", szTempTrackNum);
		tstrcat(szSource, szTempTrackNum);
	}
#endif
	int iTitleWidth = Screen()->TextWidth(szSource, &FONT_PLAYSCREEN);
	if (iTitleWidth + pString->mReal.wLeft > mReal.wRight)
	{   // Scroll this string
        m_bScrollingTitle = true;
		m_bInitialScrollPause = true;
	}
    tstrcpy(szProxy, szSource);
	pString->DataSet(szProxy);
    pString->Invalidate(pString->mReal);
    pString->Draw();
}

void
CPlayerScreen::SetMetadataString(IMetadata* metadata, int md_type, PegString* pString, TCHAR* szProxy, TCHAR* szScratch)
{
	TCHAR *szTemp;
    void *pData;
    TCHAR szBlank[] = {0};
    if (SUCCEEDED(metadata->GetAttribute(md_type, &pData)))
        szTemp = (TCHAR*) pData;
    else
        szTemp = szBlank;

    SetProxiedTextField(szTemp, pString, szProxy, szScratch);
}

void
CPlayerScreen::SetTrack(set_track_message_t* stm)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTrack\n");
    // init the scrolling flag to false, and any field that needs scrolling will set this to true
    m_bScrollingTitle = false;
    TCHAR szScratch[METADATA_STRING_SIZE];
	if (stm->metadata)
	{
        // populate the primary metadata display fields
        SetMetadataString(stm->metadata, MDA_ARTIST, m_pArtistTextString, m_szArtistTextString, szScratch);
        SetMetadataString(stm->metadata, MDA_ALBUM, m_pAlbumTextString, m_szAlbumTextString, szScratch);
        SetMetadataString(stm->metadata, MDA_TITLE, m_pTrackTextString, m_szTrackTextString, szScratch);
        // use the filename if the title is lacking.
        if (!tstrlen(m_szTrackTextString))
        {
            TCHAR temp[METADATA_STRING_SIZE];
            CharToTchar(temp,stm->fileref->LongName());
            int idx = tstrlen(temp) -1;
            while (temp[idx] != (TCHAR) '.')
                --idx;
            temp[idx] = (TCHAR) 0;
            SetProxiedTextField( temp, m_pTrackTextString, m_szTrackTextString, szScratch);
        }

        // if the Set field isn't already set by other means, fill it with genre.
        if (!m_aryMenuItemSelected[SET])
            SetMetadataString(stm->metadata, MDA_GENRE, m_pSetTextString, m_szSetTextString, szScratch);
	}
	else
	{
        // fill in the filename
        TCHAR temp[METADATA_STRING_SIZE];
        CharToTchar(temp,stm->fileref->LongName());
        int idx = tstrlen(temp) -1;
        while (temp[idx] != (TCHAR) '.')
            --idx;
        temp[idx] = (TCHAR) 0;
        SetProxiedTextField( temp, m_pTrackTextString, m_szTrackTextString, szScratch);

        // normalize the unknown fields to '<unknown>'
		TCHAR szTempString[] = {'<','u','n','k','n','o','w','n','>',0};
		tstrcpy(m_szArtistTextString, szTempString);
        m_pArtistTextString->DataSet(m_szArtistTextString);
		tstrcpy(m_szAlbumTextString, szTempString);
        m_pAlbumTextString->DataSet(m_szAlbumTextString);
        if (!m_aryMenuItemSelected[SET])
        {
            tstrcpy(m_szSetTextString, szTempString);
            m_pSetTextString->DataSet(m_szSetTextString);
        }
		m_pSetTextString->Draw();
	}

    // set up track duration
    // in order to get updated values for vbr files, we need to query the metadata first, and then fall back on the set track message data.
    void* pvoid;
    if (SUCCEEDED(stm->metadata->GetAttribute(MDA_DURATION,&pvoid)))
        m_uTrackDuration = (int)pvoid;
    else
        m_uTrackDuration = stm->duration;
    
    // if no fields need to scroll, kill the timer.
    if (!m_bScrollingTitle)
        KillTimer(TIMER_SCROLL_TITLE);
    else
        SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);

    // populate the bitrate field
    if (stm->bitrate)
    {
        char temp[8];
        if (stm->bitrate > 1000*1000)
            strcpy(temp,"1M+");
        else
            sprintf(temp,"%d", (stm->bitrate)/1000);
        CharToTchar(szScratch,temp);
    }
    else
        CharToTchar(szScratch,"VBR");
    tstrcpy(m_szBitrateTextString, szScratch);
    m_pBitrateTextString->DataSet(m_szBitrateTextString);
    m_pBitrateTextString->Invalidate(m_pBitrateTextString->mReal);
    m_pBitrateTextString->Draw();
    
    SynchControlSymbol();
    SynchTextScrolling();
    m_bConfigured = true;
    InitTrackStartTime();
	SetTime(m_iTrackStartTime);
    Invalidate(mReal);
}

void CPlayerScreen::ClearTrack()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:ClearTrack\n");
    // clear proxies
    m_szArtistTextString[0] = 0;
    m_szAlbumTextString[0] = 0;
    m_szTrackTextString[0] = 0;
    if (!m_aryMenuItemSelected[SET])
        m_szSetTextString[0] = 0;
    m_szBitrateTextString[0] = 0;
    // set proxies to peg strings
    m_pArtistTextString->DataSet(m_szArtistTextString);
    m_pAlbumTextString->DataSet(m_szAlbumTextString);
    m_pTrackTextString->DataSet(m_szTrackTextString);
    m_pSetTextString->DataSet(m_szSetTextString);
    m_pBitrateTextString->DataSet(m_szBitrateTextString);
    // the SetTime call will call Draw, so we just invalidate the whole screen to get a full draw therein.
    Invalidate(mReal);
	SetTime(0);
    m_bConfigured = false;
}

void
CPlayerScreen::SetVolume(unsigned int uVolume)
{
	PegRect newRect = m_pVolumeBarIcon->mReal;
	newRect.wRight = newRect.wLeft + uVolume;
	m_pVolumeBarFullIcon->Resize(newRect);
	Invalidate(m_pVolumeBarIcon->mReal);
	Draw();
}

void
CPlayerScreen::SetGain(unsigned int uGain)
{
    SetVolume((uGain - ADC_MIN_GAIN) * 20 / (ADC_MAX_GAIN - ADC_MIN_GAIN));
}

void
CPlayerScreen::SetTime(int nTime)
{
	if (nTime == m_iTrackTime)
		return;
    bool bPositive = true;
	m_iTrackTime = nTime;
    if (nTime < 0)
    {
        bPositive = false;
        nTime = -nTime;
    }
	int seconds = nTime % 60;
	int minutes = (nTime % 3600) / 60;
	int hours = nTime / 3600;
	char szTempTime[15] = {0};
	TCHAR szTime[15] = {0};
	// and the minus sign if we're counting down
	if(bPositive)
		sprintf(szTempTime, "  %02d:%02d:%02d", hours, minutes, seconds);
	else
		sprintf(szTempTime, "- %02d:%02d:%02d", hours, minutes, seconds);
	CharToTchar(szTime, szTempTime);
	m_pTimeTextString->DataSet(szTime);
	Draw();
}

void
CPlayerScreen::SetBattery(unsigned int uLevel)
{
	m_pBatteryFullIcon->SetIcon(&gb_Battery_Full_Bitmap);
	PegRect newRect = m_pBatteryIcon->mReal;
	switch(uLevel)
	{
		case 1:  // 1/3
			newRect.wRight = newRect.wLeft + 8;
			break;
		case 2:  // 2/3
			newRect.wRight = newRect.wLeft + 5;
			break;
		case 3:  // full
			newRect.wRight = newRect.wLeft + 0;
			break;
		case 1111:  // everything blank
			m_pBatteryFullIcon->SetIcon(&gbEmptyBitmap);
			newRect.wRight = newRect.wLeft - 1;
			break;
		default: // empty
			newRect.wRight = newRect.wRight + 13;
			break;
	}

	m_pBatteryIcon->Resize(newRect);
	Invalidate(m_pBatteryFullIcon->mReal);
	Draw();
}


void
CPlayerScreen::SetPlayModeIcon(int iPlayMode)
{
    m_pPlayModeIcon->SetIcon(Play_Mode[iPlayMode]);
	m_pPlayModeIcon->Invalidate(m_pPlayModeIcon->mReal);
	m_pPlayModeIcon->Draw();
}

void
CPlayerScreen::SetEqualizerMode(tEqualizerMode eEqualizerMode)
{
	m_eEqualizerMode = eEqualizerMode;
	m_pEqualizerModeIcon->SetIcon(EQ_Setting[m_eEqualizerMode]);
	m_pEqualizerModeIcon->Invalidate(m_pEqualizerModeIcon->mReal);
	m_pEqualizerModeIcon->Draw();
}

// set up the track start time according to the time display mode.
void CPlayerScreen::InitTrackStartTime()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:InitTrackStartTime\n");
    if (!m_bConfigured)
        return;
    switch (m_eTimeViewMode)
    {
        case CTimeMenuScreen::TRACK_ELAPSED:
            m_iTrackStartTime = 0;
            break;
        case CTimeMenuScreen::TRACK_REMAINING:
            m_iTrackStartTime = -m_uTrackDuration;
            break;
        case CTimeMenuScreen::ALBUM_ELAPSED:
        {
            CPogoPlaylist* pp = (CPogoPlaylist*) CPlayManager::GetInstance()->GetPlaylist();
            CPogoPlaylistEntry* entry = (CPogoPlaylistEntry*)pp->GetCurrentEntry();
            DBASSERT( DBG_PLAYER_SCREEN, (entry), "No playlist entry in InitTrackStartTime\n");
            m_iTrackStartTime = pp->GetTrackStartTime(entry);
            break;
        }
        case CTimeMenuScreen::ALBUM_REMAINING:
        {
            CPogoPlaylist* pp = (CPogoPlaylist*) CPlayManager::GetInstance()->GetPlaylist();
            CPogoPlaylistEntry* entry = (CPogoPlaylistEntry*)pp->GetCurrentEntry();
            int albumlen = pp->GetAlbumDuration(entry);
            m_iTrackStartTime = pp->GetTrackStartTime(entry) - albumlen;
            break;  
        }
    }
}

void
CPlayerScreen::SetTimeViewMode(CTimeMenuScreen::TimeViewMode eTimeViewMode)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTimeViewMode\n");
    if (m_eTimeViewMode != eTimeViewMode)
    {
	    m_eTimeViewMode = eTimeViewMode;
        InitTrackStartTime();
        switch (eTimeViewMode)
        {
            case CTimeMenuScreen::TRACK_ELAPSED:
            case CTimeMenuScreen::TRACK_REMAINING:
                m_pTimeByAlbumIcon->SetIcon(&gbEmptyBitmap);
                break;
            case CTimeMenuScreen::ALBUM_ELAPSED:
            case CTimeMenuScreen::ALBUM_REMAINING:
                m_pTimeByAlbumIcon->SetIcon(&gbTimeByAlbumBitmap);
                break;  
        }
	    m_pTimeByAlbumIcon->Invalidate(m_pTimeByAlbumIcon->mReal);
	    m_pTimeByAlbumIcon->Draw();
    }
}

void
CPlayerScreen::SetControlSymbol(ControlSymbol eControlSymbol)
{
	m_eControlSymbol = eControlSymbol;
	m_pControlSymbolIcon->SetIcon(Control_Symbol[eControlSymbol]);
	m_pControlSymbolIcon->Invalidate(m_pControlSymbolIcon->mReal);
	m_pControlSymbolIcon->Draw();
}

void
CPlayerScreen::SetLockIcon(bool on)
{
	if (on)
		m_pLockIcon->SetIcon(&gb_Lock_Bitmap);
	else
		m_pLockIcon->SetIcon(&gbEmptyBitmap);
	Invalidate(m_pLockIcon->mReal);
	Draw();
}

void
CPlayerScreen::SetSetText(const TCHAR* szText)
{
	tstrncpy(m_szSetTextString, szText, METADATA_STR_SIZE-1);
	Screen()->Invalidate(m_pSetTextString->mReal);
	SynchTextScrolling();
	Draw();
}

void
CPlayerScreen::SetArtistText(const TCHAR* szText)
{
	tstrncpy(m_szArtistTextString, szText, METADATA_STR_SIZE-1);
	Screen()->Invalidate(m_pArtistTextString->mReal);
	SynchTextScrolling();
	Draw();
}

void
CPlayerScreen::SetAlbumText(const TCHAR* szText)
{
	tstrncpy(m_szAlbumTextString, szText, METADATA_STR_SIZE-1);
	Screen()->Invalidate(m_pAlbumTextString->mReal);
	SynchTextScrolling();
	Draw();
}


void
CPlayerScreen::SetTrackText(const TCHAR* szText)
{
	tstrncpy(m_szTrackTextString, szText, METADATA_STR_SIZE-1);
	Screen()->Invalidate(m_pTrackTextString->mReal);
	SynchTextScrolling();
	Draw();
}


void
CPlayerScreen::SetBitrateText(const TCHAR* szText)
{
	tstrncpy(m_szBitrateTextString, szText, 9);
	Screen()->Invalidate(m_pBitrateTextString->mReal);
	SynchTextScrolling();
	Draw();
}

void
CPlayerScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
	TCHAR szTitle[256 * 2 + 4] = {0};

	// set text menu area
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wLeft + 27, mReal.wTop + 9);
	m_pSetString = new PegString(ChildRect, LS(SID_SET), 0, FF_NONE | TT_COPY);
	m_pSetString->SetFont(&FONT_PLAYSCREEN);
	m_pSetString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pSetString);

	// artist text menu area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 10, mReal.wLeft + 27, mReal.wTop + 19);
	m_pArtistString = new PegString(ChildRect, LS(SID_ARTIST), 0, FF_NONE | TT_COPY);
	m_pArtistString->SetFont(&FONT_PLAYSCREEN);
	m_pArtistString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pArtistString);

	// album text menu area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 20, mReal.wLeft + 27, mReal.wTop + 29);
	m_pAlbumString = new PegString(ChildRect, LS(SID_ALBUM), 0, FF_NONE | TT_COPY);
	m_pAlbumString->SetFont(&FONT_PLAYSCREEN);
	m_pAlbumString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pAlbumString);

	// track text menu area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 30, mReal.wLeft + 27, mReal.wTop + 39);
	m_pTrackString = new PegString(ChildRect, LS(SID_TRACK), 0, FF_NONE | TT_COPY);
	m_pTrackString->SetFont(&FONT_PLAYSCREEN);
	m_pTrackString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTrackString);

	// setup text menu area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 54, mReal.wLeft + 27, mReal.wTop + 63);
	m_pSetupString = new PegString(ChildRect, LS(SID_SETUP), 0, FF_NONE | TT_COPY);
	m_pSetupString->SetFont(&FONT_PLAYSCREEN);
	m_pSetupString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pSetupString);

	// set text area
	ChildRect.Set(mReal.wLeft + 34, mReal.wTop, mReal.wRight, mReal.wTop + 9);
	CharToTchar(m_szSetTextString, "");
	m_pSetTextString = new PegString(ChildRect, m_szSetTextString, 0, FF_NONE);
	m_pSetTextString->SetFont(&FONT_PLAYSCREEN);
	m_pSetTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pSetTextString);

	// artist text area
	ChildRect.Set(mReal.wLeft + 34, mReal.wTop + 10, mReal.wRight, mReal.wTop + 19);
	CharToTchar(m_szArtistTextString, "");
	m_pArtistTextString = new PegString(ChildRect, m_szArtistTextString, 0, FF_NONE);
	m_pArtistTextString->SetFont(&FONT_PLAYSCREEN);
	m_pArtistTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pArtistTextString);

	// album text area
	ChildRect.Set(mReal.wLeft + 34, mReal.wTop + 20, mReal.wRight, mReal.wTop + 29);
	CharToTchar(m_szAlbumTextString, "");
	m_pAlbumTextString = new PegString(ChildRect, m_szAlbumTextString, 0, FF_NONE);
	m_pAlbumTextString->SetFont(&FONT_PLAYSCREEN);
	m_pAlbumTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pAlbumTextString);

	// track text area
	ChildRect.Set(mReal.wLeft + 34, mReal.wTop + 30, mReal.wRight, mReal.wTop + 39);
	CharToTchar(m_szTrackTextString, "");
	m_pTrackTextString = new PegString(ChildRect, m_szTrackTextString, 0, FF_NONE);
	m_pTrackTextString->SetFont(&FONT_PLAYSCREEN);
	m_pTrackTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTrackTextString);

	// the vertical bar on the screen
	ChildRect.Set(mReal.wLeft + 31, mReal.wTop, mReal.wLeft + 32, mReal.wBottom);
	m_pScreenVerticalDottedBarIcon = new PegIcon(ChildRect, &gbScreenVerticalDottedBarBitmap);
	m_pScreenVerticalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pScreenVerticalDottedBarIcon);

	// the black part of the screen
	ChildRect.Set(mReal.wLeft + 6, mReal.wTop + 40, mReal.wRight, mReal.wTop + 52);
	m_pScreenBarIcon = new PegIcon(ChildRect, &gbScreenBarBitmap);
	m_pScreenBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pScreenBarIcon);

    // control symbol
	ChildRect.Set(mReal.wLeft + 10, mReal.wTop + 42, mReal.wLeft + 28, mReal.wTop + 51);
	m_pControlSymbolIcon = new PegIcon(ChildRect, Control_Symbol[STOP]);
	m_pControlSymbolIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pControlSymbolIcon);

	// the time region of the screen
	CharToTchar(szTitle, "  00:00:00");
	ChildRect.Set(mReal.wLeft + 36, mReal.wTop + 43, mReal.wLeft + 83, mReal.wTop + 51);
	m_pTimeTextString = new PegString(ChildRect, szTitle, 0, FF_NONE | TT_COPY | TJ_RIGHT);
	m_pTimeTextString->SetColor(PCI_NORMAL, BLACK);
	m_pTimeTextString->SetColor(PCI_NTEXT, WHITE);
	m_pTimeTextString->SetFont(&FONT_PLAYSCREEN);
	m_pTimeTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeTextString);

    // time by album icon
	ChildRect.Set(mReal.wLeft + 85, mReal.wTop + 43, mReal.wLeft + 93, mReal.wTop + 51);
	m_pTimeByAlbumIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pTimeByAlbumIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pTimeByAlbumIcon);

	// the bitrate area
	CharToTchar(m_szBitrateTextString, "VBR");
	ChildRect.Set(mReal.wLeft + 100, mReal.wTop + 43, mReal.wRight - 8, mReal.wTop + 51);
	m_pBitrateTextString = new PegString(ChildRect, m_szBitrateTextString, 0, FF_NONE);
	m_pBitrateTextString->SetColor(PCI_NORMAL, BLACK);
	m_pBitrateTextString->SetColor(PCI_NTEXT, WHITE);
	m_pBitrateTextString->SetFont(&FONT_PLAYSCREEN);
	m_pBitrateTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pBitrateTextString);

	// volume bar display
	ChildRect.Set(87, 55, 107, 63);
	m_pVolumeBarIcon = new PegIcon(ChildRect, &gb_Volume_Empty_Bitmap);
	m_pVolumeBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pVolumeBarIcon);

	m_pVolumeBarFullIcon = new PegIcon(ChildRect, &gb_Volume_Full_Bitmap);
	m_pVolumeBarFullIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pVolumeBarFullIcon);

	// battery
	ChildRect.Set(mReal.wRight - 13, mReal.wBottom - 8, mReal.wRight - 1, mReal.wBottom);
	m_pBatteryFullIcon = new PegIcon(ChildRect, &gb_Battery_Full_Bitmap);
	m_pBatteryFullIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pBatteryFullIcon);

	ChildRect.Set(mReal.wRight - 13, mReal.wBottom - 8, mReal.wRight - 1, mReal.wBottom);
	m_pBatteryIcon = new PegIcon(ChildRect, &gb_Battery_Empty_Bitmap);
	m_pBatteryIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pBatteryIcon);

	// play mode
	ChildRect.Set(mReal.wLeft + 35, mReal.wBottom - 8, mReal.wLeft + 46, mReal.wBottom);
	m_pPlayModeIcon = new PegIcon(ChildRect, Play_Mode[0]);
	m_pPlayModeIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pPlayModeIcon);

    // eq mode
	ChildRect.Set(mReal.wLeft + 53, mReal.wBottom - 9, mReal.wLeft + 67, mReal.wBottom);
	m_pEqualizerModeIcon = new PegIcon(ChildRect, EQ_Setting[0]);
	m_pEqualizerModeIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pEqualizerModeIcon);
	
	// lock 
	ChildRect.Set(74, 54, 80, 62);
	m_pLockIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pLockIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pLockIcon);

	// arrow icons to denote what itmes are locked for playback
	ChildRect.Set(m_pSetString->mReal.wRight + 2, m_pSetString->mReal.wTop, m_pSetString->mReal.wRight + 6, m_pSetString->mReal.wBottom);
	m_aryArrowIcon[SET] = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_aryArrowIcon[SET]->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_aryArrowIcon[SET]);
	m_aryMenuItemSelected[SET] = false;

	ChildRect.Set(m_pArtistString->mReal.wRight + 2, m_pArtistString->mReal.wTop, m_pArtistString->mReal.wRight + 6, m_pArtistString->mReal.wBottom);
	m_aryArrowIcon[ARTIST] = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_aryArrowIcon[ARTIST]->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_aryArrowIcon[ARTIST]);
	m_aryMenuItemSelected[ARTIST] = false;

	ChildRect.Set(m_pAlbumString->mReal.wRight + 2, m_pAlbumString->mReal.wTop, m_pAlbumString->mReal.wRight + 6, m_pAlbumString->mReal.wBottom);
	m_aryArrowIcon[ALBUM] = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_aryArrowIcon[ALBUM]->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_aryArrowIcon[ALBUM]);
	m_aryMenuItemSelected[ALBUM] = false;

	ChildRect.Set(m_pTrackString->mReal.wRight + 2, m_pTrackString->mReal.wTop, m_pTrackString->mReal.wRight + 6, m_pTrackString->mReal.wBottom);
	m_aryArrowIcon[TRACK] = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_aryArrowIcon[TRACK]->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_aryArrowIcon[TRACK]);
	m_aryMenuItemSelected[TRACK] = false;


	// setup the hightlight rects for the menu items on the left
	m_aryMenuItemsHighlightRect[SET] = m_pSetString->mReal;
	m_aryMenuItemsHighlightRect[SET].wTop--;
	m_aryMenuItemsHighlightRect[ARTIST] = m_pArtistString->mReal;
	m_aryMenuItemsHighlightRect[ARTIST].wTop--;
	m_aryMenuItemsHighlightRect[ALBUM] = m_pAlbumString->mReal;
	m_aryMenuItemsHighlightRect[ALBUM].wTop--;
	m_aryMenuItemsHighlightRect[TRACK] = m_pTrackString->mReal;
	m_aryMenuItemsHighlightRect[TRACK].wTop--;
	m_aryMenuItemsHighlightRect[SETUP] = m_pSetupString->mReal;
	m_aryMenuItemsHighlightRect[SETUP].wTop--;
}


// Query the play manager for its current play state
// and display the appropriate control symbol.
void
CPlayerScreen::SynchControlSymbol()
{
	if (m_bScanningForward)
	{
		SetControlSymbol(FAST_FORWARD);
		return;
	}
	if (m_bScanningBackward)
	{
		SetControlSymbol(REWIND);
		return;
	}
	switch (CPlayManager::GetInstance()->GetPlayState())
	{
		case CMediaPlayer::PLAYING:
			SetControlSymbol(PLAY);
			break;

		case CMediaPlayer::PAUSED:
			SetControlSymbol(PAUSE);
			break;

		case CMediaPlayer::STOPPED:
        case CMediaPlayer::NOT_CONFIGURED:
			SetControlSymbol(STOP);
			break;
	}
	//SetControlSymbol(m_eControlSymbol);
}


void
CPlayerScreen::SynchTextScrolling()
{
	// make sure all the strings are pointing to the right text
	m_pSetTextString->DataSet(m_szSetTextString);
	m_pArtistTextString->DataSet(m_szArtistTextString);
	m_pAlbumTextString->DataSet(m_szAlbumTextString);
	m_pTrackTextString->DataSet(m_szTrackTextString);

	int iSetTextWidth = Screen()->TextWidth(m_pSetTextString->DataGet(), m_pSetTextString->GetFont());
	int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
	int iAlbumTextWidth = Screen()->TextWidth(m_pAlbumTextString->DataGet(), m_pAlbumTextString->GetFont());
	int iTrackTextWidth = Screen()->TextWidth(m_pTrackTextString->DataGet(), m_pTrackTextString->GetFont());

	if ((iSetTextWidth > m_pSetTextString->mReal.wRight - m_pSetTextString->mReal.wLeft) ||
		(iArtistTextWidth > m_pArtistTextString->mReal.wRight - m_pArtistTextString->mReal.wLeft) ||
		(iAlbumTextWidth > m_pAlbumTextString->mReal.wRight - m_pAlbumTextString->mReal.wLeft) ||
		(iTrackTextWidth > m_pTrackTextString->mReal.wRight - m_pTrackTextString->mReal.wLeft))
	{
		SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
	}
	else
	{
		KillTimer(TIMER_SCROLL_TITLE);
	}
}


bool
CPlayerScreen::ScrollTextFields()
{
	bool bScrolled = false;
	int iSetTextWidth = Screen()->TextWidth(m_pSetTextString->DataGet(), m_pSetTextString->GetFont());
	int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
	int iAlbumTextWidth = Screen()->TextWidth(m_pAlbumTextString->DataGet(), m_pAlbumTextString->GetFont());
	int iTrackTextWidth = Screen()->TextWidth(m_pTrackTextString->DataGet(), m_pTrackTextString->GetFont());

	if (iSetTextWidth > m_pSetTextString->mReal.wRight - m_pSetTextString->mReal.wLeft)
	{
		m_pSetTextString->DataSet((m_pSetTextString->DataGet())+1);
		bScrolled = true;
	}

	if (iArtistTextWidth > m_pArtistTextString->mReal.wRight - m_pArtistTextString->mReal.wLeft)
	{
		m_pArtistTextString->DataSet((m_pArtistTextString->DataGet())+1);
		bScrolled = true;
	}

	if (iAlbumTextWidth > m_pAlbumTextString->mReal.wRight - m_pAlbumTextString->mReal.wLeft)
	{
		m_pAlbumTextString->DataSet((m_pAlbumTextString->DataGet())+1);
		bScrolled = true;
	}

	if (iTrackTextWidth > m_pTrackTextString->mReal.wRight - m_pTrackTextString->mReal.wLeft)
	{
		m_pTrackTextString->DataSet((m_pTrackTextString->DataGet())+1);
		bScrolled = true;
	}

	return bScrolled;
}

// activating this define allows the user to scroll through the list in one direction indefinitely.
#define MENU_WRAP
void
CPlayerScreen::ScrollMenu(int iMenuIndex)
{
	Screen()->Invalidate(m_aryMenuItemsHighlightRect[m_eMenuIndex]);
	if((iMenuIndex >= SET) && (iMenuIndex <= SETUP))
		m_eMenuIndex = (MenuItem)iMenuIndex;
#ifdef MENU_WRAP
	else if (iMenuIndex < 0)
		m_eMenuIndex = SETUP;
	else
		m_eMenuIndex = SET;
#endif // MENU_WRAP
	Screen()->Invalidate(m_aryMenuItemsHighlightRect[m_eMenuIndex]);
	Draw();
}


void
CPlayerScreen::SelectMenuItem(MenuItem eMenuIndex)
{
	if (m_aryMenuItemSelected[eMenuIndex])
	{
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:deselect %d\n",(int)eMenuIndex); 
        // notify the playlist that this is no longer a playlist requirement
        CPlaylistConstraint* pConstraint = CPlaylistConstraint::GetInstance();
        bool bCrntStillInList = false;
        switch (eMenuIndex)
        {
            case SET:
            {
                // zero out any genre constraint
                pConstraint->SetGenre(CMK_ALL);
                // zero out any playlist file constraint
                pConstraint->SetPlaylistFileNameRef(0);
                // (folder constraints will go away just by updating (since it isn't reflected in PlaylistConstraint))
                pConstraint->UpdatePlaylist(&bCrntStillInList);
                // set the set text with the current genre
                void* data;
                IPlaylistEntry* entry = (CPogoPlaylistEntry*)CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();
                if (entry != 0 && SUCCEEDED(entry->GetContentRecord()->GetAttribute(MDA_GENRE, &data)))
                    SetSetText((const TCHAR*)data);
                else
                {
                    TCHAR temp[1] = { 0 };
                    SetSetText(temp);
                }
                break;                
            }
            case ARTIST:
                pConstraint->SetArtist(CMK_ALL);
                pConstraint->UpdatePlaylist(&bCrntStillInList);
                break;                
            case ALBUM:
                pConstraint->SetAlbum(CMK_ALL);
                pConstraint->UpdatePlaylist(&bCrntStillInList);
                break;                
            case TRACK:
                pConstraint->SetTrack(NULL);
                pConstraint->UpdatePlaylist(&bCrntStillInList);
                break;                
            case SETUP:
                // (epg,9/28/2001): they can't do this, what would it mean?
                break;                
        }
		m_aryArrowIcon[eMenuIndex]->SetIcon(&gbEmptyBitmap);
        m_aryMenuItemSelected[eMenuIndex] = false;
		Invalidate(m_aryArrowIcon[eMenuIndex]->mReal);
	}
	else
	{   
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:sel %d\n",(int)m_eMenuIndex); 
        if (!m_bConfigured)
            return;
        // notify the playlist that this is a playlist required field.
        IPlaylistEntry* pEntry = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();
        IMediaContentRecord* pContent = pEntry->GetContentRecord();
        IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
        CPlaylistConstraint* pConstraint = CPlaylistConstraint::GetInstance();
        void* pValue;
        bool bCrntStillInList = false;
        switch (eMenuIndex)
        {
            case SET:
                // (epg,9/28/2001): FIX: maybe not genre
                if (SUCCEEDED(pContent->GetAttribute(MDA_GENRE,&pValue)))
                {
                    pConstraint->SetGenre(pQCM->GetGenreKey((TCHAR*)pValue));
                    pConstraint->UpdatePlaylist(&bCrntStillInList);
                }
                break;                
            case ARTIST:
                if (SUCCEEDED(pContent->GetAttribute(MDA_ARTIST,&pValue)))
                {
                    pConstraint->SetArtist(pQCM->GetArtistKey((TCHAR*)pValue));
                    pConstraint->UpdatePlaylist(&bCrntStillInList);
                }
                break;                
            case ALBUM:
                if (SUCCEEDED(pContent->GetAttribute(MDA_ALBUM,&pValue)))
                {
                    pConstraint->SetAlbum(pQCM->GetAlbumKey((TCHAR*)pValue));
                    pConstraint->UpdatePlaylist(&bCrntStillInList);
                }
                break;                
            case TRACK:
                pConstraint->SetTrack(pContent);
                pConstraint->UpdatePlaylist(&bCrntStillInList);
                break;                
            case SETUP:
                // (epg,9/28/2001): they can't do this, what would it mean?
                break;                
        }
    	m_aryMenuItemSelected[eMenuIndex] = true;
		m_aryArrowIcon[eMenuIndex]->SetIcon(&gb_Menu_Selected_Dot_Bitmap);
		Invalidate(m_aryArrowIcon[eMenuIndex]->mReal);
	}
	Draw();
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "9\n"); 
}


// Sets normal play mode in the playlist, play screen, and play mode menu.
void
CPlayerScreen::SetNormalPlayMode()
{
/*
	IPlaylist::GetPlaylist()->SetPlaylistMode(IPlaylist::NORMAL);
	SetPlayModeIcon(IPlaylist::NORMAL);
	g_pMainMenuScreen->GetPlayModeMenuScreen()->SetPlayModeIcon(IPlaylist::NORMAL);
*/
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CPlayerScreen::ProcessMenuOption(MenuItem eMenuIndex)
{
	switch (eMenuIndex)
	{
		case SET:
			Parent()->Add(CSetMenuScreen::GetSetMenuScreen());
			Presentation()->MoveFocusTree(CSetMenuScreen::GetSetMenuScreen());
			break;
		case ARTIST:
        {
			// tell the Browse Menu Screen what we're browsing for.
            CBrowseMenuScreen* pBMS = (CBrowseMenuScreen*)CBrowseMenuScreen::GetBrowseMenuScreen();
			pBMS->SetBrowseMode(CBrowseMenuScreen::ARTIST);
            pBMS->SetConstraints();
//            pBMS->SetParent(this);
			Parent()->Add(pBMS);
			Presentation()->MoveFocusTree(pBMS);
			break;
        }
		case ALBUM:
        {
			// tell the Browse Menu Screen what we're browsing for.
            CBrowseMenuScreen* pBMS = (CBrowseMenuScreen*)CBrowseMenuScreen::GetBrowseMenuScreen();
			pBMS->SetBrowseMode(CBrowseMenuScreen::ALBUM);
            pBMS->SetConstraints();
//            pBMS->SetParent(this);
			Parent()->Add(pBMS);
			Presentation()->MoveFocusTree(pBMS);
			break;
        }
		case TRACK:
        {
			// tell the Browse Menu Screen what we're browsing for.
            CBrowseMenuScreen* pBMS = (CBrowseMenuScreen*)CBrowseMenuScreen::GetBrowseMenuScreen();
			pBMS->SetBrowseMode(CBrowseMenuScreen::TRACK);
            pBMS->SetConstraints();
//            pBMS->SetParent(this);
			Parent()->Add(pBMS);
			Presentation()->MoveFocusTree(pBMS);
			break;
        }
		case SETUP:
			Parent()->Add(CSetupMenuScreen::GetSetupMenuScreen());
			Presentation()->MoveFocusTree(CSetupMenuScreen::GetSetupMenuScreen());
			break;

		default:
			break;
	};
}

void CPlayerScreen::UpdateConstraintDots(bool bSet, bool bArtist, bool bAlbum, bool bTrack)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:UpdateConstraintDots\n");
    // set
    m_aryMenuItemSelected[SET] = bSet;
    m_aryArrowIcon[SET]->SetIcon(bSet ? &gb_Menu_Selected_Dot_Bitmap : &gbEmptyBitmap);
    // artist
    m_aryMenuItemSelected[ARTIST] = bArtist;
    m_aryArrowIcon[ARTIST]->SetIcon(bArtist ? &gb_Menu_Selected_Dot_Bitmap : &gbEmptyBitmap);
    // album
    m_aryMenuItemSelected[ALBUM] = bAlbum;
    m_aryArrowIcon[ALBUM]->SetIcon(bAlbum ? &gb_Menu_Selected_Dot_Bitmap : &gbEmptyBitmap);
    // track
    m_aryMenuItemSelected[TRACK] = bTrack;
    m_aryArrowIcon[TRACK]->SetIcon(bTrack ? &gb_Menu_Selected_Dot_Bitmap : &gbEmptyBitmap);

    UpdateSetText();
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "10\n"); 
}

// if the Set constraint dot isn't set, then update the set text to the current genre.
void CPlayerScreen::UpdateSetText()
{
    if (!m_aryMenuItemSelected[SET])
    {
        if (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::NOT_CONFIGURED)
        {
            TCHAR scratch[PLAYLIST_STRING_SIZE];
            IPlaylistEntry* entry = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();
            if (entry)
                SetMetadataString(entry->GetContentRecord(),MDA_GENRE,m_pSetTextString,m_szSetTextString,scratch);
        }
    }
}

void CPlayerScreen::SetTrackStartTime(int iTrackStartTime)
{
    m_iTrackStartTime = iTrackStartTime;
}

void CPlayerScreen::DecayAlbumTimeDisplay()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:DecayAlbumTimeDisp\n");
    switch (m_eTimeViewMode)
    {
        case CTimeMenuScreen::ALBUM_ELAPSED:
            SetTimeViewMode(CTimeMenuScreen::TRACK_ELAPSED);
            break;   
        case CTimeMenuScreen::ALBUM_REMAINING:
            SetTimeViewMode(CTimeMenuScreen::TRACK_REMAINING);
            break;   
    }
}

void CPlayerScreen::SetPlayModeIconByPlaylistMode(CPogoPlaylist::PogoPlaylistMode ePLMode)
{
	switch (ePLMode)
	{
        case CPogoPlaylist::NORMAL:		// set normal playlist mode
        {
            SetPlayModeIcon(0);
			break;
        }
        case CPogoPlaylist::PRAGMATIC_RANDOM:
		case CPogoPlaylist::RANDOM:		// random
        {
            SetPlayModeIcon(1);
			break;
        }
		case CPogoPlaylist::REPEAT_ALL:		// repeat all
        {
            SetPlayModeIcon(4);
			break;
        }
		case CPogoPlaylist::REPEAT_RANDOM:		// repeat rand
        {
            SetPlayModeIcon(5);
			break;
        }
        case CPogoPlaylist::ALBUM:     // album
        {
            SetPlayModeIcon(2);
		    break;
        }
        case CPogoPlaylist::REPEAT_ALBUM:     // repeat album
        {
            SetPlayModeIcon(6);
		    break;
        }
	}
}

// write all persistent members into the buf stream, not to exceed len bytes.
void CPlayerScreen::SaveState(void* buf, int len)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SaveState\n");
    tPlayerScreenSavedSettings* settings = (tPlayerScreenSavedSettings*)buf;
    CPogoPlaylist* pl = (CPogoPlaylist*) CPlayManager::GetInstance()->GetPlaylist();

    settings->eEqualizerMode = m_eEqualizerMode;
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:saved eq to %d\n",settings->eEqualizerMode); 
    settings->ePogoPlaylistMode = pl->GetPogoPlaylistMode();
    settings->eTimeViewMode = m_eTimeViewMode;
    settings->nRandomNumberSeed = pl->GetRandomNumberSeed();
    settings->bBacklightOn = ((CBacklightMenuScreen*)CBacklightMenuScreen::GetBacklightMenuScreen())->GetBacklightOn();
    settings->nRecordingBitrate = ((CBitrateMenuScreen*)CBitrateMenuScreen::GetBitrateMenuScreen())->GetBitrate();
    settings->nRecordingInputSource = ((CInputSelectMenuScreen*)CInputSelectMenuScreen::GetInputSelectMenuScreen())->GetInputSelect();
    settings->nRecordingGain = ADCGetGain();
}

// read all persistent members from the buf stream, not to exceed len bytes.
void CPlayerScreen::RestoreState(void* buf, int len)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RestState\n");
    tPlayerScreenSavedSettings* settings = (tPlayerScreenSavedSettings*)buf;
    CPogoPlaylist* pl = (CPogoPlaylist*) CPlayManager::GetInstance()->GetPlaylist();
    CToneMenuScreen* tms = (CToneMenuScreen*) CToneMenuScreen::GetToneMenuScreen();

    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:restoring eq mode from %d\n",settings->eEqualizerMode); 
    SetEqualizerMode(settings->eEqualizerMode);
    tms->SetEqualizerMode(settings->eEqualizerMode);
    pl->SetPogoPlaylistMode(settings->ePogoPlaylistMode);
    SetPlayModeIconByPlaylistMode(settings->ePogoPlaylistMode);
    SetTimeViewMode(settings->eTimeViewMode);
    pl->SetRandomNumberSeed(settings->nRandomNumberSeed);
    ((CBacklightMenuScreen*)CBacklightMenuScreen::GetBacklightMenuScreen())->SetBacklightOn(settings->bBacklightOn);
    ((CBitrateMenuScreen*)CBitrateMenuScreen::GetBitrateMenuScreen())->SetBitrate(settings->bBacklightOn);
    ((CInputSelectMenuScreen*)CInputSelectMenuScreen::GetInputSelectMenuScreen())->SetInputSelect(settings->nRecordingInputSource );
    ADCSetGain(settings->nRecordingGain,settings->nRecordingGain);
}

// specify the byte count allowed in the state stream.
int CPlayerScreen::GetStateSize()
{
    return sizeof(tPlayerScreenSavedSettings) + 1;
}

void CPlayerScreen::UpdateTrackDuration()
{
    int duration = m_iTrackTime - m_iTrackStartTime;
    if (abs(m_uTrackDuration - duration) > 5) 
    {
        IMediaContentRecord* content = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetContentRecord();
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_WARNING, "updating %s from %d to %d duration\n",content->GetFileNameRef()->LongName(),m_uTrackDuration,duration);
        content->SetAttribute(MDA_DURATION, (void*)duration);
    }
    ((CMetakitContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
}

void CPlayerScreen::StartFlutterTimer(ControlSymbol eOne, ControlSymbol eTwo)
{
    m_eFlutterCtrlSymbolOne = eOne;
    m_eFlutterCtrlSymbolTwo = eTwo;
    SetTimer(TIMER_FLUTTER_CTRL_SYMBOL, sc_iCtrlSymbolChangeInterval, sc_iCtrlSymbolChangeInterval);
}

void CPlayerScreen::KillFlutterTimer()
{
	KillTimer(TIMER_FLUTTER_CTRL_SYMBOL);
}

bool CPlayerScreen::IsRecording()
{
    return m_bRecording;
}

void CPlayerScreen::HandleClipDetected()
{
     DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "*\n");
}