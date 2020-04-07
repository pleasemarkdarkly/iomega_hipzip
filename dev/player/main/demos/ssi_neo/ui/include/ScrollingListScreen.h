//........................................................................................
//........................................................................................
//.. File Name: ScrollingListScreen.h													..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: contains the definition of the CScrollingListScreen class	..
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

#ifndef SCROLLINGLISTSCREEN_H_
#define SCROLLINGLISTSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/demos/ssi_neo/ui/Screen.h>
//#include "UI.h"

#define DISPLAY_LINES	5

class CScrollingListScreen : public CScreen
{
public:
	CScrollingListScreen(CScreen* pParent, WORD wScreenTitleSID, int iDisplayLines = DISPLAY_LINES);
	virtual ~CScrollingListScreen() = 0;

	// Returns the zero-based index of the currently highlighted item.
	// If the exit caption is highlighted, then the return value is -1.
	int GetHighlightedIndex() const;

	// Returns the number of display lines the subclass uses.
	int GetDisplayLines() const
		{ return m_cDisplayLines; }

	// Hides any visible screens.
	virtual void HideScreen();

	// returns a pointer to a string for the title of this screen in other menus
	//virtual TCHAR* GetMenuCaption() = 0;

	SIGNED Message(const PegMessage &Mesg);
	void Draw();

protected:

	// Sets the number of items in the list.
	// Called by the derived classes to inform the scrolling list how far it can scroll.
	void SetItemCount(int cItems);

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	virtual void ProcessMenuOption(int iMenuIndex) = 0;

	// Force a redraw of the menu.
	virtual void ForceRedraw() = 0;

	// Notify subclasses that the line index has changed.
	virtual void NotifyLineIndexChange(int iOldIndex, int iNewIndex)
	{ 
		Draw();
	}

	// Notify subclasses that the list has scrolled up.
	virtual void NotifyScrollUp()
	{ 
		Draw();
	}

	// Notify subclasses that the list has scrolled down.
	virtual void NotifyScrollDown()
	{ 
		Draw();
	}

	// Resets the list so that the first item is at the top of the list.
	// Called when the list is exited.
	virtual void ResetToTop();

	// Redraws the up/down arrows and exit caption based on the current line and display line parameters.
	void RedrawIcons();

	//CScreen*	m_pParent;	// A pointer to the parent menu of this screen.

	int		m_cDisplayLines;	// The number of lines displayed on the subclass's screen.
	int		m_cItems;		// The number of items in the list.
	int		m_iTopIndex;	// The index of the item at the top of the screen.
	int		m_iLineIndex;	// The line number of the item currently highlighted.

private:

	// Creates all the icons used by the menu screen.
	void BuildScreen();

	PegString*	m_pScreenTitle;
	PegIcon*	m_pScreenTop;
	PegIcon*	m_pScreenUpArrow;
	PegIcon*	m_pScreenDownArrow;

	WORD		m_wScreenTitleSID;

};

#endif  // SCROLLINGLISTSCREEN_H_
