//
// QuickBrowseMenuScreen.cpp: Browse the contents of the current playlist and do operations on it.
// danb@fullplaymedia.com 11/15/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/PlaylistConstraint.h>
#include <main/ui/InfoMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/PlayerScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Fonts.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>
#include <main/ui/Messages.h>

#include <main/main/DJHelper.h>
#include <main/main/Recording.h>

#include <stdio.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_QUICKBROWSE, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_QUICKBROWSE );

CQuickBrowseMenuScreen* CQuickBrowseMenuScreen::s_pQuickBrowseMenuScreen = 0;

// This is a singleton class.
CQuickBrowseMenuScreen*
CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()
{
	if (!s_pQuickBrowseMenuScreen) {
		s_pQuickBrowseMenuScreen = new CQuickBrowseMenuScreen();
	}
	return s_pQuickBrowseMenuScreen;
}

CQuickBrowseMenuScreen::CQuickBrowseMenuScreen()
	: CDynamicMenuScreen(NULL, SID_ALBUMS),
    m_iPlaylistCount(0),
    m_iPlaylistTopIndex(0),
	m_iPlaylistLineIndex(0)
{
    BuildScreen();
    SetItemCount(0);
}

CQuickBrowseMenuScreen::~CQuickBrowseMenuScreen()
{
}

SIGNED
CQuickBrowseMenuScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        case IR_KEY_PLAY:
        case KEY_PLAY:
            SetSong(GetHighlightedIndex(), false);
            return 0;

        case IR_KEY_RECORD:
        case KEY_RECORD:
            if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
                break;
            else {
                RecordSong(GetHighlightedIndex());
                return 0;
            }

        case IR_KEY_NEXT:
            // dc- for now, dont go into info screen when 'right' is hit
            // ShowTrackInfo(GetHighlightedIndex());
            return 0;

        case IR_KEY_MENU:
        case KEY_MENU:
        case IR_KEY_PREV:
            CPlayerScreen::GetPlayerScreen()->HideMenus();
            return 0;

        case IR_KEY_CLEAR: 
            ClearPlaylist();
            return 0;
            
        case IR_KEY_DELETE:
            DeleteCurrentItem();
            return 0;
            
        case IR_KEY_EDIT:
        case IR_KEY_INFO:
            ShowTrackInfo(GetHighlightedIndex());
            return 0;
            
        case IR_KEY_SELECT:
            if (OnQueryingLine(GetHighlightedIndex()))
                return 0;
            break;

        // drop these keys
        case IR_KEY_1_misc:
        case IR_KEY_2_abc:
        case IR_KEY_3_def:
        case IR_KEY_4_ghi:
        case IR_KEY_5_jkl:
        case IR_KEY_6_mno:
        case IR_KEY_7_pqrs:
        case IR_KEY_8_tuv:
        case IR_KEY_9_wxyz:
        case IR_KEY_0_space:
        case IR_KEY_ABC_UP:
        case IR_KEY_ABC_DOWN:
        case IR_KEY_PLAY_MODE:
            return 0;
            
        // let these keys be handled by the base class or parent screen
        case KEY_EXIT:
        case IR_KEY_EXIT:
        case KEY_SELECT:
        case KEY_DOWN:
        case IR_KEY_DOWN:
        case KEY_UP:
        case IR_KEY_UP:
        case IR_KEY_CHANNEL_UP:
        case IR_KEY_CHANNEL_DOWN:
            break;
            
        default:
            return CPlayerScreen::GetPlayerScreen()->Message(Mesg);
        }
        break;
        
    case PM_KEY_RELEASE:
        return CPlayerScreen::GetPlayerScreen()->Message(Mesg);
        
	case PM_TIMER:
		
		switch (Mesg.iData)
		{
			case QBMS_TIMER_SCROLL_TITLE:
			    if (!ScrollPlaylistTitle())
			    {
				    KillTimer(QBMS_TIMER_SCROLL_TITLE);
                    if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
		                SetTimer(QBMS_TIMER_SCROLL_END, SCROLL_SLOW_END_INTERVAL, 0);
                    else
                        SetTimer(QBMS_TIMER_SCROLL_END, SCROLL_FAST_END_INTERVAL, 0);
			    }
                return 0;

			case QBMS_TIMER_SCROLL_END:
			    ResetPlaylistTitleScroll();
                return 0;

            default:
                break;
        }
        break;

    case IOPM_PLAYLIST_CLEARED:
        SetCurrentPlaylistAsDefault();
        break;

    case IOPM_CONTENT_UPDATE_END:
    case IOPM_METADATA_UPDATE_END:
        RefreshPlaylist();
        break;
    default:
        break;
    }

    return CDynamicMenuScreen::Message(Mesg);
}

