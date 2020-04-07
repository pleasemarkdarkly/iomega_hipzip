//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The CScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/17/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#ifndef SCREEN_H_
#define SCREEN_H_

#include <gui/peg/peg.hpp>


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
