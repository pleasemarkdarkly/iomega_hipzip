//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of CScreen class				..
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
#include <main/ui/Screen.h>

CScreen::CScreen(CScreen* pParent)
  : PegWindow(FF_NONE),
	m_pParent(pParent)
{
}

CScreen::~CScreen()
{
}


// Hides any visible screens.
void
CScreen::HideScreen()
{
	if (Parent())
	{
		Parent()->Remove(this);
	}
}


// Shows screens.
void
CScreen::ShowScreen()
{
	Presentation()->MoveFocusTree(this);
	Draw();
}

// Shows screens.
void
CScreen::SetParent(CScreen* pScreen)
{
	m_pParent = pScreen;
}


void
CScreen::Draw()
{
	BeginDraw();
	PegWindow::Draw();
	EndDraw();
}