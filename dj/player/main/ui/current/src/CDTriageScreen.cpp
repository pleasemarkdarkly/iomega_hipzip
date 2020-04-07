//
// CDTriageScreen.cpp: contains the implementation of the CCDTriageScreen class
// danb@fullplaymedia.com 12/04/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//

#include <main/ui/CDTriageScreen.h>

#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>
#include <content/common/ContentManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <main/cddb/CDDBHelper.h>
#include <main/main/AppSettings.h>
#include <extras/cdmetadata/CDMetadataEvents.h>
#include <extras/cdmetadata/DiskInfo.h>
#include <core/events/SystemEvents.h>   // event types
#include <util/eventq/EventQueueAPI.h>
#include <stdlib.h>
#include <stdio.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_CDTRIAGE_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_CDTRIAGE_SCREEN );

#define METADATA_STRING_SIZE 128

extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

//extern CPlayScreen* g_pMainWindow;
CCDTriageScreen* CCDTriageScreen::s_pCDTriageScreen = 0;

// This is a singleton class.
CCDTriageScreen*
CCDTriageScreen::GetInstance()
{
	if (!s_pCDTriageScreen) {
		s_pCDTriageScreen = new CCDTriageScreen(NULL);
	}
	return s_pCDTriageScreen;
}

CCDTriageScreen::CCDTriageScreen(CScreen* pParent)
  : CScrollingListScreen(pParent, SID_EMPTY_STRING)
{
    DEBUGP( DBG_CDTRIAGE_SCREEN, DBGLEV_TRACE, "CDTriageScreen:Ctor\n");
	BuildScreen();
    m_pHitList = 0;
    m_bSelectionMade = false;
    m_bCDInserted = false;
    m_pPlayManager = CPlayManager::GetInstance();
    m_pContentManager = m_pPlayManager->GetContentManager();
    m_pDJPlayerState = CDJPlayerState::GetInstance();
    m_pPlayerScreen = CPlayerScreen::GetPlayerScreen();
}

CCDTriageScreen::~CCDTriageScreen()
{
    DEBUGP( DBG_CDTRIAGE_SCREEN, DBGLEV_TRACE, "CDTriageScreen:Dtor\n");
}

SIGNED
CCDTriageScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
	case PM_KEY:

		switch (Mesg.iData)
		{
            case IR_KEY_EXIT:
			case KEY_EXIT:
                {
                    // send a message back to the event queue to do the default labeling of a cd
                    m_pPlayerScreen->SetMessageText(LS(SID_EMPTY_STRING), CSystemMessageString::STATUS);
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_SELECTION, (void*)m_pHitList->uiDiskID);
                    HideScreen();
				    return 0;
                }

            case IR_KEY_SAVE:
                if (m_cItems > 0)
                    ProcessMenuOption(GetHighlightedIndex());
				return 0;

            case IR_KEY_UP:
            case KEY_UP:
            case IR_KEY_DOWN:
            case KEY_DOWN:
            case IR_KEY_SELECT:
            case KEY_SELECT:
                // let the base class handle these
                break; 

            default:
                return 0;
        }
        break;

    case PM_TIMER:
        
        switch (Mesg.iData)
        {
		case CDTS_TIMER_SCROLL_TITLE:
			if (!ScrollTextFields())
			{
				KillTimer(CDTS_TIMER_SCROLL_TITLE);
                if (m_pDJPlayerState->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
				    SetTimer(CDTS_TIMER_SCROLL_END, SCROLL_SLOW_END_INTERVAL, 0);
                else
                    SetTimer(CDTS_TIMER_SCROLL_END, SCROLL_FAST_END_INTERVAL, 0);
			}
			else
            {
                BeginDraw();
                CScrollingListScreen::Draw();
                Line(mReal.wLeft, mReal.wTop, mReal.wRight, mReal.wTop, BLACK, 14);
                EndDraw();
            }
			break;

		case CDTS_TIMER_SCROLL_END:
			SynchTextScrolling();
            Draw();
			break;

        default:
            break;
        }
        break;

    default:
        break;
    }
    return CScrollingListScreen::Message(Mesg);
}

