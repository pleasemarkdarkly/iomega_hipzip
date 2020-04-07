//........................................................................................
//........................................................................................
//.. File Name: NetSettingsMenuScreen.cpp												..
//.. Date: 04/18/2002																	..
//.. Author(s): Chuck Ferring															..
//.. Description of content: implementation of CNetSettingsMenuScreen class				..
//.. Usage: used to choose static IP or DHCP											..
//.. Last Modified By: Chuck Ferring chuckf@fullplaymedia.com							..
//.. Modification date: 04/18/2002														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Inc.  										..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media.				..
//.. Contact Information: www.fullplaymedia.com											..
//........................................................................................
//........................................................................................
#include <main/ui/NetSettingsMenuScreen.h>
#include <main/ui/StaticSettingsMenuScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <io/net/Net.h>
#include <network.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_NET_SETTINGS_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_NET_SETTINGS_SCREEN );

// the global reference for this class
CNetSettingsMenuScreen* CNetSettingsMenuScreen::s_pNetSettingsMenuScreen = 0;

// This is a singleton class.
CScreen*
CNetSettingsMenuScreen::GetNetSettingsMenuScreen()
{
	if (!s_pNetSettingsMenuScreen) {
		s_pNetSettingsMenuScreen = new CNetSettingsMenuScreen(NULL);
	}
	return s_pNetSettingsMenuScreen;
}


CNetSettingsMenuScreen::CNetSettingsMenuScreen(CScreen* pParent)
  : CDynamicMenuScreen(&(CStaticSettingsMenuScreen::GetStaticSettingsMenuScreen), SID_NETWORK_SETTINGS),
    m_iCurrentNetSettingIndex(-1)
{
    SetItemCount(4);
}


CNetSettingsMenuScreen::~CNetSettingsMenuScreen()
{
}


void
CNetSettingsMenuScreen::Draw()
{
    ConnectionMode cm;
    if (GetInterfaceConfiguration("eth0", NULL, NULL, NULL, NULL, &cm))
    {
        m_iCurrentNetSettingIndex = cm;
        // Happens at startup on a config race condition
        if( m_iCurrentNetSettingIndex < 0 || m_iCurrentNetSettingIndex > 3 )
            m_iCurrentNetSettingIndex = 0;
    }
    else
        // we don't know what our current setting is, so don't select anything?
        m_iCurrentNetSettingIndex = -1;

	CDynamicMenuScreen::Draw();
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CNetSettingsMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    // get the current settings
    unsigned int ip;
    unsigned int gw;
    unsigned int sn;
    unsigned int dns;
    ConnectionMode cm, newcm;
    
    GetInterfaceConfiguration("eth0", &ip, &gw, &sn, &dns, &cm );

	switch (iMenuIndex)
	{
	case 0:
        DEBUGP( DBG_NET_SETTINGS_SCREEN, DBGLEV_INFO, "nms:use dhcp\n");
        m_iCurrentNetSettingIndex = iMenuIndex;
        newcm = DHCP_ONLY;
        break;
	case 1:
        DEBUGP( DBG_NET_SETTINGS_SCREEN, DBGLEV_INFO, "nms:use static\n");
        m_iCurrentNetSettingIndex = iMenuIndex;
        newcm = STATIC_ONLY;
        break;
	case 2:
        DEBUGP( DBG_NET_SETTINGS_SCREEN, DBGLEV_INFO, "nms:use dhcp with static fallback\n");
        m_iCurrentNetSettingIndex = iMenuIndex;
        newcm = STATIC_FALLBACK;
        break;
	case 3:
        GotoSubMenu(iMenuIndex);
        return;
	default:
        // appease compiler
        newcm = cm;
		break;
	};

    // dc- only reconfigure/initialize on a change
    if( newcm != cm )
    {
        ConfigureInterface("eth0", ip, gw, sn, dns, newcm);
        // dc- only initialize the interface if the cable is connected
        if( CheckInterfaceLinkStatus( "eth0" ) ) {
            CAlertScreen::GetInstance()->Config(this, 100);
            CAlertScreen::GetInstance()->SetTitleText(LS(SID_NETWORK_SETTINGS));
            CAlertScreen::GetInstance()->SetActionText(LS(SID_STARTING_NETWORK_SERVICES));
            Add(CAlertScreen::GetInstance());
        
            if(InitializeInterface("eth0") == false)
            {
                CAlertScreen::GetInstance()->Config(this, 500);
                CAlertScreen::GetInstance()->SetTitleText(LS(SID_NETWORK_SETTINGS));
                CAlertScreen::GetInstance()->SetActionText(LS(SID_NETWORK_SETTINGS_FAILED));
            }
        }
    }
    Draw();
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CNetSettingsMenuScreen::GotoSubMenu(int iMenuIndex)
{
	switch (iMenuIndex)
	{
	case 3:
        DEBUGP( DBG_NET_SETTINGS_SCREEN, DBGLEV_INFO, "nms:choose static settins\n");
        Parent()->Add(CStaticSettingsMenuScreen::GetStaticSettingsMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CStaticSettingsMenuScreen::GetStaticSettingsMenuScreen());
		break;
	default:
		break;
	};
}

bool
CNetSettingsMenuScreen::MenuItemSelected(int iMenuIndex)
{
    return (iMenuIndex == m_iCurrentNetSettingIndex);
}

const TCHAR* 
CNetSettingsMenuScreen::MenuItemCaption(int iMenuIndex)
{
    switch(iMenuIndex)
    {
        case 0:  return LS(SID_DHCP);
        case 1:  return LS(SID_STATIC_ONLY);
        case 2:  return LS(SID_DHCP_WITH_STATIC_BACKUP);
        case 3:  return LS(SID_EDIT_STATIC_SETTINGS);
        default: return LS(SID_NO);
    }
}

