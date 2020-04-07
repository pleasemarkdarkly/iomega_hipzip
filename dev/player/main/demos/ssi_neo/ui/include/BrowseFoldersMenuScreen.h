//........................................................................................
//........................................................................................
//.. File Name: BrowseFoldersMenuScreen.h														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBrowseFoldersMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef BROWSEFOLDERSMENUSCREEN_H_
#define BROWSEFOLDERSMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/demos/ssi_neo/ui/DynamicMenuScreen.h>
#include <util/datastructures/SimpleVector.h>
#include <fs/fat/sdapi.h>

class IPlaylistEntry;

struct FolderEntry
{
    char szFilename[13];
    char* szCaption;
    IPlaylistEntry* pPlaylistEntry;
    bool bSelected;
};
struct StringEntryPair
{
    char* name;
    IPlaylistEntry* entry;
};

typedef SimpleVector<FolderEntry*> FolderEntryVector;

class CBrowseFoldersMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetBrowseFoldersMenuScreen();
    static void Destroy() {
		if (s_pBrowseFoldersMenuScreen)
			delete s_pBrowseFoldersMenuScreen;
		s_pBrowseFoldersMenuScreen = 0;
    }

    void Init();
    void Minimize();

protected:
    
	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemIsChecked(int iMenuIndex);
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:
    FolderEntryVector m_vFolderEntries;
    int m_nFolderDepth;
    // maintains the full path of the current location.  parsed to highlight the proper folder on backing up out of it.
    char m_szCurrentPath[EMAXPATH];
    char *m_szCurrentURL;
    // the subfolder count lets us know which item is the play-all option, whether an item is a selectable file.
    int m_cSubFolders;

    // iterate over entries in the current dir and update the playlist if any have been selected or deselected.
    void UpdatePlaylistMembership();
    // iterate over entries in the current dir and check for membership in the current playlist.
    void CheckPlaylistMembership();
    // move the screen focus into a subfolder of the current folder.
    bool EnterFolder(int iFolderIndex);
    // move the screen focus up into the immediate parent folder.
    void ReturnToParentFolder();
    // see if the item is a plausible playlist entry
    bool IsPlayableExtension(char* szExtension);
    void PopulateFolderContents();

    CBrowseFoldersMenuScreen();
	~CBrowseFoldersMenuScreen();

	static CBrowseFoldersMenuScreen* s_pBrowseFoldersMenuScreen;
};

#endif  // BROWSEFOLDERSMENUSCREEN_H_
