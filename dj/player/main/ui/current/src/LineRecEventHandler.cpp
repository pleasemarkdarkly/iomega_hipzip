#include <main/ui/LineRecEventHandler.h>
#include <main/ui/Timers.h>

#include <main/main/LineRecorder.h>
#include <main/main/Recording.h>
#include <main/main/RecordingEvents.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Strings.hpp>

#include <core/playmanager/PlayManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/main/DJPlayerState.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_LINEREC_EVENTS, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_LINEREC_EVENTS );  // debugging prefix : (8) lre

CLineRecEventHandler::CLineRecEventHandler()
    : m_iFwdCount(0), m_iSecondsElapsed(0)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_TRACE, "lre:ctor\n"); 
}

CLineRecEventHandler::~CLineRecEventHandler()
{
    m_pPS = NULL;
}

void CLineRecEventHandler::CancelRecording()
{
    DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "lre:cancel\n"); 
    // display stop icon
    m_pPS->SetControlSymbol(CPlayerScreen::STOP);
    m_pPS->SetTime(0);
    CLineRecorder* lr = CLineRecorder::GetInstance();
    // stop recording and delete the new file
    lr->CancelRecording();
    lr->ClearReRecordingFlag();
    // the normal way to get to playback events is *through* line-in events,
    // so it is best to trigger both events here.
    m_pPS->SetEventHandlerMode(ePSLineInEvents);
    m_pPS->SetEventHandlerMode(ePSPlaybackEvents);
}

void CLineRecEventHandler::StopRecording()
{
    DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "lre:stop\n"); 
    // display stop icon
    m_pPS->SetControlSymbol(CPlayerScreen::STOP);
    CLineRecorder* lr = CLineRecorder::GetInstance();
    // stop recording and add the new file to player core.
    if (lr->StopRecording() > 0)
    {
        CPlayManager* pPlayManager = CPlayManager::GetInstance();
        if (lr->ReRecording())
        {
            m_pPS->SetTime(0);
            lr->ClearReRecordingFlag();
            IPlaylist* pPlaylist = pPlayManager->GetPlaylist();
            pPlayManager->SetSong(pPlaylist->GetCurrentEntry());
        }
        else
        {
            DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "lre:add song to content manager +\n"); 
            lr->AddRecordingToPlayerCore();
            // Move the Commit call to the player screen, after the pass-through mode has been turned off.
            // This fixes the problem of audio continuing to play back after the stop button has been pressed.
//            ((CDJContentManager*)pPlayManager->GetContentManager())->Commit();
            DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "lre:add song to content manager -\n"); 
        }
    }
    m_pPS->SetEventHandlerMode(ePSLineInEvents);
}

void CLineRecEventHandler::PauseRecording()
{
    DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "lre:pause\n");
    if( CLineRecorder::GetInstance()->PauseRecording() > 0 ) {
        m_pPS->SetControlSymbol(CPlayerScreen::RECORD);
    } else {
        m_pPS->SetControlSymbol(CPlayerScreen::PAUSE);
    }
}

// unpause the recording
void CLineRecEventHandler::ResumeRecording()
{
    if( CLineRecorder::GetInstance()->ResumeRecording() > 0 ) {
        m_pPS->SetControlSymbol(CPlayerScreen::RECORD);
    }
}

SIGNED CLineRecEventHandler::HandleKeyPlay(const PegMessage &Mesg)
{
    ResumeRecording();
    return 0;
}

SIGNED CLineRecEventHandler::HandleKeyStop(const PegMessage &Mesg)
{
    StopRecording();
    return 0;
}

SIGNED CLineRecEventHandler::HandleKeyPause(const PegMessage &Mesg)
{
    PauseRecording();
    return 0;
}

void 
CLineRecEventHandler::DisplayGainNotification()
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

SIGNED
CLineRecEventHandler::HandleKeyUp(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_TRACE, "lre:kup\n"); 

    // increase gain
    CLineRecorder::GetInstance()->IncrementGain();
    // give a gain report in the system message text.
    DisplayGainNotification();
    return 0;
}

