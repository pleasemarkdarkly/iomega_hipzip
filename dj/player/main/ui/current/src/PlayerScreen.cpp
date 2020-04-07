//
// PlayerScreen.cpp: contains the implementation of the CPlayerScreen class
// danb@fullplaymedia.com 09/04/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <stdio.h>

#include <main/ui/PlayerScreen.h>
#include <main/ui/Timers.h>

#include <main/ui/UI.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlaylistConstraint.h>

#include <main/ui/MainMenuScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/PlaylistSaveScreen.h>
#include <main/ui/CDTriageScreen.h>
#include <main/ui/YesNoScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/EditScreen.h>
#include <main/ui/EditIPScreen.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/InfoMenuScreen.h>
#include <main/ui/StaticSettingsMenuScreen.h>
#include <main/ui/PlayOrderMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/AboutScreen.h>
#include <main/ui/HDInfoMenuScreen.h>

#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/Recording.h>
#include <main/main/AppSettings.h>
#include <main/main/LEDStateManager.h>
#include <main/main/SpaceMgr.h>
#include <main/main/DJEvents.h>

#include <main/main/DJHelper.h>
#include <main/main/DJPlayerState.h>

#include <datasource/cddatasource/CDDataSource.h>

#include <_modules.h>

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif

#ifndef DISABLE_VOLUME_CONTROL
#include <io/audio/VolumeControl.h>
#endif // DISABLE_VOLUME_CONTROL

#ifdef LINE_RECORDER_ENABLED
#include <main/main/LineRecorder.h>
#endif

#include <util/registry/Registry.h>
static const RegKey PlayerScreenRegKey = REGKEY_CREATE( PLAYER_SCREEN_REGISTRY_KEY_TYPE, PLAYER_SCREEN_REGISTRY_KEY_NAME );

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYER_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PLAYER_SCREEN );

#ifndef NO_UPNP
#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#endif  // NO_UPNP

#define METADATA_STRING_SIZE 128

// Number of peg ticks to wait before firing a set song
// Used to debounce NT & PT
const int CPlayerScreen::sc_iShortTrackChangeInterval = 50;
const int CPlayerScreen::sc_iLongTrackChangeInterval = 100;

// Amount of ticks to wait before moving up to next scan speed.
const int CPlayerScreen::sc_iScanSpeedupInterval = 11;
// Array of possible scan speeds.
const int CPlayerScreen::sc_aryScanSpeeds[4] = { 5, 60, 600, 1800 };
// Maximum index into the array of scan speeds.
const int CPlayerScreen::sc_iScanMaxIndex = 3;

// Amount of time to wait for the ir lights to activate
const int CPlayerScreen::sc_iIREndInterval = 2;

// Amount of ticks to wait before sleeping the drive after a next/previous track button press.
const int CPlayerScreen::sc_iTrackChangeSleepInterval = 20;

//extern CPlayScreen* g_pMainWindow;
CPlayerScreen* CPlayerScreen::s_pPlayerScreen = 0;

struct tPlayerScreenState {
    CTimeMenuScreen::TimeViewMode eTimeMode;
    IPlaylist::PlaylistMode ePLMode;
};

// This is a singleton class.
CPlayerScreen*
CPlayerScreen::GetPlayerScreen()
{
	if (!s_pPlayerScreen) {
		s_pPlayerScreen = new CPlayerScreen(NULL);
	}
	return s_pPlayerScreen;
}

CPlayerScreen::CPlayerScreen(CScreen* pParent)
  : CScreen(pParent),
  m_pBlankWindow(0),
  // Time tracking
  m_iSec1s(0), m_iSec10s(0), m_iMin1s(0), m_iMin10s(0), m_iHr1s(0), m_iHr10s(0), 
  m_iTrackTime(0), m_iTrackStartTime(0), m_iTrackDuration(0),
  // Scanning forward/backward support
  m_bScanningForwardPrimed(false), m_bScanningForward(false), 
  m_bScanningBackwardPrimed(false), m_bScanningBackward(false),
  m_bIgnoreNextKeyup(false), m_bIgnoreScan(false),
  // Scrolling and animation
  m_bScrollingText(false), m_bInitialScrollPause(false),
  m_bDebounceTrackChange(false),
  // NT/PT Backtracking
  m_bBacktrackIfNeeded(false),
  // Playback Feedback
  m_eControlSymbol(STOP),
  m_eTimeViewMode(CTimeMenuScreen::TRACK_ELAPSED),
  // Scan tracking
  m_bSeeked(false),
  // State
  m_bConfigured(false),
  m_eEventHandlerMode(ePSPlaybackEvents)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:Ctor\n");
	BuildScreen();

	CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetParent(this);
    CMainMenuScreen::GetMainMenuScreen()->SetParent(this);
    ResetProgressBar();

    m_PlaybackEventHandler.InitPlayerScreenPtr(this);
#ifdef LINE_RECORDER_ENABLED
    m_LineInEventHandler.InitPlayerScreenPtr(this);
    m_LineRecEventHandler.InitPlayerScreenPtr(this);
#endif
}

CPlayerScreen::~CPlayerScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:Dtor\n");
    SaveToRegistry();

    // fix up our elements for deletion
    if( (m_eViewMode == CDJPlayerState::ZOOM) ||
        (m_eViewMode == CDJPlayerState::ZOOM_EXT) ) {
        Add(m_pTrackTextString);
        Add(m_pArtistTextString);
        Add(m_pControlSymbolIcon);
        Add(m_pTimeTextString);
        Add(m_pRepeatPlayModeTextString);
        Add(m_pRandomPlayModeTextString);
    }
    else {
        Add(m_pZoomTextString);
        Add(m_pZoomTimeTextString);
        Add(m_pControlSymbolLargeIcon);
    }
}

// load registry settings if possible, or else create the entry in the registry for next time.
// I had this code in the ctor, but that creates issues with order of creation (playlist queried for mode before its creation).
void
CPlayerScreen::InitRegistry()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:InitRegistry\n");
    int* settings = (int*) new unsigned char[ GetStateSize() + 1 ];
    SaveState((void*)settings, GetStateSize());
    CRegistry::GetInstance()->AddItem( PlayerScreenRegKey, (void*)settings, REGFLAG_PERSISTENT, GetStateSize() );
}

int
CPlayerScreen::SaveToRegistry() 
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SaveToReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlayerScreenRegKey );
    if( ! buf ) {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_WARNING, "PS:Couldn't Find Registry Key\n");
        return -1;
    } else {
        SaveState( buf, GetStateSize() );
        return 1;
    }
}
int
CPlayerScreen::RestoreFromRegistry() 
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RestFrReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( PlayerScreenRegKey );
    if( !buf ) {
        InitRegistry();
        return -1;  // is -1 ok?
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
    DrawProgressBar();
    EndDraw();
}

void
CPlayerScreen::ForceRedraw()
{
	Invalidate(mReal);
	Draw();
}

// Sets the view mode for all ui screens.
void
CPlayerScreen::SetViewMode(CDJPlayerState::EUIViewMode eViewMode, bool bKeyPress)
{
    if (eViewMode == CDJPlayerState::ZOOM_EXT)
    {
        if ((Presentation()->GetCurrentThing() == this) || (!bKeyPress))
        {
            // take care of ourselves
            CScreen::SetViewMode(eViewMode);
            SynchWithViewMode();
            
            // compress extended zoom down to zoom for everyone else
            eViewMode = CDJPlayerState::ZOOM;
        }
        else
        {
            // pass over extended zoom mode
            eViewMode = CDJPlayerState::NORMAL;
            CDJPlayerState::GetInstance()->SetUIViewMode(eViewMode);
            
            CScreen::SetViewMode(eViewMode);
            SynchWithViewMode();
        }
    }
    else
    {
        CScreen::SetViewMode(eViewMode);
        SynchWithViewMode();
    }
    
    // dispatch the change to the screens that would like to know
    CMainMenuScreen::GetMainMenuScreen()->SetViewMode(eViewMode);
    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetViewMode(eViewMode);
    CAlertScreen::GetInstance()->SetViewMode(eViewMode);
    CYesNoScreen::GetInstance()->SetViewMode(eViewMode);
	CEditScreen::GetInstance()->SetViewMode(eViewMode);
	CEditIPScreen::GetInstance()->SetViewMode(eViewMode);
	CInfoMenuScreen::GetInfoMenuScreen()->SetViewMode(eViewMode);
    CHDInfoMenuScreen::GetHDInfoMenuScreen()->SetViewMode(eViewMode);
    CPlaylistSaveScreen::GetPlaylistSaveScreen()->SetViewMode(eViewMode);
}

void
CPlayerScreen::TogglePlayPause()
{
    if (!m_bConfigured)
        return;
    if (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING)
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:Play\n"); 
        CPlayManager::GetInstance()->Play();
    }
    else
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:Pause\n"); 
        CPlayManager::GetInstance()->Pause();
    }
    SynchControlSymbol();
    SynchStatusMessage();
}

void
CPlayerScreen::TogglePause()
{
    if (!m_bConfigured)
        return;
    if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:Pause\n"); 
        CPlayManager::GetInstance()->Pause();
    }
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PAUSED)
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:Play\n"); 
        CPlayManager::GetInstance()->Play();
    }
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
    {
        // If we're ripping, then check with the recording manager to see if ripping is paused.
        CRecordingManager* pRM = CRecordingManager::GetInstance();
        if (pRM->IsRipping())
            if (pRM->IsPaused())
                pRM->NotifyStreamPlaying();
            else
                pRM->NotifyStreamPaused();
    }
    SynchControlSymbol();
    SynchStatusMessage();
}

SIGNED
CPlayerScreen::Message(const PegMessage &Mesg)
{
    switch (m_eEventHandlerMode)
    {
        case ePSPlaybackEvents:
            return m_PlaybackEventHandler.DispatchEvent(Mesg);
#ifdef LINE_RECORDER_ENABLED
        case ePSLineInEvents:
            return m_LineInEventHandler.DispatchEvent(Mesg);
        case ePSLineRecEvents:
            return m_LineRecEventHandler.DispatchEvent(Mesg);
#endif
        default:
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_WARNING, "PS:unsupported event handler mode %d\n",(int)m_eEventHandlerMode);
            return 1;
    }
}

