//........................................................................................
//........................................................................................
//.. File Name: MenuScreen.cpp															..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: implementation of class CMenuScreen		 				..
//.. Usage: The CMenuScreen class is an abstract base class from which more specific	..
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
#include <main/ui/MenuScreen.h>

//#include "PlayScreen.h"
#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <stdlib.h> /* free */

#define TIMER_SCROLL_TITLE				301
#define TIMER_SCROLL_END				302

const int CMenuScreen::sc_iScrollStartInterval = 25;
const int CMenuScreen::sc_iScrollEndInterval = 25;
const int CMenuScreen::sc_iScrollContinueInterval = 5;

CMenuScreen::CMenuScreen(CScreen* pParent, WORD wScreenTitleSID, MenuItemRec* pMenuItems, int cMenuItems, bool bMenuIsTitled)
  : CScrollingListScreen(pParent, wScreenTitleSID),
	m_pMenuItems(pMenuItems),
	m_pszMenuTitle(NULL),
	m_bMenuIsTitled(bMenuIsTitled),
	m_iTitleOffset(0)
{
    SetItemCount(cMenuItems);

	BuildScreen();

	// init all of the sub menus by calling them,  and set this as the parent
	for (int i = 0; i < m_cItems; i++)
		if (m_pMenuItems[i].pScreen)
			(*(m_pMenuItems[i].pScreen))()->SetParent(this);

}

CMenuScreen::~CMenuScreen()
{
	// destroy all the submenus as well.
	for (int i = 0; i < m_cItems; i++)
		if (m_pMenuItems[i].pScreen)
			Destroy((*(m_pMenuItems[i].pScreen))());

}

// Hides any visible menu screens.
void
CMenuScreen::HideScreen()
{
	KillTimer(TIMER_SCROLL_TITLE);
	// hide all the submenus as well.
	for (int i = 0; i < m_cItems; i++)
		if (m_pMenuItems[i].pScreen)
			(*(m_pMenuItems[i].pScreen))()->HideScreen();
	
	CScrollingListScreen::HideScreen();
}

// Set this instance's parent, and all of it's children
void
CMenuScreen::SetParent(CScreen* pScreen)
{
	CScreen::SetParent(pScreen);
}

SIGNED
CMenuScreen::Message(const PegMessage &Mesg)
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
CMenuScreen::Draw()
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
CMenuScreen::ForceRedraw()
{
	DrawMenu();
	Invalidate(mReal);
	Draw();
	Presentation()->MoveFocusTree(this);
}

// Notification from the scrolling list that the line index has changed.
void
CMenuScreen::NotifyLineIndexChange(int iOldIndex, int iNewIndex)
{
	Invalidate(m_aryHighlightRects[iOldIndex]);
	Invalidate(m_aryHighlightRects[iNewIndex]);
	Draw();
}

// Notification from the scrolling list that the list has scrolled up.
void
CMenuScreen::NotifyScrollUp()
{
	DrawMenu();
	PegRect rtMenu;
	rtMenu.Set(mReal.wLeft + 30, mReal.wTop, mReal.wRight, mReal.wBottom);
	Invalidate(rtMenu);
	Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CMenuScreen::NotifyScrollDown()
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
CMenuScreen::ResetToTop()
{
	CScrollingListScreen::ResetToTop();
	DrawMenu();
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CMenuScreen::GotoPreviousMenu()
{
	KillTimer(TIMER_SCROLL_TITLE);
	Parent()->Add(m_pParent);
	Parent()->Remove(this);
	Presentation()->MoveFocusTree(m_pParent);
	ResetToTop();
}

// Creates all the icons used by the menu screen.
void
CMenuScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

    int cToDisplay;
    if (m_bMenuIsTitled)
        cToDisplay = (m_cItems+2) < DISPLAY_LINES ? (m_cItems+2) : DISPLAY_LINES;
    else
        cToDisplay = m_cItems < DISPLAY_LINES ? m_cItems : DISPLAY_LINES;

	for (int i = 0; i < cToDisplay; ++i)
	{
		ChildRect.Set(mReal.wLeft + 29, 11 * i, mReal.wLeft + 32, 9 + 11 * i);
		m_aryPrevMenuArrows[i] = new PegIcon(ChildRect, &gbPrevMenuArrowBitmap);
		m_aryPrevMenuArrows[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_aryPrevMenuArrows[i]);

		ChildRect.Set(mReal.wLeft + 34, 11 * i, mReal.wLeft + 48, 9 + 11 * i);
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
		m_arySubMenuArrows[i] = new PegIcon(ChildRect, &gbSubMenuArrowBitmap);
		m_arySubMenuArrows[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_arySubMenuArrows[i]);

		m_aryHighlightRects[i].Set(mReal.wLeft + 28, (11 * i) - 1, mReal.wRight, 9 + 11 * i);
	}
}

// Draws the menu, filling in the correct captions and showing the needed icons.
void
CMenuScreen::DrawMenu()
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

	// Add the given items to the menu.
	for (int i = 0; i < cToDisplay; ++i)
	{
		CaptionRect = m_aryCaptions[i + m_iTitleOffset]->mReal;
		// if we're at the end of the list, draw the menu caption

        m_aryIcons[i + m_iTitleOffset]->SetIcon(m_pMenuItems[i + m_iTopIndex].pBitmap);

		// if the icon is empty, then adjust the caption start
		if (m_pMenuItems[i + m_iTopIndex].pBitmap != NULL)
			CaptionRect.wLeft = mReal.wLeft + 34 + m_pMenuItems[i + m_iTopIndex].pBitmap->wWidth;
		else
			CaptionRect.wLeft = mReal.wLeft + 34;

		// if there's no sub menu arrow to display, use the space for the caption
		if (m_pMenuItems[i + m_iTopIndex].bHasSubmenu && m_iLineIndex == i)
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
		m_aryCaptions[i + m_iTitleOffset]->DataSet(LS(m_pMenuItems[i + m_iTopIndex].wCaptionSID));
	}

	// see if we need to scroll the highlighted text region
	int iTextWidth = Screen()->TextWidth(m_aryCaptions[m_iLineIndex + m_iTitleOffset]->DataGet(),m_aryCaptions[m_iLineIndex + m_iTitleOffset]->GetFont());
	if (iTextWidth > m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wRight - m_aryCaptions[m_iLineIndex + m_iTitleOffset]->mReal.wLeft)
		SetTimer(TIMER_SCROLL_TITLE, sc_iScrollStartInterval, sc_iScrollContinueInterval);
	else
		KillTimer(TIMER_SCROLL_TITLE);
}

bool
CMenuScreen::ScrollHighlightedCaption()
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
CMenuScreen::SetMenuTitle(const TCHAR* pszMenuTitle)
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

