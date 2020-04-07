//........................................................................................
//........................................................................................
//.. File Name: DynamicMenuScreen.cpp															..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: implementation of class CDynamicMenuScreen		 				..
//.. Usage: The CDynamicMenuScreen class is an abstract base class from which more specific	..
//..	    menu screens are derived.  It provides functions for adding, displaying,	..
//..	    and highlighting menu items as well as automatically handling navigation	..
//..	    through keypress messages.													..
//.. Last Modified By: Todd Malsbary	toddm@fullplaymedia.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/DynamicMenuScreen.h>

//#include "PlayScreen.h"
#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <stdlib.h> /* free */

#define TIMER_SCROLL_TITLE				401
#define TIMER_SCROLL_END				402

const int CDynamicMenuScreen::sc_iScrollStartInterval = 25;
const int CDynamicMenuScreen::sc_iScrollEndInterval = 25;
const int CDynamicMenuScreen::sc_iScrollContinueInterval = 5;

CDynamicMenuScreen::CDynamicMenuScreen(pGetScreenFunc pGetChildScreenFunc, WORD wScreenTitleSID)
	: CScrollingListScreen(NULL, wScreenTitleSID),
	m_pGetChildScreenFunc(pGetChildScreenFunc),
	m_pszMenuTitle(NULL),
	m_bMenuIsTitled(false),
	m_iTitleOffset(0)
{
	SetItemCount(0);

	BuildScreen();

	// init this screens child and set it's parent as this screen
	if(m_pGetChildScreenFunc)
		(m_pGetChildScreenFunc)()->SetParent(this);

}

CDynamicMenuScreen::~CDynamicMenuScreen()
{
	// destroy the child screen.  poor child.
	if(m_pGetChildScreenFunc)
		Destroy((m_pGetChildScreenFunc)());
}

// Hides any visible menu screens.
void
CDynamicMenuScreen::HideScreen()
{
	KillTimer(TIMER_SCROLL_TITLE);
	// hide all the submenus as well.
	if(m_pGetChildScreenFunc)
		(m_pGetChildScreenFunc)()->HideScreen();
	
	CScrollingListScreen::HideScreen();
}

// Set this instance's parent, and all of it's children
void
CDynamicMenuScreen::SetParent(CScreen* pScreen)
{
	CScreen::SetParent(pScreen);
}

SIGNED
CDynamicMenuScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
	case PM_TIMER:
		
		switch (Mesg.iData)
		{
		case TIMER_SCROLL_TITLE:
			if (!ScrollHighlightedCaption())
			{
				KillTimer(TIMER_SCROLL_TITLE);
				SetTimer(TIMER_SCROLL_END, sc_iScrollEndInterval, 0);
			}
			break;
		case TIMER_SCROLL_END:
			DrawMenu();
			Draw();
			break;
		default:
			break;
		}
	default:
		return CScrollingListScreen::Message(Mesg);
	}
	return CScrollingListScreen::Message(Mesg);
}

void
CDynamicMenuScreen::Draw()
{
	BeginDraw();
	DrawMenu();
	CScrollingListScreen::Draw();
	// Highlight the selected line
	if (m_iLineIndex < m_cDisplayLines)
		Screen()->InvertRect(this, m_aryHighlightRects[m_iLineIndex + m_iTitleOffset]);
	EndDraw();
}

// Force a redraw of the menu.
void
CDynamicMenuScreen::ForceRedraw()
{
	DrawMenu();
	Invalidate(mReal);
	Draw();
	Presentation()->MoveFocusTree(this);
}

// Notification from the scrolling list that the line index has changed.
void
CDynamicMenuScreen::NotifyLineIndexChange(int iOldIndex, int iNewIndex)
{
	Invalidate(m_aryHighlightRects[iOldIndex]);
	Invalidate(m_aryHighlightRects[iNewIndex]);
	Draw();
}

