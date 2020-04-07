//........................................................................................
//........................................................................................
//.. File Name: ToneMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CToneMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef TONEMENUSCREEN_H_
#define TONEMENUSCREEN_H_

#define BASS_RANGE 12
#define TREBLE_RANGE 12

#define NORMAL_BASS 6
#define NORMAL_TREBLE 6

#define CLASSICAL_BASS 6
#define CLASSICAL_TREBLE 12

#define JAZZ_BASS 12
#define JAZZ_TREBLE 6

#define ROCK_BASS 10
#define ROCK_TREBLE 10

#include <main/ui/MenuScreen.h>

typedef enum tEqualizerMode { EQ_NORMAL, EQ_CLASSICAL, EQ_JAZZ, EQ_ROCK };

class CToneMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetToneMenuScreen();

    static void Destroy() {
		if (s_pToneMenuScreen)
			delete s_pToneMenuScreen;
		s_pToneMenuScreen = 0;
    }

    void SetEqualizerMode( tEqualizerMode eMode );

	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};


private:

	CToneMenuScreen(CScreen* pParent = NULL);
	virtual ~CToneMenuScreen();

	// A pointer to the global instance of this class.
	static CToneMenuScreen* s_pToneMenuScreen;	


};

#endif  // TONEMENUSCREEN_H_
