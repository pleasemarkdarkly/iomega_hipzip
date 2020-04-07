//
// QuickBrowseMenuScreen.h: Browse the contents of the current playlist and do operations on it.
// danb@fullplaymedia.com 11/15/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef QUICKBROWSEMENUSCREEN_H_
#define QUICKBROWSEMENUSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/ui/Strings.hpp>
#include <main/ui/DynamicMenuScreen.h>
#include <content/common/QueryableContentManager.h> // ContentKeyValueVector
#include <core/playmanager/PlayManager.h>

class CQuickBrowseMenuScreen : public CDynamicMenuScreen
{
public:
    // this is a singleton class
    static CQuickBrowseMenuScreen* GetQuickBrowseMenuScreen();
    static void Destroy() {
        if (s_pQuickBrowseMenuScreen)
            delete s_pQuickBrowseMenuScreen;
        s_pQuickBrowseMenuScreen = 0;
    }
    
    SIGNED Message(const PegMessage &Mesg);

    void SetCurrentPlaylistTitle(const char* szText);
    void SetCurrentPlaylistTitle(const TCHAR* tszText);
    void SetCurrentPlaylistTitle(const char* szText, const TCHAR* tszText);
    void SetCurrentPlaylistTitle(const TCHAR* tszText1, const TCHAR* tszText2);
    void SetCurrentPlaylistAsDefault() {SetCurrentPlaylistTitle(LS(SID_CURRENT_PLAYLIST));}
    void SetCurrentPlaylistAsUnsaved() {SetCurrentPlaylistTitle(LS(SID_CURRENT_PLAYLIST));}

    TCHAR* GetCurrentPlaylistTitle() {return m_pCurrentPlaylistTextString->DataGet();}
    
    // called to make this screen do the necessary queries to get ready to be shown
    // iCurrentTrackOffset is meant to be a way for the screen to be set up with 
    // track name in focus in relation to the current track.  if -1 is passed, then
    // the current track - 1 is displayed.  if 1 is passed, then the current track
    // + 1 is displayed, etc.
    void RefreshPlaylist(int iCurrentTrackOffset = 0, bool bForceMenuSynch = false);

    // called to notify this screen that the playlist has changed and it'd be
    // smart to resynch entries.
    //    void UpdatePlaylist();
    
    // Called after an entry in the current playlist is deleted and let's this screen
    // know that it needs to synch up and redraw.
    // iPlaylistIndex is the index of the playlist entry deleted before it was deleted.
    void SynchAfterPlaylistEntryDeletion(int iPlaylistIndex);
    
protected:
    
    void SetNumberOfText(const char* szText);
    
    // Called when the user selects a track to play
    ERESULT SetSong(int iMenuIndex, bool bForceSetSong = true);
    
    // Called when the user presses record on a track
    ERESULT RecordSong(int iMenuIndex);
    
    // Called when the user hits the delete button
    void DeleteCurrentItem();
    
    // Called when the user hits the clear/new button
    void ClearPlaylist();
    
    // Show the InfoMenuScreen with a selected tracks info
    void ShowTrackInfo(int iMenuIndex);
    
    // Called when the user hits the select button.
    // Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex);
    
    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};
    
    // Called when the user hits the previous button.
    // Acts based upon the currently highlighted menu item.
    void GotoPreviousMenu() {};
    
	// Force a redraw of the menu.
	void ForceRedraw();

    // The functions help get info about the menu items so we can draw them correctly
    // These functions must be defined by the derived class
    const TCHAR* MenuItemCaption(int iMenuIndex);
    const TCHAR* MenuTitleCaption() { return NULL; };
    void NotifyScrollUp();
    void NotifyScrollDown();
    
private:
    
    static CQuickBrowseMenuScreen* s_pQuickBrowseMenuScreen;
    CQuickBrowseMenuScreen();
    ~CQuickBrowseMenuScreen();
    
    void BuildScreen();

    bool ScrollPlaylistTitle();
    void ResetPlaylistTitleScroll();
    
    int     m_iPlaylistCount;
    int     m_iPlaylistTopIndex;	// The index of the item at the top of the screen.
    int     m_iPlaylistLineIndex;	// The line number of the item currently highlighted.
    
    PegString *m_pCurrentPlaylistTextString;
    PegString *m_pNumberOfTextString;
   	PegIcon *m_pControlSymbolIcon;

    bool OnQueryingLine(int iIndex);
};

#endif  // QUICKBROWSEMENUSCREEN_H_