// Notification from the scrolling list that the list has scrolled up.
void
CDynamicMenuScreen::NotifyScrollUp()
{
	DrawMenu();
	PegRect rtMenu;
	rtMenu.Set(mReal.wLeft + 30, mReal.wTop, mReal.wRight, mReal.wBottom);
	Invalidate(rtMenu);
	Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CDynamicMenuScreen::NotifyScrollDown()
{
	DrawMenu();
	PegRect rtMenu;
	rtMenu.Set(mReal.wLeft + 30, mReal.wTop, mReal.wRight, mReal.wBottom);
	Invalidate(rtMenu);
	Draw();
}

// Resets the menu so that the first item is at the top of the menu.
// Called when the menu is exited.
void
CDynamicMenuScreen::ResetToTop()
{
	CScrollingListScreen::ResetToTop();
	DrawMenu();
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CDynamicMenuScreen::GotoPreviousMenu()
{
	KillTimer(TIMER_SCROLL_TITLE);
	Parent()->Add(m_pParent);
	Parent()->Remove(this);
	Presentation()->MoveFocusTree(m_pParent);
	ResetToTop();
}

// Creates all the icons used by the menu screen.
void
CDynamicMenuScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	for (int i = 0; i < DISPLAY_LINES; ++i)
	{
		ChildRect.Set(mReal.wLeft + 29, 11 * i, mReal.wLeft + 32, 9 + 11 * i);
		m_aryPrevMenuArrows[i] = new PegIcon(ChildRect, &gbEmptyBitmap);
		m_aryPrevMenuArrows[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_aryPrevMenuArrows[i]);

		ChildRect.Set(mReal.wLeft + 34, 11 * i, mReal.wLeft + 45, 9 + 11 * i);
		m_aryIcons[i] = new PegIcon(ChildRect, &gbEmptyBitmap);
		m_aryIcons[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_aryIcons[i]);

		ChildRect.Set(mReal.wLeft + 34, 11 * i, mReal.wRight - 10, 9 + 11 * i);
		m_aryCaptions[i] = new PegString(ChildRect, 0, 0, FF_NONE | TT_COPY);
		m_aryCaptions[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		m_aryCaptions[i]->SetColor(PCI_NORMAL, BLACK);
		m_aryCaptions[i]->SetColor(PCI_NTEXT, WHITE);
		m_aryCaptions[i]->SetFont(&FONT_MENUSCREENENTRY);
		Add(m_aryCaptions[i]);

		ChildRect.Set(mReal.wRight - 12, 11 * i, mReal.wRight - 7, 9 + 11 * i);
		m_arySubMenuArrows[i] = new PegIcon(ChildRect, &gbEmptyBitmap);
		m_arySubMenuArrows[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_arySubMenuArrows[i]);

		m_aryHighlightRects[i].Set(mReal.wLeft + 28, (11 * i) - 1, mReal.wRight, 9 + 11 * i);
	}
}

// Draws the menu, filling in the correct captions and showing the needed icons.
void
CDynamicMenuScreen::DrawMenu()
{
	int cToDisplay = m_cItems < m_cDisplayLines ? m_cItems : m_cDisplayLines;

	PegRect CaptionRect;

    if (m_bMenuIsTitled)
	{
		CaptionRect = m_aryCaptions[0]->mReal;
		CaptionRect.wLeft = mReal.wLeft + 34;
		CaptionRect.wRight = mReal.wRight - 10;
		m_aryIcons[0]->SetIcon(&gbEmptyBitmap);
		m_aryCaptions[0]->DataSet(m_pszMenuTitle);
		m_aryCaptions[0]->Resize(CaptionRect);
		m_arySubMenuArrows[0]->SetIcon(&gbEmptyBitmap);
		CaptionRect = m_aryCaptions[1]->mReal;
		CaptionRect.wLeft = mReal.wLeft + 32;
		CaptionRect.wRight = mReal.wRight - 10;
		m_aryIcons[1]->SetIcon(&gbEmptyBitmap);
		m_aryCaptions[1]->Resize(CaptionRect);
		m_aryCaptions[1]->DataSet(LS(SID_DASHED_LINE));
		m_arySubMenuArrows[1]->SetIcon(&gbEmptyBitmap);
	}

	// draw each item in the menu.
	for (int i = 0; i < m_cDisplayLines; ++i)
	{
		CaptionRect = m_aryCaptions[i + m_iTitleOffset]->mReal;
		// if we're at the end of the list, draw the menu caption
		if(i >= cToDisplay)
		{
			CaptionRect.wLeft = mReal.wLeft + 34;
			CaptionRect.wRight = mReal.wRight - 10;
			m_aryIcons[i + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
			m_aryCaptions[i + m_iTitleOffset]->Resize(CaptionRect);
			m_aryCaptions[i + m_iTitleOffset]->DataSet(NULL);
			m_arySubMenuArrows[i + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
		}
		else
		{
			m_aryIcons[i + m_iTitleOffset]->SetIcon(MenuItemBitmap(i + m_iTopIndex));



			// if the icon is empty, then adjust the caption start
			if (MenuItemBitmap(i + m_iTopIndex) != NULL)
				CaptionRect.wLeft = mReal.wLeft + 34 + MenuItemBitmap(i + m_iTopIndex)->wWidth;
			else
				CaptionRect.wLeft = mReal.wLeft + 34;

			// if there's no sub menu arrow to display, use the space for the caption
			if (MenuItemHasSubMenu(i + m_iTopIndex) && m_iLineIndex == i)
			{
				m_arySubMenuArrows[i + m_iTitleOffset]->SetIcon(&gbSubMenuArrowBitmap);
				CaptionRect.wRight = mReal.wRight - 14;
			}
			else
			{
				m_arySubMenuArrows[i + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
				CaptionRect.wRight = mReal.wRight - 8;
			}

			// show the previous arrow only when the line is highlighted.
			if (m_iLineIndex == i)
				m_aryPrevMenuArrows[i + m_iTitleOffset]->SetIcon(&gbPrevMenuArrowBitmap);
			else
				m_aryPrevMenuArrows[i + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);

			m_aryCaptions[i + m_iTitleOffset]->Resize(CaptionRect);
			m_aryCaptions[i + m_iTitleOffset]->DataSet(MenuItemCaption(i + m_iTopIndex));
			// see if the caption needs to be scrolled or not.
		}
	}

	// see if we need to scroll the highlighted text region
	int iTextWidth = Screen()->TextWidth(m_aryCaptions[m_iLineIndex + m_iTitleOffset]->DataGet(),m_aryCaptions[m_iLineIndex + m_iTitleOffset]->GetFont());
	if (iTextWidth > m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wRight - m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wLeft)
		SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
	else
		KillTimer(TIMER_SCROLL_TITLE);
}

bool
CDynamicMenuScreen::ScrollHighlightedCaption()
{
	bool bScrolled = false;
	int iTextWidth = Screen()->TextWidth(m_aryCaptions[m_iLineIndex + m_iTitleOffset]->DataGet(),m_aryCaptions[m_iLineIndex + m_iTitleOffset]->GetFont());

	if (iTextWidth > m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wRight - m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wLeft)
	{
		m_aryCaptions[m_iLineIndex + m_iTitleOffset]->DataSet((m_aryCaptions[m_iLineIndex + m_iTitleOffset]->DataGet())+1);
		bScrolled = true;

		// do most of the draw function, without calling DrawMenu()
		BeginDraw();
		m_aryCaptions[m_iLineIndex + m_iTitleOffset]->Draw();
		// Highlight the selected line
		if (m_iLineIndex < m_cDisplayLines)
			Screen()->InvertRect(this, m_aryHighlightRects[m_iLineIndex + m_iTitleOffset]);
		EndDraw();
	}

	return bScrolled;
}


void
CDynamicMenuScreen::SetMenuTitle(const TCHAR* pszMenuTitle)
{
	if(m_pszMenuTitle)
		free((TCHAR*)m_pszMenuTitle);
	if(pszMenuTitle)
	{
		m_pszMenuTitle = (TCHAR*)malloc(sizeof(TCHAR) * (tstrlen(pszMenuTitle) + 1));
		tstrncpy(m_pszMenuTitle, pszMenuTitle, tstrlen(pszMenuTitle) + 1);
		m_iTitleOffset = 2;
		m_cDisplayLines = DISPLAY_LINES - m_iTitleOffset;
		m_bMenuIsTitled = true;
	}
	else
	{
		m_pszMenuTitle = NULL;
		m_iTitleOffset = 0;
		m_cDisplayLines = DISPLAY_LINES;
		m_bMenuIsTitled = false;
	}
}