// Hides any visible menu screens.
void
CPlayerScreen::HideMenus()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:HideMenus\n");
    // call the submenu screens and call HideScreen
    CAlertScreen::GetInstance()->HideScreen();
    CYesNoScreen::GetInstance()->HideScreen();
    CEditScreen::GetInstance()->HideScreen();
    CEditIPScreen::GetInstance()->HideScreen();
    CAboutScreen::GetAboutScreen()->HideScreen();
    CHDInfoMenuScreen::GetHDInfoMenuScreen()->HideScreen();
    CInfoMenuScreen::GetInfoMenuScreen()->HideScreen();
    CMainMenuScreen::GetMainMenuScreen()->HideScreen();
    CStaticSettingsMenuScreen::GetStaticSettingsMenuScreen()->HideScreen();
    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->HideScreen();
    CPlaylistSaveScreen::GetPlaylistSaveScreen()->HideScreen();
    CCDTriageScreen::GetInstance()->HideScreen();
    Presentation()->MoveFocusTree(this);
    Invalidate(mReal);
    ResumeScrollingText();
}

// Tells the interface to stop scrolling text and reset the text fields
void
CPlayerScreen::StopScrollingText()
{
    SynchTextScrolling();
	KillTimer(PS_TIMER_SCROLL_TEXT);
    KillTimer(PS_TIMER_SCROLL_END);
}

// Tells the interface that to resume the scrolling fields of the player screen
void
CPlayerScreen::ResumeScrollingText()
{
    SynchTextScrolling();
    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::PowerOn()
{
    // get rid of the blank overlay screen
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_INFO, "ps:PowOn\n"); 
    Remove(m_pBlankWindow);
    Presentation()->MoveFocusTree(this);
    ForceRedraw();
#ifdef LINE_RECORDER_ENABLED
    // reactivate the passthrough fiq
    if (m_eEventHandlerMode == ePSLineInEvents)
        m_LineInEventHandler.UnMutePassthroughMode();
#endif  // LINE_RECORDER_ENABLED
}

void
CPlayerScreen::PowerOff()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_INFO, "ps:PowOff\n"); 
    // stop line recording
#ifdef LINE_RECORDER_ENABLED
    if (m_eEventHandlerMode == ePSLineRecEvents)
        m_LineRecEventHandler.StopRecording();
    // deactivate the passthrough fiq temporarily
    if (m_eEventHandlerMode == ePSLineInEvents)
        m_LineInEventHandler.MutePassthroughMode();
#endif  // LINE_RECORDER_ENABLED

    // hide menus
    HideMenus();
    // blank out screen
    Add(m_pBlankWindow);
}

TCHAR*
CPlayerScreen::GetMetadataString(IMetadata* pMetadata, int md_type)
{
    void *pData;
    if (SUCCEEDED(pMetadata->GetAttribute(md_type, &pData)))
        return (TCHAR*)pData;
    else
        return 0;
}

void
CPlayerScreen::SetTrack(set_track_message_t* pSTM)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTrack\n");
    // Reset the seek flag.
    m_bSeeked = false;

    // if we're debouncing a track change, don't set a tracks metadata
    if (m_bDebounceTrackChange)
        return;

    m_bDoingTrackChange = false;
    
    // If the user is currently seeking then cancel the seek by ignoring the next key release.
	if (m_bScanningForwardPrimed || m_bScanningForward || m_bScanningBackwardPrimed || m_bScanningBackward)
    {
        m_bIgnoreNextKeyup = true;
        m_bIgnoreScan = true;
        // clear scanning state.  scanning should be properly ignored by m_bIgnoreScan
        m_bScanningForwardPrimed = false;
	    m_bScanningForward = false;
	    m_bScanningBackwardPrimed = false;
	    m_bScanningBackward = false;
    }

	if (pSTM->metadata)
    {
        m_iTrackDuration = pSTM->duration;
        SetTrackInfo(pSTM->metadata);
    }
	else
	{
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_ERROR, "ps:SetTrack *** bad set_track_message_t ***\n");
        m_pArtistTextString->DataSet(LS(SID_EMPTY_STRING));
        m_pTrackTextString->DataSet(LS(SID_EMPTY_STRING));
        m_pZoomTextString->DataSet(LS(SID_EMPTY_STRING));
		m_iTrackDuration = 0;
        ResetProgressBar(0, m_iTrackDuration);
        InitTrackStartTime();
	    SetTime(m_iTrackStartTime);  // calls Draw()
	}

	SynchControlSymbol();
    SynchStatusMessage(); // calls Draw();
    m_bConfigured = true;
}

void
CPlayerScreen::SetTrackInfo(IPlaylistEntry* pEntry)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTrackInfo\n");
    if(pEntry)
        SetTrackInfo(pEntry->GetContentRecord(), pEntry);
}


void
CPlayerScreen::SetTrackInfo(IMetadata* pMetadata, IPlaylistEntry* pEntry)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTrackInfo\n");
    if(pMetadata)
    {
        // get all the important metadata from the playlist entry
        if (CDJPlayerState::GetInstance()->GetUIShowTrackNumInTitle())
        {
            // prepend the track's playlist number to the title
            // get the track number
            char szTrackNumber[32] = {0};
            
            if (pEntry)
                sprintf(szTrackNumber, "%d. ", pEntry->GetIndex() + 1);
            else
            {
                // if we're not provided with a playlist entry,
                // assume that the current entry in the playlist is what
                // is set as the current track
                IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
                if (pCurrentPlaylist)
                {
                    IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
                    if (pCurrentEntry)
                    {
                        sprintf(szTrackNumber, "%d. ", pCurrentEntry->GetIndex() + 1);
                    }
                }
            }

            // slap the two together
            TCHAR* pszTrackText = GetMetadataString(pMetadata, MDA_TITLE);
            TCHAR* pszTrackNumberAndText = (TCHAR*) malloc( (strlen(szTrackNumber) + tstrlen(pszTrackText) + 1) * sizeof(TCHAR));
            if (pszTrackNumberAndText)
            {
                CharToTchar(pszTrackNumberAndText, szTrackNumber);
                tstrcat(pszTrackNumberAndText, pszTrackText);
                SetTrackText(pszTrackNumberAndText);
            }
            else
                SetTrackText(GetMetadataString(pMetadata, MDA_TITLE));

            free(pszTrackNumberAndText);
        }
        else
            SetTrackText(GetMetadataString(pMetadata, MDA_TITLE));

        if (CDJPlayerState::GetInstance()->GetUIShowAlbumWithArtist())
        {
            TCHAR* pszAlbumText = GetMetadataString(pMetadata, MDA_ALBUM);
            if (pszAlbumText)
            {
                TCHAR* pszArtistText = GetMetadataString(pMetadata, MDA_ARTIST);
                TCHAR* pszAlbumWithArtistText = (TCHAR*) malloc( (tstrlen(pszAlbumText) + tstrlen(LS(SID__DASH_)) + tstrlen(pszArtistText) + 1) * sizeof(TCHAR));
                // check with an assert?
                tstrcpy(pszAlbumWithArtistText, pszAlbumText);
                tstrcat(pszAlbumWithArtistText, LS(SID__DASH_));
                tstrcat(pszAlbumWithArtistText, pszArtistText);
                SetArtistText(pszAlbumWithArtistText, true);
                free(pszAlbumWithArtistText);
            }
            else
                SetArtistText(GetMetadataString(pMetadata, MDA_ARTIST), true);
        }
        else
            SetArtistText(GetMetadataString(pMetadata, MDA_ARTIST), true);


        ResetProgressBar(0, m_iTrackDuration);
        InitTrackStartTime();
        // show the change immediately
        SetTime(m_iTrackStartTime, true);
    }
}


void
CPlayerScreen::ClearTrack()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:ClearTrack\n");
    // clear scanning state
    m_bScanningForwardPrimed = false;
	m_bScanningForward = false;
	m_bScanningBackwardPrimed = false;
	m_bScanningBackward = false;

   	SynchControlSymbol();
    SynchStatusMessage();

    m_pArtistTextString->DataSet(0);
    m_pTrackTextString->DataSet(0);
    m_pZoomTextString->DataSet(0);

    Invalidate(mReal);

    // make sure everything is zero'd out
    m_iTrackTime = 0;
    m_iTrackDuration = 0;
    m_iTrackStartTime = 0;
    SetTime(0, true);
    
    ResetProgressBar(0, 0); // calls Draw() again.  oh well.  we'll live.

    m_bConfigured = false;
}

void
CPlayerScreen::DisplayInsertCDScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_INFO, "ps:PromptInsCD\n"); 
    m_iTrackDuration = 0;
    SetTime(0, true);
    UpdateProgressBar(0);
    SetTrackText(LS(SID_PLEASE_INSERT_CD));
    SetArtistText(LS(SID_EMPTY_STRING), true);

    TCHAR tszMessage[128];
    tstrcpy(tszMessage, LS(SID_MUSIC_SOURCE_COLON_));
    tstrcat(tszMessage, LS(SID_CD));
	SetMessageText(tszMessage, CSystemMessageString::STATUS);
}

void
CPlayerScreen::DisplayNoContentScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:DispNoCDContent\n"); 
    m_iTrackDuration = 0;
    SetTime(0, true);
    UpdateProgressBar(0);
    SetTrackText(LS(SID_PLEASE_SELECT_TRACKS));
    SetArtistText(LS(SID_EMPTY_STRING), true);
    SynchStatusMessage();
}

void
CPlayerScreen::DisplayNoStationScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:DispNoStation\n"); 
    m_iTrackDuration = 0;
    SetTime(0, true);
    UpdateProgressBar(0);
    SetTrackText(LS(SID_STATION_NOT_FOUND));
    SetArtistText(LS(SID_EMPTY_STRING), true);
    SynchStatusMessage();
}

void
CPlayerScreen::DisplaySelectTracksScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:DispSelectTracks\n");
    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
        m_LineInEventHandler.SetPassthroughTrack(false);
    else
    {
        m_iTrackDuration = 0;
        SetTime(0, true);
        UpdateProgressBar(0);
        SetTrackText(LS(SID_PLEASE_SELECT_TRACKS));
        SetArtistText(LS(SID_EMPTY_STRING), true);
        SynchStatusMessage();
    }
}

void
CPlayerScreen::NotifyCDInserted()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NotifyCDInserted\n"); 
    m_iTrackDuration = 0;
    SetTime(0, true);
    UpdateProgressBar(0);
    SetTrackText(LS(SID_ACCESSING_CDDB));
    SetArtistText(LS(SID_EMPTY_STRING), true);
    SetMessageText(LS(SID_FOUND_NEW_CD), CSystemMessageString::STATUS);
    // make sure we hide the menus since we're chaning to CD
    // but only if the HD isn't currently being scanned
    if (!CDJPlayerState::GetInstance()->IsScanningHD())
        HideMenus();
}

void
CPlayerScreen::NotifyCDRemoved()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NotifyCDRemoved\n"); 
    SynchStatusMessage();
}

