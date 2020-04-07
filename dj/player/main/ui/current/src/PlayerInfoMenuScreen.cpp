//
// PlayerInfoMenuScreen.cpp: implementation of CPlayerInfoMenuScreen class
// danb@fullplaymedia.com 10/01/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/PlayerInfoMenuScreen.h>

#include <main/ui/HDInfoMenuScreen.h>
#include <main/ui/AboutScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>
#include <main/main/SpaceMgr.h>
#include <main/cddb/CDDBHelper.h>
#include <_version.h>
#include <io/net/Net.h>
#include <network.h>
#include <stdio.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_PLAYER_INFO_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_PLAYER_INFO_SCREEN );

// the global reference for this class
CPlayerInfoMenuScreen* CPlayerInfoMenuScreen::s_pPlayerInfoMenuScreen = 0;

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
	: CDynamicMenuScreen(NULL, SID_PLAYER_INFO)
{
	// grab the version number info
	// todo: this needs to be done in two places for both the player and installer version info
	TCHAR pszVersionNum[STR_SIZE] = {0};
    tstrcpy(pszVersionNum, LS(SID_FIRMWARE_VERSION));
    tstrncat(pszVersionNum, LS(SID_COLON_SPACE), STR_SIZE-1);
    TCHAR tcVersion[STR_SIZE];
    CharToTcharN(tcVersion, DDO_VERSION_STR, STR_SIZE-1);
    tstrncat(pszVersionNum, tcVersion, STR_SIZE-1);
    
	tstrcpy(m_pszPlayerVersion, pszVersionNum);

    SetItemCount(6);

    CHDInfoMenuScreen::GetHDInfoMenuScreen()->SetParent(this);
    CAboutScreen::GetAboutScreen()->SetParent(this);
}

CPlayerInfoMenuScreen::~CPlayerInfoMenuScreen()
{
}


const TCHAR* 
CPlayerInfoMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if(iMenuIndex == 1) // player version number
        return m_pszPlayerVersion;
    else if(iMenuIndex == 3) // player IP address
    {
        struct in_addr ip;
        char szTempIP[STR_SIZE];
        
        GetInterfaceAddresses("eth0", (unsigned int*)&ip, NULL);

        sprintf(szTempIP, "IP: %s", inet_ntoa(ip));

        CharToTchar(m_pszPlayerIP, szTempIP);

        return m_pszPlayerIP;
    }
    else if(iMenuIndex == 4) // mac address
    {
        char szMacAddr[6];
        char szTempMAC[STR_SIZE];
        
        strcpy(szTempMAC, "MAC: ");
        GetInterfaceAddresses("eth0", NULL, &szMacAddr[0]);
        sprintf(szTempMAC, "MAC: %02x-%02x-%02x-%02x-%02x-%02x",
            szMacAddr[0],szMacAddr[1],szMacAddr[2],szMacAddr[3],szMacAddr[4],szMacAddr[5]);
        
        CharToTchar(m_pszPlayerMAC, szTempMAC);
        return m_pszPlayerMAC;
    }
    else if(iMenuIndex == 2) // cddb update info
    {
        char szTempCDDB[STR_SIZE] = "";

        unsigned short usUpdateLevel, usDataRevision;
        if (CDDBGetUpdateLevel(usUpdateLevel, usDataRevision) == SUCCESS)
        {
            sprintf(szTempCDDB, "CDDB Data Rev: %d Update Level: %d", usDataRevision, usUpdateLevel);
        }

        CharToTchar(m_pszPlayerCDDB, szTempCDDB);
        return m_pszPlayerCDDB;
    }
    else if(iMenuIndex == 0)
        return LS(SID_HD_INFO);
    else if(iMenuIndex == 5)
        return LS(SID_ABOUT_ELLIPSIS);
    else
        return LS(SID_EMPTY_STRING);
}


// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CPlayerInfoMenuScreen::GotoSubMenu(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:
        ((CHDInfoMenuScreen*)CHDInfoMenuScreen::GetHDInfoMenuScreen())->GetHDInfo();
        Parent()->Add(CHDInfoMenuScreen::GetHDInfoMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CHDInfoMenuScreen::GetHDInfoMenuScreen());
        return;
    case 5:
        Parent()->Add(CAboutScreen::GetAboutScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CAboutScreen::GetAboutScreen());
        return;
    default:
        return;
    };
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CPlayerInfoMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    GotoSubMenu(iMenuIndex);
}

