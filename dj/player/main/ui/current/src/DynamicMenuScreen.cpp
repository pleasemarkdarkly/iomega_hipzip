//
// DynamicMenuScreen.cpp: The CDynamicMenuScreen class is an abstract base class from which more specific
//      menu screens are derived.  It provides functions for adding, displaying
//      and highlighting menu items as well as automatically handling navigation
//      through keypress messages.	
// danb@fullplaymedia.com 09/11/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/DynamicMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <stdlib.h> /* free */

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_DYNAMIC_MENU_SCREEN );

#define MENU_ITEM_TOP_OFFSET            18
#define MENU_ITEM_LEFT_OFFSET           12
#define MENU_ITEM_ICON_WIDTH            14
#define MENU_ITEM_HEIGHT                14
#define MENU_ITEM_OFFSET                (MENU_ITEM_HEIGHT+1)
#define ZOOM_MENU_ITEM_OFFSET           (MENU_ITEM_TOP_OFFSET+5)
#define ZOOM_MENU_ICON_LEFT_OFFSET       4
#define ZOOM_MENU_ICON_TOP_OFFSET       32
#define ZOOM_MENU_ICON_WIDTH            18
#define ZOOM_MENU_ICON_HEIGHT           20

// this is so that multiple instances of this class can be used and
// their timers won't interfere with eachother
WORD CDynamicMenuScreen::s_uiInstaceCounter = 0;

CDynamicMenuScreen::CDynamicMenuScreen(pGetScreenFunc pGetChildScreenFunc, WORD wScreenTitleSID)
	: CScrollingListScreen(NULL, wScreenTitleSID),
	m_pGetChildScreenFunc(pGetChildScreenFunc),
	m_pszMenuTitle(NULL)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::Ctor\n");
	SetItemCount(0);

	BuildScreen();

    m_uiInstance = s_uiInstaceCounter++;
    s_uiInstaceCounter++;

	// init this screens child and set it's parent as this screen
	if(m_pGetChildScreenFunc)
		(m_pGetChildScreenFunc)()->SetParent(this);
}

CDynamicMenuScreen::~CDynamicMenuScreen()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::Dtor\n");
	// destroy the child screen.  poor child.
	if(m_pGetChildScreenFunc)
		Destroy((m_pGetChildScreenFunc)());

    // add whatever objects aren't on the screen so the destructor can mop them up for us
    if( m_eViewMode == CDJPlayerState::ZOOM ) {
        // zoom mode, so add the non-zoom items
        Add(m_pHighlightCaptionString);
        for(int i = 0; i < DISPLAY_LINES; i++) {
            Add(m_aryIcons[i]);
            Add(m_aryCaptions[i]);
        }
    } else {
        // normal mode, so add the zoom items
        Add(m_pZoomTextString);
    }
}

// Hides any visible menu screens.
void
CDynamicMenuScreen::HideScreen()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::HideScreen\n");
	KillTimer(_TIMER_SCROLL_TITLE());
	KillTimer(_TIMER_SCROLL_END());
	// hide all the submenus as well.
	if(m_pGetChildScreenFunc)
		(m_pGetChildScreenFunc)()->HideScreen();
	
	CScrollingListScreen::HideScreen();
}

WORD
CDynamicMenuScreen::_TIMER_SCROLL_TITLE()
{
    return DMS_TIMER_SCROLL_TITLE + m_uiInstance;
}

WORD
CDynamicMenuScreen::_TIMER_SCROLL_END()
{
    return DMS_TIMER_SCROLL_END + m_uiInstance;
}

SIGNED
CDynamicMenuScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::Message\n");
	switch (Mesg.wType)
	{
	case PM_KEY:

		switch (Mesg.iData)
		{
            case IR_KEY_MENU:
			case KEY_MENU:
				GotoPreviousMenu();
				return 0;
            default:
                break;
        }
        break;

	case PM_TIMER:
		
		if (Mesg.iData == _TIMER_SCROLL_TITLE())
        {
			if (!ScrollHighlightedCaption())
			{
				KillTimer(_TIMER_SCROLL_TITLE());
                if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
				    SetTimer(_TIMER_SCROLL_END(), SCROLL_SLOW_END_INTERVAL, 0);
                else
                    SetTimer(_TIMER_SCROLL_END(), SCROLL_FAST_END_INTERVAL, 0);
			}
            return 0;
        }
        else if (Mesg.iData == _TIMER_SCROLL_END())
        {
			Draw();
            return 0;
        }

	default:
		return CScrollingListScreen::Message(Mesg);
	}
	return CScrollingListScreen::Message(Mesg);
}