const TCHAR* 
CQuickBrowseMenuScreen::MenuItemCaption(int iMenuIndex)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:MenuItemCaption(%d)\n", iMenuIndex);
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        if (iMenuIndex >= pCurrentPlaylist->GetSize())
        {
            CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
            
            char cRange[128];
            sprintf(cRange, " %d - %d", m_iPlaylistCount + 1, m_iPlaylistCount + pLMS->GetQueryRemainingCount());
            TCHAR tcRange[128];
            CharToTcharN(tcRange, cRange, 128);
            
            static TCHAR tcCaption[128];
            tcCaption[0] = 0;
            tstrcat(tcCaption, LS(SID_QUERYING_TRACKS));
            tstrcat(tcCaption, tcRange);
            
            return tcCaption;
        }
        else
        {
            void* pdata;
            IPlaylistEntry* pCurrentEntry = pCurrentPlaylist->GetEntry(iMenuIndex);
            if (pCurrentEntry && SUCCEEDED(pCurrentEntry->GetContentRecord()->GetAttribute(MDA_TITLE,&pdata)))
            {
                return (TCHAR*)pdata;
            }
        }
    }
    return LS(SID_EMPTY_STRING);
}

// Force a redraw of the menu.
void
CQuickBrowseMenuScreen::ForceRedraw()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb::ForceRedraw\n");
	Invalidate(mReal);
    RefreshPlaylist();
}

// Notification from the scrolling list that the list has scrolled up.
void
CQuickBrowseMenuScreen::NotifyScrollUp()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:NotifyScrollUp\n");
    RefreshPlaylist();
}

// Notification from the scrolling list that the list has scrolled down.
void
CQuickBrowseMenuScreen::NotifyScrollDown()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:NotifyScrollDown\n");
    RefreshPlaylist();
}

void
CQuickBrowseMenuScreen::DeleteCurrentItem()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:DeleteCurrentItem\n");


    // Don't delete items from audio CD playlists.
    if ((CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD) &&
            (CDJPlayerState::GetInstance()->GetCDState() != CDJPlayerState::DATA))
        return;
    // Don't delete items from line-in playlists.
    else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
        return;

    int iMenuIndex;

    // check to see if there is a current playlist
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (!pCurrentPlaylist)
        return;
    
    iMenuIndex = CScrollingListScreen::GetHighlightedIndex();
    
    // reality check
    if(iMenuIndex < 0)
        return;

    if (OnQueryingLine(iMenuIndex))
        return;
    
    IPlaylistEntry* pSelectedEntry = pCurrentPlaylist->GetEntry(iMenuIndex);
    IPlaylistEntry* pPlayingEntry  = pCurrentPlaylist->GetCurrentEntry();
    
    if(pSelectedEntry) 
    {
        int iNewIndex = pSelectedEntry->GetIndex();
        bool bRefresh = false;
        if( iNewIndex == pCurrentPlaylist->GetSize() - 1 ) {
            iNewIndex--;
            bRefresh = true;
        }
        if(pSelectedEntry == pPlayingEntry)
            CPlayerScreen::GetPlayerScreen()->RemoveCurrentPlaylistEntry();
        else
            pCurrentPlaylist->DeleteEntry(pSelectedEntry);

        RefreshPlaylist( iNewIndex, bRefresh );
        SetCurrentPlaylistAsUnsaved();
    }

    // synch the player screen so that the current track's playlist number is correct
    CPlayerScreen::GetPlayerScreen()->RefreshCurrentTrackMetadata();

    if(pCurrentPlaylist->GetSize() <= 0)
    {
        SetItemCount(0);
        
        // exit to the player screen 
        CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
        CPlayerScreen::GetPlayerScreen()->HideMenus();
    }
}


