//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 08/22/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the implementation of CScreen class				..
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
#include <main/demos/ssi_neo/ui/Screen.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>

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