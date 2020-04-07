//........................................................................................
//........................................................................................
//.. File Name: Screen.h																..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of CScreen class				..
//.. Usage:The CScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/17/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include "Screen.h"

CScreen::CScreen(CScreen* pParent)
  : m_pParent(pParent)
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
  //Presentation()->MoveFocusTree(this);
	Draw();
}

// Shows screens.
void
CScreen::SetParent(CScreen* pScreen)
{
	m_pParent = pScreen;
}

void CBootScreen::GetReal(guiRect *rect)
{
  *rect = m_rect;
}

void
CScreen::Draw()
{
  
}