void
CCDTriageScreen::Draw()
{
    // find the current item and load it up
    if (m_pHitList && (GetHighlightedIndex() < m_pHitList->svDisks.Size()))
    {
        // do the title
        SetTitleText(m_pHitList->svDisks[GetHighlightedIndex()]->GetTitle());

        // do the artist -- genre 
        SetArtistText(m_pHitList->svDisks[GetHighlightedIndex()]->GetArtist());
    }

    // set the screen title
    m_pScreenTitle->DataSet(LS(SID_PLEASE_CHOOSE_CD_INFO));
    // center the title
    PegRect ChildRect;
	int iTextLength    = Screen()->TextWidth(m_pScreenTitle->DataGet(), m_pScreenTitle->GetFont());
	int iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pScreenTitle->mReal.wTop, mReal.wRight, m_pScreenTitle->mReal.wBottom);
    m_pScreenTitle->Resize(ChildRect);

    // set the instructions
    m_pInstructionsTextString->DataSet(LS(SID_UPDOWN_TO_CHOOSE_SELECT_TO_SAVE));

    BeginDraw();
    CScrollingListScreen::Draw();
    EndDraw();
}


void
CCDTriageScreen::HideScreen()
{
    KillTimer(CDTS_TIMER_SCROLL_TITLE);
    KillTimer(CDTS_TIMER_SCROLL_END);
    DeleteMetadataList();
    m_pPlayerScreen->Remove(this);
    Presentation()->MoveFocusTree(m_pPlayerScreen);
}

// Called when a CD is inserted.
// Resets the state of the selection screen.
void
CCDTriageScreen::NotifyCDInserted()
{
    DeleteMetadataList();
    m_bSelectionMade = false;
    m_bCDInserted = true;
}

// Called when a CD is ejected.
// Makes sure any multiple metadata messages that arrive asynchronously are not displayed.
void
CCDTriageScreen::NotifyCDRemoved()
{
    DeleteMetadataList();
    m_bCDInserted = false;
}


bool
CCDTriageScreen::ProcessMetadataList(cd_multiple_hit_event_t* pList)
{
    DBASSERT(DBG_CDTRIAGE_SCREEN, pList, "cdts:NULL multiple hit event\n");

    // If the user has already made a selection for this CD, then just delete this event.
    if (m_bSelectionMade || !m_bCDInserted)
    {
        DeleteMultipleHitEvent(pList);
        return false;
    }

    if (!m_pHitList)
    {
        if (pList->svDisks.Size() == 1)
        {
            // If there's just one hit, go ahead and make it the selection.
            DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Only one hit for CD metadata, so using that as default\n");

            ProcessSelection(pList->svDisks[0]);
            m_bSelectionMade = true;
            DeleteMultipleHitEvent(pList);

            m_pPlayerScreen->RefreshCurrentTrackMetadata();

            return false;
        }
        else
        {
            // This is the first batch of results for this disk, so just add them all to the screen.
            DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Adding List of Multiple Metadata for CD: %d hits\n", pList->svDisks.Size());

            m_pHitList = pList;
            SetItemCount(m_pHitList->svDisks.Size());
        }
    }
    else
    {
        // There are already some results, so merge this batch with the existing matches.
        DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Merging List of Multiple Metadata for CD\n");
        DiskInfoVector svNewDisks;
        int iOriginalSize = m_pHitList->svDisks.Size();
        int i = 0;
        while (i < pList->svDisks.Size())
        {
            int j;
            for (j = 0; j < iOriginalSize; ++j)
            {
                if (!tstrcmp(pList->svDisks[i]->GetTitle(), m_pHitList->svDisks[j]->GetTitle()) &&
                    !tstrcmp(pList->svDisks[i]->GetArtist(), m_pHitList->svDisks[j]->GetArtist()))
                {
                    DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Redundant match: Title: %w Artist: %w\n", pList->svDisks[i]->GetTitle(), pList->svDisks[i]->GetArtist());
                    break;
                }
            }
            if (j == iOriginalSize)
            {
                DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Adding disc: Title: %w Artist: %w\n", pList->svDisks[i]->GetTitle(), pList->svDisks[i]->GetArtist());
                m_pHitList->svDisks.PushBack(pList->svDisks.Remove(i));
            }
            else
                ++i;
        }
        DeleteMultipleHitEvent(pList);
        SetItemCount(m_pHitList->svDisks.Size());
    }

    Draw();
    return true;
}


void
CCDTriageScreen::DeleteMetadataList()
{
    if (m_pHitList)
    {
        DeleteMultipleHitEvent(m_pHitList);
        m_pHitList = 0;
    }

    SetItemCount(0);
    m_iTopIndex = -1;
}

void
CCDTriageScreen::DeleteMultipleHitEvent(cd_multiple_hit_event_t* pList)
{
    if (pList)
    {
        DEBUGP(DBG_CDTRIAGE_SCREEN, DBGLEV_INFO, "cdts:Delete List of Multiple Metadata for CD\n");
        ClearDiskList(pList->svDisks);
        delete pList;
    }
}

