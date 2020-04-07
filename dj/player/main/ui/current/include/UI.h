//........................................................................................
//........................................................................................
//.. File Name: ui.h																	..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the definition of the CUIMaster class				..
//.. Usage: The CUIMaster class is derived from the PegWindow class, and is the only	..
//..	    class that should be directly added to the presentation manager. It is		..
//..		responsible for handling basic messages from the rest of the system.		..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/17/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef UI_H_
#define UI_H_

#include <gui/peg/peg.hpp>
#include <main/ui/Screen.h>

// The full size of the screen in pixels
#define UI_X_SIZE 240
#define UI_Y_SIZE 64

// The main container window that is the parent of all other screens
class CUIMaster : public CScreen
{
public:
  CUIMaster(const PegRect& rect, WORD wStyle = FF_NONE);
  virtual ~CUIMaster();
  SIGNED Message(const PegMessage &Mesg);
};

// The main container window that is the parent of all other screens
class CRestoreUIMaster : public CScreen
{
public:
  CRestoreUIMaster(const PegRect& rect, WORD wStyle = FF_NONE, bool bUseSplashScreen = true);
  virtual ~CRestoreUIMaster();
};

#endif  // UI_H_
