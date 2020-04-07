//........................................................................................
//........................................................................................
//.. File Name: FolderMenuScreen.h														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CFolderMenuScreen class		 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef FOLDERMENUSCREEN_H_
#define FOLDERMENUSCREEN_H_

#include <gui/peg/peg.hpp>

#include <main/ui/DynamicMenuScreen.h>
#include <util/datastructures/SimpleVector.h>
#include <util/datastructures/SimpleList.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE
#include <main/util/filenamestore/FileNameStore.h>

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
struct FolderScrollSettings
{
    int topindex;
    int lineindex;
};

typedef SimpleVector<FolderEntry*> FolderEntryVector;
typedef SimpleList<FolderScrollSettings*> FolderScrollSettingsList;

class CFolderMenuScreen : public CDynamicMenuScreen
{
public:
	// this is a singleton class
	static CScreen* GetFolderMenuScreen();
    static void Destroy() {
		if (s_pFolderMenuScreen)
			delete s_pFolderMenuScreen;
		s_pFolderMenuScreen = 0;
    }
    void Init();
    void Minimize();

protected:

	// Called when the user hits the select button.
	// Acts based upon the currently highlighted menu item.
	void ProcessMenuOption(int iMenuIndex);

	// Called when the user hits the next button.
	// Acts based upon the currently highlighted menu item.
	void GotoSubMenu(int iMenuIndex);

	// Called when the user hits the previous button.
	// Acts based upon the currently highlighted menu item.
	void GotoPreviousMenu();

    // The functions help get info about the menu items so we can draw them correctly
	// These functions must be defined by the derived class
	bool MenuItemHasSubMenu(int iMenuIndex);
	const TCHAR* MenuItemCaption(int iMenuIndex);
	PegBitmap* MenuItemBitmap(int iMenuIndex);

private:
    //FolderEntryVector m_vFolderEntries;
    int m_nFolderDepth;
    FolderScrollSettingsList m_lstParentScrollSettings;
    // maintains the full path of the current location.  parsed to highlight the proper folder on backing up out of it.
//    char m_szCurrentPath[PLAYLIST_STRING_SIZE];
//    char *m_szCurrentURL;
//    int m_nCurrentUrlAlloc;

    // the reference to the current directory
    CStoreFileNameRef* m_prfCurrDir;
    FileNodeListItr m_itrDirEntry;
    int m_nitrDirEntry;
    // the subfolder count lets us know which item is the play-all option, whether an item is a selectable file.
    int m_cSubFolders;

    void SyncItrToIndex(int nIndex);

    // reset the current path/URL to the root directory
    void ResetToRoot();
    // take a path on the harddrive, and add all playable files to the current playlist (recursively)
    void CreateRecursiveFolderPlaylist(IFileNameNode* listroot);
    // move the screen focus into a subfolder of the current folder.
    bool EnterFolder(int iFolderIndex);
    // move the screen focus up into the immediate parent folder.
    void ReturnToParentFolder();
    // see if the item is a plausible playlist entry
    bool IsPlayableExtension(char* szExtension);
    void PopulateFolderContents();


	CFolderMenuScreen();
	~CFolderMenuScreen();

	static CFolderMenuScreen* s_pFolderMenuScreen;
};

#endif  // FOLDERMENUSCREEN_H_
