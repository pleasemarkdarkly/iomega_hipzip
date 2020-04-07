//........................................................................................
//........................................................................................
//.. File Name: TimeMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CTimeMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef TIMEMENUSCREEN_H_
#define TIMEMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CTimeMenuScreen : public CMenuScreen
{
public:
	// an enumeration to aid indexing into the Control_Symbol[] array
	typedef enum TimeViewMode { TRACK_ELAPSED = 0, TRACK_REMAINING, ALBUM_ELAPSED, ALBUM_REMAINING };

    static CScreen* GetTimeMenuScreen();

    static void Destroy() {
		if (s_pTimeMenuScreen)
			delete s_pTimeMenuScreen;
		s_pTimeMenuScreen = 0;
    }

	// Tells the play mode menu screen that the play mode has been changed.
	void SetTimeViewMode(TimeViewMode eTimeViewMode);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};


private:

	CTimeMenuScreen(CScreen* pParent = NULL);
	virtual ~CTimeMenuScreen();

	// A pointer to the global instance of this class.
	static CTimeMenuScreen* s_pTimeMenuScreen;	


};

#endif  // TIMEMENUSCREEN_H_
