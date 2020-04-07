//
// StaticSettingsMenuScreen.cpp: implementation of CStaticSettingsMenuScreen class
// Usage: Controls display of the different static network settings catagories.
// danb@fullplaymedia.com 04/29/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/StaticSettingsMenuScreen.h>

#include <main/ui/AlertScreen.h>
#include <main/ui/EditIPScreen.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <main/main/DJPlayerState.h>

#include <io/net/Net.h>
#include <network.h>
#include <stdio.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_STATIC_SETTINS_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_STATIC_SETTINS_SCREEN );

// the global reference for this class
CStaticSettingsMenuScreen* CStaticSettingsMenuScreen::s_pStaticSettingsMenuScreen = 0;

// This is a singleton class.
CScreen*
CStaticSettingsMenuScreen::GetStaticSettingsMenuScreen()
{
	if (!s_pStaticSettingsMenuScreen) {
		s_pStaticSettingsMenuScreen = new CStaticSettingsMenuScreen(NULL);
	}
	return s_pStaticSettingsMenuScreen;
}


CStaticSettingsMenuScreen::CStaticSettingsMenuScreen(CScreen* pParent)
	: CDynamicMenuScreen(NULL, SID_EDIT_STATIC_SETTINGS)
{
    SetItemCount(4);
    m_bSettingsHaveChanged = false;
}

CStaticSettingsMenuScreen::~CStaticSettingsMenuScreen()
{
	s_pStaticSettingsMenuScreen = 0;
}


SIGNED
CStaticSettingsMenuScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
            case IR_KEY_SELECT:
            case IR_KEY_EDIT:
            {
                EditItemInFocus();
                return 0;
            }
            case IR_KEY_MENU:
            {
                if( m_bSettingsHaveChanged ) {
                    InitializeInterface("eth0");
                    m_bSettingsHaveChanged = false;
                }
                break;
            }
            default:
                break;
        };
        
        default:
            break;
    }
    
    return CDynamicMenuScreen::Message(Mesg);
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CStaticSettingsMenuScreen::EditItemInFocus()
{
    DEBUGP( DBG_STATIC_SETTINS_SCREEN, DBGLEV_TRACE, "ssms:EditItemInFocus +\n");
	switch(GetHighlightedIndex())
	{
	case 0: // player IP address
    {
        struct in_addr ip;
        TCHAR szTempIP[STR_SIZE];
        GetInterfaceConfiguration("eth0", (unsigned int*)&ip, NULL, NULL, NULL, NULL);
        CEditIPScreen::GetInstance()->Config(this, EditItemInFocusCallback, CharToTchar(szTempIP, inet_ntoa(ip)));
        CEditIPScreen::GetInstance()->SetTitleText(LS(SID_EDIT_IP));
        Add(CEditIPScreen::GetInstance());
        break;
    }
    case 1: // player Subnet mask
    {
        struct in_addr sn;
        TCHAR szTempSubnet[STR_SIZE];
        GetInterfaceConfiguration("eth0", NULL, NULL, (unsigned int*)&sn, NULL, NULL);
        CEditIPScreen::GetInstance()->Config(this, EditItemInFocusCallback, CharToTchar(szTempSubnet, inet_ntoa(sn)));
        CEditIPScreen::GetInstance()->SetTitleText(LS(SID_EDIT_SUBNET));
        Add(CEditIPScreen::GetInstance());
        break;
    }
	case 2: // player Gateway address
    {
        struct in_addr gw;
        TCHAR szTempGateway[STR_SIZE];
        GetInterfaceConfiguration("eth0", NULL, (unsigned int*)&gw, NULL, NULL, NULL);
        CEditIPScreen::GetInstance()->Config(this, EditItemInFocusCallback, CharToTchar(szTempGateway, inet_ntoa(gw)));
        CEditIPScreen::GetInstance()->SetTitleText(LS(SID_EDIT_GATEWAY));
        Add(CEditIPScreen::GetInstance());
        break;
    }
    case 3: // player DNS server address
    {
        struct in_addr dns;
        TCHAR szTempDNS[STR_SIZE];
        GetInterfaceConfiguration("eth0", NULL, NULL, NULL, (unsigned int*)&dns, NULL);
        CEditIPScreen::GetInstance()->Config(this, EditItemInFocusCallback, CharToTchar(szTempDNS, inet_ntoa(dns)));
        CEditIPScreen::GetInstance()->SetTitleText(LS(SID_EDIT_DNS));
        Add(CEditIPScreen::GetInstance());
        break;
    }
	default:
		break;
	};
    DEBUGP( DBG_STATIC_SETTINS_SCREEN, DBGLEV_TRACE, "ssms:EditItemInFocus -\n");
}


