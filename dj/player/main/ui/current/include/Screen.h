//
// Screen.h: The CScreen class is an abstract base class from which more	specific screens are derived.
// danb@fullplaymedia.com 09/17/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef SCREEN_H_
#define SCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/main/DJPlayerState.h>

class CScreen : public PegWindow
{
public:
	CScreen(CScreen* pParent = NULL);
	virtual ~CScreen() = 0;

	//! Hides this screen.
	virtual void HideScreen();

	//! Sets the logical parent of this screen.
	virtual void SetParent(CScreen* pScreen = NULL);

    //! Sets the view mode for this screen.
    virtual void SetViewMode(CDJPlayerState::EUIViewMode eViewMode = CDJPlayerState::NORMAL);

protected:

	CScreen*    m_pParent;    //! A pointer to the logical parent of this screen.

    CDJPlayerState::EUIViewMode     m_eViewMode;  //! Current view mode for this screen.
};

//! declare a global function type
typedef CScreen* (*pGetScreenFunc)();

#endif  // SCREEN_H_
