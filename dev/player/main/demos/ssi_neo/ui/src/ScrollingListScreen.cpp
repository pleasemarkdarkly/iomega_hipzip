//........................................................................................
//........................................................................................
//.. File Name: ScrollingListScreen.cpp													..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: contains the implementation of CScrollingListScreen class	..
//.. Usage:The CScrollingListScreen class is an abstract base class from which more		..
//..	   specific menu screens are derived.  It creates the screen outline, title,	..
//..	   up/down scroll arrows, and exit caption.  Widgets within the list are		..
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
#include <main/demos/ssi_neo/ui/ScrollingListScreen.h>

//#include "PlayScreen.h"
#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Fonts.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

//extern CPlayScreen* g_pMainWindow;

// this will make the menus wrap.  for example, if you're at the top of a menu
// and you hit up, the bottom most entry in the menu (exit) will be selected.
#define MENU_WRAP  

CScrollingListScreen::CScrollingListScreen(CScreen* pParent, WORD wScreenTitleSID, int iDisplayLines)
  : CScreen(pParent),
	m_cDisplayLines(iDisplayLines),
	m_wScreenTitleSID(wScreenTitleSID)
{
	BuildScreen();
}

CScrollingListScreen::~CScrollingListScreen()
{
	Destroy(m_pScreenUpArrow);
	Destroy(m_pScreenDownArrow);
}

// Returns the zero-based index of the currently highlighted item.
// If the exit caption is highlighted, then the return value is -1.
int
CScrollingListScreen::GetHighlightedIndex() const
{
	if ((m_iTopIndex + m_iLineIndex) == m_cItems)
		return -1;
	else
		return m_iTopIndex + m_iLineIndex;
}

// Hides any visible menu screens.
void
CScrollingListScreen::HideScreen()
{
	if (Parent())
	{
		Parent()->Remove(this);
		ResetToTop();
	}
}

SIGNED
CScrollingListScreen::Message(const PegMessage &Mesg)
{
	int iCurrentIndex = 0;

	switch (Mesg.wType)
	{
		case PM_KEY:

			switch (Mesg.iData)
			{
				case KEY_PLAY_PAUSE:
				case KEY_NEXT:
				case KEY_MENU:
				{
					ProcessMenuOption(GetHighlightedIndex());
					return 0;
				}

				case KEY_UP:
					// find where we're at...  aka, the current index
					iCurrentIndex = m_iTopIndex + m_iLineIndex;

					// If we're already at the top of the menu, then ignore the keypress.
					if ((m_iLineIndex == 0) && (m_iTopIndex == 0))
					{
#ifdef MENU_WRAP
						int iOldIndex = m_iLineIndex;
						m_iLineIndex = (m_cItems >= (m_cDisplayLines - 1)) ? (m_cDisplayLines - 1) : m_cItems;
						// adjust the top index
						m_iTopIndex = (m_cItems >= m_cDisplayLines) ? ((m_cItems + 1) - m_cDisplayLines) : 0;

						m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
						if (m_iTopIndex != 0)
	    					m_pScreenUpArrow->SetIcon(&gbUpArrowInvertedBitmap);

						// Tell subclasses that the line index has changed.
						NotifyLineIndexChange(iOldIndex, m_iLineIndex);

						ForceRedraw();
#endif	// MENU_WRAP
						return 0;
					}

					// If the line index isn't on the exit caption, then decrement it.
					if (m_iLineIndex) // && (m_iLineIndex < m_cDisplayLines))
					{
						--m_iLineIndex;
						// Tell subclasses that the line index has changed.
						NotifyLineIndexChange(m_iLineIndex + 1, m_iLineIndex);
						return 0;
					}

#if 0
					// If the line index is on the exit caption, then move to the lowermost menu item.
					if (m_iLineIndex == m_cDisplayLines)
					{
						if (m_cItems < m_cDisplayLines)
							m_iLineIndex = m_cItems - 1;
						else
							m_iLineIndex = m_cDisplayLines - 1;

						// Tell subclasses that the line index has changed.
						NotifyLineIndexChange(m_cDisplayLines, m_iLineIndex);
						return 0;
					}
#endif

					// We now know that the line index is at the top of the screen and there are more
					// items above, so scroll the list.
					--m_iTopIndex;

					// The down arrow might be added already, but adding it again can't hurt.
					m_pScreenDownArrow->SetIcon(&gbDownArrowInvertedBitmap);
					Invalidate(m_pScreenDownArrow->mReal);

					// If this keypress led us to the top of the list, then hide the up arrow.
					if (m_iTopIndex == 0)
					{
						m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);
						Invalidate(m_pScreenUpArrow->mReal);
					}

					// Tell the subclasses the the list should be scrolled up.
//					Draw();
					NotifyScrollUp();

					break;

				case KEY_DOWN:
				{
					// find where we're at...  aka, the current index
					iCurrentIndex = m_iTopIndex + m_iLineIndex;

					// If we're already at the exit caption, then ignore the keypress.
					if (iCurrentIndex == m_cItems)
					{
#ifdef MENU_WRAP
						ResetToTop();
						NotifyLineIndexChange(m_cDisplayLines, m_iLineIndex);
						ForceRedraw();
#endif
						return 0;
					}

#if 0
					// If there are no more items in the list, then move the highlight to the exit caption.
					if (iCurrentIndex >= m_cItems - 1)
					{
						int iOldIndex = m_iLineIndex;
						m_iLineIndex = m_cDisplayLines;
						// Tell subclasses that the line index has changed.
						NotifyLineIndexChange(iOldIndex, m_cDisplayLines);
						return 0;
					}
#endif
					// If the line index isn't at the bottom, then increment the index.
					if (m_iLineIndex < m_cDisplayLines - 1)
					{
						++m_iLineIndex;
						// Tell subclasses that the line index has changed.
						NotifyLineIndexChange(m_iLineIndex - 1, m_iLineIndex);
						return 0;
					}

					// We now know that the line index is at the bottom of the screen and there are more
					// items below, so scroll the list.
					++m_iTopIndex;

					// The up arrow might be added already, but adding it again can't hurt.
					m_pScreenUpArrow->SetIcon(&gbUpArrowInvertedBitmap);
					Invalidate(m_pScreenUpArrow->mReal);

					// If this keypress led us to the bottom of the list, then hide the down arrow and
					// show the exit caption.
					if (++iCurrentIndex >= m_cItems)
					{
						m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
						Invalidate(m_pScreenDownArrow->mReal);
					}

					// Tell the subclasses the the list should be scrolled down.
					NotifyScrollDown();

					break;
				}

			}
			return 0;
			break;

		default:
			return CScreen::Message(Mesg);
	}
}