void
CQuickBrowseMenuScreen::ClearPlaylist()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:ClearPlaylist\n");

    // Don't clear audio CD playlists.
    if ((CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD) &&
            (CDJPlayerState::GetInstance()->GetCDState() != CDJPlayerState::DATA))
        return;
    // Don't clear line-in playlists.
    else if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::LINE_IN)
        return;

    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();

	pPS->SetMessageText(LS(SID_CLEARED_CURRENT_PLAYLIST), CSystemMessageString::INFO);

	// check to see if there is a current playlist
	IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
	if (!pCurrentPlaylist)
		return;

    SetCurrentPlaylistAsUnsaved();
    ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->CancelPendingQueries();
	pCurrentPlaylist->Clear();
    // talk to the player screen
    pPS->StopRipping();
    pPS->StopPlayback();
    // exit to the player screen 
    pPS->DisplaySelectTracksScreen();
	pPS->HideMenus();
}


// called to make this screen do the necessary queries to get ready to be shown
void
CQuickBrowseMenuScreen::RefreshPlaylist(int iCurrentTrackOffset, bool bForceMenuSynch)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:RefreshPlaylist\n");
    // check to see if there is a current playlist
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
    char szNumberOf[32];

    // must check for this now before playlist size gets updated
    bool bOnQueryingLine = OnQueryingLine(GetHighlightedIndex());
    
    // get the size of the playlist
    if (pCurrentPlaylist)
        m_iPlaylistCount = pCurrentPlaylist->GetSize();
    else
        m_iPlaylistCount = 0;

    if (m_iPlaylistCount > 0)
    {
        int iItemCount = m_iPlaylistCount;
        if (pLMS->GetQueryRemainingCount())
            ++iItemCount;
        SetItemCount(iItemCount);

        if (bOnQueryingLine)
        {
            // stay on the last line
            m_iTopIndex            = m_iPlaylistCount - 1;

            sprintf(szNumberOf, "/ %d [%d]", m_iPlaylistCount, m_iPlaylistCount + pLMS->GetQueryRemainingCount());
        }
        else
        {
            // get the playlist info
            IPlaylistEntry* pHighlightedEntry;
            
            if (bForceMenuSynch)
                pHighlightedEntry = pCurrentPlaylist->GetCurrentEntry();
            else
                pHighlightedEntry = pCurrentPlaylist->GetEntry(GetHighlightedIndex());
            
            if (pHighlightedEntry)
            {
                int iCurrentEntryIndex = pHighlightedEntry->GetIndex();
                if (bForceMenuSynch)
                {
                    m_iTopIndex = iCurrentEntryIndex - 1;  // ?
                    
                    // find the offset.  what do we want selected?
                    if (iCurrentTrackOffset != 0)
                    {
                        iCurrentEntryIndex += iCurrentTrackOffset;
                        if (iCurrentEntryIndex < 0)
                            m_iTopIndex = -1;
                        else if (iCurrentEntryIndex >= m_iPlaylistCount)
                            m_iTopIndex = m_iPlaylistCount - 2;
                        else
                            m_iTopIndex = iCurrentEntryIndex - 1;
                        
                        iCurrentEntryIndex = GetHighlightedIndex();
                    }
                }
                
                // at the very least, make sure we're not pointing past the end of the list
                if (m_iTopIndex > m_iPlaylistCount - 2)
                    m_iTopIndex = m_iPlaylistCount - 2;
                                
                if (pLMS->GetQueryRemainingCount())
                    sprintf(szNumberOf, "%d / %d [%d]", iCurrentEntryIndex + 1 /* zero index */, m_iPlaylistCount, m_iPlaylistCount + pLMS->GetQueryRemainingCount());
                else
                    sprintf(szNumberOf, "%d / %d", iCurrentEntryIndex + 1 /* zero index */, m_iPlaylistCount);
            }
            else
            {
                if (pLMS->GetQueryRemainingCount())
                    sprintf(szNumberOf, "/ %d [%d]", m_iPlaylistCount, m_iPlaylistCount + pLMS->GetQueryRemainingCount());
                else
                    sprintf(szNumberOf, "/ %d", m_iPlaylistCount);
            }
        }
    }
    else
    {
        SetCurrentPlaylistTitle(LS(SID_CURRENT_PLAYLIST));
        SetItemCount(0);
        m_iTopIndex = -1;
        strcpy(szNumberOf, "0 / 0");
    }

    // now that we know how the info on the playlist, display it
    SetNumberOfText(szNumberOf);

    // redraw if this screen is showing
    if(Presentation()->GetCurrentThing() == this)
        Draw();
}