void
CDynamicMenuScreen::Draw()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::Draw\n");
	BeginDraw();
	DrawMenu();
	CScrollingListScreen::Draw();
	EndDraw();
}

bool
CDynamicMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::MenuItemHasSubMenu\n");
    return false;
}

bool
CDynamicMenuScreen::MenuItemSelected(int iMenuIndex)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::MenuItemSelected\n");
    return false;
}

bool
CDynamicMenuScreen::MenuItemSelectable(int iMenuIndex)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::MenuItemSelectable\n");
    return false;
}

const TCHAR*
CDynamicMenuScreen::MenuItemCaption(int iMenuIndex)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::MenuItemCaption\n");
    return LS(SID_EMPTY_STRING);
}

const TCHAR*
CDynamicMenuScreen::MenuTitleCaption()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::MenuTitleCaption\n");
    LS(m_wScreenTitleSID);
}

const TCHAR*
CDynamicMenuScreen::EmptyMenuCaption()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::EmptyMenuCaption\n");
    LS(SID_EMPTY_STRING);
}

// Force a redraw of the menu.
void
CDynamicMenuScreen::ForceRedraw()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::ForceRedraw\n");
	Invalidate(mReal);
	Draw();
	//Presentation()->MoveFocusTree(this);
}

// Sets the view mode for this screen.
void
CDynamicMenuScreen::SetViewMode(CDJPlayerState::EUIViewMode eViewMode)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::SetViewMode\n");
	// pass this great info onto the child screen as well.
	if(m_pGetChildScreenFunc)
		(m_pGetChildScreenFunc)()->SetViewMode(eViewMode);

    CScreen::SetViewMode(eViewMode);
    SynchWithViewMode();
}

// Notification from the scrolling list that the list has scrolled up.
void
CDynamicMenuScreen::NotifyScrollUp()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::NotifyScrollUp\n");
	Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CDynamicMenuScreen::NotifyScrollDown()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::NotifyScrollDown\n");
	Draw();
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CDynamicMenuScreen::GotoSubMenu(int iMenuIndex)
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::GotoSubMenu\n");
	KillTimer(_TIMER_SCROLL_TITLE());
	KillTimer(_TIMER_SCROLL_END());
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CDynamicMenuScreen::GotoPreviousMenu()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::GotoPreviousMenu\n");
	KillTimer(_TIMER_SCROLL_TITLE());
	KillTimer(_TIMER_SCROLL_END());
    if (Parent() && m_pParent)
    {
        if (Parent() != m_pParent)
	        Parent()->Add(m_pParent);
	    Parent()->Remove(this);
	    Presentation()->MoveFocusTree(m_pParent);
    }
	ResetToTop();
}


