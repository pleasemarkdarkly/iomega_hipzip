//........................................................................................
//........................................................................................
//.. File Name: SystemToolsMenuScreen.h															..
//.. Date: 09/21/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CSystemToolsMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 12/14/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef SYSTEMTOOLSMENUSCREEN_H_
#define SYSTEMTOOLSMENUSCREEN_H_

#include <main/ui/MenuScreen.h>
#include <fs/fat/sdapi.h>

class CSystemToolsMenuScreen : public CMenuScreen
{
public:
	static CScreen* GetSystemToolsMenuScreen();

    static void Destroy() {
		if (s_pSystemToolsMenuScreen)
			delete s_pSystemToolsMenuScreen;
		s_pSystemToolsMenuScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);
	// Tells the play mode menu screen that the play mode has been changed.
	//void SetPlayModeIcon(IPlaylist::PlaylistMode ePlaylistMode);
	
	static void WriteImageCallback(bool bProgram);
	void WriteImageCB(bool bProgram);

    void StartHDScan(bool bHideMenuWhenDone);

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex) {};

	void UpdateImage( const char* szSourceURL );

private:
	// A pointer to the global instance of this class.
	static CSystemToolsMenuScreen* s_pSystemToolsMenuScreen;	

	CSystemToolsMenuScreen(CScreen* pParent = NULL);
	virtual ~CSystemToolsMenuScreen();

    // Stops ripping or playback and prints the appropriate message in the message text in the alert screen.
    void StopRippingAndPlayback();

	static void HDScanDoneCallback();
	void HDScanDoneCB();

    // Progress for file copies
    static void FileProgressCallback( const char* szFilename, int iCurrent, int iTotal );
    void FileProgressCB( const char* szFilename, int iCurrent, int iTotal );
    // Progress for flash burns
    static void FlashProgressCallback( int iCurrent, int iTotal );
    void FlashProgressCB( int iCurrent, int iTotal );
    
	int m_iNewVer;
	unsigned long m_ulNewSize;
	char m_newPath[EMAXPATH];

    const char* m_szSourceURL;
    bool m_bUpdatingCDDB;
    bool m_bUpdatedCDDB;
    bool m_bDoingOnlineUpdate;

    bool m_bHideMenuWhenScanDone;

};

#endif  // SYSTEMTOOLSMENUSCREEN_H_