// Called after an entry in the current playlist is deleted and let's this screen
// know that it needs to synch up and redraw.
// iPlaylistIndex is the index of the playlist entry deleted before it was deleted.
void
CQuickBrowseMenuScreen::SynchAfterPlaylistEntryDeletion(int iPlaylistIndex)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:SynchAfterPlaylistEntryDeletion\n");

    if (iPlaylistIndex < GetHighlightedIndex() && m_iTopIndex > -1)
        m_iTopIndex -= 1;

    // make sure we're not beyond the playlist size...
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
        m_iPlaylistCount = pCurrentPlaylist->GetSize();
    else
        m_iPlaylistCount = 0;

    if (m_iPlaylistCount <= 0)
    {
        if (Presentation()->GetCurrentThing() == this)
            CPlayerScreen::GetPlayerScreen()->HideMenus();
        return;
    }
    else if (m_iTopIndex > m_iPlaylistCount - 2)
        m_iTopIndex = m_iPlaylistCount - 2;

    RefreshPlaylist();
}


// Called when the user selects a track to play
ERESULT
CQuickBrowseMenuScreen::SetSong(int iMenuIndex, bool bForceSetSong)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:SetSong(%d)\n", iMenuIndex);
    ERESULT res = PM_ERROR;

    if (OnQueryingLine(iMenuIndex))
        return res;
    
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pTargetEntry = pCurrentPlaylist->GetEntry(iMenuIndex);
        if (!bForceSetSong && 
            pTargetEntry == pCurrentPlaylist->GetCurrentEntry() &&
            CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
            return PM_PLAYING;

        // stop ripping/recording
        CRecordingManager::GetInstance()->StopRipping();
        CRecordingManager::GetInstance()->StopRecording();
        
        int iCurrentEntryIndex = pTargetEntry->GetIndex();

        // redraw the current playlist selection
        m_iTopIndex = iCurrentEntryIndex - 1;
        char szNumberOf[32];
        CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
        if (pLMS->GetQueryRemainingCount())
            sprintf(szNumberOf, "%d / %d [%d]", iCurrentEntryIndex + 1 /* zero index */, m_iPlaylistCount, m_iPlaylistCount + pLMS->GetQueryRemainingCount());
        else
            sprintf(szNumberOf, "%d / %d", iCurrentEntryIndex + 1 /* zero index */, m_iPlaylistCount);
        SetNumberOfText(szNumberOf);
        // redraw if this screen is showing
        if(Presentation()->GetCurrentThing() == this)
            Draw();

        pCurrentPlaylist->SetCurrentEntry(pTargetEntry);
        if (SUCCEEDED(res = DJSetCurrentOrNext(true)))
            CPlayManager::GetInstance()->Play();

        // clear the track in anticipation of getting new metadata
        CPlayerScreen::GetPlayerScreen()->ClearTrack();

        // no good track found in the remainder of the playlist.
        // deconfigure the play manager
        if (!pTargetEntry)
            CPlayManager::GetInstance()->Deconfigure();
        else
            CPlayerScreen::GetPlayerScreen()->SetTrackText(LS(SID_LOADING));


    }
    return res;
}