SIGNED
CLineRecEventHandler::HandleKeyDown(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_TRACE, "lre:kdown\n"); 

    // decrease gain
    CLineRecorder::GetInstance()->DecrementGain();
    // give a gain report in the system message text.
    DisplayGainNotification();
    return 0;
}

SIGNED 
CLineRecEventHandler::HandleKeyZoom(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "lre:kzoom\n"); 
    m_pPS->ToggleZoom();
    return 0;
}

SIGNED
CLineRecEventHandler::HandleTrackProgress(const PegMessage& Mesg)
{
    m_iSecondsElapsed = (int)Mesg.lData;
    m_pPS->UpdateTrackProgress (Mesg);
    return 0;
}

void CLineRecEventHandler::JumpToNextFile()
{
    // stop recording, jump to a new track, and start recording again.
    // display stop icon
    m_pPS->SetControlSymbol(CPlayerScreen::STOP);
    // stop recording and add the new file to player core.
    CLineRecorder* lr = CLineRecorder::GetInstance();
    if (lr->StopRecording() > 0)
        if (lr->ReRecording())
        {
            m_pPS->SetTime(0);
            lr->ClearReRecordingFlag();
        }
        else
        {
            lr->AddRecordingToPlayerCore();
        }

    lr->StartRecording();
}

SIGNED
CLineRecEventHandler::HandleKeyFwdRelease(const PegMessage& Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "lre:kfwd new rec\n"); 

    if (m_iFwdCount >= 3) {
        m_iFwdCount = 0;
        return 0;
    }
    m_iFwdCount = 0;
    int iSecs = m_iSecondsElapsed;
    if( iSecs > (LINEREC_NEWREC_DELAY_SECS) )
    {
        DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "delay %ds > %ds, new rec\n", iSecs, LINEREC_NEWREC_DELAY_SECS ); 

        // Stop recording if the maximum number of tracks on the hard drive has been reached.
        if (((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->GetMediaRecordCount(CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID()) >= MAX_HD_TRACKS - 1)
        {
            DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_INFO, "rm:Track limit reached\n" );
            m_pPS->SetMessageText(LS(SID_MAXIMUM_NUMBER_OF_TRACKS_RECORDED), CSystemMessageString::REALTIME_INFO);
            m_pPS->SetMessageText(LS(SID_DELETE_TRACKS_TO_ENABLE_RECORDING), CSystemMessageString::INFO);
            StopRecording();
        }
        else
            JumpToNextFile();
    }
    else
    {
        DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "delay %ds <= %ds, ignore NT\n", iSecs, LINEREC_NEWREC_DELAY_SECS );
    }
    return 0;
}

SIGNED 
CLineRecEventHandler::HandleMsgSystemMessage(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "lre:sysmsg\n"); 
    if(Mesg.pData) // the TCHAR
    {
        m_pPS->SetMessageText((TCHAR*)Mesg.pData, (CSystemMessageString::SysMsgType)Mesg.iData);
        free((TCHAR*)Mesg.pData);  // allocated in CPEGUserInterface::SetMessage
    }
    return 0;
}

SIGNED 
CLineRecEventHandler::HandleMsgClipDetected(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "lre:Clip\n");
    m_pPS->SetMessageText(LS(SID_LINE_INPUT_CLIP_DETECTED), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED 
CLineRecEventHandler::HandleMsgSetUIViewMode(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_INFO, "lre:setVM\n"); 
    m_pPS->SetViewMode((CDJPlayerState::EUIViewMode)Mesg.iData, (bool)Mesg.lData);
    return 0;
}

SIGNED
CLineRecEventHandler::HandleMsgMusicSourceChanged(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_TRACE, "lre:SrcChngd\n"); 
    m_pPS->NotifyMusicSourceChanged((CDJPlayerState::ESourceMode)Mesg.iData);
    return 1;
}

