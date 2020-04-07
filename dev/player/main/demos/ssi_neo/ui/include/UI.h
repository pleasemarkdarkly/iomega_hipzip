//........................................................................................
//........................................................................................
//.. File Name: ui.h															..
//.. Date: 12/2000																	..
//.. Author(s): Dan Bolstad															..
//.. Description of content: contains the definition of the CUIMaster class			..
//.. Usage: The CUIMaster class is derived from the PegWindow class, and is the only..
//..	    class that should be directly added to the presentation manager. It is		..
//..		responsible for handling basic messages from the rest of the system.		..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef UI_H_
#define UI_H_

#include <gui/peg/peg.hpp>

// The header files for all screens
#include <main/demos/ssi_neo/ui/PlayerScreen.h>
//#include <main/demos/ssi_neo/ui/MainMenuScreen.h>
//#include <main/demos/ssi_neo/ui/PlayModeMenuScreen.h>
//#include <main/demos/ssi_neo/ui/PlaylistInfoScreen.h>
//#include <main/demos/ssi_neo/ui/PlaylistEditMenuScreen.h>
//#include <main/demos/ssi_neo/ui/PlaylistManagerMenuScreen.h>
//#include <main/demos/ssi_neo/ui/PlaylistOptionsMenuScreen.h>
//#include <main/demos/ssi_neo/ui/EqualizerMenuScreen.h>
//#include <main/demos/ssi_neo/ui/SettingsMenuScreen.h>
//#include <main/demos/ssi_neo/ui/BacklightMenuScreen.h>
//#include <main/demos/ssi_neo/ui/LanguageMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseArtistsMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseArtistsAlbumsMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseArtistsAlbumsTitlesMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseAlbumsMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseAlbumsTitlesMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseTitlesMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseGenresMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseGenresTitlesMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseFoldersMenuScreen.h>
#include <main/demos/ssi_neo/ui/BrowseRecordingsMenuScreen.h>
//#include <main/demos/ssi_neo/ui/SongInfoScreen.h>
//#include <main/demos/ssi_neo/ui/DiskInfoScreen.h>
//#include <main/demos/ssi_neo/ui/DiskManagerMenuScreen.h>
//#include <main/demos/ssi_neo/ui/DeleteContentMenuScreen.h>
//#include <main/demos/ssi_neo/ui/PlayerInfoScreen.h>


// The full size of the screen in pixels
#define UI_X_SIZE 120
#define UI_Y_SIZE 80

// The main container window that is the parent of all other screens
class CUIMaster : public CScreen
{
public:
  CUIMaster(const PegRect& rect, WORD wStyle = FF_NONE);
  virtual ~CUIMaster();

private:

};

#endif  // UI_H_