void
CPlayerScreen::NotifyCDTrayOpened()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NotifyCDTrayOpened\n"); 
    SetMessageText(LS(SID_EJECTING_CD), CSystemMessageString::REALTIME_INFO);
}

void
CPlayerScreen::NotifyCDTrayClosed()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NotifyCDTrayClosed\n"); 
    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
    {
        m_iTrackDuration = 0;
        SetTime(0);
        UpdateProgressBar(0);
        SetArtistText(LS(SID_EMPTY_STRING), true);
    }

    SetMessageText(LS(SID_LOOKING_FOR_CD), CSystemMessageString::STATUS);
}

void
CPlayerScreen::NotifyMusicSourceChanged(CDJPlayerState::ESourceMode eSource)
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NotifySourceChanged\n"); 
    ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->SetBrowseSource(eSource, true);

    // if switching to line in mode, fix up our child screen
    if( eSource == CDJPlayerState::LINE_IN ) {
        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle("Recording Session");
    }
    SynchStatusMessage();
}

bool
CPlayerScreen::SetTime(int nTime, bool bForceRedraw)
{
    if( CPlayerScreen::s_pPlayerScreen && this != CPlayerScreen::s_pPlayerScreen ) {
        // this undoubtedly leads to fatal issues down the line, or is a symptom of them
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "\n!!!!!!!!!! SetTime with wrong this ptr !!!!!!!!!!!!!!\n");
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "SetTime(%d, %d)\n", nTime, (int)bForceRedraw);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "CPlayerScreen::s_pPlayerScreen = 0x%x\n", CPlayerScreen::s_pPlayerScreen);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "                          this = 0x%x\n\n", this);

        // dvb 10/10/2002.  use an assert to help track down this strangeness
        DBASSERT( DBG_PLAYER_SCREEN, this == CPlayerScreen::s_pPlayerScreen, "SetTime with wrong this ptr");
        /*
        return CPlayerScreen::s_pPlayerScreen->SetTime(nTime, bForceRedraw);
        */
    }
    
	if (((nTime == m_iTrackTime) || m_bDoingTrackChange) && !bForceRedraw)
		return false;
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

    // total time
    int t_seconds = m_iTrackDuration % 60;
	int t_minutes = (m_iTrackDuration % 3600) / 60;
	int t_hours = m_iTrackDuration / 3600;

	char szTempTime[30] = {0};
	TCHAR szTime[30] = {0};
	// and the minus sign if we're counting down
	if(bPositive)
    {
        if(m_iTrackDuration > 0)
        {
            // todo: what if the song is over 99 hours?   improbable, but not impossible.
            if (hours || t_hours)
		        sprintf(szTempTime, "%02d:%02d / %02d:%02d", hours, minutes, t_hours, t_minutes);
            else
                sprintf(szTempTime, "%02d:%02d / %02d:%02d", minutes, seconds, t_minutes, t_seconds);
        }
        else if(m_iTrackTime > 0)
        {
            if (hours)
                sprintf(szTempTime, "%02d:%02d / --:--", hours, minutes);
            else
                sprintf(szTempTime, "%02d:%02d / --:--", minutes, seconds);
        }
        else
        {
            // this is the case where both numbers are zero
            sprintf(szTempTime,     "--:-- / --:--");
        }
    }
	else
    {
        if (hours > 0)
            sprintf(szTempTime,     "     - %02d:%02d", hours, minutes);
        else 
            sprintf(szTempTime,     "     - %02d:%02d", minutes, seconds);
    }

	CharToTchar(szTime, szTempTime);
    SetTimeText(szTime);
    
    // only redraw if this screen has input focus
    if (Presentation()->GetCurrentThing() == this)
        Draw();

    return true;
}

// set up the track start time according to the time display mode.
void
CPlayerScreen::InitTrackStartTime()
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
            m_iTrackStartTime = -m_iTrackDuration;
            break;
    }
}

void
CPlayerScreen::SetTimeViewMode(CTimeMenuScreen::TimeViewMode eTimeViewMode)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetTimeViewMode\n");
    // dvb (07/23/02) force TRACK_ELAPSED as the default.  leave in since it might come back
    eTimeViewMode = CTimeMenuScreen::TRACK_ELAPSED;

    if (m_eTimeViewMode != eTimeViewMode)
    {
	    m_eTimeViewMode = eTimeViewMode;

        switch (m_eTimeViewMode)
        {
            case CTimeMenuScreen::TRACK_ELAPSED:
                m_iTrackStartTime = 0;
                m_iTrackTime = m_iTrackTime + m_iTrackDuration;
                break;
            case CTimeMenuScreen::TRACK_REMAINING:
                m_iTrackStartTime = -m_iTrackDuration;
                m_iTrackTime = m_iTrackTime - m_iTrackDuration;
                break;
        }

        // show the change immediately
        SetTime(m_iTrackTime, true);
    }

    // dvb (07/23/02) taken out for now.  leave in since it might come back
    /*
    // always synch the menu so we're sure we know what the player screen thinks the view mode is
    ((CTimeMenuScreen*)(CTimeMenuScreen::GetTimeMenuScreen()))->SetTimeViewMode(m_eTimeViewMode);
    */
}

CTimeMenuScreen::TimeViewMode CPlayerScreen::GetTimeViewMode()
{
    return m_eTimeViewMode;
}

void
CPlayerScreen::SetControlSymbol(ControlSymbol eControlSymbol)
{
	m_eControlSymbol = eControlSymbol;
    
	m_pControlSymbolIcon->SetIcon(Control_Symbol[eControlSymbol]);
	m_pControlSymbolIcon->Invalidate(m_pControlSymbolIcon->mReal);
	m_pControlSymbolIcon->Draw();

    m_pControlSymbolLargeIcon->SetIcon(Control_Symbol_Large[eControlSymbol]);
    m_pControlSymbolLargeIcon->Invalidate(m_pControlSymbolLargeIcon->mReal);
    m_pControlSymbolLargeIcon->Draw();
}

void
CPlayerScreen::SetArtistText(const TCHAR* szText, bool bDraw = false)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pArtistTextString->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pArtistTextString->mReal.wTop, m_pArtistTextString->mReal.wRight, m_pArtistTextString->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pArtistTextString->mReal.wTop, m_pArtistTextString->mReal.wRight, m_pArtistTextString->mReal.wBottom);
    m_pArtistTextString->Resize(ChildRect);

    Screen()->Invalidate(m_pArtistTextString->mReal);
    SynchTextScrolling();
    if (bDraw && Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::SetTrackText(const TCHAR* szText, bool bDraw = false)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pTrackTextString->DataSet(szText);
    m_pZoomTextString->DataSet(szText);
    
    // center the text string
    iTextLength = Screen()->TextWidth(m_pTrackTextString->DataGet(), m_pTrackTextString->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pTrackTextString->mReal.wTop, m_pTrackTextString->mReal.wRight, m_pTrackTextString->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pTrackTextString->mReal.wTop, m_pTrackTextString->mReal.wRight, m_pTrackTextString->mReal.wBottom);
    m_pTrackTextString->Resize(ChildRect);
    
    Screen()->Invalidate(m_pTrackTextString->mReal);
    
    // center the zoom string
    iTextLength = Screen()->TextWidth(m_pZoomTextString->DataGet(), m_pZoomTextString->GetFont());
    if(iTextLength < iCaptionLength)
    {
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pZoomTextString->mReal.wTop, m_pZoomTextString->mReal.wRight, m_pZoomTextString->mReal.wBottom);
        m_pZoomTextString->Resize(ChildRect);
    }
    Screen()->Invalidate(m_pZoomTextString->mReal);
    SynchTextScrolling();
    if (bDraw && Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::SetTimeText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    if( CPlayerScreen::s_pPlayerScreen && this != CPlayerScreen::s_pPlayerScreen ) {
        // this undoubtedly leads to fatal issues down the line, or is a symptom of them
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "\n!!!!!!!!!! SetTimeText with wrong this ptr !!!!!!!!!!!!!!\n");
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "SetTimeText(%w)\n", szText);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "CPlayerScreen::s_pPlayerScreen = 0x%x\n", CPlayerScreen::s_pPlayerScreen);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "                          this = 0x%x\n\n", this);

        // dvb 10/10/2002.  use an assert to help track down this strangeness
        DBASSERT( DBG_PLAYER_SCREEN, this == CPlayerScreen::s_pPlayerScreen, "SetTimeText with wrong this ptr");
        /*
        CPlayerScreen::s_pPlayerScreen->SetTimeText(szText);
        return ;
        */
    }
    
	m_pTimeTextString->DataSet(szText);
    m_pZoomTimeTextString->DataSet(szText);

    // right justify the text string
    iTextLength    = Screen()->TextWidth(m_pZoomTimeTextString->DataGet(), m_pZoomTimeTextString->GetFont()) + 5;
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(m_pZoomTimeTextString->mReal.wRight - iTextLength, m_pZoomTimeTextString->mReal.wTop, m_pZoomTimeTextString->mReal.wRight, m_pZoomTimeTextString->mReal.wBottom);
    else 
        ChildRect.Set(mReal.wLeft, m_pZoomTimeTextString->mReal.wTop, m_pZoomTimeTextString->mReal.wRight, m_pZoomTimeTextString->mReal.wBottom);

    m_pZoomTimeTextString->Resize(ChildRect);
    
    Screen()->Invalidate(m_pZoomTimeTextString->mReal);    
}

void
CPlayerScreen::SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetMessageText [%s]\n", szText);
	m_pMessageTextString->SystemMessage(szText, iMessageType);

    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetMessageText [%w]\n", szText);
	m_pMessageTextString->SystemMessage(szText, iMessageType);

    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::ResetProgressBar(int iProgress, int iTotal)
{
    m_iProgressBarTotal = iTotal;
    UpdateProgressBar(iProgress);
}

void
CPlayerScreen::UpdateProgressBar(int iProgress)
{
    // invalidate that part of the screen
    m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom);
    Invalidate(m_ProgressBarRect);
    
    if (iProgress <= 0 || m_iProgressBarTotal <= 0)
        // we don't want to show the progress bar
        m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wLeft - 1, mReal.wBottom);
    else
    {
		int iBarWidth = (int)((double)iProgress * ((double)(mReal.wRight - mReal.wLeft) / (double)m_iProgressBarTotal));
		m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, iBarWidth, mReal.wBottom);
    }

    // only redraw if this screen has input focus
    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CPlayerScreen::DrawProgressBar()
{
    if ((Presentation()->GetCurrentThing() == this) && (m_ProgressBarRect.Width() > 0))
        Screen()->InvertRect(this, m_ProgressBarRect);
}

