//
// Screen.cpp: The CScreen class is an abstract base class from which more	specific screens are derived.
// danb@fullplaymedia.com 09/17/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/Screen.h>

CScreen::CScreen(CScreen* pParent)
  : PegWindow(FF_NONE),
	m_pParent(pParent),
    m_eViewMode(CDJPlayerState::NORMAL)
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
		Parent()->Remove(this);
}

// Sets the logical parent of this screen.
// This is separate from the pParent variable that PEG keeps for each screen.
void
CScreen::SetParent(CScreen* pScreen)
{
	m_pParent = pScreen;
}

// Sets the view mode for this screen.
void
CScreen::SetViewMode(CDJPlayerState::EUIViewMode eViewMode)
{
	m_eViewMode = eViewMode;
}
