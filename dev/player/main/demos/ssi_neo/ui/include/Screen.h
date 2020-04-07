//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 08/22/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The CScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..	
//.. Modification date: 08/22/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef SCREEN_H_
#define SCREEN_H_

#include <gui/peg/peg.hpp>
//#include "UI.h"

//class CScreen;

class CScreen : public PegWindow
{
public:
	CScreen(CScreen* pParent);
	virtual ~CScreen() = 0;

	// Hides any visible screens.
	virtual void HideScreen();

	// Shows screens.
	virtual void ShowScreen();

	// Shows screens.
	virtual void SetParent(CScreen* pScreen);

	//SIGNED Message(const PegMessage &Mesg);
	void Draw();

protected:

	CScreen*	m_pParent;	// A pointer to the parent menu of this screen.

private:

};

// declare a global function type
typedef CScreen* (*pGetScreenFunc)();

#endif  // SCREEN_H_