// remote activity light.  this will draw on top of all screens
void
CPlayerScreen::DrawIRActivity()
{
    PegRect Rect;
    Rect.Set(mReal.wRight - 3, mReal.wTop + 5, mReal.wRight, mReal.wTop + 8);
    Invalidate(Rect);
	BeginDraw();
    Line(mReal.wRight - 3, mReal.wTop + 5, mReal.wRight, mReal.wTop + 5, BLACK, 4);
    EndDraw();
    SetTimer(PS_TIMER_IR, sc_iIREndInterval, 0);
}

void
CPlayerScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
	TCHAR szTitle[256 * 2 + 4] = {0};

    m_pBlankWindow = new PegWindow(mReal, FF_NONE);

	// track text area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 16, mReal.wRight, mReal.wTop + 32);
	m_pTrackTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pTrackTextString->SetFont(&FONT_PLAYSCREENBIG);
	m_pTrackTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pTrackTextString);

	// artist text area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 34, mReal.wRight, mReal.wTop + 48);
	m_pArtistTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pArtistTextString->SetFont(&FONT_PLAYSCREENBOLD);
	m_pArtistTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pArtistTextString);

	// the horizontal bar on the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom - 13);
	m_pScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenHorizontalDottedBarIcon);

	// the message region of the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 13, mReal.wRight, mReal.wBottom);
	m_pMessageTextString = new CSystemMessageString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pMessageTextString->SetFont(&FONT_PLAYSCREEN);
	m_pMessageTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pMessageTextString);

    // large control symbol
    PegBitmap* pBitmap = Control_Symbol_Large[m_eControlSymbol];
    WORD wVert   = (mReal.wBottom - 15) - (mReal.wTop + 1);
    WORD wMargin = wVert - pBitmap->wHeight;
    WORD wBottom = (mReal.wBottom - 15) - (wMargin / 2);
    WORD wTop    = (mReal.wTop + 1) + (wMargin / 2) - 1; // the last -1 is just a tweak
    WORD wLeft   = 20;
    WORD wRight  = wLeft + pBitmap->wWidth;
    ChildRect.Set(wLeft, wTop, wRight, wBottom);
	m_pControlSymbolLargeIcon = new PegIcon(ChildRect, Control_Symbol_Large[m_eControlSymbol]);
	m_pControlSymbolLargeIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);
    
    // zoom time text area
    ChildRect.Set(mReal.wLeft, mReal.wTop + 5, mReal.wRight - 20, mReal.wBottom - 15);
	m_pZoomTimeTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pZoomTimeTextString->SetFont(&FONT_PLAYSCREENZOOM);
	m_pZoomTimeTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
    
	// the time region of the screen
	ChildRect.Set(mReal.wRight - 61, mReal.wTop, mReal.wRight, mReal.wTop + 11);
	m_pTimeTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY | TJ_RIGHT);
	m_pTimeTextString->SetFont(&FONT_PLAYSCREEN);
	m_pTimeTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	CharToTchar(szTitle, "00:00 / --:--");
    SetTimeText(szTitle);
	Add(m_pTimeTextString);

    // control symbol
	ChildRect.Set(mReal.wLeft + 157, mReal.wTop + 3, mReal.wLeft + 172, mReal.wTop + 11);
	m_pControlSymbolIcon = new PegIcon(ChildRect, Control_Symbol[m_eControlSymbol]);
	m_pControlSymbolIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pControlSymbolIcon);

	// repeat text area
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wLeft + 36, mReal.wTop + 13);
	m_pRepeatPlayModeTextString = new PegString(ChildRect, LS(SID_REPEAT), 0, FF_NONE | TT_COPY );
	m_pRepeatPlayModeTextString->SetFont(&FONT_PLAYSCREEN);
	m_pRepeatPlayModeTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pRepeatPlayModeTextString);

	// random text area
	ChildRect.Set(mReal.wLeft + 37, mReal.wTop, mReal.wLeft + 76, mReal.wTop + 13);
	m_pRandomPlayModeTextString = new PegString(ChildRect, LS(SID_EMPTY_STRING), 0, FF_NONE | TT_COPY );
	m_pRandomPlayModeTextString->SetFont(&FONT_PLAYSCREEN);
	m_pRandomPlayModeTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pRandomPlayModeTextString);

    // zoom text area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 5, mReal.wRight, mReal.wBottom - 15);
	m_pZoomTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pZoomTextString->SetFont(&FONT_PLAYSCREENZOOM);
	m_pZoomTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
	//Add(m_pZoomTextString);
}


void
CPlayerScreen::SynchWithViewMode()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SynchWithViewMode\n");
    switch(m_eViewMode)
    {
    case CDJPlayerState::ZOOM_EXT:
        Remove(m_pTrackTextString);
        Remove(m_pArtistTextString);
        Remove(m_pControlSymbolIcon);
        Remove(m_pTimeTextString);
        Remove(m_pRepeatPlayModeTextString);
        Remove(m_pRandomPlayModeTextString);
        Remove(m_pZoomTextString);
        Add(m_pControlSymbolLargeIcon);
        Add(m_pZoomTimeTextString);
        break;
    case CDJPlayerState::ZOOM:
        Remove(m_pTrackTextString);
        Remove(m_pArtistTextString);
        Remove(m_pControlSymbolIcon);
        Remove(m_pTimeTextString);
        Remove(m_pRepeatPlayModeTextString);
        Remove(m_pRandomPlayModeTextString);
        Remove(m_pControlSymbolLargeIcon);
        Remove(m_pZoomTimeTextString);
        Add(m_pZoomTextString);
        break;
    case CDJPlayerState::NORMAL:
    default:
        Remove(m_pControlSymbolLargeIcon);
        Remove(m_pZoomTimeTextString);
        Remove(m_pZoomTextString);
        Add(m_pTrackTextString);
        Add(m_pArtistTextString);
        Add(m_pControlSymbolIcon);
        Add(m_pTimeTextString);
        Add(m_pRepeatPlayModeTextString);
        Add(m_pRandomPlayModeTextString);
        break;
    }

    SynchTextScrolling();
    if (Presentation()->GetCurrentThing() == this)
        Draw();
}


// Query the media player for its current play state
// and display the appropriate control symbol.
void
CPlayerScreen::SynchControlSymbol()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SynchControlSymbol\n");
	if (m_bScanningForward)
		SetControlSymbol(FAST_FORWARD);
	else if (m_bScanningBackward)
		SetControlSymbol(REWIND);
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
        SetControlSymbol(STOP);
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PAUSED)
        SetControlSymbol(PAUSE);
    else if (CRecordingManager::GetInstance()->IsRecording() || CRecordingManager::GetInstance()->IsRipping())
        if (CRecordingManager::GetInstance()->IsPaused())
            SetControlSymbol(PAUSE);
        else
		    SetControlSymbol(RECORD);
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
        SetControlSymbol(STOP);
    else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        SetControlSymbol(PLAY);
}

// Query the player for its current state
// and display the appropriate status message is the message text area.
void
CPlayerScreen::SynchStatusMessage()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SyncStatMsg\n");

    char szNumber[32];
    TCHAR tszNumber[32];
    TCHAR tszMessage[128];
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    IPlaylistEntry* pCurrentEntry = NULL;
    if (pCurrentPlaylist)
        pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();

    // dvb:  should we be checking the current song source here?  what do we want to show the user here?
    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::HD)
    {
        tstrcpy(tszMessage, LS(SID_MUSIC_SOURCE_COLON_));
        tstrcat(tszMessage, LS(SID_HD));
		SetMessageText(tszMessage, CSystemMessageString::STATUS);
		return;
    }
    else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::FML)
    {
        CIML* pIML = CDJPlayerState::GetInstance()->GetCurrentIML();
        if (pIML)
        {
            tstrcpy(tszMessage, LS(SID_MUSIC_SOURCE_COLON_));
            tstrncat(tszMessage,pIML->GetFriendlyName(), 100);
            SetMessageText(tszMessage, CSystemMessageString::STATUS);
        }
		return;
    }
	else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
	{
        tstrcpy(tszMessage, LS(SID_MUSIC_SOURCE_COLON_));
        tstrcat(tszMessage, LS(SID_LINE_INPUT));
		SetMessageText(tszMessage, CSystemMessageString::STATUS);
		return;
	}

    if (CRecordingManager::GetInstance()->IsRippingSingle())
    {
        if (pCurrentEntry)
        {
            tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
            sprintf(szNumber, " %d ", pCurrentPlaylist->GetEntryIndex(pCurrentEntry) + 1);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_TO_HD));
            SetMessageText(tszMessage, CSystemMessageString::STATUS);
        }
		return;
    }
    else if (CRecordingManager::GetInstance()->IsRipping() && !CRecordingManager::GetInstance()->IsRippingSingle())
    {
        if (pCurrentEntry)
        {
            int iIndex = pCurrentPlaylist->GetEntryIndex(pCurrentEntry);
            int iSize = pCurrentPlaylist->GetSize() + ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->GetQueryRemainingCount();
            if (CRecordingManager::GetInstance()->FindTrackOnHD(pCurrentEntry->GetContentRecord()))
            {
                tstrcpy(tszMessage, LS(SID_ALREADY_HAVE_TRACK));
                sprintf(szNumber, " %d ", iIndex + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_ON_HD));
                SetMessageText(tszMessage, CSystemMessageString::STATUS);
            }
            else
            {
                tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
                sprintf(szNumber, " %d ", iIndex + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_OF));
                sprintf(szNumber, " %d ", iSize);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_TO_HD));
                SetMessageText(tszMessage, CSystemMessageString::STATUS);
            }
        }
		return;
    }
    else if (CRecordingManager::GetInstance()->IsRecording())
    {
        if (pCurrentEntry)
        {
            int iIndex = pCurrentPlaylist->GetEntryIndex(pCurrentEntry);
            if (CRecordingManager::GetInstance()->FindTrackOnHD(pCurrentEntry->GetContentRecord()))
            {
                tstrcpy(tszMessage, LS(SID_ALREADY_HAVE_TRACK));
                sprintf(szNumber, " %d ", iIndex + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_ON_HD));
                SetMessageText(tszMessage, CSystemMessageString::STATUS);
            }
            else
            {
                tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
                sprintf(szNumber, " %d ", iIndex + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_TO_HD));
                SetMessageText(tszMessage, CSystemMessageString::STATUS);
            }
        }
		return;
    }
    else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
    {
        tstrcpy(tszMessage, LS(SID_MUSIC_SOURCE_COLON_));
        tstrcat(tszMessage, LS(SID_CD));
		SetMessageText(tszMessage, CSystemMessageString::STATUS);
        return;
    }
}