void
CScrollingListScreen::Draw()
{
	// redo the screen title
	m_pScreenTitle->DataSet(LS(m_wScreenTitleSID));

	BeginDraw();

	// Draw the middle line.
	//Line(mReal.wLeft, 0, mReal.wRight, 0, BLACK, 16);

	CScreen::Draw();

	EndDraw();
}

// Sets the number of items in the list.
// Called by the derived classes to inform the scrolling list how far it can scroll.
void
CScrollingListScreen::SetItemCount(int cItems)
{
	m_cItems = cItems;

	// Add the down arrow to the menu if there are more items than we can display at one time
	if (m_cItems <= m_cDisplayLines)
	{
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	}
	else
	{
		m_pScreenDownArrow->SetIcon(&gbDownArrowInvertedBitmap);
	}
}

// Force a redraw of the menu.
void
CScrollingListScreen::ForceRedraw()
{
}

// Resets the list so that the first item is at the top of the list.
// Called when the list is exited.
void
CScrollingListScreen::ResetToTop()
{
	m_iLineIndex = 0;
	m_iTopIndex = 0;
	m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);
	Invalidate(m_pScreenUpArrow->mReal);
	if (m_cItems <= m_cDisplayLines)
	{
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	}
	else
	{
		m_pScreenDownArrow->SetIcon(&gbDownArrowInvertedBitmap);
	}
	Invalidate(m_pScreenDownArrow->mReal);
}

// Redraws the up/down arrows and exit caption based on the current line and display line parameters.
void
CScrollingListScreen::RedrawIcons()
{
	// If we're at the top of the list then hide the up arrow.
	if (m_iTopIndex == 0)
		m_pScreenUpArrow->SetIcon(&gbEmptyBitmap);
	else
		m_pScreenUpArrow->SetIcon(&gbUpArrowInvertedBitmap);
	Invalidate(m_pScreenUpArrow->mReal);

	// If the top index plus the number of display lines is greater than or equal to the
	// item count then show the exit caption.
	if ((m_iTopIndex + m_cDisplayLines) >= m_cItems)
	{
		m_pScreenDownArrow->SetIcon(&gbEmptyBitmap);
	}
	else
	{
		m_pScreenDownArrow->SetIcon(&gbDownArrowInvertedBitmap);
	}
	Invalidate(m_pScreenDownArrow->mReal);
}

// Creates all the icons used by the menu screen.
void
CScrollingListScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, 14);
	m_pScreenTop = new PegIcon(ChildRect, &gbBlackTitleBarBitmap);
	Add(m_pScreenTop);
	m_pScreenTop->RemoveStatus(PSF_ACCEPTS_FOCUS);

	ChildRect.Set(3, 1, UI_X_SIZE-15, 13);
	m_pScreenTitle = new PegString(ChildRect, LS(m_wScreenTitleSID), 0, FF_NONE | TT_COPY);
	m_pScreenTitle->SetFont(&FONT_MENUSCREENTITLE);
	m_pScreenTitle->SetColor(PCI_NORMAL, BLACK);
	m_pScreenTitle->SetColor(PCI_NTEXT, WHITE);
	m_pScreenTitle->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenTitle);

	ChildRect.Set(mReal.wRight - 9, 2, mReal.wRight - 1, 6);
	m_pScreenUpArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	Add(m_pScreenUpArrow);
	m_pScreenUpArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);

	ChildRect.Set(mReal.wRight - 9, 9, mReal.wRight - 1, 13);
	m_pScreenDownArrow = new PegIcon(ChildRect, &gbEmptyBitmap);
	Add(m_pScreenDownArrow);
	m_pScreenDownArrow->RemoveStatus(PSF_ACCEPTS_FOCUS);

	m_iLineIndex = 0;
	m_iTopIndex = 0;
}

