#include <main/ui/PlaybackEventHandler.h>
#include <main/ui/Timers.h>

#include <main/ui/UI.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlaylistConstraint.h>

#include <main/ui/InfoMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/LibraryEntryMenuScreen.h>
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

#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>

#include <datasource/cddatasource/CDDataSource.h>
#include <main/main/DJHelper.h>
#include <main/main/DJPlayerState.h>
#include <main/main/RecordingEvents.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>

#include <main/ui/CDTriageScreen.h>
#include <main/ui/SystemMessageString.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_PLAYBACK_EVENTS, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PLAYBACK_EVENTS );   // debugging prefix : (1) pe

CPlaybackEventHandler::CPlaybackEventHandler()
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:ctor\n"); 

    m_pDJPS = CDJPlayerState::GetInstance();
    m_pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
    m_pLEMS = (CLibraryEntryMenuScreen*)CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen();
}

CPlaybackEventHandler::~CPlaybackEventHandler() 
{
    m_pPS = NULL;
    m_pLMS = NULL;
    m_pLEMS = NULL;

}

SIGNED
CPlaybackEventHandler::HandleKeyPlay(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kplay\n"); 
    // If the CD tray is open, then interpret the play command as a close tray command.
    if (m_pDJPS->GetCDDataSource()->IsTrayOpen() && !DebounceButton(KEY_CD_EJECT, 500))
    {
        m_pPS->NotifyCDTrayClosed();
        m_pDJPS->GetCDDataSource()->CloseTray();

        if (m_pDJPS->GetSource() == CDJPlayerState::CD)
        {
            m_pPS->SynchControlSymbol();
            m_pPS->SynchStatusMessage();
            return 0;
        }
    }
    else if ((CRecordingManager::GetInstance()->IsRipping()) || (!m_pPS->Configured()))
        return 0;
    
    if (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::PLAYING)
    {
        CPlayManager::GetInstance()->Play();
    }
    m_pPS->SynchControlSymbol();
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyPause(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kpause\n"); 
    m_pPS->TogglePause();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyStop(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kstop\n"); 
    if (!m_pPS->Configured())
        return 0;
    // stop ripping
    bool bForceStop = m_pPS->StopRipping();
    // stop playback
    m_pPS->StopPlayback(bForceStop);
    // normalize the status message to the player core
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyExit(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kcncl\n"); 
    // Todo:  we should see if we're trying to do something that the user wants to cancel out of.
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyFwd(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kfwd\n"); 
    m_pPS->ScanForward();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyRew(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:krew\n"); 
    m_pPS->ScanBackward();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyUp(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kup\n"); 
    // start at the quick browse menu
    m_pPS->EnterQuickBrowseMenu(-1);
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyDown(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kdown\n"); 
    // start at the quick browse menu
    m_pPS->EnterQuickBrowseMenu(1);
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyUp10(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS, DBGLEV_TRACE, "pe:kup10\n");
    m_pPS->EnterQuickBrowseMenu(-10);
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyDown10(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS, DBGLEV_TRACE, "pe:kdown10\n");
    m_pPS->EnterQuickBrowseMenu(10);
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyRight(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS, DBGLEV_TRACE, "pe:kright\n");
    m_pPS->EnterQuickBrowseMenu(0);
}

SIGNED
CPlaybackEventHandler::HandleKeySave(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:ksave\n"); 
    if (m_pDJPS->GetSource() != CDJPlayerState::CD)
        m_pPS->EnterSavePlaylistScreen();
    else
        m_pPS->SetMessageText(LS(SID_CANT_SAVE_PLAYLIST_OF_UNRECORDED_CD_TRACKS), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyClear(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kclear\n"); 
    // we used to clear the playlist.   now we've chosen to do nothing.
    // the user can clear the playlist from the quickbrowse screen
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyDelete(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kdel\n"); 
    if ((m_pDJPS->GetSource() == CDJPlayerState::CD) && (m_pDJPS->GetCDState() == CDJPlayerState::AUDIO))
        m_pPS->SetMessageText(LS(SID_CANT_REMOVE_AUDIO_CD_TRACKS), CSystemMessageString::REALTIME_INFO);
    else
        m_pPS->RemoveCurrentPlaylistEntry();
	return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyAdd(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kadd\n"); 
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE), CSystemMessageString::REALTIME_INFO);
	return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyMenu(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kmenu\n"); 
    m_pPS->Add(CMainMenuScreen::GetMainMenuScreen());
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyRecord(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:krec\n"); 

    CDJPlayerState* pState = CDJPlayerState::GetInstance();
    // If the CD tray is open, then interpret the record command as a close tray command.
    if (m_pDJPS->GetCDDataSource()->IsTrayOpen() && !DebounceButton(KEY_CD_EJECT, 500))
    {
        m_pPS->NotifyCDTrayClosed();
        pState->GetCDDataSource()->CloseTray();

        if (pState->GetSource() == CDJPlayerState::CD)
        {
            m_pPS->SynchControlSymbol();
            m_pPS->SynchStatusMessage();
            return 0;
        }
    }
    if( pState->GetSource() != CDJPlayerState::LINE_IN ) {
        m_pPS->ToggleCDRip();
    }
    return 0;
}

// Checks to see if the CD is currently browsable.
bool
CPlaybackEventHandler::CanBrowseCD()
{
    // Don't let the user browse while the CD is being scanned.
    if (m_pDJPS->IsScanningCD())
    {
        m_pPS->SetMessageText(LS(SID_CANT_BROWSE_WHILE_SCANNING_DATA_CD), CSystemMessageString::REALTIME_INFO);
        return false;
    }
    // Don't let the user browse if there's no CD in the drive.
    else if (m_pDJPS->GetCDState() == CDJPlayerState::NONE)
    {
        m_pPS->SetMessageText(LS(SID_NO_CD), CSystemMessageString::REALTIME_INFO);
        return false;
    }
    // Don't let the user browse audio CDs.
    else if (m_pDJPS->GetCDState() == CDJPlayerState::AUDIO)
    {
        m_pPS->SetMessageText(LS(SID_CANT_BROWSE_AUDIO_CD), CSystemMessageString::REALTIME_INFO);
        return false;
    }
    return true;
}

SIGNED
CPlaybackEventHandler::HandleKeyGenre(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kgen\n");
    if (m_pLMS->GetBrowseSource() == CDJPlayerState::CD && !CanBrowseCD())
        return 0;

    // NOTE:  since CSourceMenuScreen::HideScreen() resets what we're browsing
    //        we set aside what we want to browse ahead of time and then
    //        reload what we want to browse
    CInfoMenuScreen* pIMS = CInfoMenuScreen::GetInfoMenuScreen();
    PegThing* pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (pCurrentThing == m_pLMS || pCurrentThing == m_pLEMS || (pCurrentThing == pIMS && pIMS->Parent() == m_pLMS))
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPS->GetSource();
        pIML = m_pDJPS->GetCurrentIML();
    }

    m_pLMS->SetBrowseSource(eSource);
    m_pLMS->SetBrowseIML(pIML);
    m_pLMS->SetBrowseMode(CLibraryMenuScreen::GENRE, true);
    m_pLMS->SetConstraints();
    m_pLMS->ResetAndRefresh();

    pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    if (pCurrentThing != m_pLMS)
    {
        m_pPS->Add(m_pLMS);
        m_pPS->Presentation()->MoveFocusTree(m_pLMS);
    }

    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyArtist(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kart\n");
    if (m_pLMS->GetBrowseSource() == CDJPlayerState::CD && !CanBrowseCD())
        return 0;

    CInfoMenuScreen* pIMS = CInfoMenuScreen::GetInfoMenuScreen();
    PegThing* pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (pCurrentThing == m_pLMS || pCurrentThing == m_pLEMS || (pCurrentThing == pIMS && pIMS->Parent() == m_pLMS))
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPS->GetSource();
        pIML = m_pDJPS->GetCurrentIML();
    }

    m_pLMS->SetBrowseSource(eSource);
    m_pLMS->SetBrowseIML(pIML);
    m_pLMS->SetBrowseMode(CLibraryMenuScreen::ARTIST, true);
    m_pLMS->SetConstraints();
    m_pLMS->ResetAndRefresh();

    pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    if (pCurrentThing != m_pLMS)
    {
        m_pPS->Add(m_pLMS);
        m_pPS->Presentation()->MoveFocusTree(m_pLMS);
    }

    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyAlbum(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kalb\n");
    if (m_pLMS->GetBrowseSource() == CDJPlayerState::CD && !CanBrowseCD())
        return 0;

    CInfoMenuScreen* pIMS = CInfoMenuScreen::GetInfoMenuScreen();
    PegThing* pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (pCurrentThing == m_pLMS || pCurrentThing == m_pLEMS || (pCurrentThing == pIMS && pIMS->Parent() == m_pLMS))
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPS->GetSource();
        pIML = m_pDJPS->GetCurrentIML();
    }

    m_pLMS->SetBrowseSource(eSource);
    m_pLMS->SetBrowseIML(pIML);
    m_pLMS->SetBrowseMode(CLibraryMenuScreen::ALBUM, true);
    m_pLMS->SetConstraints();
    m_pLMS->ResetAndRefresh();

    pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    if (pCurrentThing != m_pLMS)
    {
        m_pPS->Add(m_pLMS);
        m_pPS->Presentation()->MoveFocusTree(m_pLMS);
    }

    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyPlaylist(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kplst\n");
    if (m_pLMS->GetBrowseSource() == CDJPlayerState::CD && !CanBrowseCD())
        return 0;

    CInfoMenuScreen* pIMS = CInfoMenuScreen::GetInfoMenuScreen();
    PegThing* pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (pCurrentThing == m_pLMS || pCurrentThing == m_pLEMS || (pCurrentThing == pIMS && pIMS->Parent() == m_pLMS))
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPS->GetSource();
        pIML = m_pDJPS->GetCurrentIML();
    }

    m_pLMS->SetBrowseSource(eSource);
    m_pLMS->SetBrowseIML(pIML);
    m_pLMS->SetBrowseMode(CLibraryMenuScreen::PLAYLIST, true);
    m_pLMS->SetConstraints();
    m_pLMS->ResetAndRefresh();

    pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    if (pCurrentThing != m_pLMS)
    {
        m_pPS->Add(m_pLMS);
        m_pPS->Presentation()->MoveFocusTree(m_pLMS);
    }

    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyRadio(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:krdo\n");

    CInfoMenuScreen* pIMS = CInfoMenuScreen::GetInfoMenuScreen();
    PegThing* pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (pCurrentThing == m_pLMS || pCurrentThing == m_pLEMS || (pCurrentThing == pIMS && pIMS->Parent() == m_pLMS))
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPS->GetSource();
        pIML = m_pDJPS->GetCurrentIML();
    }

    // if we're not in FML mode, look for the first FML and use it's radio stations
    if (eSource == CDJPlayerState::HD || eSource == CDJPlayerState::CD)
    {
        eSource = CDJPlayerState::FML;
        if (CIMLManager::GetInstance()->GetCachedIMLCount() > 0)
            pIML = CIMLManager::GetInstance()->GetCachedIMLByIndex(0);
        else
            pIML = NULL;
    }

    if (pIML)
    {
        m_pLMS->SetBrowseSource(eSource);
        m_pLMS->SetBrowseIML(pIML);
        m_pLMS->SetBrowseMode(CLibraryMenuScreen::RADIO, true);
        m_pLMS->SetConstraints();
        m_pLMS->ResetAndRefresh();

        pCurrentThing = m_pPS->Presentation()->GetCurrentThing();
        if (pCurrentThing != m_pLMS)
        {
            m_pPS->Add(m_pLMS);
            m_pPS->Presentation()->MoveFocusTree(m_pLMS);
        }
    }
    else
    {
        m_pPS->SetMessageText(LS(SID_INTERNET_RADIO_NOT_AVAILABLE), CSystemMessageString::REALTIME_INFO);
    }

    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeySource(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:ksrc\n");
#if 0
    // tg #512, disable this until it can be debugged
    m_pPS->HideMenus();
    m_pPS->CycleKeySources();
#else
    // tg #512, go into the Menu Source Screen for now
    ((CSourceMenuScreen*)CSourceMenuScreen::GetSourceMenuScreen())->RefreshSource();
    m_pPS->Add(CSourceMenuScreen::GetSourceMenuScreen());
    m_pPS->Presentation()->MoveFocusTree(CSourceMenuScreen::GetSourceMenuScreen());
#endif
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyPlayMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kplymd\n"); 
    
    // Playmode changes in place...
    CPlayManager* pPlayManager = CPlayManager::GetInstance();
    CPlayerScreen* pPlayerScreen = CPlayerScreen::GetPlayerScreen();
    IPlaylist::PlaylistMode ePLMode = pPlayManager->GetPlaylistMode();
    
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:Playmode: %d.\n", (int)ePLMode); 
    
    // the user can't change the play order from normal while ripping an entire cd
    if (CRecordingManager::GetInstance()->IsRipping() && !CRecordingManager::GetInstance()->IsRippingSingle())
    {
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:refuse playmode change\n"); 
        m_pPS->SetMessageText(LS(SID_CANT_CHANGE_PLAY_MODE_WHILE_FAST_RECORDING));
        return 0;
    }

    if (ePLMode == IPlaylist::REPEAT_RANDOM)
        ePLMode = IPlaylist::NORMAL;	
    else
        (int)ePLMode += 1; // Hack increments for enumerations...
    
    switch (ePLMode)
    {
    case (IPlaylist::NORMAL):		
        pPlayManager->SetPlaylistMode(IPlaylist::NORMAL);
        pPlayerScreen->SetPlayModeTextByPlaylistMode(IPlaylist::NORMAL);		
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:IR setting playmode Normal\n"); 
        break;
    case (IPlaylist::RANDOM):
        pPlayManager->SetPlaylistMode(IPlaylist::RANDOM);
        pPlayerScreen->SetPlayModeTextByPlaylistMode(IPlaylist::RANDOM);		
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:IR setting playmode Random\n"); 
        break;
    case (IPlaylist::REPEAT_ALL):
        pPlayManager->SetPlaylistMode(IPlaylist::REPEAT_ALL);
        pPlayerScreen->SetPlayModeTextByPlaylistMode(IPlaylist::REPEAT_ALL);	
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:IR setting playmode Repeat All\n"); 
        break;
    case(IPlaylist::REPEAT_RANDOM):
        pPlayManager->SetPlaylistMode(IPlaylist::REPEAT_RANDOM);
        pPlayerScreen->SetPlayModeTextByPlaylistMode(IPlaylist::REPEAT_RANDOM);		
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:IR setting playmode Repeat Random\n"); 
        break;
    default:
        DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:Question Reality.\n"); 
    }
    
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleKeyInfo(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kinfo\n"); 
    m_pPS->ShowCurrentTrackInfo();
    return 0;
}

SIGNED CPlaybackEventHandler::HandleKeyMute(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kMute\n"); 
    m_pPS->MutePlayback();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyZoom(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_INFO, "pe:kzoom\n"); 
    m_pPS->ToggleZoom();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyFwdRelease(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:kfwdRel\n"); 
    m_pPS->EndScanOrNextTrack();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleKeyRewRelease(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:krewRel\n"); 
    m_pPS->EndScanOrPrevTrack();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleMsgPlay(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:play\n"); 
	m_pPS->SynchControlSymbol();
    m_pPS->SynchStatusMessage();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleMsgStop(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:stop\n"); 
    m_pPS->SynchSymbolsForStop();
    return 0;
}
SIGNED 
CPlaybackEventHandler::HandleMsgNewTrack(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:newTrack\n"); 
    set_track_message_t* stm = (set_track_message_t*)Mesg.pData;
    m_pPS->SetTrack(stm);
    delete stm;
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleMsgPlayMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:msgplymd\n"); 
    
    CPlayManager* pPlayManager = CPlayManager::GetInstance();
    CPlayerScreen* pPlayerScreen = CPlayerScreen::GetPlayerScreen();
    IPlaylist::PlaylistMode ePLMode = (IPlaylist::PlaylistMode)Mesg.iData;

    if (pPlayManager)
        pPlayManager->SetPlaylistMode(ePLMode);

    if (pPlayerScreen)
        pPlayerScreen->SetPlayModeTextByPlaylistMode(ePLMode);
    
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleMsgSystemMessage(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:sysmsg\n"); 
    if(Mesg.pData) // the TCHAR
    {
        m_pPS->SetMessageText((TCHAR*)Mesg.pData, (CSystemMessageString::SysMsgType)Mesg.iData);
        free((TCHAR*)Mesg.pData);  // allocated in CPEGUserInterface::SetMessage
    }
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleMsgRefreshMetadata(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:rfrshMD\n"); 
    m_pPS->RefreshCurrentTrackMetadata();
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleMsgMultipleMetadata(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:multMD\n"); 
    CCDTriageScreen* pCDTS = CCDTriageScreen::GetInstance();
    if (pCDTS->ProcessMetadataList((cd_multiple_hit_event_t*)Mesg.pData))
    {
        m_pPS->SetMessageText(LS(SID_PLEASE_CHOOSE_CD_INFO), CSystemMessageString::STATUS);
        m_pPS->Add(pCDTS);
        m_pPS->Presentation()->MoveFocusTree(pCDTS);
    }
    return 1;
}

SIGNED 
CPlaybackEventHandler::HandleMsgCDTrayOpened(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:CDTrayOpened\n"); 
    m_pPS->NotifyCDTrayOpened();
    return 1;
}

SIGNED 
CPlaybackEventHandler::HandleMsgCDTrayClosed(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:CDTrayClosed\n"); 
    m_pPS->NotifyCDTrayClosed();
    return 1;
}

SIGNED 
CPlaybackEventHandler::HandleMsgMediaInserted(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:mediaIns\n"); 
    m_pPS->NotifyCDInserted();
    CCDTriageScreen::GetInstance()->NotifyCDInserted();
    return 1;
}

SIGNED 
CPlaybackEventHandler::HandleMsgMediaRemoved(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:mediaRem\n"); 

    m_pPS->NotifyCDRemoved();
    CCDTriageScreen::GetInstance()->NotifyCDRemoved();
    if (m_pDJPS->GetSource() == CDJPlayerState::CD)
    {
        // Don't hide the menu screen if we're currently scanning the HD.
        if (!m_pDJPS->IsScanningHD())
            m_pPS->HideMenus();
        // Ask the user to insert a CD.
        m_pPS->DisplayInsertCDScreen();
    }
    return 1;
}

SIGNED 
CPlaybackEventHandler::HandleMsgSetUIViewMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:setVM\n"); 
    m_pPS->SetViewMode((CDJPlayerState::EUIViewMode)Mesg.iData, (bool)Mesg.lData);
    return 1;
}

SIGNED
CPlaybackEventHandler::HandleMsgMusicSourceChanged(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:SrcChngd\n"); 
    m_pPS->NotifyMusicSourceChanged((CDJPlayerState::ESourceMode)Mesg.iData);
    return 1;
}

SIGNED
CPlaybackEventHandler::HandleMsgPlaylistCleared(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:PllstClrd\n"); 
    m_pPS->DisplaySelectTracksScreen();
    return 1;
}

SIGNED
CPlaybackEventHandler::HandleTimerScrollTitle(const PegMessage& Mesg)
{
	if (!m_pPS->ScrollTextFields())
	{
        m_pPS->SetTimerForScrollEnd();
	}
	else
		m_pPS->Draw();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleTimerScrollEnd(const PegMessage& Mesg)
{
	m_pPS->SynchTextScrolling();
	m_pPS->Draw();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleTimerDoTrackChange(const PegMessage& Mesg)
{
	m_pPS->DoTrackChange();
    return 0;
}

SIGNED
CPlaybackEventHandler::HandleTrackProgress(const PegMessage& Mesg)
{
    m_pPS->UpdateTrackProgress(Mesg);
    return 0;
}

SIGNED 
CPlaybackEventHandler::HandleTimerIR(const PegMessage &Mesg)
{
    DEBUGP( DBG_PLAYBACK_EVENTS , DBGLEV_TRACE, "pe:ir\n"); 
    m_pPS->PerformFunkyIRResponse();
    return 0;
}

SIGNED
CPlaybackEventHandler::DispatchEvent(const PegMessage &Mesg)
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
                    return HandleKeyRight(Mesg);
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
                case IR_KEY_MUTE:
                    return HandleKeyMute(Mesg);
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
            return HandleTrackProgress(Mesg);
		case IOPM_PLAYLIST_LOADED:
            // get the playlist name
			break;
		case IOPM_TRACK_END:
			break;
        case IOPM_PLAYMODE:
            return HandleMsgPlayMode(Mesg);
        case IOPM_SYSTEM_MESSAGE:
            return HandleMsgSystemMessage(Mesg);
        case IOPM_REFRESH_METADATA:
            return HandleMsgRefreshMetadata(Mesg);
        case IOPM_MULTIPLE_METADATA:
            return HandleMsgMultipleMetadata(Mesg);
        case IOPM_CD_TRAY_OPENED:
            return HandleMsgCDTrayOpened(Mesg);
        case IOPM_CD_TRAY_CLOSED:
            return HandleMsgCDTrayClosed(Mesg);
        case IOPM_MEDIA_INSERTED:
            return HandleMsgMediaInserted(Mesg);
        case IOPM_MEDIA_REMOVED:
            return HandleMsgMediaRemoved(Mesg);
        case IOPM_SET_UI_VIEW_MODE:
            return HandleMsgSetUIViewMode(Mesg);
        case IOPM_MUSIC_SOURCE_CHANGED:
            return HandleMsgMusicSourceChanged(Mesg);
        case IOPM_PLAYLIST_CLEARED:
            return HandleMsgPlaylistCleared(Mesg);
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
    		}
		default:    
            return m_pPS->CScreen::Message(Mesg);
	}
	return 0;
}

void CPlaybackEventHandler::InitPlayerScreenPtr(CPlayerScreen* s)
{
    m_pPS = s;
}

