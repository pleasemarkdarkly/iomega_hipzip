//........................................................................................
//........................................................................................
//.. File Name: ui.h																	..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the definition of the CUIMaster class				..
//.. Usage: The CUIMaster class is derived from the PegWindow class, and is the only	..
//..	    class that should be directly added to the presentation manager. It is		..
//..		responsible for handling basic messages from the rest of the system.		..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/17/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#ifndef UI_H_
#define UI_H_

#include <gui/peg/peg.hpp>
#include <main/ui/Screen.h>
#include <main/ui/Fonts.h>

// The header files for all screens
#include <main/ui/SplashScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/BrowseMenuScreen.h>
#include <main/ui/SetMenuScreen.h>
#include <main/ui/SetupMenuScreen.h>
#include <main/ui/GenreMenuScreen.h>
#include <main/ui/PlaylistMenuScreen.h>
#include <main/ui/FolderMenuScreen.h>
#include <main/ui/PlayOrderMenuScreen.h>
#include <main/ui/ToneMenuScreen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/BacklightMenuScreen.h>
#include <main/ui/PlayerInfoMenuScreen.h>
#include <main/ui/SystemMessageScreen.h>
#include <main/ui/BitrateMenuScreen.h>
#include <main/ui/InputSelectMenuScreen.h>

// The full size of the screen in pixels
#define UI_X_SIZE 128
#define UI_Y_SIZE 64

// The main container window that is the parent of all other screens
class CUIMaster : public CScreen
{
public:
  CUIMaster(const PegRect& rect, WORD wStyle = FF_NONE);
  virtual ~CUIMaster();

private:

};

#endif  // UI_H_
