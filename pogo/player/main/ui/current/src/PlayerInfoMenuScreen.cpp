//........................................................................................
//........................................................................................
//.. File Name: PlayerInfoMenuScreen.cpp														..
//.. Date: 10/01/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CPlayerInfoMenuScreen class					..
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
#include <main/ui/PlayerInfoMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <stdio.h>

// the global reference for this class
CPlayerInfoMenuScreen* CPlayerInfoMenuScreen::s_pPlayerInfoMenuScreen = 0;

static DynamicMenuItemRec s_menuItems[] =
{
	{ NULL, SID_PLAYER_V, false},
	{ NULL, SID_PLAYER_BUILD, false},
	{ NULL, SID_INSTALLER_V, false},
};


// This is a singleton class.
CScreen*
CPlayerInfoMenuScreen::GetPlayerInfoMenuScreen()
{
	if (!s_pPlayerInfoMenuScreen) {
		s_pPlayerInfoMenuScreen = new CPlayerInfoMenuScreen(NULL);
	}
	return s_pPlayerInfoMenuScreen;
}


CPlayerInfoMenuScreen::CPlayerInfoMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_SETUP)
{
	// relocate the screen 
	PegRect newRect = m_pScreenTitle->mReal;
	newRect.wTop = mReal.wTop + 54;
	newRect.wBottom = mReal.wTop + 63;
	m_pScreenTitle->Resize(newRect);

    // grab central repository of version information
    CVersion* version = CVersion::GetInstance();

    char temp[VERSION_NUM_SIZE];

    // construct player version string
    tstrcpy(m_pszPlayerVersion, LS(SID_PLAYER_V));
    sprintf(temp," %d.%d",version->PlayerMajor(),version->PlayerMinor());
    CharToTchar( m_pszPlayerVersion + tstrlen(m_pszPlayerVersion), temp);

    // construct player build string
    tstrcpy(m_pszPlayerBuild, LS(SID_PLAYER_BUILD));
    sprintf(temp," %d",version->PlayerBuild());
    CharToTchar( m_pszPlayerBuild + tstrlen(m_pszPlayerBuild), temp);
    
	// construct bootloader version string
	tstrcpy(m_pszInstallerVersion, LS(SID_INSTALLER_V));
    sprintf(temp," %d.%d",version->BootloaderMajor(),version->BootloaderMinor());
    CharToTchar( m_pszInstallerVersion + tstrlen(m_pszInstallerVersion), temp);
    
	SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
	SetMenuTitle(LS(SID_PLAYER_INFO));
}

CPlayerInfoMenuScreen::~CPlayerInfoMenuScreen()
{
}



bool 
CPlayerInfoMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CPlayerInfoMenuScreen::MenuItemCaption(int iMenuIndex)
{
	if(iMenuIndex == 0) // player version number
		return m_pszPlayerVersion;
	else if(iMenuIndex == 1) // player build number
		return m_pszPlayerBuild;
	else
		return m_pszInstallerVersion;
}


PegBitmap* 
CPlayerInfoMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	return s_menuItems[iMenuIndex].pBitmap;
}