// Creates all the icons used by the menu screen.
void
CDynamicMenuScreen::BuildScreen()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::BuildScreen\n");
    int i = 0;
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	for (i = 0; i < DISPLAY_LINES; ++i)
	{
        ChildRect.Set(mReal.wLeft + MENU_ITEM_LEFT_OFFSET, MENU_ITEM_TOP_OFFSET + MENU_ITEM_OFFSET * i, mReal.wLeft + MENU_ITEM_LEFT_OFFSET + MENU_ITEM_ICON_WIDTH, MENU_ITEM_TOP_OFFSET + MENU_ITEM_HEIGHT + MENU_ITEM_OFFSET * i);
		m_aryIcons[i] = new PegIcon(ChildRect, &gbEmptyBitmap);
		m_aryIcons[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);

		ChildRect.Set(mReal.wLeft + MENU_ITEM_LEFT_OFFSET, MENU_ITEM_TOP_OFFSET + MENU_ITEM_OFFSET * i, mReal.wRight, MENU_ITEM_TOP_OFFSET + MENU_ITEM_HEIGHT + MENU_ITEM_OFFSET * i);
		m_aryCaptions[i] = new PegString(ChildRect, 0, 0, FF_NONE | TT_COPY);
		m_aryCaptions[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		if (SELECTED_LINE_INDEX == i)
        {
			m_aryCaptions[i]->SetFont(&FONT_MENUSCREENENTRY_SELECTED);
            m_aryCaptions[i]->SetColor(PCI_NORMAL, BLACK);
            m_aryCaptions[i]->SetColor(PCI_NTEXT, WHITE);
            m_HighlightTextRect = ChildRect;

            ChildRect.Set(mReal.wLeft, MENU_ITEM_TOP_OFFSET + MENU_ITEM_OFFSET * i, mReal.wRight, MENU_ITEM_TOP_OFFSET + MENU_ITEM_HEIGHT + MENU_ITEM_OFFSET * i);
		    m_pHighlightCaptionString = new PegString(ChildRect, 0, 0, FF_NONE | TT_COPY);
            m_pHighlightCaptionString->SetColor(PCI_NORMAL, BLACK);
            m_pHighlightCaptionString->SetColor(PCI_NTEXT, WHITE);
			m_pHighlightCaptionString->SetFont(&FONT_MENUSCREENENTRY_SELECTED);
		    m_pHighlightCaptionString->RemoveStatus(PSF_ACCEPTS_FOCUS);
            Add(m_pHighlightCaptionString);
        }
		else
			m_aryCaptions[i]->SetFont(&FONT_MENUSCREENENTRY);

		Add(m_aryIcons[i]);
		Add(m_aryCaptions[i]);
	}

    Add(m_pScreenTitle);

    // zoom text area
	m_ZoomTextRect.Set(mReal.wLeft + MENU_ITEM_LEFT_OFFSET, mReal.wTop + ZOOM_MENU_ITEM_OFFSET, mReal.wRight, mReal.wBottom);
	m_pZoomTextString = new PegString(m_ZoomTextRect, NULL, 0, FF_NONE | TT_COPY );
	m_pZoomTextString->SetFont(&FONT_MENUSCREENZOOM);
	m_pZoomTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	// dont' add the zoom string initally b/c we'll assume a default of non-zoom mode

    // zoom icon
    ChildRect.Set(mReal.wLeft + ZOOM_MENU_ICON_LEFT_OFFSET, ZOOM_MENU_ICON_TOP_OFFSET, mReal.wLeft + ZOOM_MENU_ICON_LEFT_OFFSET + ZOOM_MENU_ICON_WIDTH, ZOOM_MENU_ICON_TOP_OFFSET + ZOOM_MENU_ICON_HEIGHT);
    m_pZoomIcon = new PegIcon(ChildRect, &gbEmptyBitmap);
    m_pZoomIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);
}

void
CDynamicMenuScreen::SynchWithViewMode()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::SynchWithViewMode\n");
    int i;
    switch(m_eViewMode)
    {
    case CDJPlayerState::ZOOM:
        Remove(m_pHighlightCaptionString);
	    for (i = 0; i < DISPLAY_LINES; ++i)
	    {
		    Remove(m_aryIcons[i]);
		    Remove(m_aryCaptions[i]);
	    }
        Add(m_pZoomIcon);
        Add(m_pZoomTextString);
        break;
    case CDJPlayerState::NORMAL:
    default:
        Remove(m_pZoomIcon);
        Remove(m_pZoomTextString);
        Add(m_pHighlightCaptionString);
	    for (i = 0; i < DISPLAY_LINES; ++i)
	    {
		    Add(m_aryIcons[i]);
		    Add(m_aryCaptions[i]);
	    }
        break;
    }

	KillTimer(_TIMER_SCROLL_TITLE());
	KillTimer(_TIMER_SCROLL_END());

    // only redraw if this screen has input focus
    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