void
CCDTriageScreen::SetTitleText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pTitleTextString->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pTitleTextString->DataGet(), m_pTitleTextString->GetFont());
    iCaptionLength = m_TitleTextRect.wRight - m_TitleTextRect.wLeft;
    if(iTextLength < iCaptionLength - 9)
    {
        ChildRect.Set(((iCaptionLength - 9 - iTextLength) / 2) + m_TitleTextRect.wLeft, m_pTitleTextString->mReal.wTop, m_pTitleTextString->mReal.wRight, m_pTitleTextString->mReal.wBottom);
        m_pTitleTextString->Resize(ChildRect);
    }
    else if(iTextLength < iCaptionLength)
    {
        ChildRect.Set(((iCaptionLength - iTextLength) / 2) + m_TitleTextRect.wLeft, m_pTitleTextString->mReal.wTop, m_pTitleTextString->mReal.wRight, m_pTitleTextString->mReal.wBottom);
        m_pTitleTextString->Resize(ChildRect);
    }
    else
        m_pTitleTextString->Resize(m_TitleTextRect);
    
    Screen()->Invalidate(m_pTitleTextString->mReal);
    SynchTextScrolling();
    //Draw();
}


void
CCDTriageScreen::SetArtistText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pArtistTextString->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
    iCaptionLength = m_ArtistTextRect.wRight - m_ArtistTextRect.wLeft;
    if(iTextLength < iCaptionLength - 9)
    {
        ChildRect.Set(((iCaptionLength - 9 - iTextLength) / 2) + m_ArtistTextRect.wLeft, m_pArtistTextString->mReal.wTop, m_pArtistTextString->mReal.wRight, m_pArtistTextString->mReal.wBottom);
        m_pArtistTextString->Resize(ChildRect);
    }
    else if(iTextLength < iCaptionLength)
    {
        ChildRect.Set(((iCaptionLength - iTextLength) / 2) + m_ArtistTextRect.wLeft, m_pArtistTextString->mReal.wTop, m_pArtistTextString->mReal.wRight, m_pArtistTextString->mReal.wBottom);
        m_pArtistTextString->Resize(ChildRect);
    }
    else
        m_pArtistTextString->Resize(m_ArtistTextRect);
    
    Screen()->Invalidate(m_pArtistTextString->mReal);
    SynchTextScrolling();
    //Draw();
}

void
CCDTriageScreen::ProcessMenuOption(int iMenuIndex)
{
    KillTimer(CDTS_TIMER_SCROLL_TITLE);
    KillTimer(CDTS_TIMER_SCROLL_END);

    ProcessSelection(m_pHitList->svDisks[iMenuIndex]);

    m_pPlayerScreen->RefreshCurrentTrackMetadata();
    m_pPlayerScreen->SynchStatusMessage();

    m_bSelectionMade = true;

    // delete the metadata list by calling ProcessMetadataList() with nothing
    DeleteMetadataList();

    HideScreen();
}


void
CCDTriageScreen::ForceRedraw()
{
    Invalidate(mReal);
    Draw();
}


void
CCDTriageScreen::BuildScreen()
{
    PegRect ChildRect;
    mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
    InitClient();
    RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
    
    // title text area
    m_TitleTextRect.Set(mReal.wLeft + 9, mReal.wTop + 16, mReal.wRight, mReal.wTop + 32);
    m_pTitleTextString = new PegString(m_TitleTextRect, LS(SID_ALBUM), 0, FF_NONE | TT_COPY );
    m_pTitleTextString->SetFont(&FONT_PLAYSCREENBIG);
    m_pTitleTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pTitleTextString);
    
    // artist text area
    m_ArtistTextRect.Set(mReal.wLeft + 9, mReal.wTop + 34, mReal.wRight, mReal.wTop + 48);
    m_pArtistTextString = new PegString(m_ArtistTextRect, LS(SID_ARTIST), 0, FF_NONE | TT_COPY );
    m_pArtistTextString->SetFont(&FONT_PLAYSCREENBOLD);
    m_pArtistTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pArtistTextString);
    
    // instructions text area
    ChildRect.Set(mReal.wLeft, mReal.wBottom - 13, mReal.wRight, mReal.wBottom);
    m_pInstructionsTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
    m_pInstructionsTextString->SetFont(&FONT_PLAYSCREEN);
    m_pInstructionsTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);
    Add(m_pInstructionsTextString);
    
    // move the scrolling list screen arrows around.
    ChildRect.Set(mReal.wLeft, mReal.wTop + 17, mReal.wLeft + 7, mReal.wTop + 23);
    m_pScreenUpArrow->Resize(ChildRect);
    ChildRect.Set(mReal.wLeft, mReal.wBottom - 22, mReal.wLeft + 7, mReal.wBottom);
    m_pScreenDownArrow->Resize(ChildRect);

    // move the horizontal bar north a pixel
	ChildRect.Set(mReal.wLeft, mReal.wTop + 15, mReal.wRight, mReal.wTop + 15);
    m_pScreenHorizontalDottedBarIcon->Resize(ChildRect);

	// the horizontal bar on the bottom of the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom - 13);
	m_pBottomScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pBottomScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pBottomScreenHorizontalDottedBarIcon);

    Add(m_pScreenTitle);
}