const TCHAR* 
CStaticSettingsMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if(iMenuIndex == 0) // player IP address
    {
        struct in_addr ip;
        char szTempIP[STR_SIZE];
        
        GetInterfaceConfiguration("eth0", (unsigned int*)&ip, NULL, NULL, NULL, NULL);
        
        sprintf(szTempIP, "IP address: %s", inet_ntoa(ip));

        CharToTchar(m_pszPlayerIP, szTempIP);

        return m_pszPlayerIP;
    }
    if(iMenuIndex == 1) // player Subnet mask
    {
        struct in_addr sn;
        char szTempSubnet[STR_SIZE];
        
        GetInterfaceConfiguration("eth0", NULL, NULL, (unsigned int*)&sn, NULL, NULL);
        
        sprintf(szTempSubnet, "Subnet mask: %s", inet_ntoa(sn));

        CharToTchar(m_pszPlayerSubnet, szTempSubnet);

        return m_pszPlayerSubnet;
    }
    if(iMenuIndex == 2) // player Gateway address
    {
        struct in_addr gw;
        char szTempGateway[STR_SIZE];
        
        GetInterfaceConfiguration("eth0", NULL, (unsigned int*)&gw, NULL, NULL, NULL);
        
        sprintf(szTempGateway, "Default gateway: %s", inet_ntoa(gw));

        CharToTchar(m_pszPlayerGateway, szTempGateway);

        return m_pszPlayerGateway;
    }
    if(iMenuIndex == 3) // player DNS server address
    {
        struct in_addr dns;
        char szTempDNS[STR_SIZE];
        
        GetInterfaceConfiguration("eth0", NULL, NULL, NULL, (unsigned int*)&dns, NULL);
        
        sprintf(szTempDNS, "DNS server: %s", inet_ntoa(dns));

        CharToTchar(m_pszPlayerDNS, szTempDNS);

        return m_pszPlayerDNS;
    }
    else
        return LS(SID_EMPTY_STRING);
}


void
CStaticSettingsMenuScreen::EditItemInFocusCallback(bool bSave)
{
    DEBUGP( DBG_STATIC_SETTINS_SCREEN, DBGLEV_TRACE, "ssms:EditItemInFocusCallback\n");
    if (!s_pStaticSettingsMenuScreen)
        return;

    s_pStaticSettingsMenuScreen->EditItemInFocusCB(bSave);
}


void
CStaticSettingsMenuScreen::EditItemInFocusCB(bool bSave)
{
    DEBUGP( DBG_STATIC_SETTINS_SCREEN, DBGLEV_TRACE, "ssms:EditItemInFocusCB\n");
    if (bSave)
    {
        DEBUGP( DBG_STATIC_SETTINS_SCREEN, DBGLEV_INFO, "ssms:save new address\n");
        unsigned int ip;
        unsigned int gw;
        unsigned int sn;
        unsigned int dns;
        ConnectionMode cm;

        CAlertScreen::GetInstance()->Config(this, 100);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_EDIT_STATIC_SETTINGS));
        CAlertScreen::GetInstance()->SetActionText(LS(SID_SAVING_NETWORK_SETTINGS));
        Add(CAlertScreen::GetInstance());
        
        GetInterfaceConfiguration("eth0", &ip, &gw, &sn, &dns, &cm);

        unsigned int uiNewAddress = CEditIPScreen::GetInstance()->GetIPAddress();
        switch(GetHighlightedIndex())
        {
		case 0: // player IP address
            ip = uiNewAddress;
            break;
        case 1: // player Subnet mask
            sn = uiNewAddress;
            break;
		case 2: // player Gateway address
            gw = uiNewAddress;
            break;
        case 3: // player DNS server address
            dns = uiNewAddress;
            break;
		default: // bad!
			return;
        }

#if 1
        // Behave like windows and assume a 255.255.255.0 subnet, set this when
        // an ip address is entered
        if( !sn && ip && uiNewAddress ) {
            sn = 0x00ffffff;
        }
#else
        // If no subnet is entered and they just put an IP or GW in,
        //  try and determine what the subnet mask should be. This is simple
        //  8-bit oriented guessing
        if( !sn && ip && gw && dns && uiNewAddress ) {
            if( (ip & 0x00ffffff) == (gw & 0x00ffffff) ) {
                sn = 0x00ffffff;
            } else if( (ip & 0x0000ffff) == (gw & 0x0000ffff) ) {
                sn = 0x0000ffff;
            } else if( (ip & 0x000000ff) == (gw & 0x000000ff) ) {
                sn = 0x000000ff;
            }
        }
#endif
        // Only call InitializeInterface() if we are using the static settings right now
        if( cm == STATIC_ONLY || cm == STATIC_FALLBACK ) {
            m_bSettingsHaveChanged = true;
        }
        ConfigureInterface("eth0", ip, gw, sn, dns, cm);
        CDJPlayerState::GetInstance()->SaveRegistry();
    }

    Draw();
}

