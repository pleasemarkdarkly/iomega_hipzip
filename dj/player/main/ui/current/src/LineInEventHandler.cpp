#include <_modules.h>
#include <main/ui/LineInEventHandler.h>
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
#include <main/ui/AlertScreen.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/PlayOrderMenuScreen.h>

#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/Recording.h>
#include <main/main/AppSettings.h>
#include <main/main/LineRecorder.h>
#include <main/main/FatHelper.h>

#include <main/main/DJPlayerState.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <main/main/RecordingEvents.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Strings.hpp>

#include <extras/IdleCoder/IdleCoder.h>

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferInStream.h>
#endif

#include <fs/fat/sdapi.h>

#include <main/ui/CDTriageScreen.h>

#include <devs/audio/dai.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_LINEIN_EVENTS, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_LINEIN_EVENTS );  // debugging prefix : (2) lie

// (epg,2/25/2002): TODO: switch out fiq switcher code with real when available
#define PASSTHROUGH_FIQ_ENABLED (1)
#define PS_ADC_CONTROLS (1)

#define DOUBLE_CLICK_TIMEOUT_TICKS (200)

CLineInEventHandler::CLineInEventHandler() :
    m_bPassthroughMode(false),
    m_bIgnoreNewTrack(false),
    m_nReRecordPrime(0),
    m_nReRecordPrimeTimeout(0),
    /* m_pIdleCoderPickle(NULL),*/
    m_iRwdCount(0),
    m_iFwdCount(0)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:ctor\n"); 
}

CLineInEventHandler::~CLineInEventHandler() 
{
    m_pPS = NULL;
}