void
CPlayerScreen::SynchTextScrolling()
{
    // make sure all the strings are pointing to the right text
    bool m_bScrollingText = false;
    int iScreenWidth = mReal.wRight - mReal.wLeft;
    PegRect NewRect;
    
    if ((m_eViewMode == CDJPlayerState::ZOOM) ||
        (m_eViewMode == CDJPlayerState::ZOOM_EXT))
    {
        int iZoomTextWidth = Screen()->TextWidth(m_pZoomTextString->DataGet(), m_pZoomTextString->GetFont());

        if (iZoomTextWidth > iScreenWidth)
        {
            NewRect = m_pZoomTextString->mReal;
            NewRect.wLeft = mReal.wLeft;
            m_pZoomTextString->Resize(NewRect);
            m_bScrollingText = true;
        }
    }
    else
    {
        int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
        int iTrackTextWidth = Screen()->TextWidth(m_pTrackTextString->DataGet(), m_pTrackTextString->GetFont());

        if (iArtistTextWidth > iScreenWidth)
        {
            NewRect = m_pArtistTextString->mReal;
            NewRect.wLeft = mReal.wLeft;
            m_pArtistTextString->Resize(NewRect);
            m_bScrollingText = true;
        }
        
        if (iTrackTextWidth > iScreenWidth)
        {
            NewRect = m_pTrackTextString->mReal;
            NewRect.wLeft = mReal.wLeft;
            m_pTrackTextString->Resize(NewRect);
            m_bScrollingText = true;
        }
    }

    CDJPlayerState::EUITextScrollSpeed eScroll = CDJPlayerState::GetInstance()->GetUITextScrollSpeed();

    if (m_bScrollingText && eScroll != CDJPlayerState::OFF)
        if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
            SetTimer(PS_TIMER_SCROLL_TEXT, SCROLL_SLOW_START_INTERVAL, SCROLL_SLOW_CONTINUE_INTERVAL);
        else
            SetTimer(PS_TIMER_SCROLL_TEXT, SCROLL_FAST_START_INTERVAL, SCROLL_FAST_CONTINUE_INTERVAL);
    else
        KillTimer(PS_TIMER_SCROLL_TEXT);
}

void
CPlayerScreen::EnablePrebufferProgress()
{
    ResetProgressBar(0,100);

#ifdef DDOMOD_DJ_BUFFERING
    CBuffering::GetInstance()->SetPrebufCallback( &( CPlayerScreen::PrebufferProgressCallback ) );
#endif
}

void
CPlayerScreen::DisablePrebufferProgress() 
{
    ResetProgressBar(0,100);
#ifdef DDOMOD_DJ_BUFFERING
    CBuffering::GetInstance()->SetPrebufCallback( 0 );
#endif
}

void
CPlayerScreen::PrebufferProgressCallback( int PercentDone ) 
{
    if( s_pPlayerScreen ) s_pPlayerScreen->PrebufferProgressCB( PercentDone );
}

void
CPlayerScreen::PrebufferProgressCB( int PercentDone ) 
{
    ResetProgressBar( PercentDone, 100 );
    if( PercentDone == 100 ) DisablePrebufferProgress();
}

bool
CPlayerScreen::ScrollTextFields()
{
    // we don't want to be scrolling when the screen isn't even visible.
    if (Presentation()->GetCurrentThing() != this)
    {
        StopScrollingText();
        return true;
    }

    bool bScrolled = false;
    PegRect NewRect;
    if ((m_eViewMode == CDJPlayerState::ZOOM) ||
        (m_eViewMode == CDJPlayerState::ZOOM_EXT))
    {
        int iZoomTextWidth = Screen()->TextWidth(m_pZoomTextString->DataGet(), m_pZoomTextString->GetFont());
        if (iZoomTextWidth > m_pZoomTextString->mReal.wRight - m_pZoomTextString->mReal.wLeft)
        {
            NewRect = m_pZoomTextString->mReal;
            NewRect.wLeft -= 10;
            m_pZoomTextString->Resize(NewRect);
            bScrolled = true;
        }
    }
    else
    {
        int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
        int iTrackTextWidth = Screen()->TextWidth(m_pTrackTextString->DataGet(), m_pTrackTextString->GetFont());
        if (iArtistTextWidth > m_pArtistTextString->mReal.wRight - m_pArtistTextString->mReal.wLeft)
        {
            NewRect = m_pArtistTextString->mReal;
            NewRect.wLeft -= 5;
            m_pArtistTextString->Resize(NewRect);
            bScrolled = true;
        }
        if (iTrackTextWidth > m_pTrackTextString->mReal.wRight - m_pTrackTextString->mReal.wLeft)
        {
            NewRect = m_pTrackTextString->mReal;
            NewRect.wLeft -= 5;
            m_pTrackTextString->Resize(NewRect);
            bScrolled = true;
        }
    }
    return bScrolled;
}

void
CPlayerScreen::SetTrackStartTime(int iTrackStartTime)
{
    m_iTrackStartTime = iTrackStartTime;
}

// write all persistent members into the buf stream, not to exceed len bytes.
void
CPlayerScreen::SaveState(void* buf, int len)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SaveState\n");
    tPlayerScreenState* state = (tPlayerScreenState*) buf;
    state->eTimeMode = m_eTimeViewMode;
    state->ePLMode = CPlayManager::GetInstance()->GetPlaylistMode();
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:put timemode %d, plmode %d\n",(int)state->eTimeMode,(int)state->ePLMode);
}

void
CPlayerScreen::SetPlayModeTextByPlaylistMode(IPlaylist::PlaylistMode ePLMode)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetPlayModeTextByPlaylistMode\n");
	switch (ePLMode)
	{
        case IPlaylist::NORMAL:		// set normal playlist mode
            m_pRepeatPlayModeTextString->DataSet(LS(SID_EMPTY_STRING));
            m_pRandomPlayModeTextString->DataSet(LS(SID_EMPTY_STRING));
			break;
		case IPlaylist::RANDOM:		// random
            m_pRepeatPlayModeTextString->DataSet(LS(SID_EMPTY_STRING));
            m_pRandomPlayModeTextString->DataSet(LS(SID_RANDOM));
			break;
		case IPlaylist::REPEAT_ALL:		// repeat all
            m_pRepeatPlayModeTextString->DataSet(LS(SID_REPEAT));
            m_pRandomPlayModeTextString->DataSet(LS(SID_EMPTY_STRING));
			break;
		case IPlaylist::REPEAT_RANDOM:		// repeat rand
            m_pRepeatPlayModeTextString->DataSet(LS(SID_REPEAT));
            m_pRandomPlayModeTextString->DataSet(LS(SID_RANDOM));
			break;
        default:
            return;
	}

    if (Presentation()->GetCurrentThing() == this)
        Draw();
    ((CPlayOrderMenuScreen*)CPlayOrderMenuScreen::GetPlayOrderMenuScreen())->SetPlayMode(ePLMode);
}

// read all persistent members from the buf stream, not to exceed len bytes.
void
CPlayerScreen::RestoreState(void* buf, int len)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RestState\n");
    tPlayerScreenState* state = (tPlayerScreenState*)buf;
    SetPlayModeTextByPlaylistMode(state->ePLMode);
    CPlayManager::GetInstance()->SetPlaylistMode(state->ePLMode);
    SetTimeViewMode(state->eTimeMode);
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:got timemode %d, plmode %d\n",(int)state->eTimeMode, (int)state->ePLMode);
}

// specify the byte count allowed in the state stream.
int
CPlayerScreen::GetStateSize()
{
    return sizeof(tPlayerScreenState) + 1;
}

void
CPlayerScreen::SetEventHandlerMode(tPSEventHandlerMode eMode)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SetEventHandlerMode\n");
    if (eMode == m_eEventHandlerMode)
        return;
#ifdef LINE_RECORDER_ENABLED
    switch (eMode)
    {
        case ePSPlaybackEvents:
            if (m_eEventHandlerMode == ePSLineInEvents)
            {
                // entering playback from line-in
                if (CLineRecorder::GetInstance()->InSession())
                {
                    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:LRSessionClose\n"); 
                    CLineRecorder::GetInstance()->CloseSession();
                    CPlayManager::GetInstance()->GetPlaylist()->Clear();
                }
                m_LineInEventHandler.RestorePlayMode();
                m_LineInEventHandler.RestoreTimeViewMode();
                // ???? this calls idlecoder->run, which starts encoding content.
                //      why would we do this here?
                //                m_LineInEventHandler.ResumeIdleCoder();
            }
            SetLEDState(RECORDING_ENABLED, false);
            m_LineInEventHandler.ExitPassthroughMode();
            // Commit the database if any tracks have been added.
            CommitUpdates();

            break;
        case ePSLineRecEvents:
            // entering line-recording mode
            // (epg,2/19/2002): only open a session when the user actually starts recording 
            // something, se we don't pollute the session namespace when
            // they just activate passthrough mode without ever doing any recordings.
            if (!CLineRecorder::GetInstance()->InSession())
            {
                DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:LRSessionOpen\n"); 
                CLineRecorder::GetInstance()->OpenSession();
            }
            SetLEDState(RECORDING, true);
            // close the tray.  if there is a cd in, the recording will cancel.
            CDJPlayerState::GetInstance()->GetCDDataSource()->CloseTray();
            break;
        case ePSLineInEvents:
            // line in will start out in passthrough mode.
            if (m_eEventHandlerMode == ePSPlaybackEvents)
            {
                // entering line-in from playback
                CPlayManager::GetInstance()->Stop();
                CPlayManager::GetInstance()->Deconfigure();
                while (CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::STOPPED && CMediaPlayer::GetInstance()->GetPlayState() != CMediaPlayer::NOT_CONFIGURED)
                    cyg_thread_delay(5);
               	// temancl - audio driver patched up, this call no longer needed
                // cyg_thread_delay(50);
                m_LineInEventHandler.SetPassthroughTrack();
                m_LineInEventHandler.SaveAndNormalizePlayMode();
                m_LineInEventHandler.SaveAndNormalizeTimeViewMode();
                m_LineInEventHandler.StopIdleCoder();
                
            }
            else if (m_eEventHandlerMode == ePSLineRecEvents)
            {
                // entering line-in from line-recording
                // on transition from recording to line-in playback, exit passthrough mode and deactivate passthrough-fiq
                m_LineInEventHandler.ExitPassthroughMode();
                SetLEDState(RECORDING, false);
                // Commit the database if any tracks have been added.
                CommitUpdates();
            }
            SetLEDState(RECORDING, false);
            if (CSpaceMgr::GetInstance()->Status() == SPACE_OK)
                SetLEDState(RECORDING_ENABLED, true);
            break;
		default:
			DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_WARNING, "PS:Unhandled event mode set!\n"); 
    }