// Draws the menu, filling in the correct captions and showing the needed icons.
void
CDynamicMenuScreen::DrawMenu()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::DrawMenu\n");
    int iTextLength = 0, iCaptionLength = 0, i = 0;
    int cToDisplay = m_cItems < m_cDisplayLines ? m_cItems : m_cDisplayLines;
    PegRect CaptionRect;
    CDJPlayerState::EUITextScrollSpeed eScroll = CDJPlayerState::GetInstance()->GetUITextScrollSpeed();
    
    m_pScreenTitle->DataSet(MenuTitleCaption());
    // center the middle top item
    iTextLength = Screen()->TextWidth(m_pScreenTitle->DataGet(), m_pScreenTitle->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        CaptionRect.Set(((iCaptionLength - iTextLength) / 2), m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    else
        CaptionRect.Set(mReal.wLeft, m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    m_pScreenTitle->Resize(CaptionRect);
    
    if(m_eViewMode == CDJPlayerState::ZOOM)
    {
        // make sure it starts off with the right dimentions
        CaptionRect = m_ZoomTextRect;

        // adjust for a check mark icon if it's needed
        if (MenuItemSelectable(GetHighlightedIndex()))
        {
            PegBitmap* pBitmap = NULL;
            if (MenuItemSelected(GetHighlightedIndex()))
                pBitmap = &gbSelectedCheckZoomBitmap;
            else
                pBitmap = &gbUnselectedCheckZoomBitmap;

            m_pZoomIcon->SetIcon(pBitmap);
            CaptionRect.wLeft = mReal.wLeft + ZOOM_MENU_ICON_LEFT_OFFSET + ZOOM_MENU_ICON_WIDTH;
        }
        else
        {
            m_pZoomIcon->SetIcon(&gbEmptyBitmap);
        }

		m_pZoomTextString->Resize(CaptionRect);
        
        if(0 == m_cItems)
            m_pZoomTextString->DataSet(EmptyMenuCaption());
        else
            m_pZoomTextString->DataSet(MenuItemCaption(GetHighlightedIndex()));
        
        // give the user a message if there's nothing to list
        if(0 == m_cItems)
            m_pZoomTextString->DataSet(EmptyMenuCaption());
        
        // see if we need to scroll the highlighted text region
        int iTextWidth = Screen()->TextWidth(m_pZoomTextString->DataGet(),m_pZoomTextString->GetFont());
        if (iTextWidth > m_pZoomTextString->mReal.wRight - m_pZoomTextString->mReal.wLeft && eScroll != CDJPlayerState::OFF)
            if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
                SetTimer(_TIMER_SCROLL_TITLE(), SCROLL_SLOW_MENU_ITEM_START_INTERVAL, SCROLL_SLOW_CONTINUE_INTERVAL);
            else
                SetTimer(_TIMER_SCROLL_TITLE(), SCROLL_FAST_MENU_ITEM_START_INTERVAL, SCROLL_FAST_CONTINUE_INTERVAL);
        else
            KillTimer(_TIMER_SCROLL_TITLE());
    }
    else
    {
        // draw each item in the menu.
        for (i = 0; i < m_cDisplayLines; ++i)
        {
            if(i == SELECTED_LINE_INDEX)
                CaptionRect = m_HighlightTextRect;
            else
                CaptionRect = m_aryCaptions[i]->mReal;
            
            // if we're at the end of the list, draw the menu caption
            if((i > cToDisplay) ||
                ((i == 0) && (m_iTopIndex == -1)) ||
                ((i > SELECTED_LINE_INDEX) && (m_iTopIndex + m_iLineIndex == m_cItems - 1)))
            {
                m_aryIcons[i]->SetIcon(&gbEmptyBitmap);
                m_aryCaptions[i]->DataSet(NULL);
            }
            else
            {
                if (MenuItemSelectable(i + m_iTopIndex))
                {
                    PegBitmap* pBitmap = NULL;
                    if (MenuItemSelected(i + m_iTopIndex))
                        if(i + m_iTopIndex == GetHighlightedIndex())
                            pBitmap = &gbSelectedCheckInvertedBitmap;
                        else
                            pBitmap = &gbSelectedCheckBitmap;
                    else
                        if(i + m_iTopIndex == GetHighlightedIndex())
                            pBitmap = &gbUnselectedCheckInvertedBitmap;
                        else
                            pBitmap = &gbUnselectedCheckBitmap;
                            
                            m_aryIcons[i]->SetIcon(pBitmap);
                            CaptionRect.wLeft = mReal.wLeft + MENU_ITEM_LEFT_OFFSET + pBitmap->wWidth;
                }
                else
                {
                    m_aryIcons[i]->SetIcon(&gbEmptyBitmap);
                    CaptionRect.wLeft = mReal.wLeft + MENU_ITEM_LEFT_OFFSET;
                }
                
                m_aryCaptions[i]->Resize(CaptionRect);
                m_aryCaptions[i]->DataSet(MenuItemCaption(i + m_iTopIndex));
            }
        }
        
        // give the user a message if there's nothing to list
        if(0 == m_cItems)
            m_aryCaptions[m_iLineIndex]->DataSet(EmptyMenuCaption());
        
        // see if we need to scroll the highlighted text region
        int iTextWidth = Screen()->TextWidth(m_aryCaptions[m_iLineIndex]->DataGet(),m_aryCaptions[m_iLineIndex]->GetFont());
        if (iTextWidth > m_aryCaptions[m_iLineIndex]->mReal.wRight - m_aryCaptions[m_iLineIndex]->mReal.wLeft && eScroll != CDJPlayerState::OFF)
            if (CDJPlayerState::GetInstance()->GetUITextScrollSpeed() == CDJPlayerState::SLOW)
                SetTimer(_TIMER_SCROLL_TITLE(), SCROLL_SLOW_MENU_ITEM_START_INTERVAL, SCROLL_SLOW_CONTINUE_INTERVAL);
            else
                SetTimer(_TIMER_SCROLL_TITLE(), SCROLL_FAST_MENU_ITEM_START_INTERVAL, SCROLL_FAST_CONTINUE_INTERVAL);
        else
            KillTimer(_TIMER_SCROLL_TITLE());
    }
}

bool
CDynamicMenuScreen::ScrollHighlightedCaption()
{
    DEBUGP( DBG_DYNAMIC_MENU_SCREEN, DBGLEV_TRACE, "CDynamicMenuScreen::ScrollHighlightedCaption\n");
    bool bScrolled = false;
    PegRect NewRect;
    if(m_eViewMode == CDJPlayerState::ZOOM)
    {
        int iTextWidth = Screen()->TextWidth(m_pZoomTextString->DataGet(),m_pZoomTextString->GetFont());
        
        if (iTextWidth > m_pZoomTextString->mReal.wRight - m_pZoomTextString->mReal.wLeft - 1 /* -1 for a 1 pixel border */)
        {
            NewRect = m_pZoomTextString->mReal;
            NewRect.wLeft -= 10;
            m_pZoomTextString->Resize(NewRect);
            m_pZoomTextString->mClip = m_ZoomTextRect;
            bScrolled = true;
            
            // do most of the draw function, without calling DrawMenu()
            BeginDraw();
            m_pZoomTextString->Draw();
            EndDraw();
        }
    }
    else
    {
        int iTextWidth = Screen()->TextWidth(m_aryCaptions[m_iLineIndex]->DataGet(),m_aryCaptions[m_iLineIndex]->GetFont());
        
        if (iTextWidth > m_aryCaptions[m_iLineIndex]->mReal.wRight - m_aryCaptions[m_iLineIndex]->mReal.wLeft - 1 /* -1 for a 1 pixel border */)
        {
            NewRect = m_aryCaptions[m_iLineIndex]->mReal;
            NewRect.wLeft -= 5;
            m_aryCaptions[m_iLineIndex]->Resize(NewRect);
            m_aryCaptions[m_iLineIndex]->mClip = m_HighlightTextRect;
            bScrolled = true;
            
            // do most of the draw function, without calling DrawMenu()
            BeginDraw();
            m_aryCaptions[m_iLineIndex]->Draw();
            EndDraw();
        }
    }
    
    return bScrolled;
}