SIGNED
CLineInEventHandler::HandleKeyPlay(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kplay\n"); 
    // if not on end track, then play the previously recorded track.
    if ((!m_bPassthroughMode) && (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING))
    {
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:playing\n"); 
        CPlayManager::GetInstance()->Play();
        m_pPS->SetControlSymbol(CPlayerScreen::PLAY);
    }
    else
    {
        if (m_bPassthroughMode) {
            DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:NoPlay Passthru\n"); 
        }
        else {
            DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:NoPlay PlayManMode %d\n",CPlayManager::GetInstance()->GetPlayState()); 
        }
    }
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyPause(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kpause\n"); 
    // unless we're in passthrough mode, pause.
    if (m_bPassthroughMode)
        return 0;
    m_pPS->TogglePause();
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyStop(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kstop\n"); 
    if (m_bPassthroughMode)
    {
#if PS_ADC_CONTROLS
        // on passthrough track, up will mean gain up.
        // (epg,3/11/2002): remove for demo
        //CLineRecorder::GetInstance()->ToggleMicBoost();
#endif
        return 0;
    }
    // stop playback
    m_pPS->StopPlayback();
    // normalize the status message to the player core state
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyExit(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kcncl\n"); 

#if DEBUG_LEVEL != 0
#ifdef DDOMOD_DJ_BUFFERING
    MaybeTurnOnBufferDebug();
#endif
#endif

    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyFwd(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kfwd\n"); 
    if (m_bPassthroughMode) {
        m_iFwdCount = (m_iFwdCount < 3) ? ++m_iFwdCount : 3;
        return 0;
    }
    m_pPS->ScanForward();
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyRew(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:krew\n"); 
    if (m_bPassthroughMode) {
        m_iRwdCount = (m_iRwdCount < 3) ? ++m_iRwdCount : 3;
        return 0;
    }
    m_pPS->ScanBackward();
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyUp(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kup\n"); 

#if PS_ADC_CONTROLS
    // on passthrough track, up will mean gain up.
    if (m_bPassthroughMode)
    {
        CLineRecorder::GetInstance()->IncrementGain();
        // give a gain report in the system message text.
        DisplayGainNotification();
        return 0;
    }
#endif
    // (epg,2/19/2002): this will be really close to right, since all the real (previously recorded) tracks will be in the current playlist,
    // but the passthrough track (ie direct linein source monitoring) won't be automatically represented therein, so maybe some ugly special case in either
    // the current playlist, or else the browse menu itself.

    // start at the quick browse menu
    m_pPS->EnterQuickBrowseMenu(-1);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyDown(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kdown\n"); 

#if PS_ADC_CONTROLS
    // on passthrough track, dwn will mean gain dwn
    if (m_bPassthroughMode)
    {
        CLineRecorder::GetInstance()->DecrementGain();
        // give a gain report in the system message text.
        DisplayGainNotification();
        return 0;
    }
#endif
    // (epg,2/19/2002): this will be really close to right, since all the real (previously recorded) tracks will be in the current playlist,
    // but the passthrough track (ie direct linein source monitoring) won't be automatically represented therein, so maybe some ugly special case in either
    // the current playlist, or else the browse menu itself.

    // start at the quick browse menu
    m_pPS->EnterQuickBrowseMenu(1);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyUp10(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_TRACE, "pe:kup10\n");
#if PS_ADC_CONTROLS
    if (m_bPassthroughMode)
        return 0;
#endif
    m_pPS->EnterQuickBrowseMenu(-10);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyDown10(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_TRACE, "pe:kdown10\n");
#if PS_ADC_CONTROLS
    if (m_bPassthroughMode)
        return 0;
#endif
    m_pPS->EnterQuickBrowseMenu(10);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyNext(const PegMessage &Mesg)
{
    m_pPS->EnterQuickBrowseMenu(0);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeySave(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:ksave\n"); 
    // (epg,2/19/2002): possibly disallow if on passthrough (?) 
    // ... or always in this mode, I'm not sure of the logic here.
    // ... I'll disable for now, it seems weird that they would save off part of a session rather than just trigger a new session...

    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    
    //m_pPS->EnterSavePlaylistScreen();
	return 0;
}

SIGNED
CLineInEventHandler::HandleKeyClear(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kclear\n"); 
    // (epg,2/19/2002): we could give them a way to start a new session via this message, but not yet.
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyDelete(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kdel\n"); 
    // (epg,2/19/2002): this sounds keen for trimming out an entry, except that it would cause weird gaps in the session.  I think the 
    // right thing is to go ahead and delete the entry (with sysmsg confirmation) and then just allow the namespace gap.
    
    //m_pPS->RemoveCurrentPlaylistEntry();

    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyAdd(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kadd\n"); 
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyMenu(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kmen\n"); 
    // (epg,2/19/2002): make sure there are no inconsistencies between the menu and line-in modality b/c of special mode handling (think not...)

    m_pPS->Add(CMainMenuScreen::GetMainMenuScreen());
    return 0;
}

// prompt the user to hit record again to re-record over the current entry.
void 
CLineInEventHandler::PrimeRecordDoubleClick()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:PrimeRecDblClk\n"); 
    m_pPS->SetMessageText(LS(SID_PRESS_RECORD_TWICE_TO_RE_RECORD), CSystemMessageString::REALTIME_INFO);
    m_nReRecordPrime = 1;
    m_nReRecordPrimeTimeout = cyg_current_time() + DOUBLE_CLICK_TIMEOUT_TICKS;
}

SIGNED 
CLineInEventHandler::HandleKeyRecord(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:krec\n"); 

    if (m_bPassthroughMode)
    {
        if (FAILED(CRecordingManager::GetInstance()->CheckSpaceAndTrackLimits()))
        {
            DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Not enough space\n" );
            return 0;
        }

        // start a new track
        m_pPS->SetEventHandlerMode(ePSLineRecEvents);
        m_pPS->HideMenus();

        // This would get set in PS->SetTrack with other event handlers, but we need to do it
        // ourselves here since we won't SetTrack until recording is done
        m_pPS->m_bDoingTrackChange = false;
        
        CLineRecorder::GetInstance()->StartRecording();
    }
    else
    {
        m_pPS->HideMenus();
        if (m_nReRecordPrime == 2)
        {
            if (cyg_current_time() < m_nReRecordPrimeTimeout)
            {
                // If we are re-recording, we will be at the track limit, but we still need to make sure
                // we aren't at the space limit. unfortunately this expects the space to be checked before
                // the track limit in CheckSpaceAndTrackLimits() (which can't multiplex return values)
                // update: call it with messages disabled so we dont get the track limit crap. if we find we have
                //  no more space, call it again with messages enabled so we get consistency
                if( CRecordingManager::GetInstance()->CheckSpaceAndTrackLimits(false) == RECORDING_NO_SPACE )
                {
                    CRecordingManager::GetInstance()->CheckSpaceAndTrackLimits(true);
                    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Not enough space\n" );
                    return 0;
                }

                // re-record over this track
                m_pPS->SetMessageText(LS(SID_STARTED_RECORDING_THE_CURRENT_TRACK), CSystemMessageString::REALTIME_INFO);
                // stop playback, or passthrough fiq will crash.
                CPlayManager::GetInstance()->Deconfigure();
#ifdef DDOMOD_DJ_BUFFERING
                CBuffering::GetInstance()->DetachFromPlaylist();
#endif
                // delete the highlighted track and start a new recording in its place.
                IPlaylistEntry* entry = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry();
                if (!entry) {
                    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_WARNING, "LIE:unexpected no crnt entry in re-record!\n"); 
                    return 0;
                }
                const char* url = entry->GetContentRecord()->GetURL();
                pc_unlink( (char*)FilenameFromURLInPlace(url));
                EnterPassthroughMode();

                // This would get set in PS->SetTrack with other event handlers, but we need to do it
                // ourselves here since we won't SetTrack until recording is done.  Not sure if this is
                // as necessary as it is above since we're already sitting on a track.
                m_pPS->m_bDoingTrackChange = false;
                
                CLineRecorder::GetInstance()->StartRecording(entry->GetIndex()+1);
                m_pPS->SetEventHandlerMode(ePSLineRecEvents);
            }
            else
                // re prime the double click
                PrimeRecordDoubleClick();
        }
        else if (m_nReRecordPrime == 0)
            PrimeRecordDoubleClick();
    }

    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyGenre(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kgen\n");
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Goto LibraryMS in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyArtist(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kart\n");
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Goto LibraryMS in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyAlbum(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kalb\n");
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Goto LibraryMS in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyPlaylist(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kplst\n");
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Goto LibraryMS in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyRadio(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:krdo\n");
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Goto LibraryMS in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeySource(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:ksrc\n"); 
#if 0
    // tg #512, disable this until it can be debugged
    m_pPS->HideMenus();
    m_pPS->CycleKeySources();
#else
    // tg #512, go into the Menu Source Screen for now
    m_pPS->HideMenus();
    ((CSourceMenuScreen*)CSourceMenuScreen::GetSourceMenuScreen())->RefreshSource();
    m_pPS->Add(CSourceMenuScreen::GetSourceMenuScreen());
    m_pPS->Presentation()->MoveFocusTree(CSourceMenuScreen::GetSourceMenuScreen());
#endif
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyInfo(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kinfo - NOT IMPLEMENTED\n");
    // tg #795: If in pass through mode (on a soon to be recorded track) do not show info, consistent
    //          with first entering line in mode
    if( !m_bPassthroughMode ) {
        m_pPS->ShowCurrentTrackInfo();
    }
    return 0;
}

SIGNED
CLineInEventHandler::HandleKeyPlayMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:kplymd\n"); 
    DEBUGP( DBG_LINEIN_EVENTS, DBGLEV_INFO, "lie:Can't Change Play Order in Line Input Mode\n");
    m_pPS->SetMessageText(LS(SID_CANT_CHANGE_PLAY_MODE_OF_LINE_INPUT_SOURCE), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyZoom(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kzoom\n"); 
    m_pPS->ToggleZoom();
    return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyFwdRelease(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:kfwdRel\n"); 

#if DEBUG_LEVEL != 0
#ifdef DDOMOD_DJ_BUFFERING
    ResetBufferDebugCount();
#endif
#endif

    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    
    if (m_bPassthroughMode)
    {
        if (m_iFwdCount >= 3) {
            m_iFwdCount = 0;
            return 0;
        }
        else {
            m_iFwdCount = 0;
            if (CPlayManager::GetInstance()->GetPlaylist()->GetSize() == 0)
                return 0;
            // exit the passthrough mode
            SetFirstPlaylistEntry();
        }
    }
    else if ( (!m_pPS->m_bScanningForward) 
        && pl->GetEntryIndex(pl->GetCurrentEntry()) == (pl->GetSize() - 1) )
	{
        // stop playback for passthrough fiq.
        CPlayManager::GetInstance()->Deconfigure();        
        // enter passthrough mode
		m_pPS->SetControlSymbol(CPlayerScreen::NEXT_TRACK);
		m_pPS->m_bScanningForwardPrimed = false;
        SetPassthroughTrack();
	}
    else
        m_pPS->EndScanOrNextTrack();
    
    return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyRewRelease(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:krewRel\n");

    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    
    if (m_bPassthroughMode)
    {
        if (m_iRwdCount >= 3) {
            m_iRwdCount = 0;
            return 0;
        }
        else {
            m_iRwdCount = 0;
            if (CPlayManager::GetInstance()->GetPlaylist()->GetSize() == 0)
                return 0;
            // exit the passthrough mode
            SetLastPlaylistEntry();
        }
    }
    else if ( (!m_pPS->m_bScanningBackward) &&
        pl->GetEntryIndex(pl->GetCurrentEntry()) == 0 )
    {
         // stop playback for passthrough fiq.
        CPlayManager::GetInstance()->Deconfigure();        
        // enter passthrough mode
		m_pPS->SetControlSymbol(CPlayerScreen::NEXT_TRACK);
		m_pPS->m_bScanningForwardPrimed = false;
        SetPassthroughTrack();
    }
    else 
        m_pPS->EndScanOrPrevTrack();
    return 0;
}

SIGNED 
CLineInEventHandler::HandleKeyRecordRelease(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:krecRel\n"); 
    
    if (m_nReRecordPrime == 1)
        // record this part of the double click
        m_nReRecordPrime = 2;
    return 0;
}

SIGNED 
CLineInEventHandler::HandleMsgPlay(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:play\n"); 
	m_pPS->SynchControlSymbol();
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED 
CLineInEventHandler::HandleMsgStop(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:stop\n"); 
    m_pPS->SynchSymbolsForStop();
    return 0;
}
SIGNED 
CLineInEventHandler::HandleMsgNewTrack(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:newtk\n");
    if (m_bIgnoreNewTrack)
    {
        m_bIgnoreNewTrack = false;
    }
    else
    {
        set_track_message_t* stm = (set_track_message_t*)Mesg.pData;
        ExitPassthroughMode();
        m_pPS->SetTrack(stm);
        delete stm;
    }
    return 0;
}

SIGNED 
CLineInEventHandler::HandleMsgSystemMessage(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:sysmsg\n"); 
    if(Mesg.pData) // the TCHAR
    {
        m_pPS->SetMessageText((TCHAR*)Mesg.pData, (CSystemMessageString::SysMsgType)Mesg.iData);
        free((TCHAR*)Mesg.pData);  // allocated in CPEGUserInterface::SetMessage
    }
    return 0;
}

SIGNED 
CLineInEventHandler::HandleMsgRefreshMetadata(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:rfrshMD\n"); 
    // (epg,2/19/2002): I imagine this won't happen throughout this mode?
    if (m_bPassthroughMode)
        return 0;
    m_pPS->RefreshCurrentTrackMetadata();
    return 0;
}

SIGNED 
CLineInEventHandler::HandleMsgSetUIViewMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:setVM\n"); 
    m_pPS->SetViewMode((CDJPlayerState::EUIViewMode)Mesg.iData, (bool)Mesg.lData);
    return 0;
}

SIGNED
CLineInEventHandler::HandleMsgMusicSourceChanged(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:SrcChngd\n"); 
    m_pPS->NotifyMusicSourceChanged((CDJPlayerState::ESourceMode)Mesg.iData);
    return 1;
}

SIGNED
CLineInEventHandler::HandleTimerScrollTitle(const PegMessage& Mesg)
{
    // (epg,2/19/2002): possibly removeable since the default metadata used probably won't need to scroll?
	if (!m_pPS->ScrollTextFields())
	{
        m_pPS->SetTimerForScrollEnd();
	}
	else
		m_pPS->Draw();
    return 0;
}

SIGNED
CLineInEventHandler::HandleTimerScrollEnd(const PegMessage& Mesg)
{
    // (epg,2/19/2002): possibly removeable since the default metadata used probably won't need to scroll?
	m_pPS->SynchTextScrolling();
	m_pPS->Draw();
    return 0;
}

SIGNED
CLineInEventHandler::HandleTimerDoTrackChange(const PegMessage& Mesg)
{
	m_pPS->DoTrackChange();
    return 0;
}

SIGNED
CLineInEventHandler::HandleTrackProgress(const PegMessage& Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_TRACE, "lie:prgrs\n");
    m_pPS->UpdateTrackProgress(Mesg);
    return 0;
}

SIGNED 
CLineInEventHandler::HandleTimerIR(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:ir\n"); 
    // (epg,2/19/2002): if I new what this was, I might take it out :)
    m_pPS->PerformFunkyIRResponse();
    return 0;
}

SIGNED
CLineInEventHandler::DispatchEvent(const PegMessage &Mesg)
{
    switch (Mesg.wType)
	{
		case PM_KEY:
			//DEBUG_KEYBOARD_EVENTS("PEG PM_KEY: %d\n", Mesg.iData);
			switch (Mesg.iData)
			{
				case IR_KEY_PLAY:
				case KEY_PLAY:
                    return HandleKeyPlay(Mesg);
                case IR_KEY_PAUSE:
				case KEY_PAUSE:
                    return HandleKeyPause(Mesg);
                case IR_KEY_STOP:
				case KEY_STOP:
                    return HandleKeyStop(Mesg);
                case IR_KEY_EXIT:
				case KEY_EXIT:
                    return HandleKeyExit(Mesg);
                case IR_KEY_FWD:
				case KEY_FWD:
                    return HandleKeyFwd(Mesg);
                case IR_KEY_REW:
				case KEY_REW:
                    return HandleKeyRew(Mesg);
                case IR_KEY_PREV:
					break;
                case IR_KEY_NEXT:
                    return HandleKeyNext(Mesg);
                case IR_KEY_UP:
				case KEY_UP:
                    return HandleKeyUp(Mesg);
                case IR_KEY_DOWN:
				case KEY_DOWN:
                    return HandleKeyDown(Mesg);
                case IR_KEY_CHANNEL_UP:
                    return HandleKeyUp10(Mesg);
                case IR_KEY_CHANNEL_DOWN:
                    return HandleKeyDown10(Mesg);
                case IR_KEY_SAVE:
                    return HandleKeySave(Mesg);
                case IR_KEY_CLEAR: 
                    return HandleKeyClear(Mesg);
                case IR_KEY_DELETE:
                    return HandleKeyDelete(Mesg);
                case IR_KEY_ADD:
                    return HandleKeyAdd(Mesg);
                case IR_KEY_MENU:
				case KEY_MENU:
                    return HandleKeyMenu(Mesg);
                case IR_KEY_RECORD:
                case KEY_RECORD:
                    return HandleKeyRecord(Mesg);
                case IR_KEY_GENRE:
                    return HandleKeyGenre(Mesg);
                case IR_KEY_ARTIST:
                    return HandleKeyArtist(Mesg);
                case IR_KEY_ALBUM:
                    return HandleKeyAlbum(Mesg);
                case IR_KEY_PLAYLIST:
                    return HandleKeyPlaylist(Mesg);
                case IR_KEY_RADIO:
                    return HandleKeyRadio(Mesg);
                case IR_KEY_SOURCE:
                    return HandleKeySource(Mesg);
                case IR_KEY_PLAY_MODE:
                    return HandleKeyPlayMode(Mesg);
                case IR_KEY_EDIT:
                case IR_KEY_INFO:
                    return HandleKeyInfo(Mesg);
                case IR_KEY_ZOOM:
                    return HandleKeyZoom(Mesg);
				default:
					break;
			}
			break;
		case PM_KEY_RELEASE:
			switch (Mesg.iData)
			{
                case IR_KEY_FWD:
				case KEY_FWD:
                    return HandleKeyFwdRelease(Mesg);
                case IR_KEY_REW:
				case KEY_REW:
                    return HandleKeyRewRelease(Mesg);
                case IR_KEY_RECORD:
                case KEY_RECORD:
                    return HandleKeyRecordRelease(Mesg);
			}
			break;
		case IOPM_PLAY:
            return HandleMsgPlay(Mesg);
		case IOPM_STOP:
            return HandleMsgStop(Mesg);
        case IOPM_NEW_TRACK:
            return HandleMsgNewTrack(Mesg);
        case IOPM_CLEAR_TRACK:
            m_pPS->ClearTrack();
            break;
		case IOPM_TRACK_PROGRESS:
            HandleTrackProgress(Mesg);
			break;
		case IOPM_PLAYLIST_LOADED:
            // get the playlist name
			break;
		case IOPM_TRACK_END:
			break;
        case IOPM_SYSTEM_MESSAGE:
            return HandleMsgSystemMessage(Mesg);
        case IOPM_REFRESH_METADATA:
            return HandleMsgRefreshMetadata(Mesg);
        case IOPM_MULTIPLE_METADATA:
            break;
        case IOPM_MEDIA_INSERTED:
            break;
        case IOPM_MEDIA_REMOVED:
            break;
        case IOPM_ADC_CLIP_DETECTED:
            return HandleMsgClipDetected(Mesg);
            break;
        case IOPM_SET_UI_VIEW_MODE:
            return HandleMsgSetUIViewMode(Mesg);
        case IOPM_MUSIC_SOURCE_CHANGED:
            return HandleMsgMusicSourceChanged(Mesg);
        case PM_TIMER:
			switch (Mesg.iData)
			{
			    case PS_TIMER_SCROLL_TEXT:
                    return HandleTimerScrollTitle(Mesg);
                case PS_TIMER_SCROLL_END:
                    return HandleTimerScrollEnd(Mesg);
                case PS_TIMER_IR:
                    return HandleTimerIR(Mesg);
                case PS_TIMER_DO_TRACK_CHANGE:
                    return HandleTimerDoTrackChange(Mesg);
                case LIS_TIMER_CLIP_CHECK:
                    CLineRecorder::GetInstance()->MonitorClip(true);
                    break;
    		}
		default:    
			return m_pPS->CScreen::Message(Mesg);
	}
	return 0;
}

void 
CLineInEventHandler::EnterPassthroughMode(bool bHideMenus)
{
    if (!m_bPassthroughMode)
    {
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:PassthroughOn\n"); 
        // complain if we're playing back content, a no-no.
        DBASSERT( DBG_LINEIN_EVENTS, CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING, "LIE: PlayerCore playing on EnterPassthroughMode!");
        // get rid of the qbrowse menu, since that opens us up to weirdness.
        if (bHideMenus)
            m_pPS->HideMenus();
#if PASSTHROUGH_FIQ_ENABLED
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:MonitorFiq ON\n");  
		CLineRecorder::GetInstance()->InitDAIForRecording();
        m_pPS->SetTimer(LIS_TIMER_CLIP_CHECK, 150, 150);  // check every second
        DAISetLoopbackFIQ();
#endif
        m_pPS->m_iTrackDuration = 0;
        m_bPassthroughMode      = true;
        m_bIgnoreNewTrack       = true;
    }
}

void 
CLineInEventHandler::ExitPassthroughMode()
{
    if (m_bPassthroughMode)
    {
#if PASSTHROUGH_FIQ_ENABLED
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:MonitorFiq OFF\n");  
        DAISetNormalFIQ();
        m_pPS->KillTimer(LIS_TIMER_CLIP_CHECK);
#endif
        m_bPassthroughMode = false;
        m_bIgnoreNewTrack  = false;
    }
}

void
CLineInEventHandler::MutePassthroughMode()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:MutePassthrough\n"); 
    if (m_bPassthroughMode)
    {
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:MonitorFiq OFF\n");  
        DAISetNormalFIQ();
    }
}

void
CLineInEventHandler::UnMutePassthroughMode()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:UnMutePassthrough\n"); 
    if (m_bPassthroughMode)
    {
        DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:MonitorFiq ON\n");  		
        DAISetLoopbackFIQ();
    }
}

void 
CLineInEventHandler::SetLastPlaylistEntry()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:setLastPLE\n"); 
    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    pl->SetCurrentEntry(pl->GetEntry(pl->GetSize()-1,IPlaylist::NORMAL));
    CPlayManager::GetInstance()->SetSong(pl->GetCurrentEntry());
    ExitPassthroughMode();
}

void 
CLineInEventHandler::SetFirstPlaylistEntry()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:setFirstPLE\n"); 
    IPlaylist* pl = CPlayManager::GetInstance()->GetPlaylist();
    pl->SetCurrentEntry(pl->GetEntry(0,IPlaylist::NORMAL));
    CPlayManager::GetInstance()->SetSong(pl->GetCurrentEntry());
    ExitPassthroughMode();
}

void 
CLineInEventHandler::SetPassthroughTrack(bool bHideMenus)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:setPassThru\n");

    // Drop a track change if we make it to passthrough mode.  This prevents
    // a PT, NT quick combo from jumping into passthrough only to immediately
    // exit it.
    m_pPS->KillTimer(PS_TIMER_DO_TRACK_CHANGE);
    m_pPS->m_bDebounceTrackChange = false;

    m_pPS->ClearTrack();
    m_pPS->SetArtistText(LS(SID_LINEIN_MONITOR_ARTIST));
    m_pPS->SetTrackText(LS(SID_LINEIN_MONITOR_TRACKNAME));
    // re-throttle time display in case it got a late message.
    m_pPS->SetTime(0);
    EnterPassthroughMode(bHideMenus);
	m_pPS->SetControlSymbol(CPlayerScreen::STOP);
}

void 
CLineInEventHandler::InitPlayerScreenPtr(CPlayerScreen* ps)
{
    m_pPS = ps;
}

void
CLineInEventHandler::SaveAndNormalizePlayMode()
{
    m_eGlobalPlaylistMode = CPlayManager::GetInstance()->GetPlaylistMode();
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:SavePLMode %d\n",m_eGlobalPlaylistMode); 
    CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
    m_pPS->SetPlayModeTextByPlaylistMode(IPlaylist::NORMAL);
}

void
CLineInEventHandler::SaveAndNormalizeTimeViewMode()
{
    // dvb (07/23/02) taken out for now.  leave in since it might come back
    //m_eGlobalTimeViewMode = m_pPS->GetTimeViewMode();
    //DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:SaveTimeMode %d\n",m_eGlobalTimeViewMode); 
    //m_pPS->SetTimeViewMode(CTimeMenuScreen::TRACK_ELAPSED);
    //m_pPS->SetTrackStartTime(0);
}

void
CLineInEventHandler::RestorePlayMode()
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:RestPLMode %d\n",m_eGlobalPlaylistMode); 
    CPlayManager::GetInstance()->SetPlaylistMode(m_eGlobalPlaylistMode);
    m_pPS->SetPlayModeTextByPlaylistMode(m_eGlobalPlaylistMode);
}

void
CLineInEventHandler::RestoreTimeViewMode()
{
    // dvb (07/23/02) taken out for now.  leave in since it might come back
    //m_pPS->SetTimeViewMode(m_eGlobalTimeViewMode);
    //DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:RestTimeMode %d\n",m_eGlobalTimeViewMode); 
}

void 
CLineInEventHandler::DisplayGainNotification()
{
    char szNumber[32];
    TCHAR tszNumber[32];
    TCHAR tszMessage[128];
    tstrcpy(tszMessage, LS(SID_LINE_INPUT_GAIN_NOW));
    sprintf(szNumber, " +%ddB ", CLineRecorder::GetInstance()->GetGain());
    CharToTcharN(tszNumber, szNumber, 31);
    tstrcat(tszMessage, tszNumber);
    m_pPS->SetMessageText(tszMessage, CSystemMessageString::REALTIME_INFO);
}

void 
CLineInEventHandler::StopIdleCoder()
{
    // (epg,4/22/2002): it looks to me like there is a ton of ram usage that is declared
    // static inside PEM, so we have to just halt the idle coder's operations, abandoning
    // the current in-process file.  longer term, I need to rework pem maybe, but I'll just get
    // it running for now..
    // dc 5/2/02, halt may be as good as it will get; pem doesn't contain its state to the
    // regular pem structures, so savings its state is a chore.
    CIdleCoder::GetInstance()->Halt();
    
    /*
    CIdleCoder::GetInstance()->Pause();
    if (m_pIdleCoderPickle)
        delete m_pIdleCoderPickle;
    m_pIdleCoderPickle = new unsigned char [IDLE_CODER_PICKLE_BYTES];
    memcpy ((void*)m_pIdleCoderPickle,(void*)SRAM_START,IDLE_CODER_PICKLE_BYTES);
    */
}

void
CLineInEventHandler::ResumeIdleCoder()
{
    /*
    if (m_pIdleCoderPickle)
    {
        memcpy ((void*)SRAM_START,(void*)m_pIdleCoderPickle,IDLE_CODER_PICKLE_BYTES);
        delete m_pIdleCoderPickle;
        m_pIdleCoderPickle = NULL;
    }
    */
    CIdleCoder::GetInstance()->Run();
}

SIGNED 
CLineInEventHandler::HandleMsgClipDetected(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEIN_EVENTS , DBGLEV_INFO, "lie:Clip\n");
    m_pPS->SetMessageText(LS(SID_LINE_INPUT_CLIP_DETECTED), CSystemMessageString::REALTIME_INFO);
    return 0;
}