#endif
    m_eEventHandlerMode = eMode;
}

bool
CPlayerScreen::StopRipping()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:StopRipping\n");
    if (CRecordingManager::GetInstance()->IsRipping() || 
            CRecordingManager::GetInstance()->IsRecording())
    {
        CRecordingManager::GetInstance()->StopRipping();
        return true;
    }
    else
        return false;
}

void
CPlayerScreen::StopPlayback(bool bForceStop)
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:StopPlayback\n");
    // i've been told that stop never fails ans since we want to set the control
    // symbol quicker we'll just go ahead and set the values that we want right away
    SetControlSymbol(STOP);
    SetTime(m_iTrackStartTime);
    UpdateProgressBar(0);
    m_iTrackTime = m_iTrackStartTime;

    // If the user is currently seeking then cancel the seek by ignoring the next key release.
	if (m_bScanningForwardPrimed || m_bScanningForward || m_bScanningBackwardPrimed || m_bScanningBackward)
    {
        m_bIgnoreNextKeyup = true;
        m_bIgnoreScan = true;
        // clear scanning state.  scanning should be properly ignored by m_bIgnoreScan
        m_bScanningForwardPrimed = false;
	    m_bScanningForward = false;
	    m_bScanningBackwardPrimed = false;
	    m_bScanningBackward = false;
    }
    
    if (bForceStop || (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::STOPPED))
    {
		if(CPlayManager::GetInstance()->GetPlaylist()->GetSize() > 0)
		{
			CPlayManager::GetInstance()->Stop();
		}
		else
		{
			CPlayManager::GetInstance()->Deconfigure();
			// clear current track
			ClearTrack();
			// display playlist message
			SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));	
		}
    }
}

void
CPlayerScreen::ScanForward()
{
    if (!m_bConfigured)
        return;
    if (m_bScanningBackward || m_bScanningBackwardPrimed || m_bIgnoreScan)
        return;
    if (m_bScanningForward)
	{
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:ScanF-poop\n"); 
		if (m_iScanTime + sc_aryScanSpeeds[m_iScanIndex] < m_iTrackStartTime + m_iTrackDuration - 5)
		{
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
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:ScanFCeiling\n"); 
			m_iScanTime = m_iTrackStartTime + m_iTrackDuration - 5;
			// (epg,10/17/2001): TODO: why would the scan time have gotten below 0 here?  is this from scanning backwards?
            //if (m_iScanTime < 0)
			//    m_iScanTime = 0;
		}

		SetTime(m_iScanTime);
        
        // construct our scanning forward system text message
        char szNumber[32];
        TCHAR tszNumber[32];
        TCHAR tszMessage[256];
        tstrcpy(tszMessage, LS(SID_SEEKING_FORWARD));
        sprintf(szNumber, " +%d ", sc_aryScanSpeeds[m_iScanIndex]);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_SECONDS));
        SetMessageText(tszMessage, CSystemMessageString::STATUS);
    }
	else if (m_bScanningForwardPrimed)
	{
        CDJPlayerState::ESourceMode eSource = CDJPlayerState::GetInstance()->GetCurrentSongSource();

        if (eSource == CDJPlayerState::FML)
        {
            SetMessageText(LS(SID_CANT_SEEK_ON_NETWORK_TRACK), CSystemMessageString::REALTIME_INFO);
            m_bIgnoreNextKeyup = true;
        }
        else if (CRecordingManager::GetInstance()->IsRecording() ||
            CRecordingManager::GetInstance()->IsRipping())
        {
            SetMessageText(LS(SID_CANT_SEEK_WHILE_RECORDING), CSystemMessageString::REALTIME_INFO);
            m_bIgnoreNextKeyup = true;
        }
        else
        {
            // if the source isn't line-in, then recording is disabled for scanning.
            if (eSource != CDJPlayerState::LINE_IN)
            {
                if (!m_bSeeked)
                    CRecordingManager::GetInstance()->DisableRecording();
                m_bSeeked = true;
            }

			m_bScanningForward = true;
			m_bScanningForwardPrimed = false;
			m_bScanningBackward = false;
			m_bScanningBackwardPrimed = false;
			m_iScanTime = m_iTrackTime;
			SetControlSymbol(FAST_FORWARD);
        }
	}
	else
	{
        if (!CRecordingManager::GetInstance()->IsRecording() &&
            !CRecordingManager::GetInstance()->IsRipping())
        {
			m_iScanCount = 0;
			// If the track is past the hour mark then start scanning in 60-second intervals.
			if (m_iTrackTime > (m_iTrackStartTime + 3600))
				m_iScanIndex = 1;
			else
				m_iScanIndex = 0;
        }
        m_bIgnoreNextKeyup = false;

        m_bScanningForwardPrimed = true;
	}
}

void
CPlayerScreen::ScanBackward()
{
    if (!m_bConfigured)
        return;

    if (m_bScanningForward || m_bScanningForwardPrimed || m_bIgnoreScan)
        return;
    
	if (m_bScanningBackward)
	{
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:ScanB\n"); 
		if (m_iScanTime - sc_aryScanSpeeds[m_iScanIndex] >= m_iTrackStartTime)
		{
			m_iScanTime -= sc_aryScanSpeeds[m_iScanIndex];
			SetTime(m_iScanTime);
			
            // construct our scanning backward system text message
            char szNumber[32];
            TCHAR tszNumber[32];
            TCHAR tszMessage[256];
            tstrcpy(tszMessage, LS(SID_SEEKING_BACKWARD));
            sprintf(szNumber, " -%d ", sc_aryScanSpeeds[m_iScanIndex]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_SECONDS));
            SetMessageText(tszMessage, CSystemMessageString::STATUS);
            
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
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:ScanBFloor\n"); 
			m_iScanTime = m_iTrackStartTime;
			SetTime(m_iTrackStartTime);
            
            // construct our scanning backward system text message
            char szNumber[32];
            TCHAR tszNumber[32];
            TCHAR tszMessage[256];
            tstrcpy(tszMessage, LS(SID_SEEKING_BACKWARD));
            sprintf(szNumber, " -%d ", sc_aryScanSpeeds[m_iScanIndex]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_SECONDS));
            SetMessageText(tszMessage, CSystemMessageString::STATUS);
		}
	}
	else if (m_bScanningBackwardPrimed)
	{
        CDJPlayerState::ESourceMode eSource = CDJPlayerState::GetInstance()->GetCurrentSongSource();

        if (eSource == CDJPlayerState::FML)
        {
            SetMessageText(LS(SID_CANT_SEEK_ON_NETWORK_TRACK), CSystemMessageString::REALTIME_INFO);
            m_bIgnoreNextKeyup = true;
        }
        else if (CRecordingManager::GetInstance()->IsRecording() ||
            CRecordingManager::GetInstance()->IsRipping())
        {
            SetMessageText(LS(SID_CANT_SEEK_WHILE_RECORDING), CSystemMessageString::REALTIME_INFO);
            m_bIgnoreNextKeyup = true;
        }
        else
        {
            // if the source isn't line-in, then recording is disabled for scanning.
            if (eSource != CDJPlayerState::LINE_IN)
            {
                if (!m_bSeeked)
                    CRecordingManager::GetInstance()->DisableRecording();
                m_bSeeked = true;
            }

            m_bScanningForward = false;
			m_bScanningForwardPrimed = false;
			m_bScanningBackward = true;
			m_bScanningBackwardPrimed = false;
			m_iScanTime = m_iTrackTime;
			SetControlSymbol(REWIND);
        }
	}
	else
	{
        if (!CRecordingManager::GetInstance()->IsRecording() &&
            !CRecordingManager::GetInstance()->IsRipping())
        {
			m_iScanCount = 0;
			// If the track is past the hour mark then start scanning in 60-second intervals.
			if (m_iTrackTime > (m_iTrackStartTime + 3600))
				m_iScanIndex = 1;
			else
				m_iScanIndex = 0;
        }
        m_bIgnoreNextKeyup = false;
        m_bScanningBackwardPrimed = true;
	}
}

void 
CPlayerScreen::EnterQuickBrowseMenu(int iCurrentTrackOffset = 0)
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:EnterQBrowse\n"); 
  	if(CPlayManager::GetInstance()->GetPlaylist()->GetSize() > 0)
	{
		CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->RefreshPlaylist(iCurrentTrackOffset, true);
		Add(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen());
	}
	else
		SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));
}

void
CPlayerScreen::EnterSavePlaylistScreen()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:EnterSavePL\n");
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if( pCurrentPlaylist->GetSize() > 0 )
    {
        CDJPlayerState::ESourceMode source = CDJPlayerState::GetInstance()->GetSource();
        
        if( source == CDJPlayerState::LINE_IN )
            SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT));
        else if( source == CDJPlayerState::CD )
            SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_CD_DRIVE));
        else
        {
            CPlaylistSaveScreen* pPSS = (CPlaylistSaveScreen*)CPlaylistSaveScreen::GetPlaylistSaveScreen();
            CScreen* pCurrentScreen = (CScreen*)Presentation()->GetCurrentThing();
            pPSS->SetParent(pCurrentScreen);
            pPSS->SetupList(0);
            pCurrentScreen->Add(pPSS);
            Presentation()->MoveFocusTree(pPSS);
        }
    }
    else
        SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));
}

bool 
CPlayerScreen::Configured()
{ 
    return m_bConfigured;
}

void 
CPlayerScreen::RemoveCurrentPlaylistEntry()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RemoveCurrentPlaylistEntry\n");
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (!pCurrentPlaylist)
        return;
    IPlaylistEntry* pPlayingEntry = pCurrentPlaylist->GetCurrentEntry();
    int iCurrentPlaylistSize = CPlayManager::GetInstance()->GetPlaylist()->GetSize();
    if(iCurrentPlaylistSize > 0)
    {
        // go to the next track if we have more than one (this current track)
        if(iCurrentPlaylistSize > 1)
            DebounceNextTrack();
        // then delete the playlist entry
        pCurrentPlaylist->DeleteEntry(pPlayingEntry);
        // reload the current track info
        RefreshCurrentTrackMetadata();
        // let the quickbrowse screen know
        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
        // then check for no tracks state
        if(CPlayManager::GetInstance()->GetPlaylist()->GetSize() == 0)
        {
            // stop recording/ripping if this is the last track
            CRecordingManager* pRM = CRecordingManager::GetInstance();
            if (pRM->IsRecording())
                pRM->StopRecording();
            if (pRM->IsRipping())
                pRM->StopRipping();
            // stop
            CPlayManager::GetInstance()->Deconfigure();
            // clear current track
            ClearTrack();
            // display playlist message
            DisplaySelectTracksScreen();
            SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));	
        }
    }
    else
    {
        DisplaySelectTracksScreen();
        SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));						
    }
}

