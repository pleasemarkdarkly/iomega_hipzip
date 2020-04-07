//........................................................................................
//........................................................................................
//.. File Name: DiskInfoMenuScreen.cpp														..
//.. Date: 10/01/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CDiskInfoMenuScreen class					..
//.. Usage: Controls display of the different set catagories							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 10/01/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/DiskInfoMenuScreen.h>

#include <core/playmanager/PlayManager.h>
#include <main/util/filenamestore/FileNameStore.h>
#include <fs/fat/sdapi.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <stdio.h>

// the global reference for this class
CDiskInfoMenuScreen* CDiskInfoMenuScreen::s_pDiskInfoMenuScreen = 0;

static DynamicMenuItemRec s_menuItems[] =
{
    { NULL, SID_DISKINFO_DISKFREE, false},
    { NULL, SID_DISKINFO_FILECOUNT, false},
    { NULL, SID_DISKINFO_FOLDERCOUNT, false},
};


// This is a singleton class.
CScreen*
CDiskInfoMenuScreen::GetDiskInfoMenuScreen()
{
	if (!s_pDiskInfoMenuScreen) {
		s_pDiskInfoMenuScreen = new CDiskInfoMenuScreen(NULL);
	}
	return s_pDiskInfoMenuScreen;
}


CDiskInfoMenuScreen::CDiskInfoMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_SETUP)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

	SetMenuTitle(LS(SID_DISK_INFO));
}

CDiskInfoMenuScreen::~CDiskInfoMenuScreen()
{
}

// compute the disk info from the fat layer and display it.
void 
CDiskInfoMenuScreen::Init()
{
	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));

    // copy in the item description stubs
    tstrcpy(m_pszDiskFree, LS(SID_DISKINFO_DISKFREE));
	tstrcpy(m_pszFileCount, LS(SID_DISKINFO_FILECOUNT));
	tstrcpy(m_pszFolderCount, LS(SID_DISKINFO_FOLDERCOUNT));

    char temp[MAX_DISKINFO_CHARS];

    // count files
    CFileNameStore* fns = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore();
    int nDirs, nFiles;
    fns->FileCount(&nDirs,&nFiles);
    
    // files
    sprintf(temp,": %d",nFiles);
    CharToTchar(m_pszFileCount+tstrlen(m_pszFileCount), temp);
	
    // dirs
    sprintf(temp,": %d",nDirs);
    CharToTchar(m_pszFolderCount+tstrlen(m_pszFolderCount), temp);

    // force data to screen
    Draw();

    // disk free
    UINT64 nBytesFree = pc_free64(0);
    int nMBFree = nBytesFree / (1024*1024);
    sprintf(temp,": %d MB",nMBFree);
    CharToTchar(m_pszDiskFree+tstrlen(m_pszDiskFree), temp);
	Draw();
}

bool 
CDiskInfoMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CDiskInfoMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch (iMenuIndex)
    {
        case 0:
            // disk free
            return m_pszDiskFree;
        case 1:
            // files
            return m_pszFileCount;
        case 2:
            // folders
            return m_pszFolderCount;
    }
}


PegBitmap* 
CDiskInfoMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].pBitmap;
}