SIGNED
CLineRecEventHandler::HandleKeyNotAvailable(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS, DBGLEV_TRACE, "lre:KeyNotAvail\n");
    m_pPS->SetMessageText(LS(SID_NOT_AVAILABLE_WHILE_USING_LINE_INPUT), CSystemMessageString::REALTIME_INFO);
    
    return 0;
}

SIGNED
CLineRecEventHandler::HandleKeyMenu(const PegMessage &Mesg)
{
    DEBUGP( DBG_LINEREC_EVENTS , DBGLEV_TRACE, "lre:KeyMenu\n");
    m_pPS->SetMessageText(LS(SID_MENU_ACCESS_DISABLED_WHILE_RECORDING), CSystemMessageString::REALTIME_INFO);
    return 0;
}

SIGNED 
CLineRecEventHandler::DispatchEvent(const PegMessage &Mesg)
{
    switch (Mesg.wType)
	{
		case PM_KEY:
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
                case IR_KEY_FWD:
                case KEY_FWD:
                    m_iFwdCount = (m_iFwdCount < 3) ? ++m_iFwdCount : 3;
                    return 0;
				case IR_KEY_REW:
                case KEY_REW:
                    break;
                case IR_KEY_UP:
                case KEY_UP:
                    return HandleKeyUp(Mesg);                    
                case IR_KEY_DOWN:
                case KEY_DOWN:
                    return HandleKeyDown(Mesg);

                case IR_KEY_SAVE:
                case IR_KEY_CLEAR:
                case IR_KEY_DELETE:
                case IR_KEY_ADD:
                    
                case IR_KEY_GENRE:
                case IR_KEY_ARTIST:
                case IR_KEY_ALBUM:
                case IR_KEY_PLAYLIST:
                case IR_KEY_RADIO:
                    return HandleKeyNotAvailable(Mesg);
                case IR_KEY_MENU:
                case KEY_MENU:
                    return HandleKeyMenu(Mesg);
                case KEY_REFRESH_CONTENT:
                    break;
                case IR_KEY_RECORD:
                case KEY_RECORD:
                    return HandleKeyStop(Mesg);
                    break;
                case IR_KEY_ZOOM:
                    return HandleKeyZoom(Mesg);
				default:
					break;
			}
			break;
		case PM_KEY_RELEASE:
			switch (Mesg.iData)
            {
			    case IR_KEY_FWD	:
                case KEY_FWD:
                        return HandleKeyFwdRelease(Mesg);
				case IR_KEY_REW:
                case KEY_REW:
                    break;
                case IR_KEY_MENU:
                case KEY_MENU:
                    break;
            }
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
            return HandleTrackProgress(Mesg);
		case IOPM_PLAYLIST_LOADED:
			break;
        case IOPM_TRACK_END:
			break;
        case IOPM_SYSTEM_MESSAGE:
            return HandleMsgSystemMessage(Mesg);
        case IOPM_ADC_CLIP_DETECTED:
            return HandleMsgClipDetected(Mesg);
        case IOPM_SET_UI_VIEW_MODE:
            return HandleMsgSetUIViewMode(Mesg);
        case IOPM_MUSIC_SOURCE_CHANGED:
            return HandleMsgMusicSourceChanged(Mesg);
		case PM_TIMER:
			switch (Mesg.iData)
			{
                /*  // (epg,2/13/2002): we shouldn't need scrolling if the titles are tightly controlled.
                case PS_TIMER_SCROLL_TEXT:
                    m_pPS->HandleTimerScrollTitle(Mesg);
				    break;
			    case PS_TIMER_SCROLL_END:
                    m_pPS->HandleTimerScrollEnd(Mesg);
				    break;
                */
	    	}
            break;
		default:    
			return m_pPS->CScreen::Message(Mesg);
	}
	return 0;
}

void CLineRecEventHandler::InitPlayerScreenPtr(CPlayerScreen* s)
{
    m_pPS = s;
}

void CLineRecEventHandler::HandleSpaceLow()
{
    StopRecording();
    m_pPS->NotifyUserRecordingCutoff();
}

