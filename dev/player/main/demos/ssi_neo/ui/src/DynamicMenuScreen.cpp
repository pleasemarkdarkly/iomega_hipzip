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
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>

//#include "PlayScreen.h"
#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Fonts.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>

//extern CPlayScreen* g_pMainWindow;

CDynamicMenuScreen::CDynamicMenuScreen(pGetScreenFunc pGetChildScreenFunc, WORD wScreenTitleSID)
	: CScrollingListScreen(NULL, wScreenTitleSID),
	m_pGetChildScreenFunc(pGetChildScreenFunc)
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
CDynamicMenuScreen::Draw()
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
	rtMenu.Set(mReal.wLeft, 16, mReal.wRight, mReal.wBottom);
	Invalidate(rtMenu);
	Draw();
}

// Notification from the scrolling list that the list has scrolled down.
void
CDynamicMenuScreen::NotifyScrollDown()
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
CDynamicMenuScreen::ResetToTop()
{
	CScrollingListScreen::ResetToTop();
	DrawMenu();
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
CDynamicMenuScreen::DrawMenu()
{
	int cToDisplay = m_cItems < DISPLAY_LINES ? (m_cItems + 1) : DISPLAY_LINES;

	PegRect CaptionRect;
//	CaptionRect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);

	// draw each item in the menu.
	for (int i = 0; i < DISPLAY_LINES; ++i)
	{
		CaptionRect = m_aryCaptions[i]->mReal;
		// if we're at the end of the list, draw the menu caption
		if(i >= cToDisplay)
		{
			CaptionRect.wLeft = 6;
			CaptionRect.wRight = mReal.wRight - 7;
			m_aryIcons[i]->SetIcon(&gbEmptyBitmap);
			m_aryCaptions[i]->Resize(CaptionRect);
			m_aryCaptions[i]->DataSet(NULL);
			m_aryChecks[i]->SetIcon(&gbEmptyBitmap);
			m_arySubmenuArrows[i]->SetIcon(&gbEmptyBitmap);
		}
		else if((i + m_iTopIndex) == m_cItems)
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

			if (MenuItemIsChecked(i + m_iTopIndex))
				m_aryChecks[i]->SetIcon(&gbMenuSelectedDotBitmap);
			else
				m_aryChecks[i]->SetIcon(&gbEmptyBitmap);

			m_aryIcons[i]->SetIcon(MenuItemBitmap(i + m_iTopIndex));

			// if the icon is empty, then adjust the caption start
			if (MenuItemBitmap(i + m_iTopIndex) == &gbEmptyBitmap)
				CaptionRect.wLeft = 6;
			else
				CaptionRect.wLeft = 16;

			// if there's no sub menu arrow to display, use the space for the caption
			if (MenuItemHasSubMenu(i + m_iTopIndex))
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
			m_aryCaptions[i]->DataSet(MenuItemCaption(i + m_iTopIndex));
		}
	}
}