void
CCDTriageScreen::SynchTextScrolling()
{
	// make sure all the strings are pointing to the right text
	bool bScroll = false;
	int iCDTextWidth = Screen()->TextWidth(m_pTitleTextString->DataGet(), m_pTitleTextString->GetFont());
	int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
	int iScreenWidth = mReal.wRight - (mReal.wLeft + 9);
	PegRect NewRect;

	if (iCDTextWidth > iScreenWidth)
	{
		NewRect = m_pTitleTextString->mReal;
		NewRect.wLeft = m_TitleTextRect.wLeft;
		m_pTitleTextString->Resize(NewRect);
		bScroll = true;
	}

	if (iArtistTextWidth > iScreenWidth)
	{
		NewRect = m_pArtistTextString->mReal;
		NewRect.wLeft = m_ArtistTextRect.wLeft;
		m_pArtistTextString->Resize(NewRect);
		bScroll = true;
	}

    CDJPlayerState::EUITextScrollSpeed eScroll = m_pDJPlayerState->GetUITextScrollSpeed();

    if (bScroll && eScroll != CDJPlayerState::OFF)
        if (m_pDJPlayerState->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
		    SetTimer(CDTS_TIMER_SCROLL_TITLE, SCROLL_SLOW_START_INTERVAL, SCROLL_SLOW_CONTINUE_INTERVAL);
        else
            SetTimer(CDTS_TIMER_SCROLL_TITLE, SCROLL_FAST_START_INTERVAL, SCROLL_FAST_CONTINUE_INTERVAL);
	else
		KillTimer(CDTS_TIMER_SCROLL_TITLE);
}


bool
CCDTriageScreen::ScrollTextFields()
{
	bool bScrolled = false;
	int iCDTextWidth = Screen()->TextWidth(m_pTitleTextString->DataGet(), m_pTitleTextString->GetFont());
	int iArtistTextWidth = Screen()->TextWidth(m_pArtistTextString->DataGet(), m_pArtistTextString->GetFont());
	PegRect NewRect;

	if (iCDTextWidth > m_pTitleTextString->mReal.wRight - m_pTitleTextString->mReal.wLeft)
	{
		NewRect = m_pTitleTextString->mReal;
		NewRect.wLeft -= 10;
		m_pTitleTextString->Resize(NewRect);
        m_pTitleTextString->mClip = m_TitleTextRect;
		bScrolled = true;
	}

	if (iArtistTextWidth > m_pArtistTextString->mReal.wRight - m_pArtistTextString->mReal.wLeft)
	{
		NewRect = m_pArtistTextString->mReal;
		NewRect.wLeft -= 5;
		m_pArtistTextString->Resize(NewRect);
        m_pArtistTextString->mClip = m_ArtistTextRect;
		bScrolled = true;
	}

	return bScrolled;
}

// Called when the user chooses a disk.
// Adds that disk's metadata to the system.
void
CCDTriageScreen::ProcessSelection(IDiskInfo* pDiskInfo)
{
    // Adjust thread priority so playback doesn't stutter.
    int nPrio = GetMainThreadPriority();
    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

    if (CCDDataSource* pCDDS = m_pDJPlayerState->GetCDDataSource())
    {
        if (((CCDDBDiskInfo*)pDiskInfo)->IsPartial())
            ((CCDDBPartialDiskInfo*)pDiskInfo)->RetrieveDiskInfo();
#ifdef CDDB_CACHE_RESULTS
        // Add the disk info to the local cache.
        CCDDBQueryManager::GetInstance()->AddDiskToCache((CCDDBDiskInfo*)pDiskInfo);
        // Don't save the cache if we're playing, since that will cause audio dropouts.
        if (m_pPlayManager->GetPlayState() != CMediaPlayer::PLAYING)
            CCDDBQueryManager::GetInstance()->SaveCache();
#endif  // CDDB_CACHE_RESULTS
        // Set the metadata in the content manager for all of the tracks.
        SetCDRecordMetadata(m_pContentManager, pCDDS, pDiskInfo);
        
        // Send an event letting the UI know we selected something
        CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_SELECTED, (void*)m_pHitList->uiDiskID);
    }

    SetMainThreadPriority(nPrio);
}