// Called when the user presses record on a track
ERESULT
CQuickBrowseMenuScreen::RecordSong(int iMenuIndex)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:RecordSong(%d)\n", iMenuIndex);
    ERESULT res = PM_ERROR;

    if (OnQueryingLine(iMenuIndex))
        return res;
    
    CPlayManager* pPM = CPlayManager::GetInstance();
    CRecordingManager* pRM = CRecordingManager::GetInstance();
    IPlaylist* pCurrentPlaylist = pPM->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pTargetEntry = pCurrentPlaylist->GetEntry(iMenuIndex);

        if (pCurrentPlaylist->SetCurrentEntry(pTargetEntry))
        {
            bool bRecord = false;
            if (pRM->IsRipping() ||
                (pPM->GetPlayState() == CMediaPlayer::STOPPED) ||
                (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED))
            {
                res = pRM->StartRipSingle(pTargetEntry);
            }
            else if ((pPM->GetPlayState() == CMediaPlayer::PLAYING) ||
                (pPM->GetPlayState() == CMediaPlayer::PAUSED))
            {
                bRecord = true;
                res = pRM->StartRecordSingle(pTargetEntry);
                if (SUCCEEDED(res))
                    pPM->Play();
            }

            if (FAILED(res))
            {
                // Stop ripping/recording without printing the "Partial track recording cancelled" message
                // since that obscures the message that tells the user why the track couldn't be recorded.
                pRM->StopRipping(false);
                pRM->StopRecording(false);
                if (SUCCEEDED(DJSetCurrentOrNext(true)) && bRecord)
                    pPM->Play();
            }

            CPlayerScreen::GetPlayerScreen()->HideMenus();
        }
    }

    return res;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CQuickBrowseMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:ProcessMenuOption(%d)\n", iMenuIndex);

    SetSong(iMenuIndex);
    CPlayerScreen::GetPlayerScreen()->HideMenus();
}


void
CQuickBrowseMenuScreen::SetNumberOfText(const char* szText)
{
    TCHAR tszText[32];
	m_pNumberOfTextString->DataSet(CharToTcharN(tszText, szText, 31));
    // right justify the string
    int iTextLength = Screen()->TextWidth(m_pNumberOfTextString->DataGet(), m_pNumberOfTextString->GetFont());
    PegRect Rect;
    Rect.Set(m_pNumberOfTextString->mReal.wRight - iTextLength - 1, m_pNumberOfTextString->mReal.wTop, m_pNumberOfTextString->mReal.wRight, m_pNumberOfTextString->mReal.wBottom);
    m_pNumberOfTextString->Resize(Rect);

    Rect.Set(mReal.wLeft, mReal.wTop, m_pNumberOfTextString->mReal.wLeft - 4, mReal.wTop + 15);
    m_pCurrentPlaylistTextString->Resize(Rect);
}


void
CQuickBrowseMenuScreen::SetCurrentPlaylistTitle(const char* szText)
{
    TCHAR* pszNewText = (TCHAR*)malloc((strlen(szText) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewText, szText);
    SetCurrentPlaylistTitle(pszNewText);
    free(pszNewText);
}


void
CQuickBrowseMenuScreen::SetCurrentPlaylistTitle(const TCHAR* tszText)
{
	m_pCurrentPlaylistTextString->DataSet(tszText);
	m_pCurrentPlaylistTextString->Invalidate(m_pCurrentPlaylistTextString->mReal);
	ResetPlaylistTitleScroll(); // calls m_pCurrentPlaylistTextString->Draw()
}


void
CQuickBrowseMenuScreen::SetCurrentPlaylistTitle(const char* szText, const TCHAR* tszText)
{
    TCHAR* pszNewText = (TCHAR*)malloc((strlen(szText) + 1) * sizeof(TCHAR));
    CharToTchar(pszNewText, szText);
    SetCurrentPlaylistTitle(pszNewText, tszText);
    free(pszNewText);
}


void
CQuickBrowseMenuScreen::SetCurrentPlaylistTitle(const TCHAR* tszText1, const TCHAR* tszText2)
{
    int iLen = tstrlen(tszText1) + tstrlen(LS(SID_COLON_SPACE)) + tstrlen(tszText2) + 1;
    TCHAR* pszNewText = (TCHAR*)malloc((iLen) * sizeof(TCHAR));
    tstrcpy(pszNewText, tszText1);
    tstrcat(pszNewText, LS(SID_COLON_SPACE));
    tstrcat(pszNewText, tszText2);
    SetCurrentPlaylistTitle(pszNewText);
    free(pszNewText);
}


void
CQuickBrowseMenuScreen::ShowTrackInfo(int iMenuIndex)
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:ShowTrackInfo\n");

    if (OnQueryingLine(iMenuIndex))
        return;
    
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
	if (!pCurrentPlaylist)
    {
        // todo: tell the user that there's no current playlist
		return;
    }

    // we have a current playlist, now we try to get the current entry
    IPlaylistEntry* pCurrentEntry = NULL;
    pCurrentEntry = pCurrentPlaylist->GetEntry(iMenuIndex);
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
CQuickBrowseMenuScreen::BuildScreen()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:BuildScreen\n");
	PegRect ChildRect;
   	TCHAR szTitle[256 * 2 + 4] = {0};

    // we don't use the default screen title
    Remove(m_pScreenTitle);

	// the current tracks number in the playlist region of the screen
	CharToTchar(szTitle, "0 / 0");
	ChildRect.Set(mReal.wRight - 83, mReal.wTop, mReal.wRight, mReal.wTop + 15);
	m_pNumberOfTextString = new PegString(ChildRect, szTitle, 0, FF_NONE | TT_COPY | TJ_RIGHT);
	m_pNumberOfTextString->SetFont(&FONT_PLAYSCREEN);
	m_pNumberOfTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pNumberOfTextString);

	// current playlist name text area
    // dependent on the placement of the NumberOfTextString
	ChildRect.Set(mReal.wLeft, mReal.wTop, m_pNumberOfTextString->mReal.wLeft - 4, mReal.wTop + 15);
	m_pCurrentPlaylistTextString = new PegString(ChildRect, LS(SID_CURRENT_PLAYLIST), 0, FF_NONE | TT_COPY );
	m_pCurrentPlaylistTextString->SetFont(&FONT_PLAYSCREEN);
	m_pCurrentPlaylistTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pCurrentPlaylistTextString);

}


