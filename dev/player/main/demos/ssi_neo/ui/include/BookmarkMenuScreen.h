//........................................................................................
//........................................................................................
//.. File Name: BookmarkMenuScreen.h													..
//.. Date: 10/16/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: Interface to class CBookmarkMenuScreen		 				..
//.. Usage: Controls function of the bookmark menu										..
//.. Last Modified By: Ed Miller	edwardm@iobjects.com								..	
//.. Modification date: 10/19/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef BOOKMARKMENUSCREEN_H_
#define BOOKMARKMENUSCREEN_H_

#include <gui/peg/peg.hpp>
#include "Bookmark.h"
#include "Playlist.h"
#include "ScrollingListScreen.h"

class CBookmarkMenuScreen : public CScrollingListScreen
{
public:
	CBookmarkMenuScreen(CScrollingListScreen* pParent);
	~CBookmarkMenuScreen();

	// Called when a bookmark is added or deleted.
	void UpdateBookmarks();

	// Hides any visible menu screens.
	void HideMenu();

	SIGNED Message(const PegMessage &Mesg);
	void Draw();

private:

	// Resets the text so that the first item is at the top of the screen.
	// Called when the list is exited.
	void ResetToTop();

	// Creates all the icons used by the song info screen.
	void BuildScreen();

	// Tries to load a bookmark and display the info in the menu.
	void LoadBookmarkFile();

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Force a redraw of the menu.
	void ForceRedraw();

	// Notification that the line index has changed.
	void NotifyLineIndexChange(int iOldIndex, int iNewIndex);

	// Notificationthat the list has scrolled up.
	void NotifyScrollUp();

	// Notification that the list has scrolled down.
	void NotifyScrollDown();

	// Draws the menu, filling in the correct captions.
	void DrawBookmarkScreen();

	PegString*	m_pTrackTitle;
	PegString*	m_pTrackTime;
	PegIcon*	m_pPlayIcon;
	PegString*	m_pPlayCaption;

	PegRect		m_rtPlayHighlight;

};


#endif	// BOOKMARKMENUSCREEN_H_
