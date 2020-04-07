//........................................................................................
//........................................................................................
//.. File Name: ScrollingListScreen.cpp													..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: contains the implementation of CScrollingListScreen class	..
//.. Usage:The CScrollingListScreen class is an abstract base class from which more		..
//..	   specific menu screens are derived.  It creates the screen outline, title,	..
//..	   and up/down scroll arrows.  Widgets within the list are		                ..
//..	   created by the subclasses of the ScrollingListScreen.						..
//.. Last Modified By: Ed Miller	edwardm@iobjects.com								..
//.. Modification date: 10/19/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/ui/ScrollingListScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <main/ui/PlayerScreen.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_SCROLLING_LIST_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_SCROLLING_LIST_SCREEN );

// this will make the menus wrap.  for example, if you're at the top of a menu
// and you hit up, the bottom most entry in the menu will be selected.
#define MENU_WRAP  

CScrollingListScreen::CScrollingListScreen(CScreen* pParent, WORD wScreenTitleSID, int iDisplayLines)
  : CScreen(pParent),
    m_cDisplayLines(iDisplayLines),
    m_cItems(0),
    m_iTopIndex(-1),
   	m_iLineIndex(SELECTED_LINE_INDEX),
    m_wScreenTitleSID(wScreenTitleSID)
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::Ctor\n");
	BuildScreen();
}

CScrollingListScreen::~CScrollingListScreen()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::Dtor\n");
	Destroy(m_pScreenUpArrow);
	Destroy(m_pScreenDownArrow);
}

// Returns the zero-based index of the currently highlighted item.
int
CScrollingListScreen::GetHighlightedIndex() const
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::GetHighlightedIndex\n");
    return m_iTopIndex + m_iLineIndex;
}

void
CScrollingListScreen::SetHighlightedIndex(int iIndex)
{
    // i can't think of a reason why we'd want to highlight an index if we don't have an item for it
    // make sure we're not setting the highlighted index out of range
    if (iIndex >= 0 && iIndex < m_cItems)
    {
        m_iTopIndex  = iIndex - 1;
        m_iLineIndex = 1;
    }
}

// Hides any visible menu screens.
void
CScrollingListScreen::HideScreen()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::HideScreen\n");
	if (Parent())
		Parent()->Remove(this);
	ResetToTop();
}

SIGNED
CScrollingListScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::Message\n");
    int iCurrentIndex = 0;
    
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        // ignore these keys
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
        case IR_KEY_EDIT:
        case IR_KEY_INFO:
        case IR_KEY_PLAY_MODE:
        case IR_KEY_DELETE:
            return 0;

        case IR_KEY_EXIT:
        case KEY_EXIT:
        case IR_KEY_MENU:
        case KEY_MENU:
            ResetToTop();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
            return 0;
            
        case IR_KEY_NEXT:
            GotoSubMenu(GetHighlightedIndex());
            return 0;
            
        case IR_KEY_PREV:
            GotoPreviousMenu();
            return 0;
            
        case IR_KEY_SELECT:
        case KEY_SELECT:
            if (m_cItems > 0)
                ProcessMenuOption(GetHighlightedIndex());
            return 0;
            
        case IR_KEY_UP:
        case KEY_UP:
            // If there aren't any items in the list, return.  right?
            if (m_cItems <= 0)
                return 0;
            
            // find where we're at...  aka, the current index
            iCurrentIndex = m_iTopIndex + m_iLineIndex;
            
            // If we're already at the top of the menu, then ignore the keypress.
            if (m_iTopIndex == -1)
            {
#ifdef MENU_WRAP
                // adjust the top index
                m_iTopIndex = (m_cItems == 0) ? -1 : (m_cItems - 1) - m_iLineIndex;
                
                m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
                if (m_iTopIndex != -1)
                    m_pScreenUpArrow->SetIcon(&gbSolidUpArrowBitmap);
                
                ForceRedraw();
#endif	// MENU_WRAP
                return 0;
            }
            
            // We now know that the line index is at the top of the screen and there are more
            // items above, so scroll the list.
            --m_iTopIndex;
            
            // Tell the subclasses the the list should be scrolled up.
            NotifyScrollUp();
            return 0;
            
        case IR_KEY_DOWN:
        case KEY_DOWN:
            // If there aren't any items in the list, return.  right?
            if (m_cItems <= 0)
                return 0;
            
            // find where we're at...  aka, the current index
            iCurrentIndex = m_iTopIndex + m_iLineIndex;
            
            // If we're already at the end of the list, then ignore the keypress.
            if (iCurrentIndex == (m_cItems - 1))
            {
#ifdef MENU_WRAP
                ResetToTop();
                ForceRedraw();
#endif
                return 0;
            }
            
            // We now know that the line index is at the bottom of the screen and there are more
            // items below, so scroll the list.
            ++m_iTopIndex;
            
            // Tell the subclasses the the list should be scrolled down.
            NotifyScrollDown();
            return 0;

        case IR_KEY_CHANNEL_UP:
            // If there aren't any items in the list, return.  right?
            if (m_cItems <= 0)
                return 0;
            
            // find where we're at...  aka, the current index
            iCurrentIndex = m_iTopIndex + m_iLineIndex;
            
            // If we're already at the top of the menu, then ignore the keypress.
            if (m_iTopIndex == -1)
            {
#ifdef MENU_WRAP
                // adjust the top index
                m_iTopIndex = (m_cItems == 0) ? -1 : (m_cItems - 1) - m_iLineIndex;
                
                m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
                if (m_iTopIndex != -1)
                    m_pScreenUpArrow->SetIcon(&gbSolidUpArrowBitmap);
                
                ForceRedraw();
#endif	// MENU_WRAP
                return 0;
            }
            
            // We now know that there are more items above, so scroll the list.
            m_iTopIndex -= 10;
            if (m_iTopIndex < -1)
                m_iTopIndex = -1;
            
            DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_INFO, " > Jump to %d of %d\n", (m_iTopIndex+m_iLineIndex+1), m_cItems);

            // Tell the subclasses the the list should be scrolled up.
            NotifyScrollUp();
            return 0;
            
        case IR_KEY_CHANNEL_DOWN:
            // If there aren't any items in the list, return.  right?
            if (m_cItems <= 0)
                return 0;
            
            // find where we're at...  aka, the current index
            iCurrentIndex = m_iTopIndex + m_iLineIndex;
            
            // If we're already at the end of the list, then ignore the keypress.
            if (iCurrentIndex == (m_cItems - 1))
            {
#ifdef MENU_WRAP
                ResetToTop();
                ForceRedraw();
#endif
                return 0;
            }
            
            // We now know that the line index is at the bottom of the screen and there are more
            // items below, so scroll the list.
            m_iTopIndex += 10;
            if (m_iTopIndex > (m_cItems - m_iLineIndex - 1))
                m_iTopIndex = m_cItems - m_iLineIndex - 1;
            
            DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_INFO, " > Jump to %d of %d\n", (m_iTopIndex+m_iLineIndex+1), m_cItems);

            // Tell the subclasses the the list should be scrolled down.
            NotifyScrollDown();
            return 0;

        default:
            break;
        }
        
    case PM_KEY_RELEASE:
        break;
        
    default:
        break;
    }

    return CScreen::Message(Mesg);
}

