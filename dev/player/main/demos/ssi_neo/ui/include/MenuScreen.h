//........................................................................................
//........................................................................................
//.. File Name: MenuScreen.h															..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: definition of class CMenuScreen			 				..
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

#ifndef MENUSCREEN_H_
#define MENUSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/demos/ssi_neo/ui/ScrollingListScreen.h>

typedef struct
{
	pGetScreenFunc	pScreen;
	PegBitmap*	pBitmap;
	WORD		wCaptionSID;
	bool		bHasSubmenu;
	bool		bIsChecked;
} MenuItemRec;

class CMenuScreen : public CScrollingListScreen
{
public:
	CMenuScreen(CScreen* pParent, WORD wScreenTitleSID, MenuItemRec* pMenuItems, int cMenuItems);
	virtual ~CMenuScreen() = 0;

	SIGNED Message(const PegMessage &Mesg);

	// Hides any visible screens.
	virtual void HideScreen();

	// Call the constructors of all the child menus
	virtual void SetParent(CScreen* pScreen);

	void Draw();

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	virtual void ProcessMenuOption(int iMenuIndex) = 0;

	// Force a redraw of the menu.
	virtual void ForceRedraw();

	// Notification from the scrolling list that the line index has changed.
	virtual void NotifyLineIndexChange(int iOldIndex, int iNewIndex);

	// Notification from the scrolling list that the list has scrolled up.
	virtual void NotifyScrollUp();

	// Notification from the scrolling list that the list has scrolled down.
	virtual void NotifyScrollDown();

	// Resets the menu so that the first item is at the top of the menu.
	// Called when the menu is exited.
	void ResetToTop();

	// Draws the menu, filling in the correct captions and showing the needed icons.
	void DrawMenu();

private:

	// Creates all the icons used by the menu screen.
	void BuildScreen();

	MenuItemRec*	m_pMenuItems;

	PegIcon*	m_aryIcons[DISPLAY_LINES];
	PegString*	m_aryCaptions[DISPLAY_LINES];
	PegIcon*	m_arySubmenuArrows[DISPLAY_LINES];
	PegIcon*	m_aryChecks[DISPLAY_LINES];

	PegRect		m_aryHighlightRects[DISPLAY_LINES];

};

#endif  // MAINMENUSCREEN_H_