void 
CPlayerScreen::ToggleCDRip()
{
    CRecordingManager* pRM = CRecordingManager::GetInstance();

    // Check space and track limits before proceeding.
    if (FAILED(pRM->CheckSpaceAndTrackLimits()))
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:No space on drive\n");
        return;
    }

    // We can't record if the user has seeked in the track.
    if (m_bSeeked)
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:Can't record after seek\n");
        SetMessageText(LS(SID_CANT_RECORD_AFTER_SEEKING_ON_A_TRACK), CSystemMessageString::REALTIME_INFO);
        SetMessageText(LS(SID_RESTART_TRACK_TO_ENABLE_RECORDING));
        return;
    }

    // We can't record if this is a radio stream.
    IPlaylistEntry* pEntry = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();
    if (pEntry && pRM->IsRadioStream(pEntry->GetContentRecord()))
    {
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:Can't record radio stream\n");
        SetMessageText(LS(SID_CANT_RECORD_INTERNET_RADIO), CSystemMessageString::REALTIME_INFO);
        return;
    }

    if (CDJPlayerState::GetInstance()->GetSource() != CDJPlayerState::LINE_IN)
    {
        // If we're ripping, stop.
        if (pRM->IsRipping())
        {
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:RipOff\n"); 
            pRM->StopRipping();
            CPlayManager::GetInstance()->Stop();
        }
        // If we're recording, stop.
        else if (pRM->IsRecording())
        {
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:RecOff\n"); 
            // Print the stopped recording messages only if we were going to keep the track.
            pRM->StopRecording(pRM->IsKeepingCurrentTrack());
            // Hide the progress bar.
            UpdateProgressBar(0);
        }
        // If we're stopped, start a full rip.
        else if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
        {
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:RipOn\n"); 
            // Start the full CD rip.
            if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
                pRM->StartRipFull();
            else
                pRM->StartRipPlaylist();
        }
        // If we're playing or paused, start recording.
        else if ((CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING) ||
            (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PAUSED))
        {
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:RecOn\n"); 
            // Start recording the current track.
            pRM->StartRecording();
            // Show the progress bar, if we're not recording something already on disk.
            if (pRM->IsKeepingCurrentTrack())
                UpdateProgressBar(m_iTrackTime);
        }
        SynchControlSymbol();
        SynchStatusMessage();
    }
}

void
CPlayerScreen::EndScanOrNextTrack()
{
    m_bIgnoreScan = false;

    if (!m_bConfigured)
        return;
    if( m_bScanningBackward || m_bScanningBackwardPrimed )
        return;

    m_bScanningForwardPrimed = false;
    if (m_bScanningForward)
	{
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:SkF\n"); 
		m_bScanningForward = false;
		SynchControlSymbol();
        SynchStatusMessage();
        // m_bIgnoreNextKeyup is true if the user was seeking during a track change.
        // In that case, ignore the seek command.
        if (!m_bIgnoreNextKeyup)
        {
		    if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime-m_iTrackStartTime+1)))
			    m_iTrackTime = m_iScanTime;
		    else
			    SetTime(m_iTrackTime);
        }
		else
			SetTime(m_iTrackTime);
	}
    else
    {
        if (!m_bIgnoreNextKeyup)
        {
            DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:NextTk\n"); 
            if(CPlayManager::GetInstance()->GetPlaylist()->GetSize() > 0)
                DebounceNextTrack();
            else
			{
				// stop
				CPlayManager::GetInstance()->Deconfigure();
				// clear current track
				ClearTrack();
				// display playlist message
				SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));						
			}
            SynchStatusMessage();
        }
    }
}

void
CPlayerScreen::EndScanOrPrevTrack()
{
    m_bIgnoreScan = false;

    if (!m_bConfigured)
        return;
    if( m_bScanningForward || m_bScanningForwardPrimed )
        return;

    m_bScanningBackwardPrimed = false;
   	if (m_bScanningBackward)
	{
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:SkB\n"); 
		m_bScanningBackward = false;
		SynchControlSymbol();
        SynchStatusMessage();
        // m_bIgnoreNextKeyup is true if the user was seeking during a track change.
        // In that case, ignore the seek command.
        if (!m_bIgnoreNextKeyup)
        {
            if (SUCCEEDED(CPlayManager::GetInstance()->Seek(m_iScanTime-m_iTrackStartTime+1)))
			    m_iTrackTime = m_iScanTime;
		    else
			    SetTime(m_iTrackTime);
        }
		else
			SetTime(m_iTrackTime);
	}
	else
	{
		// always stop, don't reset current track
        if (!m_bIgnoreNextKeyup)
        {
			if(CPlayManager::GetInstance()->GetPlaylist()->GetSize() > 0)
                DebouncePreviousTrack();
			else
			{
				// stop
				CPlayManager::GetInstance()->Deconfigure();
				// clear current track
				ClearTrack();
				// display playlist message
				SetMessageText(LS(SID_EMPTY_CURRENT_PLAYLIST));						
			}
            SynchStatusMessage();
        }
    }
}

void
CPlayerScreen::SynchSymbolsForStop()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:SynchSymbolsForStop\n");
	SynchControlSymbol();
    SynchStatusMessage();
	if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED)
	{
		SetTime(m_iTrackStartTime);
        UpdateProgressBar(0);
		m_iTrackTime = m_iTrackStartTime;
	}
}

void
CPlayerScreen::RefreshCurrentTrackMetadata()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:RefreshCurrentTrackMetadata\n");
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMetadata* pMetadata = pCurrentEntry->GetContentRecord();
            if (pMetadata)
            {
                if (CDJPlayerState::GetInstance()->GetUIShowTrackNumInTitle())
                {
                    // prepend the track's playlist number to the title
                    // get the track number
                    char szTrackNumber[32];
                    sprintf(szTrackNumber, "%d. ", pCurrentEntry->GetIndex() + 1);
                    // slap the two together
                    TCHAR* pszTrackText = GetMetadataString(pMetadata, MDA_TITLE);
                    TCHAR* pszTrackNumberAndText = (TCHAR*) malloc( (strlen(szTrackNumber) + tstrlen(pszTrackText) + 1) * sizeof(TCHAR));
                    if (pszTrackNumberAndText)
                    {
                        CharToTchar(pszTrackNumberAndText, szTrackNumber);
                        tstrcat(pszTrackNumberAndText, pszTrackText);
                        SetTrackText(pszTrackNumberAndText);
                    }
                    else
                        SetTrackText(GetMetadataString(pMetadata, MDA_TITLE));

                    free(pszTrackNumberAndText);
                }
                else
                    SetTrackText(GetMetadataString(pMetadata, MDA_TITLE));

                if (CDJPlayerState::GetInstance()->GetUIShowAlbumWithArtist())
                {
                    TCHAR* pszAlbumText = GetMetadataString(pMetadata, MDA_ALBUM);
                    if (pszAlbumText)
                    {
                        TCHAR* pszArtistText = GetMetadataString(pMetadata, MDA_ARTIST);
                        TCHAR* pszAlbumWithArtistText = (TCHAR*) malloc( (tstrlen(pszAlbumText) + tstrlen(LS(SID__DASH_)) + tstrlen(pszArtistText) + 1) * sizeof(TCHAR));
                        // check with an assert?
                        tstrcpy(pszAlbumWithArtistText, pszAlbumText);
                        tstrcat(pszAlbumWithArtistText, LS(SID__DASH_));
                        tstrcat(pszAlbumWithArtistText, pszArtistText);
                        SetArtistText(pszAlbumWithArtistText, true);
                        free(pszAlbumWithArtistText);
                    }
                    else
                        SetArtistText(GetMetadataString(pMetadata, MDA_ARTIST), true);
                }
                else
                    SetArtistText(GetMetadataString(pMetadata, MDA_ARTIST), true);
            }
        }
        else
            ClearTrack();
    }
    else
        ClearTrack();
}

void
CPlayerScreen::SetTimerForScrollEnd()
{
	KillTimer(PS_TIMER_SCROLL_TEXT);
    if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
		SetTimer(PS_TIMER_SCROLL_END, SCROLL_SLOW_END_INTERVAL, 0);
    else
        SetTimer(PS_TIMER_SCROLL_END, SCROLL_FAST_END_INTERVAL, 0);
}

void
CPlayerScreen::UpdateTrackProgress(const PegMessage &Mesg)
{
    if( CPlayerScreen::s_pPlayerScreen && this != CPlayerScreen::s_pPlayerScreen ) {
        // this undoubtedly leads to fatal issues down the line, or is a symptom of them
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "\n!!!!!!!!!! UpdateTrackProgress with wrong this ptr !!!!!!!!!!!!!!\n");
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "UpdateTrackProgress: Mesg.lData = %d\n", Mesg.lData);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "CPlayerScreen::s_pPlayerScreen = 0x%x\n", CPlayerScreen::s_pPlayerScreen);
        DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_FATAL, "                          this = 0x%x\n\n", this);

        // dvb 10/10/2002.  use an assert to help track down this strangeness
        DBASSERT( DBG_PLAYER_SCREEN, this == CPlayerScreen::s_pPlayerScreen, "UpdateTrackProgress with wrong this ptr");
        /*
        CPlayerScreen::s_pPlayerScreen->UpdateTrackProgress(Mesg);
        return ;
        */
    }
    
	if (!m_bScanningBackward && !m_bScanningForward && 
        ((CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::STOPPED) || CRecordingManager::GetInstance()->IsRipping() || (Mesg.lData == 0)))
    {
		SetTime(Mesg.lData + m_iTrackStartTime);
        if ((CRecordingManager::GetInstance()->IsRecording() || CRecordingManager::GetInstance()->IsRipping())
            && CRecordingManager::GetInstance()->IsKeepingCurrentTrack())
            UpdateProgressBar(Mesg.lData);
        else
            UpdateProgressBar();
    }
}

// (epg,2/18/2002): TODO: rename this, depending on whatever it's actually doing
void
CPlayerScreen::PerformFunkyIRResponse()
{
    PegRect Rect;
    Rect.Set(mReal.wRight - 3, mReal.wTop + 5, mReal.wRight, mReal.wTop + 8);
    Invalidate(Rect);
    KillTimer(PS_TIMER_SCROLL_TEXT);
    BeginDraw();
    Line(mReal.wRight - 3, mReal.wTop + 5, mReal.wRight, mReal.wTop + 5, WHITE, 4);
    EndDraw();
}