void
CScrollingListScreen::Draw()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::Draw\n");

    // do the arrows
    if (m_iTopIndex >= 0)
	    m_pScreenUpArrow->SetIcon(&gbSolidUpArrowBitmap);
    else
	    m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);

	if ((m_iTopIndex + SELECTED_LINE_INDEX) < (m_cItems - 1))
        m_pScreenDownArrow->SetIcon(&gbSolidDownArrowBitmap);
    else
        m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);

    Invalidate(m_pScreenDownArrow->mReal);
    Invalidate(m_pScreenUpArrow->mReal);

	BeginDraw();
	CScreen::Draw();
	EndDraw();
}

// Sets the number of items in the list.
// Called by the derived classes to inform the scrolling list how far it can scroll.
void
CScrollingListScreen::SetItemCount(int cItems)
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::SetItemCount\n");
	m_cItems = cItems;

#if 0
	// Add the down arrow to the menu if there are more items than we can display at one time
	if (m_cItems <= m_cDisplayLines)
	{
#else
	// Add the down arrow if we've got more than one item in the list
	if (m_cItems <= 1)
	{
#endif
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	}
	else
	{
		m_pScreenDownArrow->SetIcon(&gbSolidDownArrowBitmap);
	}
	Invalidate(m_pScreenDownArrow->mReal);
}

// Force a redraw of the menu.
void
CScrollingListScreen::ForceRedraw()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::ForceRedraw\n");
}

// Resets the list so that the first item is at the top of the list.
// Called when the list is exited.
void
CScrollingListScreen::ResetToTop()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::ResetToTop\n");
	//m_iLineIndex = 0;
	m_iTopIndex = -1;
	m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);
	Invalidate(m_pScreenUpArrow->mReal);
#if 0
	if (m_cItems <= m_cDisplayLines)
	{
#else
	// Add the down arrow if we've got more than one item in the list
	if (m_cItems <= 1)
	{
#endif
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	}
	else
	{
		m_pScreenDownArrow->SetIcon(&gbSolidDownArrowBitmap);
	}
	Invalidate(m_pScreenDownArrow->mReal);
}
        
// Redraws the up/down arrows based on the current line and display line parameters.
void
CScrollingListScreen::RedrawIcons()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::RedrawIcons\n");
	// If we're at the top of the list then hide the up arrow.
	if (m_iTopIndex == 0)
		m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);
	else
		m_pScreenUpArrow->SetIcon(&gbSolidUpArrowBitmap);
	Invalidate(m_pScreenUpArrow->mReal);

	// If we're at the bottom of the list, hide the down arrow.
	if ((m_iTopIndex + m_cDisplayLines) >= m_cItems)
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	else
		m_pScreenDownArrow->SetIcon(&gbSolidDownArrowBitmap);
	Invalidate(m_pScreenDownArrow->mReal);
}

// Creates all the icons used by the menu screen.
void
CScrollingListScreen::BuildScreen()
{
    DEBUGP( DBG_SCROLLING_LIST_SCREEN, DBGLEV_TRACE, "CScrollingListScreen::BuildScreen\n");
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, mReal.wTop + 13);
	m_pScreenTitle = new PegString(ChildRect, LS(m_wScreenTitleSID), 0, FF_NONE | TT_COPY);
	m_pScreenTitle->SetFont(&FONT_MENUSCREENTITLE);
	m_pScreenTitle->RemoveStatus(PSF_ACCEPTS_FOCUS);
	//Add(m_pScreenTitle);

	ChildRect.Set(mReal.wLeft, mReal.wTop + 19, mReal.wLeft + 7, mReal.wTop + 25);
	m_pScreenUpArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pScreenUpArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenUpArrow);

	ChildRect.Set(mReal.wLeft, mReal.wBottom - 7, mReal.wLeft + 7, mReal.wBottom);
	m_pScreenDownArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	m_pScreenDownArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenDownArrow);

	// the horizontal bar on the screen
	ChildRect.Set(mReal.wLeft, mReal.wTop + 16, mReal.wRight, mReal.wTop + 17);
	m_pScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	this->Add(m_pScreenHorizontalDottedBarIcon);
}

