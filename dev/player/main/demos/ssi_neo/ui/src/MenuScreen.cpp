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
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/MenuScreen.h>

//#include "PlayScreen.h"
#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Fonts.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

//extern CPlayScreen* g_pMainWindow;

CMenuScreen::CMenuScreen(CScreen* pParent, WORD wScreenTitleSID, MenuItemRec* pMenuItems, int cMenuItems)
  : CScrollingListScreen(pParent, wScreenTitleSID),
	m_pMenuItems(pMenuItems)
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
		case PM_KEY:

			switch (Mesg.iData)
			{
				//case KEY_PLAY_PAUSE:
				case KEY_STOP:
				//case KEY_NEXT:
				case KEY_PREVIOUS:
					Parent()->Add(m_pParent);
					Parent()->Remove(this);
					Presentation()->MoveFocusTree(m_pParent);
					ResetToTop();
					return CScrollingListScreen::Message(Mesg);

				default:
					return CScrollingListScreen::Message(Mesg);
			}
			break;

		default:
			return CScrollingListScreen::Message(Mesg);
	}
}

void
CMenuScreen::Draw()
{

	BeginDraw();


	DrawMenu();
	CScrollingListScreen::Draw();

	// Highlight the selected line
	if (m_iLineIndex < DISPLAY_LINES)
		Screen()->InvertRect(this, m_aryHighlightRects[m_iLineIndex]);

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
	rtMenu.Set(mReal.wLeft, 16, mReal.wRight, mReal.wBottom);
	Invalidate(rtMenu);
	Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CMenuScreen::NotifyScrollDown()
{
	DrawMenu();
	PegRect rtMenu;
	rtMenu.Set(mReal.wLeft, 16, mReal.wRight, mReal.wBottom);
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

// Creates all the icons used by the menu screen.
void
CMenuScreen::BuildScreen()
{
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	int cToDisplay = m_cItems < DISPLAY_LINES ? (m_cItems + 1) : DISPLAY_LINES;

	for (int i = 0; i < cToDisplay; ++i)
	{
		ChildRect.Set(mReal.wLeft + 1, 20 + 13 * i, 4, 25 + 13 * i);
		m_aryChecks[i] = new PegIcon(ChildRect, &gbMenuSelectedDotBitmap);
		m_aryChecks[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_aryChecks[i]);

		ChildRect.Set(mReal.wLeft + 6, 19 + 13 * i, 15, 27 + 13 * i);
		m_aryIcons[i] = new PegIcon(ChildRect, &gbMIPlayModeBitmap);
		m_aryIcons[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_aryIcons[i]);

		ChildRect.Set(mReal.wLeft + 6, (16 + 13 * i) - 0, mReal.wRight - 7, 27 + 13 * i);
		m_aryCaptions[i] = new PegString(ChildRect, 0, 0, FF_NONE | TT_COPY);
		m_aryCaptions[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		m_aryCaptions[i]->SetFont(&FONT_MENUSCREENENTRY);
		Add(m_aryCaptions[i]);

		ChildRect.Set(mReal.wRight - 5, 19 + 13 * i, mReal.wRight - 1, 27 + 13 * i);
		m_arySubmenuArrows[i] = new PegIcon(ChildRect, &gbForwardArrowBitmap);
		m_arySubmenuArrows[i]->RemoveStatus(PSF_ACCEPTS_FOCUS);
		Add(m_arySubmenuArrows[i]);

		m_aryHighlightRects[i].Set(mReal.wLeft, 16 + 13 * i, mReal.wRight, 28 + 13 * i);
	}
}

// Draws the menu, filling in the correct captions and showing the needed icons.
void
CMenuScreen::DrawMenu()
{
	int cToDisplay = m_cItems < DISPLAY_LINES ? (m_cItems + 1) : DISPLAY_LINES;

	PegRect CaptionRect;
//	CaptionRect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);

	// Add the given items to the menu.
	for (int i = 0; i < cToDisplay; ++i)
	{
		CaptionRect = m_aryCaptions[i]->mReal;
		// if we're at the end of the list, draw the menu caption
		if((i + m_iTopIndex) == m_cItems)
		{
			CaptionRect.wLeft = 6;
			CaptionRect.wRight = mReal.wRight - 7;
			m_aryIcons[i]->SetIcon(&gbEmptyBitmap);
			m_aryCaptions[i]->Resize(CaptionRect);
			m_aryCaptions[i]->DataSet(LS(SID_PREVIOUS_MENU));
			m_aryChecks[i]->SetIcon(&gbEmptyBitmap);
			m_arySubmenuArrows[i]->SetIcon(&gbBackArrowBitmap);
		}
		else
		{

			if (m_pMenuItems[i + m_iTopIndex].bIsChecked)
				m_aryChecks[i]->SetIcon(&gbMenuSelectedDotBitmap);
			else
				m_aryChecks[i]->SetIcon(&gbEmptyBitmap);

			m_aryIcons[i]->SetIcon(m_pMenuItems[i + m_iTopIndex].pBitmap);

			// if the icon is empty, then adjust the caption start
			if (m_pMenuItems[i + m_iTopIndex].pBitmap == &gbEmptyBitmap)
				CaptionRect.wLeft = 6;
			else
				CaptionRect.wLeft = 16;

			// if there's no sub menu arrow to display, use the space for the caption
			if (m_pMenuItems[i + m_iTopIndex].bHasSubmenu)
			{
				m_arySubmenuArrows[i]->SetIcon(&gbForwardArrowBitmap);
				CaptionRect.wRight = mReal.wRight - 7;
			}
			else
			{
				m_arySubmenuArrows[i]->SetIcon(&gbEmptyBitmap);
				CaptionRect.wRight = mReal.wRight - 1;
			}

			m_aryCaptions[i]->Resize(CaptionRect);
			m_aryCaptions[i]->DataSet(LS(m_pMenuItems[i + m_iTopIndex].wCaptionSID));
		}
	}
}