bool
CQuickBrowseMenuScreen::ScrollPlaylistTitle()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:ScrollPlaylistTitle\n");
    bool bScrolled = false;
    PegRect NewRect;
    int iTextWidth = Screen()->TextWidth(m_pCurrentPlaylistTextString->DataGet(),m_pCurrentPlaylistTextString->GetFont());
    
    if (iTextWidth > m_pCurrentPlaylistTextString->mReal.wRight - m_pCurrentPlaylistTextString->mReal.wLeft - 1 /* -1 for a 1 pixel border */)
    {
        NewRect = m_pCurrentPlaylistTextString->mReal;
        NewRect.wLeft -= 5;
        m_pCurrentPlaylistTextString->Resize(NewRect);
        bScrolled = true;
        
        // do most of the draw function, without calling DrawMenu()
        BeginDraw();
        m_pCurrentPlaylistTextString->Draw();
        EndDraw();
    }
    return bScrolled;
}


void
CQuickBrowseMenuScreen::ResetPlaylistTitleScroll()
{
    DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "qb:ResetPlaylistTitleScroll\n");

    PegRect NewRect;
    NewRect = m_pCurrentPlaylistTextString->mReal;
    NewRect.wLeft = mReal.wLeft;
    m_pCurrentPlaylistTextString->Resize(NewRect);
    CDJPlayerState::EUITextScrollSpeed eScroll = CDJPlayerState::GetInstance()->GetUITextScrollSpeed();

	int iTextWidth = Screen()->TextWidth(m_pCurrentPlaylistTextString->DataGet(),m_pCurrentPlaylistTextString->GetFont());
    if (iTextWidth > m_pCurrentPlaylistTextString->mReal.wRight - m_pCurrentPlaylistTextString->mReal.wLeft & eScroll != CDJPlayerState::OFF)
        if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
            SetTimer(QBMS_TIMER_SCROLL_TITLE, SCROLL_SLOW_MENU_ITEM_START_INTERVAL, SCROLL_SLOW_CONTINUE_INTERVAL);
        else
            SetTimer(QBMS_TIMER_SCROLL_TITLE, SCROLL_FAST_MENU_ITEM_START_INTERVAL, SCROLL_FAST_CONTINUE_INTERVAL);
	else
		KillTimer(QBMS_TIMER_SCROLL_TITLE);

	KillTimer(QBMS_TIMER_SCROLL_END);
    
    // do most of the draw function, without calling DrawMenu()
    BeginDraw();
    m_pCurrentPlaylistTextString->Draw();
    EndDraw();
}

bool
CQuickBrowseMenuScreen::OnQueryingLine(int iIndex)
{
    CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
    return (pLMS->GetQueryRemainingCount() && (iIndex == m_iPlaylistCount));
}