tPSEventHandlerMode 
CPlayerScreen::GetEventHandlerMode()
{
    return m_eEventHandlerMode;
}

// notify the user that their recording was stopped due to low space
void CPlayerScreen::NotifyUserRecordingCutoff()
{
    DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_INFO, "ps:NtfyUsrRecCutoff\n"); 
    SetMessageText(LS(SID_NO_SPACE_LEFT_ON_HD_FOR_RECORDING), CSystemMessageString::REALTIME_INFO);
    SetMessageText(LS(SID_DELETE_TRACKS_TO_ENABLE_RECORDING));
}

void
CPlayerScreen::DoTrackChange()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_INFO, "ps:DoTC\n");

    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
        ERESULT res = MP_ERROR;

        // If the user is just rewinding the current track (and not actually going to the
        // previous track) and we're currently in single-track rip mode, then restart
        // the single track rip (because we need to turn off buffering for this track).
        if (pCurrentEntry && m_bBacktrackIfNeeded && CRecordingManager::GetInstance()->IsRippingSingle())
        {
            res = CRecordingManager::GetInstance()->StartRipSingle(pCurrentEntry);
        }

        if (FAILED(res))
        {
            if (m_bBacktrackIfNeeded)
                res = DJSetCurrentOrPrevious(true);
            else
                res = DJSetCurrentOrNext(true);
        }

        if (FAILED(res))
        {
            if (CRecordingManager::GetInstance()->IsRipping())
            {
                CRecordingManager::GetInstance()->StopRipping();
                CPlayManager::GetInstance()->Stop();
                pCurrentEntry = pCurrentPlaylist->GetEntry(0);
                if (pCurrentEntry)
                {
                    pCurrentPlaylist->SetCurrentEntry(pCurrentEntry);
                    if (FAILED(DJSetCurrentOrNext()))
                    {
                        DisplayNoContentScreen();
                        SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                    }
                }
                else
                {
                    DisplayNoContentScreen();
                    SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                }
            }

/*
                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0,IPlaylist::NORMAL));
                IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
                if (FAILED(DJSetSong(pCurrentEntry)))
                {
                    if (FAILED(DJNextTrack()))
                    {
                        DisplayNoContentScreen();
                        SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                    }
                }
*/
        }
    }

    SynchControlSymbol();
    m_bDebounceTrackChange = false;
}

void
CPlayerScreen::DebounceNextTrack()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:DebounceNT\n");
    m_iTrackDuration = 0;
    m_iTrackStartTime = 0;
    ResetProgressBar(0, 0);
    SetTime(0, true);
    SetControlSymbol(NEXT_TRACK);
    m_bBacktrackIfNeeded = false;

    // reset the timer if it's still active
    KillTimer(PS_TIMER_DO_TRACK_CHANGE);
    
    m_bDoingTrackChange = true;

    // If in single-track ripping mode, then stop!
    bool bStop = false;
    if (CRecordingManager::GetInstance()->IsRippingSingle())
    {
        CRecordingManager::GetInstance()->StopRipping();
        CPlayManager::GetInstance()->Stop();
        bStop = true;
    }
    // If in single-track recording mode, then stop recording but continue playback.
    else if (CRecordingManager::GetInstance()->IsRecordingSingle())
    {
        CRecordingManager::GetInstance()->StopRecording();
    }

    // stop the media player
    if(CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        CMediaPlayer::GetInstance()->Deconfigure();

    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pNextEntry = pCurrentPlaylist->SetNextEntry(CPlayManager::GetInstance()->GetPlaylistMode());

        // if pNextEntry == NULL then it's most likely the end of the playlist
        if(!pNextEntry)
        {
            pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0, CPlayManager::GetInstance()->GetPlaylistMode()));
            pNextEntry = pCurrentPlaylist->GetCurrentEntry();
        }

        if(pNextEntry)
        {
            SetTrackInfo(pNextEntry);

            // Important.  Set the timer to actually do the track change.
            // Wait a bit longer if the user has been pressing the button quickly
            if(m_bDebounceTrackChange)
                SetTimer(PS_TIMER_DO_TRACK_CHANGE, sc_iLongTrackChangeInterval, 0);
            else
            {
                m_bDebounceTrackChange = true;
                SetTimer(PS_TIMER_DO_TRACK_CHANGE, sc_iShortTrackChangeInterval, 0);
            }
        }
        else
        {
            // bad.  we couldn't set an entry in the playlist
            SynchControlSymbol();
            bStop = true;
        }
    }
    else
        bStop = true;

    if (bStop)
        CPlayManager::GetInstance()->Stop();
}

void
CPlayerScreen::DebouncePreviousTrack()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:DebouncePT\n");
    int iTrackTime = m_iTrackTime, iTrackStartTime = m_iTrackStartTime;
    m_iTrackDuration = 0;
    m_iTrackStartTime = 0;
    ResetProgressBar(0, 0);
    SetTime(0, true);
    SetControlSymbol(PREVIOUS_TRACK);
    m_bBacktrackIfNeeded = true;

    // reset the timer if it's still active
    KillTimer(PS_TIMER_DO_TRACK_CHANGE);
    
    m_bDoingTrackChange = true;

    // Distinguish rewind from previous track.
    bool bPreviousTrack = (iTrackTime <= (iTrackStartTime+5));

    // If in single-track ripping mode, then stop!
    bool bStop = false;
    if (CRecordingManager::GetInstance()->IsRippingSingle() && bPreviousTrack)
    {
        CRecordingManager::GetInstance()->StopRipping();
        CPlayManager::GetInstance()->Stop();
        bStop = true;
    }
    // If in single-track recording mode, then stop recording but continue playback.
    else if (CRecordingManager::GetInstance()->IsRecordingSingle() && bPreviousTrack)
    {
        CRecordingManager::GetInstance()->StopRecording();
    }
    
    // stop the media player
    if(CMediaPlayer::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
        CMediaPlayer::GetInstance()->Deconfigure();

    if (bPreviousTrack)
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:PrevTk\n" );
        
        IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
        if (pCurrentPlaylist)
        {
            IPlaylistEntry* pPreviousEntry = pCurrentPlaylist->SetPreviousEntry(CPlayManager::GetInstance()->GetPlaylistMode());

            // if pNextEntry == NULL then it's most likely the end of the playlist
            if(!pPreviousEntry && !pCurrentPlaylist->IsEmpty())
            {
                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry((pCurrentPlaylist->GetSize() - 1), CPlayManager::GetInstance()->GetPlaylistMode()));
                pPreviousEntry = pCurrentPlaylist->GetCurrentEntry();
            }

            if(pPreviousEntry)
            {
                SetTrackInfo(pPreviousEntry);

                // Important.  Set the timer to actually do the track change.
                // Wait a bit longer if the user has been pressing the button quickly
                if(m_bDebounceTrackChange)
                    SetTimer(PS_TIMER_DO_TRACK_CHANGE, sc_iLongTrackChangeInterval, 0);
                else
                {
                    m_bDebounceTrackChange = true;
                    SetTimer(PS_TIMER_DO_TRACK_CHANGE, sc_iShortTrackChangeInterval, 0);
                }
            }
            else
            {
                // bad.  we couldn't set an entry in the playlist
                SynchControlSymbol();
                bStop = true;
            }
        }
        else
            bStop = true;

        if (bStop)
            CPlayManager::GetInstance()->Stop();
    }
    else
    {
        DEBUGP( DBG_PLAYER_SCREEN , DBGLEV_TRACE, "ps:SeekBOF\n"); 
        m_bDebounceTrackChange = true;
        SetTime(iTrackStartTime);
        SetTimer(PS_TIMER_DO_TRACK_CHANGE, sc_iShortTrackChangeInterval, 0);
    }
}

void
CPlayerScreen::ShowCurrentTrackInfo()
{
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:ShowCurrentTrackInfo\n");
    HideMenus();
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
	if (!pCurrentPlaylist)
    {
        // todo: tell the user that there's no current playlist
		return;
    }

    // we have a current playlist, now we try to get the current entry
    IPlaylistEntry* pCurrentEntry = NULL;
    pCurrentEntry = pCurrentPlaylist->GetCurrentEntry();
    if (!pCurrentEntry)
    {
        // todo: tell the user that there's no current entry
        return;
    }

    CInfoMenuScreen::GetInfoMenuScreen()->SetTrackInfo(pCurrentEntry->GetContentRecord());
    CInfoMenuScreen::GetInfoMenuScreen()->SetParent(this);
    Add(CInfoMenuScreen::GetInfoMenuScreen());
    Presentation()->MoveFocusTree(CInfoMenuScreen::GetInfoMenuScreen());
}

void
CPlayerScreen::MutePlayback()
{    
#ifndef DISABLE_VOLUME_CONTROL
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:MutePlayback\n");            
   
    if (CVolumeControl::GetInstance()->IsMuted())
    {
        SetMessageText("Restoring Volume", CSystemMessageString::STATUS);        
        CVolumeControl::GetInstance()->ToggleMute(VOLUME_RATE);        
    }
    else
    {
        SetMessageText("Muting Jukebox", CSystemMessageString::STATUS); 
        CVolumeControl::GetInstance()->ToggleMute(VOLUME_RATE);       
    }
#endif // DISABLE_VOLUME_CONTROL
}

void
CPlayerScreen::ToggleZoom()
{    
    DEBUGP( DBG_PLAYER_SCREEN, DBGLEV_TRACE, "ps:ToggleZoom\n");            
   
    // Toggle between zoom and normal ui mode
    switch (CDJPlayerState::GetInstance()->GetUIViewMode())
    {
        case CDJPlayerState::NORMAL:
        {
            CDJPlayerState::GetInstance()->SetUIViewMode(CDJPlayerState::ZOOM, true);
            break;
        }
        case CDJPlayerState::ZOOM:
        {
            CDJPlayerState::GetInstance()->SetUIViewMode(CDJPlayerState::ZOOM_EXT, true);
            break;
        }
        case CDJPlayerState::ZOOM_EXT:
        {
            CDJPlayerState::GetInstance()->SetUIViewMode(CDJPlayerState::NORMAL, true);
            break;
        }
        default:
        {
            CDJPlayerState::GetInstance()->SetUIViewMode(CDJPlayerState::NORMAL, true);
            break;
        }
    }
}


CLineRecEventHandler* CPlayerScreen::GetLineRecEventHandler()
{
    return &m_LineRecEventHandler;
}
CLineInEventHandler* CPlayerScreen::GetLineInEventHandler()
{
    return &m_LineInEventHandler;
}
CPlaybackEventHandler* CPlayerScreen::GetPlaybackEventHandler()
{
    return &m_PlaybackEventHandler;
}

