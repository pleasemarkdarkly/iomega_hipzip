//........................................................................................
//........................................................................................
//.. File Name: PlaylistSaveScreen.h                                                 ..
//.. Date: 11/15/2001                                                                   ..
//.. Author(s): Teman Clark-Lindh                                                    ..
//.. Description of content: definition of CPlaylistSaveScreen class                 ..
//.. Usage: Browse the contents of the current playlist and do operations on it.        ..
//.. Last Modified By: Teman Clark-Lindh temancl@iobjects.com                             ..
//.. Modification date: 11/15/2001                                                      ..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.                                   ..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................
#ifndef PLAYLISTSAVESCREEN_H_
#define PLAYLISTSAVESCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector

class CPlaylistSaveScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CPlaylistSaveScreen* GetPlaylistSaveScreen();
    static void Destroy() {
		if (s_pPlaylistSaveScreen)
			delete s_pPlaylistSaveScreen;
		s_pPlaylistSaveScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

    // called to make this screen do the necessary queries to get ready to be shown
    // iCurrentTrackOffset is meant to be a way for the screen to be set up with 
    // track name in focus in relation to the current track.  if -1 is passed, then
    // the current track - 1 is displayed.  if 1 is passed, then the current track
    // + 1 is displayed, etc.
    void SetupList(int iPlaylist = 0);

protected: 

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};

    // Tell the base class what the list item captions are
	const TCHAR* MenuItemCaption(int iMenuIndex);

    // Create a new playlist
    void CreateNewPlaylist();

    // Overwrite an exsisting playlist
    void OverwritePlaylist(int iMenuIndex);

    // callback that the alert screen calls
    // we're done saving the playlist, so this callback will
    // hide this screen and put the user in the player screen
    void FinishedSavingPlaylistCB();
    static void FinishedSavingPlaylistCallback();

    // callback that the edit screen calls when
    // we're done editing the new playlist name.
    void EditNewPlaylistNameCB(bool bSave, bool bOverwrite = false);
    static void EditNewPlaylistNameCallback(bool bSave);

    static void OverwritePlaylistNameCallback(bool bOverwrite);
    
private:

    static CPlaylistSaveScreen* s_pPlaylistSaveScreen;
    CPlaylistSaveScreen();
	~CPlaylistSaveScreen();

    void BuildScreen();

    int     m_iPlaylistCount;
    int     m_iPlaylistEntryIndex;
    int     m_iPlaylistTopIndex;	// The index of the item at the top of the screen.
    int     m_iPlaylistLineIndex;	// The line number of the item currently highlighted.
	
	PlaylistRecordList  m_prlPlaylistMenuItems;
	PegString          *m_pTitleTextString;

    bool m_bSavingPlaylist;
};

#endif  // PLAYLISTSAVESCREEN_H_
